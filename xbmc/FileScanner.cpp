#include "FileScanner.h"
#include "FileSystem/File.h"
#include "FileSystem/IDirectory.h"
#include "FileSystem/HDDirectory.h"
#include "FileSystem/Directory.h"
#include "Settings.h"
#include "Util.h"
#include "FileItem.h"
#include "bxrssreader.h"
#include "bxmetadata.h"
#include "bxconfiguration.h"
#include "bxutils.h"
#include "MetadataResolver.h"
#include "GUISettings.h"

#include "boxee.h"
#include "../utils/SingleLock.h"
#include "../utils/Base64.h"
#include "Application.h"
#include "MetadataResolverVideo.h"
#include "MetadataResolverMusic.h"
#include "BoxeeUtils.h"
#include "SpecialProtocol.h"
#include "lib/libBoxee/bxmetadataengine.h"
#include "GUIUserMessages.h"
#include "GUIWindowManager.h"
#include "BrowserService.h"
#include "LocalizeStrings.h"
#include "Picture.h"
#include "GUIWindowBoxeeMediaSources.h"

#include "lib/libBoxee/bxvideodatabase.h"
#include "lib/libBoxee/bxaudiodatabase.h"

// Linux includes
#ifdef _LINUX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define MAX_ADDED_SOURCE_NAME_SIZE_IN_KAI  25

using namespace XFILE;
using namespace DIRECTORY;
using namespace BOXEE;

#define FILE_SCANNER_ID "XBMC File Scanner v1.0"

CFileScanner::CFileScanner()
{
  m_MDE = NULL;
  m_pResolver = NULL;
  m_bExtendedLog = true;

  m_pPauseLock = SDL_CreateMutex();
  m_pPauseCond = SDL_CreateCond();

}

CFileScanner::~CFileScanner()
{
  Reset();

  if (m_pPauseCond)
    SDL_DestroyCond(m_pPauseCond);

  if (m_pPauseLock)
    SDL_DestroyMutex(m_pPauseLock);

  if(m_hStopEvent)
    CloseHandle(m_hStopEvent);
}

bool CFileScanner::Init()
{
  Reset();

  m_MDE = &(BOXEE::Boxee::GetInstance().GetMetadataEngine());
  m_pResolver = new CMetadataResolver();
  m_MDE->AddMetadataResolver(m_pResolver);

  // TODO: Think about moving this into better location
  m_MDE->AddAlbumResolver(new CMetadataResolverMusic());

  m_bExtendedLog = (BXConfiguration::GetInstance().GetIntParam("Boxee.FileScanner.ExtendedLog", 0) == 1);

  // black list folders:
#ifdef __APPLE__  
  CStdString homeDir = getenv("HOME");

  // apple's iMovie internal folders
  m_noScanPaths.insert(homeDir + "/Movies/iMovie Projects.localized");
  m_noScanPaths.insert(homeDir + "/Movies/iMovie Events.localized");
  m_noScanPaths.insert(homeDir + "/Movies/iMovie Projects.localized/");
  m_noScanPaths.insert(homeDir + "/Movies/iMovie Events.localized/");

  // lightroom pictures db
  m_noScanPaths.insert(homeDir + "/Pictures/Lightroom/Lightroom Catalog Previews.lrdata/");
  m_noScanPaths.insert(homeDir + "/Pictures/Lightroom/Lightroom Catalog Previews.lrdata");
#endif

  m_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  return true;
}

void CFileScanner::Start()
{
  CLog::Log(LOGINFO,"Boxee File Scanner, Started (filescanner)");
  // Start the thread
  CThread::Create(false, 512*1024);
}

void CFileScanner::Reset()
{
  if(m_MDE != NULL)
  {
    m_MDE = NULL;
  }

  if(m_pResolver != NULL)
  {
    delete m_pResolver;
    m_pResolver = NULL;
  }
}

void CFileScanner::Stop()
{
  CLog::Log(LOGINFO,"Boxee File Scanner, Stopped (filescanner)");

  if(m_bPaused)
  {
    SDL_CondBroadcast(m_pPauseCond);
  }

  m_bStop = true;

  SetEvent(m_hStopEvent);
  StopThread();
}

void CFileScanner::Pause()
{
  if (m_bPaused)
  {
    return;
  }

  // Lock the pause mutex
  SDL_LockMutex(m_pPauseLock);
  // Update the pause protected variable
  m_bPaused = true;
  CLog::Log(LOGDEBUG,"CFileScanner::Pause - after set [m_bPaused=%d] (fs)",m_bPaused);
  // Unlock the mutex back
  SDL_UnlockMutex(m_pPauseLock);
}

void CFileScanner::Resume()
{
  if (!m_bPaused)
  {
    return;
  }

  // Lock the pause mutex
  SDL_LockMutex(m_pPauseLock);
  // Update the pause protected variable
  m_bPaused = false;
  CLog::Log(LOGDEBUG,"CFileScanner::Resume - after set [m_bPaused=%d]. going to signal (fs)",m_bPaused);
  // Signal the condition variable so that the threads would resume working
  SDL_CondBroadcast(m_pPauseCond);
  // Unlock the mutex back
  SDL_UnlockMutex(m_pPauseLock);
}

void CFileScanner::CheckPause()
{
  SDL_LockMutex(m_pPauseLock);
  if (m_bPaused)
  {
    CLog::Log(LOGDEBUG,"CFileScanner::CheckPause - BEFORE wait since [m_bPaused=%d=TRUE] (fs)",m_bPaused);
    SDL_CondWait(m_pPauseCond, m_pPauseLock);
    CLog::Log(LOGDEBUG,"CFileScanner::CheckPause - AFTER wait. [m_bPaused=%d] (fs)",m_bPaused);
  }
  SDL_UnlockMutex(m_pPauseLock);
}

void CFileScanner::InterruptibleSleep(unsigned int milliseconds)
{
  ::WaitForSingleObject(m_hStopEvent, milliseconds);
}

