// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxappboxmanager
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BXAPPBOXMANAGER_H
#define BXAPPBOXMANAGER_H

#include "bxscheduletaskmanager.h"
#include "bxappboxapplications.h"
#include "bxappboxpopularities.h"
#include "bxappboxrepositories.h"
#include <SDL/SDL.h>

#define DEFAULT_APP_CATEGORY "LIBRARY" //should be the same as id of the default section the server gives us

namespace BOXEE
{

class AppCategoryItem
{
public:
  AppCategoryItem()
  {
    m_Id = DEFAULT_APP_CATEGORY;
    m_Text = m_Id;
  };
  virtual ~AppCategoryItem(){};

  std::string m_Id;
  std::string m_Text;
};

class BXAppBoxManager
{
public:
  BXAppBoxManager();
  virtual ~BXAppBoxManager();

  bool Initialize();

  ///////////////////////////
  // BoxeeApplicationsList //
  ///////////////////////////

  bool UpdateAppBoxBoxeeApplicationsList(unsigned long executionDelayInMS, bool repeat);
  bool GetAppBoxBoxeeApplications(TiXmlDocument& boxeeApplicationsList);
  bool IsAppIdInAppBoxApplicationsList(const std::string& id);

  //////////////////////////////
  // 3rdPartyApplicationsList //
  //////////////////////////////

  bool GetAppBox3rdPartyRepositoryApplications(const std::string& repositoryId, TiXmlDocument& appBox3rdPartyRepositoryApplicationsList, bool bDontWait);

  //////////////////////
  // PopularitiesList //
  //////////////////////

  bool UpdateAppBoxPopularitiesList(unsigned long executionDelayInMS, bool repeat);
  std::string GetAppBoxPopularityById(const std::string& id);

  //////////////////////
  // RepositoriesList //
  //////////////////////

  bool UpdateAppBoxRepositoriesList(unsigned long executionDelayInMS, bool repeat);
  bool GetRepositories(BXAppBoxRepositories& repositoriesList);

  //////////////////////
  // Apps Categories  //
  //////////////////////

  bool UpdateAppsCategoriesList(unsigned long executionDelayInMS, bool repeat);
  bool GetAppsCategories(std::vector<AppCategoryItem>& categoryList);
  std::string GetAppsCategoryLabel(const std::string& id);

private:

  ///////////////////////////
  // BoxeeApplicationsList //
  ///////////////////////////

  void LockAppBoxBoxeeApplicationsList();
  void UnLockAppBoxBoxeeApplicationsList();
  void CopyAppBoxBoxeeApplicationsList(const BXAppBoxApplications& appBoxBoxeeApplicationsList);
  void SetAppBoxBoxeeApplicationsListIsLoaded(bool isLoaded);

  SDL_mutex* m_appBoxBoxeeApplicationsListGuard;
  BXAppBoxApplications m_appBoxBoxeeApplicationsList;

  class RequestAppBoxBoxeeApplicationsListFromServerTask : public BoxeeScheduleTask
  {
  public:
    
    RequestAppBoxBoxeeApplicationsListFromServerTask(BXAppBoxManager* taskHandler, unsigned long executionDelayInMS, bool repeat);
    virtual ~RequestAppBoxBoxeeApplicationsListFromServerTask();
    virtual void DoWork();
    
  private:
    
    BXAppBoxManager* m_taskHandler;
  };

  //////////////////////////////
  // 3rdPartyApplicationsList //
  //////////////////////////////

  void LockAppBox3rdPartyRepositoriesApplicationsList();
  void UnLockAppBox3rdPartyRepositoriesApplicationsList();
  void CopyAppBox3rdPartyRepositoriesApplicationsList(std::map<std::string, TiXmlDocument>& appBox3rdPartyApplicationsMap);
  void SetAppBox3rdPartyRepositoriesApplicationsListIsLoaded(bool isLoaded);

  SDL_mutex* m_appBox3rdPartyRepositoriesApplicationsListGuard;
  std::map<std::string, TiXmlDocument> m_appBox3rdPartyRepositoriesApplicationsMap;
  bool m_is3rdPartyRepositoriesMapLoaded;

  //////////////////////
  // PopularitiesList //
  //////////////////////

  void LockAppBoxPopularitiesList();
  void UnLockAppBoxPopularitiesList();
  void CopyAppBoxPopularitiesList(const BXAppBoxPopularities& appBoxPopularitiesList);
  void SetAppBoxPopularitiesApplicationsListIsLoaded(bool isLoaded);

  SDL_mutex* m_appBoxPopularitiesListGuard;
  BXAppBoxPopularities m_appBoxPopularitiesList;

  class RequestAppBoxPopularitiesListFromServerTask : public BoxeeScheduleTask
  {
  public:

    RequestAppBoxPopularitiesListFromServerTask(BXAppBoxManager* taskHandler, unsigned long executionDelayInMS, bool repeat);
    virtual ~RequestAppBoxPopularitiesListFromServerTask();
    virtual void DoWork();

  private:

    BXAppBoxManager* m_taskHandler;
  };

  //////////////////////
  // RepositoriesList //
  //////////////////////

  void LockAppBoxRepositoriesList();
  void UnLockAppBoxRepositoriesList();
  void CopyAppBoxRepositoriesList(const BXAppBoxRepositories& appBoxRepositoriesList);
  void SetAppBoxRepositoriesListIsLoaded(bool isLoaded);

  SDL_mutex* m_appBoxRepositoriesListGuard;
  BXAppBoxRepositories m_appBoxRepositoriesList;

  class RequestAppBoxRepositoriesListFromServerTask : public BoxeeScheduleTask
  {
  public:

    RequestAppBoxRepositoriesListFromServerTask(BXAppBoxManager* taskHandler, unsigned long executionDelayInMS, bool repeat);
    virtual ~RequestAppBoxRepositoriesListFromServerTask();
    virtual void DoWork();

  private:

    void BuildRepositoriesApplicationsList(const BXAppBoxRepositories& appBoxRepositoriesList, std::map<std::string, TiXmlDocument>& appBox3rdPartyRepositoriesApplicationsMap);

    BXAppBoxManager* m_taskHandler;
  };

  //////////////////////
  // Apps Categories  //
  //////////////////////
  void LockAppsCategoriesList();
  void UnLockAppsCategoriesList();
  void CopyAppsCategoriesList(const std::vector<AppCategoryItem>& appCategoriesList);

  SDL_mutex* m_appsCategoriesListGuard;
  std::vector<AppCategoryItem> m_appCategoryList;

  class RequestAppsCategoriesListFromServerTask : public BoxeeScheduleTask
  {
  public:

    RequestAppsCategoriesListFromServerTask(BXAppBoxManager* taskHandler, unsigned long executionDelayInMS, bool repeat);
    virtual ~RequestAppsCategoriesListFromServerTask();
    virtual void DoWork();

  private:

    BXAppBoxManager* m_taskHandler;
  };
};

}

#endif
