#include "BoxeeLoginWizardManager.h"
#include "GUIWindowManager.h"
#include "GUIDialogBoxeeWizardBase.h"
#include "utils/log.h"
#include "BoxeeUtils.h"
#include "LocalizeStrings.h"
#include "GUIDialogBoxeeLoggingIn.h"
#include "BoxeeLoginManager.h"
#include "Application.h"
#include "GUIDialogOK2.h"
#include "GUIDialogYesNo2.h"
#include "GUIWindowSettingsScreenSimpleCalibration.h"
#include "FileCurl.h"
#include "GUIDialogBoxeeLoginWizardChooseUserToAddType.h"
#include "GUIDialogBoxeeLoginWizardAddExistingUser.h"
#include "GUIDialogBoxeeLoginWizardAddNewUser.h"
#include "GUIDialogBoxeeLoginWizardNewUserDetails.h"

CBoxeeLoginWizardManager::CBoxeeLoginWizardManager() : CBoxeeSimpleDialogWizardManager()
{

}

CBoxeeLoginWizardManager::~CBoxeeLoginWizardManager()
{

}

CBoxeeLoginWizardManager& CBoxeeLoginWizardManager::GetInstance()
{
  static CBoxeeLoginWizardManager boxeeLoginWizardManager;
  return boxeeLoginWizardManager;
}

bool CBoxeeLoginWizardManager::RunWizard(bool shouldEndWizardOnEmptyStack)
{
  m_serverUserInfo.Reset();
  m_isAddingNewUser = false;

  if (Run(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_CHOOSE_USER_TO_ADD, shouldEndWizardOnEmptyStack))
  {
    return true;
  }

  return false;
}

