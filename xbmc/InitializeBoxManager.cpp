#include "InitializeBoxManager.h"

#ifdef HAS_EMBEDDED

#include "GUIWindowManager.h"
#include "Application.h"
#include "log.h"
#include "GUIDialogFirstTimeUseBase.h"
#include "GUIDialogFirstTimeUseEthernet.h"
#include "GUIDialogFirstTimeUseWireless.h"
#include "GUIDialogFirstTimeUseConfWirelessPassword.h"
#include "GUIDialogFirstTimeUseConfWirelessSecurity.h"
#include "GUIDialogFirstTimeUseConfWirelessSSID.h"
#include "GUIDialogFirstTimeUseConfNetwork.h"
#include "GUIDialogFirstTimeUseNetworkMessage.h"
#include "GUIDialogFirstTimeUseUpdateMessage.h"
#include "GUIDialogFirstTimeUseUpdateProgress.h"
#include "GUIDialogFirstTimeUseSimpleMessage.h"
#include "GUIDialogProgress.h"
#include "GUIDialogOK2.h"
#include "Resolution.h"
#include "LocalizeStrings.h"
#include "GUILabelControl.h"
#include "Util.h"
#include "GUIInfoManager.h"
#include "GUIWindowSettingsCategory.h"
#include "BoxeeUtils.h"
#include "utils/Weather.h"
#include "BoxeeVersionUpdateManager.h"

#define CHECK_INTERNET_WIRELESS_CONNECTION_IN_SEC   20
#define CHECK_INTERNET_WIRED_CONNECTION_IN_SEC   8
#define SUCCEEDED 0

CInitializeBoxManager::CInitializeBoxManager()
{
  m_isConnectViaEthernet = true;
  m_hal = &(CHalServicesFactory::GetInstance());
  m_loginViaCustomWirelessNetwork = false;
  m_initCompleted = false;
  m_updateCompleted = false;
  m_runFromSettings = false;
  m_updateVersionStatus = CUpdateVersionStatus::NO_UPDATE;
}

CInitializeBoxManager::~CInitializeBoxManager()
{

}

CInitializeBoxManager& CInitializeBoxManager::GetInstance()
{
  static CInitializeBoxManager initializeBoxManager;

  return initializeBoxManager;
}

bool CInitializeBoxManager::Run(bool runFromSettings)
{
  CLog::Log(LOGDEBUG,"CInitializeBoxManager::Initialize - Enter function. [runFromSettings=%d] (initbox)",runFromSettings);

  m_runFromSettings = runFromSettings;

  m_isConnectViaEthernet = true;
  m_loginViaCustomWirelessNetwork = false;
  m_initCompleted = false;
  m_updateCompleted = false;
  m_updateVersionStatus = CUpdateVersionStatus::NO_UPDATE;

  CGUIDialogFirstTimeUseBase* pDialog = NULL;

  if (m_runFromSettings)
  {
    pDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_WELCOME);
    pDialog->SetActionChose(CActionChose::NEXT);

    // if we entered from settings -> start with the action performed on exit WINDOW_DIALOG_FTU_WELCOME dialog
    pDialog = GetNextDialog(pDialog);
  }
  else
  {
    pDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_LANG);
  }

  if (!m_initCompleted && !pDialog)
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::Run - FAILED to get first FTU dialog (initbox)");
    return false;
  }

  // clear dialog stack
  while(!m_dialogStack.empty())
  {
    m_dialogStack.pop();
  }

  g_windowManager.ActivateWindow(WINDOW_FTU_BACKGROUND);

  while (!m_initCompleted)
  {
    if (!pDialog)
    {
      CLog::Log(LOGERROR,"CInitializeBoxManager::Run - init - FAILED to get next dialog -> Break (initbox)");
      break;
    }

    pDialog->DoModal();

    pDialog = GetNextDialog(pDialog);
  }

  if (m_initCompleted)
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::Run - Initializing box was completed. [InitCompleted=%d] (initbox)",m_initCompleted);

    if (m_runFromSettings)
    {
      bool retVal = PostFtuFromSettings();
      g_windowManager.PreviousWindow();
      return retVal;
    }
  }
  else
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::Run - FAILED to Initialize the box -> Finish. [InitCompleted=%d][runFromSettings=%d] (initbox)",m_initCompleted,m_runFromSettings);
    g_windowManager.PreviousWindow();
    return false;
  }

  PostFtuInitialization();

  if (m_initCompleted)
  {
    if (CheckForNewVersion())
    {
      m_updateVersionStatus = CUpdateVersionStatus::HAS_UPDATE;

      pDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_UPDATE_MESSAGE);

      while (!m_updateCompleted)
      {
        if (!pDialog)
        {
          CLog::Log(LOGERROR,"CInitializeBoxManager::Run - update - FAILED to get next dialog. Break (initbox)");
          break;
        }

        pDialog->DoModal();

        pDialog = GetNextDialog(pDialog);
      }

      if (m_updateCompleted)
      {
        m_updateVersionStatus = CUpdateVersionStatus::UPDATE_SUCCEEDED;
        CLog::Log(LOGDEBUG,"CInitializeBoxManager::Run - Downloading updating box was completed. Going to reboot box [m_updateCompleted=%d][InitCompleted=%d] (initbox)",m_updateCompleted,m_initCompleted);
      }
      else
      {
        m_updateVersionStatus = CUpdateVersionStatus::UPDATE_FAILED;
        CLog::Log(LOGERROR,"CInitializeBoxManager::Run - FAILED to update the box. [m_updateCompleted=%d][InitCompleted=%d] (initbox)",m_updateCompleted,m_initCompleted);
      }
    }
    else
    {
      CLog::Log(LOGDEBUG,"CInitializeBoxManager::Run - There is no new update -> continue. [m_updateCompleted=%d][InitCompleted=%d] (initbox)",m_updateCompleted,m_initCompleted);
    }
  }

  g_windowManager.PreviousWindow();

  return true;
}

CGUIDialogFirstTimeUseBase* CInitializeBoxManager::GetNextDialog(CGUIDialogFirstTimeUseBase* pDialog)
{
  if (!pDialog)
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::GetNextDialog - Enter function with a NULL pointer (initbox)");
    return NULL;
  }

  CActionChose::ActionChoseEnums actionChose = pDialog->GetActionChose();
  if (actionChose == CActionChose::ERROR)
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::GetNextDialog - Action chose enum is ERROR (initbox)");
    return NULL;
  }

  CGUIDialogFirstTimeUseBase* pNextWindow = NULL;

  switch(actionChose)
  {
  case CActionChose::NEXT:
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::GetNextDialog - For [windowId=%d] handling [ActionChose=%d=NEXT] (initbox)",pDialog->GetID(),actionChose);

    pNextWindow = HandleNextAction(pDialog);
  }
  break;
  case CActionChose::BACK:
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::GetNextDialog - For [windowId=%d] handling [ActionChose=%d=BACK] (initbox)",pDialog->GetID(),actionChose);

    pNextWindow = HandleBackAction(pDialog);
  }
  break;
  default:
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::GetNextDialog - FAILED to handle ActionChoseEnum [%d] (initbox)",actionChose);
  }
  break;
  }

  return pNextWindow;
}

