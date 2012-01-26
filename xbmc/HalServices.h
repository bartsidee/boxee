/*
 * HalServices.h
 *
 *  Created on: Jun 7, 2010
 *      Author: shay
 */

#ifndef HALSERVICES_H_
#define HALSERVICES_H_

#include "system.h"

#ifdef HAS_BOXEE_HAL

#include <string>
#include <vector>

enum CHalNotificationType
{
  HAL_NO_NOTIFICATION = 0,
  HAL_NOTIFY_ETHERNET_LINK_STATE,
  HAL_NOTIFY_ETHERNET_ADDR_CHANGE,
  HAL_NOTIFY_WIRELESS_LINK_STATE,
  HAL_NOTIFY_WIRELESS_ADDR_CHANGE,
  HAL_NOTIFY_POWER,
  HAL_NOTIFY_INPUT,
  HAL_NOTIFY_STORAGE,
  HAL_NOTIFY_VPN,
  HAL_NOTIFY_DVB,
  HAL_NOTIFY_ALL
};

enum CHalAddrType
{
  ADDR_NONE,
  ADDR_STATIC,
  ADDR_DYNAMIC
};

struct CHalInputDevice
{
  int         instance;
  std::string label;
  std::string path;
  bool        connected;
};

struct CHalDvbDevice
{
  int         instance;
  std::string path;
  bool        connected;
  bool        ready;
};

class CHalNotification
{
public:
  CHalNotificationType GetType() { return m_nType; }

protected:
  CHalNotificationType m_nType;
};

class CHalInputNotification : public CHalNotification
{
public:
  CHalInputNotification( bool connected, int instance, std::string &label,   std::string &path)
  {
    m_halInputDevice.instance = instance;
    m_halInputDevice.label = label;
    m_halInputDevice.path = path;
    m_halInputDevice.connected = connected;
    m_nType = HAL_NOTIFY_INPUT;
  }
  bool IsConnected() {return m_halInputDevice.connected;}
  std::string GetPath() {return m_halInputDevice.path;}
  std::string GetLabel() {return m_halInputDevice.label;}
  int GetInstance() {return m_halInputDevice.instance;}

private:
  CHalInputDevice m_halInputDevice;
};

class CHalDvbNotification : public CHalNotification
{
public:
  CHalDvbNotification(bool connected, bool ready, int instance, std::string path)
  {
    m_halDvbDevice.instance = instance;
    m_halDvbDevice.path = path;
    m_halDvbDevice.connected = connected;
    m_halDvbDevice.ready = ready;
    m_nType = HAL_NOTIFY_DVB;
  }
  std::string GetPath() {return m_halDvbDevice.path;}
  int GetInstance() {return m_halDvbDevice.instance;}
  bool IsConnected() {return m_halDvbDevice.connected;}
  bool IsReady() {return m_halDvbDevice.ready;}

private:
  CHalDvbDevice m_halDvbDevice;
};

struct CHalEthernetInfo
{
  CHalAddrType  addr_type;
  bool          link_up;
  bool          running;
  std::string   ip_address;
  std::string   netmask;
  std::string   gateway;
  std::string   dns;
  std::string   mac_address;
};

struct CHalEthernetConfig
{
  CHalAddrType  addr_type;
  // relevant only for dynamic address
  std::string   ip_address;
  std::string   netmask;
  std::string   gateway;
  std::string   dns;
};

class CHalEthernetLinkStateNotification : public CHalNotification
{
public:
  CHalEthernetLinkStateNotification (bool linkUp):m_linkUp(linkUp)  {m_nType = HAL_NOTIFY_ETHERNET_LINK_STATE;}
  bool IsLinkUp() {return m_linkUp;}

private:
  bool m_linkUp;
};

class CHalEthernetAddressNotification : public CHalNotification
{
public:
  CHalEthernetAddressNotification (const CHalEthernetInfo &ethInfo)  {m_ethInfo = ethInfo;}
  void GetLinkInfo (CHalEthernetInfo &ethInfo) const {ethInfo = m_ethInfo;}

private:
  CHalEthernetInfo m_ethInfo;
};

