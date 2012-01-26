

#include "MetadataResolver.h"
#include "MetadataResolverVideo.h"
#include "MetadataResolverMusic.h"

#include "FileSystem/Directory.h"

#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "lib/libBoxee/bxmetadataengine.h"
#include "lib/libBoxee/bxutils.h"

#include "Util.h"
#include "utils/log.h"
#include "utils/RegExp.h"
#include "utils/fstrcmp.h"
#include "../utils/Base64.h"
//#include "../utils/md5.h"
#include <map>

#include "cores/dvdplayer/DVDPlayer.h"
#include "cores/dvdplayer/DVDFileInfo.h"
#include "FileSystem/File.h"
#include "Picture.h"
#include "Application.h"
#include "SpecialProtocol.h"
#include "GUISettings.h"
#include "TextureManager.h"
#include "BoxeeUtils.h"
#include "GUIWindowManager.h"
#include "LocalizeStrings.h"
#include "GUIWindowBoxeeMediaSources.h"

#define READ_BLOCK_SIZE 16384

using namespace BOXEE;
using namespace DIRECTORY;

CMetadataResolver::CMetadataResolver()
{
  m_bStopped = false;
}

CMetadataResolver::~CMetadataResolver()
{
}

bool CMetadataResolver::Stop()
{
  CLog::Log(LOGINFO, "Boxee Metadata Resolver, stopping...");
  m_bStopped = true;

  return m_bStopped;
}

bool CMetadataResolver::CheckPath(const std::string& strPath)
{
  if (!g_settings.IsPathOnSource(strPath)) 
  {
    return false;
  }

  // do not check availability for local path
  if (!CURI::IsFileOnly(strPath) && !CUtil::IsHD(strPath) && !CUtil::IsSpecial(strPath))
  {
    if (!g_application.IsPathAvailable(strPath, false)) {
      return false;
    }
  }
  return true;
}

int CMetadataResolver::Resolve(BXMetadataScannerJob * pJob, std::vector<BXFolder*>& vecResults )
{
  if (!pJob || pJob->GetFolder() == NULL || pJob->GetFolder()->GetPath().empty()) {
    CLog::Log(LOGERROR,"CMetadataResolver::Resolve - could not resolve file because input is NULL (resolver)");
    return RESOLVER_FAILED;
  }

  const BXFolder* pInputFolder = pJob->GetFolder();

  CLog::Log(LOGDEBUG,"CMetadataResolver::Resolve - enter with [path=%s] (resolver)", pInputFolder->GetPath().c_str());
  if (!g_settings.IsPathOnSource(pInputFolder->GetPath())) 
  {
    CStdString resolvingPath;
    resolvingPath.Format("Aborted: path %s not on any attached source", pInputFolder->GetPath().c_str());

    CLog::Log(LOGDEBUG,"CMetadataResolver::Resolve - path [%s] not on any attached source. return [%d=RESOLVER_CANT_ACCESS] (resolver)", pInputFolder->GetPath().c_str(),RESOLVER_CANT_ACCESS);

    return RESOLVER_CANT_ACCESS;
  }

  if (!g_application.IsPathAvailable(pInputFolder->GetPath(), false))
  {
    //CLog::Log(LOGDEBUG, "CMetadataResolver::Resolve, resolver aborted, path not available, path = %s", pInputFolder->GetPath().c_str());
    CStdString resolvingPath;
    resolvingPath.Format("Aborted: path %s not available", pInputFolder->GetPath().c_str());

    CLog::Log(LOGDEBUG,"CMetadataResolver::Resolve - path [%s] not available. return [%d=RESOLVER_CANT_ACCESS] (resolver)", pInputFolder->GetPath().c_str(),RESOLVER_CANT_ACCESS);

    return RESOLVER_CANT_ACCESS;
  }

  int iStatus;

  UpdateResolveLabel(pInputFolder->GetPath());

  CStdString folderType = pInputFolder->GetType();
  if (folderType == MEDIA_ITEM_TYPE_MUSIC_FOLDER)
  {
    CLog::Log(LOGDEBUG,"CMetadataResolver::Resolve - going to resolve MUSIC folder [path=%s] (resolver)", pInputFolder->GetPath().c_str());
    iStatus = ResolveMusicFolder(pJob);
  }
  else if (folderType == MEDIA_ITEM_TYPE_VIDEO_FOLDER)
  {
    CLog::Log(LOGDEBUG,"CMetadataResolver::Resolve - going to resolve VIDEO folder [path=%s] (resolver)", pInputFolder->GetPath().c_str());
    iStatus = ResolveVideoFolder(pJob);
  }
  else
  {
    CLog::Log(LOGERROR,"CMetadataResolver::Resolve - could not resolve folder because of unrecognized type [%s] (resolver)", folderType.c_str());
    iStatus = RESOLVER_FAILED;
  }

  UpdateResolveLabel(g_localizeStrings.Get(53160));

  CLog::Log(LOGDEBUG,"CMetadataResolver::Resolve - exit and return [%d] for folder [%s] (resolver)",iStatus,pInputFolder->GetPath().c_str());

  return iStatus;

}

