//
// C++ Implementation: GUIDialogBoxeePostPlay
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//


#include "FileItem.h"
#include "GUIDialogBoxeePostPlay.h"

#include "lib/libBoxee/bxfriendslist.h"
#include "lib/libBoxee/boxee.h"

#include "Application.h"
#include "BoxeeUtils.h"
#include "GUIDialogKeyboard.h"

#include "GUIBaseContainer.h"
#include "GUILabelControl.h"
#include "utils/GUIInfoManager.h"

#include "Util.h"
#include "utils/log.h"
#include "LocalizeStrings.h"

#include "GUIToggleButtonControl.h"
#include "GUISettings.h"

#include "GUIDialogBoxeeShare.h"
#include "GUIUserMessages.h"
#include "GUIDialogOK2.h"
#include "GUIWindowManager.h"

#include "BoxeeItemLauncher.h"

using namespace BOXEE;

#define HIDDEN_CURR_ITEM_LIST 5000
#define HIDDEN_PREV_ITEM_LIST 5001
#define HIDDEN_NEXT_ITEM_LIST 5002

#define HAS_NEXT_PROP "HasNextItem"
#define HAS_PREV_PROP "HasPrevItem"
#define CAN_SHARE_PROP "CanShare"

#define SHOW_POPUP_SETTING_CHECKBOX 5010
#define SHOW_SHARE_DIALOG_BUTTON    5020

#define BUTTONS_LIST                700
#define SHARE_BTN                   701
#define PLAY_NEXT_BTN               702
#define REMOVE_FROM_QUEUE_BTN       703

CGUIDialogBoxeePostPlay::CGUIDialogBoxeePostPlay() : CGUIDialog(WINDOW_DIALOG_BOXEE_POST_PLAY, "boxee_post_play.xml")
{

}


CGUIDialogBoxeePostPlay::~CGUIDialogBoxeePostPlay()
{

}

bool CGUIDialogBoxeePostPlay::OnMessage(CGUIMessage &message)
{
  int action = message.GetParam1();
  if (message.GetMessage() == GUI_MSG_CLICKED && (action == ACTION_SELECT_ITEM || action == ACTION_MOUSE_LEFT_CLICK) )
  {
    switch (message.GetSenderId())
    {
    case SHOW_POPUP_SETTING_CHECKBOX:
    {
      g_guiSettings.SetBool("boxee.showpostplay", !g_guiSettings.GetBool("boxee.showpostplay"));
    }
    break;
    case SHARE_BTN:
    {
      HandleClickOnShareButton();
    }
    break;
    case PLAY_NEXT_BTN:
    {
      HandleClickOnPlayNextButton();
  }
    break;
    case REMOVE_FROM_QUEUE_BTN:
    {
      HandleClickOnRemoveFromQueueButton();
    }
    break;
    }
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogBoxeePostPlay::OnInitWindow()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePostPlay::OnInitWindow - Enter function. [ItemLabel=%s][ItemPath=%s] (postp)",m_item.GetLabel().c_str(),m_item.m_strPath.c_str());

  SET_CONTROL_SELECTED(GetID(), SHOW_POPUP_SETTING_CHECKBOX, g_guiSettings.GetBool("boxee.showpostplay"));

  CGUIDialog::OnInitWindow();
  
  // Send the items to the special containers to allow skin access
  CFileItemPtr itemPtr(new CFileItem(m_item));
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), HIDDEN_CURR_ITEM_LIST, 0, 0, itemPtr);
  g_windowManager.SendThreadMessage(winmsg);
  CGUIMessage winmsg1(GUI_MSG_LABEL_ADD, GetID(), HIDDEN_PREV_ITEM_LIST, 0, 0, m_item.GetPrevItem());
  OnMessage(winmsg1);
  CGUIMessage winmsg2(GUI_MSG_LABEL_ADD, GetID(), HIDDEN_NEXT_ITEM_LIST, 0, 0, m_item.GetNextItem());
  OnMessage(winmsg2);

  bool isInQueue = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().IsInQueue(m_item.GetProperty("boxeeid"),m_item.m_strPath);
  SetProperty("in-queue",isInQueue);

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePostPlay::OnInitWindow - [isInQueue=%d] (postp)",isInQueue);
}

void CGUIDialogBoxeePostPlay::SetItem(CFileItem *pItem)
{
  if (pItem)
  {
    m_item = *pItem;
    bool bHasNext = m_item.GetNextItem().get() && !m_item.GetNextItem()->m_strPath.empty(); 
    bool bHasPrev = m_item.GetPrevItem().get() && !m_item.GetPrevItem()->m_strPath.empty(); 
    SetProperty(HAS_NEXT_PROP, bHasNext);
    SetProperty(HAS_PREV_PROP, bHasPrev);
    
    if (BoxeeUtils::CanShare(m_item))
      m_item.SetProperty(CAN_SHARE_PROP, true);
    else
      m_item.SetProperty(CAN_SHARE_PROP, false);
  }
  else
  {
    m_item.Reset();
    SetProperty(HAS_NEXT_PROP, false);
    SetProperty(HAS_PREV_PROP, false);
    m_item.SetProperty(CAN_SHARE_PROP, false);
  }
}

void CGUIDialogBoxeePostPlay::Reset()
{
  m_item.Reset();
  SetProperty(HAS_NEXT_PROP, false);
  SetProperty(HAS_PREV_PROP, false);
}

bool CGUIDialogBoxeePostPlay::HandleClickOnShareButton()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePostPlay::HandleClickOnShareButton - Enter function (postp)");

  CGUIDialogBoxeeShare *pShare = (CGUIDialogBoxeeShare *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_SHARE);
  pShare->SetItem(&m_item);
  pShare->DoModal();

  return true;
}

bool CGUIDialogBoxeePostPlay::HandleClickOnPlayNextButton()
{
  Close();
  
  if (m_item.GetNextItem().get())
    CBoxeeItemLauncher::Launch(*(m_item.GetNextItem().get()));
  
  return true;
}

bool CGUIDialogBoxeePostPlay::HandleClickOnRemoveFromQueueButton()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePostPlay::HandleClickOnRemoveFromQueueButton - Enter function. Going to DEQUEUE item (postp)(queue)");

  bool succeeded = BoxeeUtils::Dequeue(&m_item);

  if (succeeded)
  {
    SET_CONTROL_FOCUS(SHARE_BTN,1);

    SetProperty("in-queue",0);

    g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::ICON_MINUS, "", g_localizeStrings.Get(53732), 3000 , KAI_GREEN_COLOR, KAI_GREEN_COLOR);

    CGUIMessage refreshQueueWinMsg(GUI_MSG_UPDATE, WINDOW_BOXEE_BROWSE_QUEUE, 0);
    g_windowManager.SendThreadMessage(refreshQueueWinMsg);

    CGUIMessage refreshHomeWinMsg(GUI_MSG_UPDATE, WINDOW_HOME, 0);
    g_windowManager.SendThreadMessage(refreshHomeWinMsg);
  }
  else
  {
    CGUIDialogOK2::ShowAndGetInput(53701, 53736);
  }

  return succeeded;
}
