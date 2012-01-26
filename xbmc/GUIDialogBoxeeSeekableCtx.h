#pragma once

#include "GUIDialogBoxeeCtx.h"
#include "DateTime.h"

#define CONTROL_SEEK_BUTTON 8100
//#define CONTROL_SEEK_ARROW_IMAGE 8200

class CGUIButtonControl;
class CGUIImage;

class CSeekDirection
{
public:
  enum SeekDirectionEnums
  {
    NONE=0,
    BACKWARD=1,
    FORWARD=2
  };
};

class CGUIDialogBoxeeSeekableCtx : public CGUIDialogBoxeeCtx
{
public:

  CGUIDialogBoxeeSeekableCtx(DWORD dwID, const CStdString &xmlFile);
  virtual ~CGUIDialogBoxeeSeekableCtx();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);

  virtual void Render();

  virtual void Close(bool forceClose = false);

  virtual void SetSeekDirectionOnOpen(CSeekDirection::SeekDirectionEnums seekDirection);

protected:

  virtual void OnInitWindow();

  void ChangeSeek(bool back);

  bool SetSeekControlPos(float playbackPosX);
  void adjustPlaybackPosX(float& seekButtonPosX);

  double m_totalTime;
  double m_currentTime;
  unsigned int m_seekHoldTime;
  long m_lastSeekEvent;
  bool m_seekForward;

  CStdString m_seekTimeLabel;
  bool m_onUserMove;

  TIME_FORMAT m_labelTimeFormt;

  CGUIButtonControl* m_buttonCtrl;
  //CGUIImage* m_arrowImageCtrl;

  CSeekDirection::SeekDirectionEnums m_seekDirectionOnOpen;

  bool m_showPlayTimeRemaining;

  bool m_needToAdjustPlaybackPosX;
};