int CMetadataResolver::ResolveMusicFolder(BXMetadataScannerJob* pJob)
{
  return CMetadataResolverMusic::ResolveMusicFolder(pJob);
}
  
long CMetadataResolver::GetMediaFileDurationInMin(const CStdString &strPath)
{
  CFileItemPtr item(new CFileItem);
  CDVDFileInfo::GetFileMetaData(strPath, item.get());

  long iLenMsec = BXUtils::StringToInt(item->GetProperty("duration-msec"));
  return iLenMsec / 1000 / 60;
}

/**
  This function detects which videos were removed from the disk by comparing the list 
  of retrieved files and folders with the list retrieved from the database

  The function receives a path to a folder and a list of items retrieved from it

*/
void CMetadataResolver::PurgeDeletedVideosFromDatabase(const CStdString& strFolderPath, const CFileItemList& items)
{
  std::map<std::string, BXMetadata*> mapDatabaseCache;
  std::set<std::string> setFolders;

  // Get all videos that appear in the database under this folder
  BOXEE::Boxee::GetInstance().GetMetadataEngine().GetVideosByFolder(mapDatabaseCache, strFolderPath, true);

  // Get all child folders of this folder as a set, and remove the folder itself from the list
  BOXEE::Boxee::GetInstance().GetMetadataEngine().GetChildFolders(setFolders, strFolderPath);

  setFolders.erase(strFolderPath);

  std::map<std::string, BXMetadata*>::iterator it = mapDatabaseCache.begin();
  while(it != mapDatabaseCache.end())
  {
    CStdString strFolderPath = it->first;
//    CUtil::RemoveSlashAtEnd(strFolderPath);

    if (setFolders.find(strFolderPath) != setFolders.end())
    {
      std::map<std::string, BXMetadata*>::iterator it2 = it++;
      mapDatabaseCache.erase(it2);
    }
    else
    {
      it++;
    }
  }
  
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr pItem = items[i];
    if (pItem->m_bIsFolder) 
    {
      CStdString itemPath = pItem->m_strPath;

      // This is a VERY VERY VERY ugly patch, but since the slash/backslash issue is a little bit complicated
      // and already patch base, this is the best i can do.
      // the file is stored as "rar://smb\\:xxx\xxxx
      // in order to decode it i used URL functionality 
      // and it change the slash to linux mode /
      // so i switched it again to windows mode s
      if (pItem->m_strPath.Find("rar") != -1)
      {
        CURI       url(pItem->m_strPath);
        itemPath = url.GetHostName();
#ifdef _WIN32
        itemPath.Replace("/","\\");
#endif
      }

      if (CUtil::HasSlashAtEnd(itemPath))
      {
        itemPath.Delete(itemPath.size() - 1);
      }

      // Remove the folders that do exist under this directory from the set
      // Folders that remain in the set will be marked as deleted and removed from the database

      std::set<std::string>::const_iterator it; 
      for(it = setFolders.begin(); it != setFolders.end(); ) 
      {
        CStdString path = *it;

        // make sure that both paths will be without slash at the end
        if (CUtil::HasSlashAtEnd(path))
        {
          path.Delete(path.size() - 1);
        }
        if (path.find(itemPath) != std::string::npos)
        {
          CLog::Log(LOGDEBUG,"CMetadataResolver::PurgeDeletedVideosFromDatabase, erase from setFolders when [%s] compared to [%s]",path.c_str(),itemPath.c_str());
          setFolders.erase(it++);
        }
        else
        {
          ++it;
        }
      }
    }
    else
    {
      // not a folder
      std::map<std::string, BXMetadata*>::iterator it = mapDatabaseCache.find(pItem->m_strPath);
      if (it != mapDatabaseCache.end()) 
      {
        delete it->second;
        mapDatabaseCache.erase(it);
        pItem->SetProperty("existsInDB",true);
      }
    }
  }

  if (mapDatabaseCache.size() > 0) 
  {
    for(std::map<std::string, BXMetadata*>::iterator it = mapDatabaseCache.begin() ; it != mapDatabaseCache.end() ; it++)
    {
      CLog::Log(LOGDEBUG,"CMetadataResolver::PurgeDeletedVideosFromDatabase, GetVideosByFolder returned: [%s] (resolver)(purge)",it->first.c_str());
    }


    CleanDeletedVideos(mapDatabaseCache);
    mapDatabaseCache.clear();

    // Update the related directory, so it will be rescanned again
    BOXEE::Boxee::GetInstance().GetMetadataEngine().MarkFolderNew(strFolderPath);
  }

  if (setFolders.size() > 0)
  {
    for(std::set<std::string>::iterator it = setFolders.begin() ; it != setFolders.end() ; it++)
    {
      CLog::Log(LOGDEBUG,"CMetadataResolver::PurgeDeletedVideosFromDatabase, GetChildFolders returned: [%s] (resolver)(purge)",it->c_str());
    }

    CleanDeletedFolders(setFolders);
    setFolders.clear();
  }
}

