#pragma once

#include "GUIDialogBoxBase.h"

class CGUIDialogBoxeeExitVideo : public CGUIDialogBoxBase
{
public:
  CGUIDialogBoxeeExitVideo();
  virtual ~CGUIDialogBoxeeExitVideo();

  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);
  static bool ShowAndGetInput();

private:
  bool m_stopVideo;
  bool m_dontShowDialog;
  CStdString m_settingsStr;
};
