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


#include "AppDescriptor.h"
#include "URL.h"
#include "Util.h"
#include "SkinInfo.h"
#include "File.h"
#ifdef __APPLE__
#include "SystemInfo.h"
#endif
#include "SpecialProtocol.h"

#include "LocalizeStrings.h"
#include "utils/log.h"
#include "FileItem.h"
#include "Directory.h"
#include "Crc32.h"
#include "lib/libBoxee/bxoemconfiguration.h"

CAppDescriptor::CAppDescriptor(void)
{
  m_isLoaded = false;
}

CAppDescriptor::CAppDescriptor(TiXmlElement* rootElement)
{
  m_isLoaded = false;

  if (!LoadDescriptorFile(rootElement))
  {
    CLog::Log(LOGERROR, "Cannot load app descriptor file");
    return;
  }

  SetAdditionalFields();

  m_localPath = "special://home/apps";
  m_localPath = CUtil::AddFileToFolder(m_localPath, m_id);

  m_isLoaded = true;
}

CAppDescriptor::CAppDescriptor(const CStdString& urlString)
{
  Load(urlString);
}

void CAppDescriptor::Load(const CStdString& urlString)
{
  m_isLoaded = false;
  CURI url(urlString);

  if (url.GetProtocol() != "app")
  {
    CLog::Log(LOGERROR, "Invalid URL for app");
    return;
  }

  m_id = url.GetHostName();
  
  m_localPath = "special://home/apps";
  m_localPath = CUtil::AddFileToFolder(m_localPath, m_id);

  if (!LoadDescriptorFile())
  {
    CLog::Log(LOGERROR, "Cannot load app descriptor file: %s", m_localPath.c_str());
    return;
  }

  SetAdditionalFields();

  m_isLoaded = true;
}

bool CAppDescriptor::Exists(const CStdString& urlString)
{
  CURI url(urlString);

  if (url.GetProtocol() != "app")
  {
    CLog::Log(LOGERROR, "Invalid URL for app");
    return false;
  }  
  
  CStdString id = url.GetHostName();
  
  CStdString localPath = "special://home/apps";
  localPath = CUtil::AddFileToFolder(localPath, id);
  localPath = CUtil::AddFileToFolder(localPath, "descriptor.xml");
  localPath = _P(localPath);
  
  return XFILE::CFile::Exists(localPath);
}

void CAppDescriptor::SetAdditionalFields(void)
{
  // Find the skin path
  CUtil::AddFileToFolder(m_localPath, "skin", m_skinPath);
  CUtil::AddFileToFolder(m_skinPath, CUtil::GetFileName(g_SkinInfo.GetBaseDir()), m_skinPath);
  if (!XFILE::CFile::Exists(m_skinPath))
  {
    m_skinPath = "";
    CUtil::AddFileToFolder(m_localPath, "skin", m_skinPath);
    
    // this is only for backward support - do not remove
    CUtil::AddFileToFolder(m_skinPath, "Boxee Skin NG", m_skinPath);
  }
  
  // Find the media path
  CUtil::AddFileToFolder(m_localPath, "skin", m_mediaPath);
  CUtil::AddFileToFolder(m_mediaPath, CUtil::GetFileName(g_SkinInfo.GetBaseDir()), m_mediaPath);
  if (!XFILE::CFile::Exists(m_mediaPath))
  {
    m_mediaPath = "";
    CUtil::AddFileToFolder(m_localPath, "skin", m_mediaPath);

    // this is only for backward support - do not remove
    CUtil::AddFileToFolder(m_mediaPath, "Boxee Skin NG", m_mediaPath);
  }

  m_localUrl = "app://";
  m_localUrl += m_id;
  CUtil::AddSlashAtEnd(m_localUrl);

  m_localPath = _P(m_localPath);
}

