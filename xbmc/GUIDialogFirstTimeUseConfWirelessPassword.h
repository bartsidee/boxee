#pragma once

#include "GUIDialogFirstTimeUseBase.h"

#ifdef HAS_EMBEDDED

class CGUIDialogFirstTimeUseConfWirelessPassword : public CGUIDialogFirstTimeUseBase
{
public:
  
  CGUIDialogFirstTimeUseConfWirelessPassword();
  virtual ~CGUIDialogFirstTimeUseConfWirelessPassword();

  virtual bool OnMessage(CGUIMessage &message);

  CStdString GetNetworkName();
  void SetNetworkName(const CStdString& networkName, bool isNetworkRequiresPassword = false);

  CStdString GetPassword();
  void SetPassword(const CStdString& password);

protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

  CStdString m_networkName;
  bool m_isNetworkRequiresPassword;
  CStdString m_password;
};

#endif

