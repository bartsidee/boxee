#pragma once

#include <string>

namespace BOXEE
{

class BXCurrentLocation
{
public:
  static BXCurrentLocation& GetInstance();

  std::string& GetCountry()            { return m_country; }
  std::string& GetCountryCode()        { return m_countryCode; }
  std::string& GetState()              { return m_state; }
  std::string& GetCity()               { return m_city; }
  std::string& GetPostalCode()         { return m_postalCode; }
  // F or C
  std::string& GetTemperatureScale()   { return m_temperatureScale; }
  // 12 or 24
  std::string& GetClockHours()         { return m_clockHours; }
  bool IsLocationDetected()              { return m_detected; }

private:
  BXCurrentLocation();
  bool DetectLocation();

  std::string m_country;
  std::string m_countryCode;
  std::string m_state;
  std::string m_city;
  std::string m_postalCode;
  std::string m_temperatureScale;
  std::string m_clockHours;

  bool m_detected;
};

};
