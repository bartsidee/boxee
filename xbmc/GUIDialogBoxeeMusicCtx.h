#pragma once

#include "GUIDialogBoxeeSeekableCtx.h"

class CGUIDialogBoxeeMusicCtx : public CGUIDialogBoxeeSeekableCtx
{
public:
  CGUIDialogBoxeeMusicCtx();
  virtual ~CGUIDialogBoxeeMusicCtx();
  virtual bool OnMessage(CGUIMessage &message);
  virtual void Update();

  virtual void OnMoreInfo() ;
  virtual void OnPlay();

protected:
  virtual void OnInitWindow();
};