bool CMetadataResolver::ResolveBlurayFolder(const CStdString& strFolderPath, int iFolderId, const CFileItemList& items, int& iResult)
{
  if (CUtil::IsBlurayFolder(strFolderPath,&items))
  {
    bool bResult = false;
    BXMetadata* pMetadata = new BXMetadata(MEDIA_ITEM_TYPE_VIDEO);
    BXVideo* pVideo = (BXVideo*)pMetadata->GetDetail(MEDIA_DETAIL_VIDEO);
    pVideo->m_iFolderId = iFolderId;

    // Create and initialize appropriate video content
    CStdString strFolderName = strFolderPath;
    CUtil::RemoveSlashAtEnd(strFolderName);

    CVideoFileContext context(strFolderName);
    context.m_playableFolderType = CPlayableFolderType::PF_BLURAY;
    context.Load();
    context.bMovie = true;
    context.m_userVideoCoverPath = BoxeeUtils::GetUserVideoThumbPath(strFolderName,&items);

    int iVideoId  = -1;
    BOXEE::BXMetadataEngine& MDE = BOXEE::Boxee::GetInstance().GetMetadataEngine();
    if (CMetadataResolverVideo::ResolveVideo(context, pMetadata))
    {
      CLog::Log(LOGDEBUG, "CMetadataResolver::ResolveBlurayFolder, resolved Bluray video, name %s (videoresolver) (resolver)", pVideo->m_strTitle.c_str());
      
      // Set required paramerters and add video directly to the database
      pMetadata->m_bResolved = true;
      pMetadata->SetPath(_P(strFolderPath));
      // We set the media file id to 1, to indicate that this is a real file
      pMetadata->SetMediaFileId(1);

      unsigned int modDate = MDE.GetFolderDate(_P(strFolderPath));
      pMetadata->m_iDateModified = modDate;

      // Set indication that this video consists of multiple parts
      pVideo->m_strPath = _P(strFolderPath);
      pVideo->m_iDateModified = modDate;

      // Perform caching of DVD thumb
      CacheThumb(pMetadata->GetPath(), pVideo->m_strCover);
     
      // Add the DVD Video Item to the database
      iVideoId = MDE.AddVideo(pMetadata);
      if (iVideoId != MEDIA_DATABASE_ERROR)
      {
        bResult = true;
      }
    }

    // Mark folder status according to resolver result
    if (bResult)
    {
      MDE.MarkFolderResolved(strFolderPath, -1);
      MDE.UpdateVideoFileStatus(strFolderPath, STATUS_RESOLVED);
      iResult = RESOLVER_SUCCESS;
    }
    else
    {
      MDE.MarkFolderUnresolved(strFolderPath , false);
      MDE.UpdateVideoFileStatus(strFolderPath, STATUS_UNRESOLVED);
      iResult = RESOLVER_FAILED;
    }

    delete pMetadata;
    return bResult;
  }
  else
  {
    iResult = RESOLVER_FAILED;
    return false;
  }
}

