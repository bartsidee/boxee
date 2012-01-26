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


#include "AppRepositories.h"
#include "Settings.h"
#include "Util.h"
#include "File.h"
#include "BoxeeUtils.h"
#include "lib/libBoxee/boxee.h"
#include "utils/log.h"
#include "utils/SingleLock.h"

using namespace BOXEE;

CAppRepositories::CAppRepositories()
{
  Init();
}

CAppRepositories::~CAppRepositories()
{
}

void CAppRepositories::Init()
{
  CSingleLock lock(m_lock);
  m_repositories.clear();
  CAppRepository boxeeRepository("tv.boxee", "http://dir.boxee.tv/apps", "Boxee Official Repository", "Boxee's official repository. Contains all Apps developed by Boxee and trusted sources.", "boxee_icon.png");
  Add(boxeeRepository);
  m_loaded = false;
}

bool CAppRepositories::Load(bool force)
{
  if (!force && m_loaded)
  {
    return true;
  }
  
  Init();
  
  BXAppBoxRepositories repositoriesList;
  BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetAppBoxRepositories(repositoriesList);

  for(int i=0;i<repositoriesList.GetNumOfRepositories();i++)
  {
    BXObject repo = repositoriesList.GetRepository(i);
    CAppRepository aRepository(repo.GetID(),repo.GetValue("url"),repo.GetValue("name"),repo.GetValue("description"),repo.GetValue("thumb"));

    CLog::Log(LOGDEBUG,"CAppRepositories::Load - Going to add Repository [id=%s][name=%s][url=%s] (repos)",aRepository.GetID().c_str(),aRepository.GetName().c_str(),aRepository.GetURL().c_str());

    Add(aRepository);
  }
  
  m_loaded = true;
  return true;
}

bool CAppRepositories::Save()
{
  TiXmlDocument xmlDoc;
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(TiXmlElement("repositories"));

  {
    CSingleLock lock(m_lock);
  for (size_t i = 0; i < m_repositories.size(); i++)
  {
    if (m_repositories[i].GetID() == "tv.boxee")
    {
      continue;
    }

    TiXmlNode *repo = pRoot->InsertEndChild(TiXmlElement("repository"));
    repo->InsertEndChild(TiXmlElement("id"))->InsertEndChild(TiXmlText(m_repositories[i].GetID().c_str()));
    repo->InsertEndChild(TiXmlElement("url"))->InsertEndChild(TiXmlText(m_repositories[i].GetURL().c_str()));
    repo->InsertEndChild(TiXmlElement("name"))->InsertEndChild(TiXmlText(m_repositories[i].GetName().c_str()));
    repo->InsertEndChild(TiXmlElement("description"))->InsertEndChild(TiXmlText(m_repositories[i].GetDescription().c_str()));
    repo->InsertEndChild(TiXmlElement("thumb"))->InsertEndChild(TiXmlText(m_repositories[i].GetThumbnail().c_str()));
  }
  }

  CStdString strXMLFile = g_settings.GetProfileUserDataFolder();
  CUtil::AddFileToFolder(strXMLFile, "repositories.xml", strXMLFile);

  return xmlDoc.SaveFile(strXMLFile);
}

void CAppRepositories::ReloadAppDescriptors()
{
  CSingleLock lock(m_lock);
  for (size_t i = 0; i < m_repositories.size(); i++)
  {
    m_repositories[i].ReloadAppDescriptors(true);
  }
}

bool CAppRepositories::Add(CAppRepository& repository)
{
  CSingleLock lock(m_lock);
  for (size_t i = 0; i < m_repositories.size(); i++)
  {
    if (!repository.IsValid() || m_repositories[i].GetID() == repository.GetID())
    {
      return false;
    }
  }
  
  m_repositories.push_back(repository);

  return true;
}

bool CAppRepositories::Delete(const CStdString& id, bool reportToServer)
{
  CSingleLock lock(m_lock);
  bool found = false;
  
  for (size_t i = 0; i < m_repositories.size(); i++)
  {
    if (m_repositories[i].GetID() == id)
    {
      // Remove the repository installed apps
      RemoveRepositoryInstalledApps(m_repositories[i]);

      m_repositories.erase(m_repositories.begin() + i);
      found = true;
      break;
    }
  }
  
  if (found && reportToServer)
  {
    // Report to the server about the removed repository
  BoxeeUtils::ReportRemoveRepository(id);
  }

  return found;
}

std::vector<CAppRepository> CAppRepositories::Get()
{
  CSingleLock lock(m_lock);
  return m_repositories;
}

int CAppRepositories::Size()
{
  CSingleLock lock(m_lock);
  return m_repositories.size();
}

bool  CAppRepositories::GetDescriptorById(const CStdString id, CAppDescriptor& descriptor)
{
  CSingleLock lock(m_lock);
  CStdString appId;
  
  for (size_t i = 0; i < m_repositories.size(); i++)
  {
    CAppRepository& repository = m_repositories[i];
    CAppDescriptor::AppDescriptorsMap apps = repository.GetAvailableApps();
    if (apps.find(id) != apps.end())
    {
      descriptor = apps[id];
      return true;
    }
  } 
  
  return false;
}

