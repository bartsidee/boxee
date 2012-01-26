#include "GUIDialogBoxeeLoginWizardChooseUserToAddType.h"
#include "GUIWindowManager.h"
#include "utils/log.h"

#define NEW_USER_BUTTON_CONTROL_ID         8901
#define EXISTING_USER_BUTTON_CONTROL_ID    8902

CGUIDialogBoxeeLoginWizardChooseUserToAddType::CGUIDialogBoxeeLoginWizardChooseUserToAddType() : CGUIDialogBoxeeWizardBase(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_CHOOSE_USER_TO_ADD,"boxee_login_wizard_choose_user_to_add_type.xml","CGUIDialogBoxeeLoginWizardChooseUserToAddType")
{
  m_addingNewUser = false;
}

CGUIDialogBoxeeLoginWizardChooseUserToAddType::~CGUIDialogBoxeeLoginWizardChooseUserToAddType()
{

}

void CGUIDialogBoxeeLoginWizardChooseUserToAddType::OnInitWindow()
{
  CGUIDialogBoxeeWizardBase::OnInitWindow();

  m_addingNewUser = false;
  SET_CONTROL_FOCUS(NEW_USER_BUTTON_CONTROL_ID,0);
}

bool CGUIDialogBoxeeLoginWizardChooseUserToAddType::OnAction(const CAction& action)
{
  return CGUIDialogBoxeeWizardBase::OnAction(action);
}

bool CGUIDialogBoxeeLoginWizardChooseUserToAddType::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();
    int iAction = message.GetParam1();

    m_addingNewUser = (iControl == NEW_USER_BUTTON_CONTROL_ID);
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardChooseUserToAddType::OnMessage - GUI_MSG_CLICKED - after set [addingNewUser=%d]. [Control=%d][iAction=%d] (blw)(digwiz)",m_addingNewUser,iControl,iAction);

    m_actionChoseEnum = CActionChoose::NEXT;
    Close();
    return true;
  }
  break;
  }

  return CGUIDialogBoxeeWizardBase::OnMessage(message);
}

bool CGUIDialogBoxeeLoginWizardChooseUserToAddType::HandleClickNext()
{
  return true;
}

bool CGUIDialogBoxeeLoginWizardChooseUserToAddType::HandleClickBack()
{
  return true;
}

bool CGUIDialogBoxeeLoginWizardChooseUserToAddType::IsAddingNewUser()
{
  return m_addingNewUser;
}
