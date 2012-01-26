#include "GUIDialogBoxeeNetworkNotification.h"
#include "GUIWindowManager.h"

#define ID_BUTTON_OK           10
#define ID_BUTTON_EDIT_NETWORK 11

CGUIDialogBoxeeNetworkNotification::CGUIDialogBoxeeNetworkNotification() : CGUIDialogBoxBase(WINDOW_DIALOG_BOXEE_NETWORK_NOTIFICATION, "boxee_network_notification.xml")
{

}

CGUIDialogBoxeeNetworkNotification::~CGUIDialogBoxeeNetworkNotification(void)
{

}

bool CGUIDialogBoxeeNetworkNotification::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();

    switch(iControl)
    {
    case ID_BUTTON_OK:
    {
      m_bConfirmed = true;
      Close();
      return true;
    }
    break;
    case ID_BUTTON_EDIT_NETWORK:
    {
      m_bConfirmed = true;
      Close();
      g_windowManager.ActivateWindow(WINDOW_SETTINGS_NETWORK);
      return true;
    }
    break;
    }
  }
  break;
  }

  return CGUIDialogBoxBase::OnMessage(message);
}

void CGUIDialogBoxeeNetworkNotification::ShowAndGetInput(int heading, int line)
{
  CGUIDialogBoxeeNetworkNotification *dialog = (CGUIDialogBoxeeNetworkNotification*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_NETWORK_NOTIFICATION);
  if (!dialog)
  {
    return;
  }
  dialog->DoModal();
}

void CGUIDialogBoxeeNetworkNotification::ShowAndGetInput(const CStdString&  heading, const CStdString& line)
{
  CGUIDialogBoxeeNetworkNotification *dialog = (CGUIDialogBoxeeNetworkNotification*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_NETWORK_NOTIFICATION);
  if (!dialog)
  {
    return;
  }
  dialog->DoModal();
}

