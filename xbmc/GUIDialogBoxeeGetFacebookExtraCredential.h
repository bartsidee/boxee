#pragma once

#include "GUIDialogBoxBase.h"

class CGUIDialogBoxeeGetFacebookExtraCredential:
public CGUIDialogBoxBase
{
public:
  CGUIDialogBoxeeGetFacebookExtraCredential(void);
  virtual ~CGUIDialogBoxeeGetFacebookExtraCredential(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);
  virtual void OnInitWindow();

  void SetDialogText(const CStdString& heading, const CStdString& line, const CStdString& yesLabel, const CStdString& noLabel);

};