enum CHalWirelessAuthType
{
  AUTH_NONE,
  AUTH_DONTCARE,
  AUTH_WEP_KEY,
  AUTH_WEP_PASSPHRASE,
  AUTH_WPAPSK,
  AUTH_WPA2PSK
};

struct CHalWirelessInfo
{
  std::string       ssid;
  std::string       mac_address;
  CHalAddrType     addr_type;
  bool             connected;
  std::string       ip_address;
  std::string       netmask;
  std::string       gateway;
  std::string       dns;
};

struct CHalWirelessConfig
{
  CHalAddrType   addr_type;
  std::string     ssid;
  CHalWirelessAuthType authType;
  std::string     password;
  std::string     ip_address;
  std::string     netmask;
  std::string     gateway;
  std::string     dns;
};

struct CHalWirelessNetwork
{
  CHalWirelessNetwork() {}

  CHalWirelessNetwork(const CHalWirelessNetwork& rhs)
  {
    *this = rhs;
  }

  const CHalWirelessNetwork& operator=(const CHalWirelessNetwork& rhs)
  {
    ssid = rhs.ssid;
    signal_strength = rhs.signal_strength;
    secure = rhs.secure;

    return *this;
  }

  std::string     ssid;
  unsigned long   signal_strength;
  bool            secure;
};

class CHalWirelessStateNotification : public CHalNotification
{
public:
  CHalWirelessStateNotification (bool linkUp):m_linkUp(linkUp) { m_nType = HAL_NOTIFY_WIRELESS_LINK_STATE;}
  bool IsUp() {return m_linkUp;}

private:
  bool m_linkUp;
};

class CHalWirelessAddressNotification : public CHalNotification
{
public:
  CHalWirelessAddressNotification (const CHalWirelessInfo &linkInfo)  {m_linkInfo = linkInfo;}
  void GetLinkInfo (CHalWirelessInfo &linkInfo) const {linkInfo = m_linkInfo;}

private:
  CHalWirelessInfo m_linkInfo;
};

#define HAL_MOUNT_POINT "/tmp/mnt/"

struct CHalStorageDeviceInfo
{
  std::string dev_type; /* usb, sd, internal_hd */
  std::string fs_type;
  std::string path;
  std::string label;
  bool        mounted;
};

struct CHalShareInfo
{
  bool          enabled;
  std::string   path;
  std::string   username;
};

class CHalStorageNotification : public CHalNotification
{
public:
  CHalStorageNotification(bool mounted, std::string &path, bool successfully)
  {
    m_halStorageDevice.path = path;
    m_halStorageDevice.mounted = mounted;
    m_nType = HAL_NOTIFY_STORAGE;
    m_bSuccessfulMount = successfully;
  }
  bool IsMounted() {return m_halStorageDevice.mounted;};
  bool IsSuccessful() {return m_bSuccessfulMount;};
  std::string GetPath() {return m_halStorageDevice.path;};

private:
  CHalStorageDeviceInfo m_halStorageDevice;
  bool m_bSuccessfulMount;
};

struct CHalSoftwareInfo
{
  std::string   version;
  std::string   regionSKU;
  std::string   language;
  std::string   compositeVideo; // "PAL" or "NTSC"
};

struct CHalHardwareInfo
{
  std::string   vendor;
  std::string   model;
  std::string   revision;
  std::string   serialNumber;
  std::string   uniqueId;
};

enum CHalVpnMppeSetting
{
  CHalVpnMppeAuto,
  CHalVpnMppeForce
};

enum CHalVpnTunnelType
{
  CHalVpnPPTP
};

enum CHalVpnStatus
{
  CHalVpnConnected,
  CHalVpnDisconnected,
  CHalVpnDisabled
};

struct CHALVpnConfig
{
  CHalVpnTunnelType   tunnelType;
  std::string         host;
  std::string         username;
  std::string         password;
  CHalVpnMppeSetting  encType;
};

struct CHALVpnInfo
{
  CHalVpnTunnelType   tunnelType;
  std::string         host;
  std::string         username;
  std::string         password;
  CHalVpnMppeSetting  encType;
  CHalVpnStatus       status;
};