CGUIDialogFirstTimeUseBase* CInitializeBoxManager::HandleNextAction(CGUIDialogFirstTimeUseBase* pDialog)
{
  if (!pDialog)
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextAction - Enter function with a NULL pointer (initbox)");
    return NULL;
  }

  int id = pDialog->GetID();

  CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextAction - Enter function with [id=%d] (initbox)",id);

  CGUIDialogFirstTimeUseBase* pNextDialog = NULL;
  bool addToStack = false;

  switch(id)
  {
  case WINDOW_DIALOG_FTU_LANG:
  {
    pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_WELCOME);

    if (pNextDialog)
    {
      CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextAction - For [id=%d=WINDOW_DIALOG_FTU_LANG] got next window [id=%d=WINDOW_DIALOG_FTU_WELCOME] (initbox)",id,WINDOW_DIALOG_FTU_WELCOME);
      addToStack = true;
    }
  }
  break;
  case WINDOW_DIALOG_FTU_WELCOME:
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextAction - For [id=%d=WINDOW_DIALOG_FTU_WELCOME] going to call HandleNextWellcomeDialog() (initbox)",id);
    addToStack = HandleNextWellcomeDialog(&pNextDialog,ADDR_DYNAMIC);
  }
  break;
  case WINDOW_DIALOG_FTU_NETWORK_MESSAGE:
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextAction - For [id=%d=WINDOW_DIALOG_FTU_NETWORK_MESSAGE] going to call HandleNextNetworkMessageDialog() (initbox)",id);
    HandleNextNetworkMessageDialog(&pNextDialog);
  }
  break;
  case WINDOW_DIALOG_FTU_WIRELESS:
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextAction - For [id=%d=WINDOW_DIALOG_FTU_AUDIO] going to call HandleNextAudioDialog() (initbox)",id);
    addToStack = HandleNextWirelessDialog(&pNextDialog);
  }
  break;
  case WINDOW_DIALOG_FTU_CONF_WIRELESS_PASSWORD:
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextAction - For [id=%d=WINDOW_DIALOG_FTU_CONF_WIRELESS_PASSWORD] going to call HandleNextConfWirelessPasswordDialog() (initbox)",id);
    HandleNextConfWirelessPasswordDialog(&pNextDialog);
  }
  break;
  case WINDOW_DIALOG_FTU_CONF_WIRELESS_SSID:
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextAction - For [id=%d=WINDOW_DIALOG_FTU_CONF_WIRELESS_SSID] going to call HandleNextConfWirelessSsidDialog() (initbox)",id);
    HandleNextConfWirelessSsidDialog(&pNextDialog);
  }
  break;
  case WINDOW_DIALOG_FTU_CONF_WIRELESS_SECURITY:
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextAction - For [id=%d=WINDOW_DIALOG_FTU_CONF_WIRELESS_SECURITY] going to call HandleNextConfWirelessSecurityDialog() (initbox)",id);
    HandleNextConfWirelessSecurityDialog(&pNextDialog);
  }
  break;
  case WINDOW_DIALOG_FTU_CONF_NETWORK:
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextAction - For [id=%d=WINDOW_DIALOG_FTU_CONF_NETWORK] going to call HandleNextConfNetworkDialog() (initbox)",id);
    HandleNextConfNetworkDialog(&pNextDialog);
  }
  break;
  case WINDOW_DIALOG_FTU_UPDATE_MESSAGE:
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextAction - For [id=%d=WINDOW_DIALOG_FTU_UPDATE_MESSAGE] going to call HandleNextUpdateMessageDialog() (initbox)",id);
    HandleNextUpdateMessageDialog(&pNextDialog);
  }
  break;
  case WINDOW_DIALOG_FTU_UPDATE_PROGRESS:
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextAction - For [id=%d=WINDOW_DIALOG_FTU_UPDATE_PROGRESS] going to call HandleNextUpdateProgressDialog() (initbox)",id);
    HandleNextUpdateProgressDialog(&pNextDialog);
  }
  break;
  case WINDOW_DIALOG_FTU_SIMPLE_MESSAGE:
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextAction - For [id=%d=WINDOW_DIALOG_FTU_SIMPLE_MESSAGE] going to call HandleNextSimpleMessageDialog() (initbox)",id);
    HandleNextSimpleMessageDialog(&pNextDialog);
  }
  default:
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextAction - FAILED to handle WindowId [%d] (initbox)",id);
  }
  break;
  }

  if (addToStack && (m_dialogStack.empty() || m_dialogStack.top() != id))
  {
    m_dialogStack.push(id);
  }
  else
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextAction - Not adding [id=%d] to stack. [addToStack=%d][DialogStackSize=%d] (initbox)",id,addToStack,(int)m_dialogStack.size());
  }

  CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextAction - Exit function and return [NextDialog=%p] for [id=%d] (initbox)",pNextDialog,id);

  return pNextDialog;
}

bool CInitializeBoxManager::HandleNextWellcomeDialog(CGUIDialogFirstTimeUseBase** pNextDialog, CHalAddrType addrType, bool onRetry)
{
  m_isConnectViaEthernet = true;

  if (!IsEthernetConnected())
  {
    if (onRetry)
    {
      CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextWellcomeDialog - Not connect via Ethernet. Show error message. [onRetry=%d]  (initbox)",onRetry);
      *pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_NETWORK_MESSAGE);

      if (*pNextDialog)
      {
        ((CGUIDialogFirstTimeUseNetworkMessage*)(*pNextDialog))->SetMessage(g_localizeStrings.Get(54637));
      }
      else
      {
        CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextWellcomeDialog - 1 - FAILED to get WINDOW_DIALOG_FTU_NETWORK_MESSAGE in order to set the correct message. [onRetry=%d] (initbox)",onRetry);
      }
    }
    else
    {
      m_isConnectViaEthernet = false;

      CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextWellcomeDialog - Not connect via Ethernet. Set next window [WINDOW_DIALOG_FTU_WIRELESS=%d]. [onRetry=%d] (initbox)",WINDOW_DIALOG_FTU_WIRELESS,onRetry);
      *pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_WIRELESS);
    }

    return true;
  }

  if (!ConnectToEthernet(0,addrType/*ADDR_DYNAMIC*/))
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextWellcomeDialog - Has Ethernet connection but FAILED to connect. Set next window [WINDOW_DIALOG_FTU_NETWORK_MESSAGE=%d] (initbox)",WINDOW_DIALOG_FTU_NETWORK_MESSAGE);
    *pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_NETWORK_MESSAGE);

    if (*pNextDialog)
    {
      ((CGUIDialogFirstTimeUseNetworkMessage*)(*pNextDialog))->SetMessage(g_localizeStrings.Get(54638));
    }
    else
    {
      CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextWellcomeDialog - 2 - FAILED to get WINDOW_DIALOG_FTU_NETWORK_MESSAGE in order to set the correct message (initbox)");
    }

    return true;
  }

  // check internet for wired connection
  if (HasInternetConnection(WIRED_INTERFACE_ID, CHECK_INTERNET_WIRED_CONNECTION_IN_SEC) != CONNECTED_TO_INTERNET)
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextLangDialog - Test internet connection FAILED. Set next window [WINDOW_DIALOG_FTU_NETWORK_MESSAGE=%d] (initbox)",WINDOW_DIALOG_FTU_NETWORK_MESSAGE);
    *pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_NETWORK_MESSAGE);

    if (*pNextDialog)
    {
      ((CGUIDialogFirstTimeUseNetworkMessage*)(*pNextDialog))->SetMessage(g_localizeStrings.Get(54696));
    }
    else
    {
      CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextWellcomeDialog - 3 - FAILED to get WINDOW_DIALOG_FTU_NETWORK_MESSAGE in order to set the correct message (initbox)");
    }

    return true;
  }

  // Internet is OK -> done with FTU
  CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextLangDialog - SUCCEEDED to connect via Ethernet. [runFromSettings=%d] (initbox)",m_runFromSettings);

  m_initCompleted = true;
  CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextLangDialog - After Set [InitCompleted=%d] (initbox)",m_initCompleted);

  return true;
}

