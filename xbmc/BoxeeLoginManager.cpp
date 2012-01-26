
#include "StdString.h"
#include "BoxeeLoginManager.h"
#include "Application.h"
#include "Settings.h"
#include "GUIWindowLoginScreen.h"
#include "GUIWindowManager.h"
#include "GUIDialogBoxeeLoggingIn.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogYesNo2.h"
#include "utils/Weather.h"
#include "utils/Network.h"
#include "GUIDialogOK.h"
#include "GUIDialogOK2.h"
#include "BoxeeVersionUpdateManager.h"
#include "SkinInfo.h"
#include "FileSystem/File.h"
#include "SystemInfo.h"
#include "GUIDialogBoxeeUserLogin.h"
#include "SpecialProtocol.h"
#include "GUIWindowSettingsScreenSimpleCalibration.h"
#include "DirectoryCache.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "GUISettings.h"
#include "FileCurl.h"
#include "AppManager.h"
#include "lib/libBoxee/bxcredentials.h"
#include "FileSystem/Directory.h"

#ifdef HAS_PYTHON
#include "lib/libPython/XBPython.h"
#endif

using namespace XFILE;

CBoxeeLoginManager::CBoxeeLoginManager()
{
  m_inChangeUserProcess = false;
}

CBoxeeLoginManager::~CBoxeeLoginManager()
{

}

bool CBoxeeLoginManager::Login()
{
  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::Login - Enter function. Try to do AutoLoging by call DoAutoLogin() (login)");

  CBoxeeLoginStatusTypes::BoxeeLoginStatusEnums autoLoginStatus = CBoxeeLoginStatusTypes::LS_ERROR;

  g_application.SetOfflineMode(true);

  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::Login - After [SetOfflineMode=TRUE]. Going to call canTryAutoLogin() (login)");

  ///////////////////
  // Try AutoLogin //
  ///////////////////

  if(canTryAutoLogin())
  {
    autoLoginStatus = DoAutoLogin();

    if(autoLoginStatus == CBoxeeLoginStatusTypes::LS_OK)
    {
      CLog::Log(LOGDEBUG,"CBoxeeLoginManager::Login - Call DoAutoLogin() SUCCEEDED. [autoLoginStatus=%d=%s] (login)",autoLoginStatus,CBoxeeLoginManager::CBoxeeLoginStatusEnumAsString(autoLoginStatus));
    }
    else if (g_application.IsOfflineMode())
    {
      autoLoginStatus = CBoxeeLoginStatusTypes::LS_OK;
      CLog::Log(LOGDEBUG,"CBoxeeLoginManager::Login - Call DoAutoLogin() FAILED but [IsOfflineMode=%d] ->  autoLoginStatus was set to [%d=%s] (login)",g_application.IsOfflineMode(),autoLoginStatus,CBoxeeLoginManager::CBoxeeLoginStatusEnumAsString(autoLoginStatus));
    }
    else
    {
      // Error log will be written later
    }

    if(autoLoginStatus == CBoxeeLoginStatusTypes::LS_OK)
    {
      // AutoLogin was Successful

      FinishSuccessfulLogin();

      return true;
    }
  }

  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::Login - Call DoAutoLogin() FAILED. [autoLoginStatus=%d=%s] (login)",autoLoginStatus,CBoxeeLoginManager::CBoxeeLoginStatusEnumAsString(autoLoginStatus));

  // Activate CGUIWindowLoginScreen
  CGUIWindowLoginScreen::Show();

  return false;
}

