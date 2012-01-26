// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxcscmanager
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEECSCMANAGER_H
#define BOXEECSCMANAGER_H

#include "bxscheduletaskmanager.h"
#include "bxrecommendationsmanager.h"
#include "bxqueuemanager.h"
#include "bxfriendsmanager.h"
#include "bxapplicationsmanager.h"
#include "bxsubscriptionsmanager.h"
#include "bxservicesmanager.h"
#include "bxentitlementsmanager.h"
#include "bxsourcesmanager.h"
#include "bxfeaturedmanager.h"
#include "bxappboxmanager.h"
#include "bxgenresmanager.h"
#include "bxwebfavoritesmanager.h"
#include "bxtrailersmanager.h"

#include <SDL/SDL.h>

namespace BOXEE
{

class CBoxeeClientServerComManager
{
public:
  CBoxeeClientServerComManager();
  virtual ~CBoxeeClientServerComManager();

  bool Initialize();
  bool Deinitialize();

  bool UpdateAllUserListsNow();

  bool AddRequestFromServer(BoxeeScheduleTask& requestTask);
  
  bool GetRecommendations(BXBoxeeFeed& recommendationsList);
  int GetRecommendationsSize();
  
  bool GetQueue(BXBoxeeFeed& queueList, CQueueItemsType::QueueItemsTypeEnums queueType = CQueueItemsType::QIT_ALL);
  int GetQueueSize(CQueueItemsType::QueueItemsTypeEnums queueType);
  void SetValidQueueSize(int validQueueSize);
  int GetValidQueueSize();
  bool IsInQueue(const std::string& boxeeId, const std::string& path);
  bool IsInQueue(const std::string& boxeeId, const std::string& path, std::string& referral);
  bool UpdateQueueNow();
  bool UpdateQueue(unsigned long executionDelayInMS, bool repeat);

  bool GetFriends(BXFriendsList& friendsList, const std::string &strUserId = "", time_t listTimeStamp=0);
  
  bool GetUserApplications(BXBoxeeApplications& applicationsList);
  bool GetUserApplications(std::vector<BXApplicationItem>& applicationVec);
  bool UpdateUserApplicationsListNow();
  bool UpdateGenresListNow();
  bool UpdateUserApplicationsList(unsigned long executionDelayInMS, bool repeat);
  bool IsInUserApplications(const std::string& id);

  bool GetSubscriptions(BXBoxeeSubscriptions& subscriptionsList);
  bool GetSubscriptions(CSubscriptionType::SubscriptionTypeEnums subscriptionType,std::vector<BXSubscriptionItem>& subscriptionsVec);
  bool GetSubscriptionIds(CSubscriptionType::SubscriptionTypeEnums subscriptionType,std::vector<std::string>& subscriptionsIdsVec);
  bool UpdateSubscriptionsListNow();
  bool UpdateSubscriptionsList(unsigned long executionDelayInMS, bool repeat);
  bool IsInSubscriptions(const std::string& src);
  
  bool GetWebFavorites(BXBoxeeWebFavorites& WebFavortiesList);
  bool GetWebFavorites(std::vector<BXObject>& webFavoritesVec);
  bool UpdateWebFavoritesListNow();
  bool UpdateWebFavoritesList(unsigned long executionDelayInMS, bool repeat);
  bool IsInWebFavorites(const std::string& src);

  bool GetServices(BXBoxeeServices& servicesList);
  bool GetServices(std::vector<BXServicesItem>& servicesVec);
  bool GetServicesIds(std::vector<std::string>& servicesIdsVec);
  bool IsRegisterToServices(const std::string& serviceIdentifier, CServiceIdentifierType::ServiceIdentifierTypeEnums identifierTypeEnum);

  bool GetEntitlements(BXBoxeeEntitlements& entitlementsList);
  bool GetEntitlements(std::vector<BXEntitlementsItem>& entitlementsVec);
  bool GetEntitlementsIds(std::vector<std::string>& entitlementsIdsVec);
  bool IsInEntitlements(const std::string& productsList);
  bool UpdateUserEntitlementsNow();
  bool UpdateUserEntitlements(unsigned long executionDelayInMS, bool repeat);

  bool GetSources(BXBoxeeSources& sourcesList);
  bool GetSources(std::vector<BXSourcesItem>& sourcesVec);
  bool GetTvSources(std::vector<BXSourcesItem>& sourcesVec);
  bool GetMovieSources(std::vector<BXSourcesItem>& sourcesVec);
  bool GetSourcesIds(std::vector<std::string>& sourcesIdsVec);
  bool IsInSources(const std::string& productsList);

  bool UpdateExcludedSourcesNow();
  bool UpdateExcludedSources(unsigned long executionDelayInMS, bool repeat);
  void SetExcludedSources(const std::string& excludedSources);
  std::string GetExcludedSources();

  bool GetFeatured(BXBoxeeFeed& featuredList);

  bool GetAppBoxApplications(TiXmlDocument& applicationsList);
  bool IsAppIdInAppBoxApplicationsList(const std::string& id);
  bool UpdateAppBoxApplicationsListNow();
  bool UpdateAppBoxApplicationsList(unsigned long executionDelayInMS, bool repeat);
  std::string GetAppBoxPopularitiesById(const std::string& id);
  bool UpdateAppBoxPopularitiesListNow();
  bool UpdateAppBoxPopularitiesList(unsigned long executionDelayInMS, bool repeat);
  bool GetAppBoxRepositories(BXAppBoxRepositories& repositoriesList);
  bool UpdateAppBoxRepositoriesListNow();
  bool UpdateAppBoxRepositoriesList(unsigned long executionDelayInMS, bool repeat);
  bool GetAppBox3rdPartyRepositoryApplications(const std::string& repositoryId, TiXmlDocument& applicationsList, bool bDontWait);

  bool GetMovieGenres(std::vector<GenreItem>& movieGenresVec);
  bool GetTvGenres(std::vector<GenreItem>& tvGenresVec);
  bool GetBadWords(std::vector<std::string>& badWordsVec);

  bool GetMovieTrailerSections(std::vector<TrailerSectionItem>& movieTrailersVec);

  bool UpdateAppsCategoriesListNow();
  bool GetAppsCategories(std::vector<AppCategoryItem>& vecAppCategories);
  std::string GetAppCategoryLabel(const std::string& id);

private:

  bool InitializeUserLists();
  bool AddUserListsToScheduleTaskManager();

  BXRecommendationsManager m_recommendationsManager;
  BXQueueManager m_queueManager;
  BXFriendsManager m_friendsManager;
  BXApplicationsManager m_userApplicationsManager;
  BXSubscriptionsManager m_subscriptionsManager;
  BXServicesManager m_servicesManager;
  BXEntitlementsManager m_entitlementsManager;
  BXSourcesManager m_sourcesManager;
  BXFeaturedManager m_featuredManager;
  BXAppBoxManager m_appBoxApplicationsManager;
  BXGenresManager m_genresManager;
  BXWebFavoritesManager m_webFavoritesManager;
  BXTrailerSectionsManager m_trailerSectionsManager;
};

} // namespace

#endif