bool CInitializeBoxManager::HandleNextNetworkMessageDialog(CGUIDialogFirstTimeUseBase** pNextDialog)
{
  CGUIDialogFirstTimeUseNetworkMessage* pDialog = (CGUIDialogFirstTimeUseNetworkMessage*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_NETWORK_MESSAGE);
  if (!pDialog)
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextNetworkMessageDialog - FAILED to get WINDOW_DIALOG_FTU_NETWORK_MESSAGE (initbox)");
    return false;
  }

  int buttonClicked = pDialog->GetButtonClicked();

  switch (buttonClicked)
  {
  case TRY_AGAIN_BUTTON_CONTROL:
  {
    if (m_isConnectViaEthernet)
    {
      return HandleNextWellcomeDialog(pNextDialog,ADDR_DYNAMIC,true);
    }
    else
    {
      *pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_WIRELESS);
      return true;

      //return HandleNextWirelessDialog(pNextDialog);
    }
  }
  break;
  case SWITCH_CONNECTION_TYPE_BUTTON_CONTROL:
  {
    int switchTo = pDialog->GetSwitchToType();

    if (switchTo == SWITCH_TO_ETHETNET_BUTTON)
    {
      m_isConnectViaEthernet = true;
      return HandleNextWellcomeDialog(pNextDialog,ADDR_DYNAMIC,true);
    }
    else if (switchTo == SWITCH_TO_WIRELESS_BUTTON)
    {
      m_isConnectViaEthernet = false;
      CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextNetworkMessageDialog - [buttonId=%d=SWITCH_TO_WIRELESS_CONTROL] -> Set next window [WINDOW_DIALOG_FTU_WIRELESS=%d] (initbox)",buttonClicked,WINDOW_DIALOG_FTU_WIRELESS);
      *pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_WIRELESS);
      return true;
    }
    else
    {
      CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextNetworkMessageDialog - FAILED to handle button with switch type of [%d] (initbox)",switchTo);
      return false;
    }
  }
  break;
  case ADJUST_NETWORK_SETTINGS_BUTTON_CONTROL:
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextNetworkMessageDialog - [buttonId=%d=ADJUST_NETWORK_SETTINGS_BUTTON_CONTROL] -> Set next window [WINDOW_DIALOG_FTU_CONF_NETWORK=%d] (initbox)",buttonClicked,WINDOW_DIALOG_FTU_CONF_NETWORK);
    *pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_CONF_NETWORK);
    return true;
  }
  break;
  default:
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextNetworkMessageDialog - FAILED to handle [buttonId=%d] (initbox)",buttonClicked);
    return false;
  }
  break;
  }

  return false;
}

bool CInitializeBoxManager::HandleNextWirelessDialog(CGUIDialogFirstTimeUseBase** pNextDialog)
{
  CGUIDialogFirstTimeUseWireless* pDialog = (CGUIDialogFirstTimeUseWireless*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_WIRELESS);
  if (!pDialog)
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextWirelessDialog - FAILED to get WINDOW_DIALOG_FTU_WIRELESS (initbox)");
    return false;
  }

  m_isConnectViaEthernet = false;

  if (!pDialog)
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextWirelessDialog - Enter function with [pDialog=NULL] (initbox)");
    return false;
  }

  if (pDialog->HasSelectedItem())
  {
    // item was selected from the list

    m_loginViaCustomWirelessNetwork = false;
    CFileItemPtr wlItem = pDialog->GetSelectedItem();
    if (wlItem->GetPropertyBOOL("wln-secure"))
    {
      *pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_CONF_WIRELESS_PASSWORD);

      if (*pNextDialog)
      {
        ((CGUIDialogFirstTimeUseConfWirelessPassword*)(*pNextDialog))->SetNetworkName(wlItem->GetProperty("wln-ssid"),true);
      }

      CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextWirelessDialog - For [id=%d=WINDOW_DIALOG_FTU_WIRELESS] got next window [id=%d=WINDOW_DIALOG_FTU_CONF_WIRELESS_PASSWORD] (initbox)",WINDOW_DIALOG_FTU_WIRELESS,WINDOW_DIALOG_FTU_CONF_WIRELESS_PASSWORD);
      return true;
    }
    else
    {
      CStdString errorMessage = "";
      if (!ConnectToWireless(0,ADDR_DYNAMIC,"",errorMessage))
      {
        // error log will be written from ConnectToWireless

        *pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_NETWORK_MESSAGE);
        if (*pNextDialog)
        {
          ((CGUIDialogFirstTimeUseNetworkMessage*)(*pNextDialog))->SetMessage(errorMessage);
        }
        else
        {
          CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextWirelessDialog - FAILED to get WINDOW_DIALOG_FTU_NETWORK_MESSAGE in order to set the correct message (initbox)");
        }

        return false;
      }

      // Internet is OK -> done with FTU
      m_initCompleted = true;
      return true;
    }
  }
  else
  {
    switch(pDialog->GetChoiceSelected())
    {
    case JOIN_OTHER_NETWORKS_BUTTON:
    {
      m_loginViaCustomWirelessNetwork = true;

      // reset old data in CGUIDialogFirstTimeUseConfWirelessSSID and CGUIDialogFirstTimeUseConfWirelessPassword
      CGUIDialogFirstTimeUseConfWirelessSSID* pSsidWireless = (CGUIDialogFirstTimeUseConfWirelessSSID*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_CONF_WIRELESS_SSID);
      if (pSsidWireless)
      {
        pSsidWireless->SetSSID("");
      }

      CGUIDialogFirstTimeUseConfWirelessPassword* pPasswordDialog = (CGUIDialogFirstTimeUseConfWirelessPassword*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_CONF_WIRELESS_PASSWORD);
      if (pPasswordDialog)
      {
        pPasswordDialog->SetPassword("");
      }

      *pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_CONF_WIRELESS_SSID);
      CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextWirelessDialog - For [id=%d=WINDOW_DIALOG_FTU_WIRELESS] got next window [id=%d=WINDOW_DIALOG_FTU_CONF_WIRELESS_SSID] (initbox)",WINDOW_DIALOG_FTU_WIRELESS,WINDOW_DIALOG_FTU_CONF_WIRELESS_SSID);
      return true;
    }
    break;
    case SWITCH_TO_ETHERNET_BUTTON:
    {
      m_isConnectViaEthernet = true;
      return HandleNextWellcomeDialog(pNextDialog,ADDR_DYNAMIC,true);
    }
    break;
    }

    return true;
  }
}