bool CAppDescriptor::LoadDescriptorFile(TiXmlElement* rootElement)
{
  if (strcmpi(rootElement->Value(), "app") != 0)
  {
    CLog::Log(LOGERROR, "Invalid descriptor file for app, root element should be app: %s", m_localPath.c_str());
    return false;
  }

  TiXmlElement* element;

  element = rootElement->FirstChildElement("id");
  if (!element || (element && !element->FirstChild()) || (element && element->FirstChild() && !element->FirstChild()->Value()))
  {
    CLog::Log(LOGERROR, "Invalid descriptor file for app, id element not found: %s", m_localPath.c_str());
    return false;
  }
  m_id = element->FirstChild()->Value();

  element = rootElement->FirstChildElement("name");
  if (!element || (element && !element->FirstChild()) || (element && element->FirstChild() && !element->FirstChild()->Value()))
  {
    CLog::Log(LOGERROR, "Invalid descriptor file for app, name element not found: %s", m_localPath.c_str());
    return false;
  }
  m_name = element->FirstChild()->Value();

  element = rootElement->FirstChildElement("thumb");
  if (!element || (element && !element->FirstChild()) || (element && element->FirstChild() && !element->FirstChild()->Value()))
  {
    m_thumb = "http://dir.boxee.tv/download/";
    m_thumb += m_id;
    m_thumb += "-thumb.png";
  }
  else
  {
    m_thumb = element->FirstChild()->Value();
  }

  element = rootElement->FirstChildElement("description");
  if (!element || (element && !element->FirstChild()) || (element && element->FirstChild() && !element->FirstChild()->Value()))
  {
    m_description = "";
  }
  else
  {    
    m_description = element->FirstChild()->Value();
  }
  
  element = rootElement->FirstChildElement("tags");
  if (!element || (element && !element->FirstChild()) || (element && element->FirstChild() && !element->FirstChild()->Value()))
  {
    m_tagsStr = "";
  }
  else
  {
    m_tagsStr = element->FirstChild()->Value();
  }

  element = rootElement->FirstChildElement("version");
  if (!element || (element && !element->FirstChild()) || (element && element->FirstChild() && !element->FirstChild()->Value()))
  {
    CLog::Log(LOGERROR, "Invalid descriptor file for app, version element not found: %s", m_localPath.c_str());
    return false;
  }
  m_version = element->FirstChild()->Value();

  element = rootElement->FirstChildElement("type");
  if (!element || (element && !element->FirstChild()) || (element && element->FirstChild() && !element->FirstChild()->Value()))
  {
    CLog::Log(LOGERROR, "Invalid descriptor file for app, type element not found: %s", m_localPath.c_str());
    return false;
  }
  m_type = element->FirstChild()->Value();

  element = rootElement->FirstChildElement("repository");
  if (!element || (element && !element->FirstChild()) || (element && element->FirstChild() && !element->FirstChild()->Value()))
  {
    m_repository = "http://dir.boxee.tv/apps";
  }
  else
  {
    m_repository = element->FirstChild()->Value();
  }
  
  element = rootElement->FirstChildElement("repositoryid");
  if (!element || (element && !element->FirstChild()) || (element && element->FirstChild() && !element->FirstChild()->Value()))
  {
    m_repositoryId = m_id;
  }
  else
  {
    m_repositoryId = element->FirstChild()->Value();
  }  

  element = rootElement->FirstChildElement("media");
  if (!element || (element && !element->FirstChild()) || (element && element->FirstChild() && !element->FirstChild()->Value()))
  {
    CLog::Log(LOGERROR, "Invalid descriptor file for app, media element not found: %s", m_localPath.c_str());
    return false;
  }
  m_mediaType = element->FirstChild()->Value();

  element = rootElement->FirstChildElement("minversion");
  if (!element || (element && !element->FirstChild()) || (element && element->FirstChild() && !element->FirstChild()->Value()))
  {
    CLog::Log(LOGERROR, "Invalid descriptor file for app, minversion element not found: %s", m_localPath.c_str());
    return false;
  }
  m_minVersion = element->FirstChild()->Value();

  element = rootElement->FirstChildElement("maxversion");
  if (element && element->FirstChild() && element->FirstChild()->Value())
  {
    m_maxVersion = element->FirstChild()->Value();
  }

  element = rootElement->FirstChildElement("author");
  if (element && element->FirstChild() && element->FirstChild()->Value())
  {
    m_author = element->FirstChild()->Value();
  }

  m_releaseDate = "0";
  element = rootElement->FirstChildElement("releasedate");
  if (element && element->FirstChild() && element->FirstChild()->Value())
  {
    m_releaseDate = element->FirstChild()->Value();
  }

  m_adult = false;
  element = rootElement->FirstChildElement("rating");
  if (element && element->FirstChild() && element->FirstChild()->Value())
  {
    m_adult = (stricmp(element->FirstChild()->Value(), "adult") == 0);
  }
  
  m_testApp = false;
  element = rootElement->FirstChildElement("test-app");
  if (element && element->FirstChild() && element->FirstChild()->Value())
  {
    m_testApp = ((stricmp(element->FirstChild()->Value(), "true") == 0) || (strcmp(element->FirstChild()->Value(), "1") == 0));
  }

  m_countries = "all";
  m_countriesAllow = true;

  element = rootElement->FirstChildElement("country-allow");
  if (element && element->FirstChild() && element->FirstChild()->Value())
  {
    m_countriesAllow = true;
    m_countries = element->FirstChild()->Value();
  } 

  element = rootElement->FirstChildElement("country-deny");
  if (element && element->FirstChild() && element->FirstChild()->Value())
  {
    m_countriesAllow = false;
    m_countries = element->FirstChild()->Value();
  } 

  m_countries.ToLower();
  
  element = rootElement->FirstChildElement("platform");
  if (!element || (element && !element->FirstChild()) || (element && element->FirstChild() && !element->FirstChild()->Value()))
  {
    CLog::Log(LOGERROR, "Invalid descriptor file for app, platform element not found: %s", m_localPath.c_str());
    return false;
  }
  m_platform = element->FirstChild()->Value();
  
  // For backward compatibility
  if (m_type == "rss")
  {
    m_type = "url";
  }
  
  if (m_type == "skin" || m_type == "skin-native")
  {
    element = rootElement->FirstChildElement("startWindow");
    if (!element)
    {
      element = rootElement->FirstChildElement("startwindow");

      if (!element)
      {
        CLog::Log(LOGERROR, "Invalid descriptor file for app, startWindow element not found: %s", m_localPath.c_str());
        return false;
      }
    }
    m_startWindow = element->FirstChild()->Value();
  }
  else if (m_type == "url" || m_type == "html")
  {
    element = rootElement->FirstChildElement("url");
    if (!element)
    {
      CLog::Log(LOGERROR, "Invalid descriptor file for app, url element not found: %s", m_localPath.c_str());
      return false;
    }
    m_url = element->FirstChild()->Value();
    
    element = rootElement->FirstChildElement("backgroundImageUrl");
    if (element && element->FirstChild() && element->FirstChild()->Value())
    {
      m_backgroundImageURL = element->FirstChild()->Value();
    }

    element = rootElement->FirstChildElement("controller");
    if (element && element->FirstChild() && element->FirstChild()->Value())
    {
      m_controller = element->FirstChild()->Value();
  }
  }

  element = rootElement->FirstChildElement("partner-id");
  if (element && element->FirstChild() && element->FirstChild()->Value())
  {
    m_partnerId = element->FirstChild()->Value();
  }

  element = rootElement->FirstChildElement("handler");
  if (element && element->FirstChild() && element->FirstChild()->Value())
  {
    m_globalHandler = element->FirstChild()->Value();
  }

  CStdString elementName = "signature-";
#if defined(DEVICE_PLATFORM)
  if (BOXEE::BXOEMConfiguration::GetInstance().HasParam("Boxee.Device.Name"))
    elementName += BOXEE::BXOEMConfiguration::GetInstance().GetStringParam("Boxee.Device.Name");
  else
    elementName += DEVICE_PLATFORM;
#elif defined(__APPLE__)
  elementName += "osx";
#elif defined(_WIN32)
  elementName += "win32";
#else
  elementName += "linux";
#endif

  element = rootElement->FirstChildElement(elementName);
  if (element && element->FirstChild() && element->FirstChild()->Value())
  {
    m_boxeeSig = element->FirstChild()->Value();
  }

  element = rootElement->FirstChildElement("is-persistent");
  if (element && element->FirstChild() && element->FirstChild()->Value())
  {
    m_persistent = true;
  }
  else 
  {
    m_persistent = false;
  }

  element = rootElement->FirstChildElement("sharedlib");
  while (element)
  {
    if (element->FirstChild() && element->FirstChild()->Value() && element->Attribute("platform"))
    {
      const char* library = element->FirstChild()->Value();
      const char* platform = element->Attribute("platform");

      if (CUtil::MatchesPlatform(platform))
      {
        m_additionalSharedLibraries.push_back(library);
      }
    }

    element = element->NextSiblingElement("sharedlib");
  }

  return true;
}

