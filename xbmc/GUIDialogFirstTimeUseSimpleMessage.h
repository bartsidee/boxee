#pragma once

#include "GUIDialogFirstTimeUseBase.h"

#ifdef HAS_EMBEDDED

class CGUIDialogFirstTimeUseSimpleMessage : public CGUIDialogFirstTimeUseBase
{
public:
  
  CGUIDialogFirstTimeUseSimpleMessage();
  virtual ~CGUIDialogFirstTimeUseSimpleMessage();

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage &message);

  void SetMessage(const CStdString& m_message);

protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

  CStdString m_message;
};

#endif

