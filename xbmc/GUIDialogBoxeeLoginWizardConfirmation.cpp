#include "GUIDialogBoxeeLoginWizardConfirmation.h"
#include "utils/log.h"

CGUIDialogBoxeeLoginWizardConfirmation::CGUIDialogBoxeeLoginWizardConfirmation() : CGUIDialogBoxeeWizardBase(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_CONFIRMATION,"boxee_login_wizard_confirmation.xml","CGUIDialogBoxeeLoginWizardConfirmation")
{

}

CGUIDialogBoxeeLoginWizardConfirmation::~CGUIDialogBoxeeLoginWizardConfirmation()
{

}

void CGUIDialogBoxeeLoginWizardConfirmation::OnInitWindow()
{
  CGUIDialogBoxeeWizardBase::OnInitWindow();
}

bool CGUIDialogBoxeeLoginWizardConfirmation::OnAction(const CAction& action)
{
  if (action.id == ACTION_PREVIOUS_MENU)
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardConfirmation::OnAction - ACTION_PREVIOUS_MENU - can't go back from here (blw)(digwiz)");
    return true;
  }

  return CGUIDialogBoxeeWizardBase::OnAction(action);
}

bool CGUIDialogBoxeeLoginWizardConfirmation::OnMessage(CGUIMessage& message)
{
  return CGUIDialogBoxeeWizardBase::OnMessage(message);
}

bool CGUIDialogBoxeeLoginWizardConfirmation::HandleClickNext()
{
  return true;
}

bool CGUIDialogBoxeeLoginWizardConfirmation::HandleClickBack()
{
  return true;
}
