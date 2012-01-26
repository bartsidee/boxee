/*
 *      Copyright (C) 2005-2009 Team Boxee
 *      http://www.boxee.tv
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


#include "AppRepository.h"
#include "Util.h"
#include "bxversion.h"
#include "bxcurl.h"
#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxappboxapplications.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "BoxeeUtils.h"
#include "GUISettings.h"
#include "Application.h"
#ifdef __APPLE__
#include "SystemInfo.h"
#endif

#ifdef HAS_EMBEDDED
#include "HalServices.h"
#endif

#include "utils/log.h"
#include "LocalizeStrings.h"

// 1 hour
#define APPS_INDEX_CACHE_TIME (60 * 60)

CAppRepository::CAppRepository()
{
  m_valid = false;
}

CAppRepository::CAppRepository(const CStdString& url)
{
  m_valid = false;
  m_descriptorsLoaded = false;
  m_cachedAvailableApps.clear();
  m_cachedAvailableAppsTimestamp = 0;
  
  if (LoadRepositoryFromURL(url))
  {
    m_valid = true;
  }
}

CAppRepository::CAppRepository(const CStdString& id, const CStdString& url, const CStdString& name, const CStdString& description, const CStdString& thumbnail)
{
  m_valid = true;
  m_id = id;
  m_url = url;
  m_name = name;
  m_description = description;
  m_thumbnail = thumbnail;
  
  m_descriptorsLoaded = false;
  m_cachedAvailableApps.clear();
  m_cachedAvailableAppsTimestamp = 0;
}

CAppRepository::CAppRepository(const CAppRepository& app)
{
  m_valid = app.m_valid;
  m_id = app.m_id;
  m_url = app.m_url;
  m_name = app.m_name;
  m_description = app.m_description;
  m_thumbnail = app.m_thumbnail;
  m_descriptorsLoaded = false;
  m_cachedAvailableAppsTimestamp = 0;
}

CAppRepository::~CAppRepository()
{
}
  
const CStdString& CAppRepository::GetURL() const
{
  if (!m_valid)
  {
    return StringUtils::EmptyString;
  }
  
  return m_url;
}

const CStdString& CAppRepository::GetName() const
{
  if (!m_valid)
  {
    return StringUtils::EmptyString;
  }

  return m_name;
}

const CStdString& CAppRepository::GetDescription() const
{
  if (!m_valid)
  {
    return StringUtils::EmptyString;
  }

  return m_description;
}

const CStdString& CAppRepository::GetID() const
{
  if (!m_valid)
  {
    return StringUtils::EmptyString;
  }

  return m_id;
}

const CStdString& CAppRepository::GetThumbnail() const
{
  if (!m_valid)
  {
    return StringUtils::EmptyString;
  }

  return m_thumbnail;
}

bool CAppRepository::IsValid()
{
  return m_valid;
}

CAppDescriptor::AppDescriptorsMap CAppRepository::GetAvailableApps(const CStdString& category)
{
  if (!m_valid)
  {
    return m_cachedAvailableApps;
  }

  if (m_cachedAvailableAppsTimestamp + APPS_INDEX_CACHE_TIME > (unsigned int) time(NULL))
  {
    return m_cachedAvailableApps;
  }

  ReloadAppDescriptors(false,category);
  return m_cachedAvailableApps;
}

bool CAppRepository::ReloadAppDescriptors(bool bDontWait , const CStdString& category)
{
  // this is updated at the beginning, so that even if a repository is down it will not 
  // get the system to keep trying to query it until the cache has expired
  
  m_cachedAvailableAppsTimestamp = time(NULL);  
  m_descriptorsLoaded = false;
  CAppDescriptor::AppDescriptorsMap result;

  bool succeeded = false;

  if (m_id == "tv.boxee")
  {
    // Get the applications from the Boxee repository
    succeeded = GetBoxeeApplications(result , category);

    if(!succeeded)
    {
      CLog::Log(LOGERROR, "FAILED to get boxee repository applications (appbox)");
      return false;
    }
  }
  else
  {
    // Get the applications from the 3rdParty repositories
    succeeded = Get3rdPartyApplications(result, bDontWait);

    if(!succeeded)
    {
      CLog::Log(LOGERROR, "FAILED to get 3rd-party repositories applications (appbox)");
      return false;
    }
  }

  m_cachedAvailableApps = result;
  m_descriptorsLoaded = true;
  
  return true;
}

bool CAppRepository::LoadRepositoryFromURL(const CStdString& url)
{
  // Build the URL of the index based on platform/version
  CStdString indexURL = url;
  if (!CUtil::HasSlashAtEnd(indexURL))
  {
    indexURL += "/";
  }
  indexURL += "repository.xml";

  // Retrieve the file
  CLog::Log(LOGDEBUG,"CAppRepository::LoadRepositoryFromURL - Reading repository descriptor from [url=%s]",indexURL.c_str());

  BOXEE::BXXMLDocument document;
  if (!document.LoadFromURL(indexURL.c_str()))
  {
    LOG(LOG_LEVEL_ERROR,"CAppRepository::LoadRepositoryFromURL - FAILED to load repository descriptor from [url=%s]",indexURL.c_str());
    return false;
  }

  TiXmlElement* rootElement = document.GetDocument().RootElement();
  if (!rootElement)
  {
    CLog::Log(LOGERROR,"CAppRepository::LoadRepositoryFromURL - FAILED to get root element. [url=%s]",indexURL.c_str());
    return false;
  }

  if (strcmpi(rootElement->Value(), "repository") != 0)
  {
    CLog::Log(LOGERROR,"CAppRepository::LoadRepositoryFromURL - Invalid repository descriptor file. root element is <%s> and should be <repository>. [url=%s]",rootElement->Value(),indexURL.c_str());
    return false;
  }

  *this = CAppRepository::LoadRepositoryFromXML(rootElement);
  return IsValid();
}

CAppRepository CAppRepository::LoadRepositoryFromXML(const TiXmlNode* rootElement)
{
  const TiXmlElement* element = rootElement->FirstChildElement("id");
  if (!element || (element && !element->FirstChild()) || (element && element->FirstChild() && !element->FirstChild()->Value()))
  {
    CLog::Log(LOGERROR, "Invalid repository file, id element not found for one of the repositories");
    return CAppRepository();
  }  
  CStdString id = element->FirstChild()->Value();  

  element = rootElement->FirstChildElement("url");
  if (!element || (element && !element->FirstChild()) || (element && element->FirstChild() && !element->FirstChild()->Value()))
  {
    CLog::Log(LOGERROR, "Invalid repository file, url element not found for one of the repositories");
    return CAppRepository();
  }  
  CStdString url = element->FirstChild()->Value();  

  element = rootElement->FirstChildElement("thumb");
  if (!element || (element && !element->FirstChild()) || (element && element->FirstChild() && !element->FirstChild()->Value()))
  {
    CLog::Log(LOGERROR, "Invalid repository file, thumb element not found for one of the repositories");
    return CAppRepository();
  }  
  CStdString thumb = element->FirstChild()->Value();  
  
  element = rootElement->FirstChildElement("description");
  if (!element || (element && !element->FirstChild()) || (element && element->FirstChild() && !element->FirstChild()->Value()))
  {
    CLog::Log(LOGERROR, "Invalid repository file, description element not found for one of the repositories");
    return CAppRepository();
  }  
  CStdString description = element->FirstChild()->Value();  
  
  element = rootElement->FirstChildElement("name");
  if (!element || (element && !element->FirstChild()) || (element && element->FirstChild() && !element->FirstChild()->Value()))
  {
    CLog::Log(LOGERROR, "Invalid repository name, name element not found for one of the repositories");
    return CAppRepository();
  }  
  CStdString name = element->FirstChild()->Value();      

  return CAppRepository(id, url, name, description, thumb);
}

bool CAppRepository::GetBoxeeApplications(CAppDescriptor::AppDescriptorsMap& result, const CStdString& category)
{
  bool succeeded = false;

  //TiXmlDocument xmlDoc;

  //BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetAppBoxApplications(xmlDoc);

  BOXEE::BXAppBoxApplications appBoxBoxeeApplicationsList;

  appBoxBoxeeApplicationsList.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  appBoxBoxeeApplicationsList.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

  std::string strUrl = BOXEE::BXConfiguration::GetInstance().GetURLParam("Boxee.AppBoxApplicationsPrefixUrl","http://app.boxee.tv/appindex");

  strUrl += "/";
  strUrl += BOXEE::Boxee::GetInstance().GetPlatform();
  strUrl += "/";
  strUrl += BOXEE_VERSION;

  std::map<CStdString, CStdString> mapRemoteOptions;

  if (!CUtil::IsAdultAllowed())
  {
    mapRemoteOptions["adult"] = "no";
  }

  if (g_guiSettings.GetBool("filelists.filtergeoip2"))
  {
    CStdString countryCode = g_application.GetCountryCode();

    if (!countryCode.IsEmpty())
    {
      mapRemoteOptions["geo"] = countryCode;
    }
  }

  if (!category.IsEmpty())
  {
    mapRemoteOptions["category"] = category;
  }

#ifdef HAS_EMBEDDED
  CHalHardwareInfo hwInfo;
  CHalSoftwareInfo swInfo;
  IHalServices& client = CHalServicesFactory::GetInstance();

  if(client.GetHardwareInfo(hwInfo))
  {
    mapRemoteOptions["serialnumber"] = hwInfo.serialNumber;
  }

  if (client.GetSoftwareInfo(swInfo))
  {
    mapRemoteOptions["regionsku"] = swInfo.regionSKU;
  }
#endif

  if (mapRemoteOptions.size() > 0)
  {
    strUrl += BoxeeUtils::BuildParameterString(mapRemoteOptions);
  }

  appBoxBoxeeApplicationsList.LoadFromURL(strUrl);

  //bool isLoaded = appBoxBoxeeApplicationsList.IsLoaded();
  //long lastRetCode = appBoxBoxeeApplicationsList.GetLastRetCode();

  TiXmlElement* rootElement = appBoxBoxeeApplicationsList.GetRoot();
  if (!rootElement)
  {
    CLog::Log(LOGERROR, "CAppRepository::GetBoxeeApplications - cant retrieve apps list");
    return false;
  }
  
  if (strcmpi(rootElement->Value(), "apps") != 0)
  {
    CLog::Log(LOGERROR, "CAppRepository::GetBoxeeApplications - Invalid apps list. Root element should be <apps>. [RootElement=%s] (appbox)",rootElement->Value());
    return false;
  }

  succeeded = GetApps(result,rootElement,"BOXEE");

  if(!succeeded)
  {
    // Error log was written from GetApps() function
  }

  return succeeded;
}

bool CAppRepository::Get3rdPartyApplications(CAppDescriptor::AppDescriptorsMap& result, bool bDontWait)
{
  bool succeeded = false;

  TiXmlDocument xmlDoc;
  succeeded = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetAppBox3rdPartyRepositoryApplications(m_id,xmlDoc, bDontWait);

  if (!succeeded)
  {
    CLog::Log(LOGERROR, "CAppRepository::Get3rdPartyApplications - Call to GetAppBox3rdPartyRepositoryApplications FAILED. [repoId=%s] (appbox)(repos)", m_id.c_str());
    return succeeded;
  }

  TiXmlElement* rootElement = xmlDoc.RootElement();
  if (rootElement != NULL)
  {
    if (strcmpi(rootElement->Value(), "apps") != 0)
    {
      CLog::Log(LOGERROR, "CAppRepository::Get3rdPartyApplications - Invalid apps list. Root element should be <apps>. [repoId=%s][RootElement=%s] (appbox)",m_id.c_str(),rootElement->Value());
      return succeeded;
    }

    succeeded = GetApps(result,rootElement,"3rd-Party");

    if(!succeeded)
    {
      // Error log was written from GetApps() function
    }
  }
  else
  {
    CLog::Log(LOGERROR, "CAppRepository::Get3rdPartyApplications - Invalid apps list. Root element is null. [repoId=%s] (appbox)",m_id.c_str());
  }

  return succeeded;
}

bool CAppRepository::GetApps(CAppDescriptor::AppDescriptorsMap& result, TiXmlElement* rootElement, const CStdString& repositoryType)
{
  if(!rootElement)
  {
    CLog::Log(LOGERROR, "CAppRepository::GetApps - Enter function with empty Root element. [repoId=%s][repositoryType=%s] (appbox)",m_id.c_str(),repositoryType.c_str());
    return false;
  }

  int numOfAllApps = 0;
  int numOfAppsAdded = 0;

  TiXmlElement *node = rootElement->FirstChildElement("app");
  while (node)
  {
    CAppDescriptor appDesc(node);

    numOfAllApps++;

    //CLog::Log(LOGDEBUG, "CAppRepository::GetApps - [%d] -  Handling app [id=%s][name=%s][MediaType=%s][IsLoaded=%d][MatchesPlatform=%d]. [repositoryType=%s] (appbox)",numOfAllApps,appDesc.GetId().c_str(),appDesc.GetName().c_str(),appDesc.GetMediaType().c_str(),appDesc.IsLoaded(),appDesc.MatchesPlatform(),repositoryType.c_str());

    if (appDesc.IsLoaded())
    {
      result[appDesc.GetId()] = appDesc;

      //CLog::Log(LOGDEBUG, "CAppRepository::GetApps - [%d] -  After add app [id=%s][name=%s][MediaType=%s]. [repositoryType=%s] (appbox)",numOfAllApps,appDesc.GetId().c_str(),appDesc.GetName().c_str(),appDesc.GetMediaType().c_str(),repositoryType.c_str());

      numOfAppsAdded++;
    }

    node = node->NextSiblingElement("app");
  }

  CLog::Log(LOGDEBUG, "CAppRepository::GetApps - Got [%d] apps out of [%d]. [repoId=%s][repositoryType=%s] (appbox)",numOfAppsAdded,numOfAllApps,m_id.c_str(),repositoryType.c_str());

  return true;
}

