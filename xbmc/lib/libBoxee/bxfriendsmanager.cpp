//
// C++ Implementation: bxfriendsmanager
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxfriendsmanager.h"
#include "boxee.h"
#include "bxconfiguration.h"
#include "bxexceptions.h"
#include "logger.h"
#include "../../Application.h"

namespace BOXEE
{

BXFriendsManager::BXFriendsManager()
{
  m_friendsListGuard = SDL_CreateMutex();
  m_friendsList.Clear();
}

BXFriendsManager::~BXFriendsManager()
{
  SDL_DestroyMutex(m_friendsListGuard);
  
}

bool BXFriendsManager::Initialize()
{
  m_friendsList.Clear();
  
  return true;
}

bool BXFriendsManager::UpdateFriendsList(unsigned long executionDelayInMS, bool repeat)
{
  RequestFriendsListFromServerTask* reqFriendsListTask = new RequestFriendsListFromServerTask(this,executionDelayInMS,repeat);

  if(reqFriendsListTask)
  {
    if(executionDelayInMS == 0)
    {
      // In case the request is for immediate execution -> Set the status of the list to NOT LOADED in order 
      // for get function to wait for update

      LockFriendsList();

      m_friendsList.SetLoaded(false);
      
      UnLockFriendsList();      
    }
    
    Boxee::GetInstance().GetBoxeeScheduleTaskManager().AddScheduleTask(reqFriendsListTask);
    return true;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"BXFriendsManager::UpdateFriendsList - FAILED to allocate RequestFriendsListFromServerTask (friends)");
    return false;
  }
}

void BXFriendsManager::LockFriendsList()
{
  SDL_LockMutex(m_friendsListGuard);
}

void BXFriendsManager::UnLockFriendsList()
{
  SDL_UnlockMutex(m_friendsListGuard);
}

void BXFriendsManager::CopyFriendsList(const BXFriendsList& friendsList)
{
  LockFriendsList();
  
  m_friendsList = friendsList;
  m_friendsList.SetLoaded(true);
  
  UnLockFriendsList();
}

void BXFriendsManager::SetFriendsListIsLoaded(bool isLoaded)
{
  LockFriendsList();

  m_friendsList.SetLoaded(isLoaded);

  UnLockFriendsList();
}

bool BXFriendsManager::GetFriendsList(BXFriendsList& friendsList, const std::string &user, time_t listTimeStamp)
{
  LOG(LOG_LEVEL_DEBUG,"BXFriendsManager::GetFrinedsList - Enter function (friends)");

  if(BOXEE::Boxee::GetInstance().IsInOfflineMode())
  {
    LOG(LOG_LEVEL_DEBUG,"BXFriendsManager::GetFrinedsList - In offline mode. Going to return FALSE (friends)");
    return false;
  }

  LOG(LOG_LEVEL_DEBUG,"BXFriendsManager::GetFrinedsList - Enter function with [user=%s] (friends)",user.c_str());

  std::string currentUser = BOXEE::Boxee::GetInstance().GetCredentials().GetUserName();

  if((user == "") || (user == currentUser))
  {
    // retreive the FriendsList of the current user

    LOG(LOG_LEVEL_DEBUG,"BXFriendsManager::GetFrinedsList - User received [%s]. Going to get FriendsList of current user [%s] (friends)",user.c_str(),currentUser.c_str());

    bool friendsListWasLoaded = false;
    
    LockFriendsList();

    friendsListWasLoaded = m_friendsList.IsLoaded();
    
    while (!friendsListWasLoaded)
    {
      // FriendsList ISN'T loaded yet -> UnLock the FriendsList and wait for it to load
      
      UnLockFriendsList();

      LOG(LOG_LEVEL_DEBUG,"BXFriendsManager::GetFriendsList - FriendsList is not loaded yet. Going to try again in [%dms] (friends)",DELAY_FOR_CHECK_FEED_LOADED);

      SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

      LockFriendsList();
        
      friendsListWasLoaded = m_friendsList.IsLoaded();      
    }

    friendsList = m_friendsList;

    UnLockFriendsList();

    LOG(LOG_LEVEL_DEBUG,"BXFriendsManager::GetFriendsList - Exit function. After set [FriendsListSize=%d] (queue)",friendsList.GetCount());

    return true;
  }
  else
  {
    // retreive the FriendsList of a specific user

    LOG(LOG_LEVEL_DEBUG,"BXFriendsManager::GetFriendsList - User received [%s]. Going to get FriendsList of this user [%s] (friends)",user.c_str(),user.c_str());

    std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.FriendsListUrl","http://app.boxee.tv/user/");
    strUrl += user;
    strUrl += "/friends";

    BXCredentials credentials;
    credentials.SetUserName(BOXEE::Boxee::GetInstance().GetCredentials().GetUserName());
    credentials.SetPassword(BOXEE::Boxee::GetInstance().GetCredentials().GetPassword());

    friendsList.SetCredentials(credentials);
    friendsList.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

    bool retVal = friendsList.LoadFromURL(strUrl);

    LOG(LOG_LEVEL_DEBUG,"BXFriendsManager::GetFriendsList - Call LoadFromURL with [%s] returned [FriendsListSize=%d][retVal=%d] (friends)",strUrl.c_str(),friendsList.GetCount(),retVal);

    return retVal;
  }
}

////////////////////////////////////////////////
// RequestFriendsListFromServerTask functions //
////////////////////////////////////////////////

BXFriendsManager::RequestFriendsListFromServerTask::RequestFriendsListFromServerTask(BXFriendsManager* taskHandler, unsigned long executionDelayInMS, bool repeat):BoxeeScheduleTask("RequestFriendsList",executionDelayInMS,repeat)
{
  m_taskHandler = taskHandler;
}

BXFriendsManager::RequestFriendsListFromServerTask::~RequestFriendsListFromServerTask()
{
  
}

void BXFriendsManager::RequestFriendsListFromServerTask::DoWork()
{
  LOG(LOG_LEVEL_DEBUG,"RequestFriendsListFromServerTask::DoWork - Enter function (friends)");

  if (!g_application.ShouldConnectToInternet())
  {
    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetFriendsListIsLoaded(true);

    LOG(LOG_LEVEL_DEBUG,"RequestFriendsListFromServerTask::DoWork - [ShouldConnectToInternetd=FALSE] -> Exit function (friends)");
    return;
  }

  BXFriendsList friendsList;
  
  friendsList.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  friendsList.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());
  
  std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.FriendsListUrl","http://app.boxee.tv/user/");
  strUrl += BOXEE::Boxee::GetInstance().GetCredentials().GetUserName();
  strUrl += "/friends";

  friendsList.LoadFromURL(strUrl);
  m_taskHandler->CopyFriendsList(friendsList);

  LOG(LOG_LEVEL_DEBUG,"RequestFriendsListFromServerTask::DoWork - Exit function (friends)");

  return;
}

}