bool CBoxeeLoginManager::canTryAutoLogin()
{
  bool isInChangeUserProcess = IsInChangeUserProcess();
  
  if (isInChangeUserProcess)
  {
    CLog::Log(LOGDEBUG,"CBoxeeLoginManager::canTryAutoLogin - Enter function and [isInChangeUserProcess=%d] -> Exit and return FALSE (login)",isInChangeUserProcess);
    return false;
  }
  
  int numOfProfilesInVec = (int)g_settings.m_vecProfiles.size();
  
  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::canTryAutoLogin - Enter function. [VecProfiles.size=%d] (login)",numOfProfilesInVec);

  // Check if we can try and do AutoLoging (Is there a profile other then the MasterProfile in the vec)
  if(numOfProfilesInVec <= 1)
  {
    CLog::Log(LOGDEBUG,"CBoxeeLoginManager::canTryAutoLogin - There are no profiles. Exit function and return FALSE. [VecProfilesSize=%d] (login)",numOfProfilesInVec);
    return false;
  }
  
  int lastUsedProfileIndex = g_settings.m_iLastUsedProfileIndex;
  int lastLoadedProfileIndex = g_settings.m_iLastLoadedProfileIndex;
  CStdString lastUsedProfileLockCode = g_settings.m_vecProfiles[lastUsedProfileIndex].getLockCode();
  
  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::canTryAutoLogin - [lastUsedProfileIndex=%d][lastLoadedProfileIndex=%d][lastUsedProfileLockCode=%s]. [numOfProfilesInVec=%d] (login)",lastUsedProfileIndex,lastLoadedProfileIndex,lastUsedProfileLockCode.c_str(),numOfProfilesInVec);
    
  if((lastUsedProfileIndex <= 0) || (lastUsedProfileIndex >= numOfProfilesInVec) || (lastUsedProfileLockCode == "-"))
  {
    CLog::Log(LOGWARNING,"CBoxeeLoginManager::canTryAutoLogin - There is a problem with the LastUsedProfile [lastUsedProfileIndex=%d][lastUsedProfileLockCode=%s][numOfProfilesInVec=%d]. Exit function and return FALSE (login)",lastUsedProfileIndex,lastUsedProfileLockCode.c_str(),numOfProfilesInVec);
    return false;    
  }

  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::canTryAutoLogin - Exit function and return TRUE -> Going to try AutoLoging (login)");

  return true;
}

CBoxeeLoginStatusTypes::BoxeeLoginStatusEnums CBoxeeLoginManager::DoAutoLogin()
{    
  // There IS a password saved in profile -> Try to AutoLogin

  int lastUsedProfileIndex = g_settings.m_iLastUsedProfileIndex;
  CStdString lastUsedProfileLockCode = g_settings.m_vecProfiles[lastUsedProfileIndex].getLockCode();

  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::DoAutoLogin - [lastUsedProfileIndex=%d] and its [LockCode=%s] -> Going to try AutoLogin by call ExecuteLoginForRegisterUser() (login)",lastUsedProfileIndex,lastUsedProfileLockCode.c_str());
  
  CBoxeeLoginStatusTypes::BoxeeLoginStatusEnums autoLoginStatus;// = CBoxeeLoginStatusTypes::LS_OK;

  autoLoginStatus = DoUserLogin(lastUsedProfileIndex);

  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::DoAutoLogin - Call to DoUserLogin returned [autoLoginStatus=%d]. Exit function and return [autoLoginStatus=%d] (login)",(int)autoLoginStatus,(int)autoLoginStatus);

  return autoLoginStatus;
}

void CBoxeeLoginManager::FinishSuccessfulLogin(bool firstUser)
{
  DIRECTORY::CDirectory::Create("special://profile/cache");
  g_application.GetHttpCacheManager().Initialize("special://profile/cache");

  PostLoginActions();

  // Initialize CBoxeeClientServerComManager
  bool suceeded = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().Initialize();
  if(!suceeded)
  {
    LOG(LOG_LEVEL_ERROR,"CBoxeeLoginManager::FinishSuccessfulLogin - FAILED to initialize CBoxeeClientServerComManager (login)");
  }

  if (firstUser)
  {
    // case of first user -> propose to first set Screen Calibration

    CLog::Log(LOGDEBUG,"CBoxeeLoginManager::FinishSuccessfulLogin - Case of first user [firstUser=%d] -> Propose to the user to set Screen Calibration (login)",firstUser);

    if(CGUIDialogYesNo2::ShowAndGetInput(53400,53401,53420))
    {
      CGUIWindowSettingsScreenSimpleCalibration* pSimpleCalibration = (CGUIWindowSettingsScreenSimpleCalibration*)g_windowManager.GetWindow(WINDOW_SCREEN_SIMPLE_CALIBRATION);
      if (pSimpleCalibration)
      {
        pSimpleCalibration->SetLaunchFromLogin(true);
      }
      else
      {
        CLog::Log(LOGERROR,"CBoxeeLoginManager::FinishSuccessfulLogin - FAILED to get CGUIWindowSettingsScreenSimpleCalibration object (login)");
      }

      CLog::Log(LOGDEBUG,"CBoxeeLoginManager::FinishSuccessfulLogin - Because first user [firstUser=%d] and YES was hit -> Going to WINDOW_SCREEN_SIMPLE_CALIBRATION (login)",firstUser);
      g_windowManager.ChangeActiveWindow(WINDOW_SCREEN_SIMPLE_CALIBRATION);
    }
    else
    {
      g_windowManager.ChangeActiveWindow(WINDOW_HOME);
      CLog::Log(LOGDEBUG,"CBoxeeLoginManager::FinishSuccessfulLogin - Because first user [firstUser=%d] and NO was hit -> Going to WINDOW_HOME (login)",firstUser);
    }
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeLoginManager::FinishSuccessfulLogin - Because NOT first user [firstUser=%d] -> Going to WINDOW_HOME (login)",firstUser);

    g_windowManager.ChangeActiveWindow(WINDOW_HOME);
  }

  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::FinishSuccessfulLogin - Going to call Application::PostLoginInitializations() (login)");

  g_application.PostLoginInitializations();
}

