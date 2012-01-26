#pragma once

#include "GUIDialogBoxeeWizardBase.h"

class CGUIDialogBoxeeLoginWizardQuickTip : public CGUIDialogBoxeeWizardBase
{
public:
  CGUIDialogBoxeeLoginWizardQuickTip();
  virtual ~CGUIDialogBoxeeLoginWizardQuickTip();

  virtual bool OnAction(const CAction& action);
  virtual bool OnMessage(CGUIMessage &message);
  
protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();
};

