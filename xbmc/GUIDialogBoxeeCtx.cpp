
#include "GUIWindowManager.h"
#include "GUIDialogBoxeeCtx.h"
#include "GUIWindowSettingsCategory.h"
#include "BoxeeUtils.h"
#include "Application.h"
#include "GUIDialogBoxeeShare.h"
#include "GUIDialogBoxeeRate.h"
#include "cores/dvdplayer/DVDPlayer.h"
#include "GUIWindowBoxeeMediaMain.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogBoxeeMediaAction.h"
#include "GUIUserMessages.h"
#include "LocalizeStrings.h"
#include "ItemLoader.h"
#include "utils/log.h"
#include "GUIDialogBoxeeDropdown.h"
#include "lib/libBoxee/bxutils.h"
#include "GUIDialogBoxeeVideoQuality.h"
#include "lib/libBoxee/boxee.h"

//#define BTN_WATCH     9001
//#define BTN_TRAILER   9002

#define BTN_MORE_INFO 9003

#define BTN_RATE      9006
#define BTN_SHARE     9007


#define BTN_COMMENT   9008
#define BTN_LANG      9009
#define BTN_OPTIONS   9010
#define BTN_PRESET    9020

#define BTN_QUALITY   9111

#define CONTROL_THUMB 19

#define INFO_HIDDEN_LIST 5000

CGUIDialogBoxeeCtx::CGUIDialogBoxeeCtx(DWORD dwID, const CStdString &xmlFile) : 
  CGUIDialog(dwID, xmlFile), m_pDlgVideoQuality(NULL)
{
}

CGUIDialogBoxeeCtx::~CGUIDialogBoxeeCtx(void)
{
}

void CGUIDialogBoxeeCtx::SetItem(const CFileItem &item)
{
  if (m_item.m_strPath != item.m_strPath)
  {
    m_item = item;

    // set properties for MoreInfo and Share for the OSD
    m_item.SetProperty("has-more-info",BoxeeUtils::HasDescription(m_item));

    if (!m_item.HasProperty("disable-recommend"))
    {
      m_item.SetProperty("disable-recommend",!BoxeeUtils::CanShare(m_item, false));
    }
  }
  
  if (m_item.IsVideo() && !m_item.GetPropertyBOOL("MetaDataExtracted"))
  {
    g_application.GetItemLoader().LoadFileMetadata(GetID(), INFO_HIDDEN_LIST, &m_item);
  }
}

void CGUIDialogBoxeeCtx::Update()
{
}

void CGUIDialogBoxeeCtx::Close(bool forceClose)
{
  // close child dialogs
  if (m_pDlgVideoQuality != NULL)
  {
    m_pDlgVideoQuality->Close(forceClose);
  }
  CGUIDialog::Close(forceClose);
}

