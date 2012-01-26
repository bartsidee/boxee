#pragma once

#include "BoxeeSimpleDialogWizardManager.h"
#include "BoxeeOTAConfigurationData.h"

class CGUIDialogBoxeeWizardBase;

class CBoxeeOTAConfigurationManager : public CBoxeeSimpleDialogWizardManager
{
public:
  static CBoxeeOTAConfigurationManager& GetInstance();
  virtual ~CBoxeeOTAConfigurationManager();

  CBoxeeOTAConfigurationData& GetConfigurationData() { return m_data; }
  bool RunWizard(bool rescan);

protected:
  virtual CGUIDialogBoxeeWizardBase* HandleNextAction(CGUIDialogBoxeeWizardBase* pDialog, bool& addCurrentDlgToStack);

private:
  CBoxeeOTAConfigurationManager();

  CBoxeeOTAConfigurationData m_data;
  bool m_isConnectedToFacebook;
  bool m_rescan;
};