void CFileScanner::Process()
{
#ifdef _WIN32
  ::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#endif

  InitSourcesOnStart();

  time_t now;
  int iMillisecondsToSleep = BXConfiguration::GetInstance().GetIntParam("Boxee.FileScanner.Scan.Interval", 180000);

  // let everything initialize first before we start the first scan. this is to prevent a cpu-boost on startup
  int iDelayStartInMS = BXConfiguration::GetInstance().GetIntParam("Boxee.FileScanner.Delay.Start", 60000);
  CLog::Log(LOGDEBUG,"CFileScanner::Process - wait for %d ms before start (fs)",iDelayStartInMS);
  InterruptibleSleep(iDelayStartInMS);;

  while (!m_bStop)
  {
    // Clear the set of private paths
    m_PrivatePaths.clear();

    time(&now);

    if (m_bExtendedLog)
      CLog::Log(LOGDEBUG,"CFileScanner::Process, scan round started (filescanner)");

    CheckPause();

    CLog::Log(LOGDEBUG,"CFileScanner::Process - going to scan VIDEO shares. [m_bPaused=%d][m_bStop=%d] (fs)",m_bPaused,m_bStop);
    VECSOURCES video = *g_settings.GetSourcesFromType("video");
    ScanShares(&video, "video");
    CleanMediaFolders("videoFolder", video);

    CLog::Log(LOGDEBUG,"CFileScanner::Process - going to scan MUSIC shares. [m_bPaused=%d][m_bStop=%d] (fs)",m_bPaused,m_bStop);
    VECSOURCES audio = *g_settings.GetSourcesFromType("music");
    ScanShares(&audio, "music");
    CleanMediaFolders("musicFolder", audio);

    std::set<std::pair<CStdString, CStdString> > setUserPaths;

    {
      CSingleLock lock(m_lock);
      setUserPaths = m_setUserPaths;
    }

    int counter = 0;
    CLog::Log(LOGDEBUG,"CFileScanner::Process - going to scan USER paths [UserPathsSize=%zu]. [m_bPaused=%d][m_bStop=%d] (fs)",setUserPaths.size(),m_bPaused,m_bStop);

    std::set<std::pair<CStdString, CStdString> >::iterator it = setUserPaths.begin();
    while (it != setUserPaths.end() && !m_bStop)
    {
      counter++;
      CStdString strPath = (*it).first;
      CStdString strType = (*it).second;

      CLog::Log(LOGDEBUG,"CFileScanner::Process - [%d/%zu] - handle [path=%s][type=%s]. [m_bPaused=%d][m_bStop=%d] (fs)",counter,setUserPaths.size(),strPath.c_str(),strType.c_str(),m_bPaused,m_bStop);

      // scan path can be share or directory
      VECSOURCES shares = *g_settings.GetSourcesFromType(strType);
      VECSOURCES::iterator shareIt = shares.begin();
      CMediaSource* pShare = NULL;
      while (shareIt != shares.end())
      {
        if (_P(shareIt->strPath) == strPath)
        {
          pShare = &(*shareIt);
          break;
        }
        shareIt++;
      }

      if (pShare)
      {
        ScanShare(pShare, strType);
      }
      else
      {
        ScanMediaFolder(strPath, strType, strPath);
        UpdateScanLabel(g_localizeStrings.Get(53160));
      }

      // Notify using a message that the path has been scanned
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_FILE_SCANNER_UPDATE);
      msg.SetStringParam(strPath);
      g_windowManager.SendThreadMessage(msg);

      it++;
    }

    {
      CSingleLock lock(m_lock);
      m_setUserPaths.clear();
    }

    time_t then = time(NULL);
    if (m_bExtendedLog)
      CLog::Log(LOGDEBUG,"CFileScanner::Process, scan round finished, time = %lu (filescanner)", then - now);

    do
    {
      CLog::Log(LOGDEBUG,"CFileScanner::Process - BEFORE sleep for [%dms]. [m_bPaused=%d][m_bStop=%d] (fs)",iMillisecondsToSleep,m_bPaused,m_bStop);
      InterruptibleSleep(iMillisecondsToSleep);
      CLog::Log(LOGDEBUG,"CFileScanner::Process - AFTER sleep for [%dms]. [m_bPaused=%d][m_bStop=%d] (fs)",iMillisecondsToSleep,m_bPaused,m_bStop);
    }
    while (g_application.IsPlayingVideo() && !m_bStop);
  } // while

  CLog::Log(LOGDEBUG,"Boxee File Scanner, Exiting process loop (filescanner)");
}

void CFileScanner::ScanShares(VECSOURCES * pVecShares, const CStdString& strShareType)
{
  if (!pVecShares)
  {
    return;
  }

  CMediaSource* pShare = NULL;
  for (IVECSOURCES it = pVecShares->begin(); !m_bStop && it != pVecShares->end() && !m_bStop; it++)
  {
    pShare = &(*it);
    // Initialize the set of  private paths
    if (pShare->m_iScanType == CMediaSource::SCAN_TYPE_PRIVATE)
    {
      for (unsigned int i = 0; !m_bStop && i < pShare->vecPaths.size() && !m_bStop; i++)
      {
        m_PrivatePaths.insert(pShare->vecPaths[i]);
      }
    }
  }

  for (IVECSOURCES it = pVecShares->begin(); !m_bStop && it != pVecShares->end() && !m_bStop; it++)
  {
    pShare = &(*it);
    CLog::Log(LOGDEBUG,"CFileScanner::ScanShares - handle [name=%s][path=%s][type=%d]. [m_bPaused=%d][m_bStop=%d] (fs)",pShare->strName.c_str(),pShare->strPath.c_str(),pShare->m_iScanType,m_bPaused,m_bStop);

    if (ShouldScan(pShare))
    {
      CLog::Log(LOGDEBUG,"CFileScanner::ScanShares - going to scan [name=%s][path=%s][type=%d]. [m_bPaused=%d][m_bStop=%d] (fs)",pShare->strName.c_str(),pShare->strPath.c_str(),pShare->m_iScanType,m_bPaused,m_bStop);
      ScanShare(pShare, strShareType);
    }
  }
}