bool CMetadataResolver::ResolveDvdFolder(const CStdString& strFolderPath, int iFolderId, const CFileItemList& items, int& iResult)
{
  if (CUtil::IsDVDFolder(strFolderPath,&items))
  {
    bool bResult = false;

    BXMetadata* pMetadata = new BXMetadata(MEDIA_ITEM_TYPE_VIDEO);
    BXVideo* pVideo = (BXVideo*)pMetadata->GetDetail(MEDIA_DETAIL_VIDEO);
    pVideo->m_iFolderId = iFolderId;

    // Create and initialize appropriate video content
    CStdString strFolderName = strFolderPath;
    CUtil::RemoveSlashAtEnd(strFolderName);

    CVideoFileContext context(strFolderName);
    context.m_playableFolderType = CPlayableFolderType::PF_DVD;
    context.Load();
    context.bMovie = true; // we have no way of resolving tv show dvds for now
    context.m_userVideoCoverPath = BoxeeUtils::GetUserVideoThumbPath(strFolderName,&items);

    BOXEE::BXMetadataEngine& MDE = BOXEE::Boxee::GetInstance().GetMetadataEngine();

    CFileItemList vobFiles;
    int iVideoId  = -1;
    if (CMetadataResolverVideo::ResolveVideo(context, pMetadata))
    {
      CLog::Log(LOGDEBUG, "CMetadataResolver::ResolveDvdFolder, resolved DVD folder, name %s (videoresolver) (resolver)", pVideo->m_strTitle.c_str());

      // Set required paramerters and add video directly to the database
      pMetadata->m_bResolved = true;
      pMetadata->SetPath(_P(strFolderPath));
      // We set the media file id to 1, to indicate that this is a real file
      pMetadata->SetMediaFileId(1);

      unsigned int modDate = MDE.GetFolderDate(_P(strFolderPath));
      pMetadata->m_iDateModified = modDate;

      // Set indication that this video consists of multiple parts
      pVideo->m_strPath = _P(strFolderPath);
      pVideo->m_strType = "part";
      pVideo->m_iDateModified = modDate;

      // Perform caching of DVD thumb
      CacheThumb(pMetadata->GetPath(), pVideo->m_strCover);

      // Add the DVD Video Item to the database
      iVideoId = MDE.AddVideo(pMetadata);
      if (iVideoId == MEDIA_DATABASE_ERROR) 
      {
        bResult = false;
      }
      else
      {
        // Get all vob and ifo files in the folder

        bool foundVideoTsDirectory = false;
        for (int i=0; i < items.Size() && !m_bStopped && !foundVideoTsDirectory; i++)
        {
          CStdString itemPath = items[i]->m_strPath;
          CUtil::RemoveSlashAtEnd(itemPath);
          if(CUtil::GetFileName(itemPath) == "VIDEO_TS")
          {
            foundVideoTsDirectory = true;
            CFileItemList videoTsItems;
            CDirectory::GetDirectory(itemPath, videoTsItems);

            for (int k=0; k < videoTsItems.Size(); k++)
            {
              CStdString strItemPath = videoTsItems[k]->m_strPath;
              CStdString strExtension = CUtil::GetExtension(strItemPath);
              if ((strExtension.ToLower() == ".vob") || (strExtension.ToLower() == ".ifo"))
              {
                CFileItemPtr item(new CFileItem(*videoTsItems[k]));
                vobFiles.Add(item);
              }
            }
          }
        }

        // Sort .vob files alphabetically, which should be good enough to ensure correct playback order
        vobFiles.Sort(SORT_METHOD_FILE, SORT_ORDER_ASC);
        bResult = true;
      }
    }
    else 
    {
      bResult = false;
    }

    // Mark folder status according to resolver result
    if (bResult)
    {
      MDE.MarkFolderResolved(strFolderPath, -1);
      MDE.UpdateVideoFileStatus(strFolderPath, STATUS_RESOLVED);
      iResult = RESOLVER_SUCCESS;
    }
    else 
    {
      MDE.MarkFolderUnresolved(strFolderPath , false);
      MDE.UpdateVideoFileStatus(strFolderPath, STATUS_UNRESOLVED);
      iResult = RESOLVER_FAILED;
    }

    for (int i = 0; i < vobFiles.Size(); i++)
    {
      if (bResult)
      {
        MDE.AddPart(iVideoId, i, vobFiles[i]->m_strPath);
        MDE.UpdateVideoFileStatus(vobFiles[i]->m_strPath, STATUS_RESOLVED);
      }
      else
      {
        MDE.UpdateVideoFileStatus(vobFiles[i]->m_strPath, STATUS_UNRESOLVED);
      }
    }

    delete pMetadata;

    // this is a DVD folder
    return bResult;
  }
  else
  {
    // this is not a DVD folder
    iResult = RESOLVER_FAILED;
    return false;
  }
}

/**
  This function detects and handles a frequent special case of a folder with two videos that 
  are parts of the same movie (like CD1, CD2 and similar
*/
bool CMetadataResolver::ResolveTwoVideoFolder(std::vector<CFileItemPtr>& items)
{
  // Get the number of video items in the folder
  int iNumVideoItems = items.size();

  // Special case, if there are two video files in the folder, check whether they are parts of the same movie
  if (iNumVideoItems == 2) 
  {
    CStdString strPath1 = items[0]->m_strPath;
    CStdString strPath2 = items[1]->m_strPath;
    if (!CMetadataResolverVideo::IsSample(strPath1) && !CMetadataResolverVideo::IsSample(strPath2) && !CMetadataResolverVideo::IsTrailer(strPath1) && !CMetadataResolverVideo::IsTrailer(strPath2))
    {
      // Check that video file names do not contain the series tag, in which case this is just two episodes in one folder
      CVideoFileContext context1(strPath1);
      context1.LoadNameFromPath();

      CVideoFileContext context2(strPath2);
      context2.LoadNameFromPath();

      int minimumSize = g_guiSettings.GetInt("myvideos.videosize") * 1024 * 1024;
      if (context1.iSize < minimumSize || context2.iSize < minimumSize )
      {
        return false;
      }

      if (context1.bMovie && context2.bMovie)
      {
        double relevance = fstrcmp(strPath1.c_str(), strPath2.c_str(), 0.0f);
        if (relevance > 0.95)
        {
          CLog::Log(LOGDEBUG,"CMetadataResolver::ResolveTwoVideoFolder, %s is relevant to %s by %f (resolve)", strPath1.c_str(), strPath2.c_str(), relevance);
          return true;
        }
      }
    }
    return false;
  }
  else
  {
    return false;
  }
}

