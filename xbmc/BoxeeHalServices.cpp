#include "system.h"

#ifdef HAS_BOXEE_HAL

#include "BoxeeHalServices.h"
#include "json/reader.h"
#include "log.h"

CBoxeeHalServices::CBoxeeHalServices()
{
  m_pRequest = new CBoxeeHalClient();
  m_pRequest->AddListener(this);
};

bool CBoxeeHalServices::GetJson(const std::string &strResponse, Json::Value &rootValue)
{
  Json::Reader reader;

  if (!reader.parse(strResponse, rootValue, false))
  {
#ifndef BOXEE_STANDALONE_HAL_CLIENT
    CLog::Log(LOGERROR, "Error parsing json: %s", reader.getFormatedErrorMessages().c_str());
#endif
    return false;
  }

  return true;
}

bool CBoxeeHalServices::GetEthernetConfig(unsigned int nInstance, CHalEthernetConfig &ethernetConfig)
{
  StringMap params;
  std::string strRequest, strResponse;

  params.AddIntParameter("instance", nInstance);
  if (!m_pRequest->SendRequest("ethernet", "GetConfig", &params, strResponse))
    return false;

  Json::Reader reader;
  Json::Value rootValue;

  if (!reader.parse(strResponse, rootValue, false))
  {
#ifndef BOXEE_STANDALONE_HAL_CLIENT
    CLog::Log(LOGERROR, "Error parsing json: %s", reader.getFormatedErrorMessages().c_str());
#endif
    return false;
  }

  ethernetConfig.addr_type = GetAddrType(rootValue["config"].asString());

  if (ethernetConfig.addr_type != ADDR_STATIC)
    return true;

  ethernetConfig.ip_address = rootValue["ipAddress"].asString();
  ethernetConfig.netmask = rootValue["netmask"].asString();
  ethernetConfig.gateway = rootValue["gateway"].asString();
  ethernetConfig.dns = rootValue["dns"].asString();
  return true;
}

bool CBoxeeHalServices::SetEthernetConfig(unsigned int nInstance, const CHalEthernetConfig &ethernetConfig)
{
  StringMap params;
  std::string strRequest, strResponse;

  params.AddIntParameter("instance", nInstance);
  params["config"] = GetAddrTypeStr(ethernetConfig.addr_type);

  if(ethernetConfig.addr_type == ADDR_STATIC)
  {
    params["ipAddress"] = ethernetConfig.ip_address;
    params["netmask"  ] = ethernetConfig.netmask;
    params["gateway"] = ethernetConfig.gateway;
    params["dns"] = ethernetConfig.dns;
  }

  std::string response;
  return m_pRequest->SendRequest("ethernet", "SetConfig", &params, response);
}

bool CBoxeeHalServices::EthernetInfoMapTranslate (const Json::Value &rootValue, CHalEthernetInfo &ethernetInfo)
{
  ethernetInfo.addr_type = GetAddrType(rootValue["config"].asString());
  ethernetInfo.running= (rootValue["running"].asString() == "true");
  ethernetInfo.link_up = (rootValue["link-up"].asString() == "true");
  ethernetInfo.ip_address = rootValue["ipAddress"].asString();
  ethernetInfo.netmask = rootValue["netmask"].asString();
  ethernetInfo.gateway = rootValue["gateway"].asString();
  ethernetInfo.dns = rootValue["dns"].asString();
  ethernetInfo.mac_address = rootValue["macAddress"].asString();
  return true;
}

bool CBoxeeHalServices::GetEthernetInfo(unsigned int nInstance, CHalEthernetInfo &ethernetInfo)
{

  StringMap params;
  std::string strRequest, strResponse;

  params.AddIntParameter("instance", nInstance);
  if (!m_pRequest->SendRequest("ethernet", "GetInfo", &params, strResponse))
    return false;
  Json::Reader reader;
  Json::Value rootValue;

  if (!reader.parse(strResponse, rootValue, false))
  {
#ifndef BOXEE_STANDALONE_HAL_CLIENT
    CLog::Log(LOGERROR, "Error parsing json: %s", reader.getFormatedErrorMessages().c_str());
#endif
    return false;
  }

  return EthernetInfoMapTranslate(rootValue, ethernetInfo);
}