class CHalVpnNotification : public CHalNotification
{
public:
  CHalVpnNotification(bool connected)
  {
    m_connected = connected;
    m_nType = HAL_NOTIFY_VPN;
  }
  bool IsConnected() {return m_connected;};

private:
  bool m_connected;
};


class IHalListener
{
public:
  virtual ~IHalListener() {}
  virtual void HandleNotification(CHalNotification &notification) = 0;
};

class IHalServices
{
public:
  virtual ~IHalServices() {}
  virtual bool GetEthernetConfig(unsigned int nInstance, CHalEthernetConfig &ethernetConfig ) = 0;
  virtual bool SetEthernetConfig(unsigned int nInstance, const CHalEthernetConfig &ethernetDevice) = 0;
  virtual bool GetEthernetInfo(unsigned int nInstance, CHalEthernetInfo &ethDevice) = 0;

  // wireless
  virtual bool GetWirelessConfig(unsigned int nInstance, CHalWirelessConfig& wirelessDevice) = 0;
  virtual bool SetWirelessConfig(unsigned int nInstance, CHalWirelessConfig& wirelessDevice) = 0;
  virtual bool GetWirelessInfo(unsigned int nInstance, CHalWirelessInfo& wirelessDevice) = 0;
  virtual bool SearchWireless(unsigned int nInstance, std::vector<CHalWirelessNetwork>& wirelessNetworks) = 0;

  // clock
  virtual bool SetTimezone(const std::string& strTimezone) = 0;
  virtual bool GetTimezone(std::string& strTimezone) = 0;
  virtual bool SetTime(const std::string& strTime) = 0;

  // power
  virtual bool StandBy() = 0;
  virtual bool Shutdown() = 0;
  virtual bool Reboot() = 0;
  virtual bool Resume() = 0;

  // input
  virtual bool GetAllInputDevices(std::vector<CHalInputDevice>& inputDevices) = 0;

  // dvb
  virtual bool GetAllDvbDevices(std::vector<CHalDvbDevice>& dvbDevices) = 0;

  // storage
  virtual bool GetAllStorageDevices(std::vector <CHalStorageDeviceInfo>& storageDevicesInfo) = 0;
  virtual bool EnableSambaShares(const std::string password, const std::string username = "", const std::string workgroup = "", const std::string hostname = "") = 0;
  virtual bool DisableSambaShares() = 0;
  virtual bool GetSambaConfig(bool &status, std::string &password, std::string &workgroup, std::string &hostname) = 0;
  virtual bool EjectStorage(const std::string& path) = 0;

  // host
  virtual bool SetHostName(const std::string& strHostname) = 0;
  virtual bool GetHostName(std::string& strHostname) = 0;

  // led
  virtual bool SetLEDBrightness(int instance, int startBrightness, int endBrightness) = 0;
  virtual bool SetLEDState(bool isPlaying, bool isConnectedToInternet, bool isPowerSaving) = 0;
  // thermals
  virtual bool GetCPUTemperature(int& temperature) = 0;
  virtual bool GetCPUFanSpeed(int& speed) = 0;

  // system
  virtual bool GetHardwareInfo(CHalHardwareInfo& hardwareInfo) = 0;
  virtual bool GetSoftwareInfo(CHalSoftwareInfo& softwareInfo) = 0;
  virtual bool RequestUpgrade() = 0;
  virtual bool SetAudioDACState(bool mute) = 0;
  virtual bool AvahiDaemonRestart() = 0;
  virtual bool DjmountRestart() = 0;

  //vpn
  virtual bool VpnSetConfig(const CHALVpnConfig &vpnConfig) = 0;
  virtual bool VpnDial() = 0;
  virtual bool VpnHangup() = 0;
  virtual bool VpnGetInfo(CHALVpnInfo &vpnInfo) = 0;
  virtual bool VpnGetConfig(CHALVpnConfig &vpnConfig) = 0;

  // listener
  virtual void AddListener(IHalListener *listener) = 0;
};

class CHalServicesFactory
{
public:
  static IHalServices& GetInstance();
};

#endif

#endif /* HALSERVICES_H_ */
