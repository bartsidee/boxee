#pragma once

#include "GUIDialogBoxeeWizardBase.h"

class CGUIDialogBoxeeLoginWizardQuickTipAirPlay : public CGUIDialogBoxeeWizardBase
{
public:
  CGUIDialogBoxeeLoginWizardQuickTipAirPlay();
  virtual ~CGUIDialogBoxeeLoginWizardQuickTipAirPlay();

  virtual bool OnAction(const CAction& action);
  virtual bool OnMessage(CGUIMessage &message);

protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();
};