bool CFileScanner::ShouldScan(const CMediaSource* pShare)
{
  // Get last scan time from database
  time_t iLastScanned;
  if (!BOXEE::Boxee::GetInstance().GetMetadataEngine().GetScanTime(pShare->strName, pShare->strPath, pShare->m_type, iLastScanned))
  {
    iLastScanned = 0;
    CLog::Log(LOGWARNING,"CFileScanner::ShouldScan - FAILED to read LastScanTime of [name=%s][path=%s][type=%d]. set [iLastScanned=%lu=0]. [m_bPaused=%d][m_bStop=%d] (fs)",pShare->strName.c_str(),pShare->strPath.c_str(),pShare->m_iScanType,iLastScanned,m_bPaused,m_bStop);
  }

  // in case that the share is marked as being resolved we assume that the program was shutdown during
  // the resolving process, in this case we will treat is as time 0.
  if (iLastScanned == SHARE_TIMESTAMP_RESOLVING)
  {
    iLastScanned = SHARE_TIMESTAMP_NOT_SCANNED;
    CLog::Log(LOGDEBUG,"CFileScanner::ShouldScan - since [LastScanTime=%d] for [name=%s][path=%s][type=%d] it was set to [iLastScanned=%lu=0]. [m_bPaused=%d][m_bStop=%d] (fs)",SHARE_TIMESTAMP_RESOLVING,pShare->strName.c_str(),pShare->strPath.c_str(),pShare->m_iScanType,iLastScanned,m_bPaused,m_bStop);
  }

  // Check if the share should be scanned now according to its scan type
  time_t now = time(NULL);
  CLog::Log(LOGDEBUG,"CFileScanner::ShouldScan - going to check ShouldScan for [name=%s][path=%s][type=%d][iLastScanned=%lu][now=%lu][diff=%lu]. [m_bPaused=%d][m_bStop=%d] (fs)",pShare->strName.c_str(),pShare->strPath.c_str(),pShare->m_iScanType,iLastScanned,now,(now-iLastScanned),m_bPaused,m_bStop);

#ifndef HAS_EMBEDDED
  if ((pShare->m_iScanType == CMediaSource::SCAN_TYPE_MONITORED && (now - iLastScanned) > 60 * 5) ||
      (pShare->m_iScanType == CMediaSource::SCAN_TYPE_DAILY && (now - iLastScanned) > 60 * 60 * 24) ||
      (pShare->m_iScanType == CMediaSource::SCAN_TYPE_HOURLY && (now - iLastScanned) > 60 * 60 * 1) ||
      (pShare->m_iScanType == CMediaSource::SCAN_TYPE_ONCE && iLastScanned == 0))
#else
  if (((pShare->m_iScanType == CMediaSource::SCAN_TYPE_HOURLY || pShare->m_iScanType == CMediaSource::SCAN_TYPE_MONITORED) && (now - iLastScanned) > 60 * 60 * 1) ||
      (pShare->m_iScanType == CMediaSource::SCAN_TYPE_DAILY && (now - iLastScanned) > 60 * 60 * 24) ||
      (pShare->m_iScanType == CMediaSource::SCAN_TYPE_ONCE && iLastScanned == 0))
#endif
  {
    CLog::Log(LOGDEBUG,"CFileScanner::ShouldScan - for [name=%s][path=%s][type=%d] return [ShouldScan=TRUE]. [m_bPaused=%d][m_bStop=%d] (fs)",pShare->strName.c_str(),pShare->strPath.c_str(),pShare->m_iScanType,m_bPaused,m_bStop);
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG,"CFileScanner::ShouldScan - for [name=%s][path=%s][type=%d] return [ShouldScan=FALSE]. [m_bPaused=%d][m_bStop=%d] (fs)",pShare->strName.c_str(),pShare->strPath.c_str(),pShare->m_iScanType,m_bPaused,m_bStop);
    return false;
  }

  CLog::Log(LOGERROR,"CFileScanner::ShouldScan - FAILED to check for [name=%s][path=%s][type=%d]. return [ShouldScan=TRUE]. [m_bPaused=%d][m_bStop=%d] (fs)",pShare->strName.c_str(),pShare->strPath.c_str(),pShare->m_iScanType,m_bPaused,m_bStop);
  return false;
}

void CFileScanner::ScanShare(CMediaSource* pShare, const CStdString& strShareType)
{
  if (!IsScannable(pShare->strPath))
  {
    CLog::Log(LOGDEBUG,"CFileScanner::ScanShare - for [name=%s][path=%s][type=%d] got [IsScannable=FALSE] -> Exit. [m_bPaused=%d][m_bStop=%d] (fs)",pShare->strName.c_str(),pShare->strPath.c_str(),pShare->m_iScanType,m_bPaused,m_bStop);
    return;
  }

  CheckPause();

  if (m_bExtendedLog)
    CLog::Log(LOGDEBUG,"CFileScanner::ScanShare, Scanning source, name = %s, path = %s (filescanner)", pShare->strName.c_str(), _P(pShare->strPath).c_str());

  // first check if the path is still at the sources list
  std::vector<CMediaSource> vecSources;
  if (!g_settings.GetSourcesFromPath(pShare->strPath, strShareType,vecSources ))
  {
    CLog::Log(LOGDEBUG,"CFileScanner::ScanShare - for [name=%s][path=%s][type=%d] got [GetSourcesFromPath=FALSE] -> Exit. [m_bPaused=%d][m_bStop=%d] (fs)",pShare->strName.c_str(),pShare->strPath.c_str(),pShare->m_iScanType,m_bPaused,m_bStop);
    return;
  }
  {
    CSingleLock lock(m_shareLock);
    m_currenlyScannedShare = pShare->strPath;
    m_currentShareValid = true;
  }

  time_t iLastScanned;
  BOXEE::Boxee::GetInstance().GetMetadataEngine().GetScanTime(pShare->strName, pShare->strPath, pShare->m_type, iLastScanned);

  if (iLastScanned == 0)
  {
    // this is the first scan of share
    CFileScanner::ShowSourcesStatusKaiDialog(51046, pShare->strName);
  }

  if (!BOXEE::Boxee::GetInstance().GetMetadataEngine().UpdateScanTime(pShare->strName, pShare->strPath, pShare->m_type, SHARE_TIMESTAMP_RESOLVING))
  {
    CLog::Log(LOGERROR,"CFileScanner::ScanShare - FAILED to update iLastScanned from [%lu] to [%d] for [name=%s][path=%s][type=%d] before scan -> Exit. [m_bPaused=%d][m_bStop=%d] (fs)",iLastScanned,SHARE_TIMESTAMP_RESOLVING,pShare->strName.c_str(),pShare->strPath.c_str(),pShare->m_iScanType,m_bPaused,m_bStop);
    return;
  }

  CLog::Log(LOGDEBUG,"CFileScanner::ScanShare - BEGIN - after update iLastScanned from [%lu] to [%d] for [name=%s][path=%s][type=%d][pathsVecSize=%zu]. [m_bPaused=%d][m_bStop=%d] (fs)",iLastScanned,SHARE_TIMESTAMP_RESOLVING,pShare->strName.c_str(),pShare->strPath.c_str(),pShare->m_iScanType,pShare->vecPaths.size(),m_bPaused,m_bStop);

  // Go over all items in the share
  for (size_t i = 0; i < pShare->vecPaths.size() && !m_bStop; i++)
  {
    CStdString strPath = _P(pShare->vecPaths[i]);
    ScanMediaFolder(strPath, strShareType, strPath);
    UpdateScanLabel(g_localizeStrings.Get(53160));
  }

  if (iLastScanned == 0)
  {
    // this is the first scan of share
    CFileScanner::ShowSourcesStatusKaiDialog(51047, pShare->strName, pShare->strPath, pShare->m_type);
  }

  {
    CSingleLock lock(m_shareLock);
    m_currenlyScannedShare = "";
    m_currentShareValid = true;
  }

  time_t now = time(NULL);
  if (!BOXEE::Boxee::GetInstance().GetMetadataEngine().UpdateScanTime(pShare->strName, pShare->strPath, pShare->m_type, now))
  {
    CLog::Log(LOGERROR,"CFileScanner::ScanShare - FAILED to update iLastScanned to [%lu] for [name=%s][path=%s][type=%d] after scan -> Exit. [m_bPaused=%d][m_bStop=%d] (fs)",now,pShare->strName.c_str(),pShare->strPath.c_str(),pShare->m_iScanType,m_bPaused,m_bStop);
  }
  else
  {
    CLog::Log(LOGDEBUG,"CFileScanner::ScanShare - END - after update [iLastScanned=%lu] for [name=%s][path=%s][type=%d]. [m_bPaused=%d][m_bStop=%d] (fs)",now,pShare->strName.c_str(),pShare->strPath.c_str(),pShare->m_iScanType,m_bPaused,m_bStop);
  }
}

