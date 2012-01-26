#pragma once

#include "GUIDialogFirstTimeUseWithList.h"

#ifdef HAS_EMBEDDED

#include "Resolution.h"

class CGUIDialogFirstTimeUseResolution : public CGUIDialogFirstTimeUseWithList
{
public:
  
  CGUIDialogFirstTimeUseResolution();
  virtual ~CGUIDialogFirstTimeUseResolution();

  CStdString GetSelectedResStr();

protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

  virtual bool FillListOnInit();
  virtual bool HandleListChoice();

  RESOLUTION GetSelectedResolutionEnum();

  bool TestNewRes();
};

#endif