int CMetadataResolver::ResolveVideoFolder (BXMetadataScannerJob* pJob)
{
  // Get the path of the resolved folder  
  const CStdString strFolderPath = _P(pJob->GetFolder()->GetPath());
  if (strFolderPath == "") 
  {
    CLog::Log(LOGERROR,"CMetadataResolver::ResolveVideoFolder - unable to resolve folder, path is empty (resolver)(videoresolver)");
    return RESOLVER_FAILED;
  }
  
  CVideoFolderContext folderContext(strFolderPath);
  folderContext.Load();

  CLog::Log(LOGDEBUG,"CMetadataResolver::ResolveVideoFolder - Resolving video folder [path=%s][ParentFolder=%s][ParentFolderName=%s] (resolver)(videoresolver)",strFolderPath.c_str(), folderContext.strEffectiveParentPath.c_str(), folderContext.strName.c_str());
  
  unsigned int iResolvedItems = 0;

  // TODO: Merge the two vector and file item list implementations
  std::vector<CFileItemPtr> vecVideoFiles;
  CFileItemList videoFileList; // used for subsequent folder level processing
  
  std::map<std::string, BXMetadata*> mapResolvedByFolder;
  std::map<std::string, std::pair<std::string, std::string> > mapHashSizeToImdbId;

  BOXEE::BXMetadataEngine& MDE = BOXEE::Boxee::GetInstance().GetMetadataEngine();

  int iFolderId = MDE.GetMediaFolderId(strFolderPath, "videoFolder");

  MDE.MarkNewFolderFilesAsProcessing(iFolderId);

  int iVideoId = -1;
  bool bTwoPartFolder = false;

  // Retrieve the list of all items from the path
  CFileItemList items;
  if (!CDirectory::GetDirectory(strFolderPath, items)) 
  {
    return RESOLVER_ABORTED;
  }

  // 1. Purge deleted files and folders from the database
  PurgeDeletedVideosFromDatabase(strFolderPath, items);

  // 2. Check if this is a DVD folder
  int iResult = RESOLVER_FAILED;

  // The function analyzes the folder, performs the resolving and
  // sets the iResult output parameter, in order to avoid multiple folder reads
  if (ResolveDvdFolder(strFolderPath, iFolderId, items, iResult))
  {
    return iResult;
  }

  if (ResolveBlurayFolder(strFolderPath, iFolderId, items, iResult))
  {
    return iResult;
  }

  //I. Phase 1: Get all video files from the folder that should be resolved
  for (int i=0; i < items.Size() && !m_bStopped; i++)
  {
    // Lock the job before going into next iteration, if the job was paused this call will block
    pJob->Lock();

    CFileItemPtr pItem = items[i];

    if (!pItem->IsHidden() && pItem->IsVideo() && !pItem->IsPlayList())
    {      
      if (pItem->GetPropertyBOOL("existsInDB"))
      {
        // file already exists, should not be resolved
        continue;
      }
      vecVideoFiles.push_back(pItem);
      videoFileList.Add(pItem);
    }
  } //for

  // Handles the case of folder with two parts of the same video
  if (ResolveTwoVideoFolder(vecVideoFiles))
  {
    bTwoPartFolder = true;
    // Sort by path, which in most cases is enough to determine the correct ordering
    if (strcmp(vecVideoFiles[0]->m_strPath.c_str(), vecVideoFiles[1]->m_strPath.c_str()) > 0)
    {
       CFileItemPtr temp = vecVideoFiles[0];
       vecVideoFiles[0] = vecVideoFiles[1];
       vecVideoFiles[1] = temp;
    }
  }

  // Determine the longest common prefix of all videos in the folder, in attempt to determine whether this is a TV series folder

  // TODO: Future, maybe there could be a possible improvement of the algorithm to handle the cases with multiple series in the same folder
  // The idea is to perform bucket sort where each bucket eventually holds the entries with longest common substrings

//  std::map<CStdString, int> mapPrefixes;
//  videoFileList.Sort(SORT_METHOD_TITLE, SORT_ORDER_ASC);
//
//  for (int i = 0; i < videoFileList.Size(); i++)
//  {
//    for (int j = i + 1; j < videoFileList.Size(); j++)
//    {
//      CStdString strPrefix = CUtil::LongestCommonPrefix(videoFileList.Get(i)->GetLabel(), videoFileList.Get(j)->GetLabel());
//      if (strPrefix.IsEmpty())
//        break;
//
//      std::map<CStdString, int>::iterator it = mapPrefixes.find(strPrefix);
//      if (it == mapPrefixes.end())
//      {
//        mapPrefixes[strPrefix] = 1;
//      }
//      else
//      {
//         int count = mapPrefixes[strPrefix];
//         mapPrefixes[strPrefix] = count + 1;
//      }
//    }
//  }

  // II. Phase 2: Go over all video items in the folder and resolve them

  for (size_t i=0; i < vecVideoFiles.size() && !m_bStopped; i++)
  {
    // Lock the job before going into next iteration, if the job was paused this call will block
    pJob->Lock();

    CFileItemPtr pItem = vecVideoFiles[i];

    CStdString strPath = _P(pItem->m_strPath);
#ifndef HAS_UPNP_AV
    CVideoInfoTag* vtag = 
#endif
      pItem->GetVideoInfoTag();

    // TODO: Check whether this update is really necessary?
    MDE.UpdateVideoFileStatus(strPath, STATUS_RESOLVING);

    if (CMetadataResolverVideo::IsSample(strPath) || CMetadataResolverVideo::IsTrailer(strPath))
    {
      // we should not resolve items that are samples (if they have the word "sample" in the name
      MDE.UpdateVideoFileStatus(strPath, STATUS_UNRESOLVED);
      continue;
    }

    CVideoFileContext context(strPath);
    bool bUpdateVideoFileStatus = !CUtil::IsUPnP(strPath);
#ifndef HAS_UPNP_AV
    if( vtag && CUtil::IsUPnP(strPath))
    {
      // For now, only do this for UPnP content. For everything else rely on filenames
      context.LoadWithTag(vtag);
      CUtil::UrlDecode(context.strPath);
      context.Load();
    }
    else
#endif
    {
      bUpdateVideoFileStatus = true;
      context.Load();
    }
    context.folderContext = folderContext;
    context.m_userVideoCoverPath = BoxeeUtils::GetUserVideoThumbPath(strPath,&items);

    // files less than a user specified size or with RAR extension shouldn't be resolved.
    // UPnP files or files with unknown file size should always be resolved (typically upnp content server)
    if (bUpdateVideoFileStatus && ((context.iSize != -1 && context.iSize < g_guiSettings.GetInt("myvideos.videosize") * 1024 * 1024) || pItem->IsRAR()))
    {
      MDE.UpdateVideoFileStatus(strPath, STATUS_UNRESOLVED);
      continue;
    }

    // Create new empty metadata object to hold the result of the resolving
    BXMetadata* pMetadata = new BXMetadata(MEDIA_ITEM_TYPE_VIDEO);

    // Set fields that are necessary for further item processing
    BXVideo* pVideo = (BXVideo*)pMetadata->GetDetail(MEDIA_DETAIL_VIDEO);
    pMetadata->SetPath(strPath);

    // We set the media file id to 1, to indicate that this is a real file (opposite to virtual entries from feed, queue etc...)
    pMetadata->SetMediaFileId(1);

    pVideo->m_iDateModified = vecVideoFiles[i]->GetPropertyInt("modificationDate");
    pVideo->m_strPath = strPath;
    pVideo->m_iFolderId = iFolderId;

    bool bResult = CMetadataResolverVideo::ResolveVideo(context, pMetadata);

    if (bResult) 
    {
      CLog::Log(LOGDEBUG, "CMetadataResolver::ResolveVideoFolder, Resolved metadata for video, path = %s, name = %s  (resolver) (videoresolver)", context.strPath.c_str(), context.strName.c_str());
      pMetadata->m_bResolved = true;

      // We want to keep track of videos that were resolved by their folder name (context.bResolvedByFolder is set). In case
      // more than 3 videos in the same folder were resovled by folder name, we do not add them (this is probably a case of home
      // videos folder which we want to avoid)
      if (context.bResolvedByFolder && !bTwoPartFolder) 
      {
        // Add the file to the temporary map
        CLog::Log(LOGDEBUG, "CMetadataResolver::ResolveVideoFolder, Video was resolved by folder, path = %s, name = %s, id = %s  (resolver) (videoresolver)",
            context.strPath.c_str(), context.strName.c_str(), pVideo->m_strBoxeeId.c_str());

        mapResolvedByFolder[context.strPath] = new BXMetadata(*pMetadata);
      }
      else 
      {
        iResolvedItems++;

        CLog::Log(LOGDEBUG, "CMetadataResolver::ResolveVideoFolder, Video was resolved by name, adding to DB, path = %s, name = %s, id = %s  (resolver) (videoresolver)",
            context.strPath.c_str(), context.strName.c_str(), pVideo->m_strBoxeeId.c_str());

        int iPart;

        if (bTwoPartFolder) 
        {
          // Clear case of two part folder (CD1, CD2)
          pVideo->m_strType = "part";
          pVideo->m_strPath = strFolderPath;
          pMetadata->SetPath(strFolderPath);
          if (iVideoId == -1) 
          {
            CacheThumb(pMetadata->GetPath(), pVideo->m_strCover);
            iVideoId = MDE.AddVideo(pMetadata);

            if (iVideoId == MEDIA_DATABASE_ERROR)
            {
              continue;
            }
          }

          // Add part to the database
          MDE.AddPart(iVideoId, i, strPath);
          
          // Mark video file as resolved
          MDE.UpdateVideoFileStatus(strPath, STATUS_RESOLVED);
        }
        else if (CMetadataResolverVideo::CheckPart(pVideo->m_strPath, iPart)) 
        {
          // Multi part folder CD1, CD2, CD3 (rare, probably can be removed)
          pVideo->m_strType = "part";
          pVideo->m_strPath = strFolderPath;
          pMetadata->SetPath(strFolderPath);

          CacheThumb(pMetadata->GetPath(), pVideo->m_strCover);

          // Add the entire video item to the database and get the video id
          iVideoId = MDE.AddVideo(pMetadata);
          if (iVideoId == MEDIA_DATABASE_ERROR)
          {
            continue;
          }

          // Add the specific part
          MDE.AddPart(iVideoId, iPart, strPath);

          // Mark video file as resolved
          MDE.UpdateVideoFileStatus(strPath, STATUS_RESOLVED);
        }
        else 
        {
          if (!pVideo->m_bMovie)
          {
            // TV series, series thumb should be cached as well
            BXSeries* pSeries = (BXSeries*)pMetadata->GetDetail(MEDIA_DETAIL_SERIES);
            if (pSeries)
            {
              // Create series path
              std::string strSeriesPath = "boxeedb://series/";
              strSeriesPath += pSeries->m_strBoxeeId;
              strSeriesPath += "/";
              CacheThumb(strSeriesPath, pSeries->m_strCover);
            }
          }
          CacheThumb(pMetadata->GetPath(), pVideo->m_strCover);

          // Add the video file to the database
          iVideoId = MDE.AddVideo(pMetadata);
          if (iVideoId == MEDIA_DATABASE_ERROR)
          {
            continue;
          }

          // Mark video file as resolved
          MDE.UpdateVideoFileStatus(pVideo->m_strPath, STATUS_RESOLVED);
        }
      }
    }
    else
    {
      CLog::Log(LOGDEBUG, "CMetadataResolver::ResolveVideoFolder, Could not resolve metadata for video, path = %s, name = %s (resolver) (videoresolver)", context.strPath.c_str(), context.strName.c_str());
      MDE.UpdateVideoFileStatus(strPath, STATUS_UNRESOLVED); //use the path before it was decoded, for UPnP support
    }

    delete pMetadata;

  } // for

  // In case there were fewer than 3 videos resolved by folder add them
  if (mapResolvedByFolder.size() < 4) 
  {
    for (std::map<std::string, BXMetadata*>::iterator i = mapResolvedByFolder.begin(); i != mapResolvedByFolder.end(); i++)
    {
      iResolvedItems++;
      i->second->m_bResolved = true;

      CacheThumb(i->second->GetPath(), ((BXVideo*)i->second->GetDetail(MEDIA_DETAIL_VIDEO))->m_strCover);

      iVideoId = MDE.AddVideo(i->second);
      if (iVideoId == MEDIA_DATABASE_ERROR)
      {
        continue;
      }

      MDE.UpdateVideoFileStatus(i->second->GetPath(), STATUS_RESOLVED);
      delete i->second;
    }
    mapResolvedByFolder.clear();
  }
  else 
  {
    for (std::map<std::string, BXMetadata*>::iterator i = mapResolvedByFolder.begin(); i != mapResolvedByFolder.end(); i++)
    {
      MDE.UpdateVideoFileStatus(i->second->GetPath(), STATUS_UNRESOLVED);
      delete i->second;
    }
    mapResolvedByFolder.clear();
  }

  // Finished resolving, update folder status
  int process_counter = MDE.GetShareUnresolvedVideoFilesCountByFolder(iFolderId, STATUS_PROCESSING);

  if (process_counter > 0 )
  {
    CLog::Log(LOGDEBUG,"CMetadataResolver::FinishResolveVideoFolder, folder %s still have processing files - resolve it next path  (videoresolver)", strFolderPath.c_str());
    MDE.MarkFolderNew(strFolderPath);
  }

  CLog::Log(LOGDEBUG, "CMetadataResolver::ResolveVideoFolder, Updating folder status, path = %s, items = %d, resolved = %d (resolver) (videoresolver)",
          strFolderPath.c_str(), (int)vecVideoFiles.size(), iResolvedItems);

  if (vecVideoFiles.size() > iResolvedItems)
  {
    // Mark folder as unresolved
    BOXEE::Boxee::GetInstance().GetMetadataEngine().MarkFolderUnresolved(strFolderPath , false);
    return RESOLVER_FAILED;
  }
  else 
  {
    BOXEE::Boxee::GetInstance().GetMetadataEngine().MarkFolderResolved(strFolderPath, -1 /* no metadata id */);
    return RESOLVER_SUCCESS;
  }
}

