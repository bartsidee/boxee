// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxqueuesmanager
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEBXQUEUEMANAGER_H
#define BOXEEBXQUEUEMANAGER_H

#include "bxscheduletaskmanager.h"
#include "bxboxeefeed.h"
#include <SDL/SDL.h>

namespace BOXEE
{

class BXQueueManager
{
public:
  BXQueueManager();
  virtual ~BXQueueManager();

  bool Initialize();
  
  bool UpdateQueueList(unsigned long executionDelayInMS, bool repeat);
  bool GetQueueList(BXBoxeeFeed& queueList);
  int GetQueueSize();
  bool IsInQueueList(const std::string& boxeeId, const std::string& path);
  bool IsInQueueList(const std::string& boxeeId, const std::string& path, std::string& referral);

  void SetValidQueueSize(int validQueueSize);
  int GetValidQueueSize();

private:

  void LockQueueList();
  void UnLockQueueList();
  void CopyQueueList(const BXBoxeeFeed& queueList);
  void SetQueueListIsLoaded(bool isLoaded);

  SDL_mutex* m_queueListGuard;
  BXBoxeeFeed m_queueList;
  
  int m_validQueueSize;

  class RequestQueueListFromServerTask : public BoxeeScheduleTask
  {
  public:

    RequestQueueListFromServerTask(BXQueueManager* taskHandler, unsigned long executionDelayInMS, bool repeat);
    virtual ~RequestQueueListFromServerTask();
    virtual void DoWork();

  private:

    bool CanExecute();

    BXQueueManager* m_taskHandler;
  };
};

}

#endif
