#pragma once

#include "GUIDialog.h"
#include "GUIDialogBoxeeCtx.h"

class CGUIDialogBoxeeVideoCtx : public CGUIDialogBoxeeCtx
{
public:
  CGUIDialogBoxeeVideoCtx(void);
  virtual ~CGUIDialogBoxeeVideoCtx(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual bool OnAction(const CAction &action);
  virtual void Update();
  virtual void Render();

  virtual void OnMoreInfo() ;

protected:
  virtual void OnInitWindow();
};