bool CInitializeBoxManager::HandleNextConfWirelessPasswordDialog(CGUIDialogFirstTimeUseBase** pNextDialog)
{
  CGUIDialogFirstTimeUseConfWirelessPassword* pConfWirelessDialog = (CGUIDialogFirstTimeUseConfWirelessPassword*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_CONF_WIRELESS_PASSWORD);
  if (!pConfWirelessDialog)
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextConfWirelessPasswordDialog - FAILED to get WINDOW_DIALOG_FTU_CONF_WIRELESS_PASSWORD (initbox)");
    return false;
  }

  // try to connect via wireless
  CStdString errorMessage = "";
  if (!ConnectToWireless(0,ADDR_DYNAMIC,pConfWirelessDialog->GetPassword(),errorMessage))
  {
    // error log will be written from ConnectToWireless

    *pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_NETWORK_MESSAGE);
    if (*pNextDialog)
    {
      ((CGUIDialogFirstTimeUseNetworkMessage*)(*pNextDialog))->SetMessage(errorMessage);
    }
    else
    {
      CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextConfWirelessPasswordDialog - FAILED to get WINDOW_DIALOG_FTU_NETWORK_MESSAGE in order to set the correct message (initbox)");
    }

    return false;
  }

  // Internet is OK -> done with FTU
  m_initCompleted = true;
  return true;
}

bool CInitializeBoxManager::HandleNextConfWirelessSsidDialog(CGUIDialogFirstTimeUseBase** pNextDialog)
{
  *pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_CONF_WIRELESS_SECURITY);
  if (!(*pNextDialog))
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextConfWirelessSsidDialog - FAILED to get WINDOW_DIALOG_FTU_CONF_WIRELESS_SECURITY (initbox)");
  }

  return true;
}

bool CInitializeBoxManager::HandleNextConfWirelessSecurityDialog(CGUIDialogFirstTimeUseBase** pNextDialog)
{
  CGUIDialogFirstTimeUseConfWirelessSecurity* pDialog = (CGUIDialogFirstTimeUseConfWirelessSecurity*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_CONF_WIRELESS_SECURITY);
  if (!pDialog)
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextConfWirelessSecurityDialog - FAILED to get WINDOW_DIALOG_FTU_CONF_WIRELESS_SECURITY (initbox)");
    return false;
  }

  if (pDialog->GetAuth() != AUTH_NONE)
  {
    *pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_CONF_WIRELESS_PASSWORD);
    if (!(*pNextDialog))
    {
      CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextConfWirelessSecurityDialog - FAILED to get WINDOW_DIALOG_FTU_CONF_WIRELESS_PASSWORD (initbox)");
    }
    else
    {
      CGUIDialogFirstTimeUseConfWirelessSSID* pSsidWireless = (CGUIDialogFirstTimeUseConfWirelessSSID*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_CONF_WIRELESS_SSID);
      if (pSsidWireless)
      {
        ((CGUIDialogFirstTimeUseConfWirelessPassword*)(*pNextDialog))->SetNetworkName(pSsidWireless->GetSSID(), true);
      }
    }

    return true;
  }
  else
  {
    CStdString errorMessage = "";
    if (!ConnectToWireless(0,ADDR_DYNAMIC,"",errorMessage))
    {
      // error log will be written from ConnectToWireless

      *pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_NETWORK_MESSAGE);
      if (*pNextDialog)
      {
        ((CGUIDialogFirstTimeUseNetworkMessage*)(*pNextDialog))->SetMessage(errorMessage);
      }
      else
      {
        CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextWirelessDialog - FAILED to get WINDOW_DIALOG_FTU_NETWORK_MESSAGE in order to set the correct message (initbox)");
      }

      return false;
    }

    // Internet is OK -> done with FTU
    m_initCompleted = true;
    return true;
  }
}

bool CInitializeBoxManager::HandleNextConfNetworkDialog(CGUIDialogFirstTimeUseBase** pNextDialog)
{
  if (m_isConnectViaEthernet)
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextConfNetworkDialog - Try to connect via Ethernet (initbox)");
    return HandleNextWellcomeDialog(pNextDialog,ADDR_STATIC,true);
  }
  else
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextConfNetworkDialog - Try to connect via Wireless (initbox)");

    CStdString errorMessage = "";
    if (!ConnectToWireless(0,ADDR_STATIC,"",errorMessage))
    {
      *pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_NETWORK_MESSAGE);
      if (*pNextDialog)
      {
        ((CGUIDialogFirstTimeUseNetworkMessage*)(*pNextDialog))->SetMessage(errorMessage);
      }
      else
      {
        CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextConfNetworkDialog - FAILED to get WINDOW_DIALOG_FTU_NETWORK_MESSAGE in order to set the correct message (initbox)");
      }

      return false;
    }
  }

  // connect -> done with FTU
  CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextConfNetworkDialog - SUCCEEDED to connect. Set [m_initCompleted=%d] (initbox)",m_initCompleted);
  m_initCompleted = true;
  return true;
}

