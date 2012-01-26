#include "GUIDialogBoxeeOTAFacebookConnect.h"
#include "GUIWindowManager.h"
#include "BoxeeOTAConfigurationManager.h"
#include "GUIDialogBoxeeOTALocationConfiguration.h"
#include "utils/log.h"
#include "Application.h"
#include "LocalizeStrings.h"

#define FACEBOOK_BUTTON1 1
#define FACEBOOK_BUTTON2 4
#define LEFT_SIDE_LABEL  3

///////////////////////////////////////////////////////////////////////////////////////////////////////

CGUIDialogBoxeeOTAFacebookConnect::CGUIDialogBoxeeOTAFacebookConnect()
  : CGUIDialogBoxeeWizardBase(WINDOW_OTA_FACEBOOK_CONNECT, "boxee_ota_facebook_connect.xml", "CGUIDialogBoxeeOTAFacebookConnect")
{
}

CGUIDialogBoxeeOTAFacebookConnect::~CGUIDialogBoxeeOTAFacebookConnect()
{
}

void CGUIDialogBoxeeOTAFacebookConnect::OnInitWindow()
{
  CGUIDialogBoxeeWizardBase::OnInitWindow();

  if (g_application.GetBoxeeSocialUtilsManager().IsConnected(FACEBOOK_SERVICE_ID, true) &&
      g_application.GetBoxeeSocialUtilsManager().RequiresReconnect(FACEBOOK_SERVICE_ID))
  {
    SET_CONTROL_LABEL(LEFT_SIDE_LABEL, g_localizeStrings.Get(58058));
    SET_CONTROL_HIDDEN(FACEBOOK_BUTTON1);
    SET_CONTROL_VISIBLE(FACEBOOK_BUTTON2);
    SET_CONTROL_FOCUS(FACEBOOK_BUTTON2, 0);
  }
}

void CGUIDialogBoxeeOTAFacebookConnect::OnDeinitWindow(int nextWindowID)
{
  CGUIWindow::OnDeinitWindow(nextWindowID);
}

bool CGUIDialogBoxeeOTAFacebookConnect::OnAction(const CAction& action)
{
  if (action.id == ACTION_SELECT_ITEM)
  {
    int iControl = GetFocusedControlID();

    if (iControl == FACEBOOK_BUTTON1 || iControl == FACEBOOK_BUTTON2)
    {
      g_application.GetBoxeeSocialUtilsUIManager().HandleUISocialUtilConnect(FACEBOOK_SERVICE_ID);
    }

    m_actionChoseEnum = CActionChoose::NEXT;
    Close();
    return true;
  }

  return CGUIDialogBoxeeWizardBase::OnAction(action);
}
