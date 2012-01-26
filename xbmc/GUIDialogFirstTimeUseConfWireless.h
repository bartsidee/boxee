#pragma once

#include "GUIDialogFirstTimeUseBase.h"

#ifdef HAS_EMBEDDED

class CGUIDialogFirstTimeUseConfWireless : public CGUIDialogFirstTimeUseBase
{
public:
  
  CGUIDialogFirstTimeUseConfWireless();
  virtual ~CGUIDialogFirstTimeUseConfWireless();

  virtual bool OnMessage(CGUIMessage &message);

  int GetChoiceSelected();

  CStdString GetPassword();

protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

  void SetSelectAUTOMATIC();
  void SetSelectMANUALLY();

  int m_buttonSelected;

  CStdString m_password;
};

#endif

