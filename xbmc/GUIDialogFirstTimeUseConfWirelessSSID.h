#pragma once

#include "GUIDialogFirstTimeUseBase.h"

#ifdef HAS_EMBEDDED

class CGUIDialogFirstTimeUseConfWirelessSSID : public CGUIDialogFirstTimeUseBase
{
public:

  CGUIDialogFirstTimeUseConfWirelessSSID();
  virtual ~CGUIDialogFirstTimeUseConfWirelessSSID();

  virtual bool OnMessage(CGUIMessage &message);

  CStdString GetSSID();
  void SetSSID(const CStdString& ssid);

protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

  CStdString m_ssid;
};

#endif

