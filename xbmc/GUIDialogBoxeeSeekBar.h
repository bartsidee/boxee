#pragma once

#include "GUIDialog.h"
#include "DateTime.h"

class CGUIButtonControl;
class CGUIImage;

class CGUIDialogBoxeeSeekBar : public CGUIDialog
{
public:

  CGUIDialogBoxeeSeekBar();
  virtual ~CGUIDialogBoxeeSeekBar(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);

  virtual void OnInitWindow();
  virtual void Render();

protected:

  void ChangeSeek(bool back);

  bool SetSeekControlPos(float playbackPosX);
  double m_totalTime;
  double m_currentTime;
  unsigned int m_seekHoldTime;
  long m_lastSeekEvent;
  bool m_seekForward;

  CStdString m_seekTimeLabel;
  bool m_onUserMove;

  TIME_FORMAT m_labelTimeFormt;

  CGUIButtonControl* m_buttonCtrl;
  CGUIImage* m_arrowImageCtrl;
};

