#ifndef GUI_WINDOW_BOXEE_WIZARD_SOURCE_MANAGER
#define GUI_WINDOW_BOXEE_WIZARD_SOURCE_MANAGER

#pragma once

#include <vector>
#include "GUIDialog.h"
#include "Settings.h"
#include "MediaSource.h"

class CGUIWindowBoxeeWizardSourceManager : public CGUIDialog
{
public:
  CGUIWindowBoxeeWizardSourceManager(void);
  virtual ~CGUIWindowBoxeeWizardSourceManager(void);
  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);
  
private:
  bool IsPredefinedShare(CMediaSource& share);
  void LoadAllShares();
  void LoadShares(VECSOURCES& shares, int controlId, int listitemsIndex);
  
  CFileItemList m_sources[3];
};

#endif
