/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "Weather.h"
#include "GUISettings.h"
#include "Application.h"
#include "log.h"
#include "lib/libBoxee/bxcurl.h"
#include "Util.h"
#include <json/json.h>
#include <json/reader.h>

CWeather& CWeather::GetInstance()
{
  static CWeather instance;
  return instance;
}

CWeather::CWeather(void) : CInfoLoader(30 * 60 * 1000) // 30 minutes
{
  Reset();
}

void CWeather::Reset()
{
  m_temperature = 0;
  m_conditionsIcon = "";
  m_isFetched = false;
}

void CWeather::DoWork()
{
  if (!g_guiSettings.GetBool("network.enableinternet"))
    return;

  // wait for the network
  if (!g_application.IsConnectedToNet())
    return;

  CStdString locationSetting = g_guiSettings.GetString("weather.areacode1");
  CStdStringArray splitLocation;
  StringUtils::SplitString(locationSetting, " - ", splitLocation);
  if (splitLocation.size() == 0)
    return;
  CStdString location = splitLocation[splitLocation.size() - 1];

  CStdString tempScale = g_guiSettings.GetString("locale.tempscale");

  if (location.IsEmpty() || tempScale.IsEmpty())
  {
    CLog::Log(LOGERROR,"CWeather::DoWork - cannot load weather, areacode or tempscale are empty");
    return;
  }

  CUtil::URLEncode(location);

  CStdString strUrl;
  strUrl.Format("http://app.boxee.tv/location/weather?location=%s&metric=%d",
      location, (tempScale == "C" ? 1 : 0));

  CLog::Log(LOGDEBUG, "Fetching weather from url: %s\n", strUrl.c_str());

  BOXEE::BXCurl curl;
  curl.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());
  curl.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());

  std::string strJson = curl.HttpGetString(strUrl.c_str(), false);

  int returnCode = curl.GetLastRetCode();
  if (returnCode != 200)
  {
    CLog::Log(LOGERROR,"CWeather::DoWork - cannot load weather, got http code: %d", returnCode);
    return;
  }

  Json::Value json;
  Json::Reader reader;
  if (!reader.parse(strJson, json))
  {
    CLog::Log(LOGERROR, "CWeather::DoWork - Error parsing json: %s", reader.getFormatedErrorMessages().c_str());
    return;
  }

#if 0
  printf("Got weather:");
  Json::StyledWriter writer;
  std::string weatherStr = writer.write(json);
  printf("%s\n", weatherStr.c_str());
#endif


  if (!json["temperature"].isString() || !json["weathericon"].isString())
  {
    CLog::Log(LOGERROR, "CWeather::DoWork - Cannot find proper temperature or weathericon in response");
    return;
  }

  m_temperature = atoi(json["temperature"].asCString());
  m_conditionsIcon.Format("weather/%s.png", json["weathericon"].asCString());

  CLog::Log(LOGDEBUG, "Weather got: temp=%d icon=%s\n", m_temperature, m_conditionsIcon.c_str());

  m_isFetched = true;
}

bool CWeather::IsFetched()
{
  // call GetInfo() to make sure that we actually start up
  GetInfo(0);
  return m_isFetched;
}

