#include "BoxeeOTAConfigurationManager.h"
#include "GUIDialogBoxeeOTALocationConfiguration.h"
#include "GUIDialogBoxeeOTAfavouriteShows.h"
#include "GUIDialogBoxeeOTAfavouriteShowsNotification.h"
#include "BoxeeOTAConfigurationData.h"
#include "GUIWindowManager.h"
#include "GUIDialogBoxeeWizardBase.h"
#include "utils/log.h"
#include "GUIDialogBoxeeOTAConfiguration.h"
#include "Settings.h"
#include "GUISettings.h"
#include "lib/libBoxee/bxcurrentlocation.h"
#include "Application.h"


CBoxeeOTAConfigurationManager::CBoxeeOTAConfigurationManager() : CBoxeeSimpleDialogWizardManager()
{
}

CBoxeeOTAConfigurationManager::~CBoxeeOTAConfigurationManager()
{
}

CBoxeeOTAConfigurationManager& CBoxeeOTAConfigurationManager::GetInstance()
{
  static CBoxeeOTAConfigurationManager boxeeOtaConfigurationManager;
  return boxeeOtaConfigurationManager;
}

bool CBoxeeOTAConfigurationManager::RunWizard(bool rescan)
{
  m_rescan = rescan;
  m_isConnectedToFacebook = g_application.GetBoxeeSocialUtilsManager().IsConnected(FACEBOOK_SERVICE_ID) &&
      !g_application.GetBoxeeSocialUtilsManager().RequiresReconnect(FACEBOOK_SERVICE_ID);

  if (Run(WINDOW_OTA_WELCOME_CONFIGURATION, true))
  {
    m_data.SaveData();
    g_guiSettings.SetBool("ota.run", true);
    g_settings.Save();
    return true;
  }

  return false;
}

CGUIDialogBoxeeWizardBase* CBoxeeOTAConfigurationManager::HandleNextAction(CGUIDialogBoxeeWizardBase* pDialog, bool& addCurrentDlgToStack)
{
  if (!pDialog)
  {
    CLog::Log(LOGERROR,"CBoxeeOTAConfigurationManager::HandleNextAction - Enter function with a NULL pointer (digwiz)");
    return NULL;
  }

  int id = pDialog->GetID();

  CLog::Log(LOGDEBUG,"CBoxeeOTAConfigurationManager::HandleNextAction - Enter function with [id=%d] (digwiz)",id);

  CGUIDialogBoxeeWizardBase* pNextDialog = NULL;

  switch(id)
  {
  case WINDOW_OTA_WELCOME_CONFIGURATION:
  {
    m_data.ClearData();
    m_data.DetectLocation();

    // If we know the country, ask the user to confirm, otherwise let the user select the location
    BOXEE::BXCurrentLocation& location = BOXEE::BXCurrentLocation::GetInstance();
    if (location.IsLocationDetected())
      pNextDialog = (CGUIDialogBoxeeOTAConfirmLocation*)g_windowManager.GetWindow(WINDOW_OTA_LOCATION_CONFIRMATION);
    else
      pNextDialog = (CGUIDialogBoxeeOTACountriesLocationConfiguration*)g_windowManager.GetWindow(WINDOW_OTA_COUNTRIES_CONFIGURATION);

    if (pNextDialog)
      addCurrentDlgToStack = true;
  }
  break;

  case WINDOW_OTA_LOCATION_CONFIRMATION:
  {
    CGUIDialogBoxeeOTAConfirmLocation* pLocationDialog = (CGUIDialogBoxeeOTAConfirmLocation*)g_windowManager.GetWindow(WINDOW_OTA_LOCATION_CONFIRMATION);
    if (!pLocationDialog)
      break;

    if (pLocationDialog->GetYesButtonPressed() && m_data.IsInNorthAmerica())
      pNextDialog = (CGUIDialogBoxeeOTAZipcodeLocationConfiguration*)g_windowManager.GetWindow(WINDOW_OTA_ZIPCODE_CONFIGURATION);
    else if (pLocationDialog->GetYesButtonPressed() && !m_data.IsInNorthAmerica() && !m_isConnectedToFacebook)
      pNextDialog = (CGUIDialogBoxeeWizardBase*)g_windowManager.GetWindow(WINDOW_OTA_FACEBOOK_CONNECT);
    else if (pLocationDialog->GetYesButtonPressed() && !m_data.IsInNorthAmerica() && m_isConnectedToFacebook)
      SetWizardComplete(true);
    else
      pNextDialog = (CGUIDialogBoxeeOTACountriesLocationConfiguration*)g_windowManager.GetWindow(WINDOW_OTA_COUNTRIES_CONFIGURATION);

    if (pNextDialog)
      addCurrentDlgToStack = true;
  }
  break;

  case WINDOW_OTA_COUNTRIES_CONFIGURATION:
  {
    if (m_data.IsInNorthAmerica())
      pNextDialog = (CGUIDialogBoxeeOTAZipcodeLocationConfiguration*)g_windowManager.GetWindow(WINDOW_OTA_ZIPCODE_CONFIGURATION);
    else if (!m_isConnectedToFacebook)
      pNextDialog = (CGUIDialogBoxeeWizardBase*)g_windowManager.GetWindow(WINDOW_OTA_FACEBOOK_CONNECT);
    else
      SetWizardComplete(true);

    if (pNextDialog)
      addCurrentDlgToStack = true;
  }
  break;

  case WINDOW_OTA_ZIPCODE_CONFIGURATION:
  {
    pNextDialog = (CGUIDialogBoxeeOTAConnectionConfiguration*)g_windowManager.GetWindow(WINDOW_OTA_CONNECTION_CONFIGURATION);
    if (pNextDialog)
    {
      addCurrentDlgToStack = true;
    }
  }
  break;

  case WINDOW_OTA_CONNECTION_CONFIGURATION:
  {
    if (!m_isConnectedToFacebook)
       pNextDialog = (CGUIDialogBoxeeWizardBase*)g_windowManager.GetWindow(WINDOW_OTA_FACEBOOK_CONNECT);
    else
       SetWizardComplete(true);

    if (pNextDialog)
    {
      addCurrentDlgToStack = true;
    }
  }
  break;

  case WINDOW_OTA_FACEBOOK_CONNECT:
  {
    SetWizardComplete(true);
  }
  break;

  default:
  {
    CLog::Log(LOGERROR,"CBoxeeOTAConfigurationManager::HandleNextAction - FAILED to handle WindowId [%d] (digwiz)",id);
  }
  break;
  }

  if (addCurrentDlgToStack)
  {
    AddToStack(id);
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeOTAConfigurationManager::HandleNextAction - Not adding [id=%d] to stack. [addCurrentDlgToStack=%d][DialogStackSize=%d] (digwiz)",id,addCurrentDlgToStack,GetStackSize());
  }

  CLog::Log(LOGDEBUG,"CBoxeeOTAConfigurationManager::HandleNextAction - Exit function and return [NextDialog=%p] for [id=%d] (digwiz)",pNextDialog,id);

  return pNextDialog;
}
