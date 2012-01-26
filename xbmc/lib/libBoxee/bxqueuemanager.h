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

class CQueueItemsType
{
public:
  enum QueueItemsTypeEnums
  {
    QIT_CLIP=0,
    QIT_TVSHOW=1,
    QIT_MOVIE=2,
    QIT_ALL=3,
    NUM_OF_QUEUE_ITEMS_TYPE=4
  };

  static std::string GetQueueItemTypeAsStringId(CQueueItemsType::QueueItemsTypeEnums queueItemType);
  static CQueueItemsType::QueueItemsTypeEnums GetQueueItemTypeAsEnum(const std::string& _queueItem);
};

class BXQueueManager
{
public:
  BXQueueManager();
  virtual ~BXQueueManager();

  bool Initialize();
  
  bool UpdateQueueList(unsigned long executionDelayInMS, bool repeat);
  bool GetQueueList(BXBoxeeFeed& queueList, CQueueItemsType::QueueItemsTypeEnums queueType = CQueueItemsType::QIT_ALL);
  int GetQueueSize(CQueueItemsType::QueueItemsTypeEnums queueType);
  bool IsInQueueList(const std::string& boxeeId, const std::string& path);
  bool IsInQueueList(const std::string& boxeeId, const std::string& path, std::string& referral);

  void SetValidQueueSize(int validQueueSize);
  int GetValidQueueSize();

private:

  void LockQueueList();
  void UnLockQueueList();
  void CopyQueueList(const BXBoxeeFeed& queueList, CQueueItemsType::QueueItemsTypeEnums queueType);
  void SetQueueListIsLoaded(CQueueItemsType::QueueItemsTypeEnums queueType, bool isLoaded);

  bool IsLoaded(CQueueItemsType::QueueItemsTypeEnums queueType);

  BXBoxeeFeed* GetQueueListByType(CQueueItemsType::QueueItemsTypeEnums queueType);

  SDL_mutex* m_queueListGuard;

  BXBoxeeFeed m_queueList;
  BXBoxeeFeed m_queueClipList;
  BXBoxeeFeed m_queueMovieList;
  BXBoxeeFeed m_queueTvList;

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