bool CGUIDialogBoxeeCtx::OnAction(const CAction &action)
{
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeeCtx::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_ITEM_LOADED:
  {
    // New data received from the item loader, update existing item
    CFileItemPtr pItem ((CFileItem *)message.GetPointer());
    message.SetPointer(NULL);
    if (pItem) {
      m_item = *pItem;
      CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), INFO_HIDDEN_LIST, 0);
      OnMessage(msg);
      
      CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), INFO_HIDDEN_LIST, 0, 0, pItem);
      OnMessage(winmsg);
    }

    return true;
  }

  case GUI_MSG_CLICKED:
    {
      unsigned int iControl = message.GetSenderId();
      if (iControl == BTN_MORE_INFO)
      {
        OnMoreInfo();
        return true;
      }
      else if (iControl == BTN_RATE)
      {
        bool bLike;
        if (CGUIDialogBoxeeRate::ShowAndGetInput(bLike))
        {
          BoxeeUtils::Rate(&m_item, bLike);
          g_application.m_guiDialogKaiToast.QueueNotification("", "", g_localizeStrings.Get(51034), 5000);
        }
      }
      else if (iControl == BTN_SHARE)
      {
        CGUIDialogBoxeeShare* pShare = (CGUIDialogBoxeeShare *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_SHARE);

        if (pShare)
        {
          pShare->SetItem(&m_item);
          pShare->DoModal();
        }
        else
        {
          CLog::Log(LOGERROR,"CGUIDialogBoxeeCtx::OnMessage - GUI_MSG_CLICKED - BTN_SHARE - FAILED to get WINDOW_DIALOG_BOXEE_SHARE (share)");
        }
      }
      else if (iControl == BTN_PRESET)
      {
        if (m_item.GetPropertyBOOL("IsPreset"))
        {
          CGUIDialogYesNo* dlgYesNo = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
          if (dlgYesNo)
          {
            dlgYesNo->SetHeading(51020);
            dlgYesNo->SetLine(0, 51021);
            dlgYesNo->SetLine(1, m_item.GetLabel() + "?");
            dlgYesNo->SetLine(2, "");
            dlgYesNo->DoModal();

            if (dlgYesNo->IsConfirmed())
            {
              g_settings.DeleteSource(GetItemShareType(), m_item.GetProperty("ShareName"), m_item.m_strPath);
              CGUIWindow *pWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
              if (pWindow)
              {
                CGUIMessage msg(GUI_MSG_REFRESH_APPS, 0, 0);
                pWindow->OnMessage(msg);
              }
            }
          }
        } 
        else
        {
          CMediaSource newShare;
          newShare.strPath = m_item.m_strPath;
          newShare.strName = m_item.GetLabel();
          if (m_item.HasProperty("OriginalThumb") && !m_item.GetProperty("OriginalThumb").IsEmpty())
            newShare.m_strThumbnailImage = m_item.GetProperty("OriginalThumb");
          else
            newShare.m_strThumbnailImage = m_item.GetThumbnailImage();
          newShare.vecPaths.push_back(m_item.m_strPath);
          g_settings.AddShare(GetItemShareType(), newShare);
        }
        Close();
        return true;
      }
      else if (iControl == BTN_QUALITY)
      {
        return HandleQualityList();
      }
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
  case GUI_MSG_VISUALISATION_UNLOADING:
    {
    }
    break;
  case GUI_MSG_VISUALISATION_LOADED:
    {
    }
  }
  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogBoxeeCtx::HandleQualityList()
{
  const CFileItemList* linksFileItemList = m_item.GetLinksList();

  if (!linksFileItemList)
  {
    return true;
  }

  if (linksFileItemList->Size() == 1)
  {
    return true;
  }

  int FocusedItem = 0;
  m_pDlgVideoQuality = (CGUIDialogBoxeeVideoQuality*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_VIDEO_QUALITY);
  m_pDlgVideoQuality->Reset();
  for (int i=0; i<linksFileItemList->Size(); i++)
  {
    CFileItemPtr linkFileItemToAdd = linksFileItemList->Get(i);
    m_pDlgVideoQuality->Add((*linkFileItemToAdd));
    // if the user ask for a different quality - switch to a different link
    if (m_item.GetPropertyInt("quality") ==linkFileItemToAdd->GetPropertyInt("quality"))
    {
      FocusedItem = i;
    }

  }

  m_pDlgVideoQuality->ChangeDialogType(LIST_CVQ_DIALOG);
  m_pDlgVideoQuality->SetFocusedItem(FocusedItem);
  m_pDlgVideoQuality->DoModal();

  if (m_pDlgVideoQuality->IsCanceled())
  {
    return false;
  }
  int chosenItemPos = m_pDlgVideoQuality->GetSelectedItemPos();
  CFileItemPtr chosenItem = linksFileItemList->Get(chosenItemPos);

  if( m_pDlgVideoQuality->GetSavePerference())
  {
    BOXEE::Boxee::GetInstance().GetMetadataEngine().UpdateProviderPerf(chosenItem->GetProperty("link-providername"),
        chosenItem->GetPropertyInt("quality"));
  }

  // if the user ask for a different quality - switch to a different link
  if (m_item.GetPropertyInt("quality") !=chosenItem->GetPropertyInt("quality"))
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeCtx::HandleQualityList change quality to  %d", chosenItem->GetPropertyInt("quality"));

    if (chosenItem.get() == NULL)
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::OnPlayMedia choose NULL item");
      return false;
    }

    // copy the relevenat properties from the choosen item
    m_item.m_strPath =  chosenItem->m_strPath;
    m_item.SetProperty("link-boxeetype", chosenItem->GetProperty("link-boxeetype"));
    m_item.SetProperty("link-provider", chosenItem->GetProperty("link-provider"));
    m_item.SetProperty("link-providername", chosenItem->GetProperty("link-providername"));
    m_item.SetProperty("link-providerthumb", chosenItem->GetProperty("link-providerthumb"));
    m_item.SetProperty("link-countrycodes", chosenItem->GetProperty("link-countrycodes"));
    m_item.SetProperty("link-countryrel", chosenItem->GetProperty("link-countryrel"));
    m_item.SetProperty("quality-lbl", chosenItem->GetProperty("quality-lbl"));
    m_item.SetProperty("quality", chosenItem->GetPropertyInt("quality"));

    OnPlay();
  }

  return true;
}