CBoxeeLoginStatusTypes::BoxeeLoginStatusEnums CBoxeeLoginManager::DoUserLogin(int iItem)
{
  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::DoUserLogin - Enter function with [userProfileIndex=%d] (login)",iItem);
  
  CBoxeeLoginStatusTypes::BoxeeLoginStatusEnums result = CBoxeeLoginStatusTypes::LS_ERROR;
  BOXEE::BXCredentials creds;
  CGUIDialogBoxeeLoggingIn* pLoggingIn = (CGUIDialogBoxeeLoggingIn*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_LOGGING_IN);
  
  if(!pLoggingIn)
  {
    CLog::Log(LOGERROR,"CBoxeeLoginManager::DoUserLogin - FAILED to get CGUIDialogBoxeeLoggingIn object. Exit function and return LS_ERROR. [userProfileIndex=%d] (login)",iItem);
    return CBoxeeLoginStatusTypes::LS_ERROR;
  }
  
  creds.SetUserName(g_settings.m_vecProfiles[iItem].getID());
  creds.SetPassword(g_settings.m_vecProfiles[iItem].getLockCode());
  
  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::DoUserLogin - Credentials was set to [UserName=%s][Pass=%s] (login)",creds.GetUserName().c_str(),creds.GetPassword().c_str());

  pLoggingIn->Reset();
  pLoggingIn->SetProfileToLoad(iItem);
  pLoggingIn->Close();
  
  CBoxeeLoginManager::SetProxyCreds(creds);
  
  BOXEE::Boxee::GetInstance().SetCredentials(creds);

  XFILE::CFileCurl::SetCookieJar(BOXEE::BXCurl::GetCookieJar());

  BOXEE::Boxee::GetInstance().Login_bg(creds);
  
  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::DoUserLogin - Going to call CGUIDialogBoxeeLoggingIn::DoModal while trying to login (login)");

  pLoggingIn->DoModal();

  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::DoUserLogin - After call to CGUIDialogBoxeeLoggingIn::DoModal (login)");

  switch (pLoggingIn->IsLoginSuccessful())
  {
  case BOXEE::LOGIN_SUCCESS:
  {
    g_application.SetOfflineMode(false);
    result = CBoxeeLoginStatusTypes::LS_OK;
    CLog::Log(LOGDEBUG,"CBoxeeLoginManager::DoUserLogin - Login was successful. result was set to [%d=LOGIN_STATUS_OK] and SetOfflineMode was called with [FALSE] (login)",result);
  
    // check if profile directory exists or it has been removed
    if (!DIRECTORY::CDirectory::Exists("special://profile"))
    {
      CLog::Log(LOGWARNING,"***folder for <%s> does not exist!! ", g_settings.m_vecProfiles[iItem].getID().c_str());
      CreateProfile(g_settings.m_vecProfiles[iItem].getID(), pLoggingIn->GetUserObj());
      g_settings.CreateProfileFolders();
    }
  }
  break;
  case BOXEE::LOGIN_GENERAL_ERROR:
  {
    CGUIDialogOK::ShowAndGetInput(20068,51075,20022,20022);

    g_application.SetOfflineMode(false);
    result = CBoxeeLoginStatusTypes::LS_ERROR;

    CLog::Log(LOGERROR,"CBoxeeLoginManager::DoUserLogin - result was set to [%d=LS_ERROR] and SetOfflineMode was called with [FALSE] (login)",result);
  }
  break;
  case BOXEE::LOGIN_ERROR_ON_NETWORK:
  {
    CLog::Log(LOGERROR,"CBoxeeLoginManager::DoUserLogin - Login FAILED with [LOGIN_ERROR_ON_NETWORK] (login)");
    if (CGUIDialogYesNo::ShowAndGetInput(20068,51051,51052,51053))
    {
      g_application.SetOfflineMode(true);
      g_settings.LoadProfile(iItem);

      result = CBoxeeLoginStatusTypes::LS_NETWORK_ERROR;
      
      CLog::Log(LOGERROR,"CBoxeeLoginManager::DoUserLogin - result was set to [%d=LS_NETWORK_ERROR] and SetOfflineMode was called with [TRUE] (login)",result);
    }
    else
    {
      g_application.SetOfflineMode(false);
      result = CBoxeeLoginStatusTypes::LS_ERROR;
      
      CLog::Log(LOGERROR,"CBoxeeLoginManager::DoUserLogin - result was set to [%d=LS_STATUS_ERROR] and SetOfflineMode was called with [FALSE] (login)",result);
    }    
  }
  break;
  case BOXEE::LOGIN_ERROR_ON_CREDENTIALS:
  {
    CLog::Log(LOGERROR,"CBoxeeLoginManager::DoUserLogin - Login FAILED with [LOGIN_ERROR_ON_CREDENTIALS] (login)");
    CGUIDialogOK::ShowAndGetInput(20068,20117,20022,20022);
    g_application.SetOfflineMode(false);
    result = CBoxeeLoginStatusTypes::LS_CREDENTIALS_ERROR;
    CLog::Log(LOGERROR,"CBoxeeLoginManager::DoUserLogin - result was set to [%d=LS_CREDENTIALS_ERROR] (login)",result);    
  }
  break;
  default:
    break;
  }

  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::DoUserLogin - Exit function and return [result=%d] (login)",result);
  
  return result;
}

