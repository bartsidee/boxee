// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxboxeeapplicationsmanager
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEAPPSMANAGER_H
#define BOXEEAPPSMANAGER_H

#include "bxscheduletaskmanager.h"
#include "bxboxeeapplications.h"
#include <SDL/SDL.h>

namespace BOXEE
{

class BXApplicationItem : public BXItem
{
public:
  BXApplicationItem(const std::string& type, const std::string& id, const std::string& name);
  virtual ~BXApplicationItem(){};

private:

};

class BXApplicationsManager
{
public:
  BXApplicationsManager();
  virtual ~BXApplicationsManager();

  bool Initialize();
  
  bool UpdateApplicationsList(unsigned long executionDelayInMS, bool repeat);
  
  bool GetApplications(BXBoxeeApplications& applicationsList);
  bool GetApplications(std::vector<BXApplicationItem>& applicationsVec);
  
  bool IsInApplications(const std::string& id);

private:

  void LockApplicationsList();
  void UnLockApplicationsList();
  void CopyApplicationsList(const BXBoxeeApplications& applicationsList);
  void SetApplicationsListIsLoaded(bool isLoaded);

  SDL_mutex* m_applicationsListGuard;
  BXBoxeeApplications m_applicationsList;
  
  class RequestApplicationsListFromServerTask : public BoxeeScheduleTask
  {
  public:
    
    RequestApplicationsListFromServerTask(BXApplicationsManager* taskHandler, unsigned long executionDelayInMS, bool repeat);
    virtual ~RequestApplicationsListFromServerTask();
    virtual void DoWork();
    
  private:
    
    BXApplicationsManager* m_taskHandler;
  };
};

}

#endif
