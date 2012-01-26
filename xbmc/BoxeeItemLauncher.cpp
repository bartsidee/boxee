/*
 * BoxeeItemLauncher.cpp
 *
 *  Created on: Jul 13, 2009
 *      Author: yuvalt
 */


#include "BoxeeItemLauncher.h"
#include "GUIWindowManager.h"
#include "GUIDialogBoxeeMediaAction.h"
#include "AppManager.h"
#include "Application.h"
#include "lib/libBoxee/bxutils.h"
#include "lib/libBoxee/boxee.h"
#include "FileSystem/BoxeeDatabaseDirectory.h"
#include "Util.h"
#include "lib/libPython/XBPython.h"
#include "URL.h"
#include "BoxeeUtils.h"
#include "SpecialProtocol.h"
#include "FileSystem/cdioSupport.h"
#include "DetectDVDType.h"
#include "utils/log.h"
#include "VideoInfoTag.h"

using namespace std;
using namespace BOXEE;
using namespace DIRECTORY;

bool CBoxeeItemLauncher::ExecutePlayableFolder (const CFileItem& item)
{
  CStdString strPath = item.m_strPath;

  CLog::Log(LOGDEBUG,"CBoxeeItemLauncher::ExecutePlayableFolder - checking for [path=%s] (epf)",strPath.c_str());

  if (strPath.IsEmpty())
  {
    return false;
  }

  IsPlayableFolderJob* pPlayableFolderJob = new IsPlayableFolderJob(strPath);
  Job_Result jobResult = CUtil::RunInBG(pPlayableFolderJob,false);

  if (jobResult == JOB_SUCCEEDED)
  {
    CPlayableFolderType::PlayableFolderEnums result = pPlayableFolderJob->m_result;

    delete pPlayableFolderJob;

    if (result == CPlayableFolderType::PF_BLURAY)
    {
      CLog::Log(LOGDEBUG,"CBoxeeItemLauncher::ExecutePlayableFolder - [path=%s] is a Bluray folder. (epf)",strPath.c_str());
      PlayFolder(item,0);
      return true;
    }
    else if (result == CPlayableFolderType::PF_DVD)
    {
      CLog::Log(LOGDEBUG,"CBoxeeItemLauncher::ExecutePlayableFolder - [path=%s] is a DVD folder. (epf)",strPath.c_str());
      PlayFolder(item,1);
      return true;
    }
    else if (result == CPlayableFolderType::PF_NO)
    {
      CLog::Log(LOGDEBUG,"CBoxeeItemLauncher::ExecutePlayableFolder - [path=%s] is not a DVD or Bluray folder. (epf)",strPath.c_str());
      return false;
    }
  }
  else if (jobResult == JOB_ABORTED)
  {
    CLog::Log(LOGDEBUG,"CBoxeeItemLauncher::ExecutePlayableFolder - [path=%s] query was aborted. (epf)",strPath.c_str());
    return true;
  }

  CLog::Log(LOGDEBUG,"CBoxeeItemLauncher::ExecutePlayableFolder - [path=%s] is not a DVD or Bluray folder. (epf)",strPath.c_str());
  return false;
}

