#include "GUIWindowBoxeeMediaSources.h"
#include "Application.h"
#include "GUIWindowBoxeeMediaSourceInfo.h"
#include "GUIWindowBoxeeMediaSourceAddShare.h"
#include "BoxeeMediaSourceList.h"
#include "GUIWindowManager.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "utils/log.h"
#include "Util.h"
#include "lib/libBoxee/bxvideodatabase.h"
#include "lib/libBoxee/bxaudiodatabase.h"
#include "lib/libBoxee/bxmetadata.h"
#include "lib/libBoxee/boxee.h"
#include "BoxeeUtils.h"
#include "boxee.h"
#include "SpecialProtocol.h"
#include "GUIListContainer.h"


#define CONTROL_ADD_LOCAL_SOURCES        51
#define CONTROL_SHOW_UNIDENTIFIED      52
#define CONTROL_NETWORK_APPLICATIONS 53
#define CONTROL_MANUALLY_ADD_SOURCES 54
#define CONTROL_SOURCE_LIST              56
#define CONTROL_SCAN_LABEL              151
#define CONTROL_RESOLVE_LABEL           161

CGUIWindowBoxeeMediaSources::CGUIWindowBoxeeMediaSources(void) : CGUIDialog(WINDOW_BOXEE_MEDIA_SOURCES, "boxee_media_sources.xml"), m_renderCount(0)
{

}

CGUIWindowBoxeeMediaSources::~CGUIWindowBoxeeMediaSources(void)
{

}

void CGUIWindowBoxeeMediaSources::Render()
{
  CGUIDialog::Render();

  m_renderCount++;

  if (m_renderCount > 120)
  {
    Refresh();
    m_renderCount = 0;
  }
}

