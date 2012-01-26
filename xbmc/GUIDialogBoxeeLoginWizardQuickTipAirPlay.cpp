#include "GUIDialogBoxeeLoginWizardQuickTipAirPlay.h"
#include "log.h"

CGUIDialogBoxeeLoginWizardQuickTipAirPlay::CGUIDialogBoxeeLoginWizardQuickTipAirPlay() : CGUIDialogBoxeeWizardBase(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_QUICK_TIP_AIRPLAY, "boxee_login_wizard_quick_tip_airplay.xml", "CGUIDialogBoxeeLoginWizardQuickTipAirPlay")
{

}


CGUIDialogBoxeeLoginWizardQuickTipAirPlay::~CGUIDialogBoxeeLoginWizardQuickTipAirPlay()
{

}

bool CGUIDialogBoxeeLoginWizardQuickTipAirPlay::OnAction(const CAction& action)
{
  if (action.id == ACTION_PREVIOUS_MENU)
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardQuickTipAirPlay::OnAction - ACTION_PREVIOUS_MENU - can't go back from here (blw)(digwiz)");
    return true;
  }

  return CGUIDialogBoxeeWizardBase::OnAction(action);
}

bool CGUIDialogBoxeeLoginWizardQuickTipAirPlay::OnMessage(CGUIMessage& message)
{
  return CGUIDialogBoxeeWizardBase::OnMessage(message);
}

void CGUIDialogBoxeeLoginWizardQuickTipAirPlay::OnInitWindow()
{
  CGUIDialogBoxeeWizardBase::OnInitWindow();
}

bool CGUIDialogBoxeeLoginWizardQuickTipAirPlay::HandleClickNext()
{
  return true;
}

bool CGUIDialogBoxeeLoginWizardQuickTipAirPlay::HandleClickBack()
{
  return true;
}