CGUIDialogBoxeeWizardBase* CBoxeeLoginWizardManager::HandleNextAction(CGUIDialogBoxeeWizardBase* pDialog, bool& addCurrentDlgToStack)
{
  if (!pDialog)
  {
    CLog::Log(LOGERROR,"CBoxeeLoginWizardManager::HandleNextAction - Enter function with a NULL pointer (blw)(digwiz)");
    return NULL;
  }

  int id = pDialog->GetID();

  CLog::Log(LOGDEBUG,"CBoxeeLoginWizardManager::HandleNextAction - Enter function with [id=%d] (blw)(digwiz)",id);

  CGUIDialogBoxeeWizardBase* pNextDialog = NULL;

  switch(id)
  {
  case WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_CHOOSE_USER_TO_ADD:
  {
    CGUIDialogBoxeeLoginWizardChooseUserToAddType* pAddUserDlg = (CGUIDialogBoxeeLoginWizardChooseUserToAddType*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_CHOOSE_USER_TO_ADD);
    if (!pAddUserDlg)
    {
      CLog::Log(LOGERROR,"CBoxeeLoginWizardManager::HandleNextAction - FAILED to get DialogBoxeeLoginWizardAddUser (blw)(digwiz)");
      break;
    }

    m_isAddingNewUser = pAddUserDlg->IsAddingNewUser();

    if (m_isAddingNewUser)
    {
      pNextDialog = (CGUIDialogBoxeeWizardBase*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_ADD_NEW_USER);
      CLog::Log(LOGDEBUG,"CBoxeeLoginWizardManager::HandleNextAction - IsAddingNewUser returned TRUE. Set next dialog to ADD_NEW_USER. [isAddingNewUser=%d=TRUE] (blw)(digwiz)",m_isAddingNewUser);
    }
    else
    {
      pNextDialog = (CGUIDialogBoxeeWizardBase*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_ADD_EXISTING_USER);
      CLog::Log(LOGDEBUG,"CBoxeeLoginWizardManager::HandleNextAction - IsAddingNewUser returned FALSE. Set next dialog to ADD_EXISTING_USER. [isAddingNewUser=%d=FALSE] (blw)(digwiz)",m_isAddingNewUser);
    }

    IsAddingDialogToStack(pNextDialog,addCurrentDlgToStack);
  }
  break;
  case WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_ADD_EXISTING_USER:
  {
    pNextDialog = (CGUIDialogBoxeeWizardBase*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_MENU_CUST);
  }
  break;
  case WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_ADD_NEW_USER:
  {
    pNextDialog = (CGUIDialogBoxeeWizardBase*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_NEW_USER_DETAILS);
  }
  break;
  case WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_NEW_USER_DETAILS:
  {
    pNextDialog = (CGUIDialogBoxeeWizardBase*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_TOU);
  }
  break;
  case WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_TOU:
  {
#ifdef HAS_EMBEDDED
    pNextDialog = (CGUIDialogBoxeeWizardBase*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_CONNECT_SOCIAL_SERVICES);
#else
    pNextDialog = (CGUIDialogBoxeeWizardBase*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_MENU_CUST);
#endif
  }
  break;
  case WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_CONNECT_SOCIAL_SERVICES:
  {
    pNextDialog = (CGUIDialogBoxeeWizardBase*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_MENU_CUST);
  }
  break;
  case WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_MENU_CUST:
  {
    if (m_isAddingNewUser)
    {
      pNextDialog = (CGUIDialogBoxeeWizardBase*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_CONFIRMATION);
    }
    else
    {
#ifdef HAS_EMBEDDED
      pNextDialog = (CGUIDialogBoxeeWizardBase*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_QUICK_TIP_AIRPLAY);
#else
      SetWizardComplete(true);
#endif
    }
  }
  break;
  case WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_CONFIRMATION:
  {
#ifdef HAS_EMBEDDED
    pNextDialog = (CGUIDialogBoxeeWizardBase*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_QUICK_TIP_AIRPLAY);
#else
    SetWizardComplete(true);
#endif
  }
  break;
  case WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_QUICK_TIP_AIRPLAY:
  {
    if (strcmpi(BoxeeUtils::GetPlatformStr(),"dlink.dsm380") == 0)
    {
      CLog::Log(LOGDEBUG,"CBoxeeLoginWizardManager::HandleNextAction - [platform=%s] -> show RemoteQuickTip (blw)(digwiz)",BoxeeUtils::GetPlatformStr());
      pNextDialog = (CGUIDialogBoxeeWizardBase*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_QUICK_TIP);
    }
    else
    {
      SetWizardComplete(true);
    }
  }
  break;
  case WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_QUICK_TIP:
  {
    SetWizardComplete(true);
  }
  break;
  default:
  {
    CLog::Log(LOGERROR,"CBoxeeLoginWizardManager::HandleNextAction - FAILED to handle WindowId [%d] (blw)(digwiz)",id);
  }
  break;
  }

  if (addCurrentDlgToStack)
  {
    AddToStack(id);
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeLoginWizardManager::HandleNextAction - Not adding [id=%d] to stack. [addCurrentDlgToStack=%d][DialogStackSize=%d] (blw)(digwiz)",id,addCurrentDlgToStack,GetStackSize());
  }

  CLog::Log(LOGDEBUG,"CBoxeeLoginWizardManager::HandleNextAction - Exit function and return [NextDialog=%p] for [id=%d] (blw)(digwiz)",pNextDialog,id);

  return pNextDialog;
}