CAppDescriptor::AppDescriptorsMap CAppRepositories::GetAvailableApps(bool force, bool boxeeAppsOnly)
{
  CSingleLock lock(m_lock);
  if (boxeeAppsOnly)
  {
    CAppRepository& repository = m_repositories[0];
    return repository.GetAvailableApps();    
  }
  else
  {
    Load(force);
    
    m_lastAvailableApps.clear();
    for (size_t i = 0; i < m_repositories.size(); i++)
    {
      CAppRepository& repository = m_repositories[i];
      const CAppDescriptor::AppDescriptorsMap& availableAppsDesc = repository.GetAvailableApps();
      m_lastAvailableApps.insert(availableAppsDesc.begin(), availableAppsDesc.end());
    }

    return m_lastAvailableApps;
  }
}

void CAppRepositories::GetAvailableApps(const CAppDescriptor::AppDescriptorsMap& availableAppsDesc, CFileItemList& availableAppsFileItems, const CStdString& mediaType)
{
  for (CAppDescriptor::AppDescriptorsMap::const_iterator it = availableAppsDesc.begin(); it != availableAppsDesc.end(); it++)
  {
    CAppDescriptor appDesc = it->second;

    if(!mediaType.IsEmpty() && (mediaType != appDesc.GetMediaType()))
    {
      continue;
    }

    CStdString country;
    bool allow;
    appDesc.GetCountryRestrictions(country, allow);

    CFileItemPtr pItem(new CFileItem());
    pItem->SetLabel(appDesc.GetName());
    pItem->SetThumbnailImage(appDesc.GetThumb());
    pItem->m_strPath = appDesc.GetLocalURL();
    pItem->SetProperty("app-localpath", appDesc.GetLocalPath());
    pItem->SetProperty("app-media", appDesc.GetMediaType());
    pItem->SetProperty("app-author", appDesc.GetAuthor());
    pItem->SetProperty("app-version", appDesc.GetVersion());
    pItem->SetProperty("description", appDesc.GetDescription());
    pItem->SetProperty("tags", appDesc.GetTagsStr());
    pItem->SetProperty("appid", appDesc.GetId());
    pItem->SetProperty("app-releasedate", appDesc.GetReleaseDate());
    pItem->SetAdult(appDesc.IsAdult());
    pItem->SetCountryRestriction(country, allow);
    availableAppsFileItems.Add(pItem);
  }
}

int CAppRepositories::RemoveRepositoryInstalledApps(CAppRepository& repo)
{
  CLog::Log(LOGDEBUG,"CAppRepositories::RemoveRepositoryInstalledApps - Enter function with repository [id=%s][name=%s] (repos)",repo.GetID().c_str(),repo.GetName().c_str());

  CAppDescriptor::AppDescriptorsMap repoAppDescs = repo.GetAvailableApps();

  int counter = 0;
  int removeInstalledAppsCounter = 0;

  if (repoAppDescs.size() > 0)
  {
    CAppDescriptor::AppDescriptorsMap::iterator it = repoAppDescs.begin();

    while (it != repoAppDescs.end())
    {
      counter++;

      CAppDescriptor appDesc = (*it).second;
      CStdString appId = appDesc.GetId();
      CStdString appName = appDesc.GetName();

      bool isInUserApp = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().IsInUserApplications(appId);

      if (isInUserApp)
      {
        // report to the server about the removed app
        bool succeeded = BoxeeUtils::ReportRemoveApp(appId);

        if (succeeded)
        {
          removeInstalledAppsCounter++;
          CLog::Log(LOGDEBUG,"CAppRepositories::RemoveRepositoryInstalledApps - Remove app [id=%s][name=%s] of repository [id=%s][name=%s] was SUCCESSFUL (repos)",appId.c_str(),appName.c_str(),repo.GetID().c_str(),repo.GetName().c_str());
        }
        else
        {
          CLog::Log(LOGERROR,"CAppRepositories::RemoveRepositoryInstalledApps - FAILED to remove app [id=%s][name=%s] of repository [id=%s][name=%s] (repos)",appId.c_str(),appName.c_str(),repo.GetID().c_str(),repo.GetName().c_str());
        }
      }

      it++;
    }
  }
  else
  {
    CLog::Log(LOGERROR,"CAppRepositories::RemoveRepositoryInstalledApps - FAILED to get AppsDesc of repository [id=%s][name=%s] (repos)",repo.GetID().c_str(),repo.GetName().c_str());
  }

  CLog::Log(LOGDEBUG,"CAppRepositories::RemoveRepositoryInstalledApps - Exit function with [removeInstalledApps=%d] of repository [id=%s][name=%s] (repos)",removeInstalledAppsCounter,repo.GetID().c_str(),repo.GetName().c_str());

  return removeInstalledAppsCounter;
}