void CMetadataResolver::CacheThumb(const CStdString& strItemPath, const CStdString& strThumbnailPath, bool bOverwrite)
{
  CLog::Log(LOGDEBUG, "CMetadataResolver::CacheThumb, strItemPath = %s, strThumbnailPath = %s (thumb)", strItemPath.c_str(), strThumbnailPath.c_str());
  if (g_application.m_bStop) return;

  if (strThumbnailPath == "")
    return;

  CFileItemPtr tmpItem(new CFileItem(strItemPath, false));

  CStdString cachedThumbPath = tmpItem->GetCachedVideoThumb();
  CLog::Log(LOGDEBUG, "CMetadataResolver::CacheThumb, strThumbnailPath = %s, cached path = %s (thumb)", strThumbnailPath.c_str(), cachedThumbPath.c_str());

  if (bOverwrite)
  {
    CLog::Log(LOGDEBUG, "CMetadataResolver::CacheThumb, delete previous thumb = %s (thumb)", cachedThumbPath.c_str());
    XFILE::CFile::Delete(cachedThumbPath);
  }

  CPicture::CreateThumbnail(strThumbnailPath, tmpItem->GetCachedVideoThumb(), true);
}

void CMetadataResolver::CleanDeletedFolders(const std::set<std::string> &setFolders)
{
  std::set<std::string>::const_iterator it = setFolders.begin();
  for (;it != setFolders.end(); it++) 
  {
    std::string path = *it;
    BOXEE::Boxee::GetInstance().GetMetadataEngine().RemoveFolderByPath(path); // this is the only table that doesnt have trigger - will be fixed later ?
    BOXEE::Boxee::GetInstance().GetMetadataEngine().RemoveAudioByFolder(path);
  }
}