bool CBoxeeLoginWizardManager::OnWizardComplete()
{
  size_t numOfProfiles = g_settings.m_vecProfiles.size();

#ifndef HAS_EMBEDDED

  CLog::Log(LOGDEBUG,"CBoxeeLoginWizardManager::OnWizardComplete - Case of new user [numOfProfiles=%zu] (login)",numOfProfiles);

  if (numOfProfiles == 2)
  {
    CLog::Log(LOGDEBUG,"CBoxeeLoginWizardManager::OnWizardComplete - Case of first user [numOfProfiles=%zu] -> Propose to the user to set Screen Calibration (login)",numOfProfiles);

    if(CGUIDialogYesNo2::ShowAndGetInput(53400,53401,53420))
    {
      CGUIWindowSettingsScreenSimpleCalibration* pSimpleCalibration = (CGUIWindowSettingsScreenSimpleCalibration*)g_windowManager.GetWindow(WINDOW_SCREEN_SIMPLE_CALIBRATION);
      if (pSimpleCalibration)
      {
        pSimpleCalibration->SetLaunchFromLogin(true);
      }
      else
      {
        CLog::Log(LOGERROR,"CBoxeeLoginWizardManager::OnWizardComplete - FAILED to get CGUIWindowSettingsScreenSimpleCalibration object (login)");
      }

      CLog::Log(LOGDEBUG,"CBoxeeLoginWizardManager::OnWizardComplete - Because first user [numOfProfiles=%zu] and YES was hit -> Going to WINDOW_SCREEN_SIMPLE_CALIBRATION (login)",numOfProfiles);
      g_windowManager.ChangeActiveWindow(WINDOW_SCREEN_SIMPLE_CALIBRATION);
    }
    else
    {
      g_windowManager.ChangeActiveWindow(WINDOW_HOME);
      CLog::Log(LOGDEBUG,"CBoxeeLoginWizardManager::OnWizardComplete - Because first user [numOfProfiles=%zu] and NO was hit -> Going to WINDOW_HOME (login)",numOfProfiles);
    }
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeLoginWizardManager::OnWizardComplete - [numOfProfiles=%zu] -> Going to WINDOW_HOME (login)",numOfProfiles);
    g_windowManager.ChangeActiveWindow(WINDOW_HOME);
  }
#else

  CLog::Log(LOGDEBUG,"CBoxeeLoginWizardManager::OnWizardComplete - [numOfProfiles=%zu] -> Going to WINDOW_HOME (login)",numOfProfiles);
  g_windowManager.ChangeActiveWindow(WINDOW_HOME);

#endif

  return true;
}

void CBoxeeLoginWizardManager::IsAddingDialogToStack(CGUIDialogBoxeeWizardBase* pNextDialog,bool& addCurrentDlgToStack)
{
  addCurrentDlgToStack = (pNextDialog != NULL);
}

CServerUserInfo CBoxeeLoginWizardManager::GetServerUserInfo()
{
  return m_serverUserInfo;
}

CServerUserInfo& CBoxeeLoginWizardManager::GetServerUserInfoByRef()
{
  return m_serverUserInfo;
}

void CServerUserInfo::Reset()
{
  CLog::Log(LOGDEBUG,"CServerUserInfo::Reset - enter function (blw)(cnu)(login)");

  m_id = "";
  m_name = "";
  m_firstName = "";
  m_lastName = "";
  m_link = "";
  m_username = "";
  m_birthday = "";
  m_gender = "";
  m_email = "";
  m_timezone = 0;
  m_locale = "";
  m_verified = false;
  m_updatedTime = "";
  m_accessToken = "";
  m_code = "";

  m_serviceId = "";

  m_isInitialize = false;
}

