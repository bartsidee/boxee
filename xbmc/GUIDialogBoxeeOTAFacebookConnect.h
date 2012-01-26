#pragma once

#include "GUIDialogBoxeeWizardBase.h"

class CGUIDialogBoxeeOTAFacebookConnect : public CGUIDialogBoxeeWizardBase
{
public:
  CGUIDialogBoxeeOTAFacebookConnect();
  virtual ~CGUIDialogBoxeeOTAFacebookConnect();
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  virtual bool OnAction(const CAction& action);

private:
};