bool CBoxeeItemLauncher::Launch(const CFileItem& item)
{
  if (item.GetPropertyBOOL("addshare")) 
  {
    g_windowManager.ActivateWindow(WINDOW_BOXEE_MEDIA_SOURCES);
    return true;
  }
#ifndef HAS_EMBEDDED
  else if (item.IsScript()) 
  {
    RunScript(item.m_strPath);
  }
#endif
  else if (item.IsApp()) 
  {
    if ((item.GetProperty("FeedTypeItem") == MSG_ACTION_TYPE_FEATURED) && (item.GetProperty("rts-mediatype").IsEmpty()))
    {
      // ugly hack - in case of app or section within an app -> Launch the app
      CAppManager::GetInstance().Launch(item, false);
    }
    else if (item.GetPropertyBOOL("IsSearchItem") || CAppManager::GetInstance().IsPlayable(item.m_strPath))
    {
      return CGUIDialogBoxeeMediaAction::ShowAndGetInput(&item);
    }
    else
    {
      CAppManager::GetInstance().Launch(item, false);
    }
  }
  else if (item.IsPicture()) 
  {    
    return LaunchPictureItem(item);
  }
  else if (item.GetPropertyBOOL("isalbum") && item.HasMusicInfoTag()) 
  {
    return CGUIDialogBoxeeMediaAction::ShowAndGetInput(&item);
  }
  else if (item.GetPropertyBOOL("istvshowfolder") && !item.GetProperty("boxeeid").IsEmpty())
  {
    CStdString strSeriesUrl;
    CStdString strTitle = item.GetLabel();

    CUtil::URLEncode(strTitle);

    strSeriesUrl.Format("boxeeui://tvshows/?seriesId=%s&title=%s", item.GetProperty("boxeeid").c_str(), strTitle.c_str());

    g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_TVEPISODES, strSeriesUrl);
    return true;
  }
  else if ((item.GetPropertyBOOL("isvideo") || (item.GetProperty("isvideo") == "true")) && item.HasVideoInfoTag() && !item.m_bIsFolder) 
  {
    if (!ExecutePlayableFolder(item))
      return CGUIDialogBoxeeMediaAction::ShowAndGetInput(&item);
    else
      return true;
  }
  else if ((item.GetPropertyBOOL("isinternetstream") || item.GetPropertyBOOL("isrss") || item.GetPropertyBOOL("isradio")) && !item.m_bIsFolder) 
  {
    return CGUIDialogBoxeeMediaAction::ShowAndGetInput(&item);
  }
  else if (item.GetPropertyBOOL("isplugin") && !item.m_bIsFolder) 
  {
    return CGUIDialogBoxeeMediaAction::ShowAndGetInput(&item);
  }
  else if (item.GetPropertyBOOL("photoslauncher"))
  {
    g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_PHOTOS);
  }
  else if (item.GetPropertyBOOL("musiclauncher"))
  {
    g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_ALBUMS);
  }
  else if (item.GetPropertyBOOL("videolauncher"))
  {
    g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_MOVIES , "boxeeui://movies/?category=local");
  }
  else if (item.GetPropertyBOOL("showslauncher"))
  {
    g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_TVSHOWS , "boxeeui://shows/?category=local");
  }
  else if ((item.m_bIsFolder) && (!item.IsPlayList()))
  {
    if (item.GetPropertyBOOL("ishistory") && !item.GetProperty("originWindowId").IsEmpty())
    {
      g_windowManager.ActivateWindow(atoi(item.GetProperty("originWindowId")), item.m_strPath);
    }
    else
    {
      return ExecutePlayableFolder(item);
    }
  }
  else
  {
    if(item.IsAudio())
    {
      CLog::Log(LOGDEBUG,"Intend to play file [%s]. Going to build a playlist from all of the audio files in the directory (apl)", (item.m_strPath).c_str());

      CFileItem newItem(item);
      BXMetadata metadata(MEDIA_ITEM_TYPE_AUDIO);
      if (BOXEE::Boxee::GetInstance().GetMetadataEngine().GetAudioByPath(_P(newItem.m_strPath), &metadata) == MEDIA_DATABASE_OK)
      {
        DIRECTORY::CBoxeeDatabaseDirectory::CreateAudioItem(&metadata, &newItem);

        //Set the correct path so that the album can be found,
        //CStdString strAlbumPath = "boxeedb://album/?id=";
        //strAlbumPath += BXUtils::IntToString(((BXAlbum*)metadata.GetDetail(MEDIA_DETAIL_ALBUM))->m_iId);
        //strAlbumPath += "/";

        //newItem.m_strPath = strAlbumPath;

        // Set the property with the track number that should start playing
        newItem.SetProperty("PlayTrack", ((BXAudio*)metadata.GetDetail(MEDIA_DETAIL_AUDIO))->m_iTrackNumber);
      }

      newItem.SetProperty("isFolderItem", true);

      //newItem.Dump();

      CGUIDialogBoxeeMediaAction::OnPlayAudioFromFolder(newItem);
    }
    else
    {
      //g_application.GetBoxeeItemsHistoryList().AddItemToHistory(item);
      CFileItem newItem(item);
      newItem.SetProperty("isFolderItem", true);

      CLog::Log(LOGDEBUG,"In CGUIDialogBoxeeMediaAction::OnPlay - Going to play file from folder mode, path = %s", item.m_strPath.c_str());

      item.Dump();
      CGUIDialogBoxeeMediaAction::ShowAndGetInput(&newItem);
      //OnPlayMedia(item);
    }
  }

  return true;  
}

