//
// C++ Implementation: GUIDialogBoxeeShare
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//


#include "FileItem.h"
#include "GUIDialogBoxeeShare.h"

#include "lib/libBoxee/bxfriendslist.h"
#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "json/value.h"
#include "json/reader.h"
#include "GUILargeTextureManager.h"

#include "Application.h"
#include "BoxeeUtils.h"
#include "GUIDialogKeyboard.h"

#include "GUIBaseContainer.h"
#include "GUILabelControl.h"
#include "utils/GUIInfoManager.h"

#include "Util.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIWrappingListContainer.h"
#include "GUIWindowManager.h"
#include "GUIDialogOK2.h"

#include "GUIEditControl.h"
#include "bxutils.h"

#include "MusicInfoTag.h"
#include "VideoInfoTag.h"
#include "guilib/GUIImage.h"
#include "guilib/GUIWebControl.h"

#include "GUIWebDialog.h"

#include "GUIWindowStateDatabase.h"

// list control id
#define MSG_SHARE_BTN       700
#define MSG_CUSTOM_BTN      720
#define SHARE_TEST_LABEL_ID 800

#define HIDDEN_CONTAINER    5000
#define USER_INFO_CONTAINER 5001
#define SERVICE_LIST        9000
#define EDIT_CONTROL        9001
#define SHARE_BTN           10
#define USER_THUMB          9011

#define WEB_CONTROL         1000

#define LEFT_TO_INPUT_SIZE "input-size"

using namespace BOXEE;

CGUIDialogBoxeeShare::CGUIDialogBoxeeShare() : CGUIDialog(WINDOW_DIALOG_BOXEE_SHARE, "boxee_share.xml")
{

}


CGUIDialogBoxeeShare::~CGUIDialogBoxeeShare()
{

}

bool CGUIDialogBoxeeShare::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_WINDOW_INIT)
  {
    //if (!LoadSocialServices(true))
    int retCode;
    if (BoxeeUtils::GetShareServicesJson(m_jsonServiceList,retCode,true) != JOB_SUCCEEDED)
    {
      Close();
      return true;
    }
  }
  else if (message.GetMessage() == GUI_MSG_WINDOW_DEINIT)
  {
    m_item.Reset();
  }
  else if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    switch(message.GetSenderId())
    {
    case EDIT_CONTROL:
    {
      CGUIEditControl* editControl = (CGUIEditControl*)GetControl(EDIT_CONTROL);
      CGUIWindow* activeWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
      if (editControl && activeWindow)
      {
        CStdString leftToInput = BOXEE::BXUtils::IntToString(editControl->GetMaxInputSize() - (int)editControl->GetLabel2().size());
        activeWindow->SetProperty(LEFT_TO_INPUT_SIZE,leftToInput);
      }
    }
    break;
    case SHARE_BTN:
    {
      CGUIEditControl* editControl = (CGUIEditControl*)GetControl(EDIT_CONTROL);
      if (!editControl)
      {
        CGUIDialogOK2::ShowAndGetInput(257, 55195);
        return true;
      }

      m_strText = editControl->GetLabel2();

      if (m_strText.IsEmpty())
      {
        CGUIDialogOK2::ShowAndGetInput(54000, 54001);
        return true;
      }

      for(int i = 0 ;i < m_servicesList.Size() ;i++)
      {
        if(m_servicesList[i]->GetProperty("serviceId") == FACEBOOK_SERVICE_ID && m_servicesList[i]->GetPropertyBOOL("isSelected"))
        {
          BoxeeUtils::LaunchGetFacebookExtraCredentials(false);
          break;
        }
      }

      std::vector<BOXEE::BXFriend> recommendTo; // always empty at the moment - which means - "to all the world".

      BoxeeUtils::Share(&m_item, recommendTo, m_servicesList, false, m_strText);
      Close();
      g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::ICON_HEART, "", g_localizeStrings.Get(51033), 3000, KAI_RED_COLOR, KAI_GREY_COLOR);
    }
    break;
    case SERVICE_LIST:
    {
      CGUIBaseContainer* serviceList = (CGUIBaseContainer*) GetControl(SERVICE_LIST);
      CGUIListItemPtr itemUI = serviceList->GetSelectedItemPtr();

      int iItem = serviceList->GetSelectedItem();
      if (iItem < m_servicesList.Size() && iItem >= 0 )
      {
        CFileItemPtr fileItem = m_servicesList.Get(iItem);
        CFileItem* fItem = fileItem.get();

        //fItem->Dump();

        if (!fileItem->GetPropertyBOOL("enable"))
        {
          //open up the browser dialog with the link we got from the server
#ifdef CANMORE
          if (!g_application.IsPlaying() || g_application.GetCurrentPlayer() != PCID_FLASHLAYER)
          {
            //browser reported success
            int retCode;
            if (CGUIWebDialog::ShowAndGetInput(fileItem->GetProperty("connect")) && BoxeeUtils::GetShareServicesJson(m_jsonServiceList,retCode,true) == JOB_SUCCEEDED)
            {
              UpdateShareDialog();
            }
          }
          else
#endif
          {
            CStdString text = g_localizeStrings.Get(80001);
            text += " " + fileItem->GetProperty("link");
            CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(10014),text);
          }
          return true;
        }

        bool bSelection = itemUI->GetPropertyBOOL("isSelected");
        fItem->SetProperty("isSelected", !bSelection);
        itemUI->SetProperty("isSelected", !bSelection);
      }
    }
    break;
    }
	}

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogBoxeeShare::SetItem(const CFileItem *item)
{
  if (item)
  {
    m_item = *item;
  }
  else
  {
    m_item.Reset();
  }
}

