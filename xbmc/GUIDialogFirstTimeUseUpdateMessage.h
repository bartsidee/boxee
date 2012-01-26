#pragma once

#include "GUIDialogFirstTimeUseBase.h"

#ifdef HAS_EMBEDDED

class CGUIDialogFirstTimeUseUpdateMessage : public CGUIDialogFirstTimeUseBase
{
public:

  CGUIDialogFirstTimeUseUpdateMessage();
  virtual ~CGUIDialogFirstTimeUseUpdateMessage();

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage &message);

  void SetMessage(const CStdString& message);
  void SetButtonLabel(const CStdString& buttonLabel);

protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

  CStdString m_message;
  CStdString m_buttonLabel;
};

#endif

