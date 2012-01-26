#include "GUIDialogBoxeeLoginWizardQuickTip.h"
#include "GUIWindowManager.h"
#include "BoxeeUtils.h"
#include "log.h"

CGUIDialogBoxeeLoginWizardQuickTip::CGUIDialogBoxeeLoginWizardQuickTip() : CGUIDialogBoxeeWizardBase(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_QUICK_TIP, "boxee_login_wizard_quick_tip.xml", "CGUIDialogBoxeeLoginWizardQuickTip")
{

}


CGUIDialogBoxeeLoginWizardQuickTip::~CGUIDialogBoxeeLoginWizardQuickTip()
{

}

bool CGUIDialogBoxeeLoginWizardQuickTip::OnAction(const CAction& action)
{
  if (action.id == ACTION_PREVIOUS_MENU)
  {
    m_actionChoseEnum = CActionChoose::NEXT;
    Close();
    return true;
  }

  return CGUIDialogBoxeeWizardBase::OnAction(action);
}

bool CGUIDialogBoxeeLoginWizardQuickTip::OnMessage(CGUIMessage& message)
{
  return CGUIDialogBoxeeWizardBase::OnMessage(message);
}

void CGUIDialogBoxeeLoginWizardQuickTip::OnInitWindow()
{
  CGUIDialogBoxeeWizardBase::OnInitWindow();
}

bool CGUIDialogBoxeeLoginWizardQuickTip::HandleClickNext()
{
  return true;
}

bool CGUIDialogBoxeeLoginWizardQuickTip::HandleClickBack()
{
  return true;
}
