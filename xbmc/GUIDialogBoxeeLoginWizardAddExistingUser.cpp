#include "GUIDialogBoxeeLoginWizardAddExistingUser.h"
#include "GUIWindowManager.h"
#include "BoxeeLoginManager.h"
#include "utils/log.h"
#include "GUIRadioButtonControl.h"
#include "GUIEditControl.h"
#include "Application.h"
#include "GUIDialogOK2.h"
#include "BoxeeLoginWizardManager.h"
#include "lib/libBoxee/bxcredentials.h"
#include "lib/libBoxee/boxee.h"

#define USERNAME_EDIT_CONTROL_ID        8605
#define PASSWORD_EDIT_CONTROL_ID        8602
#define REMEMBER_PASSWORD_CONTROL_ID    8603
#define SHOW_PASSWORD_CONTROL_ID        8607

CGUIDialogBoxeeLoginWizardAddExistingUser::CGUIDialogBoxeeLoginWizardAddExistingUser() : CGUIDialogBoxeeWizardBase(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_ADD_EXISTING_USER,"boxee_login_wizard_add_existing_user.xml","CGUIDialogBoxeeLoginWizardAddExistingUser")
{
  m_rememberPassword = false;
}

CGUIDialogBoxeeLoginWizardAddExistingUser::~CGUIDialogBoxeeLoginWizardAddExistingUser()
{

}

void CGUIDialogBoxeeLoginWizardAddExistingUser::OnInitWindow()
{
  CGUIDialogBoxeeWizardBase::OnInitWindow();

  m_rememberPassword = true;
  ((CGUIRadioButtonControl*)GetControl(REMEMBER_PASSWORD_CONTROL_ID))->SetSelected(m_rememberPassword);

  SET_CONTROL_FOCUS(USERNAME_EDIT_CONTROL_ID,0);
}

bool CGUIDialogBoxeeLoginWizardAddExistingUser::OnAction(const CAction& action)
{
  return CGUIDialogBoxeeWizardBase::OnAction(action);
}

bool CGUIDialogBoxeeLoginWizardAddExistingUser::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    if (HandleClick(message))
    {
      // continue to stay in this screen
      return true;
    }
  }
  break;
  }

  return CGUIDialogBoxeeWizardBase::OnMessage(message);
}

bool CGUIDialogBoxeeLoginWizardAddExistingUser::HandleClick(CGUIMessage& message)
{
  int iControl = message.GetSenderId();

  switch(iControl)
  {
  case REMEMBER_PASSWORD_CONTROL_ID:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddExistingUser::HandleClick - handling click on [Control=%d=REMEMBER_PASSWORD] (blw)(digwiz)",iControl);
    m_rememberPassword = ((CGUIRadioButtonControl*)GetControl(REMEMBER_PASSWORD_CONTROL_ID))->IsSelected();
    return true;
  }
  break;
  case SHOW_PASSWORD_CONTROL_ID:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddExistingUser::HandleClick - handling click on [Control=%d=SHOW_PASSWORD] (blw)(digwiz)",iControl);

    CGUIRadioButtonControl* pControl = (CGUIRadioButtonControl*)GetControl(SHOW_PASSWORD_CONTROL_ID);
    if (pControl)
    {
      if (pControl->IsSelected())
      {
        ((CGUIEditControl*)GetControl(PASSWORD_EDIT_CONTROL_ID))->SetInputType(CGUIEditControl::INPUT_TYPE_TEXT,0);
      }
      else
      {
        ((CGUIEditControl*)GetControl(PASSWORD_EDIT_CONTROL_ID))->SetInputType(CGUIEditControl::INPUT_TYPE_PASSWORD,0);
      }
    }

    return true;
  }
  break;
  case DIALOG_WIZARD_BUTTON_NEXT:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddExistingUser::HandleClick - handling click on [Control=%d=LOGIN=BUTTON_NEXT] (blw)(digwiz)",iControl);

    if (!OnLoginButton())
    {
      // stay in the dialog
      return true;
    }
  }
  break;
  }

  return false;
}