void CMetadataResolver::CleanDeletedVideos(const std::map<std::string, BXMetadata*> &mapDeletedVideos)
{
  // Go over the remainining files in the map and remove redundant ones

  // Create a map for series that should be checked and removed if all episodes from it has been removed
  std::map<int, int> mapSeries;

  std::map<std::string, BXMetadata*>::const_iterator it = mapDeletedVideos.begin();
  for (;it != mapDeletedVideos.end(); it++)
  {
    CLog::Log(LOGINFO, "LIBRARY, ERASE, Remove video, path = %s", it->first.c_str());
    BOXEE::Boxee::GetInstance().GetMetadataEngine().RemoveVideoByPath(it->first);
    BOXEE::Boxee::GetInstance().GetMetadataEngine().RemoveUnresolvedVideoByPath(it->first);
    BXMetadata* pMetadata = it->second;
    BXVideo* pVideo = (BXVideo*)pMetadata->GetDetail(MEDIA_DETAIL_VIDEO);
    BXSeries* pSeries = (BXSeries*)pMetadata->GetDetail(MEDIA_DETAIL_SERIES);

    // Get the series and store it in the map
    if (pSeries->m_iId != -1)
    {
      mapSeries[pSeries->m_iId] = pVideo->m_iSeason;
    }

    delete it->second;
  }

  // Go over all series and determine whether there are any more episodes in them
  std::map<int, int>::iterator it2 = mapSeries.begin();
  for (;it2 != mapSeries.end(); it2++)
  {
    BOXEE::Boxee::GetInstance().GetMetadataEngine().RemoveSeries(it2->first, it2->second);
  }
}

void CMetadataResolver::UpdateResolveLabel(const CStdString& path)
{
  CGUIWindowBoxeeMediaSources* pDlgSourceSource = (CGUIWindowBoxeeMediaSources*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_SOURCES);
  if (pDlgSourceSource && pDlgSourceSource->IsVisible())
  {
    pDlgSourceSource->UpdateResolveLabel(path);
  }
}

