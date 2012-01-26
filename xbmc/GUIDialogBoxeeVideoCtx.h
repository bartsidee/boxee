#pragma once

#include "GUIDialog.h"
#include "GUIDialogBoxeeSeekableCtx.h"

class CGUIDialogBoxeeVideoCtx : public CGUIDialogBoxeeSeekableCtx
{
public:
  CGUIDialogBoxeeVideoCtx();
  virtual ~CGUIDialogBoxeeVideoCtx();
  virtual bool OnMessage(CGUIMessage &message);
  virtual bool OnAction(const CAction &action);
  virtual void Update();
  virtual void Render();
  virtual void Close(bool forceClose = false);

  virtual void OnMoreInfo();
  virtual void OnTechInfo();

protected:

  virtual void OnInitWindow();
};

