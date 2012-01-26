
#include "StdString.h"
#include <map>
#include "PlatformDefs.h"
#include "BoxeeServerOTADirectory.h"
#include "Application.h"
#include "json/value.h"
#include "json/reader.h"
#include "../../lib/libBoxee/boxee.h"
#include "../../lib/libBoxee/bxconfiguration.h"
#include "../BoxeeUtils.h"
#include "FileSystem/DirectoryCache.h"
#include "SpecialProtocol.h"
#include "URL.h"
#include "FileSystem/FileCurl.h"
#include "Util.h"
#include "GUISettings.h"
#include "../cores/dvb/dvbmanager.h"


using namespace BOXEE;
using namespace XFILE;

namespace DIRECTORY
{

CBoxeeServerOTADirectory::CBoxeeServerOTADirectory()
{
  m_cacheDirectory = DIR_CACHE_ALWAYS;  // by default, caching is done.
}

CBoxeeServerOTADirectory::~CBoxeeServerOTADirectory()
{

}

bool CBoxeeServerOTADirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CLog::Log(LOGDEBUG, "CBoxeeServerOTADirectory::GetDirectory -  enter function with [path=%s] (ota)", strPath.c_str());

  m_cacheDirectory = DIR_CACHE_ALWAYS;

  CURI url(strPath);
  CStdString strProtocol = url.GetProtocol();
  std::map<CStdString, CStdString> mapOptions = url.GetOptionsAsMap(); // NOTICE: the URL options are decoded here

  if (strProtocol.CompareNoCase("ota") != 0)
    return false;

  CLog::Log(LOGDEBUG, "CBoxeeServerOTADirectory::GetDirectory - [HostName=%s][ShareName=%s][Domain=%s][FileName=%s][Options=%s] (ota)", url.GetHostName().c_str(), url.GetShareName().c_str(), url.GetDomain().c_str(), url.GetFileName().c_str(), url.GetOptions().c_str());

  // The type of the request movies or tvshows
  CStdString strType = url.GetHostName();
  // The actual request
  CStdString strRequest = url.GetShareName();

  if (strType == "location")
  {
    return HandleLocationRequest(strRequest, mapOptions, items);
  }
  else if(strType == "epg" && !mapOptions.empty())
  {
    return HandleEPGRequest(strRequest, mapOptions, items);
  }
  else
  {
    CLog::Log(LOGERROR, "CBoxeeServerOTADirectory::GetDirectory - unrecognized url [%s] or not enough parameters (ota)", strPath.c_str());
    return false;
  }

  return true;
}

bool CBoxeeServerOTADirectory::HandleEPGRequest(const CStdString& strRequest, std::map<CStdString, CStdString>& mapOptions, CFileItemList &items)
{
  Json::Value response;
  CStdString strUrl;

  if (strRequest == "channels")
  {
    //use http://app.boxee.tv/epg/channel?dvb_triplets=CSV
    //use http://app.boxee.tv/epg/channel?provider_gn_id=CSV
    strUrl = BOXEE::BXConfiguration::GetInstance().GetURLParam("Boxee.ApiGetChannels","http://app.boxee.tv/epg/channel");
  }
  else if (strRequest == "programs")
  {
    //use it with http://app.boxee.tv/epg/program?channel_gn_ids=CSV
    strUrl = BOXEE::BXConfiguration::GetInstance().GetURLParam("Boxee.ApiGetPrograms","http://app.boxee.tv/epg/program");
  }

  strUrl += BoxeeUtils::BuildParameterString(mapOptions);
  long retCode = HandleJsonRequest(strUrl,response);

  if (retCode == 200)
  {
    if (ParseJsonResponse(response, items) > 0)
      return true;
  }

  return false;
}

