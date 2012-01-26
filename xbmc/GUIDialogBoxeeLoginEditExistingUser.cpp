#include "GUIDialogBoxeeLoginEditExistingUser.h"
#include "GUIWindowManager.h"
#include "BoxeeLoginManager.h"
#include "utils/log.h"
#include "GUIRadioButtonControl.h"
#include "GUIEditControl.h"
#include "GUIImage.h"
#include "Application.h"
#include "GUIDialogOK2.h"
#include "BoxeeLoginWizardManager.h"
#include "GUIUserMessages.h"
#include "lib/libBoxee/bxcredentials.h"
#include "lib/libBoxee/boxee.h"

#define PASSWORD_EDIT_CONTROL_ID        8602
#define REMEMBER_PASSWORD_CONTROL_ID    8603
#define LOGIN_BUTTON_CONTROL_ID         8604
#define REMOVE_BUTTON_CONTROL_ID        8606
#define SHOW_PASSWORD_CONTROL_ID        8607

#define USERNAME_LABEL_CONTROL_ID       8610
#define USERNAME_THUMB_CONTROL_ID       8650

#define CHECK_DIALOG_PASSWORD_FILED_DELAY       15

CGUIDialogBoxeeLoginEditExistingUser::CGUIDialogBoxeeLoginEditExistingUser() : CGUIDialog(WINDOW_DIALOG_BOXEE_LOGIN_EDIT_EXISTING_USER,"boxee_login_edit_existing_user.xml")
{
  m_checkPasswordFieldDelay = 0;
}

CGUIDialogBoxeeLoginEditExistingUser::~CGUIDialogBoxeeLoginEditExistingUser()
{

}

void CGUIDialogBoxeeLoginEditExistingUser::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  m_checkPasswordFieldDelay = 0;
  m_needToCheckEmptyPassword = true;

  SET_CONTROL_LABEL(USERNAME_LABEL_CONTROL_ID,m_username);
  ((CGUIEditControl*)GetControl(PASSWORD_EDIT_CONTROL_ID))->SetLabel2(m_password);
  ((CGUIRadioButtonControl*)GetControl(REMEMBER_PASSWORD_CONTROL_ID))->SetSelected(m_rememberPassword);
  ((CGUIEditControl*)GetControl(PASSWORD_EDIT_CONTROL_ID))->SetInputType(CGUIEditControl::INPUT_TYPE_PASSWORD,0);
  ((CGUIImage*)GetControl(USERNAME_THUMB_CONTROL_ID))->SetFileName(m_thumb);

  if (!m_password.IsEmpty())
  {
    SET_CONTROL_HIDDEN(SHOW_PASSWORD_CONTROL_ID);
  }
  else
  {
    SET_CONTROL_VISIBLE(SHOW_PASSWORD_CONTROL_ID);
    m_needToCheckEmptyPassword = false;
  }

  SET_CONTROL_FOCUS(PASSWORD_EDIT_CONTROL_ID,0);
}

bool CGUIDialogBoxeeLoginEditExistingUser::OnAction(const CAction& action)
{
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeeLoginEditExistingUser::OnMessage(CGUIMessage& message)
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

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogBoxeeLoginEditExistingUser::Render()
{
  m_checkPasswordFieldDelay++;

  if (m_needToCheckEmptyPassword && (m_checkPasswordFieldDelay > CHECK_DIALOG_PASSWORD_FILED_DELAY))
  {
    m_checkPasswordFieldDelay = 0;

    if (IsPasswordFiledEmpty())
    {
      SET_CONTROL_VISIBLE(SHOW_PASSWORD_CONTROL_ID);
      m_needToCheckEmptyPassword = false;
    }
  }

  CGUIDialog::Render();
}

bool CGUIDialogBoxeeLoginEditExistingUser::IsPasswordFiledEmpty()
{
  CGUIEditControl* pPasswordEditControl = (CGUIEditControl*)GetControl(PASSWORD_EDIT_CONTROL_ID);
  if (!pPasswordEditControl)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginEditExistingUser::IsPasswordFiledEmpty - FAILED to PASSWORD_EDIT_CONTROL (login)");
    return false;
  }

  CStdString password = pPasswordEditControl->GetLabel2();

  return password.IsEmpty();
}

