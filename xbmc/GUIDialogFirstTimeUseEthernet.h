#pragma once

#include "GUIDialogFirstTimeUseBase.h"

#ifdef HAS_EMBEDDED

#define AUTOMATIC_CONF 281
#define MANUALLY_CONF 282
#define SWITCH_TO_WIRELESS 283

class CGUIDialogFirstTimeUseEthernet : public CGUIDialogFirstTimeUseBase
{
public:
  
  CGUIDialogFirstTimeUseEthernet();
  virtual ~CGUIDialogFirstTimeUseEthernet();

  virtual bool OnMessage(CGUIMessage &message);

  int GetChoiceSelected();

protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

  void SetSelectAUTOMATIC();
  void SetSelectMANUALLY();
  void SetSelectSWITCH_TO_WIRELESS();

  int m_buttonSelected;

};

#endif

