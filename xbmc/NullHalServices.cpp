/*
 * NullHalServices.cpp
 *
 *  Created on: Jun 13, 2010
 *      Author: shay
 */

#include "NullHalServices.h"

#ifdef HAS_BOXEE_HAL

CNullHalServices::CNullHalServices()
{
}

bool CNullHalServices::GetEthernetConfig(unsigned int nInstance, CHalEthernetConfig &ethernetConfig )
{
  ethernetConfig.addr_type = ADDR_NONE;
  return true;
}


bool CNullHalServices::SetEthernetConfig(unsigned int nInstance, const CHalEthernetConfig &ethernetConfig)
{
  sleep(5);
  return true;
}

bool CNullHalServices::GetEthernetInfo(unsigned int nInstance, CHalEthernetInfo &ethernetInfo)
{
  ethernetInfo.addr_type = ADDR_NONE;
  ethernetInfo.link_up = false;
  ethernetInfo.running = true;
  return true;
}

// wireless
bool CNullHalServices::GetWirelessConfig(unsigned int nInstance, CHalWirelessConfig& wirelessConfig)
{
  wirelessConfig.addr_type = ADDR_STATIC;

  return true;
}

bool CNullHalServices::SetWirelessConfig(unsigned int nInstance, CHalWirelessConfig& wirelessConfig)
{
  sleep(4);
  return true;
}

bool CNullHalServices::GetWirelessInfo(unsigned int nInstance, CHalWirelessInfo& wirelessInfo)
{
  return true;
}

bool CNullHalServices::SearchWireless(unsigned int nInstance, std::vector<CHalWirelessNetwork>& wirelessNetworks)
{
  if (wirelessNetworks.size()== 0)
  {
    CHalWirelessNetwork wn;
    wn.signal_strength = 100;
    wn.ssid = "my-network";
    wn.secure = true;
    wirelessNetworks.push_back(wn);

    wn.signal_strength = 50;
    wn.secure = true;
    wn.ssid = "erez-home";
    wirelessNetworks.push_back(wn);

    wn.signal_strength = 73;
    wn.secure = false;
    wn.ssid = "dikla-home";
    wirelessNetworks.push_back(wn);

    wn.signal_strength = 73;
    wn.secure = true;
    wn.ssid = "yuval-home";
    wirelessNetworks.push_back(wn);

    wn.signal_strength = 73;
    wn.secure = false;
    wn.ssid = "shay-home";
    wirelessNetworks.push_back(wn);
  }

  return true;
}

 // clock
bool CNullHalServices::SetTimezone(const std::string& strTimezone)
{
  return true;
}

bool CNullHalServices::GetTimezone(std::string& strTimezone)
{
  return true;
}

bool CNullHalServices::SetTime(const std::string& strTime)
{
  return true;
}

// power
bool CNullHalServices::StandBy()
{
  return true;
}

bool CNullHalServices::Resume()
{
  return true;
}

bool CNullHalServices::Shutdown()
{
  return true;
}

bool CNullHalServices::Reboot()
{
  return true;
}

// input
bool CNullHalServices::GetAllInputDevices(std::vector<CHalInputDevice>& inputDevices)
{
  return true;
}

// dvb
bool CNullHalServices::GetAllDvbDevices(std::vector<CHalDvbDevice>& dvbDevices)
{
  return true;
}

// storage
bool CNullHalServices::GetAllStorageDevices(std::vector <CHalStorageDeviceInfo>& storageDevices)
{
  CHalStorageDeviceInfo dev;
  dev.dev_type = "sd";
  dev.path = "/media/an_sd";
  dev.label = "An SD";
  storageDevices.push_back(dev);

  dev.dev_type = "usb";
  dev.path = "/media/dok";
  dev.label = "DOK";
  storageDevices.push_back(dev);

  return true;
}

bool CNullHalServices::EnableSambaShares(const std::string password, const std::string username, const std::string workgroup, const std::string hostname)
{
  return true;
}

bool CNullHalServices::DisableSambaShares()
{
  return true;
}

bool CNullHalServices::GetSambaConfig(bool &status, std::string &password, std::string &workgroup, std::string &hostname)
{
  return true;
}

bool CNullHalServices::EjectStorage(const std::string &path)
{
  return true;
}

// host
bool CNullHalServices::SetHostName(const std::string& strHostname)
{
  return true;
}

bool CNullHalServices::GetHostName(std::string& strHostname)
{
  return true;
}

CNullHalServices& CNullHalServices::GetInstance()
{
  static CNullHalServices halServices;

  return halServices;
}

// system
bool CNullHalServices::GetHardwareInfo(CHalHardwareInfo& hardwareInfo)
{
  hardwareInfo.model = "SIMAN3";
  hardwareInfo.revision = "R1.2";
  hardwareInfo.serialNumber = "12345678";
  hardwareInfo.uniqueId = "uniqui";
  hardwareInfo.vendor = "ACME";

  return true;
}

bool CNullHalServices::GetSoftwareInfo(CHalSoftwareInfo& softwareInfo)
{
  softwareInfo.language = "EN";
  softwareInfo.regionSKU = "IL";
  softwareInfo.version = "0.9.21";

  return true;
}

bool CNullHalServices::RequestUpgrade()
{
  return true;
}

bool CNullHalServices::SetAudioDACState(bool mute)
{
  return true;
}

bool AvahiDaemonRestart()
{
  return true;
}

bool DjmountRestart()
{
  return true;
}

// led
bool CNullHalServices::SetLEDBrightness(int instance, int startBrightness, int endBrightness)
{
  return true;
}

bool CNullHalServices::SetLEDState(bool isPlaying, bool isConnectedToInternet, bool isPowerSaving)
{
  return true;
}

// thermals
bool CNullHalServices::GetCPUTemperature(int& temperature)
{
  return true;
}

bool CNullHalServices::GetCPUFanSpeed(int& speed)
{
  return true;
}

//vpn
bool CNullHalServices::VpnSetConfig(const CHALVpnConfig &vpnConfig)
{
  return true;
}

bool CNullHalServices::VpnDial()
{
  return true;
}

bool CNullHalServices::VpnHangup()
{
  return true;
}

bool CNullHalServices::VpnGetInfo(CHALVpnInfo &vpnInfo)
{
  return true;
}

bool CNullHalServices::VpnGetConfig(CHALVpnConfig &vpnConfig)
{
  return true;
}

#endif
