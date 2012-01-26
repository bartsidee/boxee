
#include "GUIWindowBoxeeMediaInfo.h"
#include "GUIWindowManager.h"
#include "GUIDialogBoxeeMediaAction.h"
#include "GUIDialogBoxeeRate.h"
#include "GUIDialogBoxeeShare.h"
//#include "GUIDialogBoxeeManualResolve.h"
#include "BoxeeUtils.h"
#include "Application.h"
#include "VideoInfoTag.h"
#include "GUIDialogYesNo.h"
#include "lib/libBoxee/boxee.h"
#include "Util.h"
#include "GUIDialogYesNo2.h"
#include "GUIWindowSlideShow.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "utils/log.h"

#define BTN_PLAY      9001
#define BTN_DOWNLOAD  9002
#define BTN_TRAILER   9005
#define BTN_RATE      9006
#define BTN_RECOMMEND 9007
#define BTN_MANUAL    9011

CGUIWindowBoxeeMediaInfo::CGUIWindowBoxeeMediaInfo()
: CGUIWindow(WINDOW_BOXEE_MEDIA_INFO, "boxee_media_info.xml")
{

}

CGUIWindowBoxeeMediaInfo::~CGUIWindowBoxeeMediaInfo()
{

}

void CGUIWindowBoxeeMediaInfo::Show(CFileItem* pItem)
{
  if (!pItem) return;

  CGUIWindowBoxeeMediaInfo *pWindow = (CGUIWindowBoxeeMediaInfo*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_INFO);
  if (pWindow)
  {
    pWindow->m_item = *pItem;
    g_windowManager.ActivateWindow(WINDOW_BOXEE_MEDIA_INFO);
  }
  
  g_windowManager.CloseDialogs(true);
}

void CGUIWindowBoxeeMediaInfo::OnInitWindow()
{
  CGUIWindow::OnInitWindow();

  // Send the item to the special container to allow skin acceess 
  CFileItemPtr itemPtr(new CFileItem(m_item));
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), 5000, 0, 0, itemPtr);
  g_windowManager.SendThreadMessage(winmsg);
  
  if (BoxeeUtils::CanRecommend(m_item))
  {
    SET_CONTROL_VISIBLE(BTN_RATE);
    SET_CONTROL_VISIBLE(BTN_RECOMMEND);
  }
  else 
  {
    SET_CONTROL_HIDDEN(BTN_RATE);
    SET_CONTROL_HIDDEN(BTN_RECOMMEND);
  }

  if (BoxeeUtils::HasTrailer(m_item))
  {
    SET_CONTROL_VISIBLE(BTN_TRAILER);
  }
  else 
  {
    SET_CONTROL_HIDDEN(BTN_TRAILER);
  }

  SET_CONTROL_HIDDEN(BTN_DOWNLOAD);

  if (BoxeeUtils::CanPlay(m_item))
  {
    SET_CONTROL_VISIBLE(BTN_PLAY);
    SET_CONTROL_FOCUS(BTN_PLAY, 0);
  }
  else 
  {
    SET_CONTROL_HIDDEN(BTN_PLAY);
  }

  if (BoxeeUtils::CanRemove(m_item))
  {
    SET_CONTROL_VISIBLE(BTN_MANUAL);
  }
  else
  {
    SET_CONTROL_HIDDEN(BTN_MANUAL);
  }

}

bool CGUIWindowBoxeeMediaInfo::OnAction(const CAction &action)
{

  if (action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    g_windowManager.PreviousWindow();
    return true;
  }

  return CGUIWindow::OnAction(action);
}