bool CBoxeeLoginManager::PostLoginActions()
{
  // Check if need to handle update
  HandleUpdateOnLogin();

  CGUIWindow* pWindow = g_windowManager.GetWindow(WINDOW_HOME);
  if (pWindow)
  {
    pWindow->ResetControlStates();
  }

  g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].setDate();
  g_settings.SaveProfiles(PROFILES_FILE);

  if (getenv("BX_COUNTRY_CODE"))
  {
    g_application.SetCountryCode(getenv("BX_COUNTRY_CODE"));
  }
  else
  {
    BOXEE::BXObject obj;
    BOXEE::Boxee::GetInstance().GetCurrentUser(obj);
    std::string cc = obj.GetValue("country");
    g_application.SetCountryCode(cc);
  }
  
  g_weatherManager.Refresh();
  
#ifdef HAS_PYTHON
  g_pythonParser.m_bLogin = true;
#endif

  return true;
}

void CBoxeeLoginManager::HandleUpdateOnLogin()
{
  ////////////////////////////////////////////////////////////////////
  // Need to Check if a force update exist only for OSX and WINDOWS //
  ////////////////////////////////////////////////////////////////////

  bool needToCheckForUpdateOnLogin = false;
  
#if defined(_LINUX) && defined(__APPLE__)

  ///////////////////
  // APPLE section //
  ///////////////////

  if(CSysInfo::IsAppleTV() == false)
  {
    needToCheckForUpdateOnLogin = true;
    
    CLog::Log(LOGDEBUG,"CBoxeeLoginManager::HandleUpdateOnLogin - Under OSX -> needToCheckForUpdateOnLogin was set to [%d] (update)",needToCheckForUpdateOnLogin);
  }

#elif defined(_WIN32)
  
  /////////////////////
  // WINDOWS section //
  /////////////////////

  needToCheckForUpdateOnLogin = true;
  
  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::HandleUpdateOnLogin - Under WINDOWS -> needToCheckForUpdateOnLogin was set to [%d] (update)",needToCheckForUpdateOnLogin);

#endif
  
  if(needToCheckForUpdateOnLogin)
  {
    //////////////////////////////////////
    // Check if there is a force update //
    //////////////////////////////////////
  
    CLog::Log(LOGDEBUG,"CBoxeeLoginManager::HandleUpdateOnLogin - Under OSX or WINDOWS -> Going to Check if a force update exist (update)");

    CBoxeeVersionUpdateJob versionUpdateJob = g_boxeeVersionUpdateManager.GetBoxeeVerUpdateJob();
    if(versionUpdateJob.acquireVersionUpdateJobForPerformUpdate())
    {
      VERSION_UPDATE_FORCE versionUpdateForce = (versionUpdateJob.GetVersionUpdateInfo()).GetVersionUpdateForce();
    
      if(versionUpdateForce == VUF_YES)
      {
        CLog::Log(LOGDEBUG,"CBoxeeLoginManager::HandleUpdateOnLogin - UpdateForce is VUF_YES, going to open Update dialog WITHOUT CANCEL button (update)");
      
        bool retVal = CBoxeeVersionUpdateManager::HandleUpdateVersionButton(true);
      
        CLog::Log(LOGDEBUG,"CBoxeeLoginManager::HandleUpdateOnLogin - Call to CBoxeeVersionUpdateManager::HandleUpdateVersionButton() returned [%d] (update)",retVal);             
      }
      else
      {
        CLog::Log(LOGDEBUG,"CBoxeeLoginManager::HandleUpdateOnLogin - UpdateForce is VUF_NO or there is no update. Continue... (update)");
      }             
    }
    else
    {
      CLog::Log(LOGDEBUG,"CBoxeeLoginManager::HandleUpdateOnLogin - Call to acquireVersionUpdateJobForPerformUpdate returned FALSE. Continue... (update)");
    }
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeLoginManager::HandleUpdateOnLogin - NOT Under OSX or WINDOWS -> NOT going to Check if force update exist (update)");
  }  
}

