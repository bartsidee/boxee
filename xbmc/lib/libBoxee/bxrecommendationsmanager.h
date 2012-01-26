// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxrecommendationsmanager
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEBXRECOMMENDATIONSMANAGER_H
#define BOXEEBXRECOMMENDATIONSMANAGER_H

#include "bxscheduletaskmanager.h"
#include "bxboxeefeed.h"
#include <SDL/SDL.h>


namespace BOXEE 
{

/**
*/
class BXRecommendationsManager
{
public:
  BXRecommendationsManager();
  virtual ~BXRecommendationsManager();

  bool Initialize();
  
  bool UpdateRecommendationsList(unsigned long executionDelayInMS, bool repeat);
  bool GetRecommendationsList(BXBoxeeFeed& recommendationsList);
  int GetRecommendationsListSize();

private:

  void LockRecommendationsList();
  void UnLockRecommendationsList();
  void CopyRecommendationsList(const BXBoxeeFeed& recommendationsList);
  void SetRecommendationsListIsLoaded(bool isLoaded);

  SDL_mutex* m_recommendationsListGuard;
  BXBoxeeFeed m_recommendationsList;
  
  class RequestRecommendationsListFromServerTask : public BoxeeScheduleTask
  {
  public:
    
    RequestRecommendationsListFromServerTask(BXRecommendationsManager* taskHandler, unsigned long executionDelayInMS, bool repeat);
    virtual ~RequestRecommendationsListFromServerTask();
    virtual void DoWork();
    
  private:
    
    bool CanExecute();

    BXRecommendationsManager* m_taskHandler;
  };
};

}

#endif
