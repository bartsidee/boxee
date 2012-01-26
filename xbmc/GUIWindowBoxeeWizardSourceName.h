#ifndef GUI_WINDOW_BOXEE_WIZARD_SOURCE_NAME
#define GUI_WINDOW_BOXEE_WIZARD_SOURCE_NAME

#pragma once

#include <vector>
#include "GUIDialog.h"

class CGUIWindowBoxeeWizardSourceName : public CGUIDialog
{
public:
  CGUIWindowBoxeeWizardSourceName(void);
  virtual ~CGUIWindowBoxeeWizardSourceName(void);
  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);

  void SetSourcePath(const CStdString &strPath);
  
  bool IsConfirmed();
  CStdString GetSourceName();
  
private:
   bool m_confirmed;
   CStdString m_sourceName;
   CStdString m_sourcePath;
};

#endif
