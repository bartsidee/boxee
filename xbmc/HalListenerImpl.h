#ifndef HAL_LISTENER_IMPL
#define HAL_LISTENER_IMPL

#include "system.h"

#ifdef HAS_BOXEE_HAL

#include "HalServices.h"

class HalListenerImpl : public IHalListener
{
public:
  HalListenerImpl();
  virtual void HandleNotification(CHalNotification &notification);

  void SetVpnConnected(bool connected);
  bool GetVpnConnected();

  bool IsDvbConnected();

private:
  bool m_vpnConnected;
};

extern HalListenerImpl g_halListener;

#endif

#endif

