#pragma once

#include "GUIDialogFirstTimeUseWithList.h"

#ifdef HAS_EMBEDDED

class CGUIDialogFirstTimeUseLang : public CGUIDialogFirstTimeUseWithList
{
public:
  
  CGUIDialogFirstTimeUseLang();
  virtual ~CGUIDialogFirstTimeUseLang();

  virtual bool OnMessage(CGUIMessage &message);

protected:

  bool UpdateLanguage();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

  virtual bool FillListOnInit();
  virtual bool HandleListChoice();
};

#endif

