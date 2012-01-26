// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxboxeesourcesmanager
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEESOURCEMANAGER_H
#define BOXEESOURCEMANAGER_H

#include "bxscheduletaskmanager.h"
#include "bxboxeesources.h"
#include <SDL/SDL.h>

namespace BOXEE
{

class BXSourcesItem : public BXItem
{
public:
  BXSourcesItem(const std::string& type, const std::string& id, const std::string& name, const std::string& sourceId, const std::string& sourceName, const std::string& sourceType, const std::string& sourceGeo);
  virtual ~BXSourcesItem(){};

  std::string GetSourceId(){return m_sourceId; }
  std::string GetSourceName(){return m_sourceName; }
  std::string GetSourceType(){return m_sourceType; }
  std::string GetSourceGeo(){return m_sourceGeo; }

private:

  std::string m_sourceId;
  std::string m_sourceName;
  std::string m_sourceType;
  std::string m_sourceGeo;
};

class BXSourcesManager
{
public:
  BXSourcesManager();
  virtual ~BXSourcesManager();

  bool Initialize();
  
  bool UpdateSourcesList(unsigned long executionDelayInMS, bool repeat);
  bool GetSources(BXBoxeeSources& sourcesList, const std::string& strType = "");
  bool GetSources(std::vector<BXSourcesItem>& sourcesVec, const std::string& strType = "");
  bool GetTVSources(std::vector<BXSourcesItem>& sourcesVec);
  bool GetMovieSources(std::vector<BXSourcesItem>& sourcesVec);

  bool GetSourcesIds(std::vector<std::string>& sourcesIdsVec);

  bool IsInSources(const std::string& productsList);

private:

  void LockSourcesList();
  void UnLockSourcesList();
  void CopySourcesList(const BXBoxeeSources& sourcesList);
  void SetSourcesListIsLoaded(bool isLoaded);

  SDL_mutex* m_sourcesListGuard;
  BXBoxeeSources m_sourcesList;
  BXBoxeeSources	m_tvSourcesList;
  BXBoxeeSources	m_movieSourcesList;

  class RequestSourcesListFromServerTask : public BoxeeScheduleTask
  {
  public:
    
    RequestSourcesListFromServerTask(BXSourcesManager* taskHandler, unsigned long executionDelayInMS, bool repeat);
    virtual ~RequestSourcesListFromServerTask();
    virtual void DoWork();
    
  private:
    
    BXSourcesManager* m_taskHandler;
  };
};

}

#endif