//
//  Synchronize function will sync the file system with two different tables at the database
//  - video_files||audio_files - the fileScanner wont add files to those table, just can delete from it.
//         the resolver is the only thread that add files to those tables.
//  - unresolved_video_files||unresolved_audio_files - delete and add files from and to the tables.
void CFileScanner::SynchronizeFolderAndDatabase(const CStdString& strPath, const CStdString& strShareType,  const CStdString& strSharePath, CFileItemList &folderItems)
{

  CLog::Log(LOGDEBUG,"CFileScanner::SynchronizeFolderAndDatabase, sync folder %s  with database (filescanner)", strPath.c_str());

  int iFolderId = 0;
  bool  bMarkAsNewFolder = false;
  bool  updateFolderStatus = false;
  std::map<std::string, BOXEE::BXMetadata*> dbResolvedItems;
  std::map<std::string, BOXEE::BXMetadata*>::iterator db_resolved_it;

  std::map<std::string, BOXEE::BXMetadata*> dbUnResolvedItems;
  std::map<std::string, BOXEE::BXMetadata*>::iterator db_unresolved_it;

  // if the folder doesnt exist then we should create a new db folder
  CStdString folderType = (strShareType == "music") ? "musicFolder" : "videoFolder";
  iFolderId = m_MDE->GetMediaFolderId(strPath.c_str(), folderType.c_str());

  if (iFolderId == 0)
  {
    iFolderId = m_MDE->AddMediaFolder(strPath.c_str(), folderType.c_str(), BoxeeUtils::GetModTime(strPath));
    bMarkAsNewFolder = true;
  }

  CLog::Log(LOGDEBUG,"CFileScanner::SynchronizeFolderAndDatabase, folder %s id %d (filescanner)",strPath.c_str(), iFolderId);

  if (strShareType == "music")
  {
    m_MDE->GetAudiosByFolder(dbResolvedItems, strPath.c_str());
    m_MDE->GetUnresolvedAudioFilesByFolder(dbUnResolvedItems, iFolderId);
  }
  else if (strShareType == "video")
  {
    m_MDE->GetVideosByFolder(dbResolvedItems, strPath.c_str(), true );
    m_MDE->GetUnresolvedVideoFilesByFolder(dbUnResolvedItems, iFolderId);
  }
  else
  {
    CLog::Log(LOGDEBUG,"CFileScanner::SynchronizeFolderAndDatabase, not music or video share - skip (filescanner)");
    return;
  }

  // Go over all folder items
  int i =0;
  while (m_currentShareValid && i < folderItems.Size() &&  (!dbResolvedItems.empty() || !dbUnResolvedItems.empty()))
  {
    db_resolved_it = dbResolvedItems.find(folderItems[i]->m_strPath);
    db_unresolved_it = dbUnResolvedItems.find(folderItems[i]->m_strPath);
    bool bRemoveFromFolderItems = false;

    // the item was found in the both resolved_filles table and folder,
    // so we shouldn't add or remove it from the data base
    // we will only modify the ResolvedItems since  we can just delete
    // from those tables and not add to it
    if (db_resolved_it != dbResolvedItems.end())
    {
      // check also if the video file still fits for size limitation then
      if ((strShareType != "video")  || folderItems[i]->m_bIsFolder || folderItems[i]->GetSize() >=  g_guiSettings.GetInt("myvideos.videosize") * 1024 * 1024)
      {
        delete db_resolved_it->second;
        dbResolvedItems.erase(db_resolved_it);
      }
      else
      {
        CLog::Log(LOGDEBUG,"CFileScanner::SynchronizeFolderAndDatabase file %s doesn't fit size limitation %lld - remove from the data base (filescanner)",folderItems[i]->m_strPath.c_str(),folderItems[i]->GetSize());
      }
      bRemoveFromFolderItems = true;
    }
    else
    {
      CLog::Log(LOGDEBUG,"CFileScanner::SynchronizeFolderAndDatabase, file %s wasn't found in video_files  (filescanner)", folderItems[i]->m_strPath.c_str());
    }

    // the item was found in the both data base and folder,
    // so we shouldn't add or remove it from the data base
    if (db_unresolved_it != dbUnResolvedItems.end())
    {
      // check also if the file still fits for size limitation then
      if  ((strShareType != "video")  || folderItems[i]->m_bIsFolder || folderItems[i]->GetSize() >=  g_guiSettings.GetInt("myvideos.videosize") * 1024 * 1024)
      {
        delete db_unresolved_it->second;
        dbUnResolvedItems.erase(db_unresolved_it);
      }
      else
      {
        CLog::Log(LOGDEBUG,"CFileScanner::SynchronizeFolderAndDatabase  file %s doesn't fit size limitation - remove from the data base (filescanner)",folderItems[i]->m_strPath.c_str());
      }
      bRemoveFromFolderItems = true;
    }
    else
    {
      CLog::Log(LOGDEBUG,"CFileScanner::SynchronizeFolderAndDatabase, file %s wasn't found in unresolved_video_files  (filescanner)", folderItems[i]->m_strPath.c_str());
    }

    if (bRemoveFromFolderItems)
      folderItems.Remove(i);
    else
      i++;
  }

  // after this phase we have too lists:
  // dbResolvedlist   - items that exist only in the resolved table and need to be deleted
  // dbUnResolvedlist - items that exist only in the unresolved tables and need to be deleted
  // folderlist - new items that exist only in the folder and need to be add to the unresolved_folder
  for (i = 0 ; m_currentShareValid && i < folderItems.Size(); i++ )
  {
    iFolderId = m_MDE->GetMediaFolderId(strPath.c_str(), folderType.c_str());

    updateFolderStatus = true;

    if (strShareType == "video")
    {
      if (folderItems[i]->m_bIsFolder || folderItems[i]->GetSize() >=  g_guiSettings.GetInt("myvideos.videosize") * 1024 * 1024)
      {
        CLog::Log(LOGDEBUG,"CFileScanner::SynchronizeFolderAndDatabase, Added %s %s to db (filescanner)",(folderItems[i]->m_bIsFolder)?"folder":"file", folderItems[i]->m_strPath.c_str());
        m_MDE->AddVideoFile(folderItems[i]->m_strPath, strSharePath, iFolderId, folderItems[i]->m_dwSize);
      }
      else
      {
        CLog::Log(LOGDEBUG,"CFileScanner::SynchronizeFolderAndDatabase, file %s size is %lld smaller then limitation (filescanner)", folderItems[i]->m_strPath.c_str(), folderItems[i]->GetSize());
      }
    }
    else
    {
      CLog::Log(LOGDEBUG,"CFileScanner::SynchronizeFolderAndDatabase, Add file %s to db (filescanner)", folderItems[i]->m_strPath.c_str());
      m_MDE->AddAudioFile(folderItems[i]->m_strPath, strSharePath, iFolderId, folderItems[i]->m_dwSize);
    }
  }

  db_resolved_it = dbResolvedItems.begin();
  while (m_currentShareValid && db_resolved_it != dbResolvedItems.end())
  {
    CLog::Log(LOGDEBUG,"CFileScanner::SynchronizeFolderAndDatabase, delete file %s from resolved table (filescanner)", db_resolved_it->first.c_str());
    updateFolderStatus = true;
    if (strShareType == "video")
      m_MDE->RemoveVideoByPath(db_resolved_it->first);
    else
      m_MDE->RemoveAudioByPath(db_resolved_it->first);
    ++db_resolved_it;
  }


  db_unresolved_it = dbUnResolvedItems.begin();
  while (m_currentShareValid && db_unresolved_it != dbUnResolvedItems.end())
  {
    CLog::Log(LOGDEBUG,"CFileScanner::SynchronizeFolderAndDatabase, delete file %s from unresolved table (filescanner)", db_unresolved_it->first.c_str());
    updateFolderStatus = true;
    if (strShareType == "video")
      m_MDE->RemoveUnresolvedVideoByPath(db_unresolved_it->first);
    else
      m_MDE->RemoveUnresolvedAudioByPath(db_unresolved_it->first);
    ++db_unresolved_it;
  }

  if (updateFolderStatus || bMarkAsNewFolder)
  {
    m_MDE->MarkFolderNew(strPath.c_str());
  }

}
void CFileScanner::ScanMediaFolder(CStdString& strPath, const CStdString& strShareType, const CStdString& strSharePath)
{
  bool isMediaFolderBeingResolved = m_MDE->IsMediaFolderBeingResolved(strPath);
  if (isMediaFolderBeingResolved)
  {
    CLog::Log(LOGDEBUG,"CFileScanner::ScanMediaFolder - NOT scanning since [isMediaFolderBeingResolved=%d]. [path=%s][shareType=%s][sharePath=%s][m_bPaused=%d][m_bStop=%d] (fs)",isMediaFolderBeingResolved,strPath.c_str(),strShareType.c_str(),strSharePath.c_str(),m_bPaused,m_bStop);
    return;
  }

  UpdateScanLabel(strPath);

  bool isScannable = IsScannable(strPath);
  if (m_bStop || !isScannable)
  {
    CLog::Log(LOGDEBUG,"CFileScanner::ScanMediaFolder - NOT scanning since [m_bStop=%d][isScannable=%d]. [path=%s][shareType=%s][sharePath=%s][m_bPaused=%d][m_bStop=%d] (fs)",m_bStop,isScannable,strPath.c_str(),strShareType.c_str(),strSharePath.c_str(),m_bPaused,m_bStop);
    return;
  }

  CheckPause();

  // first translate the media folder
  strPath = _P(strPath);

  if (m_bExtendedLog)
  {
    CLog::Log(LOGDEBUG,"CFileScanner::ScanMediaFolder, scan folder  %s  (filescanner)", strPath.c_str());
  }

  CleanChildFolders(strSharePath, strPath);

  CFileItemPtr pItem;

  // Retrieve the list of all items from the path
  CFileItemList items, dirItems, audioItems, videoItems ;

  if (!CDirectory::GetDirectory(strPath, items, "", false))
  {
    return;
  }

  std::vector<CStdString> playableFolders;
  // Go over all items in the folder - split it into
  // directories, audio files and movie files
  for (int i=0; !m_bStop && m_currentShareValid && i<items.Size(); i++)
  {
    pItem = items[i];

    if(strShareType == "video" && pItem->m_bIsFolder && !pItem->IsRAR() && !pItem->IsZIP() && (CUtil::IsBlurayFolder(pItem->m_strPath) || CUtil::IsDVDFolder(pItem->m_strPath)))
    {
      //if its a playable folder, we need it to be in videoItems since we pass it later to sync with the db
      CLog::Log(LOGDEBUG,"CFileScanner::ScanMediaFolder - PLAYABLE - [path=%s]. [m_bPaused=%d][m_bStop=%d] (fs)",pItem->m_strPath.c_str(),m_bPaused,m_bStop);
      videoItems.Add(pItem);

      //we treat playable folders as video files, but we add them to the media_folders table in the db (hack)
      //we do that because the resolver reads the items it should resolve from the media_folders
      int iFolderId = m_MDE->GetMediaFolderId(pItem->m_strPath.c_str(), "videoFolder");

      if (iFolderId == 0)
      {
        // need to add it when we synchronize
        playableFolders.push_back(pItem->m_strPath);
      }

    }
    else if ((pItem->m_bIsFolder || pItem->IsRAR() || pItem->IsZIP()) && (!pItem->IsPlayList()))
    {
      CLog::Log(LOGDEBUG,"CFileScanner::ScanMediaFolder - DIR - [path=%s]. [m_bPaused=%d][m_bStop=%d] (fs)",pItem->m_strPath.c_str(),m_bPaused,m_bStop);
      dirItems.Add(pItem);
    }
    else if ((strShareType == "music") &&
        (pItem->IsAudio() && !pItem->IsRAR() && !pItem->IsZIP() && !pItem->IsPlayList() ))
    {
      CLog::Log(LOGDEBUG,"CFileScanner::ScanMediaFolder - MUSIC - [path=%s]. [m_bPaused=%d][m_bStop=%d] (fs)",pItem->m_strPath.c_str(),m_bPaused,m_bStop);
      audioItems.Add(pItem);
    }
    else if ((strShareType == "video") &&
        (pItem->IsVideo() && !pItem->IsRAR() && !pItem->IsZIP() && !pItem->IsPlayList() ))
    {
      CLog::Log(LOGDEBUG,"CFileScanner::ScanMediaFolder - VIDEO - [path=%s]. [m_bPaused=%d][m_bStop=%d] (fs)",pItem->m_strPath.c_str(),m_bPaused,m_bStop);
      videoItems.Add(pItem);
    }
    else
    {
      CLog::Log(LOGDEBUG,"CFileScanner::ScanMediaFolder, file %s is not directory, audio or video - skip (filescanner)",pItem->m_strPath.c_str());
    }
  }

  // recursive call to all sub directories
  for (int i=0; !m_bStop && m_currentShareValid && i<dirItems.Size(); i++)
  {
    CLog::Log(LOGDEBUG,"CFileScanner::ScanMediaFolder, dive into subdirectry  %s  (filescanner)", dirItems[i]->m_strPath.c_str());
    ScanMediaFolder(dirItems[i]->m_strPath, strShareType, strSharePath);
  }

  // if it is a music share, and the folder contains audio files sync it with the database
  if ((strShareType == "music"))
  {
    CLog::Log(LOGDEBUG,"CFileScanner::ScanMediaFolder, synchronized music folder  %s  (filescanner)", strPath.c_str());
    SynchronizeFolderAndDatabase(strPath, "music", strSharePath, audioItems);
  }
  // if it is a audio share, and the folder contains video files sync it with the database
  if ((strShareType == "video"))
  {
    CLog::Log(LOGDEBUG,"CFileScanner::ScanMediaFolder, synchronized video folder  %s  (filescanner)", strPath.c_str());
    SynchronizeFolderAndDatabase(strPath, "video", strSharePath, videoItems);

    for (std::vector<CStdString>::iterator it = playableFolders.begin(); it != playableFolders.end() ; it++)
    {
      CStdString playableFolderPath = (*it);
      m_MDE->AddMediaFolder(playableFolderPath, "videoFolder", BoxeeUtils::GetModTime(playableFolderPath));
      m_MDE->MarkFolderNew(playableFolderPath);
    }
  }

  Sleep(1000);
}

