/* CGUIDialogBoxeeOTAConfiguration .cpp
 *
 */
#include "GUIDialogBoxeeOTALocationConfiguration.h"
#include "GUIWindowManager.h"
#include "BoxeeOTAConfigurationManager.h"
#include "utils/log.h"
#include "FileSystem/FactoryDirectory.h"
#include "FileSystem/Directory.h"
#include "GUISettings.h"
#include "bxconfiguration.h"
#include "bxutils.h"
#include "json/value.h"
#include "json/reader.h"
#include "GUIEditControl.h"
#include "GUIDialogOK2.h"
#include "LocalizeStrings.h"
#include "lib/libBoxee/bxcurrentlocation.h"

#define CONTROL_LOCATION_LIST   80
#define CONTROL_ZIP_CODE_EDIT   15001
#define CONTROL_LOCATION_LABEL  15000
#define CONTROL_BUTTON_NO       3 // YES is button NEXT

CGUIDialogBoxeeOTAConfirmLocation::CGUIDialogBoxeeOTAConfirmLocation() :
   CGUIDialogBoxeeWizardBase(WINDOW_OTA_LOCATION_CONFIRMATION, "boxee_ota_location_confirmation.xml", "CGUIDialogBoxeeOTALocationConfirmation")
{
}

void CGUIDialogBoxeeOTAConfirmLocation::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  BOXEE::BXCurrentLocation& location = BOXEE::BXCurrentLocation::GetInstance();

  CStdString locationStr;
  if (location.IsLocationDetected())
  {
    locationStr = location.GetCountry();
    locationStr += "?";
  }

  SET_CONTROL_LABEL(CONTROL_LOCATION_LABEL , locationStr);
}

CGUIDialogBoxeeOTAConfirmLocation::~CGUIDialogBoxeeOTAConfirmLocation()
{
}

bool CGUIDialogBoxeeOTAConfirmLocation::GetYesButtonPressed()
{
  return m_bYesButtonPressed;
}

bool CGUIDialogBoxeeOTAConfirmLocation::HandleClickNext()
{
  m_bYesButtonPressed = true;
  return true;
}

bool CGUIDialogBoxeeOTAConfirmLocation::HandleClickBack()
{
  return true;
}

bool CGUIDialogBoxeeOTAConfirmLocation::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED && message.GetSenderId() == CONTROL_BUTTON_NO)
  {
    m_bYesButtonPressed = false;
    m_actionChoseEnum = CActionChoose::NEXT;
    Close();
    return true;
  }

  return CGUIDialogBoxeeWizardBase::OnMessage(message);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CGUIDialogBoxeeOTACountriesLocationConfiguration::CGUIDialogBoxeeOTACountriesLocationConfiguration() :
    CGUIDialogBoxeeWizardBase(WINDOW_OTA_COUNTRIES_CONFIGURATION, "boxee_ota_countries_configuration.xml", "CGUIDialogBoxeeOTACountriesLocationConfiguration")
{
}

CGUIDialogBoxeeOTACountriesLocationConfiguration::~CGUIDialogBoxeeOTACountriesLocationConfiguration()
{
}

void CGUIDialogBoxeeOTACountriesLocationConfiguration::OnInitWindow()
{
  if (m_countries.IsEmpty())
  {
    GetCountriesFromServer();
  }

  CGUIDialog::OnInitWindow();

  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LOCATION_LIST);
  OnMessage(msg);

  CGUIMessage msg2(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LOCATION_LIST, 0, 0, &m_countries);
  OnMessage(msg2);
}

