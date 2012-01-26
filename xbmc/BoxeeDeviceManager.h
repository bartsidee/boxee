#pragma once

#include "StdString.h"
#include "utils/CriticalSection.h"
#include <map>
#include "tinyXML/tinyxml.h"

class CDeviceTypes
{
public:
  enum DeviceTypesEnums
  {
    BDT_UNKNOWN = 0,
    BDT_TABLET  = 1,
    BDT_PHONE   = 2,
    BDT_REMOTE  = 3,
    BDT_OTHER   = 4
  };
};

class CBoxeeDeviceItem
{
public:

  CBoxeeDeviceItem();
  virtual ~CBoxeeDeviceItem();

  bool FromXML(const TiXmlElement* deviceElement);
  bool ToXML(TiXmlElement& deviceElement);

  bool Initialize(const CStdString& deviceId, const CStdString& deviceLabel, const CStdString& deviceType, const CStdString& deviceIcon, const CStdString& applicationId);
  bool Reset();

  void SetDeviceId(const CStdString& deviceId);
  CStdString GetDeviceId() const;

  void SetDeviceLabel(const CStdString& deviceLabel);
  CStdString GetDeviceLabel() const;

  bool SetDeviceType(const CStdString& deviceType);
  CDeviceTypes::DeviceTypesEnums GetDeviceTypeAsEnum() const;
  CStdString GetDeviceTypeAsString() const;

  void SetDeviceIcon(const CStdString& deviceIcon);
  CStdString GetDeviceIcon() const;

  bool SetApplicationId(const CStdString& applicationId);
  CStdString GetApplicationId() const;

  bool IsInitialize();

protected:

  bool m_isInitialize;

  CStdString m_deviceId;
  CStdString m_deviceLabel;
  CDeviceTypes::DeviceTypesEnums m_deviceType;
  CStdString m_deviceIcon;
  CStdString m_applicationId;
};

class CBoxeeDeviceManager
{
public:
  CBoxeeDeviceManager();
  virtual ~CBoxeeDeviceManager();
  
  bool Initialize();

  bool PairDevice(const CBoxeeDeviceItem& deviceItem);
  bool UnPairDevice(const CStdString& deviceId);
  bool IsPairDevice(const CStdString& deviceId);

  const std::map<CStdString, CBoxeeDeviceItem>& GetPairedDeviceMap();

  static CStdString GetDeviceTypeAsString(CDeviceTypes::DeviceTypesEnums deviceTypeEnum);
  static CDeviceTypes::DeviceTypesEnums GetDeviceTypeAsEnum(CStdString deviceTypeStr);

protected:
  bool PairDeviceInternal(const CBoxeeDeviceItem& deviceItem);
  bool Save();
  std::map<CStdString, CBoxeeDeviceItem> m_pairedDeviceMap;
  CCriticalSection m_deviceMaplock;
};