// helper
struct FindProfile
{
  FindProfile(const CStdString &id) : m_id(id) {}
  bool operator()( const CProfile &prof ) { return prof.getID() == m_id; }
  CStdString m_id;
};

int CBoxeeLoginManager::CreateProfile(const CStdString& username, const BOXEE::BXObject& userObj)
{
  // Create the directory
  CStdString strDirectory = "profiles";
  strDirectory += PATH_SEPARATOR_STRING;
  strDirectory += username;

  //////////////////////////////////////////
  // Create sources.xml and shortcuts.xml //
  //////////////////////////////////////////

  // Init sources.xml file path
  CStdString sourcesXml = "special://home/";
  sourcesXml += strDirectory;
  sourcesXml += "/sources.xml";
  sourcesXml = _P(sourcesXml);

  // Init shortcuts.xml file path
  CStdString shortcutsXml = "special://home/";
  shortcutsXml += strDirectory;
  shortcutsXml += "/shortcuts.xml";
  shortcutsXml = _P(shortcutsXml);

#ifdef __APPLE__
  if(CSysInfo::IsAppleTV())
  {
    CFile::Cache("special://xbmc/userdata/sources.xml.in.appletv", sourcesXml);
    CFile::Cache("special://xbmc/userdata/shortcuts.xml.in.appletv", shortcutsXml);
  }
  else
  {
    CFile::Cache("special://xbmc/userdata/sources.xml.in.osx", sourcesXml);
    CFile::Cache("special://xbmc/userdata/shortcuts.xml.in.osx", shortcutsXml);
  }
#elif _WIN32
  CFile::Cache("special://xbmc/userdata/sources.xml.in.win", sourcesXml);
  CFile::Cache("special://xbmc/userdata/shortcuts.xml.in.win", shortcutsXml);
#else
  CFile::Cache("special://xbmc/userdata/sources.xml.in.linux", sourcesXml);
  CFile::Cache("special://xbmc/userdata/shortcuts.xml.in.linux", shortcutsXml);
#endif

  // Create the profile
  CProfile p;
  p.setID(username);
  p.setName(userObj.GetValue(MSG_KEY_NAME));
  p.setDirectory(strDirectory);
  p.setThumb(userObj.GetValue(MSG_KEY_THUMB));
  p.setWriteDatabases(true);
  p.setWriteSources(true);
  p.setDatabases(true);
  p.setSources(true);
  p.setLockCode("-");
  p.setLastLockCode("-");
  p.setLockMode(LOCK_MODE_QWERTY);
  p.setMusicLocked(false);
  p.setVideoLocked(false);
  p.setSettingsLocked(false);
  p.setFilesLocked(false);
  p.setPicturesLocked(false);
  p.setProgramsLocked(false);
  
  if ( std::find_if( g_settings.m_vecProfiles.begin(), g_settings.m_vecProfiles.end(), FindProfile(username) ) == g_settings.m_vecProfiles.end() )
  {
    g_settings.m_vecProfiles.push_back(p);
    g_settings.SaveProfiles(PROFILES_FILE);
  }
  
  return g_settings.m_vecProfiles.size() - 1;
}

