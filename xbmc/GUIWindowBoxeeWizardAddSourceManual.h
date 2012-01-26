#ifndef GUI_WINDOW_BOXEE_WIZARD_ADD_SOURCE_MANUAL
#define GUI_WINDOW_BOXEE_WIZARD_ADD_SOURCE_MANUAL

#pragma once

#include <vector>
#include "GUIDialog.h"

class CGUIWindowBoxeeWizardAddSourceManual : public CGUIDialog
{
public:
  CGUIWindowBoxeeWizardAddSourceManual(void);
  virtual ~CGUIWindowBoxeeWizardAddSourceManual(void);
  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);

  CStdString GetURL();
  bool IsConfirmed();
  void SetProtocol(const CStdString newProtocol);
  CStdString GetProtocol();
  
private:
  CStdString m_host;
  CStdString m_protocol;
  CStdString m_user;
  CStdString m_password;
  bool m_IsConfirmed;  
};

#endif
