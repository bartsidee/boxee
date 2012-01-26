#ifndef GUI_WINDOW_BOXEE_WIZARD_NETWORK_MANUAL
#define GUI_WINDOW_BOXEE_WIZARD_NETWORK_MANUAL

#pragma once

#include <vector>
#include "GUIDialog.h"

class CGUIWindowBoxeeWizardNetworkManual : public CGUIDialog
{
public:
  CGUIWindowBoxeeWizardNetworkManual(void);
  virtual ~CGUIWindowBoxeeWizardNetworkManual(void);
  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);
};

#endif
