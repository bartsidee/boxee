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

// list control id
#define MSG_SHARE_BTN       700
#define MSG_CUSTOM_BTN      720
#define SHARE_TEST_LABEL_ID 800

using namespace BOXEE;

CGUIDialogBoxeeShare::CGUIDialogBoxeeShare()
 : CGUIDialog(WINDOW_DIALOG_BOXEE_SHARE, "boxee_share.xml")
{

}


CGUIDialogBoxeeShare::~CGUIDialogBoxeeShare()
{

}

CStdString CGUIDialogBoxeeShare::GetShareText()
{
  const CGUIControl* labelControl = GetControl(SHARE_TEST_LABEL_ID);
  if (labelControl && labelControl->GetControlType() == CGUIControl::GUICONTROL_LABEL)
  {
    const CGUILabelControl* label = (const CGUILabelControl*)labelControl;
    CStdString labelStr = label->GetLabel();
    BoxeeUtils::RemoveMatchingPatternFromString(labelStr,"(\\[.*?\\])");
    return labelStr;
  }
  return StringUtils::EmptyString;
}

bool CGUIDialogBoxeeShare::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_WINDOW_DEINIT)
  {
    m_item.Reset();
  }
  else if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    if (message.GetControlId() == WINDOW_DIALOG_BOXEE_SHARE)
    {
      std::vector<BOXEE::BXFriend> recommendTo; // always empty at the moment - which means - "to all the world".
      int action = message.GetParam1();
      if (action == ACTION_SELECT_ITEM || action == ACTION_MOUSE_LEFT_CLICK)
      {
        bool bSend = false;
        if (message.GetSenderId() == MSG_CUSTOM_BTN)
        {
          CGUIDialogKeyboard *pKeyboard = (CGUIDialogKeyboard*)g_windowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);
          if (pKeyboard)
          {
            pKeyboard->SetDoneText(g_localizeStrings.Get(53462));
            bSend = pKeyboard->ShowAndGetInput(m_strText, g_localizeStrings.Get(53461), false);

            if (bSend)
            {
              BoxeeUtils::Share(&m_item, recommendTo, false, m_strText);
              Close();
              g_application.m_guiDialogKaiToast.QueueNotification("", "", g_localizeStrings.Get(51033), 3000);
            }
            else if (pKeyboard->IsConfirmed() && m_strText.IsEmpty())
            {
              CGUIDialogOK2::ShowAndGetInput(54000, 54001);
            }
          }
        }
        else if (message.GetSenderId() == MSG_SHARE_BTN)
        {          
          BoxeeUtils::Rate(&m_item, true);
          Close();
          g_application.m_guiDialogKaiToast.QueueNotification("", "", g_localizeStrings.Get(51033), 3000);
        }        
      }
    }
	}
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogBoxeeShare::SetItem(const CFileItem *item)
{
  if (item)
    m_item = *item;
  else
    m_item.Reset();
}

void CGUIDialogBoxeeShare::OnInitWindow()
{
  //if (m_item.m_strPath.empty())
  //  m_item = g_infoManager.GetCurrentItem();

  CGUIWindow::OnInitWindow();

  m_strText.clear();
  
  // Send the item to the special container to allow skin access
  CFileItemPtr itemPtr(new CFileItem(m_item));
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), 5000, 0, 0, itemPtr);
  g_windowManager.SendMessage(winmsg);
  
  SET_CONTROL_FOCUS(700,0);
}

