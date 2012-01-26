
#include "SDL/SDL.h"
#ifdef _WIN32
#include "win32/PlatformDefs.h"
#else
#include "linux/PlatformDefs.h"
#endif
#include "Application.h"
#include "AppsDirectory.h"
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
#include "Util.h"
#include "ZipManager.h"
#include "AppSecurity.h"
#include "SpecialProtocol.h"
#include "MediaManager.h"

#define BOXEE_EX_APPS_FOLDER "BoxeeApps"

using namespace XFILE;
using namespace DIRECTORY;
using namespace BOXEE;

CAppsDirectory::CAppsDirectory()
{

}

CAppsDirectory::~CAppsDirectory()
{

}

bool CAppsDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CLog::Log(LOGDEBUG,"CAppsDirectory::GetDirectory - Enter function with [strPath=%s] (bapps)",strPath.c_str());

  CURI url(strPath);
  CStdString strProtocol = url.GetProtocol();

  if (strProtocol.CompareNoCase("apps") != 0)
  {
    CLog::Log(LOGERROR,"CAppsDirectory::GetDirectory - Enter function without apps:// protocol. [strPath=%s] (bapps)",strPath.c_str());
    return false;
  }

  return HandleAppRequest(url, items);
}

bool CAppsDirectory::HandleAppRequest(const CURI& url, CFileItemList &items)
{
  CStdString strPath = url.Get();
  CStdString strExtLink="";

  if (url.GetOptions().size() > 0)
  {
    std::map<CStdString, CStdString> mapOptions;
    mapOptions = url.GetOptionsAsMap();

    std::map<CStdString, CStdString>::iterator it = mapOptions.find("external-link");

    if (it != mapOptions.end())
    {
      strExtLink = it->second;
    }
  }
  // get all available apps only if we're not in external app mode
  if (strExtLink.size() <= 0)
  {
    const CAppDescriptor::AppDescriptorsMap availableAppsDesc = CAppManager::GetInstance().GetRepositories().GetAvailableApps(true,false);

    int numOfDescriptors = availableAppsDesc.size();

    CLog::Log(LOGWARNING,"CAppsDirectory::HandleAppRequest - Got [%d] apps descriptors. [strPath=%s] (bapps)",numOfDescriptors,strPath.c_str());

    if (numOfDescriptors < 1)
    {
      CLog::Log(LOGWARNING,"CAppsDirectory::HandleAppRequest - Since we got [numOfDescriptors=%d] not going to handle the request. [strPath=%s] (bapps)",numOfDescriptors,strPath.c_str());
      return true;
    }

    GetAppDirectoryByType(availableAppsDesc, url, items);
  }
  else
  {
    const CAppDescriptor::AppDescriptorsMap emptyAppsDesc;
    GetAppDirectoryByType(emptyAppsDesc, url, items , strExtLink);
  }

  return true;
}