bool CInitializeBoxManager::HandleNextUpdateMessageDialog(CGUIDialogFirstTimeUseBase** pNextDialog)
{
  CGUIDialogFirstTimeUseUpdateMessage* pDialog = (CGUIDialogFirstTimeUseUpdateMessage*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_UPDATE_MESSAGE);
  if (!pDialog)
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextUpdateMessageDialog - FAILED to get WINDOW_DIALOG_FTU_UPDATE_MESSAGE (initbox)");
    return false;
  }

  switch(m_updateVersionStatus)
  {
  case CUpdateVersionStatus::NO_UPDATE:
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextUpdateMessageDialog - Update status is [%d=NO_UPDATE] -> Not handling (initbox)",(int)m_updateVersionStatus);
  }
  break;
  case CUpdateVersionStatus::HAS_UPDATE:
  {
#ifdef HAS_EMBEDDED
    g_boxeeVersionUpdateManager.StartUpdate();
#endif
    *pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_UPDATE_PROGRESS);
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextUpdateMessageDialog - Update status is [%d=HAS_UPDATE]. got next window [id=%d=WINDOW_DIALOG_FTU_UPDATE_PROGRESS] (initbox)",(int)m_updateVersionStatus,WINDOW_DIALOG_FTU_UPDATE_PROGRESS);
  }
  break;
  case CUpdateVersionStatus::UPDATING:
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextUpdateMessageDialog - Update status is [%d=UPDATING] -> Not handling (initbox)",(int)m_updateVersionStatus);
  }
  break;
  case CUpdateVersionStatus::UPDATE_SUCCEEDED:
  {
    *pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_UPDATE_MESSAGE);
    m_updateVersionStatus = CUpdateVersionStatus::REQUEST_UPGRADE;
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextUpdateMessageDialog - Update status is [%d=UPDATE_SUCCEEDED]. Set it to [REQUEST_UPGRADE]. [m_updateCompleted=%d] (initbox)",(int)m_updateVersionStatus,m_updateCompleted);
  }
  break;
  case CUpdateVersionStatus::UPDATE_FAILED:
  {
    *pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_UPDATE_MESSAGE);
    m_updateVersionStatus = CUpdateVersionStatus::HAS_UPDATE;
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextUpdateMessageDialog - Update status is [%d=UPDATE_FAILED]. Set it to [HAS_UPDATE]. [m_updateCompleted=%d] (initbox)",(int)m_updateVersionStatus,m_updateCompleted);
  }
  break;
  default:
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextUpdateMessageDialog - UNKNOWN Update status [%d] -> Not handling (initbox)",(int)m_updateVersionStatus);
  }
  break;
  }

  return true;
}

bool CInitializeBoxManager::HandleNextUpdateProgressDialog(CGUIDialogFirstTimeUseBase** pNextDialog)
{
  CGUIDialogFirstTimeUseUpdateProgress* pDialog = (CGUIDialogFirstTimeUseUpdateProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_UPDATE_PROGRESS);
  if (!pDialog)
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextUpdateProgressDialog - FAILED to get WINDOW_DIALOG_FTU_UPDATE_PROGRESS (initbox)");
    return false;
  }

  VERSION_UPDATE_DOWNLOAD_STATUS downloadStatus = pDialog->GetDownloadStatus();
  switch(downloadStatus)
  {
  case VUDS_FINISHED:
  {
    m_updateVersionStatus = CUpdateVersionStatus::UPDATE_SUCCEEDED;

    *pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_UPDATE_MESSAGE);
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextUpdateProgressDialog - Update finished successfully. Set status to [%d=UPDATE_SUCCEEDED]. got next window [id=%d=WINDOW_DIALOG_FTU_UPDATE_MESSAGE] (initbox)",(int)m_updateVersionStatus,WINDOW_DIALOG_FTU_UPDATE_MESSAGE);
  }
  break;
  case VUDS_FAILED:
  {
    m_updateVersionStatus = CUpdateVersionStatus::UPDATE_FAILED;

    *pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_UPDATE_MESSAGE);
    CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextUpdateProgressDialog - Update FAILED. Set status to [%d=UPDATE_FAILED]. got next window [id=%d=WINDOW_DIALOG_FTU_UPDATE_MESSAGE] (initbox)",(int)m_updateVersionStatus,WINDOW_DIALOG_FTU_UPDATE_MESSAGE);
  }
  break;
  default:
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::HandleNextUpdateProgressDialog - UNKNOWN download status [%d] -> Not handling (initbox)",(int)downloadStatus);
  }
  break;
  }

  return true;
}

bool CInitializeBoxManager::HandleNextSimpleMessageDialog(CGUIDialogFirstTimeUseBase** pNextDialog)
{
  m_initCompleted = true;
  CLog::Log(LOGDEBUG,"CInitializeBoxManager::HandleNextSimpleMessageDialog - Enter function. After set [InitCompleted=%d]. [runFromSettings=%d] (initbox)",m_initCompleted,m_runFromSettings);
  return true;
}

CGUIDialogFirstTimeUseBase* CInitializeBoxManager::HandleBackAction(CGUIDialogFirstTimeUseBase* pDialog)
{
  if (!pDialog)
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::HandleBackAction - Enter function with a NULL pointer (initbox)");
    return NULL;
  }

  CGUIDialogFirstTimeUseBase* pNextDialog = NULL;

  if ((int)m_dialogStack.size() < 1)
  {
    if (g_stSettings.m_doneFTU)
    {
      CLog::Log(LOGWARNING,"CInitializeBoxManager::HandleBackAction - Size of dialogStack is [%d] (initbox)",(int)m_dialogStack.size());
      return NULL;
    }
    else
    {
      pNextDialog = pDialog;
    }
  }
  else
  {
    bool foundBackWindow = false;
    int nextDialogId;

    while (!foundBackWindow)
    {
      nextDialogId = m_dialogStack.top();
      m_dialogStack.pop();

      if (nextDialogId != pDialog->GetID() || ((int)m_dialogStack.size()) < 1)
      {
        // not the same id OR no more id's in the stack -> break
        foundBackWindow = true;
      }
    }

    pNextDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(nextDialogId);

    if (!pNextDialog)
    {
      CLog::Log(LOGERROR,"CInitializeBoxManager::HandleBackAction - FAILED to get dialog for [id=%d] (initbox)",nextDialogId);
    }
  }

  return pNextDialog;
}

bool CInitializeBoxManager::IsEthernetConnected()
{
  CHalEthernetInfo heInfo;

  if (!m_hal->GetEthernetInfo(0,heInfo))
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::IsEthernetConnected - FAILED to get EthernetInfo. Return FALSE for HasEthernet (initbox)");
    return false;
  }

  bool hasEthernet = (heInfo.link_up && heInfo.running);

  if (hasEthernet)
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::IsEthernetConnected - [hasEthernet=%d] -> Return TRUE (initbox)",hasEthernet);
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::IsEthernetConnected - [hasEthernet=%d] -> Return FALSE (initbox)",hasEthernet);
    return false;
  }
}