bool CAppRepositories::UpdateServerOfRepositoriesFile()
{
  CStdString repositoriesFilePath = g_settings.GetProfileUserDataFolder();
  CUtil::AddFileToFolder(repositoriesFilePath, "repositories.xml", repositoriesFilePath);

  if (!XFILE::CFile::Exists(repositoriesFilePath))
  {
    CLog::Log(LOGDEBUG,"CAppRepositories::UpdateServerOfRepositoriesFile - Repositories file [%s] don't exist. No need to report to server (rtspf)", repositoriesFilePath.c_str());
    return true;
  }

  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(repositoriesFilePath.c_str()))
  {
    CLog::Log(LOGERROR,"CAppRepositories::UpdateServerOfRepositoriesFile - FAILED to load [RepositoriesFilePath=%s]. Exit and return FALSE (rtspf)",repositoriesFilePath.c_str());
    return false;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (!pRootElement)
  {
    CLog::Log(LOGERROR,"CAppRepositories::UpdateServerOfRepositoriesFile - FAILED to get root element from [RepositoriesFilePath=%s]. Exit and return FALSE (rtspf)",repositoriesFilePath.c_str());
    return false;
  }

  CStdString strValue = pRootElement->Value();
  if (strValue != "repositories")
  {
    CLog::Log(LOGERROR,"CAppRepositories::UpdateServerOfRepositoriesFile - Root element of [RepositoriesFilePath=%s] isn't <repositories> tag. Exit and return FALSE (rtspf)",repositoriesFilePath.c_str());
    return false;
  }

  if (pRootElement->Attribute("version"))
  {
    CLog::Log(LOGDEBUG,"CAppRepositories::UpdateServerOfRepositoriesFile - RootElement has attribute [version] -> It is a new file [%s] -> No need to report to server (rtspf)",repositoriesFilePath.c_str());
    return true;
  }

  std::vector<CAppRepository> repositories;

  const TiXmlNode *pChild = pRootElement->FirstChild("repository");
  while (pChild)
  {
    CAppRepository aRepository(CAppRepository::LoadRepositoryFromXML(pChild));
    repositories.push_back(aRepository);

    pChild = pChild->NextSiblingElement("repository");
  }

  int numOfRepositories = (int)repositories.size();

  CLog::Log(LOGDEBUG,"CAppRepositories::UpdateServerOfRepositoriesFile - Read [%d] repositories from [RepositoriesFilePath=%s]. Going to report them to the server (rtspf)",numOfRepositories,repositoriesFilePath.c_str());

  bool succeeded = true;

  if (numOfRepositories > 0)
  {
    succeeded &= BoxeeUtils::ReportInstalledRepositories(repositories);

    if (succeeded)
    {
      CLog::Log(LOGDEBUG,"CAppRepositories::UpdateServerOfRepositoriesFile - Succeeded to report repositories to server (rtspf)");

      // after report to server -> remove repositories from repositories.xml file

      for(size_t i=0; i<repositories.size(); i++)
      {
        bool retVal = Delete(repositories[i].GetID(), false);

        if (!retVal)
        {
          CLog::Log(LOGERROR,"CAppRepositories::UpdateServerOfRepositoriesFile - FAILED to delete repository [name=%s][id=%s][url=%s] from repositories.xml (rtspf)", repositories[i].GetName().c_str(), repositories[i].GetID().c_str(), repositories[i].GetURL().c_str());
        }
      }

      Save();
    }
    else
    {
      CLog::Log(LOGERROR,"CAppRepositories::UpdateServerOfRepositoriesFile - FAILED to report repositories to server (rtspf)");
    }
  }
  else
  {
    CLog::Log(LOGDEBUG,"CAppRepositories::UpdateServerOfRepositoriesFile - [NumOfRepositories=%d] -> No repositories to report (rtspf)",numOfRepositories);
  }

  if (succeeded)
  {
    // reload the repositories.xml file for edit
    if (!xmlDoc.LoadFile(repositoriesFilePath.c_str()))
    {
      CLog::Log(LOGERROR,"CSettings::UpdateServerOfRepositoriesFile - FAILED to reload [RepositoriesFilePath=%s] (rtspf)",repositoriesFilePath.c_str());
    }
    else
    {
      TiXmlElement* pRootElement = xmlDoc.RootElement();

      if (pRootElement)
      {
        pRootElement->SetAttribute("version",1);
        xmlDoc.SaveFile(repositoriesFilePath);
      }
      else
      {
        CLog::Log(LOGERROR,"CSettings::UpdateServerOfRepositoriesFile - FAILED to get RootElement from [RepositoriesFilePath=%s] in reload. [pRootElement=%p] (rtspf)",repositoriesFilePath.c_str(),pRootElement);
      }
    }
  }

  return succeeded;
}

CAppDescriptor::AppDescriptorsMap CAppRepositories::m_lastAvailableApps;