void CBoxeeItemLauncher::RunScript(const CStdString& strPath) 
{
  CURI url(strPath);

  // path is special://home/scripts<path from here>
  CStdString pathToScript = "special://home/scripts";
  CUtil::AddFileToFolder(pathToScript, url.GetHostName(), pathToScript);
  fflush(stdout);

  CUtil::AddFileToFolder(pathToScript, "default.py", pathToScript);

  // setup our parameters to send the script
  CStdString strHandle;
  strHandle.Format("%i", -1);

  // run the script
  if (g_pythonParser.evalFile(pathToScript.c_str()) < 0)
    CLog::Log(LOGERROR, "Unable to run script %s", pathToScript.c_str());

  //g_windowManager.PreviousWindow();
}

void CBoxeeItemLauncher::PlayDVD()
{
#ifdef HAS_DVD_DRIVE
  CFileItem dvdItem;
  dvdItem.m_strPath = "iso9660://";
  dvdItem.SetProperty("isdvd", true);
  dvdItem.SetThumbnailImage("defaultmusicalbum.png");
  
  CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();
  
  CStdString strDiscLabel = g_application.CurrentFileItem().GetLabel();
  if (pInfo)
    strDiscLabel = pInfo->GetDiscLabel();
  
  strDiscLabel.Trim();
  
  if (strDiscLabel == "")
    strDiscLabel = "DVD";
  
  dvdItem.SetLabel(strDiscLabel);
  
  CGUIDialogBoxeeMediaAction::ShowAndGetInput(&dvdItem);
#endif
}

bool CBoxeeItemLauncher::LaunchPictureItem(const CFileItem& item)
{
  int activeWindow = g_windowManager.GetActiveWindow();
  if (activeWindow == WINDOW_BOXEE_BROWSE_QUEUE)
  {
    return CGUIDialogBoxeeMediaAction::ShowAndGetInput(&item);
  }
  else
  {
    g_application.GetBoxeeItemsHistoryList().AddItemToHistory(item);

    if (CUtil::IsHTTP(item.m_strPath))
    {
      BoxeeUtils::PlayPicture(item);
    }
    else
    {
      CStdString parentPath = item.m_strPath;
      CUtil::GetParentPath(item.m_strPath, parentPath);
      CGUIDialogBoxeeMediaAction::RunSlideshow(parentPath, item.m_strPath);
    }
  }

  return true;
}

bool CBoxeeItemLauncher::PlayFolder(const CFileItem& item, bool isDVD)
{
  CFileItem newItem(item);

  newItem.SetProperty("isFolderItem", true);

  newItem.SetProperty("HasNextItem", true);
  newItem.SetProperty("HasPrevItem", true);

  newItem.SetPlayableFolder(isDVD);

  BOXEE::Boxee::GetInstance().GetMetadataEngine().AddPlayableFolder(item.m_strPath);

  CLog::Log(LOGINFO,"CBoxeeItemLauncher::PlayFolder - Going to play %s folder [%s]", isDVD ? "DVD" : "Bluray", (newItem.m_strPath).c_str());

  CGUIDialogBoxeeMediaAction::ShowAndGetInput(&newItem);

  return true;
}