bool CGUIDialogBoxeeLoginWizardAddExistingUser::OnLoginButton()
{
  if (!CanExecuteLogin())
  {
    // Error logs was written in CanExecuteLogin()
    return false;
  }

  CStdString username = ((CGUIEditControl*)GetControl(USERNAME_EDIT_CONTROL_ID))->GetLabel2();
  CStdString password = ((CGUIEditControl*)GetControl(PASSWORD_EDIT_CONTROL_ID))->GetLabel2();
  password = CBoxeeLoginManager::EncodePassword(password);
  BOXEE::BXLoginStatus loginStatus = CBoxeeLoginManager::DoLogin(username,password,m_rememberPassword);

  if (loginStatus == BOXEE::LOGIN_SUCCESS)
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddExistingUser::OnLoginButton - Login Succeeded [loginStatus=%d=LOGIN_SUCCESS] -> Going to call FinishSuccessfulLogin(true) (blw)",loginStatus);
    g_application.GetBoxeeLoginManager().FinishSuccessfulLogin();

    return true;
  }

  ((CGUIEditControl*)GetControl(PASSWORD_EDIT_CONTROL_ID))->SetLabel2("");
  SET_CONTROL_FOCUS(PASSWORD_EDIT_CONTROL_ID,0);

  return false;
}

bool CGUIDialogBoxeeLoginWizardAddExistingUser::CanExecuteLogin()
{
  if(IsThereEmptyFieldsForLogin())
  {
    // Error logs was written in IsThereEmptyfieldsForLogin()
    return false;
  }

  const CStdString username = ((CGUIEditControl*)GetControl(USERNAME_EDIT_CONTROL_ID))->GetLabel2();
  if (CBoxeeLoginManager::DoesThisUserAlreadyExist(username))
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddExistingUser::CanExecuteLogin - user [%s] already exist on this machine (blw)",username.c_str());
    CGUIDialogOK2::ShowAndGetInput(20068,51603);
    return false;
  }

  bool isConnectedToInternet = g_application.IsConnectedToInternet();
  if(!isConnectedToInternet)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddExistingUser::CanExecuteLogin - can't add user since [isConnectedToInternet=%d] (blw)",isConnectedToInternet);
    CGUIDialogOK2::ShowAndGetInput(53743, 53744);
    return false;
  }

  return true;
}

bool CGUIDialogBoxeeLoginWizardAddExistingUser::IsThereEmptyFieldsForLogin()
{
  CStdString username = ((CGUIEditControl*)GetControl(USERNAME_EDIT_CONTROL_ID))->GetLabel2();

  if(username.IsEmpty())
  {
    // Case of empty UserName field
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddExistingUser::IsThereEmptyfieldsForLogin - [username=%s] is empty (blw)",username.c_str());
    CGUIDialogOK2::ShowAndGetInput(20068,51058);
    SET_CONTROL_FOCUS(USERNAME_EDIT_CONTROL_ID,0);
    return true;
  }

  CStdString password = ((CGUIEditControl*)GetControl(PASSWORD_EDIT_CONTROL_ID))->GetLabel2();

  if(password.IsEmpty())
  {
    // Case of empty Password field
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddExistingUser::IsThereEmptyfieldsForLogin - [password=%s] is empty (blw)",password.c_str());
    CGUIDialogOK2::ShowAndGetInput(20068,51059);
    SET_CONTROL_FOCUS(PASSWORD_EDIT_CONTROL_ID,0);
    return true;
  }

  return false;
}

bool CGUIDialogBoxeeLoginWizardAddExistingUser::HandleClickNext()
{
  return true;
}

bool CGUIDialogBoxeeLoginWizardAddExistingUser::HandleClickBack()
{
  return true;
}

bool CGUIDialogBoxeeLoginWizardAddExistingUser::GetIsRememberPassword()
{
  return m_rememberPassword;
}
