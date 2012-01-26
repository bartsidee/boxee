// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxboxeefriendsmanager
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEBXFRIENDSMANAGER_H
#define BOXEEBXFRIENDSMANAGER_H

#include "bxscheduletaskmanager.h"
#include "bxfriendslist.h"
#include <SDL/SDL.h>

namespace BOXEE
{

class BXFriendsManager
{
public:
  BXFriendsManager();
  virtual ~BXFriendsManager();

  bool Initialize();
  
  bool UpdateFriendsList(unsigned long executionDelayInMS, bool repeat);
  bool GetFriendsList(BXFriendsList& friendsList, const std::string &user, time_t listTimeStamp);

private:

  void LockFriendsList();
  void UnLockFriendsList();
  void CopyFriendsList(const BXFriendsList& friendsList);
  void SetFriendsListIsLoaded(bool isLoaded);

  SDL_mutex* m_friendsListGuard;
  BXFriendsList m_friendsList;
  
  class RequestFriendsListFromServerTask : public BoxeeScheduleTask
  {
  public:
    
    RequestFriendsListFromServerTask(BXFriendsManager* taskHandler, unsigned long executionDelayInMS, bool repeat);
    virtual ~RequestFriendsListFromServerTask();
    virtual void DoWork();
    
  private:
    
    BXFriendsManager* m_taskHandler;
  };
};

}

#endif
