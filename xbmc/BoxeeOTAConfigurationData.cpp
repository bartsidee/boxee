#include "BoxeeOTAConfigurationData.h"
#include "GUISettings.h"
#include "lib/libBoxee/bxcurrentlocation.h"

CBoxeeOTAConfigurationData::CBoxeeOTAConfigurationData()
{
  m_isCable = g_guiSettings.GetBool("ota.selectedcable");
  m_strCountryCode = g_guiSettings.GetString("ota.countrycode");
  m_strZipCode = g_guiSettings.GetString("ota.zipcode");
}

CBoxeeOTAConfigurationData::~CBoxeeOTAConfigurationData()
{
}

void CBoxeeOTAConfigurationData::ClearData()
{
  m_strCountryCode = "";
  m_strZipCode = "";
  m_isCable = false;
}

void CBoxeeOTAConfigurationData::DetectLocation()
{
  BOXEE::BXCurrentLocation& location = BOXEE::BXCurrentLocation::GetInstance();

  if (location.IsLocationDetected())
  {
    m_strCountryCode = location.GetCountryCode();
    m_strZipCode = location.GetPostalCode();
  }
}

void CBoxeeOTAConfigurationData::SaveData()
{
  g_guiSettings.SetBool("ota.selectedcable", m_isCable);
  g_guiSettings.SetString("ota.countrycode", m_strCountryCode);
  g_guiSettings.SetString("ota.zipcode", m_strZipCode);
}

void  CBoxeeOTAConfigurationData::SetCountryCode(const CStdString& strCountryCode)
{
  m_strCountryCode = strCountryCode;
}

void  CBoxeeOTAConfigurationData::SetZipCode(const CStdString& strZipCode)
{
  m_strZipCode = strZipCode;
}

void CBoxeeOTAConfigurationData::SetIsCable(bool isCable)
{
  m_isCable = isCable;
}
