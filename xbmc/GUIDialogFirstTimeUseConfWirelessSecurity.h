#pragma once

#include "GUIDialogFirstTimeUseWithList.h"

#ifdef HAS_EMBEDDED

#include "HalServices.h"

class CGUIDialogFirstTimeUseConfWirelessSecurity : public CGUIDialogFirstTimeUseWithList
{
public:
  
  CGUIDialogFirstTimeUseConfWirelessSecurity();
  virtual ~CGUIDialogFirstTimeUseConfWirelessSecurity();

  virtual bool OnMessage(CGUIMessage &message);

  CHalWirelessAuthType GetAuth();
  void SetAuth(CHalWirelessAuthType auth);

  static CStdString GetAuthAsString(CHalWirelessAuthType auth);
  static CHalWirelessAuthType GetAuthLabelAsEnum(const CStdString& authLabel);

protected:

  virtual void OnInitWindow();

  virtual bool FillListOnInit();
  virtual bool HandleListChoice();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

  CHalWirelessAuthType m_auth;
};

#endif