bool CGUIWindowBoxeeMediaSources::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    int iControl = message.GetSenderId();
    if(iControl == CONTROL_ADD_LOCAL_SOURCES)
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSources::OnMessage - Enter GUI_MSG_CLICKED case with [iControl=CONTROL_ADD_LOCAL_SOURCES] (msmk)");

      CGUIWindowBoxeeMediaSourceAddShare *pDlgSourceInfo = (CGUIWindowBoxeeMediaSourceAddShare*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_SOURCE_ADD_SHARE);
      if (pDlgSourceInfo)
      {
        m_selectedSource = SOURCE_LOCAL;
        OpenGUIWindow(WINDOW_BOXEE_MEDIA_SOURCE_ADD_SHARE);
      }
    }
    else if(iControl == CONTROL_SHOW_UNIDENTIFIED)
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSources::OnMessage - Enter GUI_MSG_CLICKED case with [iControl=CONTROL_SHOW_UNIDENTIFIED] (msmk)");
      g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_LOCAL, "boxeeui://files/?path=boxeedb://unresolvedVideoFiles");

    }
    else if(iControl == CONTROL_NETWORK_APPLICATIONS)
    {
      m_selectedSource = SOURCE_NETWORK_APPLICATIONS;

      OpenGUIWindow(WINDOW_BOXEE_MEDIA_SOURCE_LIST);
      return true;
    }
    else if(iControl == CONTROL_MANUALLY_ADD_SOURCES)
    {
      m_selectedSource = MANUALLY_ADD_SOURCE;

      CGUIWindowBoxeeMediaSourceInfo* pDlgSourceInfo = (CGUIWindowBoxeeMediaSourceInfo*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_SOURCE_INFO);

      if(pDlgSourceInfo)
      {
        pDlgSourceInfo->SetAddSource("");
        pDlgSourceInfo->SetSourceThumbPath("");
        pDlgSourceInfo->SetEnableLocationEdit(false);

        OpenGUIWindow(WINDOW_BOXEE_MEDIA_SOURCE_INFO);
        return true;
      }
    }
    else if(iControl == CONTROL_SOURCE_LIST)
    {
      if (message.GetParam1() != ACTION_BUILT_IN_FUNCTION)
      {
        // Handle only GUI_MSG_CLICKED on CONTROL_SOURCE_LIST that origin from navigation

        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSources::OnMessage - Enter GUI_MSG_CLICKED case with [SenderId=CONTROL_SOURCE_LIST]. Going to call ProccessItemSelectedInControlSourceList() (msmk)");

        ProccessItemSelectedInControlSourceList();

        return true;
      }
    }
    else
    {
      CLog::Log(LOGERROR,"In CGUIWindowBoxeeMediaSources::OnMessage - Failed to handle in [ACTION_SELECT_ITEM] an unknown FocusedControlID [%d]",iControl);
    }
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIWindowBoxeeMediaSources::OnAction(const CAction &action)
{
  //  int iControl = GetFocusedControlID();

  //  bool bSelectAction = ((action.id == ACTION_SELECT_ITEM) || (action.id == ACTION_MOUSE_LEFT_CLICK));
  //
  //  CLog::Log(LOGDEBUG, "CGUIWindowBoxeeMediaSources::OnAction, action id = %ld", action.id);
  //
  //  if(bSelectAction)
  //  {
  //    if(iControl == CONTROL_LOCAL_SOURCES)
  //    {
  //      m_selectedSource = SOURCE_LOCAL;
  //
  //      OpenGUIWindow(WINDOW_BOXEE_MEDIA_SOURCE_LIST);
  //      return true;
  //    }
  //    else if(iControl == CONTROL_NETWORK_SOURCES)
  //    {
  //      m_selectedSource = SOURCE_NETWORK;
  //
  //      OpenGUIWindow(WINDOW_BOXEE_MEDIA_SOURCE_LIST);
  //      return true;
  //    }
  //    else if(iControl == CONTROL_NETWORK_APPLICATIONS)
  //    {
  //      m_selectedSource = SOURCE_NETWORK_APPLICATIONS;
  //
  //      OpenGUIWindow(WINDOW_BOXEE_MEDIA_SOURCE_LIST);
  //      return true;
  //    }
  //    else if(iControl == CONTROL_MANUALLY_ADD_SOURCES)
  //    {
  //      m_selectedSource = MANUALLY_ADD_SOURCE;
  //
  //      CGUIWindowBoxeeMediaSourceInfo* pDlgSourceInfo = (CGUIWindowBoxeeMediaSourceInfo*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_SOURCE_INFO);
  //
  //      if(pDlgSourceInfo)
  //      {
  //        pDlgSourceInfo->SetAddSource("");
  //        pDlgSourceInfo->SetSourceThumbPath("");
  //        pDlgSourceInfo->SetEnableLocationEdit(true);
  //
  //        OpenGUIWindow(WINDOW_BOXEE_MEDIA_SOURCE_INFO);
  //        return true;
  //      }
  //    }
  //    else
  //    {
  //      CLog::Log(LOGERROR,"In CGUIWindowBoxeeMediaSources::OnAction - Failed to handle in [ACTION_SELECT_ITEM] an unknown FocusedControlID [%d]",iControl);
  //    }
  //  }
  //  else
  if (action.id == ACTION_PREVIOUS_MENU)
  {
    Close();
    return true;
  }
  else if (action.id == ACTION_PARENT_DIR)
  {
    Close();
    return true;
  }

  return CGUIWindow::OnAction(action);
}

void CGUIWindowBoxeeMediaSources::OnInitWindow()
{
  CGUIWindow::OnInitWindow();

  LoadAllShares();
  m_renderCount = 0;
}

int CGUIWindowBoxeeMediaSources::getSelectedSource()
{
  return m_selectedSource;
}

void CGUIWindowBoxeeMediaSources::OpenGUIWindow(int windowID)
{
  g_windowManager.ActivateWindow(windowID);
}