bool CBoxeeServerOTADirectory::HandleLocationRequest(const CStdString& strRequest, std::map<CStdString, CStdString>& mapOptions, CFileItemList &items)
{
  Json::Value response;
  CStdString strUrl;

  if (strRequest == "zipcode")
  {
    //http://app.boxee.tv/user/location
    strUrl = BOXEE::BXConfiguration::GetInstance().GetURLParam("Boxee.ApiDetermineLocation","http://app.boxee.tv/user/location");
  }
  else if (strRequest == "countries")
  {
      //http://app.boxee.tv/location/countries
      strUrl = BOXEE::BXConfiguration::GetInstance().GetURLParam("Boxee.ApiGetCountries","http://app.boxee.tv/location/countries");
  }
  else if(strRequest == "cities")
  {
      //http://app.boxee.tv/location/city?country=US
      strUrl = BOXEE::BXConfiguration::GetInstance().GetURLParam("Boxee.ApiGetCities","http://app.boxee.tv/location/city");
  }
/*  //http://app.boxee.tv/user/location
      strUrl = BOXEE::BXConfiguration::GetInstance().GetURLParam("Boxee.ApiDetermineLocation","http://app.boxee.tv/user/location");
    break;
    //http://app.boxee.tv/location/city?postal_code=10001 //need to ask server team*/

  strUrl += BoxeeUtils::BuildParameterString(mapOptions);
  long retCode = HandleJsonRequest(strUrl,response);

  if (retCode == 200)
  {
    if (ParseJsonResponse(response, items) > 0)
      return true;
  }

  return false;
}

int CBoxeeServerOTADirectory::ParseJsonResponse(const Json::Value& jsonValue, CFileItemList& outputList)
{
  outputList.Clear();

  if (jsonValue.isArray())
  {
    for (size_t j = 0; j < jsonValue.size(); j++)
    {
      Json::Value m = jsonValue[(int) j];
      Json::Value::Members keys = m.getMemberNames();

      CFileItemPtr jsonItem(new CFileItem("jsonObject"));

      for (size_t i = 0; i < keys.size(); i++)
      {
        jsonItem->SetProperty(keys[i], m[keys[i]].asString());
      }
      outputList.Add(jsonItem);
    }
  }

  return outputList.Size();
}



bool CBoxeeServerOTADirectory::HasOption(const std::map<CStdString, CStdString>& mapOptions, const CStdString& optionName, CStdString& optionValue)
{
  std::map<CStdString, CStdString>::const_iterator lb = mapOptions.lower_bound(optionName);
  if(lb != mapOptions.end() && !(mapOptions.key_comp()(optionName, lb->first)))
  {
    optionValue = lb->second;
    return true;
  }
  return false;
}

bool CBoxeeServerOTADirectory::Exists(const char* strPath)
{
  // NOT IMPLEMENTED
  return true;

}

void CBoxeeServerOTADirectory::AddParametersToRemoteRequest(std::map<CStdString, CStdString>& mapRemoteOptions)
{
  if (!CUtil::IsAdultAllowed())
  {
    mapRemoteOptions["adult"] = "no";
  }
  else
  {
    mapRemoteOptions["adult"] = "yes";
  }

  if (g_guiSettings.GetBool("filelists.filtergeoip2"))
  {
    CStdString countryCode = g_application.GetCountryCode();

    if (!countryCode.IsEmpty())
    {
      mapRemoteOptions["geo"] = countryCode;
    }
  }
}

long CBoxeeServerOTADirectory::HandleJsonRequest(const std::string& url, Json::Value& response)
{
  if (url.empty())
    return -1;

  std::string strUrl = url;

  BXCurl curl;

  curl.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());
  curl.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());

  ListHttpHeaders headers;
  headers.push_back("Connection: keep-alive");
  curl.HttpSetHeaders(headers);

  CStdString strJson = curl.HttpGetString(strUrl.c_str());

  Json::Reader  reader;

  long returnCode = curl.GetLastRetCode();

  if (returnCode==200 && !strJson.IsEmpty())
  {
    if (!reader.parse(strJson,response))
    {
      return -1;
    }
  }

  return returnCode;
}

DIR_CACHE_TYPE CBoxeeServerOTADirectory::GetCacheType(const CStdString& strPath) const
{
  return m_cacheDirectory;
}

}

