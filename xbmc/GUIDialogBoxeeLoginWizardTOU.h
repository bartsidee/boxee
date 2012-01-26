#pragma once

#include "GUIDialogBoxeeWizardBase.h"

class CGUIDialogBoxeeLoginWizardTOU : public CGUIDialogBoxeeWizardBase
{
public:

  CGUIDialogBoxeeLoginWizardTOU();
  virtual ~CGUIDialogBoxeeLoginWizardTOU();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);

protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

private:

  bool HandleClick(CGUIMessage& message);

  bool HandleClickOnTouButton();
  bool HandleClickOnPpButton();

  bool InitContainerText(const CStdString& text);

  CStdString m_touStr;
  CStdString m_ppStr;
};
