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

#include <set>

namespace BOXEE
{

class BXSourcesItem : public BXItem
{
public:
  BXSourcesItem(const std::string& type, const std::string& id, const std::string& name, const std::string& sourceId, const std::string& sourceName, const std::string& sourceType, const std::string& sourceGeo, const std::string& sourceThumb, const std::string& sourcePremium, const std::string& sourceOffer);
  virtual ~BXSourcesItem(){};

  std::string GetSourceId(){return m_sourceId; }
  std::string GetSourceName(){return m_sourceName; }
  std::string GetSourceType(){return m_sourceType; }
  std::string GetSourceGeo(){return m_sourceGeo; }
  std::string GetSourceThumb(){return m_sourceThumb; }
  std::string GetSourcePremium(){return m_sourcePremium; }
  std::string GetSourceOffer(){return m_sourceOffer; }

private:

  std::string m_sourceId;
  std::string m_sourceName;
  std::string m_sourceType;
  std::string m_sourceGeo;
  std::string m_sourceThumb;
  std::string m_sourcePremium;
  std::string m_sourceOffer;
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

  /////////////////////
  // ExcludedSources //
  /////////////////////

  bool UpdateExcludedSources(unsigned long executionDelayInMS, bool repeat);
  void SetExcludedSources(const std::string& excludedSources);
  std::string GetExcludedSources();

private:

  void LockSourcesList();
  void UnLockSourcesList();
  void CopySourcesList(const BXBoxeeSources& sourcesList);
  void SetSourcesListIsLoaded(bool isLoaded);

  SDL_mutex* m_sourcesListGuard;
  BXBoxeeSources m_sourcesList;
  BXBoxeeSources	m_tvSourcesList;
  BXBoxeeSources	m_movieSourcesList;
  
  /////////////////////
  // ExcludedSources //
  /////////////////////

  void LockExcludedSources();
  void UnLockExcludedSources();
  void SetExcludedSourcesLoaded(bool isLoaded);
  SDL_mutex* m_excludedSourcesGuard;
  std::string m_excludedSources;
  bool m_isExcludedSourcesLoaded;

  class RequestSourcesListFromServerTask : public BoxeeScheduleTask
  {
  public:
    
    RequestSourcesListFromServerTask(BXSourcesManager* taskHandler, unsigned long executionDelayInMS, bool repeat);
    virtual ~RequestSourcesListFromServerTask();
    virtual void DoWork();
    
  private:
    
    BXSourcesManager* m_taskHandler;
  };

  /////////////////////
  // ExcludedSources //
  /////////////////////

  class RequestExcludedSourcesFromServerTask : public BoxeeScheduleTask
  {
  public:

    RequestExcludedSourcesFromServerTask(BXSourcesManager* taskHandler, unsigned long executionDelayInMS, bool repeat);
    virtual ~RequestExcludedSourcesFromServerTask();
    virtual void DoWork();

  private:

    BXSourcesManager* m_taskHandler;
  };
};

}

#endif