// wireless
bool CBoxeeHalServices::GetWirelessConfig(unsigned int nInstance, CHalWirelessConfig& wirelessConfig)
{
  StringMap params;
  std::string strRequest, strResponse;

  params.AddIntParameter("instance", nInstance);
  if (!m_pRequest->SendRequest("wireless", "GetConfig", &params, strResponse))
    return false;

  StringMap wirelessConfigMap;

  Json::Reader reader;
  Json::Value rootValue;

  if (!reader.parse(strResponse, rootValue, false))
  {
#ifndef BOXEE_STANDALONE_HAL_CLIENT
    CLog::Log(LOGERROR, "Error parsing json: %s", reader.getFormatedErrorMessages().c_str());
#endif
    return false;
  }

  wirelessConfig.addr_type = GetAddrType(rootValue["config"].asString());
  wirelessConfig.ssid = rootValue["ssid"].asString();
  wirelessConfig.password = rootValue["key"].asString();

  if (rootValue["auth"].asString() == "none")
    wirelessConfig.authType = AUTH_NONE;
  if (rootValue["auth"].asString() == "dontcare")
    wirelessConfig.authType = AUTH_DONTCARE;
  if (rootValue["auth"].asString() == "wep-key")
    wirelessConfig.authType = AUTH_WEP_KEY;
  if (rootValue["auth"].asString() == "wep-passphrase")
    wirelessConfig.authType = AUTH_WEP_PASSPHRASE;
  if (rootValue["auth"].asString() == "wpa-psk")
    wirelessConfig.authType = AUTH_WPAPSK;
  if (rootValue["auth"].asString() == "wpa2-psk")
    wirelessConfig.authType = AUTH_WPA2PSK;

  if (wirelessConfig.addr_type != ADDR_STATIC)
    return true;

  wirelessConfig.ip_address = rootValue["ipAddress"].asString();
  wirelessConfig.netmask = rootValue["netmask"].asString();
  wirelessConfig.gateway = rootValue["gateway"].asString();
  wirelessConfig.dns = rootValue["dns"].asString();

  return true;
}

std::string CBoxeeHalServices::GetAddrTypeStr (CHalAddrType  AddrType)
{
  if (AddrType == ADDR_STATIC)
    return "static";
  else if (AddrType == ADDR_DYNAMIC)
    return "dhcp";
  else
    return "none";
}

CHalAddrType CBoxeeHalServices::GetAddrType (const std::string &AddrTypeStr)
{
  if (AddrTypeStr == "static")
    return ADDR_STATIC;
  else if (AddrTypeStr == "dhcp")
    return ADDR_DYNAMIC;
  else
    return ADDR_NONE;
}

bool CBoxeeHalServices::SetWirelessConfig(unsigned int nInstance, CHalWirelessConfig& wirelessConfig)
{
  StringMap params;
  std::string strRequest, strResponse;

  params.AddIntParameter("instance", nInstance);
  params["config"] = GetAddrTypeStr(wirelessConfig.addr_type);
  if (wirelessConfig.addr_type != ADDR_NONE)
  {
    params["ssid"] = wirelessConfig.ssid;
    params["key"] = wirelessConfig.password;
    switch (wirelessConfig.authType)
    {
    case AUTH_NONE:
      params["auth"] = "none";
      break;
    case AUTH_DONTCARE:
      params["auth"] = "dontcare";
      break;
    case AUTH_WEP_KEY:
      params["auth"] = "wep-key";
      break;
    case AUTH_WEP_PASSPHRASE:
      params["auth"] = "wep-passphrase";
      break;
    case AUTH_WPAPSK:
      params["auth"] = "wpa-psk";
      break;
    case AUTH_WPA2PSK:
      params["auth"] = "wpa2-psk";
      break;
    default:
      return false;
    }

    if (wirelessConfig.addr_type == ADDR_STATIC)
    {
      params["ipAddress"] = wirelessConfig.ip_address;
      params["netmask"  ] = wirelessConfig.netmask;
      params["gateway"] = wirelessConfig.gateway;
      params["dns"] = wirelessConfig.dns;
    }
  }

  return m_pRequest->SendRequest("wireless", "SetConfig", &params , strResponse);
}

