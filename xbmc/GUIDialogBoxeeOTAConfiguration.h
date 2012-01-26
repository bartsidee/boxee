#pragma once

#include "GUIDialogBoxeeWizardBase.h"
#include "GUIDialogBoxeeOTALocationConfiguration.h"

class CGUIDialogBoxeeOTAProviderConfiguration : public CGUIDialogBoxeeWizardBase
{
public:

  CGUIDialogBoxeeOTAProviderConfiguration();
  virtual ~CGUIDialogBoxeeOTAProviderConfiguration();
  void OnInitWindow();
  virtual bool OnAction(const CAction& action);

private:
  bool GetProvidersFromServer();
  bool HandleSelectProvider();
  CFileItemList m_providers;
};

class CGUIDialogBoxeeOTAConnectionConfiguration : public CGUIDialogBoxeeWizardBase
{
public:

  CGUIDialogBoxeeOTAConnectionConfiguration();
  virtual ~CGUIDialogBoxeeOTAConnectionConfiguration();
  virtual bool OnAction(const CAction& action);
};

class CGUIDialogBoxeeOTAWelcome : public CGUIDialogBoxeeWizardBase
{
public:
  CGUIDialogBoxeeOTAWelcome();
  virtual ~CGUIDialogBoxeeOTAWelcome();
  void setRescan(bool rescan) { m_rescan = rescan; }

private:
  virtual void OnInitWindow();
  bool m_rescan;
};