bool CGUIDialogBoxeeLoginEditExistingUser::HandleClick(CGUIMessage& message)
{
  int iControl = message.GetSenderId();

  switch(iControl)
  {
  case REMEMBER_PASSWORD_CONTROL_ID:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginEditExistingUser::HandleClick - handling click on [Control=%d=REMEMBER_PASSWORD] (login)",iControl);
    m_rememberPassword = ((CGUIRadioButtonControl*)GetControl(REMEMBER_PASSWORD_CONTROL_ID))->IsSelected();
    return true;
  }
  break;
  case SHOW_PASSWORD_CONTROL_ID:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginEditExistingUser::HandleClick - handling click on [Control=%d=SHOW_PASSWORD] (login)",iControl);

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
  case LOGIN_BUTTON_CONTROL_ID:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginEditExistingUser::HandleClick - handling click on [Control=%d=LOGIN_BUTTON_CONTROL_ID] (login)",iControl);

    if (OnLoginButton())
    {
      Close();
    }

    return true;
  }
  break;
  case REMOVE_BUTTON_CONTROL_ID:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginEditExistingUser::HandleClick - handling click on [Control=%d=REMOVE_BUTTON_CONTROL_ID] (login)",iControl);

    if (OnRemoveButton())
    {
      Close();
    }

    // stay in the dialog
    return true;
  }
  break;
  }

  return false;
}

bool CGUIDialogBoxeeLoginEditExistingUser::OnLoginButton()
{
  if (!CanExecuteLogin())
  {
    // Error logs was written in CanExecuteLogin()
    return false;
  }

  CStdString password = ((CGUIEditControl*)GetControl(PASSWORD_EDIT_CONTROL_ID))->GetLabel2();
  if(password != m_password)
  {
    // user changed the password in the editbox -> need to encode and save it
    password = CBoxeeLoginManager::EncodePassword(password);
    m_password = password;
  }

  BOXEE::BXLoginStatus loginStatus = CBoxeeLoginManager::DoLogin(m_userId,m_password,m_rememberPassword,m_userProfileIndex);

  if (loginStatus == BOXEE::LOGIN_SUCCESS)
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginEditExistingUser::OnLoginButton - Login Succeeded [loginStatus=%d=LOGIN_SUCCESS] -> Going to call FinishSuccessfulLogin(true) (login)",loginStatus);
    g_application.GetBoxeeLoginManager().FinishSuccessfulLogin();

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginEditExistingUser::OnLoginButton - Going to WINDOW_HOME (login)");
    g_windowManager.ChangeActiveWindow(WINDOW_HOME);

    return true;
  }

  ((CGUIEditControl*)GetControl(PASSWORD_EDIT_CONTROL_ID))->SetLabel2("");
  SET_CONTROL_FOCUS(PASSWORD_EDIT_CONTROL_ID,0);

  return false;
}

bool CGUIDialogBoxeeLoginEditExistingUser::CanExecuteLogin()
{
  bool isConnectedToInternet = g_application.IsConnectedToInternet();
  if(!isConnectedToInternet)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginEditExistingUser::CanExecuteLogin - can't add user since [isConnectedToInternet=%d] (login)",isConnectedToInternet);
    CGUIDialogOK2::ShowAndGetInput(53743, 53744);
    return false;
  }

  CStdString password = ((CGUIEditControl*)GetControl(PASSWORD_EDIT_CONTROL_ID))->GetLabel2();

  if(password.IsEmpty())
  {
    // Case of empty Password field
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginEditExistingUser::CanExecuteLogin - [password=%s] is empty (login)",password.c_str());
    CGUIDialogOK2::ShowAndGetInput(20068,51059);
    SET_CONTROL_FOCUS(PASSWORD_EDIT_CONTROL_ID,0);
    return false;
  }

  return true;
}