bool CServerUserInfo::InitializeByJson(const Json::Value& jResponse)
{
  if (!jResponse.isMember("id"))
  {
    CLog::Log(LOGERROR,"CServerUserInfo::InitializeByJson - FAILED to get parameter <id> from jResponse (blw)(cnu)(login)");
    //return false;
  }
  else
  {
    m_id = jResponse["id"].asString();
  }

  if (!jResponse.isMember("name"))
  {
    CLog::Log(LOGERROR,"CServerUserInfo::InitializeByJson - FAILED to get parameter <name> from jResponse (blw)(cnu)(login)");
    return false;
  }
  else
  {
    m_name = jResponse["name"].asString();
    m_name.Replace(' ','.');
    m_name.Replace('-','.');
  }

  if (!jResponse.isMember("first_name"))
  {
    CLog::Log(LOGERROR,"CServerUserInfo::InitializeByJson - FAILED to get parameter <first_name> from jResponse (blw)(cnu)(login)");
    //return false;
  }
  else
  {
    m_firstName = jResponse["first_name"].asString();
  }

  if (!jResponse.isMember("last_name"))
  {
    CLog::Log(LOGERROR,"CServerUserInfo::InitializeByJson - FAILED to get parameter <last_name> from jResponse (blw)(cnu)(login)");
    //return false;
  }
  else
  {
    m_lastName = jResponse["last_name"].asString();
  }

  if (!jResponse.isMember("link"))
  {
    CLog::Log(LOGERROR,"CServerUserInfo::InitializeByJson - FAILED to get parameter <link> from jResponse (blw)(cnu)(login)");
    //return false;
  }
  else
  {
    m_link = jResponse["link"].asString();
  }

  if (!jResponse.isMember("username"))
  {
    CLog::Log(LOGERROR,"CServerUserInfo::InitializeByJson - FAILED to get parameter <username> from jResponse (blw)(cnu)(login)");
    //return false;
  }
  else
  {
    m_username = jResponse["username"].asString();
  }

  if (!jResponse.isMember("birthday"))
  {
    CLog::Log(LOGERROR,"CServerUserInfo::InitializeByJson - FAILED to get parameter <birthday> from jResponse (blw)(cnu)(login)");
    //return false;
  }
  else
  {
    m_birthday = jResponse["birthday"].asString();
  }

  if (!jResponse.isMember("gender"))
  {
    CLog::Log(LOGERROR,"CServerUserInfo::InitializeByJson - FAILED to get parameter <gender> from jResponse (blw)(cnu)(login)");
    //return false;
  }
  else
  {
    m_gender = jResponse["gender"].asString();
  }

  if (!jResponse.isMember("email"))
  {
    CLog::Log(LOGERROR,"CServerUserInfo::InitializeByJson - FAILED to get parameter <email> from jResponse (blw)(cnu)(login)");
    //return false;
  }
  else
  {
    m_email = jResponse["email"].asString();
  }

  if (!jResponse.isMember("timezone"))
  {
    CLog::Log(LOGERROR,"CServerUserInfo::InitializeByJson - FAILED to get parameter <timezone> from jResponse (blw)(cnu)(login)");
    //return false;
  }
  else
  {
    m_timezone = jResponse["timezone"].asInt();
  }

  if (!jResponse.isMember("locale"))
  {
    CLog::Log(LOGERROR,"CServerUserInfo::InitializeByJson - FAILED to get parameter <locale> from jResponse (blw)(cnu)(login)");
    //return false;
  }
  else
  {
    m_locale = jResponse["locale"].asString();
  }

  if (!jResponse.isMember("verified"))
  {
    CLog::Log(LOGERROR,"CServerUserInfo::InitializeByJson - FAILED to get parameter <verified> from jResponse (blw)(cnu)(login)");
    //return false;
  }
  else
  {
    m_verified = jResponse["verified"].asBool();
  }

  if (!jResponse.isMember("updated_time"))
  {
    CLog::Log(LOGERROR,"CServerUserInfo::InitializeByJson - FAILED to get parameter <updated_time> from jResponse (blw)(cnu)(login)");
    //return false;
  }
  else
  {
    m_updatedTime = jResponse["updated_time"].asString();
  }

  if (!jResponse.isMember("access_token"))
  {
    CLog::Log(LOGERROR,"CServerUserInfo::InitializeByJson - FAILED to get parameter <access_token> from jResponse (blw)(cnu)(login)");
    return false;
  }
  else
  {
    m_accessToken = jResponse["access_token"].asString();
  }

  if (!jResponse.isMember("code"))
  {
    CLog::Log(LOGERROR,"CServerUserInfo::InitializeByJson - FAILED to get parameter <code> from jResponse (blw)(cnu)(login)");
    return false;
  }
  else
  {
    m_code = jResponse["code"].asString();
  }

  m_isInitialize = true;

  return m_isInitialize;
}

bool CServerUserInfo::IsInitialize()
{
  return m_isInitialize;
}

