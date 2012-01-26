
#include "GUIDialogBoxeeUserPassword.h"
#include "Application.h"
#include "GUIEditControl.h"
#include "GUIDialogKeyboard.h"
#include "utils/log.h"
#include "GUIWindowManager.h"

#define CONTROL_USER      9002
#define CONTROL_PASSWORD  9003
#define CONTROL_OK        9004
#define CONTROL_CANCEL    9005

CGUIDialogBoxeeUserPassword::CGUIDialogBoxeeUserPassword(void) : CGUIDialog(WINDOW_DIALOG_BOXEE_USER_PASSWORD, "boxee_user_password.xml")
{

}

CGUIDialogBoxeeUserPassword::~CGUIDialogBoxeeUserPassword(void)
{

}

bool CGUIDialogBoxeeUserPassword::OnAction(const CAction &action)
{
  if(action.id == ACTION_PREVIOUS_MENU)
  {
    m_IsConfirmed = false;
    Close();
    return true;
  }
  // don't allow any built in actions to act here.
  // this forces only navigation type actions to be performed.
  else if (action.id == ACTION_BUILT_IN_FUNCTION)
  {
    return true;  // pretend we handled it
  }
  
  return CGUIWindow::OnAction(action);
}

bool CGUIDialogBoxeeUserPassword::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {    
    DWORD senderId = message.GetSenderId();

    if(senderId == CONTROL_OK)
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUserPassword::OnMessage - GUI_MSG_CLICKED - case [CONTROL_OK] (msmk)");

      CGUIEditControl* textButton = (CGUIEditControl*) GetControl(CONTROL_USER);
      m_user = textButton->GetLabel2();
      textButton = (CGUIEditControl*) GetControl(CONTROL_PASSWORD);
      m_password = textButton->GetLabel2();      
      
      m_IsConfirmed = true;
      Close();
      return true;       
    }
    else if(senderId == CONTROL_CANCEL)
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUserPassword::OnMessage - GUI_MSG_CLICKED - case [CONTROL_CANCEL] (msmk)");

      m_IsConfirmed = false;
      Close();
      return true;
    }
  }
  break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogBoxeeUserPassword::OnInitWindow()
{
  CGUIWindow::OnInitWindow();

  if(!m_password.IsEmpty())
  {
    CGUIEditControl* textButton = (CGUIEditControl*)GetControl(CONTROL_PASSWORD);
    if (textButton)
    {
      textButton->SetLabel2(m_password);
    }
  }
  if(!m_user.IsEmpty())
  {
    CGUIEditControl* textButton = (CGUIEditControl*)GetControl(CONTROL_USER);
    if (textButton)
    {
      textButton->SetLabel2(m_user);
    }
  }
}


bool CGUIDialogBoxeeUserPassword::IsConfirmed()
{
   return m_IsConfirmed;
}

bool CGUIDialogBoxeeUserPassword::ShowAndGetUserAndPassword(CStdString& strUser, CStdString& strPassword, const CStdString& strURL)
{
  CGUIDialogBoxeeUserPassword* dialog = (CGUIDialogBoxeeUserPassword*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_USER_PASSWORD);
  if (!dialog)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeUserPassword::ShowAndGetUserAndPassword - FAILED to get dialog");
    return false;
  }

  dialog->SetUser(strUser);
  dialog->SetPassword(strPassword);
  dialog->DoModal();

  bool retVal = false;

  if (dialog->IsConfirmed())
  {
    strUser = dialog->GetUser();
    strPassword = dialog->GetPassword();
    retVal = true;
  }

  // reset
  dialog->SetUser("");
  dialog->SetPassword("");

  return retVal;
}
