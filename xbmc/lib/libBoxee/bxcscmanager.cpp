//
// C++ Implementation: bxbgprocess
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxcscmanager.h"
#include "bxscheduletaskmanager.h"
#include "boxee.h"
#include "bxconfiguration.h"
#include "bxexceptions.h"

namespace BOXEE
{

#define RECOMMENDATION_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS    600000 // 10 minutes
#define QUEUE_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS             600000 // 10 minutes
#define FRIENDS_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS           600000 // 10 minutes
#define APPLICATIONS_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS      600000 // 10 minutes
#define SUBSCRIPTIONS_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS     600000 // 10 minutes
#define WEB_FAVORITES_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS     600000 // 10 minutes
#define SERVICES_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS          600000 // 10 minutes
#define ENTITLEMENTS_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS      600000 // 10 minutes
#define SOURCES_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS           600000 // 10 minutes
#define FEATURED_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS          21600000 // 6h
#define APPBOX_APPS_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS       600000 // 10 minutes
#define APPBOX_POPU_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS       600000 // 10 minutes
#define APPBOX_REPOS_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS      600000 // 10 minutes
#define APPBOX_CATEGORY_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS   600000 // 10 minutes
#define TRAILERS_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS          86400000 // 24h


CBoxeeClientServerComManager::CBoxeeClientServerComManager()
{
  
}

CBoxeeClientServerComManager::~CBoxeeClientServerComManager()
{
  
}

bool CBoxeeClientServerComManager::Initialize()
{
  bool succeeded = false;
  
  m_recommendationsManager.Initialize();
  m_queueManager.Initialize();
  m_friendsManager.Initialize();
  m_userApplicationsManager.Initialize();
  m_subscriptionsManager.Initialize();
  m_servicesManager.Initialize();
  m_entitlementsManager.Initialize();
  m_sourcesManager.Initialize();
  m_featuredManager.Initialize();
  m_appBoxApplicationsManager.Initialize();
  m_genresManager.Initialize();
  m_webFavoritesManager.Initialize();

  //////////////////////////////////////
  // Add tasks to ScheduleTaskManager //
  //////////////////////////////////////
  
  succeeded = InitializeUserLists();
  if(!succeeded)
  {
    LOG(LOG_LEVEL_ERROR,"CBoxeeClientServerComManager::Initialize - Call to initializeUserListsNow() FAILED (bcscm)");
    
    // Quit ???
  }

  succeeded = AddUserListsToScheduleTaskManager();
  if(!succeeded)
  {
    LOG(LOG_LEVEL_ERROR,"CBoxeeClientServerComManager::Initialize - Call to initializeUserListsNow() FAILED (bcscm)");
    
    // Quit ???
  }
 
  return true;
}

bool CBoxeeClientServerComManager::Deinitialize()
{
  return true;
}

bool CBoxeeClientServerComManager::InitializeUserLists()
{
  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::InitializeUserListsNow - Enter function (bcscm)");

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::InitializeUserListsNow - Going to initialize RECOMMENDATION (bcscm)(rec)");
  m_recommendationsManager.UpdateRecommendationsList(0,false);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::InitializeUserListsNow - Going to initialize QUEUE (bcscm)(queue)");
  m_queueManager.UpdateQueueList(0,false);

  //LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::InitializeUserListsNow - Going to initialize FRIEND (bcscm)(friends)");
  //m_friendsManager.UpdateFriendsList(0,false);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::InitializeUserListsNow - Going to initialize USER-APPS (bcscm)(apps)");
  m_userApplicationsManager.UpdateApplicationsList(0,false);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::InitializeUserListsNow - Going to initialize SUBSCRIPTION (bcscm)(subs)");
  m_subscriptionsManager.UpdateSubscriptionsList(0,false);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::InitializeUserListsNow - Going to initialize WEBFAVORITES (bcscm)(subs)");
  m_webFavoritesManager.UpdateWebFavoritesList(0,false);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::InitializeUserListsNow - Going to initialize SERVICES (bcscm)(serv)");
  m_servicesManager.UpdateServicesList(0,false);

  //LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::InitializeUserListsNow - Going to initialize ENTITLEMENTS (bcscm)(entitle)");
  //m_entitlementsManager.UpdateEntitlementsList(0,false);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::InitializeUserListsNow - Going to initialize SOURCES (bcscm)(source)");
  m_sourcesManager.UpdateSourcesList(0,false);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::InitializeUserListsNow - Going to initialize FEATURED (bcscm)(feat)");
  m_featuredManager.UpdateFeaturedList(0,false);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::InitializeUserListsNow - Going to initialize APPBOX-APPS_LIST (bcscm)(appbox)");
  m_appBoxApplicationsManager.UpdateAppBoxBoxeeApplicationsList(0,false);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::InitializeUserListsNow - Going to initialize APPBOX-POPU_LIST (bcscm)(appbox)");
  m_appBoxApplicationsManager.UpdateAppBoxPopularitiesList(0,false);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::InitializeUserListsNow - Going to initialize APPBOX-REPOS_LIST (bcscm)(repos)");
  m_appBoxApplicationsManager.UpdateAppBoxRepositoriesList(0,false);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::InitializeUserListsNow - Going to initialize APPBOX-CATEGORIES_LIST (bcscm)(appcategories)");
  m_appBoxApplicationsManager.UpdateAppsCategoriesList(0,false);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::InitializeUserListsNow - Going to initialize APPBOX-REPOS_LIST (bcscm)(genres)");
  m_genresManager.UpdateGenres(0,false);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::InitializeUserListsNow - Going to initialize MOVIE_TRAILER_LIST (bcscm)(trailers)");
  m_trailerSectionsManager.UpdateTrailerSections(0,false);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::InitializeUserListsNow - Going to initialize EXCLUDED SOURCES (bcscm)(cf)");
  m_sourcesManager.UpdateExcludedSources(0,false);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::InitializeUserListsNow - Exit function. Going to return TRUE (bcscm)");

  return true;
}

bool CBoxeeClientServerComManager::AddUserListsToScheduleTaskManager()
{
  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::AddUserListsToScheduleTaskManager - Enter function (bcscm)");

  unsigned long interval = 0;
  
  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::AddUserListsToScheduleTaskManager - Going to add RECOMMENDATION task (bcscm)(rec)");
  interval = BOXEE::BXConfiguration::GetInstance().GetIntParam("Boxee.RecommendationsListFreqInMs",RECOMMENDATION_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS);
  m_recommendationsManager.UpdateRecommendationsList(interval,true);
  
  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::AddUserListsToScheduleTaskManager - Going to add QUEUE (bcscm)(queue)");
  interval = BOXEE::BXConfiguration::GetInstance().GetIntParam("Boxee.QueueListFreqInMs",QUEUE_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS);
  m_queueManager.UpdateQueueList(interval,true);

  //LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::AddUserListsToScheduleTaskManager - Going to add FRIEND (bcscm)(friends)");
  //interval = BOXEE::BXConfiguration::GetInstance().GetIntParam("Boxee.FriendsListFreqInMs",FRIENDS_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS);
  //m_friendsManager.UpdateFriendsList(interval,true);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::AddUserListsToScheduleTaskManager - Going to add APPS (bcscm)(apps)");
  interval = BOXEE::BXConfiguration::GetInstance().GetIntParam("Boxee.ApplicationsListFreqInMs",APPLICATIONS_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS);
  m_userApplicationsManager.UpdateApplicationsList(interval,true);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::AddUserListsToScheduleTaskManager - Going to add SUBSCRIPTION (bcscm)(subs)");
  interval = BOXEE::BXConfiguration::GetInstance().GetIntParam("Boxee.SubscriptionsListFreqInMs",SUBSCRIPTIONS_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS);
  m_subscriptionsManager.UpdateSubscriptionsList(interval,true);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::AddUserListsToScheduleTaskManager - Going to add WEBFAVORITES (bcscm)(subs)");
  interval = BOXEE::BXConfiguration::GetInstance().GetIntParam("Boxee.SubscriptionsListFreqInMs",WEB_FAVORITES_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS);
  m_webFavoritesManager.UpdateWebFavoritesList(interval,true);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::AddUserListsToScheduleTaskManager - Going to add SERVICES (bcscm)(serv)");
  interval = BOXEE::BXConfiguration::GetInstance().GetIntParam("Boxee.ServicesListFreqInMs",SERVICES_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS);
  m_servicesManager.UpdateServicesList(interval,true);

  //LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::AddUserListsToScheduleTaskManager - Going to add ENTITLEMENTS (bcscm)(entitle)");
  //interval = BOXEE::BXConfiguration::GetInstance().GetIntParam("Boxee.EntitlementsListFreqInMs",ENTITLEMENTS_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS);
  //m_entitlementsManager.UpdateEntitlementsList(interval,true);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::AddUserListsToScheduleTaskManager - Going to add SOURCES (bcscm)(entitle)");
  interval = BOXEE::BXConfiguration::GetInstance().GetIntParam("Boxee.SourcesListFreqInMs",SOURCES_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS);
  m_sourcesManager.UpdateSourcesList(interval,true);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::AddUserListsToScheduleTaskManager - Going to add FEATURED (bcscm)(feat)");
  interval = BOXEE::BXConfiguration::GetInstance().GetIntParam("Boxee.FeaturedListFreqInMs",FEATURED_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS);
  m_featuredManager.UpdateFeaturedList(interval,true);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::AddUserListsToScheduleTaskManager - Going to add APPBOX-APPS_LIST (bcscm)(appbox)");
  interval = BOXEE::BXConfiguration::GetInstance().GetIntParam("Boxee.AppBoxApplicationsListFreqInMs",APPBOX_APPS_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS);
  m_appBoxApplicationsManager.UpdateAppBoxBoxeeApplicationsList(interval,true);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::AddUserListsToScheduleTaskManager - Going to add APPBOX-POPU_LIST (bcscm)(appbox)");
  interval = BOXEE::BXConfiguration::GetInstance().GetIntParam("Boxee.AppBoxPopularitiesListFreqInMs",APPBOX_POPU_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS);
  m_appBoxApplicationsManager.UpdateAppBoxPopularitiesList(interval,true);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::AddUserListsToScheduleTaskManager - Going to add APPBOX-REPOS_LIST (bcscm)(repos)");
  interval = BOXEE::BXConfiguration::GetInstance().GetIntParam("Boxee.AppBoxRepositoriesListFreqInMs",APPBOX_REPOS_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS);
  m_appBoxApplicationsManager.UpdateAppBoxRepositoriesList(interval,true);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::AddUserListsToScheduleTaskManager - Going to add APPBOX-CATEGORIES (bcscm)(appcategories)");
  interval = BOXEE::BXConfiguration::GetInstance().GetIntParam("Boxee.AppBoxCategoryListFreqInMs",APPBOX_CATEGORY_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS);
  m_appBoxApplicationsManager.UpdateAppsCategoriesList(interval,true);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::AddUserListsToScheduleTaskManager - Going to add MOVIE_TRAILERS_LIST (bcscm)(trailers)");
  interval = BOXEE::BXConfiguration::GetInstance().GetIntParam("Boxee.MovieTrailersListFreqInMs",TRAILERS_LIST_REQUEST_FROM_SERVER_FREQ_IN_MS);
  m_trailerSectionsManager.UpdateTrailerSections(interval,true);

  LOG(LOG_LEVEL_DEBUG,"CBoxeeClientServerComManager::AddUserListsToScheduleTaskManager - Exit function. Going to return TRUE (bcscm)");

  return true;

}

bool CBoxeeClientServerComManager::AddRequestFromServer(BoxeeScheduleTask& requestTask)
{
  Boxee::GetInstance().GetBoxeeScheduleTaskManager().AddScheduleTask(&requestTask);
  return true;  
}

bool CBoxeeClientServerComManager::UpdateAllUserListsNow()
{
  return InitializeUserLists();
}

/////////////////////
// Recommendations //
/////////////////////

bool CBoxeeClientServerComManager::GetRecommendations(BXBoxeeFeed& recommendationsList)
{
  return m_recommendationsManager.GetRecommendationsList(recommendationsList);
}

int CBoxeeClientServerComManager::GetRecommendationsSize()
{
  return m_recommendationsManager.GetRecommendationsListSize();
}

///////////
// Queue //
///////////

bool CBoxeeClientServerComManager::GetQueue(BXBoxeeFeed& queueList, CQueueItemsType::QueueItemsTypeEnums queueType)
{
  return m_queueManager.GetQueueList(queueList,queueType);
}

int CBoxeeClientServerComManager::GetQueueSize(CQueueItemsType::QueueItemsTypeEnums queueType)
{
  return m_queueManager.GetQueueSize(queueType);
}

void CBoxeeClientServerComManager::SetValidQueueSize(int validQueueSize)
{
  return m_queueManager.SetValidQueueSize(validQueueSize);
}

int CBoxeeClientServerComManager::GetValidQueueSize()
{
  return m_queueManager.GetValidQueueSize();
}

bool CBoxeeClientServerComManager::IsInQueue(const std::string& boxeeId, const std::string& path)
{
  return m_queueManager.IsInQueueList(boxeeId,path);
}

bool CBoxeeClientServerComManager::IsInQueue(const std::string& boxeeId, const std::string& path, std::string& referral)
{
  return m_queueManager.IsInQueueList(boxeeId,path,referral);
}

bool CBoxeeClientServerComManager::UpdateQueueNow()
{
  return UpdateQueue(0,false);
}

bool CBoxeeClientServerComManager::UpdateQueue(unsigned long executionDelayInMS, bool repeat)
{
  return m_queueManager.UpdateQueueList(executionDelayInMS,repeat);
}

/////////////
// Friends //
/////////////

bool CBoxeeClientServerComManager::GetFriends(BXFriendsList& friendsList, const std::string& user, time_t listTimeStamp)
{
  //return m_friendsManager.GetFriendsList(friendsList,user,listTimeStamp);
  return true;
}

//////////////////
// Applications //
//////////////////

bool CBoxeeClientServerComManager::GetUserApplications(BXBoxeeApplications& applicationsList)
{
  return m_userApplicationsManager.GetApplications(applicationsList);
}

bool CBoxeeClientServerComManager::GetUserApplications(std::vector<BXApplicationItem>& applicationVec)
{
  return m_userApplicationsManager.GetApplications(applicationVec);
}

bool CBoxeeClientServerComManager::IsInUserApplications(const std::string& id)
{
  return m_userApplicationsManager.IsInApplications(id);
}

bool CBoxeeClientServerComManager::UpdateUserApplicationsListNow()
{
  return UpdateUserApplicationsList(0,false);
}

bool CBoxeeClientServerComManager::UpdateGenresListNow()
{
  return m_genresManager.UpdateGenres(0,false);
}


bool CBoxeeClientServerComManager::UpdateUserApplicationsList(unsigned long executionDelayInMS, bool repeat)
{
  return m_userApplicationsManager.UpdateApplicationsList(executionDelayInMS,repeat);
}

///////////////////
// Subscriptions //
///////////////////

bool CBoxeeClientServerComManager::GetSubscriptions(BXBoxeeSubscriptions& subscriptionsList)
{
  return m_subscriptionsManager.GetSubscriptions(subscriptionsList);
}

bool CBoxeeClientServerComManager::GetSubscriptions(CSubscriptionType::SubscriptionTypeEnums subscriptionType, std::vector<BXSubscriptionItem>& subscriptionsVec)
{
  return m_subscriptionsManager.GetSubscriptions(subscriptionType,subscriptionsVec);
}

bool CBoxeeClientServerComManager::GetSubscriptionIds(CSubscriptionType::SubscriptionTypeEnums subscriptionType,std::vector<std::string>& subscriptionsIdsVec)
{
  return m_subscriptionsManager.GetSubscriptionIds(subscriptionType,subscriptionsIdsVec);
}

bool CBoxeeClientServerComManager::UpdateSubscriptionsListNow()
{
  return UpdateSubscriptionsList(0,false);
}

bool CBoxeeClientServerComManager::UpdateSubscriptionsList(unsigned long executionDelayInMS, bool repeat)
{
  return m_subscriptionsManager.UpdateSubscriptionsList(executionDelayInMS,repeat);
}


bool CBoxeeClientServerComManager::IsInSubscriptions(const std::string& src)
{
  return m_subscriptionsManager.IsInSubscriptions(src);
}

//////////////////
// WebFavorites //
//////////////////

bool CBoxeeClientServerComManager::GetWebFavorites(BXBoxeeWebFavorites& WebFavoritesList)
{
  return m_webFavoritesManager.GetWebFavorites(WebFavoritesList);
}

bool CBoxeeClientServerComManager::GetWebFavorites(std::vector<BXObject>& webFavoritesVec)
{
  return m_webFavoritesManager.GetWebFavorites(webFavoritesVec);
}

bool CBoxeeClientServerComManager::UpdateWebFavoritesListNow()
{
  return m_webFavoritesManager.UpdateWebFavoritesList(0,false);
}

bool CBoxeeClientServerComManager::UpdateWebFavoritesList(unsigned long executionDelayInMS, bool repeat)
{
  return m_webFavoritesManager.UpdateWebFavoritesList(executionDelayInMS,repeat);
}

bool CBoxeeClientServerComManager::IsInWebFavorites(const std::string& src)
{
  return m_webFavoritesManager.IsInWebFavorites(src);
}

//////////////
// Services //
//////////////

bool CBoxeeClientServerComManager::GetServices(BXBoxeeServices& servicesList)
{
  return m_servicesManager.GetServices(servicesList);
}

bool CBoxeeClientServerComManager::GetServices(std::vector<BXServicesItem>& servicesVec)
{
  return m_servicesManager.GetServices(servicesVec);
}

bool CBoxeeClientServerComManager::GetServicesIds(std::vector<std::string>& servicesIdsVec)
{
  return m_servicesManager.GetServicesIds(servicesIdsVec);
}

bool CBoxeeClientServerComManager::IsRegisterToServices(const std::string& serviceIdentifier, CServiceIdentifierType::ServiceIdentifierTypeEnums identifierTypeEnum)
{
  return m_servicesManager.IsRegisterToServices(serviceIdentifier,identifierTypeEnum);
}

//////////////////
// Entitlements //
//////////////////

bool CBoxeeClientServerComManager::GetEntitlements(BXBoxeeEntitlements& entitlementsList)
{
  return m_entitlementsManager.GetEntitlements(entitlementsList);
}

bool CBoxeeClientServerComManager::GetEntitlements(std::vector<BXEntitlementsItem>& entitlementsVec)
{
  return m_entitlementsManager.GetEntitlements(entitlementsVec);
}

bool CBoxeeClientServerComManager::GetEntitlementsIds(std::vector<std::string>& entitlementsIdsVec)
{
  return m_entitlementsManager.GetEntitlementsIds(entitlementsIdsVec);
}

bool CBoxeeClientServerComManager::IsInEntitlements(const std::string& productsList)
{
  return m_entitlementsManager.IsInEntitlements(productsList);
}

bool CBoxeeClientServerComManager::UpdateUserEntitlementsNow()
{
  return UpdateUserEntitlements(0,false);
}

bool CBoxeeClientServerComManager::UpdateUserEntitlements(unsigned long executionDelayInMS, bool repeat)
{
  return m_entitlementsManager.UpdateEntitlementsList(executionDelayInMS,repeat);
}

//////////////////
// Sources      //
//////////////////

bool CBoxeeClientServerComManager::GetSources(BXBoxeeSources& sourcesList)
{
  return m_sourcesManager.GetSources(sourcesList);
}

bool CBoxeeClientServerComManager::GetSources(std::vector<BXSourcesItem>& sourcesVec)
{
  return m_sourcesManager.GetSources(sourcesVec);
}

bool CBoxeeClientServerComManager::GetTvSources(std::vector<BXSourcesItem>& sourcesVec)
{
  return m_sourcesManager.GetTVSources(sourcesVec);
}

bool CBoxeeClientServerComManager::GetMovieSources(std::vector<BXSourcesItem>& sourcesVec)
{
  return m_sourcesManager.GetMovieSources(sourcesVec);
}

bool CBoxeeClientServerComManager::GetSourcesIds(std::vector<std::string>& sourcesIdsVec)
{
  return m_sourcesManager.GetSourcesIds(sourcesIdsVec);
}

bool CBoxeeClientServerComManager::IsInSources(const std::string& productsList)
{
  return m_sourcesManager.IsInSources(productsList);
}

bool CBoxeeClientServerComManager::UpdateExcludedSourcesNow()
{
  return UpdateExcludedSources(0,false);
}

bool CBoxeeClientServerComManager::UpdateExcludedSources(unsigned long executionDelayInMS, bool repeat)
{
  return m_sourcesManager.UpdateExcludedSources(executionDelayInMS,repeat);
}

void CBoxeeClientServerComManager::SetExcludedSources(const std::string& excludedSources)
{
  return m_sourcesManager.SetExcludedSources(excludedSources);
}

std::string CBoxeeClientServerComManager::GetExcludedSources()
{
  return m_sourcesManager.GetExcludedSources();
}

//////////////
// Featured //
//////////////

bool CBoxeeClientServerComManager::GetFeatured(BXBoxeeFeed& featuredList)
{
  return m_featuredManager.GetFeaturedList(featuredList);
}

////////////
// AppBox //
////////////

bool CBoxeeClientServerComManager::GetAppBoxApplications(TiXmlDocument& applicationsList)
{
  return m_appBoxApplicationsManager.GetAppBoxBoxeeApplications(applicationsList);
}

bool CBoxeeClientServerComManager::IsAppIdInAppBoxApplicationsList(const std::string& id)
{
  return m_appBoxApplicationsManager.IsAppIdInAppBoxApplicationsList(id);
}

bool CBoxeeClientServerComManager::UpdateAppBoxApplicationsListNow()
{
  return UpdateAppBoxApplicationsList(0,false);
}

bool CBoxeeClientServerComManager::UpdateAppBoxApplicationsList(unsigned long executionDelayInMS, bool repeat)
{
  return m_appBoxApplicationsManager.UpdateAppBoxBoxeeApplicationsList(executionDelayInMS,repeat);
}

std::string CBoxeeClientServerComManager::GetAppBoxPopularitiesById(const std::string& id)
{
  return m_appBoxApplicationsManager.GetAppBoxPopularityById(id);
}

bool CBoxeeClientServerComManager::UpdateAppBoxPopularitiesListNow()
{
  return UpdateAppBoxPopularitiesList(0,false);
}

bool CBoxeeClientServerComManager::UpdateAppBoxPopularitiesList(unsigned long executionDelayInMS, bool repeat)
{
  return m_appBoxApplicationsManager.UpdateAppBoxPopularitiesList(executionDelayInMS,repeat);
}

bool CBoxeeClientServerComManager::GetAppBoxRepositories(BXAppBoxRepositories& repositoriesList)
{
  return m_appBoxApplicationsManager.GetRepositories(repositoriesList);
}

bool CBoxeeClientServerComManager::UpdateAppBoxRepositoriesListNow()
{
  return UpdateAppBoxRepositoriesList(0,false);
}

bool CBoxeeClientServerComManager::UpdateAppBoxRepositoriesList(unsigned long executionDelayInMS, bool repeat)
{
  return m_appBoxApplicationsManager.UpdateAppBoxRepositoriesList(executionDelayInMS,repeat);
}

bool CBoxeeClientServerComManager::GetAppBox3rdPartyRepositoryApplications(const std::string& repositoryId, TiXmlDocument& applicationsList, bool bDontWait)
{
  return m_appBoxApplicationsManager.GetAppBox3rdPartyRepositoryApplications(repositoryId, applicationsList, bDontWait);
}

////////////
// Genres //
////////////

bool CBoxeeClientServerComManager::GetMovieGenres(std::vector<GenreItem>& movieGenresVec)
{
  return m_genresManager.GetMovieGenres(movieGenresVec);
}

bool CBoxeeClientServerComManager::GetTvGenres(std::vector<GenreItem>& tvGenresVec)
{
  return m_genresManager.GetTvGenres(tvGenresVec);
}

///////////////
// Bad Words //
///////////////

bool CBoxeeClientServerComManager::GetBadWords(std::vector<std::string>& badWordsVec)
{
  return m_genresManager.GetBadWords(badWordsVec);
}

//////////////
// Trailers //
//////////////
bool CBoxeeClientServerComManager::GetMovieTrailerSections(std::vector<TrailerSectionItem>& movieTrailersVec)
{
  return m_trailerSectionsManager.GetMovieTrailerSections(movieTrailersVec);
}

////////////////////
// App Categories //
////////////////////
bool CBoxeeClientServerComManager::UpdateAppsCategoriesListNow()
{
  return m_appBoxApplicationsManager.UpdateAppsCategoriesList(0,false);
}

bool CBoxeeClientServerComManager::GetAppsCategories(std::vector<AppCategoryItem>& vecAppCategories)
{
  return m_appBoxApplicationsManager.GetAppsCategories(vecAppCategories);
}

std::string CBoxeeClientServerComManager::GetAppCategoryLabel(const std::string& id)
{
  return m_appBoxApplicationsManager.GetAppsCategoryLabel(id);
}

}
