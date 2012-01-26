#include "BoxeeDeviceManager.h"
#include "log.h"
#include "utils/SingleLock.h"
#include "SpecialProtocol.h"
#include "File.h"

#define DEVICES_FILE "special://home/devices.xml"

#define DEVICE_TYPE_PHONE  "phone"
#define DEVICE_TYPE_TABLET "tablet"
#define DEVICE_TYPE_REMOTE "remote"
#define DEVICE_TYPE_OTHER  "other"

CBoxeeDeviceManager::CBoxeeDeviceManager()
{

}

CBoxeeDeviceManager::~CBoxeeDeviceManager()
{

}

bool CBoxeeDeviceManager::Initialize()
{
  m_pairedDeviceMap.clear();

  CStdString strPath = _P(DEVICES_FILE);

  if (!XFILE::CFile::Exists(strPath))
  {
    return true;
  }

  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(strPath))
  {
    CLog::Log(LOGERROR, "%s Error loading %s: Line %d, %s", __FUNCTION__, strPath.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (!pRootElement)
  {
    CLog::Log(LOGERROR, "%s devices.xml file does not have root element", __FUNCTION__);
    return false;
  }

  CStdString strValue = pRootElement->Value();
  if (strValue != "devices")
  {
    CLog::Log(LOGERROR, "%s devices.xml file does not contain <devices>", __FUNCTION__);
    return false;
  }

  const TiXmlElement* pChild = pRootElement->FirstChildElement("device");
  while (pChild)
  {
    CBoxeeDeviceItem device;
    if (device.FromXML(pChild))
    {
      PairDeviceInternal(device);
    }

    pChild = pChild->NextSiblingElement();
  }

  return true;
}

bool CBoxeeDeviceManager::PairDeviceInternal(const CBoxeeDeviceItem& deviceItem)
{
  CLog::Log(LOGDEBUG,"CBoxeeDeviceManager::PairDevice -  enter function with DeviceItem [DeviceId=%s][DeviceLabel=%s][DeviceType=%s=%d][DeviceIcon=%s][DeviceApplicationId=%s] (bdm)",deviceItem.GetDeviceId().c_str(),deviceItem.GetDeviceLabel().c_str(),deviceItem.GetDeviceTypeAsString().c_str(),deviceItem.GetDeviceTypeAsEnum(),deviceItem.GetDeviceIcon().c_str(),deviceItem.GetApplicationId().c_str());

  CSingleLock lock(m_deviceMaplock);

  // check if DeviceId already exist

  CStdString newDeviceId = deviceItem.GetDeviceId();

  std::map<CStdString, CBoxeeDeviceItem>::iterator it;
  it = m_pairedDeviceMap.find(newDeviceId);

  if(it != m_pairedDeviceMap.end())
  {
    CLog::Log(LOGDEBUG,"CBoxeeDeviceManager::PairDevice - FAILED to pair device with [DeviceId=%s]. Id already exist (bdm)",newDeviceId.c_str());
    return false;
  }

  m_pairedDeviceMap[newDeviceId] = deviceItem;

  return true;
}

bool CBoxeeDeviceManager::PairDevice(const CBoxeeDeviceItem& deviceItem)
{
  if (!PairDeviceInternal(deviceItem))
    return false;

  return Save();
}

bool CBoxeeDeviceManager::UnPairDevice(const CStdString& deviceId)
{
  CLog::Log(LOGDEBUG,"CBoxeeDeviceManager::UnPairDevice -  enter function with [DeviceId=%s] (bdm)",deviceId.c_str());

  CSingleLock lock(m_deviceMaplock);

  std::map<CStdString, CBoxeeDeviceItem>::iterator it;
  it = m_pairedDeviceMap.find(deviceId);

  if(it == m_pairedDeviceMap.end())
  {
    CLog::Log(LOGDEBUG,"CBoxeeDeviceManager::UnPairDevice - FAILED to unpair device with [DeviceId=%s]. Id DOESN'T exist (bdm)",deviceId.c_str());
    return false;
  }

  m_pairedDeviceMap.erase(it);

  return Save();
}

bool CBoxeeDeviceManager::IsPairDevice(const CStdString& deviceId)
{
  CSingleLock lock(m_deviceMaplock);

  if(m_pairedDeviceMap.find(deviceId) == m_pairedDeviceMap.end())
  {
    return false;
  }

  return true;
}

CStdString CBoxeeDeviceManager::GetDeviceTypeAsString(CDeviceTypes::DeviceTypesEnums deviceTypeEnum)
{
  switch(deviceTypeEnum)
  {
  case CDeviceTypes::BDT_PHONE:
    return DEVICE_TYPE_PHONE;
    break;
  case CDeviceTypes::BDT_TABLET:
    return DEVICE_TYPE_TABLET;
    break;
  case CDeviceTypes::BDT_REMOTE:
    return DEVICE_TYPE_REMOTE;
    break;
  case CDeviceTypes::BDT_OTHER:
    return DEVICE_TYPE_OTHER;
    break;
  default:
    CLog::Log(LOGDEBUG,"CBoxeeDeviceManager::GetDeviceTypeAsString - Invalid DeviceTypes enum [%d]. Going to return [UNKNOWN] as type (bdm)",deviceTypeEnum);
    return "UNKNOWN";
    break;
  }
}

CDeviceTypes::DeviceTypesEnums CBoxeeDeviceManager::GetDeviceTypeAsEnum(CStdString deviceTypeStr)
{
  CStdString deviceTypeStrLower = deviceTypeStr;
  deviceTypeStrLower.ToLower();

  if(deviceTypeStrLower == DEVICE_TYPE_PHONE)
  {
    return CDeviceTypes::BDT_PHONE;
  }
  else if(deviceTypeStrLower == DEVICE_TYPE_TABLET)
  {
    return CDeviceTypes::BDT_TABLET;
  }
  else if(deviceTypeStrLower == DEVICE_TYPE_REMOTE)
  {
    return CDeviceTypes::BDT_REMOTE;
  }
  else if(deviceTypeStrLower == DEVICE_TYPE_OTHER)
  {
    return CDeviceTypes::BDT_OTHER;
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeDeviceManager::GetDeviceTypeAsEnum - Failed to convert [deviceTypeStr=%s] to ENUM. Going to return [UNKNOWN] (bdm)",deviceTypeStr.c_str());
    return CDeviceTypes::BDT_UNKNOWN;
  }
}

const std::map<CStdString, CBoxeeDeviceItem>& CBoxeeDeviceManager::GetPairedDeviceMap()
{
  return m_pairedDeviceMap;
}

bool CBoxeeDeviceManager::Save()
{
  TiXmlDocument xmlDoc;
  TiXmlElement xmlRootElement("devices");
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;

  std::map<CStdString, CBoxeeDeviceItem>::iterator it;

  for (it = m_pairedDeviceMap.begin(); it != m_pairedDeviceMap.end(); ++it)
  {
    TiXmlElement deviceElement("device");
    it->second.ToXML(deviceElement);
    pRoot->InsertEndChild(deviceElement);
  }

  CStdString strPath = _P(DEVICES_FILE);
  return xmlDoc.SaveFile(strPath);
}

CBoxeeDeviceItem::CBoxeeDeviceItem()
{
  Reset();
}

CBoxeeDeviceItem::~CBoxeeDeviceItem()
{

}

bool CBoxeeDeviceItem::Initialize(const CStdString& deviceId, const CStdString& deviceLabel, const CStdString& deviceType, const CStdString& deviceIcon, const CStdString& applicationId)
{
  m_isInitialize = false;

  SetDeviceId(deviceId);
  SetDeviceLabel(deviceLabel);

  if (!SetDeviceType(deviceType))
  {
    CLog::Log(LOGERROR,"CBoxeeDeviceItem::initialize - FAILED to set DeviceType with unknown [DeviceType=%s] (bdm)",deviceType.c_str());
    return false;
  }

  SetDeviceIcon(deviceIcon);

  if (!SetApplicationId(applicationId))
  {
    CLog::Log(LOGERROR,"CBoxeeDeviceItem::initialize - FAILED to set ApplicationId with unknown [ApplicationId=%s] (bdm)",applicationId.c_str());
    return false;
  }

  m_isInitialize = true;

  return true;
}

bool CBoxeeDeviceItem::FromXML(const TiXmlElement* deviceElement)
{
  const char* id = deviceElement->Attribute("id");
  if (!id)
  {
    CLog::Log(LOGERROR,"CBoxeeDeviceItem::initialize - FAILED to get attr id (bdm)");
    return false;
  }

  const char* label = deviceElement->Attribute("label");
  if (!label)
  {
    CLog::Log(LOGERROR,"CBoxeeDeviceItem::initialize - FAILED to get attr label (bdm)");
    return false;
  }

  const char* type = deviceElement->Attribute("type");
  if (!type)
  {
    CLog::Log(LOGERROR,"CBoxeeDeviceItem::initialize - FAILED to get attr type (bdm)");
    return false;
  }

  const char* icon = deviceElement->Attribute("icon");
  if (!icon)
  {
    CLog::Log(LOGERROR,"CBoxeeDeviceItem::initialize - FAILED to get attr icon (bdm)");
    return false;
  }

  const char* applicationId = deviceElement->Attribute("application-id");
  if (!applicationId)
  {
    CLog::Log(LOGERROR,"CBoxeeDeviceItem::initialize - FAILED to get attr applicationId (bdm)");
    return false;
  }

  return Initialize(id, label, type, icon, applicationId);
}

bool CBoxeeDeviceItem::ToXML(TiXmlElement& deviceElement)
{
  deviceElement.SetAttribute("id", GetDeviceId().c_str());
  deviceElement.SetAttribute("label", GetDeviceLabel().c_str());
  deviceElement.SetAttribute("type", GetDeviceTypeAsString().c_str());
  deviceElement.SetAttribute("icon", GetDeviceIcon().c_str());
  deviceElement.SetAttribute("application-id", GetApplicationId().c_str());

  return true;
}

bool CBoxeeDeviceItem::Reset()
{
  m_deviceId = "";
  m_deviceLabel = "";
  m_deviceType = CDeviceTypes::BDT_UNKNOWN;
  m_deviceIcon = "";
  m_applicationId = "";

  m_isInitialize = false;

  return true;
}

bool CBoxeeDeviceItem::IsInitialize()
{
  return m_isInitialize;
}

void CBoxeeDeviceItem::SetDeviceId(const CStdString& deviceId)
{
  m_deviceId = deviceId;
}

CStdString CBoxeeDeviceItem::GetDeviceId() const
{
  return m_deviceId;
}

void CBoxeeDeviceItem::SetDeviceLabel(const CStdString& deviceLabel)
{
  m_deviceLabel = deviceLabel;
}

CStdString CBoxeeDeviceItem::GetDeviceLabel() const
{
  return m_deviceLabel;
}

bool CBoxeeDeviceItem::SetDeviceType(const CStdString& deviceType)
{
  m_deviceType = CBoxeeDeviceManager::GetDeviceTypeAsEnum(deviceType);

  if (m_deviceType == CDeviceTypes::BDT_UNKNOWN)
  {
    return false;
  }

  return true;
}

CDeviceTypes::DeviceTypesEnums CBoxeeDeviceItem::GetDeviceTypeAsEnum() const
{
  return m_deviceType;
}

CStdString CBoxeeDeviceItem::GetDeviceTypeAsString() const
{
  return CBoxeeDeviceManager::GetDeviceTypeAsString(m_deviceType);
}

void CBoxeeDeviceItem::SetDeviceIcon(const CStdString& deviceIcon)
{
  m_deviceIcon = deviceIcon;
}

CStdString CBoxeeDeviceItem::GetDeviceIcon() const
{
  return m_deviceIcon;
}

bool CBoxeeDeviceItem::SetApplicationId(const CStdString& applicationId)
{
  m_applicationId = applicationId;
  return true;
}

CStdString CBoxeeDeviceItem::GetApplicationId() const
{
  return m_applicationId;
}

