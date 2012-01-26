#pragma once

#include "GUIDialogBoxeeWizardBase.h"

class CGUIDialogBoxeeLoginWizardAddExistingUser : public CGUIDialogBoxeeWizardBase
{
public:

  CGUIDialogBoxeeLoginWizardAddExistingUser();
  virtual ~CGUIDialogBoxeeLoginWizardAddExistingUser();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);
  
  bool GetIsRememberPassword();

protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

private:

  bool HandleClick(CGUIMessage& message);
  bool OnLoginButton();
  bool CanExecuteLogin();
  bool IsThereEmptyFieldsForLogin();

  bool DoLogin();

  bool m_rememberPassword;

};
