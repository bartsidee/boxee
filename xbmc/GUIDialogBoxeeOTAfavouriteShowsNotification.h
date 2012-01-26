#pragma once

#include "GUIDialogBoxeeWizardBase.h"
#include "FileItem.h"

class CGUIDialogBoxeeOTAfavouriteShowsNotification : public CGUIDialogBoxeeWizardBase
{
public:

  CGUIDialogBoxeeOTAfavouriteShowsNotification();
  virtual ~CGUIDialogBoxeeOTAfavouriteShowsNotification();
  void OnInitWindow();
  virtual bool OnAction(const CAction& action);
};