bool CAppsDirectory::GetAppDirectoryByType(const CAppDescriptor::AppDescriptorsMap& availableAppsDesc, const CURI& url, CFileItemList &items, const CStdString& strExtLink/* ="" */)
  {
  CStdString strType = "";

  if (url.GetHostName() != "all")
  {
    strType = url.GetHostName();
  }

  bool doSearch = false;
  CStdString strSearchTerm = "";
  std::map<CStdString, CStdString> optionsMap = url.GetOptionsAsMap();
  if ((int)optionsMap.size() > 0)
  {
    strSearchTerm = optionsMap["term"];

    if (!strSearchTerm.IsEmpty())
    {
      strSearchTerm.Trim();
      strSearchTerm.ToLower();

      doSearch = true;
    }
  }

  CLog::Log(LOGDEBUG,"CAppsDirectory::GetAppDirectoryByType - Enter function with [strType=%s][strSearch=%s] (bapps)",url.GetHostName().c_str(),strSearchTerm.c_str());

  std::set<CStdString> userInstalledAppsSet;

  // filter all available apps by type and get it as FileItemList
  CFileItemList availableAppsItemList;

  if (strExtLink.size() <= 0)
  {
    //fetch the server only if we're not in external app mode
    CAppManager::GetInstance().GetRepositories().GetAvailableApps(availableAppsDesc, availableAppsItemList, strType);
    CAppManager::GetInstance().RefreshAppsStats();
  }

  // need to filter out the apps that are not the user
  for(int i=0;i<availableAppsItemList.Size();i++)
  {
    CFileItemPtr appItem = availableAppsItemList.Get(i);
    CStdString appId = appItem->GetProperty("appid");

    bool isInUserApplications = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().IsInUserApplications(appId);

    // in case of app:// request -> add only apps that ARE in the user list

    if(isInUserApplications)
    {
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
        if ((appLabel.Find(strSearchTerm) == (-1)) && (tagsSet.find(strSearchTerm) == tagsSet.end()))
        {
          continue;
        }
      }

      CFileItemPtr appItemToAdd(new CFileItem(*appItem));

      // add app-popularity of the app
      CStdString appPopularity = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetAppBoxPopularitiesById(appId);
      appItemToAdd->SetProperty("app-popularity",appPopularity);
      int appLastOpenedDate = CAppManager::GetInstance().GetAppLastOpenedDate(appId);
      appItemToAdd->SetProperty("app-last-used", appLastOpenedDate);
      int appUsageCounter = CAppManager::GetInstance().GetAppTimesOpened(appId);
      appItemToAdd->SetProperty("app-usage", appUsageCounter);

      bool isAllowed = appItemToAdd->IsAllowed();
      if (isAllowed)
      {
        items.Add(appItemToAdd);
        userInstalledAppsSet.insert(appItemToAdd->GetProperty("app-localpath"));
        CLog::Log(LOGDEBUG,"CAppsDirectory::GetDirectoryByType - [%d/%d] - After adding [locaPath=%s] to set. [SetSize=%d] (bapps)",i+1,availableAppsItemList.Size(),appItemToAdd->GetProperty("app-localpath").c_str(),(int)userInstalledAppsSet.size());
      }
      else
      {
        CLog::Log(LOGDEBUG,"CAppsDirectory::GetDirectoryByType - [%d/%d] - NOT adding app [label=%s] since [IsAllowed=%d] (bapps)",i+1,availableAppsItemList.Size(),appItemToAdd->GetLabel().c_str(),isAllowed);
      }
    }
  }

  // in case of app:// request and there are test application -> add them to the user list

  int numOfApps = items.Size();

  CLog::Log(LOGDEBUG,"CAppsDirectory::GetDirectoryByType - Num of apps found [%d] (bapps)",numOfApps);

  if (!doSearch)
  {
    // if not in search -> add TestApps
    
    if (strExtLink.empty())
    {
      //genereate external links and read specific folder
      CLog::Log(LOGDEBUG,"CAppsDirectory::GetAppDirectoryByType, going to call AddTestApps searching for apps in user's lib");
      AddTestApps(userInstalledAppsSet,items);

      VECSOURCES drives;
      g_mediaManager.GetRemovableDrives(drives);

      VECSOURCES::iterator it = drives.begin();

      for (; it != drives.end() ; it++)
      {
        CStdString path = it->strPath;
        CUtil::AddSlashAtEnd(path);
        path+=BOXEE_EX_APPS_FOLDER;
        CUtil::AddSlashAtEnd(path);
        CLog::Log(LOGDEBUG,"CAppsDirectory::GetAppDirectoryByType, going to call AddTestApps on path: %s",path.c_str());
        AddTestApps(userInstalledAppsSet,items,path,true);
      }
    }
    else
    {
      CLog::Log(LOGDEBUG,"CAppsDirectory::GetAppDirectoryByType, going to call AddTestApps on path: %s",strExtLink.c_str());
      AddTestApps(userInstalledAppsSet,items,strExtLink);
    }
  }

  return true;
}


bool CAppsDirectory::VerifyDeviceSignature(const CFileItemPtr& appItem, const CStdString& path)
{
  bool bVerified = false;

#ifdef HAS_EMBEDDED
  do
  {
    std::vector<CStdString> vecFindResults;
    CStdString certFile;

    // search for device cert
    if(!CUtil::FindFile("dev-certificate.xml", path, vecFindResults))
    {
      CLog::Log(LOGERROR,"CAppsDirectory::%s - Cannot find device-certificate.xml in [%s]", __func__, path.c_str());

    }
    else
    {
      certFile = vecFindResults[0];
    }

    if(certFile.IsEmpty())
    {
      break;
    }

    CLog::Log(LOGINFO, "CAppsDirectory::%s - Found device certificate [%s]", __func__, certFile.c_str());

    CAppSecurity appSecurity(appItem->m_strPath, certFile);

    if(!appSecurity.VerifySignature(FLAG_TEST_APP))
    {
      CLog::Log(LOGERROR,"CAppsDirectory::%s - FAILED to verify app [%s], incorrect signature", __func__, appItem->m_strPath.c_str());
      break;
    }

    bVerified = true;
  
    CLog::Log(LOGINFO, "Successfully verified app [%s]", appItem->m_strPath.c_str());

  } while(false);
#endif

  return bVerified;
}

