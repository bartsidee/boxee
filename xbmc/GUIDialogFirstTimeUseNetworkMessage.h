#pragma once

#include "GUIDialogFirstTimeUseBase.h"

#ifdef HAS_EMBEDDED

#define NETWORK_MESSAGE_LABEL_CONTROL            310
#define TRY_AGAIN_BUTTON_CONTROL                 300
#define SWITCH_CONNECTION_TYPE_BUTTON_CONTROL    400
#define SWITCH_LABEL_CONTROL                     401
#define ADJUST_NETWORK_SETTINGS_BUTTON_CONTROL   500

#define SWITCH_TO_ETHETNET_BUTTON 1
#define SWITCH_TO_WIRELESS_BUTTON 2

class CGUIDialogFirstTimeUseNetworkMessage : public CGUIDialogFirstTimeUseBase
{
public:
  
  CGUIDialogFirstTimeUseNetworkMessage();
  virtual ~CGUIDialogFirstTimeUseNetworkMessage();

  virtual bool OnMessage(CGUIMessage &message);

  void SetMessage(const CStdString& m_message);

  int GetSwitchToType();

  int GetButtonClicked();

protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

  CStdString m_message;
  CStdString m_switchButtonLabel;
  int m_switchToType;

  int m_buttonClicked;
};

#endif

