#pragma once

#include "GUIDialogBoxeeWizardBase.h"

class CGUIDialogBoxeeLoginWizardMenuCust : public CGUIDialogBoxeeWizardBase
{
public:

  CGUIDialogBoxeeLoginWizardMenuCust();
  virtual ~CGUIDialogBoxeeLoginWizardMenuCust();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);

  void SetUserMenuCust();

protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

private:

  bool HandleClick(CGUIMessage& message);

  CStdString m_tvShowMenuCategory;
  CStdString m_movieMenuCategory;
  CStdString m_appMenuCategory;
};
