#pragma once

#include "GUIDialogFirstTimeUseBase.h"

#ifdef HAS_EMBEDDED

class CGUIDialogFirstTimeUseWelcome : public CGUIDialogFirstTimeUseBase
{
public:
  
  CGUIDialogFirstTimeUseWelcome();
  virtual ~CGUIDialogFirstTimeUseWelcome();

protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

};

#endif
