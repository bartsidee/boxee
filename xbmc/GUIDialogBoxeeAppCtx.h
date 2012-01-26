#pragma once

#include "GUIDialog.h"
#include "GUIDialogBoxeeCtx.h"

class CGUIDialogBoxeeAppCtx : public CGUIDialogBoxeeCtx
{
public:
  CGUIDialogBoxeeAppCtx(void);
  virtual ~CGUIDialogBoxeeAppCtx(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void Update();

  virtual void OnMoreInfo() ;

protected:
  virtual void OnInitWindow();
  void ShowLastFMSettings(void);
};