bool CAppDescriptor::GenerateSignature()
{
  static const char* knownExtensions[] = {".py", ".pyo", ".pyc", ".xml"};
  CFileItemList items;
  CStdString localPath = "special://home/apps";
  CStdString appSignature;
  unsigned __int64 appCrc = 0;

  if(m_localPath.empty())
    return false;
  
  DIRECTORY::CDirectory::GetDirectory(m_localPath, items);
  
  for(int i=0; i<items.Size(); i++)
  {
    CStdString ext = CUtil::GetExtension(items[i]->m_strPath);
    bool bKnown = false;

    for(size_t j=0; j<sizeof(knownExtensions)/sizeof(knownExtensions[0]); j++)
    {
      if(!ext.compare(knownExtensions[j]))
      {
        bKnown = true;
        break;
      }
    }

    if(bKnown)
    {     
      CStdString hash = CUtil::MD5File(items[i]->m_strPath);
      Crc32 crc;

      crc.Compute(hash);

      appCrc += (unsigned __int32)crc;
    }
  }

  m_signature.Format("%I64x", appCrc);

  return true;
}

bool CAppDescriptor::LoadDescriptorFile()
{
  CStdString descriptorFile = m_localPath;
  descriptorFile = CUtil::AddFileToFolder(descriptorFile, "descriptor.xml");
  descriptorFile = _P(descriptorFile);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(descriptorFile))
  {
    CLog::Log(LOGERROR, "Cannot load descriptor file for app: %s at row=%d/col=%d from file %s", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow(), xmlDoc.ErrorCol(), descriptorFile.c_str());
    return false;
  }

  TiXmlElement* rootElement = xmlDoc.RootElement();
  return LoadDescriptorFile(rootElement);
}