void CFileScanner::AddUserPath(const CStdString& strPath)
{
  std::vector<CStdString> vecTypes;
  g_application.GetBoxeeFileScanner().GetFolderStatus(strPath, vecTypes);

  for (int i = 0; i < (int)vecTypes.size(); i++)
  {
    CLog::Log(LOGINFO,"CFileScanner::AddUserPath, scan user path %s of type %s (checkpath)", strPath.c_str(), vecTypes[i].c_str());
    CSingleLock lock(m_lock);
    m_setUserPaths.insert(make_pair(_P(strPath), vecTypes[i]));
  }
}

bool CFileScanner::IsScanning(const CStdString& _strPath)
{
  std::set<std::pair<CStdString, CStdString> > setUserPaths;

  {
    CSingleLock lock(m_lock);
    setUserPaths = m_setUserPaths;
  }

  // in m_setUserPaths paths are insert with _P
  CStdString strPath = _P(_strPath);

  std::set<std::pair<CStdString, CStdString> >::iterator it = setUserPaths.begin();
  while (it != setUserPaths.end())
  {
    CStdString strTempPath = it->first;

    if (strTempPath == strPath)
    {
      return true;
    }

    if (strPath.Find(strTempPath) != -1)
    {
      return true;
    }

    it++;
  }

  return false;
}

