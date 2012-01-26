#include <json/json.h>

#include "bxcurrentlocation.h"
#include "bxconfiguration.h"
#include "bxutils.h"

BOXEE::BXCurrentLocation& BOXEE::BXCurrentLocation::GetInstance()
{
  static BOXEE::BXCurrentLocation g_instance;
  return g_instance;
}

BOXEE::BXCurrentLocation::BXCurrentLocation()
{
  // Defaults
  m_country = "United States";
  m_countryCode = "US";
  m_city = "New York";
  m_state = "NY";
  m_postalCode = "10001";
  m_temperatureScale = "F";
  m_clockHours = "12";

  m_detected = DetectLocation();
}

bool BOXEE::BXCurrentLocation::DetectLocation()
{
  std::string strUrl = BOXEE::BXConfiguration::GetInstance().GetURLParam("Boxee.ApiGetLocation", "http://app.boxee.tv/location/detect");
  Json::Value jResponse;
  int returnCode;

  if (!BOXEE::BXUtils::PerformJSONGetRequest(strUrl,jResponse,returnCode))
  {
    return false;
  }

  if (jResponse.isNull())
  {
    return false;
  }

  m_country          = jResponse["country_name"].asString();
  m_countryCode      = jResponse["country_code"].asString();
  m_city             = jResponse["city"].asString();
  m_postalCode       = jResponse["postal_code"].asString();
  m_state            = jResponse["state_code"].asString();
  m_temperatureScale = jResponse["temp_scale"].asString();
  m_clockHours       = jResponse["clock_hours"].asString();

  if (m_country == "" || m_countryCode == "")
  {
    return false;
  }

  // TEMPORARY CODE UNTIL THE SERVER RETURNS EVERYTHING
  if (m_temperatureScale == "")
  {
    if (m_country == "US")
      m_temperatureScale = "F";
    else
      m_temperatureScale = "C";
  }

  if (m_clockHours == "")
  {
    if (m_country == "US")
      m_clockHours = "12";
    else
      m_clockHours = "24";
  }
  return true;
}
