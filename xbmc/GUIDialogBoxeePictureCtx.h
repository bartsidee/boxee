#ifndef GUIDIALOGBOXEEPICTURECTX_H_
#define GUIDIALOGBOXEEPICTURECTX_H_

#pragma once

#include "GUIDialog.h"
#include "GUIDialogBoxeeCtx.h"

class CGUIDialogBoxeePictureCtx : public CGUIDialogBoxeeCtx
{
public:
  CGUIDialogBoxeePictureCtx(void);
  virtual ~CGUIDialogBoxeePictureCtx(void);
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void Update();

  virtual void OnMoreInfo() ;

protected:
  virtual void OnInitWindow();
  
  void ZoomInActionUpdate();
  void ZoomOutActionUpdate();

  bool m_bIsZooming;
  bool m_bWasPlaying;
  
  int m_zoomInFactor;
  
  int m_numOfItemsInSlideshow;
};

#endif /* GUIDIALOGBOXEEPICTURECTX_H_ */
