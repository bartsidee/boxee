#pragma once
#include "GUIDialog.h"

class CGUIDialogBoxeeCredits : public CGUIDialog
{
public:
  CGUIDialogBoxeeCredits(void);
  virtual ~CGUIDialogBoxeeCredits(void);
  virtual bool OnMessage(CGUIMessage& message);
};