bool CGUIDialogBoxeeLoginEditExistingUser::OnRemoveButton()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginEditExistingUser::OnRemoveButton - Enter function. [m_userSelectedIndex=%d][username=%s] (login)",m_userProfileIndex,m_username.c_str());

  bool succeeded = g_settings.DeleteProfile(m_userProfileIndex);

  if (succeeded)
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginEditExistingUser::OnRemoveButton - User [username=%s] was successfully removed (login)",m_username.c_str());
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginEditExistingUser::OnRemoveButton - FAILED to remove user [username=%s] (login)",m_username.c_str());
  }

  CGUIMessage msg(GUI_MSG_UPDATE, WINDOW_LOGIN_SCREEN, 0);
  g_windowManager.SendThreadMessage(msg);

  return succeeded;

}

bool CGUIDialogBoxeeLoginEditExistingUser::HandleClickNext()
{
  return true;
}

bool CGUIDialogBoxeeLoginEditExistingUser::HandleClickBack()
{
  return true;
}

void CGUIDialogBoxeeLoginEditExistingUser::SetUsername(const CStdString& username)
{
  m_username = username;
}

CStdString CGUIDialogBoxeeLoginEditExistingUser::GetUsername()
{
  return m_username;
}

void CGUIDialogBoxeeLoginEditExistingUser::SetUserId(const CStdString& userId)
{
  m_userId = userId;
}

CStdString CGUIDialogBoxeeLoginEditExistingUser::GetUserId()
{
  return m_userId;
}

void CGUIDialogBoxeeLoginEditExistingUser::SetPassword(const CStdString& password)
{
  m_password = password;
}

CStdString CGUIDialogBoxeeLoginEditExistingUser::GetPassword()
{
  return m_password;
}

void CGUIDialogBoxeeLoginEditExistingUser::SetRememberPassword(bool rememberPassword)
{
  m_rememberPassword = rememberPassword;
}

bool CGUIDialogBoxeeLoginEditExistingUser::IsRememberPassword()
{
  return m_rememberPassword;
}

void CGUIDialogBoxeeLoginEditExistingUser::SetUserProfileIndex(int userProfileIndex)
{
  m_userProfileIndex = userProfileIndex;
}

int CGUIDialogBoxeeLoginEditExistingUser::GetUserProfileIndex()
{
  return m_userProfileIndex;
}

void CGUIDialogBoxeeLoginEditExistingUser::SetThumb(const CStdString& thumb)
{
  m_thumb = thumb;
}

CStdString CGUIDialogBoxeeLoginEditExistingUser::GetThumb()
{
  return m_thumb;
}

bool CGUIDialogBoxeeLoginEditExistingUser::Show(int userProfileIndex, const CStdString& username, const CStdString& userId, CStdString& password, bool& rememberPassword, const CStdString& thumb)
{
  if(userProfileIndex >= (int)g_settings.m_vecProfiles.size())
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginEditExistingUser::Show - Could not find profile for [m_userSelectedIndex=%d]. [VecProfilesSize=%zu] (login)",userProfileIndex,g_settings.m_vecProfiles.size());
    return false;
  }

  CGUIDialogBoxeeLoginEditExistingUser* pDlg = (CGUIDialogBoxeeLoginEditExistingUser*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_LOGIN_EDIT_EXISTING_USER);
  if (!pDlg)
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginEditExistingUser::Show - FAILED to get GUIDialogBoxeeLoginEditExistingUser object (login)");
    return false;
  }

  pDlg->SetUsername(username);
  pDlg->SetUserId(userId);
  pDlg->SetPassword(password);
  pDlg->SetRememberPassword(rememberPassword);
  pDlg->SetUserProfileIndex(userProfileIndex);
  pDlg->SetThumb(thumb);

  pDlg->DoModal();

  password = pDlg->GetPassword();
  rememberPassword = pDlg->IsRememberPassword();

  return true;
}