bool CAppsDirectory::AddTestApps(std::set<CStdString> userInstalledAppsSet, CFileItemList &items ,const CStdString& path /*="special://home/apps"*/, bool bOnRemovableStorage)
{
  CFileItemList appItems;
  DIRECTORY::CDirectory::GetDirectory(path, appItems);

  std::set<CStdString>::iterator it;

  for (int i = 0; i < appItems.Size(); i++)
  {
    CFileItemPtr appItem = appItems.Get(i);
    CStdString appPath = appItem->m_strPath;
    CUtil::RemoveSlashAtEnd(appPath);

    CLog::Log(LOGDEBUG,"CAppsDirectory::AddTestApps - [%d/%d] - Check if app [%s] is already install (testapp)",i+1,appItems.Size(),appPath.c_str());

    ////////////////////////////////////////////////////////////////
    // check if this appItem is of an app that the user installed //
    ////////////////////////////////////////////////////////////////

    it=userInstalledAppsSet.find(appPath);

    if(it != userInstalledAppsSet.end())
    {
      CLog::Log(LOGDEBUG,"CAppsDirectory::AddTestApps - [%d/%d] - App [%s] is already install -> continue (testapp)",i+1,appItems.Size(),appPath.c_str());

      // this app is installed -> it is not a test one
      continue;
    }

    CLog::Log(LOGDEBUG,"CAppsDirectory::AddTestApps - [%d/%d] - App [%s] isn't installed -> check if TestApp (testapp)",i+1,appItems.Size(),appPath.c_str());

    ///////////////////////////////////////////////////////////////////////////////////////
    // This appItem is of an app that the user didn't installed -> check if it a TestApp //
    ///////////////////////////////////////////////////////////////////////////////////////

    CStdString appLocalPath = appItem->m_strPath;
    CStdString descFilePath = (appLocalPath + "descriptor.xml");

    if (!XFILE::CFile::Exists(descFilePath))
    {
      CLog::Log(LOGDEBUG,"CAppsDirectory::AddTestApps - FAILED to find [descriptor.xml] under [%s] -> continue (testapp)",appItem->m_strPath.c_str());
      continue;
    }

    TiXmlDocument xmlDoc;
    if (!xmlDoc.LoadFile(descFilePath))
    {
      CLog::Log(LOGDEBUG,"CAppsDirectory::AddTestApps - FAILED to load [%s] -> continue (testapp)",descFilePath.c_str());
      continue;
    }

    CAppDescriptor appDesc(xmlDoc.RootElement());

    if (appDesc.IsTestApp())
    {
#ifdef HAS_EMBEDDED
      if(bOnRemovableStorage)
      {
        bool bVerified = false;
        CURI url(appItem->m_strPath);

        CStdString protocol = url.GetProtocol();

        if(appItem->m_bIsFolder && !(protocol == "zip" || protocol == "rar"))
        {
          bVerified = VerifyDeviceSignature(appItem, path);
        }

        if(!bVerified)
        {
          continue;
        }
      }
#endif
      ////////////////////////////////////////////////////////
      // This appItem is of a TestApp -> add it to the list //
      ////////////////////////////////////////////////////////

      CStdString country;
      bool allow;
      appDesc.GetCountryRestrictions(country, allow);

      CFileItemPtr pItem(new CFileItem());
      pItem->SetLabel(appDesc.GetName());
      pItem->SetThumbnailImage(appDesc.GetThumb());
      pItem->m_strPath = appDesc.GetLocalURL();
      pItem->SetProperty("testapp", appDesc.IsTestApp());
      pItem->SetProperty("app-localpath", appDesc.GetLocalPath());
      pItem->SetProperty("app-media", appDesc.GetMediaType());
      pItem->SetProperty("app-author", appDesc.GetAuthor());
      pItem->SetProperty("app-version", appDesc.GetVersion());
      pItem->SetProperty("description", appDesc.GetDescription());
      pItem->SetProperty("tags", appDesc.GetTagsStr());
      pItem->SetProperty("appid", appDesc.GetId());
      pItem->SetProperty("app-releasedate", appDesc.GetReleaseDate());
      pItem->SetProperty("actual-path" , appLocalPath);
      pItem->SetAdult(appDesc.IsAdult());
      pItem->SetCountryRestriction(country, allow);
      items.Add(pItem);

      CLog::Log(LOGDEBUG,"CAppsDirectory::AddTestApps - [%d/%d] - TestApp [localPath=%s][localUrl=%s] was added. [IsTestApp=%d] (testapp)",i+1,appItems.Size(),pItem->GetProperty("app-localpath").c_str(),pItem->m_strPath.c_str(),appDesc.IsTestApp());
    }
    else
    {
      CLog::Log(LOGDEBUG,"CAppsDirectory::AddTestApps - [%d/%d] - App [localPath=%s][localUrl=%s] ISN'T a TestApp. [IsTestApp=%d] (testapp)",i+1,appItems.Size(),appDesc.GetLocalPath().c_str(),appDesc.GetLocalURL().c_str(),appDesc.IsTestApp());
    }
  }

  return true;
}