bool CInitializeBoxManager::ConnectToEthernet(unsigned int instance, CHalAddrType addrType)
{
  CGUIDialogProgress* pDialogProgress = NULL;
  StartDialogProgress(&pDialogProgress);

  CHalEthernetConfig heConfig;

  switch(addrType)
  {
  case ADDR_DYNAMIC:
  {
    heConfig.addr_type = addrType;
  }
  break;
  case ADDR_STATIC:
  {
    CGUIDialogFirstTimeUseConfNetwork* pConfNetworkDialog = (CGUIDialogFirstTimeUseConfNetwork*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_CONF_NETWORK);

    if (!pConfNetworkDialog)
    {
      CLog::Log(LOGERROR,"CInitializeBoxManager::ConnectToEthernet - FAILED to get CGUIDialogFirstTimeUseConfNetwork object (initbox)");
      StopDialogProgress(pDialogProgress);
      return false;
    }

    heConfig.addr_type = addrType;
    heConfig.dns = pConfNetworkDialog->GetDNS();
    heConfig.gateway = pConfNetworkDialog->GetGateway();
    heConfig.ip_address = pConfNetworkDialog->GetIpAddress();
    heConfig.netmask = pConfNetworkDialog->GetNetmask();
  }
  break;
  default:
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::ConnectToEthernet - UNKNOWN [addrType=%d] to handle (initbox)",addrType);
    StopDialogProgress(pDialogProgress);
    return false;
  }
  break;
  }

  bool retVal;

  if (!m_hal->SetEthernetConfig(instance,heConfig))
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::ConnectToEthernet - FAILED to connect via Ethernet. [instance=%d][AddrType=%d] (initbox)",instance,heConfig.addr_type);
    retVal = false;
  }
  else
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::ConnectToEthernet - SUCCEEDED to connect via Ethernet. [instance=%d][AddrType=%d] (initbox)",instance,heConfig.addr_type);

    // succeeded to connect via Ethernet -> check Internet connection
    if (HasInternetConnection(WIRED_INTERFACE_ID, CHECK_INTERNET_WIRED_CONNECTION_IN_SEC) != CONNECTED_TO_INTERNET)
    {
      CLog::Log(LOGDEBUG,"CInitializeBoxManager::ConnectToEthernet - Has Ethernet connection but check Internet connection FAILED (initbox)");
      retVal = false;
    }
    else
    {
      CLog::Log(LOGDEBUG,"CInitializeBoxManager::ConnectToEthernet - Has Ethernet connection and check Internet connection SUCCEEDED (initbox)");
      g_application.NetworkConfigurationChanged();
      retVal = true;
    }
  }

  StopDialogProgress(pDialogProgress);

  return retVal;
}

bool CInitializeBoxManager::ConnectToWireless(unsigned int instance, CHalAddrType addrType, const CStdString& password, CStdString& errorMessage)
{
  CFileItem wlItem;
  if (!GetWirelessNetworkToConnectItem(wlItem,password))
  {
    errorMessage = g_localizeStrings.Get(54639);
    CLog::Log(LOGERROR,"CInitializeBoxManager::ConnectToWireless - FAILED to get selected wireless network item (initbox)");
    return false;
  }

  CGUIDialogProgress* pDialogProgress = NULL;
  StartDialogProgress(&pDialogProgress);

  CHalWirelessConfig hwConfig;

  switch(addrType)
  {
  case ADDR_DYNAMIC:
  {
    hwConfig.addr_type = addrType;
    hwConfig.ssid = wlItem.GetProperty("wln-ssid");
    hwConfig.password = wlItem.GetProperty("wln-password");
    hwConfig.authType = (CHalWirelessAuthType)wlItem.GetPropertyInt("wln-auth");
  }
  break;
  case ADDR_STATIC:
  {

    CGUIDialogFirstTimeUseConfNetwork* pConfNetworkDialog = (CGUIDialogFirstTimeUseConfNetwork*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_CONF_NETWORK);

    if (!pConfNetworkDialog)
    {
      CLog::Log(LOGERROR,"CInitializeBoxManager::ConnectToWireless - FAILED to get CGUIDialogFirstTimeUseConfNetwork object (initbox)");
      StopDialogProgress(pDialogProgress);
      return false;
    }

    hwConfig.addr_type = addrType;
    hwConfig.ssid = wlItem.GetProperty("wln-ssid");
    hwConfig.password = wlItem.GetProperty("wln-password");
    hwConfig.dns = pConfNetworkDialog->GetDNS();
    hwConfig.gateway = pConfNetworkDialog->GetGateway();
    hwConfig.ip_address = pConfNetworkDialog->GetIpAddress();
    hwConfig.netmask = pConfNetworkDialog->GetNetmask();
  }
  break;
  default:
  {
    errorMessage = g_localizeStrings.Get(54639);
    CLog::Log(LOGERROR,"CInitializeBoxManager::ConnectToWireless - UNKNOWN [addrType=%d] to handle (initbox)",addrType);
    StopDialogProgress(pDialogProgress);
    return false;
  }
  break;
  }

  bool retVal;

  if (!m_hal->SetWirelessConfig(instance,hwConfig))
  {
    errorMessage = g_localizeStrings.Get(54639);
    CLog::Log(LOGERROR,"CInitializeBoxManager::ConnectToWireless - FAILED to connect via Wireless. [instance=%d][ssid=%s][AddrType=%d] (initbox)",instance,hwConfig.ssid.c_str(),hwConfig.addr_type);
    retVal = false;
  }
  else
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::ConnectToWireless - SUCCEEDED to connect via Wireless [instance=%d][ssid=%s][AddrType=%d] (initbox)",instance,hwConfig.ssid.c_str(),hwConfig.addr_type);

    NetworkConnectionStatus netStatus = HasInternetConnection(WIRELESS_INTERFACE_ID, CHECK_INTERNET_WIRELESS_CONNECTION_IN_SEC);
    // succeeded to connect via Wireless -> check Internet connection
    if (netStatus == NETWORK_DOWN)
    {
      errorMessage = g_localizeStrings.Get(57100);
      CLog::Log(LOGERROR,"CInitializeBoxManager::ConnectToWireless - Has Wireless connection [%s] but check Internet connection FAILED (initbox)",hwConfig.ssid.c_str());
      retVal = false;
    }
    else if (netStatus == NO_INTERNET_CONNECTION)
    {
      errorMessage = g_localizeStrings.Get(54697);
      CLog::Log(LOGERROR,"CInitializeBoxManager::ConnectToWireless - Has Wireless connection [%s] but check Internet connection FAILED (initbox)",hwConfig.ssid.c_str());
      retVal = false;
    }
    else
    {
      CLog::Log(LOGDEBUG,"CInitializeBoxManager::ConnectToWireless - Has Wireless connection [%s] and check Internet connection SUCCEEDED (initbox)",hwConfig.ssid.c_str());
      retVal = true;

      g_application.NetworkConfigurationChanged();
    }
  }

  StopDialogProgress(pDialogProgress);

  return retVal;
}

bool CInitializeBoxManager::GetWirelessNetworkToConnectItem(CFileItem& wlItem, const CStdString& password)
{
  if (!m_loginViaCustomWirelessNetwork)
  {
    CGUIDialogFirstTimeUseWireless* pWirelessDialog = (CGUIDialogFirstTimeUseWireless*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_WIRELESS);
    if (!pWirelessDialog)
    {
      CLog::Log(LOGERROR,"CInitializeBoxManager::GetWirelessNetworkToConnectItem - FAILED to get CGUIDialogFirstTimeUseWireless object (initbox)");
      return false;
    }

    wlItem.SetProperty("wln-ssid",pWirelessDialog->GetSelectedItem()->GetProperty("wln-ssid"));
    wlItem.SetProperty("wln-password","");
    wlItem.SetProperty("wln-auth",AUTH_DONTCARE);
  }
  else
  {
    CGUIDialogFirstTimeUseConfWirelessSSID* pSsidWireless = (CGUIDialogFirstTimeUseConfWirelessSSID*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_CONF_WIRELESS_SSID);
    CGUIDialogFirstTimeUseConfWirelessSecurity* pSecurityDialog = (CGUIDialogFirstTimeUseConfWirelessSecurity*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_CONF_WIRELESS_SECURITY);

    if (!pSsidWireless || !pSecurityDialog)
    {
      CLog::Log(LOGERROR,"CInitializeBoxManager::GetWirelessNetworkToConnectItem - FAILED to get object (initbox)");
      return false;
    }

    wlItem.SetProperty("wln-ssid",pSsidWireless->GetSSID());
    wlItem.SetProperty("wln-password","");
    wlItem.SetProperty("wln-auth",(int)pSecurityDialog->GetAuth());
  }

  if (!password.IsEmpty())
  {
    wlItem.SetProperty("wln-password",password);
  }

  return true;
}