void CGUIWindowBoxeeMediaSources::Refresh()
{
  LoadAllShares();
}
void CGUIWindowBoxeeMediaSources::LoadAllShares()
{
  time_t                 LastScanned;
  int                    SavedFocusedControlId = 0;
  int                    SavedFocusedItem = 0;
  int                    total_unres_file = 0;

  m_selectedSource = SOURCE_LOCAL;

  // keep the focus control
  CGUIControl *focusedControl = GetFocusedControl();

  if (focusedControl)
  {
    SavedFocusedControlId = focusedControl->GetID();
    if (SavedFocusedControlId == CONTROL_SOURCE_LIST)
    {
      CGUIListContainer *listControl = (CGUIListContainer *)GetControl(CONTROL_SOURCE_LIST);
      if (listControl)
      {
        SavedFocusedItem = listControl->GetSelectedItem();
      }
    }
  }
  // Clear the list of sources
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_SOURCE_LIST);
  OnMessage(msgReset);
  m_sources.Clear();
  m_sourcesExist = false;

  CBoxeeMediaSourceList sourceList;
  BoxeeMediaSourceMap::iterator sourcesIterator;

  for (sourcesIterator = sourceList.getMap().begin(); sourcesIterator != sourceList.getMap().end(); sourcesIterator++)
  {
    CBoxeeMediaSource& source = (*sourcesIterator).second;
    int resolved_count = 0;
    int unresolved_count = 0;
    int new_count = 0;
    int total_count = 0;
    bool display_counters = true;

    // Create new share FileItem
    CStdString statusVal;
    CFileItemPtr share ( new CFileItem(source.name) );
    share->m_strPath = source.path;
    share->SetThumbnailImage(source.thumbPath);
    if (source.isVideo) share->SetProperty("IsVideo", true);
    if (source.isMusic) share->SetProperty("IsMusic", true);
    if (source.isPicture) share->SetProperty("IsPictures", true);
    if (source.isPrivate) share->SetProperty("IsPrivate", true);
    share->SetLabel(source.name);
    share->SetProperty("name",source.name);
    if (source.isNetwork)
    {
      share->SetProperty("foldericon","network_folder");
    }
    else
    {
      share->SetProperty("foldericon","folder");
    }

    if (g_application.IsPathAvailable(source.path, true))
    {
      if (((source.isVideo) || (source.isMusic))  &&  source.scanType != CMediaSource::SCAN_TYPE_PRIVATE)
      {
        CStdString strLabel = share->GetLabel();
        CStdString strPath = source.path;
        //CStdString strType = source.isVideo ? "video" : "music";

        time_t LastScannedVideo = 0;
        time_t LastScannedMusic = 0;

        if (source.isVideo)
        {
          // get the resolved/unresolved video files number
          BOXEE::BXVideoDatabase video_db;
          resolved_count = video_db.GetShareUnresolvedVideoFilesCount(_P(source.path), STATUS_RESOLVED);
          unresolved_count = video_db.GetShareUnresolvedVideoFilesCount(_P(source.path), STATUS_UNRESOLVED);
          new_count = video_db.GetShareUnresolvedVideoFilesCount(_P(source.path), STATUS_NEW);
          total_count = video_db.GetShareUnresolvedVideoFilesCount(_P(source.path), STATUS_ALL);
          total_unres_file += unresolved_count;

          BOXEE::Boxee::GetInstance().GetMetadataEngine().GetScanTime(strLabel ,strPath, "video", LastScannedVideo);
        }

        if (source.isMusic)
        {
          // get the resolved/unresolved music files number
          BOXEE::BXAudioDatabase audio_db;
          resolved_count += audio_db.GetShareUnresolvedAudioFilesCount(_P(source.path), STATUS_RESOLVED);
          unresolved_count += 0; // unresolved music file doesnt count in the status
          new_count += audio_db.GetShareUnresolvedAudioFilesCount(_P(source.path), STATUS_NEW);
          total_count += audio_db.GetShareUnresolvedAudioFilesCount(_P(source.path), STATUS_ALL);

          BOXEE::Boxee::GetInstance().GetMetadataEngine().GetScanTime(strLabel ,strPath, "music", LastScannedMusic);
        }

        if ((LastScannedVideo == SHARE_TIMESTAMP_RESOLVING) || (LastScannedMusic == SHARE_TIMESTAMP_RESOLVING))
        {
          LastScanned = SHARE_TIMESTAMP_RESOLVING;
        }
        else
        {
          LastScanned = std::max(LastScannedVideo,LastScannedMusic);
        }

        if (LastScanned == SHARE_TIMESTAMP_NOT_SCANNED)
        {
          statusVal = g_localizeStrings.Get(51531);
          share->SetProperty("scan_status", statusVal);
          display_counters = false;
        }
        else if (LastScanned == SHARE_TIMESTAMP_RESOLVING)
        {
          statusVal = g_localizeStrings.Get(51532);
          share->SetProperty("scan_status", statusVal);

          display_counters = false;

          if (source.isNetwork)
          {
            share->SetProperty("foldericon","scanning_network_folder");
          }
          else
          {
            share->SetProperty("foldericon","scanning_folder");
          }
        }
        else
        {
          // if we scanned the folder already, but there are still new files of
          // the share we should display Identifying label

          if (new_count)
          {
            statusVal = g_localizeStrings.Get(51535);
            share->SetProperty("scan_status", statusVal);
          }
          else
          {
            CStdString timeStr = BoxeeUtils::FormatTime(LastScanned);
            statusVal = g_localizeStrings.Get(51533);

            statusVal.Replace("%s", timeStr.c_str());
            share->SetProperty("scan_status", statusVal);
          }
        }

        // set the file counters label
        if (display_counters && (total_count > 0))
        {
          if (unresolved_count > 0 )
          {
            if (total_count == 1 )
            {
              statusVal.Format(g_localizeStrings.Get(51537),total_count, unresolved_count);
            }
            else
            {
              statusVal.Format(g_localizeStrings.Get(51539),total_count, unresolved_count);
            }
          }
          else
          {
            if (total_count == 1 )
            {
              statusVal.Format(g_localizeStrings.Get(51536),total_count);
            }
            else
            {
              statusVal.Format(g_localizeStrings.Get(51538),total_count);
            }
          }

          share->SetLabel2(statusVal.c_str());
        }
      }
    }
    else
    {
      // path is not available
      statusVal = g_localizeStrings.Get(51534);
      share->SetProperty("scan_status", statusVal);
      share->SetProperty("foldericon","disconnected_folder");
    }

    m_sources.Add(share);

    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_SOURCE_LIST, 0, 0, share);
    OnMessage(msg);

    m_sourcesExist = true;
  }

  if (total_unres_file == 0)
  {
    SET_CONTROL_HIDDEN(CONTROL_SHOW_UNIDENTIFIED);
  }
  else
  {
    SET_CONTROL_VISIBLE(CONTROL_SHOW_UNIDENTIFIED);
  }

  if (!m_sourcesExist)
  {
    CONTROL_DISABLE(CONTROL_SOURCE_LIST);
  }
  else
  {
    CONTROL_ENABLE(CONTROL_SOURCE_LIST);
  }

  if (SavedFocusedControlId == CONTROL_SOURCE_LIST)
  {
    if (SavedFocusedItem >= m_sources.Size())
    {
      if (m_sources.Size() == 0 )
      {
        SavedFocusedItem = 0;
      } else
      {
        SavedFocusedItem = m_sources.Size() -1 ;
      }
    }

    SET_CONTROL_FOCUS(CONTROL_SOURCE_LIST,SavedFocusedItem);
  }
  else if (SavedFocusedControlId != 0)
  {
    SET_CONTROL_FOCUS(SavedFocusedControlId,0);
  }
}

