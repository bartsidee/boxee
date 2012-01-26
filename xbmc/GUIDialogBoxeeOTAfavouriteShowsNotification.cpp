/* GUIDialogBoxeeOTAfavouriteShowsNotification.cpp
 *
 */
#include "GUIDialogBoxeeOTAfavouriteShowsNotification.h"
#include "GUIWindowManager.h"
#include "BoxeeOTAConfigurationManager.h"
#include "GUIDialogBoxeeOTALocationConfiguration.h"
#include "utils/log.h"
#include "bxconfiguration.h"
#include "bxutils.h"
#include "RssSourceManager.h"

CGUIDialogBoxeeOTAfavouriteShowsNotification::CGUIDialogBoxeeOTAfavouriteShowsNotification() :
CGUIDialogBoxeeWizardBase(WINDOW_OTA_NOTIFICATION_CONFIG,"custom_boxee_livetv_setup_7.xml","CGUIDialogBoxeeOTAfavouriteShowsNotification")
{

}

CGUIDialogBoxeeOTAfavouriteShowsNotification::~CGUIDialogBoxeeOTAfavouriteShowsNotification()
{

}

void CGUIDialogBoxeeOTAfavouriteShowsNotification::OnInitWindow()
{

}



bool CGUIDialogBoxeeOTAfavouriteShowsNotification::OnAction(const CAction& action)
{

  return CGUIDialogBoxeeWizardBase::OnAction(action);
}
