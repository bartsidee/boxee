/* CGUIDialogBoxeeOTAConfiguration .cpp
 *
 */
#include "GUIDialogBoxeeOTAConfiguration.h"
#include "GUIWindowManager.h"
#include "BoxeeOTAConfigurationManager.h"
#include "GUIDialogBoxeeOTALocationConfiguration.h"
#include "utils/log.h"
#include "LocalizeStrings.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////

CGUIDialogBoxeeOTAConnectionConfiguration::CGUIDialogBoxeeOTAConnectionConfiguration()
  : CGUIDialogBoxeeWizardBase(WINDOW_OTA_CONNECTION_CONFIGURATION,"boxee_ota_connection_configuration.xml","CGUIDialogBoxeeOTAConnectionConfiguration")
{
}

CGUIDialogBoxeeOTAConnectionConfiguration::~CGUIDialogBoxeeOTAConnectionConfiguration()
{
}

bool CGUIDialogBoxeeOTAConnectionConfiguration::OnAction(const CAction& action)
{
  if (action.id == ACTION_SELECT_ITEM)
  {
    int iControl = GetFocusedControlID();

    if(iControl == DIALOG_WIZARD_BUTTON_NEXT)
    {
      CBoxeeOTAConfigurationManager::GetInstance().GetConfigurationData().SetIsCable(false);
    }
    if(iControl == DIALOG_WIZARD_BUTTON_BACK)
    {
      CBoxeeOTAConfigurationManager::GetInstance().GetConfigurationData().SetIsCable(true);
    }
    m_actionChoseEnum = CActionChoose::NEXT;
    Close();
    return true;
  }
  return CGUIDialogBoxeeWizardBase::OnAction(action);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

#define CONTROL_LABEL_HEADING 1
#define CONTROL_LABEL_TEXT    2

CGUIDialogBoxeeOTAWelcome::CGUIDialogBoxeeOTAWelcome() : CGUIDialogBoxeeWizardBase(WINDOW_OTA_WELCOME_CONFIGURATION, "boxee_ota_welcome_configuration.xml", "CGUIDialogBoxeeOTAConfiguration")
{
  m_rescan = false;
}

CGUIDialogBoxeeOTAWelcome::~CGUIDialogBoxeeOTAWelcome()
{
}

void CGUIDialogBoxeeOTAWelcome::OnInitWindow()
{
  CGUIDialogBoxeeWizardBase::OnInitWindow();

  if (m_rescan)
  {
    SET_CONTROL_LABEL(CONTROL_LABEL_HEADING, g_localizeStrings.Get(58095));
    SET_CONTROL_LABEL(CONTROL_LABEL_TEXT, g_localizeStrings.Get(58096));
  }
}