CAppDescriptor::~CAppDescriptor()
{
  m_isLoaded = false;
}

bool CAppDescriptor::IsLoaded() const
{
  return m_isLoaded;
}

const CStdString& CAppDescriptor::GetThumb() const
{
  return m_thumb;
}

const CStdString& CAppDescriptor::GetName() const
{
  return m_name;
}

const CStdString& CAppDescriptor::GetDescription() const
{
  return m_description;
}

const CStdString& CAppDescriptor::GetTagsStr() const
{
  return m_tagsStr;
}

const CStdString& CAppDescriptor::GetLocalPath() const
{
  return m_localPath;
}

const CStdString& CAppDescriptor::GetStartWindow() const
{
  return m_startWindow;
}

const CStdString CAppDescriptor::GetSkinPath(CStdString& xmlName) const
{
  RESOLUTION res;
  return g_SkinInfo.GetSkinPath(xmlName, &res, m_skinPath);
}

const CStdString& CAppDescriptor::GetMediaPath() const
{
  return m_mediaPath;
}

const CStdString& CAppDescriptor::GetLocalURL() const
{
  return m_localUrl;
}

const CStdString& CAppDescriptor::GetId() const
{
  return m_id;
}

const CStdString& CAppDescriptor::GetMediaType() const
{
  return m_mediaType;
}

const CStdString& CAppDescriptor::GetVersion() const
{
  return m_version;
}

const CStdString& CAppDescriptor::GetType() const
{
  return m_type;
}

const CStdString& CAppDescriptor::GetRepository() const
{
  return m_repository;
}

const CStdString& CAppDescriptor::GetRepositoryId() const
{
  return m_repositoryId;
}

const CStdString& CAppDescriptor::GetURL() const
{
  return m_url;
}

const CStdString& CAppDescriptor::GetReleaseDate() const
{
  return m_releaseDate;
}

const CStdString& CAppDescriptor::GetMinVersion() const
{
  return m_minVersion;
}

const CStdString& CAppDescriptor::GetMaxVersion() const
{
  return m_maxVersion;
}

const CStdString& CAppDescriptor::GetPlatform() const
{
  return m_platform;
}

const CStdString& CAppDescriptor::GetAuthor() const
{
  return m_author;
}

const CStdString& CAppDescriptor::GetBackgroundImageURL() const
{
  return m_backgroundImageURL;
}

const CStdString& CAppDescriptor::GetController() const
{
  return m_controller;
}

bool CAppDescriptor::MatchesPlatform()
{
  return CUtil::MatchesPlatform(m_platform);
}

bool CAppDescriptor::IsBoxeeApp() const
{
  return (m_repositoryId == "" || m_repositoryId == "tv.boxee");  
}

bool CAppDescriptor::IsAdult() const
{
  return m_adult;
}

bool CAppDescriptor::IsTestApp() const
{
  return m_testApp;
}

void CAppDescriptor::GetCountryRestrictions(CStdString& countries, bool& allow) const
{
  countries = m_countries;
  allow = m_countriesAllow;
}

bool CAppDescriptor::IsAllowed() const
{
  if (IsAdult() && !CUtil::IsAdultAllowed())
  {
    return false;
  }
  
  return IsCountryAllowed();  
}

bool CAppDescriptor::IsCountryAllowed() const
{
  if (m_countries == "all" && m_countriesAllow)
    return true;
  
  return CUtil::IsCountryAllowed(m_countries, m_countriesAllow);
}

const CStdString& CAppDescriptor::GetPartnerId() const
{
  return m_partnerId;
}

const CStdString& CAppDescriptor::GetGlobalHandler() const
{
  return m_globalHandler;
}

const CStdString& CAppDescriptor::GetSignature() const
{
  return m_boxeeSig;
}

bool CAppDescriptor::IsPersistent() const
{
  return m_persistent;
}

const std::vector<CStdString>& CAppDescriptor::GetAdditionalSharedLibraries() const
{
  return m_additionalSharedLibraries;
}