bool CInitializeBoxManager::StartDialogProgress(CGUIDialogProgress** pDialogProgress)
{
  *pDialogProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

  if (*pDialogProgress)
  {
    (*pDialogProgress)->StartModal();
    (*pDialogProgress)->Progress();
    return true;
  }

  return false;
}

bool CInitializeBoxManager::StopDialogProgress(CGUIDialogProgress* pDialogProgress)
{
  if (pDialogProgress)
  {
    pDialogProgress->Close();
    return true;
  }

  return false;
}

CInitializeBoxManager::NetworkConnectionStatus CInitializeBoxManager::HasInternetConnection(int connectionType, int numOfIntervals)
{
  class CheckInternetConnectionJob : public IRunnable
  {
  public:
    CheckInternetConnectionJob(int connectionType, int numOfIntervals)
    {
      m_connectionType = connectionType;
      m_numOfIntervals = numOfIntervals;
      m_hasConnectToInternet = false;
      m_isNetworkUp = false;
    }

    bool IsNetworkUp()
    {
      return CWaitNetworkUpBG::IsNetworkUp(m_connectionType);
    }

    virtual void Run()
    {
      int interval = 0;
      while (interval < m_numOfIntervals)
      {
        sleep(1);

        interval++;
        CLog::Log(LOGDEBUG, "before IsNetworkUp()\n");
        if ((m_isNetworkUp = IsNetworkUp()))
        {
          break;
        }
        CLog::Log(LOGDEBUG, "after IsNetworkUp()\n");
      }
      CLog::Log(LOGDEBUG, "isNetworkUp = %d\n", m_isNetworkUp);

      if (m_isNetworkUp) {
        CLog::Log(LOGDEBUG, "before g_application.IsConnectedToInternet(true)\n");
        m_hasConnectToInternet = g_application.IsConnectedToInternet(true);
        CLog::Log(LOGDEBUG, "after g_application.IsConnectedToInternet(true). m_hasConnectToInternet  = %d\n", m_hasConnectToInternet);
      }

    }

    int m_connectionType;
    int m_numOfIntervals;
    bool m_hasConnectToInternet;
    bool m_isNetworkUp;
  };

  CheckInternetConnectionJob* cicJob = new CheckInternetConnectionJob(connectionType,numOfIntervals);
  if (CUtil::RunInBG(cicJob,false) == JOB_SUCCEEDED)
  {
    CInitializeBoxManager::NetworkConnectionStatus status;

    if (!cicJob->m_isNetworkUp)
    {
      status = NETWORK_DOWN;
    }
    else if (!cicJob->m_hasConnectToInternet)
    {
      status = NO_INTERNET_CONNECTION;
    }
    else
    {
      status = CONNECTED_TO_INTERNET;
    }

    delete cicJob;
    return status;
  }
  else
  {
    return NETWORK_DOWN;
  }
}

bool CInitializeBoxManager::IsConnectViaEthernet()
{
  return m_isConnectViaEthernet;
}

void CInitializeBoxManager::Reboot()
{
  m_hal->Reboot();
}

void CInitializeBoxManager::RequestUpgrade()
{
  m_hal->RequestUpgrade();
}

CUpdateVersionStatus::UpdateVersionStatusEnums CInitializeBoxManager::GetUpdateStatus()
{
  return m_updateVersionStatus;
}

bool CInitializeBoxManager::PostFtuFromSettings()
{
  // in case FTU was run from settings -> show summary

  CGUIDialogFirstTimeUseBase* pDialog = NULL;
  pDialog = (CGUIDialogFirstTimeUseBase*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_SIMPLE_MESSAGE);

  if (pDialog)
  {
    if (m_isConnectViaEthernet)
    {
      ((CGUIDialogFirstTimeUseSimpleMessage*)pDialog)->SetMessage(g_localizeStrings.Get(54635));
    }
    else
    {
      ((CGUIDialogFirstTimeUseSimpleMessage*)pDialog)->SetMessage(g_localizeStrings.Get(54636));
    }

    pDialog->DoModal();
  }
  else
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::Run - FAILED to get WINDOW_DIALOG_FTU_SIMPLE_MESSAGE in order to set the correct message (initbox)");
  }

  CLog::Log(LOGDEBUG,"CInitializeBoxManager::Run - Enter from settings -> Finish. [runFromSettings=%d] (initbox)",m_runFromSettings);
  return true;
}

bool CInitializeBoxManager::PostFtuInitialization()
{
  bool retVal = true;

  CLog::Log(LOGDEBUG,"CInitializeBoxManager::PostFtuInitialization - going to get data from server (ftu)");

  if (!GetDataFromServer())
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::PostFtuInitialization - Call to GetDataFromServer return FALSE (ftu)");
    retVal = false;
  }

  return retVal;
}

bool CInitializeBoxManager::CheckForNewVersion()
{
  /////////////////////////////////////
  // check if there is a new version //
  /////////////////////////////////////

  bool hasNewUpdate = false;
  CStdString newVersionBuildNumStr = "";

#ifdef HAS_EMBEDDED
  if (g_boxeeVersionUpdateManager.CheckForUpdate(hasNewUpdate,newVersionBuildNumStr) == SUCCEEDED)
  {
    if (hasNewUpdate)
    {
      CLog::Log(LOGDEBUG,"CInitializeBoxManager::CheckForNewVersion - There is a new update (ftu)");
    }
    else
    {
      CLog::Log(LOGDEBUG,"CInitializeBoxManager::CheckForNewVersion - There is no new update -> continue (ftu)");
    }
  }
  else
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::CheckForNewVersion - Call to CheckForUpdate FAILED. [hasNewUpdate=%d][newVersionBuildNum=%s]\n",hasNewUpdate,newVersionBuildNumStr.c_str());
  }
#endif

  return hasNewUpdate;
}

bool CInitializeBoxManager::GetDataFromServer()
{
  if (g_stSettings.m_doneFTU)
  {
    CLog::Log(LOGDEBUG,"CInitializeBoxManager::GetDataFromServer - FTU already made at first login. No need to get data from server (ftu)");
    return true;
  }

  CReadDataFromServer* rdfsJob = new CReadDataFromServer();
  bool result = (CUtil::RunInBG(rdfsJob) == JOB_SUCCEEDED);

  CLog::Log(LOGDEBUG,"CInitializeBoxManager::GetDataFromServer - going to return [%d] (ftu)",rdfsJob->m_bJobResult);

  return result;
}