bool CBoxeeHalServices::WirelessInfoMapTranslate (const Json::Value &rootValue,  CHalWirelessInfo &wirelessInfo)
{
  wirelessInfo.ssid = rootValue["ssid"].asString();
  wirelessInfo.mac_address = rootValue["macAddress"].asString();
  wirelessInfo.connected = (rootValue["associated"].asString() == "true");
  wirelessInfo.addr_type = GetAddrType(rootValue["config"].asString());
  wirelessInfo.ip_address = rootValue["ipAddress"].asString();
  wirelessInfo.netmask = rootValue["netmask"].asString();
  wirelessInfo.gateway = rootValue["gateway"].asString();
  wirelessInfo.dns = rootValue["dns"].asString();
  return true;
}

bool CBoxeeHalServices::GetWirelessInfo(unsigned int nInstance, CHalWirelessInfo& wirelessInfo)
{
  StringMap params;
  std::string strRequest, strResponse;

  params.AddIntParameter("instance", nInstance);
  if (!m_pRequest->SendRequest("wireless", "GetInfo", &params ,strResponse))
    return false;
  Json::Reader reader;
  Json::Value rootValue;

  if (!reader.parse(strResponse, rootValue, false))
  {
#ifndef BOXEE_STANDALONE_HAL_CLIENT
    CLog::Log(LOGERROR, "Error parsing json: %s", reader.getFormatedErrorMessages().c_str());
#endif
    return false;
  }

  return WirelessInfoMapTranslate(rootValue, wirelessInfo);
}

bool CBoxeeHalServices::SearchWireless(unsigned int nInstance, std::vector<CHalWirelessNetwork>& vecWirelessNetworks)
{
  StringMap params;
  std::string strRequest, strResponse;

  params.AddIntParameter("instance", nInstance);
  if (!m_pRequest->SendRequest("wireless", "Scan", &params ,strResponse))
    return false;

  Json::Reader reader;
  Json::Value rootValue;

  if (!reader.parse(strResponse, rootValue, false))
  {
#ifndef BOXEE_STANDALONE_HAL_CLIENT
    CLog::Log(LOGERROR, "Error parsing json: %s", reader.getFormatedErrorMessages().c_str());
#endif
    return false;
  }

  if (!rootValue["networks"].isArray())
  {
#ifndef BOXEE_STANDALONE_HAL_CLIENT
    CLog::Log(LOGERROR, "Error parsing json: %s", reader.getFormatedErrorMessages().c_str());
#endif
    return false;
  }

  std::vector<CHalWirelessNetwork> myVecWirelessNetworks;

  for (size_t i = 0; i < rootValue["networks"].size(); i++)
  {
    CHalWirelessNetwork wNetwork;
    wNetwork.ssid =  rootValue["networks"][i]["ssid"].asString();
    wNetwork.secure =  rootValue["networks"][i]["secure"].asString() == "true"? true : false;
    wNetwork.signal_strength =  rootValue["networks"][i]["signal"].asInt();
    myVecWirelessNetworks.push_back(wNetwork);
    CLog::Log(LOGDEBUG,"CBoxeeHalServices::SearchWireless - [%d/%d] - going to add [label=%s][secure=%d][signal=%lu] (ap)",i+1,rootValue["networks"].size(),wNetwork.ssid.c_str(),wNetwork.secure,wNetwork.signal_strength);
  }

  vecWirelessNetworks = myVecWirelessNetworks;

  return true;
}

 // clock
bool CBoxeeHalServices::SetTimezone(const std::string& strTimezone)
{
  StringMap params;
  std::string strRequest, strResponse;

  params["timezone"] = strTimezone;
  return m_pRequest->SendRequest("clock", "SetTimezone", &params, strResponse);
}

bool CBoxeeHalServices::GetTimezone(std::string& strTimezone)
{
  StringMap params, storageShareMap;
  std::string strRequest, strResponse;

  if (!m_pRequest->SendRequest("clock", "GetTimeZone", &params, strResponse))
    return false;

  Json::Value rootValue;
  if (!GetJson(strResponse, rootValue))
    return false;

  strTimezone = rootValue["timezone"].asString();

  return true;
}

