#pragma once

#include "GUIDialogBoxeeWizardBase.h"

class CGUIDialogBoxeeLoginWizardConfirmation : public CGUIDialogBoxeeWizardBase
{
public:

  CGUIDialogBoxeeLoginWizardConfirmation();
  virtual ~CGUIDialogBoxeeLoginWizardConfirmation();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);

  void SetUserMenuCust();

protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

private:

};
