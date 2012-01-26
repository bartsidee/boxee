
#include "GUIDialogBoxeePaymentOkPlay.h"
#include "GUIWindowManager.h"
#include "log.h"

#define OK_PLAY_BUTTON                 6531

CGUIDialogBoxeePaymentOkPlay::CGUIDialogBoxeePaymentOkPlay() : CGUIDialog(WINDOW_DIALOG_BOXEE_PAYMENT_OK_PLAY, "boxee_payment_ok_play.xml")
{
  m_bConfirmed = true;
}

CGUIDialogBoxeePaymentOkPlay::~CGUIDialogBoxeePaymentOkPlay()
{

}

void CGUIDialogBoxeePaymentOkPlay::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  SET_CONTROL_FOCUS(OK_PLAY_BUTTON, 0);
}

void CGUIDialogBoxeePaymentOkPlay::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
}

bool CGUIDialogBoxeePaymentOkPlay::OnAction(const CAction& action)
{
  switch (action.id)
  {
  case ACTION_PARENT_DIR:
  case ACTION_PREVIOUS_MENU:
  {
    m_bConfirmed = false;
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentOkPlay::OnAction - Handling action [%d]. After set [Confirmed=%d] and going to call Close() (pay)",action.id,m_bConfirmed);
    Close();
    return true;
  }
  break;
  default:
  {
    // do nothing
  }
  break;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeePaymentOkPlay::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    return OnClick(message);
  }
  break;
  default:
  {
    // do nothing
  }
  break;
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogBoxeePaymentOkPlay::OnClick(CGUIMessage& message)
{
  bool succeeded = false;

  int iControl = message.GetSenderId();

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentOkPlay::OnClick - Enter function with [iControl=%d] (pay)",iControl);

  switch(iControl)
  {
  case OK_PLAY_BUTTON:
  {
    m_bConfirmed = true;
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentOkPlay::OnClick - Handling click on [%d=OK_PLAY_BUTTON] - going to call Close(). [Confirmed=%d] (pay)",iControl,m_bConfirmed);
    Close();
    return true;
  }
  break;
  default:
  {
    CLog::Log(LOGWARNING,"CGUIDialogBoxeePaymentOkPlay::OnClick - UNKNOWN control [%d] was click (pay)",iControl);
  }
  break;
  }

  return succeeded;
}

bool CGUIDialogBoxeePaymentOkPlay::Show()
{
  CGUIDialogBoxeePaymentOkPlay* dialog = (CGUIDialogBoxeePaymentOkPlay*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_PAYMENT_OK_PLAY);
  if (!dialog)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentOkPlay::Show - FAILED to get dialog to show (pay)");
    return false;
  }

  dialog->DoModal();

  return dialog->m_bConfirmed;
}