bool CBoxeeHalServices::SetTime(const std::string& strTime)
{
  StringMap params;
  std::string strRequest, strResponse;

  params["time"] = strTime;
  return m_pRequest->SendRequest("clock", "SetTime", &params, strResponse);
}

// power
bool CBoxeeHalServices::StandBy()
{
  std::string strRequest, strResponse;

  return m_pRequest->SendRequest("power", "Standby", NULL, strResponse);
}

bool CBoxeeHalServices::Resume()
{
  std::string strRequest, strResponse;

  return m_pRequest->SendRequest("power", "Resume", NULL, strResponse);
}

bool CBoxeeHalServices::Shutdown()
{
  std::string strRequest, strResponse;

  return m_pRequest->SendRequest("power", "Shutdown", NULL, strResponse);
}

bool CBoxeeHalServices::Reboot()
{
  std::string strRequest, strResponse;

  return m_pRequest->SendRequest("power", "Reboot", NULL, strResponse);
}

// input
bool CBoxeeHalServices::GetAllInputDevices(std::vector<CHalInputDevice>& inputDevices)
{
  std::string strResponse;

  if (!m_pRequest->SendRequest("input", "GetAllDevices", NULL, strResponse))
    return false;

  Json::Reader reader;
  Json::Value rootValue;

  if (!reader.parse(strResponse, rootValue, false))
  {
#ifndef BOXEE_STANDALONE_HAL_CLIENT
    CLog::Log(LOGERROR, "Error parsing json: %s", reader.getFormatedErrorMessages().c_str());
#endif
    return false;
  }

  if (!rootValue["devices"].isArray())
  {
#ifndef BOXEE_STANDALONE_HAL_CLIENT
    CLog::Log(LOGERROR, "Error parsing json: %s", reader.getFormatedErrorMessages().c_str());
#endif
    return false;
  }

  inputDevices.clear();

  for (size_t i = 0; i < rootValue["devices"].size(); i++)
  {
    CHalInputDevice inputDevice;

    inputDevice.instance = rootValue["devices"][i]["instance"].asInt();
    inputDevice.path = rootValue["devices"][i]["path"].asString();
    inputDevice.label = rootValue["devices"][i]["label"].asString();
    inputDevices.push_back(inputDevice);
  }

  return true;
}

// dvb
bool CBoxeeHalServices::GetAllDvbDevices(std::vector<CHalDvbDevice>& dvbDevices)
{
  std::string strResponse;

  if (!m_pRequest->SendRequest("dvb", "GetAllDevices", NULL, strResponse))
    return false;

  Json::Reader reader;
  Json::Value rootValue;

  if (!reader.parse(strResponse, rootValue, false))
  {
#ifndef BOXEE_STANDALONE_HAL_CLIENT
    CLog::Log(LOGERROR, "Error parsing json: %s", reader.getFormatedErrorMessages().c_str());
#endif
    return false;
  }

  if (!rootValue["devices"].isArray())
  {
#ifndef BOXEE_STANDALONE_HAL_CLIENT
    CLog::Log(LOGERROR, "Error parsing json: %s", reader.getFormatedErrorMessages().c_str());
#endif
    return false;
  }

  dvbDevices.clear();

  for (size_t i = 0; i < rootValue["devices"].size(); i++)
  {
    CHalDvbDevice dvbDevice;

    dvbDevice.instance = rootValue["devices"][i]["instance"].asInt();
    dvbDevice.path = rootValue["devices"][i]["path"].asString();
    dvbDevice.ready = rootValue["devices"][i]["ready"].asBool();
    dvbDevice.connected = rootValue["devices"][i]["connected"].asBool();

    dvbDevices.push_back(dvbDevice);
  }

  return true;
}
// storage
bool CBoxeeHalServices::GetAllStorageDevices(std::vector <CHalStorageDeviceInfo>& storageDevices)
{
  std::string strRequest, strResponse;
  StringMapVector devicesMap ;

  if (!m_pRequest->SendRequest("storage", "GetAllDevices", NULL, strResponse))
    return false;

#ifndef BOXEE_STANDALONE_HAL_CLIENT
  CLog::Log(LOGINFO, "reply for storage.GetAllDevices - %s\n", strResponse.c_str());
#endif
  Json::Reader reader;
  Json::Value rootValue;

  if (!reader.parse(strResponse, rootValue, false))
  {
#ifndef BOXEE_STANDALONE_HAL_CLIENT
    CLog::Log(LOGERROR, "Error parsing json: %s", reader.getFormatedErrorMessages().c_str());
#endif
    return false;
  }

  if (!rootValue["devices"].isArray())
  {
#ifndef BOXEE_STANDALONE_HAL_CLIENT
    CLog::Log(LOGERROR, "Error parsing json: %s", reader.getFormatedErrorMessages().c_str());
#endif
    return false;
  }

  storageDevices.clear();

  for (size_t i = 0; i < rootValue["devices"].size(); i++)
  {
    CHalStorageDeviceInfo storageDevice;

    storageDevice.fs_type = rootValue["devices"][i]["fstype"].asString();
    storageDevice.path = rootValue["devices"][i]["path"].asString();
    storageDevice.label = rootValue["devices"][i]["label"].asString();
    storageDevices.push_back(storageDevice);
  }

  return true;
}

