#pragma once

#include "StdString.h"

class CBoxeeOTAConfigurationData
{
public:
  CBoxeeOTAConfigurationData();
  ~CBoxeeOTAConfigurationData();

  CStdString GetCountryCode() { return m_strCountryCode; }
  CStdString GetZipCode()     { return m_strZipCode; }
  bool GetIsCable()         { return m_isCable; }

  bool IsInNorthAmerica() { return (m_strCountryCode == "US" || m_strCountryCode == "CA"); }

  void SetCountryCode(const CStdString& strCountryCode);
  void SetZipCode(const CStdString& strZipCode);
  void SetIsCable(bool isCable);

  void DetectLocation();
  void ClearData();
  void SaveData();

private:
  CStdString m_strCountryCode;
  CStdString m_strZipCode;
  bool m_isCable;
};
