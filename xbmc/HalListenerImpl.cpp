#include "HalListenerImpl.h"

#ifdef HAS_BOXEE_HAL

#include "Application.h"
#include "log.h"
#include "guilib/common/LinuxInputDevices.h"
#include "WinEvents.h"
#include "GUIWindowManager.h"
#include "GUIWindowSettingsCategory.h"
#include "cores/dvb/dvbmanager.h"
#include "Util.h"
#include <boost/foreach.hpp>

HalListenerImpl g_halListener;

HalListenerImpl::HalListenerImpl()
{
  CHALVpnInfo vpnInfo;
  m_vpnConnected = false;
  if (CHalServicesFactory::GetInstance().VpnGetInfo(vpnInfo))
  {
    if (vpnInfo.status == CHalVpnConnected)
      m_vpnConnected = true;
  }

  CLog::Log(LOGINFO, "Hal Listener: vpn is %s", (m_vpnConnected ? "connected" : "disconnected"));
}

void HalListenerImpl::HandleNotification(CHalNotification &notification)
{
  CLog::Log(LOGINFO, "HalListenerImpl::HandleNotification notification type %d", notification.GetType());
  if (notification.GetType() == HAL_NOTIFY_INPUT)
  {
    CHalInputNotification& inputNotification = (CHalInputNotification&) notification;
    CLog::Log(LOGINFO, "HAL Input notification: connect=%d label=%s path=%s", inputNotification.IsConnected(),
        inputNotification.GetLabel().c_str(), inputNotification.GetPath().c_str());
#ifdef HAS_INTELCE
    // Sleep a few seconds to let the files to be created by udev
    Sleep(3000);
    CWinEvents::RefreshDevices();
#endif
  }

  else if (notification.GetType() == HAL_NOTIFY_STORAGE)
  {
    CHalStorageNotification& storageNotification = (CHalStorageNotification&) notification;
    CLog::Log(LOGINFO, "HAL Storage notification: path=%s", storageNotification.GetPath().c_str());
  }

  else if (notification.GetType() == HAL_NOTIFY_VPN)
  {
    CHalVpnNotification& vpnNotification = (CHalVpnNotification&) notification;
    SetVpnConnected(vpnNotification.IsConnected());
    g_application.SetConnectionToInternetChanged();
    CLog::Log(LOGINFO, "HAL vpn notification: %s", (vpnNotification.IsConnected() ? "connected" : "disconnected"));
  }
  else if (notification.GetType() == HAL_NOTIFY_ETHERNET_LINK_STATE)
  {
    CHalEthernetLinkStateNotification ethNotification = (CHalEthernetLinkStateNotification&) notification;
    CLog::Log(LOGINFO, "Ethernet link notification: link %s", (ethNotification.IsLinkUp() ? "up" : "down"));
    if (ethNotification.IsLinkUp())
      g_application.SetConnectionToInternetChanged();
    else
      g_application.SetIsConnectedToInternet(false);
  }
#ifdef HAS_DVB
  else if (notification.GetType() == HAL_NOTIFY_DVB)
  {
    CHalDvbNotification& dvbNotification = (CHalDvbNotification&) notification;

    if (dvbNotification.IsReady())
    {
      DVBManager::GetInstance().OnDvbDongleReady(dvbNotification.GetInstance());
    }
    else if (dvbNotification.IsConnected())
    {
      DVBManager::GetInstance().OnDvbDongleInserted();
    }
    else
    {
      DVBManager::GetInstance().OnDvbDongleRemoved(dvbNotification.GetInstance());
    }

    CLog::Log(LOGINFO, "HAL dvb notification: %s", (dvbNotification.IsConnected() ? "connected" : "disconnected"));
  }
#endif
}

void HalListenerImpl::SetVpnConnected(bool connected)
{
  m_vpnConnected = connected;
}

bool HalListenerImpl::GetVpnConnected()
{
  return m_vpnConnected;
}

#endif

