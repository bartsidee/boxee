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

#include "Picture.h"

// Linux includes
#ifdef _LINUX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif


using namespace XFILE;
using namespace DIRECTORY;
using namespace BOXEE;

#define FILE_SCANNER_ID "XBMC File Scanner v1.0"

CFileScanner::CFileScanner()
{
  m_MDE = NULL;
  m_pResolver = NULL;
  m_bExtendedLog = true;
}

CFileScanner::~CFileScanner()
{
  Reset();
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

  return true;
}

void CFileScanner::Process()
{
#ifdef _WIN32
  ::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#endif

  time_t now;
  int iMillisecondsToSleep = BXConfiguration::GetInstance().GetIntParam("Boxee.FileScanner.Scan.Interval", 180000);

  // let everything initialize first before we start the first scan. this is to prevent a cpu-boost on startup
  Sleep(5000);

  while (!m_bStop)
  {
    // Clear the set of private paths
    m_PrivatePaths.clear();

    time(&now);

    if (m_bExtendedLog)
      CLog::Log(LOGDEBUG,"CFileScanner::Process, scan round started (filescanner)");

    VECSOURCES video = *g_settings.GetSourcesFromType("video");
    ScanShares(&video, "video");
    CleanMediaFolders("videoFolder", video);

    VECSOURCES audio = *g_settings.GetSourcesFromType("music");
    ScanShares(&audio, "music");
    CleanMediaFolders("musicFolder", audio);

    // dont scan pictures for now. it takes a long time (depends on picture software installed) and it is used in folder more anyway.
    //VECSOURCES pictures = *g_settings.GetSourcesFromType("pictures");
    //ScanShares(&pictures, "picture", SCAN_TYPE_FOLDERS);
    //CleanMediaFolders("pictureFolder", pictures);

    // Lock the
    std::set<std::pair<CStdString, CStdString> > setUserPaths;

    {
      CSingleLock lock(m_lock);
      setUserPaths = m_setUserPaths;
    }

    std::set<std::pair<CStdString, CStdString> >::iterator it = setUserPaths.begin();
    while (it != setUserPaths.end() && !m_bStop)
    {
      CStdString strPath = (*it).first;
      CStdString strType = (*it).second;

      CLog::Log(LOGDEBUG,"CFileScanner::Process, scan user folder %s of type %s (filescanner) (checkpath)", strPath.c_str(), strType.c_str());

      ScanMediaFolder(strPath, strType, strPath);

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
      Sleep(iMillisecondsToSleep);
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

    if (ShouldScan(pShare))
    {
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
    CLog::Log(LOGERROR,"CFileScanner::Process, unable to get scan time for share %s, path %s, scan type %d, assume 0 (filescanner)",
        pShare->strName.c_str(), pShare->strPath.c_str(), pShare->m_iScanType);
    iLastScanned = 0;
  }

  // in case that the share is marked as being resolved we assume that the program was shutdown during
  // the resolving process, in this case we will treat is as time 0.
  if (iLastScanned == SHARE_TIMESTAMP_RESOLVING)
	  iLastScanned = SHARE_TIMESTAMP_NOT_SCANNED;

  CLog::Log(LOGDEBUG,"CFileScanner::Process, scan share %s, scan type %d, last scanned %d (filescanner)",
      pShare->strPath.c_str(), pShare->m_iScanType, iLastScanned);

  // Check if the share should be scanned now according to its scan type
  time_t now = time(NULL);
  if ((pShare->m_iScanType == CMediaSource::SCAN_TYPE_MONITORED && (now - iLastScanned) > 60 * 5) ||
      (pShare->m_iScanType == CMediaSource::SCAN_TYPE_DAILY && (now - iLastScanned) > 60 * 60 * 24) ||
      (pShare->m_iScanType == CMediaSource::SCAN_TYPE_ONCE && iLastScanned == 0))
  {
    CLog::Log(LOGDEBUG,"CFileScanner::Process, SCANNING share %s, scan type %d (filescanner)", pShare->strPath.c_str(), pShare->m_iScanType);
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG,"CFileScanner::Process, NOT SCANNING share %s, scan type %d (filescanner)", pShare->strPath.c_str(), pShare->m_iScanType);
    return false;
  }
  return false;
}

void CFileScanner::ScanShare(CMediaSource* pShare, const CStdString& strShareType)
{
  if (!IsScannable(pShare->strPath))
  {
    return;
  }

  if (m_bExtendedLog)
    CLog::Log(LOGDEBUG,"CFileScanner::ScanShare, Scanning source, name = %s, path = %s (filescanner)", pShare->strName.c_str(), _P(pShare->strPath).c_str());

  // first check if the path is still at the sources list
  std::vector<CMediaSource> vecSources;
  if (!g_settings.GetSourcesFromPath(pShare->strPath, strShareType,vecSources ))
  {
	  CLog::Log(LOGDEBUG,"CFileScanner::ScanShare Share %s was already deleted from the local sources list (filescanner)", pShare->strPath.c_str());
	  return;
  }
  {
    CSingleLock lock(m_shareLock);
	m_currenlyScannedShare = pShare->strPath;
	m_currentShareValid = true;
  }

  BOXEE::Boxee::GetInstance().GetMetadataEngine().UpdateScanTime(pShare->strName, pShare->strPath, pShare->m_type, SHARE_TIMESTAMP_RESOLVING);
  // Go over all items in the share
  for (size_t i = 0; i < pShare->vecPaths.size() && !m_bStop; i++)
  {
    CStdString strPath = _P(pShare->vecPaths[i]);
    ScanMediaFolder(strPath, strShareType, strPath);
  }

  {
    CSingleLock lock(m_shareLock);
	m_currenlyScannedShare = "";
	m_currentShareValid = true;
  }
  BOXEE::Boxee::GetInstance().GetMetadataEngine().UpdateScanTime(pShare->strName, pShare->strPath, pShare->m_type, time(NULL));
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
  }

  CLog::Log(LOGDEBUG,"CFileScanner::SynchronizeFolderAndDatabase, folder %s id %d (filescanner)",strPath.c_str(), iFolderId);

  if (strShareType == "music")
  {
    m_MDE->GetAudiosByFolder(dbResolvedItems, strPath.c_str());
    m_MDE->GetUnresolvedAudioFilesByFolder(dbUnResolvedItems, iFolderId);
  }
  else if (strShareType == "video")
  {
    m_MDE->GetVideosByFolder(dbResolvedItems, strPath.c_str());
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


    // the item was found in the both resolved_filles table and folder,
    // so we shouldn't add or remove it from the data base
    // we will only modify the ResolvedItems since  we can just delete
    // from those tables and not add to it
    if (db_resolved_it != dbResolvedItems.end())
    {
      // check also if the video file still fits for size limitation then
      if ((strShareType != "video")  || folderItems[i]->GetSize() >=  g_guiSettings.GetInt("myvideos.videosize") * 1024 * 1024)
      {
        delete db_resolved_it->second;
        dbResolvedItems.erase(db_resolved_it);
      }
      else
      {
        CLog::Log(LOGDEBUG,"CFileScanner::SynchronizeFolderAndDatabase file %s doesnt fit size limitation %d - remove from the data base (filescanner)",folderItems[i]->m_strPath.c_str(),
        		folderItems[i]->GetSize());
      }
    }

    // the item was found in the both data base and folder,
    // so we shouldn't add or remove it from the data base
    if (db_unresolved_it != dbUnResolvedItems.end())
    {
      // check also if the file still fits for size limitation then
      if  ((strShareType != "video")  || folderItems[i]->GetSize() >=  g_guiSettings.GetInt("myvideos.videosize") * 1024 * 1024)
      {
        delete db_unresolved_it->second;
        dbUnResolvedItems.erase(db_unresolved_it);
      }
      else
      {
        CLog::Log(LOGDEBUG,"CFileScanner::SynchronizeFolderAndDatabase  file %s doesnt fit size limitation - remove from the data base (filescanner)",folderItems[i]->m_strPath.c_str());
      }
      folderItems.Remove(i);
    }
    else
    {
      CLog::Log(LOGDEBUG,"CFileScanner::SynchronizeFolderAndDatabase, file %s was'nt found in unresolved_video_files  (filescanner)", folderItems[i]->m_strPath.c_str());
      i++;
    }
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
    	if (folderItems[i]->GetSize() >=  g_guiSettings.GetInt("myvideos.videosize") * 1024 * 1024)
        {
    	  CLog::Log(LOGDEBUG,"CFileScanner::SynchronizeFolderAndDatabase, Add file %s to db (filescanner)", folderItems[i]->m_strPath.c_str());
          m_MDE->AddVideoFile(folderItems[i]->m_strPath, strSharePath, iFolderId, folderItems[i]->m_dwSize);
        }
    	else
    	{
      	  CLog::Log(LOGDEBUG,"CFileScanner::SynchronizeFolderAndDatabase, file %s size is %d smaller then limitation (filescanner)", folderItems[i]->m_strPath.c_str(), folderItems[i]->GetSize());
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
    //CLog::Log(LOGDEBUG,"CFileScanner::SynchronizeFolderAndDatabase, delete file %s from resolved table (filescanner)", db_resolved_it->first.c_str());
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

  if (updateFolderStatus)
  {
    m_MDE->MarkFolderNew(strPath.c_str());
  }

}
void CFileScanner::ScanMediaFolder(CStdString& strPath, const CStdString& strShareType, const CStdString& strSharePath)
{
  if (m_bStop || !IsScannable(strPath))
    return;

  // first transalate the media folder
  strPath = _P(strPath);
  CLog::Log(LOGDEBUG,"CFileScanner::ScanMediaFolder, scann folder  %s  (filescanner)", strPath.c_str());

  CleanChildFolders(strSharePath, strPath);

  CFileItemPtr pItem;

  // Retrieve the list of all items from the path
  CFileItemList items, dirItems, audioItems, videoItems ;

  if (!CDirectory::GetDirectory(strPath, items, "", false))
    return;

  // Go over all items in the folder - split it into
  // directories, audio files and movie files
  for (int i=0; !m_bStop && m_currentShareValid && i<items.Size(); i++)
  {

    pItem = items[i];
    if ((pItem->m_bIsFolder || pItem->IsRAR() || pItem->IsZIP()) && (!pItem->IsPlayList()))
    {
      dirItems.Add(pItem);
    } else if ((strShareType == "music") &&
              (pItem->IsAudio() && !pItem->IsRAR() && !pItem->IsZIP() && !pItem->IsPlayList()))
    {
      audioItems.Add(pItem);

    } else if ((strShareType == "video") &&
        (pItem->IsVideo() && !pItem->IsRAR() && !pItem->IsZIP() && !pItem->IsPlayList() ))
    {
      CLog::Log(LOGDEBUG,"CFileScanner::ScanMediaFolder, add to folder item ");
      pItem->Dump();
      videoItems.Add(pItem);
    } else {
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
  if ((strShareType == "music") && audioItems.Size () > 0 )
  {
    CLog::Log(LOGDEBUG,"CFileScanner::ScanMediaFolder, synchronized music folder  %s  (filescanner)", strPath.c_str());
    SynchronizeFolderAndDatabase(strPath, "music", strSharePath, audioItems);
  }
  // if it is a audio share, and the folder contains video files sync it with the database
  if ((strShareType == "video") && videoItems.Size () > 0 )
  {
    CLog::Log(LOGDEBUG,"CFileScanner::ScanMediaFolder, synchronized video folder  %s  (filescanner)", strPath.c_str());
    SynchronizeFolderAndDatabase(strPath, "video", strSharePath, videoItems);
  }
}

/*void CFileScanner::ScanMediaFolder1(const CStdString& strPath, const CStdString& strShareType, const CStdString& strSharePath)
{
  if (m_bStop || !IsScannable(strPath)) 
    return;

  // this method has a potential of hogging cpu (recursive, depends on how deep the file structure is).
  // since its a background process we try to "slow it down" and keep it from effecting ui by introducing a short sleep.
  Sleep(100);

  if (m_bExtendedLog)
    CLog::Log(LOGDEBUG,"CFileScanner::ScanMediaFolder, scanning folder, path = %s (filescanner)", strPath.c_str());

  CleanChildFolders(strPath);

  bool bFolderAdded = false;
  int iFolderId = -1;

  // Retrieve the list of all items from the path                                                                  
  CFileItemList items; 
  if (CDirectory::GetDirectory(strPath, items, "", false))
  {
    // Go over all items in the folder
    for (int i=0; !m_bStop && i<items.Size(); i++)
    {
      CFileItemPtr pItem = items[i];

      if ((pItem->m_bIsFolder || pItem->IsRAR() || pItem->IsZIP()) && (!pItem->IsPlayList()))
      {
        // Recursively dive into the directory
        ScanMediaFolder(pItem->m_strPath, strShareType, strSharePath);
      }
      else 
      {
        if (strShareType == "music")
        {
          if (pItem->IsAudio() && !pItem->IsRAR() && !pItem->IsZIP() && !pItem->IsPlayList() )
          {
            if (!bFolderAdded)
            {
              iFolderId = m_MDE->AddMediaFolder(strPath, "musicFolder", BoxeeUtils::GetModTime(strPath));
              bFolderAdded = true;
            }

            // Add video to the database
            m_MDE->AddAudioFile(pItem->m_strPath, strSharePath, iFolderId);
          }
        }
        else if (strShareType == "video")
        {
          if (pItem->IsVideo() && !pItem->IsRAR() && !pItem->IsZIP() && !pItem->IsPlayList() )
          {
            if (!bFolderAdded)
            {
              iFolderId = m_MDE->AddMediaFolder(strPath, "videoFolder", BoxeeUtils::GetModTime(strPath));

              bFolderAdded = true;
            }

            // Add video to the database
            m_MDE->AddVideoFile(pItem->m_strPath, strSharePath, iFolderId);
          }
        }
//        else if (strShareType == "picture")
//        {
//          if (pItem->IsPicture() && !pItem->IsRAR() && !pItem->IsZIP() && !pItem->IsPlayList() )
//          {
//            bIsMediaFolder = true;
//          }
//        }
        else
        {
          // TODO: Add error handling
        }
      }
    } // for
  }
}
*/
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

bool CFileScanner::IsScanning(const CStdString& strPath)
{
  std::set<std::pair<CStdString, CStdString> > setUserPaths;

  {
    CSingleLock lock(m_lock);
    setUserPaths = m_setUserPaths;
  }

  std::set<std::pair<CStdString, CStdString> >::iterator it = setUserPaths.begin();
  while (it != setUserPaths.end())
  {
    CStdString strTempPath = it->first;

    if (strTempPath == strPath)
      return true;

    if (strPath.Find(strTempPath) != -1)
      return true;

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

  CURL url(strPath);
  CStdString strProtocol = url.GetProtocol();

  // We only handle directories on the hard drive
  if (strProtocol.size() != 0)
  {
    if (strProtocol == "smb")
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
      CLog::Log(LOGDEBUG,"CFileScanner::CleanChildFolders, check if %s exists (filescanner)", strFolderPath.c_str());

      // Only is the share exists and the specific folder under it does not, otherwise it could be the entire source is not available (external HD disconnected)
      // in which case we do not want to remove the folders from the database
      if (!CDirectory::Exists(strFolderPath))
      {
        CLog::Log(LOGERROR,"CFileScanner::CleanChildFolders, ERASE, Media folder not available path = %s (filescanner)", strFolderPath.c_str());
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
  SetEvent(m_WakeEvent);
  StopThread();
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
      if (strPath.Left(10) == "sources://")
      {
        iStatus = FOLDER_STATUS_NONE;
      }
      else
      {
        iStatus = FOLDER_STATUS_NOT_SHARE;
      }
    }
  }

  CLog::Log(LOGDEBUG,"CLocalBrowseWindowState::GetFolderStatus, path = %s, status = %d (checkpath)", strPath.c_str(), iStatus);

  return iStatus;
}
