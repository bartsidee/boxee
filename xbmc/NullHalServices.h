/*
 * NullHalServices.h
 *
 *  Created on: Jun 13, 2010
 *      Author: shay
 */

#ifndef NULLHALSERVICES_H_
#define NULLHALSERVICES_H_

#include "system.h"

#ifdef HAS_BOXEE_HAL

#include "xbmc/lib/libBoxeeHalClient/BoxeeHalClient.h"
#include "HalServices.h"
#include <string>
#include <map>

class CNullHalServices : public IHalServices
{
public:
  static CNullHalServices &GetInstance();
  virtual bool GetEthernetConfig(unsigned int nInstance, CHalEthernetConfig &ethernetConfig);
  virtual bool SetEthernetConfig(unsigned int nInstance, const CHalEthernetConfig &ethernetDevice);
  virtual bool GetEthernetInfo(unsigned int nInstance, CHalEthernetInfo &ethDevice);

  // wireless
  virtual bool GetWirelessConfig(unsigned int nInstance, CHalWirelessConfig& wirelessDevice);
  virtual bool SetWirelessConfig(unsigned int nInstance, CHalWirelessConfig& wirelessDevice);
  virtual bool GetWirelessInfo(unsigned int nInstance, CHalWirelessInfo& wirelessDevice);
  virtual bool SearchWireless(unsigned int nInstance, std::vector<CHalWirelessNetwork>& wirelessNetworks);

  // clock
  virtual bool SetTimezone(const std::string& strTimezone);
  virtual bool GetTimezone(std::string& strTimezone);
  virtual bool SetTime(const std::string& strTime);

  // power
  virtual bool StandBy();
  virtual bool Shutdown();
  virtual bool Reboot();
  virtual bool Resume();

  // input
  virtual bool GetAllInputDevices(std::vector<CHalInputDevice>& inputDevices);

  // dvb
  virtual bool GetAllDvbDevices(std::vector<CHalDvbDevice>& dvbDevices);

  // storage
  virtual bool GetAllStorageDevices(std::vector<CHalStorageDeviceInfo>& storageDevicesInfo);
  virtual bool EnableSambaShares(const std::string password, const std::string username = "", const std::string workgroup = "", const std::string hostname = "");
  virtual bool GetSambaConfig(bool &status, std::string &password, std::string &workgroup, std::string &hostname);
  virtual bool DisableSambaShares();
  virtual bool EjectStorage(const std::string& path);

  // host
  virtual bool SetHostName(const std::string& strHostname);
  virtual bool GetHostName(std::string& strHostname);

  // led
  virtual bool SetLEDBrightness(int instance, int startBrightness, int endBrightness);
  virtual bool SetLEDState(bool isPlaying, bool isConnectedToInternet, bool isPowerSaving);

  // thermals
  virtual bool GetCPUTemperature(int& temperature);
  virtual bool GetCPUFanSpeed(int& speed);

  // system
  virtual bool GetHardwareInfo(CHalHardwareInfo& hardwareInfo);
  virtual bool GetSoftwareInfo(CHalSoftwareInfo& softwareInfo);
  virtual bool RequestUpgrade();
  virtual bool SetAudioDACState(bool mute);
  virtual bool AvahiDaemonRestart();
  virtual bool DjmountRestart();

  //vpn
  virtual bool VpnSetConfig(const CHALVpnConfig &vpnConfig);
  virtual bool VpnDial();
  virtual bool VpnHangup();
  virtual bool VpnGetInfo(CHALVpnInfo &vpnInfo);
  virtual bool VpnGetConfig(CHALVpnConfig &vpnConfig);


  // listener
  virtual void AddListener (IHalListener *listener) { }

private:
  CNullHalServices();
  CHalAddrType GetAddrType (const std::string &AddrTypeStr);

  std::string GetAddrTypeStr (CHalAddrType  addrType);
  CBoxeeHalClient* m_pRequest;


};

#endif

#endif /* NULLHALSERVICES_H_ */