void CGUIDialogBoxeeShare::UpdateShareDialog()
{
  m_servicesList.Clear();
  //if (jsonServicesResponse.compare(m_jsonServiceList) != 0)
  {
    BoxeeUtils::ParseJsonShareServicesToFileItems(m_jsonServiceList,m_servicesList);
    BoxeeUtils::LoadSocialShareServicesStatus(m_servicesList);
  }

  CGUIMessage msgServicesReset(GUI_MSG_LABEL_RESET, GetID(), SERVICE_LIST , 0);
  g_windowManager.SendMessage(msgServicesReset);

  CGUIMessage msgServices(GUI_MSG_LABEL_BIND, GetID(), SERVICE_LIST, 0, 0, &m_servicesList);
  g_windowManager.SendMessage(msgServices);

  if (m_servicesList.Size() > 1)
    CONTROL_SELECT_ITEM(SERVICE_LIST,m_servicesList.Size()-1);
}
/*
bool CGUIDialogBoxeeShare::LoadSocialServices(bool bRunInBG)
{
  RequestSocialServices* request = new RequestSocialServices();

  if (bRunInBG)
  {
    //run in bg and display progress
    CUtil::RunInBG(request,false);
  }
  else
  {
    //run it on the requesting thread
    request->Run();
  }

  m_jsonServiceList = request->m_response;

  if (!bRunInBG)
  {
    //we didn't run in bg, delete the allocation and quit
    delete request;
    return true;
  }

  if (!request->m_IsCanceled)
  {
    //if it wasn't canceled
    delete request;
    return true;
  }
  else
  {
    //the bg was canceled
    Close();
    return false;
  }
}*/

void CGUIDialogBoxeeShare::OnInitWindow()
{
  CGUIWindow::OnInitWindow();

  m_strText = GetDefaultShareText();
  
  // Send the item to the special container to allow skin access
  CFileItemPtr itemPtr(new CFileItem(m_item));
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), HIDDEN_CONTAINER, 0, 0, itemPtr);
  g_windowManager.SendMessage(winmsg);
  
  CGUIEditControl* editControl = (CGUIEditControl*)GetControl(EDIT_CONTROL);
  if (editControl)
  {
    editControl->SetLabel2(m_strText);
    CGUIWindow* activeWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
    if (activeWindow)
    {
      CStdString leftToInput = BOXEE::BXUtils::IntToString(editControl->GetMaxInputSize() - (int)editControl->GetLabel2().size());
      activeWindow->SetProperty(LEFT_TO_INPUT_SIZE,leftToInput);
    }
  }

  // Set the user profile thumb - start
  CProfile profile = g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex];

  CFileItemPtr item(new CFileItem(profile.getName()));
  CStdString strLabel;

  if (profile.getDate().IsEmpty())
    strLabel = g_localizeStrings.Get(20113);
  else
    strLabel.Format(g_localizeStrings.Get(20112),profile.getDate());

  item->SetLabel2(strLabel);
  item->SetThumbnailImage(profile.getThumb());
  item->SetCachedPictureThumb();
  if (profile.getThumb().IsEmpty() || profile.getThumb().Equals("-"))
    item->SetThumbnailImage("unknown-user.png");

  item->SetLabelPreformated(true);

  CGUIMessage winmsg2(GUI_MSG_LABEL_ADD, GetID(), USER_INFO_CONTAINER, 0, 0, item);
  g_windowManager.SendMessage(winmsg2);

  UpdateShareDialog();

  SET_CONTROL_FOCUS(SHARE_BTN,0);
}

void CGUIDialogBoxeeShare::OnDeinitWindow(int nextWindowID)
{
  CGUIWindowStateDatabase statedb;

  for (int i = 0 ; i < m_servicesList.Size() ; i++)
  {
    CFileItemPtr fileItemPtr = m_servicesList.Get(i);
    if (fileItemPtr.get())
    {
      statedb.SetUserServiceState(fileItemPtr->GetProperty("id"),fileItemPtr->GetPropertyBOOL("isSelected"));
    }
    fileItemPtr.reset();
  }

  CGUIDialog::OnDeinitWindow(nextWindowID);
}

