#pragma once

#include "GUIWindowSettingsScreenCalibration.h"

#ifdef HAS_EMBEDDED

class CGUIWindowFirstTimeUseCalibration : public CGUIWindowSettingsScreenCalibration
{
public:
  
  CGUIWindowFirstTimeUseCalibration();
  virtual ~CGUIWindowFirstTimeUseCalibration();

  virtual bool OnAction(const CAction& action);
  virtual bool OnMessage(CGUIMessage &message);

protected:

  virtual void OnInitWindow();

private:

};

#endif