bool CFileScanner::PathInPrivateShare(const CStdString& strPath)
{
  CStdString folderPath = strPath;
  CUtil::AddSlashAtEnd(folderPath);
  std::set<CStdString>::iterator it = m_PrivatePaths.begin();
  for (; it != m_PrivatePaths.end(); it++)
  {
    CStdString sharePath = *it;
    CUtil::AddSlashAtEnd(sharePath);
    std::string::size_type startPos = folderPath.find(sharePath, 0);
    if (startPos == 0)
    {
      return true;
    }
  }
  return false;
}

bool CFileScanner::IsScannable(const CStdString& strPath)
{
  // check blacklist
  if (m_noScanPaths.find(strPath) != m_noScanPaths.end())
    return false;

  // Check if this is a sample folder and discard it
  if (CMetadataResolverVideo::IsSample(BXUtils::GetFolderName(strPath)))
  {
    return false;
  }

  CURI url(strPath);
  CStdString strProtocol = url.GetProtocol();

  // We only handle directories on the hard drive
  if (strProtocol.size() != 0)
  {
    if (strProtocol == "smb" || strProtocol == "upnp" || strProtocol == "nfs" || strProtocol == "afp" || strProtocol == "bms")
    {
      if (!g_application.IsPathAvailable(strPath, false))
      {
        return false;
      }
    }
    else if (strProtocol == "file")
    {
      // local path, always available
    }
    else if (strProtocol == "special")
    {
      // local path, always available
    }
    else if (strProtocol == "rar" || strProtocol == "zip")
    {
      if (!g_application.IsPathAvailable(strPath, true))
      {
        return false;
      }
    }
    else
    {
      return false;
    }
  }
  else
  {
    // local file
    if (!g_application.IsPathAvailable(strPath, true))
    {
      return false;
    }
  }

  if (PathInPrivateShare(strPath))
    return false;

  return true;
}