void CInitializeBoxManager::CReadDataFromServer::Run()
{
  m_bJobResult = false;

  CStdString url = "http://app.boxee.tv/api/regiondefaults";

  CLog::Log(LOGDEBUG,"CInitializeBoxManager::GetDataFromServer - before getting data from server. [url=%s] (ftu)",url.c_str());

  BOXEE::BXCurl curl;
  CStdString strResp = curl.HttpGetString(url, false);

  CLog::Log(LOGDEBUG,"CInitializeBoxManager::GetDataFromServer - after getting data from server. [url=%s][respLen=%d] (ftu)",url.c_str(),(int)strResp.length());

  BOXEE::BXXMLDocument reader;
  if (strResp.empty())
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::GetDataFromServer - Not handling server response to [url=%s] because it is empty (ftu)",url.c_str());
    return;
  }

  if(!reader.LoadFromString(strResp))
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::GetDataFromServer - Not handling server response to [url=%s] because failed to load it to BXXMLDocument (ftu)",url.c_str());
    return;
  }

  TiXmlElement* root = reader.GetRoot();

  if(!root)
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::GetDataFromServer - Failed to get root from BXXMLDocument of the ping response (ftu)");
    return;
  }

  if((strcmp(root->Value(),"boxee") != 0))
  {
    CLog::Log(LOGERROR,"CInitializeBoxManager::GetDataFromServer - Failed to parse ping response because the root tag ISN'T <boxee> (ftu)");
    return;
  }

  TiXmlElement* pingChildElem = NULL;
  pingChildElem = root->FirstChildElement();

  CStdString countryCode = "";
  CStdString region = "";
  CStdString timezone = "";
  CStdString tempUnit = "";
  CStdString timeFormat = "";

  CGUIWindowSettingsCategory* pSettingsWindow = (CGUIWindowSettingsCategory*)g_windowManager.GetWindow(WINDOW_SETTINGS_MYPICTURES);

  while (pingChildElem)
  {
    if (strcmp(pingChildElem->Value(),"country_code") == 0)
    {
      if (pingChildElem && pingChildElem->FirstChild())
      {
        countryCode = pingChildElem->FirstChild()->Value();
      }
      else
      {
        CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::CheckTimezoneSettings - FAILED to read <country_code> element (tz)");
      }
    }
//    else if (strcmp(pingChildElem->Value(),"region") == 0)
//    {
//      if (pingChildElem && pingChildElem->FirstChild())
//      {
//        region = pingChildElem->FirstChild()->Value();
//      }
//      else
//      {
//        CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::CheckTimezoneSettings - FAILED to read <region> element (tz)");
//      }
//    }
    else if (strcmp(pingChildElem->Value(),"time_zone") == 0)
    {
      if (pingChildElem && pingChildElem->FirstChild())
      {
        timezone = pingChildElem->FirstChild()->Value();

        if (!CHalServicesFactory::GetInstance().SetTimezone(timezone))
        {
          CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::CheckTimezoneSettings - FAILED to set Timezone to [timezonePath=%s] (tz)",timezone.c_str());
        }
      }
      else
      {
        CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::CheckTimezoneSettings - FAILED to read <Timezone> element (tz)");
      }
    }
    else if (strcmp(pingChildElem->Value(),"temperature_symbol") == 0)
    {
      if (pingChildElem && pingChildElem->FirstChild())
      {
        tempUnit = pingChildElem->FirstChild()->Value();

        if (pSettingsWindow)
        {
          pSettingsWindow->SetTempUnit(tempUnit);
        }
        else
        {
          CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::CheckTimezoneSettings - FAILED to set TemperatureSymbol because SettingsWindow is NULL (tz)");
        }
      }
      else
      {
        CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::CheckTimezoneSettings - FAILED to read <temperature_symbol> element (tz)");
      }
    }
    else if (strcmp(pingChildElem->Value(),"clock_time") == 0)
    {
      if (pingChildElem && pingChildElem->FirstChild())
      {
        timeFormat = pingChildElem->FirstChild()->Value();

        if (pSettingsWindow)
        {
          pSettingsWindow->SetTimeFormat(timeFormat);
        }
        else
        {
          CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::CheckTimezoneSettings - FAILED to set ClockTime because SettingsWindow is NULL (tz)");
        }
      }
      else
      {
        CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::CheckTimezoneSettings - FAILED to read <clock_time> element (tz)");
      }
    }

    pingChildElem = pingChildElem->NextSiblingElement();
  }

  CLog::Log(LOGDEBUG,"CInitializeBoxManager::GetDataFromServer - after parse server data. [countryCode=%s][region=%s][timezone=%s][tempUnit=%s][timeFormat=%s] (ftu)",countryCode.c_str(),region.c_str(),timezone.c_str(),tempUnit.c_str(),timeFormat.c_str());

#ifdef HAS_EMBEDDED
  ///////////////////////////////////
  // set timezone country and city //
  ///////////////////////////////////

  CStdString countryName = "";
  CStdString cityName = "";

  if (pSettingsWindow)
  {
    countryName = pSettingsWindow->GetCountryByCode(countryCode);

    if (!countryName.IsEmpty())
    {
      g_guiSettings.SetString("timezone.country",countryName);
    }
    else
    {
      CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::CheckTimezoneSettings - FAILED to set timezone.country because CountryName is EMPTY. [countryName=%s][countryCode=%s] (tz)",countryName.c_str(),countryCode.c_str());
    }

    int pos = timezone.Find("/");
    if (pos != -1)
    {
      cityName = timezone.substr(pos+1);

      if (!cityName.IsEmpty() && pSettingsWindow->IsTimezoneCityExist(cityName))
      {
        g_guiSettings.SetString("timezone.city",cityName);
      }
      else
      {
        CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::CheckTimezoneSettings - FAILED to set timezone.city because CityName is EMPTY or NOT-VALID. [cityName=%s] (tz)",cityName.c_str());
      }
    }
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::CheckTimezoneSettings - FAILED to set timezone country and city because SettingsWindow is NULL (tz)");
  }

  //////////////////////
  // set weather city //
  //////////////////////

  if (!countryCode.IsEmpty() && !cityName.IsEmpty())
  {
    CLog::Log(LOGDEBUG,"CGUIWindowSettingsCategory::CheckTimezoneSettings - going to set weather location for [cityName=%s][countryCode=%s] (tz)(wl)",cityName.c_str(),countryCode.c_str());

    if (!BoxeeUtils::SetWeatherLocation(cityName,countryCode))
    {
      CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::CheckTimezoneSettings - FAILED to set weather location for [cityName=%s][countryCode=%s] (tz)(wl)",cityName.c_str(),countryCode.c_str());
    }
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::CheckTimezoneSettings - FAILED to set weather city because of an EMPTY data. [cityName=%s][countryCode=%s] (tz)(wl)",cityName.c_str(),countryCode.c_str());
  }
#endif

  g_settings.Save();

  m_bJobResult = true;

  return;
}

#endif

