
#include "SDL/SDL.h"
#ifdef _WIN32
#include "win32/PlatformDefs.h"
#else
#include "linux/PlatformDefs.h"
#endif
#include "Application.h"
#include "AppBoxDirectory.h"
#include "Util.h"
#include "utils/log.h"
#include "Settings.h"
#include "URL.h"
#include "FileItem.h"
#include "VirtualDirectory.h"
#include "Directory.h"
#include "../lib/libBoxee/boxee.h"
#include "BoxeeUtils.h"
#include "AppManager.h"
#include "File.h"

using namespace XFILE;
using namespace DIRECTORY;
using namespace BOXEE;

CAppBoxDirectory::CAppBoxDirectory()
{

}

CAppBoxDirectory::~CAppBoxDirectory()
{

}

bool CAppBoxDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CLog::Log(LOGDEBUG,"CAppBoxDirectory::GetDirectory - Enter function with [strPath=%s] (bapps)",strPath.c_str());

  CURL url(strPath);
  CStdString strProtocol = url.GetProtocol();

  if (strProtocol.CompareNoCase("appbox") != 0)
  {
    CLog::Log(LOGERROR,"CAppBoxDirectory::GetDirectory - Enter function without appbox:// protocol. [strPath=%s] (bapps)",strPath.c_str());
    return false;
  }

  return HandleAppBoxRequest(url, items);
}

bool CAppBoxDirectory::HandleAppBoxRequest(const CURL& url, CFileItemList &items)
{
  CLog::Log(LOGDEBUG,"CAppBoxDirectory::HandleAppBoxRequest - Enter function with [protocol=%s][host=%s][share=%s][FileName=%s][GetOptions=%s] (bapps)",url.GetProtocol().c_str(),url.GetHostName().c_str(),url.GetShareName().c_str(),url.GetFileName().c_str(),url.GetOptions().c_str());

  CStdString strType = "";

  CStdString repoId = "tv.boxee";

  // get all available apps
  CAppDescriptor::AppDescriptorsMap availableAppsDesc;

  CStdString shareName = url.GetShareName();
  if (shareName == "repository")
  {
    // there is a repository -> get all repository id from the path
    // there is a repository -> get the Repository id

    std::map<CStdString, CStdString> mapOptions;
    mapOptions = url.GetOptionsAsMap();

    CStdString strPath;
    url.GetURL(strPath);
    CLog::Log(LOGDEBUG,"CAppBoxDirectory::HandleAppBoxRequest - For [strPath=%s] mapOptions size is [%d]. [options=%s] (bapps)",strPath.c_str(),(int)mapOptions.size(),url.GetProtocolOptions().c_str());

    // get the Repository id
    std::map<CStdString, CStdString>::iterator it = mapOptions.find("id");
    if (it != mapOptions.end())
    {
      repoId = it->second;
      CLog::Log(LOGDEBUG,"CAppBoxDirectory::HandleAppBoxRequest - After getting [repoId=%s] (bapps)",repoId.c_str());
    }
    else
    {
      CStdString strPath;
      url.GetURL(strPath);
      CLog::Log(LOGERROR,"CAppBoxDirectory::HandleAppBoxRequest - FAILED to get repoid from [strPath=%s] (bapps)",strPath.c_str());
      return false;
    }
  }

  std::vector<CAppRepository> repositories = CAppManager::GetInstance().GetRepositories().Get();
  for (size_t i = 0; i < repositories.size(); i++)
  {
    CAppRepository& repository = repositories[i];

    CLog::Log(LOGDEBUG,"CAppBoxDirectory::HandleAppBoxRequest - Handling repository [name=%s][id=%s][url=%s] (erepo)",repository.GetName().c_str(),repository.GetID().c_str(),repository.GetURL().c_str());

    if (repoId == repository.GetID())
    {
      availableAppsDesc = repository.GetAvailableApps();
      break;
    }
  }

  if (availableAppsDesc.size() == 0)
  {
    CStdString strPath;
    url.GetURL(strPath);
    CLog::Log(LOGWARNING,"CAppBoxDirectory::HandleAppBoxRequest - FAILED to get apps descriptors. [strPath=%s] (bapps)",strPath.c_str());
    return false;
  }

  int numOfDescriptors = availableAppsDesc.size();
  if (numOfDescriptors < 1)
  {
    CStdString strPath;
    url.GetURL(strPath);
    CLog::Log(LOGWARNING,"CAppBoxDirectory::HandleAppBoxRequest - Got [%d] apps descriptors. [strPath=%s] (bapps)",numOfDescriptors,strPath.c_str());
    return true;
  }

  GetAppBoxDirectoryByType(availableAppsDesc, url, items);

  return true;
}

bool CAppBoxDirectory::GetAppBoxDirectoryByType(const CAppDescriptor::AppDescriptorsMap& availableAppsDesc, const CURL& url, CFileItemList &items)
{
  CStdString strType = "";

  if (url.GetHostName() != "all")
  {
    strType = url.GetHostName();
  }

  bool doSearch = false;
  CStdString strSearch = "";
  std::map<CStdString, CStdString> optionsMap = url.GetOptionsAsMap();
  if ((int)optionsMap.size() > 0)
  {
    strSearch = optionsMap["search"];
    if (!strSearch.IsEmpty())
    {
      strSearch.Trim();
      strSearch.ToLower();

      doSearch = true;
    }
  }

  CLog::Log(LOGDEBUG,"CAppBoxDirectory::GetAppBoxDirectoryByType - Enter function with [strType=%s][strSearch=%s] (bapps)",url.GetHostName().c_str(),strSearch.c_str());

  // filter all available apps by type and get it as FileItemList
  CFileItemList availableAppsItemList;

  CAppManager::GetInstance().GetRepositories().GetAvailableApps(availableAppsDesc, availableAppsItemList, strType);

  // need to filter out the apps that are not the user
  for(int i=0;i<availableAppsItemList.Size();i++)
  {
    CFileItemPtr appItem = availableAppsItemList.Get(i);
    CStdString appId = appItem->GetProperty("appid");

    if (doSearch)
    {
      CStdString appLabel = appItem->GetLabel();
      appLabel.Trim();
      appLabel.ToLower();

      std::set<CStdString> tagsSet;
      CStdString appTags = appItem->GetProperty("tags");
      if (!appTags.IsEmpty())
      {
        BoxeeUtils::StringTokenize(appItem->GetProperty("tags"),tagsSet,",",true,true);
      }

      // if strSearch isn't empty -> check if app label match
      if ((appLabel.Find(strSearch) == (-1)) && (tagsSet.find(strSearch) == tagsSet.end()))
      {
        continue;
      }
    }

    CFileItemPtr appItemToAdd(new CFileItem(*appItem));

    // add app-popularity of the app
    CStdString appPopularity = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetAppBoxPopularitiesById(appId);
    appItemToAdd->SetProperty("app-popularity",appPopularity);

    // if installed -> add IsInstalled property
    bool isInUserApplications = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().IsInUserApplications(appId);
    if (isInUserApplications)
    {
      appItemToAdd->SetProperty("IsInstalled",true);
    }

    if (doSearch)
    {
      // add property "value" so it can be chose from the search results
      appItemToAdd->SetProperty("value",appItemToAdd->GetLabel());
    }

    items.Add(appItemToAdd);
  }

  return true;
}