void CFileScanner::CleanChildFolders(const CStdString& strSharePath, const CStdString& strPath)
{

  if (m_bExtendedLog)
    CLog::Log(LOGDEBUG,"CFileScanner::CleanChildFolders, cleaning path = %s (filescanner)", strPath.c_str());
  // Retrieve all child folder of the specific folder
  std::set<std::string> setFolders;
  BOXEE::Boxee::GetInstance().GetMetadataEngine().GetChildFolders(setFolders, strPath, false);

  if (CDirectory::Exists(strSharePath))
  {
    // run over all child folders
    std::set<std::string>::iterator it = setFolders.begin();
    for (; it != setFolders.end(); it++)
    {
      CStdString strFolderPath = (*it);
      if (m_bExtendedLog)
        CLog::Log(LOGDEBUG,"CFileScanner::CleanChildFolders, check if %s exists (filescanner)", strFolderPath.c_str());

      // Only is the share exists and the specific folder under it does not, otherwise it could be the entire source is not available (external HD disconnected)
      // in which case we do not want to remove the folders from the database
      if (!CDirectory::Exists(strFolderPath))
      {
        CLog::Log(LOGWARNING,"CFileScanner::CleanChildFolders, ERASE, Media folder not available path = %s (filescanner)", strFolderPath.c_str());
        BOXEE::Boxee::GetInstance().GetMetadataEngine().RemoveAudioByFolder(strFolderPath); // this is the only table that doesnt have trigger - will be fixed later ?
        BOXEE::Boxee::GetInstance().GetMetadataEngine().RemoveFolderByPath(strFolderPath);
      }
    }
  }
}

void CFileScanner::CleanMediaFolders(const CStdString& strType, VECSOURCES& vecShares) 
{
  // Get list of all media folder from the database
  std::vector<BXFolder*> vecFolders;
  BOXEE::Boxee::GetInstance().GetMetadataEngine().GetMediaFolders(vecFolders, strType);

  std::vector<BXFolder*>::iterator it = vecFolders.begin();

  for (; it != vecFolders.end(); it++)
  {
    CStdString strPath = (*it)->GetPath();

    // Get the share for this path and check whether it is available
    if (g_application.IsPathAvailable(strPath, false))
    {
      // If the share is available, but the specific folder is not accessible, remove it
      bool bIsSourceName;
      int shareNum = CUtil::GetMatchingSource(strPath, vecShares, bIsSourceName);

      if (shareNum != -1)
      {
        CMediaSource* pShare = &vecShares[shareNum];
        if (pShare->m_iScanType == CMediaSource::SCAN_TYPE_PRIVATE)
        {
          CLog::Log(LOGINFO,"ERASE, Media folder located on private share, path = %s (filescanner)", strPath.c_str());
          BOXEE::Boxee::GetInstance().GetMetadataEngine().RemoveAudioByFolder(strPath); // this is the only table that doesnt have trigger - will be fixed later ?
          BOXEE::Boxee::GetInstance().GetMetadataEngine().RemoveFolderByPath(strPath);
        }
      }
    }

    delete *it;
  }
  vecFolders.clear();
}

void CFileScanner::InformRemoveShare(const CStdString sharePath)
{
  CLog::Log(LOGDEBUG, "source %s was removed from local sources list (filescanner)",sharePath.c_str());
  CSingleLock lock(m_shareLock);

  if (sharePath == m_currenlyScannedShare)
  {
    CLog::Log(LOGDEBUG, "source %s was currently scanned - stop scanning it (filescanner)",sharePath.c_str());
    m_currentShareValid = false;
  }
}

int CFileScanner::GetFolderStatus(const CStdString& strPath, std::vector<CStdString>& vecTypes)
{
  int iStatus = FOLDER_STATUS_NONE;

  if (CUtil::IsBoxeeDb(strPath))
    return FOLDER_STATUS_NONE;

  // check if the path is currently being scanned
  if (g_application.GetBoxeeFileScanner().IsScanning(strPath))
  {
    iStatus = FOLDER_STATUS_SCANNING;
  }
  else
  {
    std::vector<CMediaSource> vecSources;
    if (g_settings.GetSourcesFromPath(strPath, "", vecSources))
    {
      for (int i = 0; i < (int)vecSources.size(); i++)
      {
        if (vecSources[i].m_iScanType == CMediaSource::SCAN_TYPE_PRIVATE)
        {
          iStatus =  FOLDER_STATUS_PRIVATE;
          break;
        }

        CStdString strType = vecSources[i].m_type;
        vecTypes.push_back(strType);
        iStatus = FOLDER_STATUS_ON_SHARE;
      }
    }
    else
    {
      if (strPath.Left(10) == "sources://" || strPath.Left(10) == "network://")
      {
        iStatus = FOLDER_STATUS_NONE;
      }
      else
      {
        iStatus = FOLDER_STATUS_NOT_SHARE;
      }
    }
  }

  CLog::Log(LOGDEBUG,"CFileScanner::GetFolderStatus, path = %s, status = %d (checkpath)", strPath.c_str(), iStatus);

  return iStatus;
}

