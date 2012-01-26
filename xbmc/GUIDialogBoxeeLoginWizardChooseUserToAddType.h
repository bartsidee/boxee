#pragma once

#include "GUIDialogBoxeeWizardBase.h"

class CGUIDialogBoxeeLoginWizardChooseUserToAddType : public CGUIDialogBoxeeWizardBase
{
public:

  CGUIDialogBoxeeLoginWizardChooseUserToAddType();
  virtual ~CGUIDialogBoxeeLoginWizardChooseUserToAddType();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);
  
  bool IsAddingNewUser();

protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

private:

  bool m_addingNewUser;

};
