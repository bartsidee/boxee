#pragma once

#include "GUIDialogBoxeeWizardBase.h"
#include "FileItem.h"

class CGUIDialogBoxeeOTAConfirmLocation : public CGUIDialogBoxeeWizardBase
{
public:
  CGUIDialogBoxeeOTAConfirmLocation();
  virtual ~CGUIDialogBoxeeOTAConfirmLocation();
  bool GetYesButtonPressed();
  void OnInitWindow();
  virtual bool OnMessage(CGUIMessage& message);

protected:
  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

private:
  bool m_bYesButtonPressed;
};

class CGUIDialogBoxeeOTACountriesLocationConfiguration : public CGUIDialogBoxeeWizardBase
{
public:
  CGUIDialogBoxeeOTACountriesLocationConfiguration();
  virtual ~CGUIDialogBoxeeOTACountriesLocationConfiguration();
  virtual bool OnAction(const CAction& action);
  void OnInitWindow();

private:
  bool GetCountriesFromServer();
  bool HandleSelectCountry();
  bool OnInit();

  CFileItemList m_countries;
};

class CGUIDialogBoxeeOTAZipcodeLocationConfiguration : public CGUIDialogBoxeeWizardBase
{
public:
  CGUIDialogBoxeeOTAZipcodeLocationConfiguration();
  virtual ~CGUIDialogBoxeeOTAZipcodeLocationConfiguration();
  virtual bool OnAction(const CAction& action);
  void OnInitWindow();

private:
  bool HandleSelectZipCode();
  void HandleInvalidZipCode();
};
