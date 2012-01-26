#pragma once

#include "GUIDialog.h"
#include "GUIDialogBoxeeCtx.h"

class CGUIDialogBoxeeMusicCtx : public CGUIDialogBoxeeCtx
{
public:
  CGUIDialogBoxeeMusicCtx(void);
  virtual ~CGUIDialogBoxeeMusicCtx(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void Update();

  virtual void OnMoreInfo() ;
  virtual void OnPlay();

protected:
  virtual void OnInitWindow();
};