void CBoxeeLoginManager::UpdateProfile(int profileId, const CStdString& password, bool rememberPassword)
{
  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::UpdateProfile - Enter function with [profileId=%d][password=%s][rememberPassword=%d] (login)",profileId,password.c_str(),rememberPassword);

  CProfile& p = g_settings.m_vecProfiles[profileId];

  if (rememberPassword)
  {
    p.setLockCode(password);
  }
  else
  {
    p.setLockCode("-");
  }

  p.setLastLockCode(password);

  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::UpdateProfile - After set [LockCode=%s][LastLockCode=%s]. Going to save Profiles file (login)",p.getLockCode().c_str(),p.getLastLockCode().c_str());

  g_settings.SaveProfiles(PROFILES_FILE);
}

bool CBoxeeLoginManager::Logout()
{
  bool LogoutSucceeded = false;
  
  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::Logout - Enter function (logout)");

  g_application.getApplicationMessenger().SetSwallowMessages(true);
  
  g_application.StopPlaying();
  
  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::Logout - Going to call Boxee::Logout() (logout)");

  CAppManager::GetInstance().StopAllApps();

  BOXEE::Boxee::GetInstance().Logout();

  g_application.BoxeeUserLogoutAction();

  g_windowManager.CloseDialogs(true);

  //g_windowManager.DeInitialize();

  g_directoryCache.ClearAll();
  
  g_settings.m_iLastLoadedProfileIndex = 0;
  g_settings.SaveProfiles(PROFILES_FILE);

  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::Logout - Going to call Boxee::Start() (login)(logout)");

  bool bRc = BOXEE::Boxee::GetInstance().Start();

  g_settings.bUseLoginScreen = true;
  if (bRc)
  {
    CLog::Log(LOGINFO,"CBoxeeLoginManager::Logout - successfully initialized boxee library (login)(logout)");
  }
  else
  {
    CLog::Log(LOGWARNING,"CBoxeeLoginManager::Logout - FAILED to initialize boxee library (login)(logout)");
  }

  g_application.GetBoxeeLoginManager().SetInChangeUserProcess(true);

  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::Logout - Going to call Login() (login)(logout)");

  g_application.getApplicationMessenger().SetSwallowMessages(false);

  bool loginSucceeded = Login();
  
  if(!loginSucceeded)
  {
    CLog::Log(LOGERROR,"CBoxeeLoginManager::Logout - Login FAILED. Call to Login() returned [loginSucceeded=%d] (login)(logout)",loginSucceeded);
  }
  else
  {
    CLog::Log(LOGERROR,"CBoxeeLoginManager::Logout - Login SUCCEEDED. Call to Login() returned [loginSucceeded=%d] (login)(logout)",loginSucceeded);
    
    g_application.BoxeePostLoginInitializations();
    
    LogoutSucceeded = true;
  }
  
  return LogoutSucceeded;
}

void CBoxeeLoginManager::SetProxyCreds(BOXEE::BXCredentials &creds)
{
  if (g_guiSettings.GetBool("network.usehttpproxy")  && !g_guiSettings.GetString("network.httpproxyserver").empty())
  {
    std::string strProxy = "http://" + g_guiSettings.GetString("network.httpproxyserver") + ":" + g_guiSettings.GetString("network.httpproxyport");
    creds.SetProxy(strProxy);
  }
  else
  {
    creds.SetProxy("");  
  }
}

void CBoxeeLoginManager::SetInChangeUserProcess(bool inChangeUserProcess)
{
  m_inChangeUserProcess = inChangeUserProcess;
  
  CLog::Log(LOGDEBUG,"CBoxeeLoginManager::SetInChangeUserProcess - After setting [m_inChangeUserProcess=%d] (login)(logout)",m_inChangeUserProcess);
}

bool CBoxeeLoginManager::IsInChangeUserProcess()
{
  return m_inChangeUserProcess;
}

const char* CBoxeeLoginManager::CBoxeeLoginStatusEnumAsString(CBoxeeLoginStatusTypes::BoxeeLoginStatusEnums boxeeLoginStatusEnum)
{
  switch(boxeeLoginStatusEnum)
  {
  case CBoxeeLoginStatusTypes::LS_OK:
    return "LS_OK";
  case CBoxeeLoginStatusTypes::LS_ERROR:
    return "LS_ERROR";
  case CBoxeeLoginStatusTypes::LS_NETWORK_ERROR:
    return "LS_NETWORK_ERROR";
  case CBoxeeLoginStatusTypes::LS_CREDENTIALS_ERROR:
    return "LS_CREDENTIALS_ERROR";
  default:
    CLog::Log(LOGERROR,"Failed to convert enum [%d] to string BoxeeLoginStatusEnum. Return LS_NONE - (login)",boxeeLoginStatusEnum);
    return "LS_NONE";
  }
}