void CGUIWindowBoxeeMediaSources::ProccessItemSelectedInControlSourceList()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_SOURCE_LIST);
  OnMessage(msg);
  int iItem = msg.GetParam1();

  CGUIWindowBoxeeMediaSourceInfo *pDlgSourceInfo = (CGUIWindowBoxeeMediaSourceInfo*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_SOURCE_INFO);
  if (pDlgSourceInfo)
  {
    pDlgSourceInfo->SetEditedSource(m_sources[iItem]->GetProperty("name"));
    pDlgSourceInfo->SetSourceThumbPath((m_sources[iItem]->GetThumbnailImage()));
    pDlgSourceInfo->SetEnableLocationEdit(false);
  }

  g_windowManager.ActivateWindow(WINDOW_BOXEE_MEDIA_SOURCE_INFO);
}

void CGUIWindowBoxeeMediaSources::UpdateScanLabel(const CStdString& path)
{
  CStdString pathToShow = GetScanResolvePathToShow(path);
  CUtil::UrlDecode(pathToShow);

  if (!pathToShow.IsEmpty())
  {
    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_SCAN_LABEL);
    msg.SetLabel(pathToShow);
    g_windowManager.SendThreadMessage(msg);
  }
}

void CGUIWindowBoxeeMediaSources::UpdateResolveLabel(const CStdString& path)
{
  CStdString pathToShow = GetScanResolvePathToShow(path);
  CUtil::UrlDecode(pathToShow);

  if (!pathToShow.IsEmpty())
  {
    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_RESOLVE_LABEL);
    msg.SetLabel(pathToShow);
    g_windowManager.SendThreadMessage(msg);
  }
}

CStdString CGUIWindowBoxeeMediaSources::GetScanResolvePathToShow(const CStdString& path)
{
  CStdString pathToShow = "";

  if (!CUtil::IsInArchive(path))
  {
    // Translate the path
    pathToShow = _P(path);

    if (CUtil::IsHD(pathToShow))
    {
      CUtil::HideExternalHDPath(pathToShow, pathToShow);
    }

    // Shorten the path
    CStdString shortPath = pathToShow;
    CUtil::MakeShortenPath(pathToShow,shortPath,80);
    pathToShow = shortPath;

    if(!pathToShow.IsEmpty())
    {
      CUtil::RemovePasswordFromPath(pathToShow);
    }
  }

  return pathToShow;
}
