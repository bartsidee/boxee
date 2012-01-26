
#include "GUIDialogBoxeeLoggingIn.h"
#include "Settings.h"
#include "GUIWindowManager.h"
#include "FileItem.h"
#include "BoxeeAuthenticator.h"
#include "Application.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "BoxeeUtils.h"

#define USER_NAME_LABEL 1222
#define WINDOW_MSG_CLOSE_LOGGING_IN (GUI_MSG_USER + 101)

CGUIDialogBoxeeLoggingIn::CGUIDialogBoxeeLoggingIn(void) : CGUIDialog(WINDOW_DIALOG_BOXEE_LOGGING_IN, "boxee_logging_in.xml")
{
  Reset();
  BOXEE::Boxee::GetInstance().AddListener(this);
}

CGUIDialogBoxeeLoggingIn::~CGUIDialogBoxeeLoggingIn(void)
{
  BOXEE::Boxee::GetInstance().RemoveListener(this);
}

bool CGUIDialogBoxeeLoggingIn::OnAction(const CAction &action)
{
  // don't allow any built in actions to act here.
  // this forces only navigation type actions to be performed.
  if (action.id == ACTION_BUILT_IN_FUNCTION || action.id == ACTION_PREVIOUS_MENU)
    return true;  // pretend we handled it

  return CGUIDialog::OnAction(action);
}

void CGUIDialogBoxeeLoggingIn::Render()
{
  CGUIDialog::Render();
}

void CGUIDialogBoxeeLoggingIn::Reset()
{
  m_nProfileToLoad = -1;
  m_bLoginOk = false;
  m_userObj.SetValid(false);
}

bool CGUIDialogBoxeeLoggingIn::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      SET_CONTROL_LABEL(USER_NAME_LABEL, m_strName);
      return true;
    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
      return CGUIDialog::OnMessage(message);
    }
    break;

  case WINDOW_MSG_CLOSE_LOGGING_IN:
    {
      LoginDone();
      Close();

      if (m_eLoginStat == BOXEE::LOGIN_SUCCESS)
      {
        g_windowManager.ChangeActiveWindow(WINDOW_BOXEE_LOGING_PROMPT);
      }
    }
    return true;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogBoxeeLoggingIn::SetName(const CStdString &strName)
{
  m_strName = strName;
}

void CGUIDialogBoxeeLoggingIn::SetProfileToLoad(int nProfile)
{
  m_nProfileToLoad = nProfile;
}

void CGUIDialogBoxeeLoggingIn::LoginDone() 
{
  if ((m_eLoginStat == BOXEE::LOGIN_SUCCESS) && m_nProfileToLoad >= 0 && m_userObj.IsValid())
  {
    CLog::Log(LOGINFO,"Loading profile [%d] (login)", m_nProfileToLoad);            
    // NOTE: Loading the profile closes all windows since it's reloading the skin (for string localization reasons)
    // it has to run from gui thread 
    g_settings.LoadProfile(m_nProfileToLoad);    
  }  
}

void CGUIDialogBoxeeLoggingIn::OnLoginEnded(const BOXEE::BXCredentials &creds, BOXEE::BXLoginStatus eResult, const BOXEE::BXObject &userObj) 
{
  BOXEE::BXCredentials credsTmp = creds;
  //CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoggingIn::OnLoginEnded - Enter function with [creds.UserName=%s][creds.Pass=%s][LoginStatus=%d=%s]. [m_nProfileToLoad=%d] (login)",credsTmp.GetUserName().c_str(),credsTmp.GetPassword().c_str(),(int)eResult,BOXEE::Boxee::CBoxeeLoginStatusEnumAsString(eResult),m_nProfileToLoad);

  m_eLoginStat = eResult; 
  m_userObj = userObj;

  if ((m_eLoginStat == BOXEE::LOGIN_SUCCESS) && m_nProfileToLoad >= 0 && m_userObj.IsValid())
  {
    CLog::Log(LOGINFO,"CGUIDialogBoxeeLoggingIn::OnLoginEnded - Update profile [%d] (up)(login)", m_nProfileToLoad);    
    UpdateProfileToLoad();
    
#if 0
    // disable authenticator for now
    //
    // Initialize authenticator
    if (g_application.GetBoxeeAuthenticator().Init())
    {
      if (!g_application.GetBoxeeAuthenticator().AuthenticateMain())
      {
        CLog::Log(LOGWARNING,"CGUIDialogBoxeeLoggingIn::OnLoginEnded - FAILED to authenticate boxee main (login)");
      }
    }
    else
    {
      CLog::Log(LOGWARNING,"CGUIDialogBoxeeLoggingIn::OnLoginEnded - FAILED to initialize boxee authenticator (login)");
    }
#endif
    
  }  

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoggingIn::OnLoginEnded - Going to send WINDOW_MSG_CLOSE_LOGGING_IN message (login)");

  CGUIMessage winmsg(WINDOW_MSG_CLOSE_LOGGING_IN, GetID(), 0);
  g_windowManager.SendThreadMessage(winmsg);
}

BOXEE::BXLoginStatus CGUIDialogBoxeeLoggingIn::IsLoginSuccessful()
{
  return m_eLoginStat;
}

void CGUIDialogBoxeeLoggingIn::UpdateProfileToLoad()
{
  return BoxeeUtils::UpdateProfile(m_nProfileToLoad,m_userObj);
}