CStdString CGUIDialogBoxeeShare::GetDefaultShareText()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeShare::GetDefaultShareText - Enter function. [label=%s][HasMusicInfoTag=%d][HasVideoInfoTag=%d][IsLiveTV=%d] (share)",m_item.GetLabel().c_str(),m_item.HasMusicInfoTag(),m_item.HasVideoInfoTag(),m_item.HasProperty("livetv"));

  CStdString defaultShareText = m_item.HasProperty("livetv") ? g_localizeStrings.Get(53485) : g_localizeStrings.Get(53463);
  defaultShareText += " ";

  if (m_item.HasMusicInfoTag())
  {
    if (!GetDefaultMusicShareText(defaultShareText))
    {
      CLog::Log(LOGWARNING,"CGUIDialogBoxeeShare::GetDefaultShareText - FAILED to get DefaultMusicShareText from item [label=%s] (share)",m_item.GetLabel().c_str());
      defaultShareText = "";
    }
  }
  else if (m_item.HasVideoInfoTag())
  {
    if (!GetDefaultTvShowShareText(defaultShareText))
    {
      CLog::Log(LOGWARNING,"CGUIDialogBoxeeShare::GetDefaultShareText - FAILED to get DefaultTvShowShareText from item [label=%s] (share)",m_item.GetLabel().c_str());
      defaultShareText = "";
    }
  }
  else
  {
    defaultShareText = "";
  }

  if (defaultShareText.IsEmpty())
  {
    defaultShareText = m_item.HasProperty("livetv") ? g_localizeStrings.Get(53485) : g_localizeStrings.Get(53463);
    defaultShareText += " ";
    defaultShareText += m_item.GetLabel();
  }

  defaultShareText += " ";

  if (!m_item.HasProperty("livetv"))
  {
    defaultShareText += g_localizeStrings.Get(53466);
  }
  else
  {
    defaultShareText += g_localizeStrings.Get(53471);
  }

  BoxeeUtils::RemoveMatchingPatternFromString(defaultShareText,"(\\[.*?\\])");

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeShare::GetDefaultShareText - Exit function with [defaultShareText=%s]. [label=%s][HasMusicInfoTag=%d][HasVideoInfoTag=%d] (share)",defaultShareText.c_str(),m_item.GetLabel().c_str(),m_item.HasMusicInfoTag(),m_item.HasVideoInfoTag());

  return defaultShareText;
}

bool CGUIDialogBoxeeShare::GetDefaultMusicShareText(CStdString& defaultMusicShareText)
{
  CMusicInfoTag* musicInfoTag = m_item.GetMusicInfoTag();

  if (!musicInfoTag || musicInfoTag->GetAlbum().IsEmpty() || musicInfoTag->GetArtist().IsEmpty())
  {
    return false;
  }

  defaultMusicShareText += musicInfoTag->GetAlbum();
  defaultMusicShareText += " by ";
  defaultMusicShareText += musicInfoTag->GetArtist();

  return true;
}

bool CGUIDialogBoxeeShare::GetDefaultTvShowShareText(CStdString& defaultTvShowShareText)
{
  CVideoInfoTag* videoInfoTag = m_item.GetVideoInfoTag();

  if (!videoInfoTag)
  {
    return false;
  }

  if (!videoInfoTag->m_strShowTitle.IsEmpty())
  {
    defaultTvShowShareText += videoInfoTag->m_strShowTitle;
  }
  else if (m_item.GetPropertyBOOL("IsTVShow"))
  {
    defaultTvShowShareText += m_item.GetLabel();
  }
  else
  {
    return false;
  }

  CStdString season = "";
  if (videoInfoTag->m_iSpecialSortSeason > 0)
  {
    season.Format("%d",videoInfoTag->m_iSpecialSortSeason);
  }
  else if (videoInfoTag->m_iSeason > 0)
  {
    season.Format("%d",videoInfoTag->m_iSeason);
  }

  if (season.IsEmpty())
  {
    defaultTvShowShareText += " (";
    defaultTvShowShareText += m_item.GetLabel();
    defaultTvShowShareText += ")";
    return true;
  }

  defaultTvShowShareText += " S";
  defaultTvShowShareText += season;

  CStdString episode = "";
  if (videoInfoTag->m_iSpecialSortEpisode > 0)
  {
    episode.Format("%d",videoInfoTag->m_iSpecialSortEpisode);
  }
  else if (videoInfoTag->m_iEpisode >= 0)
  {
    episode.Format("%d",videoInfoTag->m_iEpisode);
  }

  if (episode.IsEmpty())
  {
    return false;
  }

  defaultTvShowShareText += "|E";
  defaultTvShowShareText += episode;

  return true;
}
