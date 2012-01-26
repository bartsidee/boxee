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
#include "GUIWindowBoxeeMain.h"

using namespace std;
using namespace BOXEE;
using namespace DIRECTORY;

bool CBoxeeItemLauncher::Launch(const CFileItem& item)
{
  if (item.GetPropertyBOOL("addshare")) 
  {
    g_windowManager.ActivateWindow(WINDOW_BOXEE_MEDIA_SOURCES);
    return true;
  }
  else if (item.IsScript()) 
  {
    RunScript(item.m_strPath);
  }
  else if (item.IsApp()) 
  {
    if (CAppManager::GetInstance().IsPlayable(item.m_strPath))
    {
      CGUIDialogBoxeeMediaAction::ShowAndGetInput(&item);
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
    CGUIDialogBoxeeMediaAction::ShowAndGetInput(&item);
  }
  else if (item.GetPropertyBOOL("istvshowfolder") && !item.GetProperty("boxeeid").IsEmpty())
  {
    // We always show both local and remote episodes
    CStdString strPath = "boxee://tvshows/episodes?local=true&remote=true&seriesId=";
    strPath += item.GetProperty("boxeeid");
    g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_TVEPISODES, strPath);
    return true;
  }
  else if ((item.GetPropertyBOOL("isvideo") || (item.GetProperty("isvideo") == "true")) && item.HasVideoInfoTag() && !item.m_bIsFolder) 
  {
    CGUIDialogBoxeeMediaAction::ShowAndGetInput(&item);

  }
  else if ((item.GetPropertyBOOL("isinternetstream") || item.GetPropertyBOOL("isrss") || item.GetPropertyBOOL("isradio")) && !item.m_bIsFolder) 
  {
    CGUIDialogBoxeeMediaAction::ShowAndGetInput(&item);
  }
  else if (item.GetPropertyBOOL("isplugin") && !item.m_bIsFolder) 
  {
    CGUIDialogBoxeeMediaAction::ShowAndGetInput(&item);
  }
  else if ((item.m_bIsFolder) && (!item.IsPlayList()))
  {
    if (item.GetPropertyBOOL("ishistory") && !item.GetProperty("originWindowId").IsEmpty())
    {
      g_windowManager.ActivateWindow(atoi(item.GetProperty("originWindowId")), item.m_strPath);
    }
    else
    {
      return false;
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

        // Set the correct path so that the album can be found,
//        CStdString strAlbumPath = "boxeedb://album/?id=";
//        strAlbumPath += BXUtils::IntToString(((BXAlbum*)metadata.GetDetail(MEDIA_DETAIL_ALBUM))->m_iId);
//        strAlbumPath += "/";
//
//        newItem.m_strPath = strAlbumPath;

        // Set the property with the track number that should start playing
        newItem.SetProperty("PlayTrack", ((BXAudio*)metadata.GetDetail(MEDIA_DETAIL_AUDIO))->m_iTrackNumber);
      }

      newItem.SetProperty("isFolderItem", true);

      //newItem.Dump();

      CGUIDialogBoxeeMediaAction::ShowAndGetInput(&newItem);
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
  CURL url(strPath);

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
  CGUIWindowBoxeeMain* pHomeWindow = (CGUIWindowBoxeeMain*)g_windowManager.GetWindow(WINDOW_HOME);
  if (!pHomeWindow)
  {
    CLog::Log(LOGERROR,"CBoxeeItemLauncher::LaunchPictureItem - FAILED to get WINDOW_HOME. item [label=%s][path=%s][boxeeId=%s] (bma)",item.GetLabel().c_str(),item.m_strPath.c_str(),item.GetProperty("boxeeid").c_str());
  }

  int activeWindow = g_windowManager.GetActiveWindow();
  if ((activeWindow == WINDOW_BOXEE_BROWSE_QUEUE) || ((activeWindow == WINDOW_HOME) && pHomeWindow && (pHomeWindow->GetFocusedControlID() == LIST_QUEUE)))
  {
    CGUIDialogBoxeeMediaAction::ShowAndGetInput(&item);
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
