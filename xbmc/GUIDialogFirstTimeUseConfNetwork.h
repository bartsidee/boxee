#pragma once

#include "GUIDialogFirstTimeUseBase.h"

#ifdef HAS_EMBEDDED

class CGUIDialogFirstTimeUseConfNetwork : public CGUIDialogFirstTimeUseBase
{
public:
  
  CGUIDialogFirstTimeUseConfNetwork();
  virtual ~CGUIDialogFirstTimeUseConfNetwork();

  virtual bool OnMessage(CGUIMessage &message);

  CStdString GetIpAddress();
  CStdString GetNetmask();
  CStdString GetGateway();
  CStdString GetDNS();

protected:

  virtual void OnInitWindow();

  bool ValidateFields();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

  CStdString m_ipAddress;
  CStdString m_netmask;
  CStdString m_gateway;
  CStdString m_dns;
};

#endif