bool CBoxeeHalServices::EnableSambaShares(const std::string password, const std::string username, const std::string workgroup, const std::string hostname)
{
  StringMap params;
  std::string strRequest, strResponse;

  params["password"] = password;

  if(!username.empty())
    params["username" ] = username;

  if(!workgroup.empty())
    params["workgroup" ] = workgroup;

  if(!hostname.empty())
    params["hostname" ] = hostname;

  return m_pRequest->SendRequest("storage", "EnableSambaShares", &params, strResponse);
}

bool CBoxeeHalServices::DisableSambaShares()
{
  StringMap params, storageShareMap;
  std::string strResponse;

  return m_pRequest->SendRequest("storage", "DisableSambaShares", &params, strResponse);
}

bool CBoxeeHalServices::GetSambaConfig(bool &status, std::string &password, std::string &workgroup, std::string &hostname)
{
  StringMap params, storageShareMap;
  std::string strRequest, strResponse;

  if (!m_pRequest->SendRequest("storage", "GetSambaConfig", &params, strResponse))
    return false;

  Json::Value rootValue;
  if (!GetJson(strResponse, rootValue))
    return false;

  password = rootValue["password"].asString();
  workgroup = rootValue["workgroup"].asString();
  hostname= rootValue["hostname"].asString();
  status = rootValue["status"].asUInt();

  return true;
}

bool CBoxeeHalServices::EjectStorage(const std::string& path)
{
  StringMap params;
  std::string strRequest, strResponse;

  params["path"] = path;
  return m_pRequest->SendRequest("storage", "Eject", &params, strResponse);
}

// host
bool CBoxeeHalServices::SetHostName(const std::string& strHostname)
{
  StringMap params;
  std::string strRequest, strResponse;

  params["hostname"] = strHostname;
  return m_pRequest->SendRequest("host", "SetHostName", &params, strResponse);
}

bool CBoxeeHalServices::GetHostName(std::string& strHostname)
{
  StringMap params, storageShareMap;
  std::string strRequest, strResponse;

  if (!m_pRequest->SendRequest("host", "GetHostName", &params, strResponse))
    return false;

  Json::Value rootValue;
  if (!GetJson(strResponse, rootValue))
    return false;

  strHostname = rootValue["hostname"].asString();

  return true;
}

bool CBoxeeHalServices::SetLEDState(bool isPlaying, bool isConnectedToInternet, bool isPowerSaving)
{
  StringMap params;
  std::string strRequest, strResponse;

  params.AddIntParameter("playing", isPlaying);
  params.AddIntParameter("connected", isConnectedToInternet);
  params.AddIntParameter("powersaving", isPowerSaving);

  return m_pRequest->SendRequest("led", "SetState", &params, strResponse);
}

// led
bool CBoxeeHalServices::SetLEDBrightness(int instance, int startBrightness, int endBrightness)
{
  StringMap params;
  std::string strRequest, strResponse;

  params.AddIntParameter("instance", instance);
  params.AddIntParameter("startBrightness", startBrightness);
  params.AddIntParameter("endBrightness", endBrightness);

  return m_pRequest->SendRequest("led", "SetBrightness", &params, strResponse);
}

