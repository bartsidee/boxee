// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxfeaturedmanager
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEBXFEATUREDMANAGER_H
#define BOXEEBXFEATUREDMANAGER_H

#include "bxscheduletaskmanager.h"
#include "bxboxeefeed.h"
#include <SDL/SDL.h>


namespace BOXEE 
{

/**
*/
class BXFeaturedManager
{
public:
  BXFeaturedManager();
  virtual ~BXFeaturedManager();

  bool Initialize();
  
  bool UpdateFeaturedList(unsigned long executionDelayInMS, bool repeat);
  bool GetFeaturedList(BXBoxeeFeed& featuredList);

private:

  void LockFeaturedList();
  void UnLockFeaturedList();
  void CopyFeaturedList(const BXBoxeeFeed& featuredList);
  void SetFeaturedListIsLoaded(bool isLoaded);

  SDL_mutex* m_featuredListGuard;
  BXBoxeeFeed m_featuredList;
  
  class RequestFeaturedListFromServerTask : public BoxeeScheduleTask
  {
  public:
    
    RequestFeaturedListFromServerTask(BXFeaturedManager* taskHandler, unsigned long executionDelayInMS, bool repeat);
    virtual ~RequestFeaturedListFromServerTask();
    virtual void DoWork();
    
  private:
    
    bool CanExecute();

    BXFeaturedManager* m_taskHandler;
  };
};

}

#endif