bool CFileScanner::ShowSourcesStatusKaiDialog(int messageId, const CStdString& sourceName, const CStdString& sourcePath, const CStdString& sourceType)
{
  CLog::Log(LOGDEBUG,"CFileScanner::ShowSourcesStatusKaiDialog - Enter function with [messageId=%d][sourceName=%s][sourcePath=%s][sourceType=%s] (ssk)",messageId,sourceName.c_str(),sourcePath.c_str(),sourceType.c_str());

  bool isPlayingVideo = g_application.IsPlayingVideo();
  if (isPlayingVideo)
  {
    CLog::Log(LOGDEBUG,"CFileScanner::ShowSourcesStatusKaiDialog - don't show SourcesStatusKaiDialog because [isPlayingVideo=%d=TRUE]. [messageId=%d] (ssk)",isPlayingVideo,messageId);
    return true;
  }

  if (sourceName.IsEmpty())
  {
    CLog::Log(LOGERROR,"CFileScanner::ShowSourcesStatusKaiDialog - FAILED to show SourcesStatusKaiDialog because [sourceName=%s] is EMPTY. [messageId=%d] (ssk)",sourceName.c_str(),messageId);
    return false;
  }

  CStdString messageStruct = g_localizeStrings.Get(messageId);

  CStdString messageCaption = "";
  CStdString messageDescription = "";
  CGUIDialogKaiToast::PopupIconEnums messageIcon;
  CStdString messageIconColor = "";
  CStdString messageBgColor = "";

  CStdString validSourceName = sourceName;
  if ((int)validSourceName.size() > MAX_ADDED_SOURCE_NAME_SIZE_IN_KAI)
  {
    validSourceName = validSourceName.Left(MAX_ADDED_SOURCE_NAME_SIZE_IN_KAI - 3);
    validSourceName += "...";
  }

  switch(messageId)
  {
  case 51037:
  {
    messageDescription.Format(messageStruct.c_str(), validSourceName);
    messageIcon = CGUIDialogKaiToast::ICON_SCAN;
    messageIconColor = KAI_GREY_COLOR;
    messageBgColor = KAI_GREY_COLOR;
  }
  break;
  case 51046:
  {
    messageDescription.Format(messageStruct.c_str(), validSourceName);
    messageIcon = CGUIDialogKaiToast::ICON_SCAN;
    messageIconColor = KAI_GREEN_COLOR;
    messageBgColor = KAI_GREEN_COLOR;
  }
  break;
  case 51047:
  {
    if (sourcePath.IsEmpty() || sourceType.IsEmpty())
    {
      CLog::Log(LOGERROR,"CFileScanner::ShowSourcesStatusKaiDialog - FAILED to show SourcesStatusKaiDialog because [sourcePath=%s] or [sourceType=%s] is EMPTY. [messageId=%d] (ssk)",sourcePath.c_str(),sourceType.c_str(),messageId);
      return false;
    }

    int numOfResolvedFiles = 0;

    if (sourceType.CompareNoCase("video") == 0)
    {
      BOXEE::BXVideoDatabase video_db;
      numOfResolvedFiles = video_db.GetShareUnresolvedVideoFilesCount(_P(sourcePath), STATUS_ALL);
    }
    else if (sourceType.CompareNoCase("music") == 0)
    {
      BOXEE::BXAudioDatabase audio_db;
      numOfResolvedFiles = audio_db.GetShareUnresolvedAudioFilesCount(_P(sourcePath), STATUS_ALL);
    }
    else
    {
      CLog::Log(LOGERROR,"CFileScanner::ShowSourcesStatusKaiDialog - FAILED to show SourcesStatusKaiDialog because [sourceType=%s] ISN'T video or audio. [messageId=%d] (ssk)",sourceType.c_str(),messageId);
      return false;
    }

    messageCaption.Format(messageStruct.c_str(),numOfResolvedFiles,validSourceName);
    messageDescription = g_localizeStrings.Get(51048);

    if (numOfResolvedFiles > 0)
    {
      messageIcon = CGUIDialogKaiToast::ICON_CHECK;
      messageIconColor = KAI_GREEN_COLOR;
      messageBgColor = KAI_GREEN_COLOR;
    }
    else
    {
      messageIcon = CGUIDialogKaiToast::ICON_EXCLAMATION;
      messageIconColor = KAI_RED_COLOR;
      messageBgColor = KAI_RED_COLOR;
    }
  }
  break;
  default:
  {
    CLog::Log(LOGERROR,"CFileScanner::ShowSourcesStatusKaiDialog - FAILED to handle [messageId=%d] (ssk)",messageId);
    return false;
  }
  break;
  }

  CLog::Log(LOGDEBUG,"CFileScanner::ShowSourcesStatusKaiDialog - Going to show kai dialog with [messageCaption=%s][messageDescription=%s] (ssk)",messageCaption.c_str(),messageDescription.c_str());

  g_application.m_guiDialogKaiToast.QueueNotification(messageIcon, messageCaption, messageDescription, TOAST_DISPLAY_TIME, messageIconColor , messageBgColor);

  return true;
}

void CFileScanner::UpdateScanLabel(const CStdString& path)
{
  CGUIWindowBoxeeMediaSources* pDlgSourceSource = (CGUIWindowBoxeeMediaSources*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_SOURCES);
  if (pDlgSourceSource && pDlgSourceSource->IsVisible())
  {
    pDlgSourceSource->UpdateScanLabel(path);
  }
}

void CFileScanner::InitSourcesOnStart()
{
  VECSOURCES sources = *(g_settings.GetSourcesFromType("all"));

  CMediaSource* pShare = NULL;
  for (IVECSOURCES it = sources.begin(); it != sources.end(); it++)
  {
    pShare = &(*it);
    time_t iLastScanned;
    if (!BOXEE::Boxee::GetInstance().GetMetadataEngine().GetScanTime(pShare->strName, pShare->strPath, pShare->m_type, iLastScanned))
    {
      CLog::Log(LOGWARNING,"CFileScanner::InitSourcesOnStart - FAILED to read LastScanTime of [name=%s][path=%s][type=%d]. [m_bPaused=%d][m_bStop=%d] (fs)",pShare->strName.c_str(),pShare->strPath.c_str(),pShare->m_iScanType,m_bPaused,m_bStop);
      continue;
    }

    if (iLastScanned == SHARE_TIMESTAMP_RESOLVING)
    {
      CLog::Log(LOGDEBUG,"CFileScanner::InitSourcesOnStart - for [name=%s][path=%s][type=%d] LastScanTime is [%lu=RESOLVING] going to reset it. [m_bPaused=%d][m_bStop=%d] (fs)",pShare->strName.c_str(),pShare->strPath.c_str(),pShare->m_iScanType,iLastScanned,m_bPaused,m_bStop);

      if (!BOXEE::Boxee::GetInstance().GetMetadataEngine().UpdateScanTime(pShare->strName, pShare->strPath, pShare->m_type, SHARE_TIMESTAMP_NOT_SCANNED))
      {
        CLog::Log(LOGERROR,"CFileScanner::InitSourcesOnStart - FAILED to reset iLastScanned of source [name=%s][path=%s][type=%d]. [m_bPaused=%d][m_bStop=%d] (fs)",pShare->strName.c_str(),pShare->strPath.c_str(),pShare->m_iScanType,m_bPaused,m_bStop);
      }
    }
  }
}