// thermals
bool CBoxeeHalServices::GetCPUTemperature(int& temperature)
{
  StringMap params, storageShareMap;
  std::string strRequest, strResponse;

  if (!m_pRequest->SendRequest("system", "GetCPUTemperature", &params, strResponse))
    return false;

  Json::Value rootValue;
  if (!GetJson(strResponse, rootValue))
    return false;

  temperature = rootValue["temperature"].asInt();

  return true;
}

bool CBoxeeHalServices::GetCPUFanSpeed(int& speed)
{
  StringMap params, storageShareMap;
  std::string strRequest, strResponse;

  if (!m_pRequest->SendRequest("system", "GetCPUFanSpeed", &params, strResponse))
    return false;

  Json::Value rootValue;
  if (!GetJson(strResponse, rootValue))
    return false;

  speed = rootValue["speed"].asInt();

  return true;
}

CBoxeeHalServices& CBoxeeHalServices::GetInstance()
{
  static CBoxeeHalServices halServices;

  return halServices;
}

bool CBoxeeHalServices::Notify(const std::string& notificationStr)
{
  Json::Reader reader;
  Json::Value rootValue;

  if (!reader.parse(notificationStr, rootValue, false))
  {
#ifndef BOXEE_STANDALONE_HAL_CLIENT
    CLog::Log(LOGERROR, "Error parsing json: %s", reader.getFormatedErrorMessages().c_str());
#endif
    return false;
  }

  NOTIFICATION_TYPE type = NO_NOTIFICATION;
  CLog::Log(LOGINFO, "notification string '%s'\n", notificationStr.c_str());
  if (rootValue["class"] == "ethernet" && (rootValue["notification"] == "LinkUp" || rootValue["notification"] == "LinkDown"))
    return NotifyEthernetLinkState (rootValue);
  if (rootValue["class"] == "ethernet" && rootValue["notification"] == "AddressChange")
    return NotifyEthernetAddress (rootValue);
  if (rootValue["class"] == "wireless" && (rootValue["notification"] == "LinkUp" || rootValue["notification"] == "LinkDown"))
    return NotifyWirelessLinkState (rootValue);
  if (rootValue["class"] == "wireless" && rootValue["notification"] == "AddressChange")
    return NotifyWirelessAddress (rootValue);
  else if (rootValue["class"] == "power")
    type = NOTIFY_POWER;
  else if (rootValue["class"] == "input")
  {
    std::string label = rootValue["label"].asString();
    std::string path = rootValue["path"].asString();
    CHalInputNotification halInputNotification(rootValue["notification"].asString() == "Connect",
        atoi(rootValue["instance"].asString().c_str()), label, path);

    for (size_t i = 0; i < m_halListeners.size(); i++)
      m_halListeners[i]->HandleNotification(halInputNotification);
  }
  else if (rootValue["class"] == "storage")
  {
    std::string path = rootValue["path"].asString();
    bool successfully = true;
    if (!rootValue["successfully"].isNull())
      successfully = rootValue["successfully"].asUInt();
    CHalStorageNotification halStorageNotification(rootValue["notification"].asString() == "Mount", path, successfully);
    for (size_t i = 0; i < m_halListeners.size(); i++)
      m_halListeners[i]->HandleNotification(halStorageNotification);
  }
  else if (rootValue["class"] == "vpn")
  {
    CHalVpnNotification halVpnNotification(rootValue["status"].asInt());
    for (size_t i = 0; i < m_halListeners.size(); i++)
      m_halListeners[i]->HandleNotification(halVpnNotification);
  }
  else if (rootValue["class"] == "dvb")
  {
    CHalDvbNotification halDvbNotification(
          rootValue["notification"].asString() == "Connect" || rootValue["notification"].asString() == "Ready",
          rootValue["notification"].asString() == "Ready",
          rootValue["adapter"].asInt(), rootValue["path"].asString());
    for (size_t i = 0; i < m_halListeners.size(); i++)
      m_halListeners[i]->HandleNotification(halDvbNotification);
  }

  return true;
}

