#ifndef GUI_WINDOW_BOXEE_WIZARD_TIMEZONE
#define GUI_WINDOW_BOXEE_WIZARD_TIMEZONE

#pragma once

#include <vector>
#include "GUIDialog.h"

class CGUIWindowBoxeeWizardTimezone : public CGUIDialog
{
public:
  CGUIWindowBoxeeWizardTimezone(void);
  virtual ~CGUIWindowBoxeeWizardTimezone(void);
  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);
};

#endif