bool CGUIDialogBoxeeOTACountriesLocationConfiguration::GetCountriesFromServer()
{

  CStdString strUrl = BOXEE::BXConfiguration::GetInstance().GetURLParam("Boxee.ApiGetCountries", "http://app.boxee.tv/location/countries?service=livetv");
  Json::Value jResponse;
  int returnCode;

  if(BOXEE::BXUtils::PerformJSONGetRequestInBG(strUrl,jResponse,returnCode) != JOB_SUCCEEDED)
  {
    return false;
  }

  if (jResponse.isArray())
  {
    if (jResponse.size() == 0)
    {
      return false;
    }

    for (size_t j = 0; j < jResponse.size(); j++)
    {
      Json::Value m = jResponse[(int) j];

      CFileItemPtr jsonItem(new CFileItem("country"));
      jsonItem->SetLabel(m["title"].asString());
      jsonItem->SetProperty("country_code", m["code"].asString());
      m_countries.Add(jsonItem);
    }
  }

  return true;
}

bool CGUIDialogBoxeeOTACountriesLocationConfiguration::OnAction(const CAction& action)
{
  if (action.id == ACTION_SELECT_ITEM)
  {
    if (HandleSelectCountry())
    {
      m_actionChoseEnum = CActionChoose::NEXT;
      Close();
      return true;
    }
    else
    {
      m_actionChoseEnum = CActionChoose::BACK;
      Close();
      return false;
    }
  }

  return CGUIDialogBoxeeWizardBase::OnAction(action);
}

bool CGUIDialogBoxeeOTACountriesLocationConfiguration::HandleSelectCountry()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LOCATION_LIST);
  OnMessage(msg);
  int iCountry = msg.GetParam1();

  if (iCountry < m_countries.Size() && iCountry > -1)
    CBoxeeOTAConfigurationManager::GetInstance().GetConfigurationData().SetCountryCode(m_countries[iCountry]->GetProperty("country_code"));
  else
    return false;

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CGUIDialogBoxeeOTAZipcodeLocationConfiguration::CGUIDialogBoxeeOTAZipcodeLocationConfiguration() :
    CGUIDialogBoxeeWizardBase(WINDOW_OTA_ZIPCODE_CONFIGURATION, "boxee_ota_zipcode_configuration.xml", "CGUIDialogBoxeeOTAZipcodeLocationConfiguration")
{

}

CGUIDialogBoxeeOTAZipcodeLocationConfiguration::~CGUIDialogBoxeeOTAZipcodeLocationConfiguration()
{
}

void CGUIDialogBoxeeOTAZipcodeLocationConfiguration::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  CGUIEditControl *editControl = (CGUIEditControl *)GetControl(CONTROL_ZIP_CODE_EDIT);
  if (editControl)
    editControl->SetLabel2(CBoxeeOTAConfigurationManager::GetInstance().GetConfigurationData().GetZipCode());

  SET_CONTROL_FOCUS(CONTROL_ZIP_CODE_EDIT, 0);
}

bool CGUIDialogBoxeeOTAZipcodeLocationConfiguration::OnAction(const CAction& action)
{
  if (action.id == ACTION_SELECT_ITEM)
  {
    if (!HandleSelectZipCode())
    {
      HandleInvalidZipCode();
      return true;
    }

    m_actionChoseEnum = CActionChoose::NEXT;
    Close();
    return true;
  }

  return CGUIDialogBoxeeWizardBase::OnAction(action);
}

bool CGUIDialogBoxeeOTAZipcodeLocationConfiguration::HandleSelectZipCode()
{
  CGUIEditControl *editControl = (CGUIEditControl *)GetControl(CONTROL_ZIP_CODE_EDIT);
  if (!editControl)
  {
    return false;
  }

  CStdString zipCode = editControl->GetLabel2();
  CBoxeeOTAConfigurationManager::GetInstance().GetConfigurationData().SetZipCode(zipCode);

  return true;
}

void CGUIDialogBoxeeOTAZipcodeLocationConfiguration::HandleInvalidZipCode()
{
  CGUIDialogOK2 *dialog = (CGUIDialogOK2 *)g_windowManager.GetWindow(WINDOW_DIALOG_OK_2);
  if(dialog)
  {
    dialog->SetHeading(g_localizeStrings.Get(257));
    dialog->SetLine(0,g_localizeStrings.Get(58031));
    dialog->DoModal();
  }

  SET_CONTROL_FOCUS(CONTROL_ZIP_CODE_EDIT, 0);
}