bool CBoxeeHalServices::NotifyEthernetLinkState(const Json::Value &rootValue)
{
  bool linkUp = rootValue["notification"].asString() == "LinkUp" ? true : false;
  CHalEthernetLinkStateNotification eNotify(linkUp);
  for (size_t i = 0; i < m_halListeners.size(); i++)
    m_halListeners[i]->HandleNotification(eNotify);
  return true;
}

bool CBoxeeHalServices::NotifyEthernetAddress(const Json::Value &rootValue)
{
  CHalEthernetInfo ethernetInfo;
//  EthernetInfoMapTranslate (map, ethernetInfo);
  CHalEthernetAddressNotification eNotify(ethernetInfo);
  for (size_t i = 0; i < m_halListeners.size(); i++)
    m_halListeners[i]->HandleNotification(eNotify);
  return true;
}

bool CBoxeeHalServices::NotifyWirelessLinkState(const Json::Value &rootValue)
{
  bool linkUp = rootValue["notification"].asString() == "LinkUp" ? true : false;
  CHalEthernetLinkStateNotification wNotify(linkUp);
  for (size_t i = 0; i < m_halListeners.size(); i++)
    m_halListeners[i]->HandleNotification(wNotify);
  return true;
}

bool CBoxeeHalServices::NotifyWirelessAddress(const Json::Value &rootValue)
{
  CHalWirelessInfo wirelessInfo;
  WirelessInfoMapTranslate (rootValue, wirelessInfo);
  CHalWirelessAddressNotification wNotify(wirelessInfo);
  for (size_t i = 0; i < m_halListeners.size(); i++)
    m_halListeners[i]->HandleNotification(wNotify);
  return true;
}

// system
bool CBoxeeHalServices::GetHardwareInfo(CHalHardwareInfo& hardwareInfo)
{
  StringMap params, storageShareMap;
  std::string strRequest, strResponse;

  if (!m_pRequest->SendRequest("system", "GetHardwareInfo", &params, strResponse))
    return false;

  Json::Value rootValue;
  if (!GetJson(strResponse, rootValue))
    return false;

  hardwareInfo.model = rootValue["model"].asString();
  hardwareInfo.vendor = rootValue["vendor"].asString();
  hardwareInfo.revision = rootValue["revision"].asString();
  hardwareInfo.serialNumber = rootValue["serialNumber"].asString();
  hardwareInfo.uniqueId = rootValue["uniqueId"].asString();

  return true;
}

bool CBoxeeHalServices::GetSoftwareInfo(CHalSoftwareInfo& softwareInfo)
{
  StringMap params, storageShareMap;
  std::string strRequest, strResponse;

  if (!m_pRequest->SendRequest("system", "GetSoftwareInfo", &params, strResponse))
    return false;

  Json::Value rootValue;
  if (!GetJson(strResponse, rootValue))
    return false;

  softwareInfo.version = rootValue["version"].asString();
  softwareInfo.regionSKU = rootValue["regionSKU"].asString();
  softwareInfo.language = rootValue["language"].asString();
  if (rootValue["compositeVideo"].isNull())
  {
    softwareInfo.compositeVideo = "none";
  }
  else
  {
    softwareInfo.compositeVideo = rootValue["compositeVideo"].asString();
  }

  return true;
}

bool CBoxeeHalServices::RequestUpgrade()
{
  StringMap params;
  std::string strRequest, strResponse;

  return m_pRequest->SendRequest("system", "RequestUpgrade", &params, strResponse);
}

bool CBoxeeHalServices::SetAudioDACState(bool mute)
{
  StringMap params;
  std::string strRequest, strResponse;

  params["mute"] = mute ? "true" : "false";
  return m_pRequest->SendRequest("system", "SetAudioDACState", &params, strResponse);
}

bool CBoxeeHalServices::AvahiDaemonRestart()
{
  StringMap params;
  std::string strRequest, strResponse;

  return m_pRequest->SendRequest("system", "AvahiDaemonRestart", &params, strResponse);
}

bool CBoxeeHalServices::DjmountRestart()
{
  StringMap params;
  std::string strRequest, strResponse;

  return m_pRequest->SendRequest("system", "DjmountRestart", &params, strResponse);
}

