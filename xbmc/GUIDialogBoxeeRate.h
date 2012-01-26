#pragma once
#include "GUIDialog.h"

class CGUIDialogBoxeeRate :
      public CGUIDialog
{
public:
  CGUIDialogBoxeeRate(void);
  virtual ~CGUIDialogBoxeeRate(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);
  
  static bool ShowAndGetInput(bool &bLike);
protected:
  bool m_bConfirmed;
  bool m_bLike;
};