bool CGUIWindowBoxeeMediaInfo::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();

    if (iControl == BTN_PLAY) 
    {
      if(m_item.IsVideo() || m_item.HasProperty("isvideo"))
      {
        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaInfo::OnMessage - In BTN_PLAY - Item is video -> Going to call HandlePlayForVideoItem (mip)");

        HandlePlayForVideoItem();
      }
      else if(m_item.IsPicture() || m_item.HasProperty("ispicture"))
      {
        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaInfo::OnMessage - In BTN_PLAY - Item is picture -> Going to call HandlePlayForPictureItem (mip)");

        HandlePlayForPictureItem();
      }
      else
      {
        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaInfo::OnMessage - In BTN_PLAY. Item isn't VIDEO or PICTURE -> Not handling item (mip)");
        g_windowManager.PreviousWindow();        
      }
      
      return true;
    }
    else if (iControl == BTN_TRAILER)
    {
      CGUIDialogBoxeeMediaAction::PlayTrailer(m_item);
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
    else if (iControl == BTN_RECOMMEND)
    {
      CGUIDialogBoxeeShare *pFriends = (CGUIDialogBoxeeShare *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_SHARE);
      pFriends->DoModal();
    }
    else if (iControl == BTN_MANUAL)
    {
      //CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaInfo::OnMessage, MANUAL, resolve item manually, path = %s", m_item.m_strPath.c_str());
      //CGUIDialogBoxeeManualResolve::Show(&m_item);
    }
  }
  case GUI_MSG_ITEM_LOADED:
  {
    CFileItem *pItem = (CFileItem *)message.GetPointer();
    message.SetPointer(NULL);
    if (pItem) {
      m_item = *pItem;
      
      CGUIMessage winmsg1(GUI_MSG_LABEL_RESET, GetID(), 5000);
      g_windowManager.SendThreadMessage(winmsg1);
      
      
      CFileItemPtr itemPtr(new CFileItem(*pItem));
      CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), 5000, 0, 0, itemPtr);
      g_windowManager.SendThreadMessage(winmsg);

      delete pItem;
    }
  }
  break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowBoxeeMediaInfo::HandlePlayForVideoItem()
{
  if(g_application.m_pPlayer && g_application.m_pPlayer->IsPlaying())
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaInfo::HandlePlayForVideoItem - [IsPlaying=%d][IsPaused=%d] (mip)",g_application.m_pPlayer->IsPlaying(),g_application.m_pPlayer->IsPaused());

    bool dlgWasCanceled;
    if (CGUIDialogYesNo2::ShowAndGetInput(0,51964,-1,-1,dlgWasCanceled,0,0))
    {
      if(g_application.m_pPlayer)
      {
        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaInfo::HandlePlayForVideoItem - Return from CGUIDialogYesNo2::ShowAndGetInput [dlgWasCanceled=%d]. [IsPaused=%d] and click RESTART -> Going to RESTART (mip)",dlgWasCanceled,g_application.m_pPlayer->IsPaused());
      }

      // Restart
      
      CGUIDialogBoxeeMediaAction::OnPlay(m_item);
    }
    else
    {
      if(g_application.m_pPlayer)
      {        
        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaInfo::HandlePlayForVideoItem - Return from CGUIDialogYesNo2::ShowAndGetInput [dlgWasCanceled=%d]. [IsPaused=%d] and click RESUME -> Going to RESUME (mip)",dlgWasCanceled,g_application.m_pPlayer->IsPaused());
        
        // In case of PAUSE -> We want to RESUME the video
        if(g_application.m_pPlayer->IsPaused())
        {
          g_application.m_pPlayer->Pause();
        }
      }

      // Resume
      
      g_windowManager.PreviousWindow();
    }
  }
  else 
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaInfo::HandlePlayForVideoItem - [g_application.m_pPlayer=NULL] or [g_application.m_pPlayer.IsPlaying=FALSE] -> Going to RESTART (mip)");
    
    // Restart
    
    CGUIDialogBoxeeMediaAction::OnPlay(m_item);
  } 
}

void CGUIWindowBoxeeMediaInfo::HandlePlayForPictureItem()
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaInfo::HandlePlayForPictureItem - Enter function [ItemPath=%s] (mip)",m_item.m_strPath.c_str());

  CGUIWindowSlideShow* pSlideShow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
  
  if (!pSlideShow)
  {
    return;
  }

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaInfo::HandlePlayForPictureItem - Enter function [ItemPath=%s][IsPlaying=%d][IsPaused=%d] (mip)",m_item.m_strPath.c_str(),pSlideShow->IsPlaying(),pSlideShow->IsPaused());

  CStdString strParentPath;
  CUtil::GetParentPath(m_item.m_strPath,strParentPath);

  pSlideShow->Reset();
  pSlideShow->RunSlideShow(strParentPath,false,false,false,m_item.m_strPath,true);
}