//vpn
bool CBoxeeHalServices::VpnSetConfig(const CHALVpnConfig &vpnConfig)
{
  StringMap params;
  std::string strRequest, strResponse;

  params["host"] = vpnConfig.host;
  params["username"] = vpnConfig.username;
  params["password"] = vpnConfig.password;
  if (vpnConfig.encType == CHalVpnMppeAuto)
    params["encryption"] = "auto";
  else if (vpnConfig.encType == CHalVpnMppeForce)
    params["encryption"] = "force";
  return m_pRequest->SendRequest("vpn", "Config", &params, strResponse);
}

bool CBoxeeHalServices::VpnDial()
{
  StringMap params;
  std::string strRequest, strResponse;
  if (!m_pRequest->SendRequest("vpn", "Dial", &params, strResponse))
  {
    CLog::Log(LOGERROR, "CBoxeeHalServices::VpnDial failed to send request");
    return false;
  }

  Json::Reader reader;
  Json::Value rootValue;

  if (!reader.parse(strResponse, rootValue, false))
  {
  #ifndef BOXEE_STANDALONE_HAL_CLIENT
    CLog::Log(LOGERROR, "Error parsing json: %s", reader.getFormatedErrorMessages().c_str());
  #endif
    return false;
  }

  if (!rootValue["errorCode"].empty() &&
      rootValue["errorCode"].asString() != "0") {
    CLog::Log(LOGERROR, "CBoxeeHalServices::VpnDial failed with error code %s", rootValue["errorCode"].asString().c_str());
    return false;
  }
  CLog::Log(LOGDEBUG, "CBoxeeHalServices::VpnDial succeeded");
  return true;


}

bool CBoxeeHalServices::VpnHangup()
{
  StringMap params;
  std::string strRequest, strResponse;
  if (!m_pRequest->SendRequest("vpn", "Hangup", &params, strResponse))
  {
    CLog::Log(LOGERROR, "CBoxeeHalServices::VpnHangup failed to send request");
    return false;
  }

  Json::Reader reader;
  Json::Value rootValue;

  if (!reader.parse(strResponse, rootValue, false))
  {
  #ifndef BOXEE_STANDALONE_HAL_CLIENT
    CLog::Log(LOGERROR, "Error parsing json: %s", reader.getFormatedErrorMessages().c_str());
  #endif
    return false;
  }

  if (!rootValue["errorCode"].empty() &&
      rootValue["errorCode"].asString() != "0") {
    CLog::Log(LOGERROR, "CBoxeeHalServices::VpnHangup failed with error code %s", rootValue["errorCode"].asString().c_str());
    return false;
  }
  CLog::Log(LOGDEBUG, "CBoxeeHalServices::VpnHangup succeeded");
  return true;
}

bool CBoxeeHalServices::VpnGetInfo(CHALVpnInfo &vpnInfo)
{
  StringMap params;
  std::string strRequest, strResponse;
  if (!m_pRequest->SendRequest("vpn", "GetInfo", &params, strResponse))
    return false;

  Json::Reader reader;
  Json::Value rootValue;

  if (!reader.parse(strResponse, rootValue, false))
  {
  #ifndef BOXEE_STANDALONE_HAL_CLIENT
    CLog::Log(LOGERROR, "Error parsing json: %s", reader.getFormatedErrorMessages().c_str());
  #endif
    return false;
  }

  vpnInfo.host = rootValue["host"].asString();
  vpnInfo.username = rootValue["username"].asString();
  vpnInfo.password = rootValue["password"].asString();
  vpnInfo.encType = (rootValue["encryption"].asString() == "force"? CHalVpnMppeForce : CHalVpnMppeAuto);
  vpnInfo.tunnelType = CHalVpnPPTP;
  if (rootValue["vpnStatus"].asString() == "Connected")
    vpnInfo.status = CHalVpnConnected;
  else if (rootValue["vpnStatus"].asString() == "Disconnected")
    vpnInfo.status = CHalVpnDisconnected;
  else if (rootValue["vpnStatus"].asString() == "Disabled")
    vpnInfo.status = CHalVpnDisabled;
  else
    return false;

  return true;
}

bool CBoxeeHalServices::VpnGetConfig(CHALVpnConfig &vpnConfig)
{
  return true;
}

#endif