void CGUIDialogBoxeeCtx::OnPlay()
{
  g_application.PlayFile(m_item);
}

void CGUIDialogBoxeeCtx::OnInitWindow()
{
  m_pDlgVideoQuality = NULL;

  if (BoxeeUtils::HasDescription(m_item))
  {
    SET_CONTROL_VISIBLE(BTN_MORE_INFO);
  }
  else 
  {
    SET_CONTROL_HIDDEN(BTN_MORE_INFO); 
  }
  
  if ( (m_item.GetPropertyBOOL("isloaded") || m_item.IsApp() ) && m_item.HasVideoInfoTag() || m_item.HasMusicInfoTag() || m_item.IsRSS() || m_item.IsInternetStream()) 
  {
    SET_CONTROL_VISIBLE(BTN_RATE);
    SET_CONTROL_VISIBLE(BTN_SHARE);
  }
  else 
  {
    SET_CONTROL_HIDDEN(BTN_RATE);
    SET_CONTROL_HIDDEN(BTN_SHARE);
  }

  //CONTROL_SELECT_ITEM(INFO_HIDDEN_LIST, 1);

  if (m_item.IsRSS() || m_item.IsLastFM() || m_item.IsShoutCast())
  {
    VECSOURCES *shares = g_settings.GetSourcesFromType(GetItemShareType());
    if (shares)
    {
      bool bFound = false;
      for (size_t i=0; !bFound && i < shares->size(); i++)
      {
        if ((*shares)[i].strPath == m_item.m_strPath)
        {
          m_item.SetProperty("ShareName", (*shares)[i].strName);
          bFound = true;
        }
      }
    
      m_item.SetProperty("IsPreset", bFound);
      m_item.SetProperty("CanPreset", true);
    }
  }

  SetAutoClose(8000);

  CGUIDialog::OnInitWindow();
}

CStdString CGUIDialogBoxeeCtx::GetItemShareType()
{
  if (m_item.GetPropertyBOOL("IsMusic") || m_item.GetPropertyBOOL("IsMusicShare") || m_item.GetPropertyBOOL("IsMusicFolder"))
    return "music";
  else if (m_item.GetPropertyBOOL("IsVideo") || m_item.GetPropertyBOOL("IsVideoShare") || m_item.GetPropertyBOOL("IsVideoFolder"))
    return "video";
  else if (m_item.GetPropertyBOOL("IsPicture") || m_item.GetPropertyBOOL("IsPictureShare") || m_item.GetPropertyBOOL("IsPictureFolder"))
    return "pictures";

  return "";
}
