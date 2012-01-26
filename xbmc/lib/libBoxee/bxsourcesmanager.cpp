//
// C++ Implementation: bxsourcesmanager
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxsourcesmanager.h"
#include "boxee.h"
#include "bxconfiguration.h"
#include "bxexceptions.h"
#include "bxutils.h"
#include "logger.h"
#include "../../Application.h"
#include "../../BoxeeUtils.h"

#define ENTITLEMENTS_DELIMITER ";"

namespace BOXEE
{

BXSourcesManager::BXSourcesManager()
{
  m_sourcesListGuard = SDL_CreateMutex();
  m_sourcesList.Clear();
}

BXSourcesManager::~BXSourcesManager()
{
  SDL_DestroyMutex(m_sourcesListGuard);
  
}

bool BXSourcesManager::Initialize()
{
  m_sourcesList.Clear();
  
  return true;
}

bool BXSourcesManager::UpdateSourcesList(unsigned long executionDelayInMS, bool repeat)
{
  RequestSourcesListFromServerTask* reqSourcesListTask = new RequestSourcesListFromServerTask(this,executionDelayInMS,repeat);

  if(reqSourcesListTask)
  {
    if(executionDelayInMS == 0)
    {
      // In case the request is for immediate execution -> Set the status of the list to NOT LOADED in order 
      // for get function to wait for update

      LockSourcesList();

      m_sourcesList.SetLoaded(false);
      
      UnLockSourcesList();
    }
    
    Boxee::GetInstance().GetBoxeeScheduleTaskManager().AddScheduleTask(reqSourcesListTask);
    return true;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"BXSourcesManager::UpdateSourcesList - FAILED to allocate RequestSourcesListFromServerTask (entitle)");
    return false;
  }
}

void BXSourcesManager::LockSourcesList()
{
  SDL_LockMutex(m_sourcesListGuard);
}

void BXSourcesManager::UnLockSourcesList()
{
  SDL_UnlockMutex(m_sourcesListGuard);
}

void BXSourcesManager::CopySourcesList(const BXBoxeeSources& sourcesList)
{
  LockSourcesList();
  
  m_sourcesList = sourcesList;
  m_sourcesList.SetLoaded(true);

  m_movieSourcesList.Clear();
  m_tvSourcesList.Clear();
  
  // Break the list into separate lists for movie and tv shows lists
  for (int i = 0; i < m_sourcesList.GetNumOfSources(); i++)
  {
	  BXObject bxSource = m_sourcesList.GetSource(i);
	  std::string strType = bxSource.GetValue("source_type");

	  // Tokenize the path and go over the tokens
	  std::vector<std::string> vecTokens = BXUtils::StringTokenize(strType, " ");
	  for (int j = 0; j < vecTokens.size(); j++)
	  {
		  if (vecTokens[j] == "movie")
			  m_movieSourcesList.AddSource(bxSource);
		  else if (vecTokens[j] == "tv")
			  m_tvSourcesList.AddSource(bxSource);
		  else
			  LOG(LOG_LEVEL_ERROR,"BXSourcesManager::CopySourcesList - Unknown source type %s (source)", vecTokens[j].c_str());
	  }
  }

  UnLockSourcesList();
}

void BXSourcesManager::SetSourcesListIsLoaded(bool isLoaded)
{
  LockSourcesList();

  m_sourcesList.SetLoaded(isLoaded);

  UnLockSourcesList();
}

bool BXSourcesManager::GetSources(BXBoxeeSources& sourcesList, const std::string& strType)
{
  LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::GetSourcesList - Enter function (entitle)");

  if(BOXEE::Boxee::GetInstance().IsInOfflineMode())
  {
    LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::GetSourcesList - In offline mode. Going to return FALSE (entitle)");
    return false;
  }

  bool sourcesListWasLoaded = false;
  
  LockSourcesList();

  sourcesListWasLoaded = m_sourcesList.IsLoaded();
  
  while (!sourcesListWasLoaded)
  {
    // SourcesList ISN'T loaded yet -> UnLock the SourcesList and wait for it to load
    
    UnLockSourcesList();

    LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::GetSourcesList - SourcesList is not loaded yet. Going to try again in [%dms] (entitle)",DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockSourcesList();
      
    sourcesListWasLoaded = m_sourcesList.IsLoaded();
  }

  if (strType == "tv")
  {
    sourcesList = m_tvSourcesList;
  }
  else if (strType == "movie")
  {
    sourcesList = m_movieSourcesList;
  }
  else
  {
    sourcesList = m_sourcesList;
  }

  UnLockSourcesList();

  LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::GetSourcesList - Exit function. After set [SourcesListSize=%d] (entitle)",sourcesList.GetNumOfSources());

  return true;
}

bool BXSourcesManager::GetSources(std::vector<BXSourcesItem>& sourcesVec, const std::string& strType)
{
  BXBoxeeSources sources;
  
  GetSources(sources, strType);
  
  int numOfSources = sources.GetNumOfSources();
  
  if(numOfSources < 1)
  {
    LOG(LOG_LEVEL_ERROR,"BXSourcesManager::GetSources - Call to GetSources with type = %s returned [SourcesListSize=%d] (source)", strType.c_str(), numOfSources);
    return false;
  }
  
  sourcesVec.clear();
  for(int i=0; i<numOfSources; i++)
  {
    BXObject obj = sources.GetSource(i);
    
    std::string type = obj.GetType();
    std::string id = obj.GetID();
    std::string name = obj.GetValue(MSG_KEY_NAME);
    std::string sourceId = obj.GetValue("source_id");
    std::string sourceName = obj.GetValue("source_name");
    std::string sourceType = obj.GetValue("source_type");
    std::string sourceGeo = obj.GetValue("source_geo");

    BXSourcesItem sourcesItem(type, id, name, sourceId, sourceName, sourceType, sourceGeo);
    
    sourcesVec.push_back(sourcesItem);
  }

  LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::GetSources - Going to return  [SourcesListSize=%d] for type = %s (source)",sourcesVec.size(), strType.c_str());

  return true;
}

bool BXSourcesManager::GetTVSources(std::vector<BXSourcesItem>& sourcesVec)
{
  return GetSources(sourcesVec, "tv");
}

bool BXSourcesManager::GetMovieSources(std::vector<BXSourcesItem>& sourcesVec)
{
  return GetSources(sourcesVec, "movie");
}

bool BXSourcesManager::GetSourcesIds(std::vector<std::string>& sourcesIdsVec)
{
  BXBoxeeSources sources;

  GetSources(sources);

  int numOfSources = sources.GetNumOfSources();

  if(numOfSources < 1)
  {
    LOG(LOG_LEVEL_ERROR,"BXSourcesManager::GetSourcesIds - Call to GetSources returned [SourcesListSize=%d] (entitle)",numOfSources);
    return false;
  }

  for(int i=0; i<numOfSources; i++)
  {
    BXObject obj = sources.GetSource(i);

    sourcesIdsVec.push_back(obj.GetID());
  }

  LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::GetSourcesIds - Going to return  [sourcesIdsVecSize=%d] (entitle)",sourcesIdsVec.size());

  return true;
}

bool BXSourcesManager::IsInSources(const std::string& productsList)
{
  if (productsList.empty())
  {
    LOG(LOG_LEVEL_WARNING,"BXSourcesManager::IsInSources - Enter with an EMPTY productsList therefore going to return FALSE. [productsList=%s] (entitle)",productsList.c_str());
    return false;
  }

  bool sourcesListWasLoaded = false;

  LockSourcesList();

  sourcesListWasLoaded = m_sourcesList.IsLoaded();

  while (!sourcesListWasLoaded)
  {
    // SourcesList ISN'T loaded yet -> UnLock the SourcesList and wait for it to load

    UnLockSourcesList();

    LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::IsInSources - SourcesList is not loaded yet. Going to try again in [%dms] (entitle)",DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockSourcesList();

    sourcesListWasLoaded = m_sourcesList.IsLoaded();
  }

  int numOfSources = m_sourcesList.GetNumOfSources();

  LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::IsInSources - Enter function with [productsList=%s][SourcesListSize=%d] (entitle)",productsList.c_str(),numOfSources);

  if(numOfSources < 1)
  {
    UnLockSourcesList();
    LOG(LOG_LEVEL_WARNING,"BXSourcesManager::IsInSources - [SourcesListSize=%d] therefore going to return FALSE (entitle)",numOfSources);
    return false;
  }

  std::set<CStdString> productsSet;
  int numOfProducts = BoxeeUtils::StringTokenize(productsList,productsSet,ENTITLEMENTS_DELIMITER,true,true);
  if (numOfProducts < 1)
  {
    UnLockSourcesList();
    LOG(LOG_LEVEL_WARNING,"BXSourcesManager::IsInSources - Tokenize of [productsList=%s] return [size=%d] therefore going to return FALSE (entitle)",productsList.c_str(),numOfProducts);
    return false;
  }

  for (int i=0; i < numOfSources; i++)
  {
    BXObject product = m_sourcesList.GetSource(i);

    std::string productId = product.GetValue("product_id");
    if(productsSet.find(productId) != productsSet.end())
    {
      UnLockSourcesList();
      LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::IsInSources - [productId=%s] WAS found in the SourcesList. Exit function and return TRUE (entitle)",productId.c_str());
      return true;
    }
  }

  UnLockSourcesList();
  LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::IsInSources - None of [productsList=%s] was found in the SourcesList. Exit function and return FALSE (entitle)",productsList.c_str());
  return false;
}

/////////////////////////////////////////////////
// RequestSourcesListFromServerTask functions //
/////////////////////////////////////////////////

BXSourcesManager::RequestSourcesListFromServerTask::RequestSourcesListFromServerTask(BXSourcesManager* taskHandler, unsigned long executionDelayInMS, bool repeat):BoxeeScheduleTask("RequestSourcesList",executionDelayInMS,repeat)
{
  m_taskHandler = taskHandler;
}

BXSourcesManager::RequestSourcesListFromServerTask::~RequestSourcesListFromServerTask()
{
  
}

void BXSourcesManager::RequestSourcesListFromServerTask::DoWork()
{
  LOG(LOG_LEVEL_DEBUG,"RequestSourcesListFromServerTask::DoWork - Enter function (entitle)");
  
  if (!g_application.IsConnectedToInternet())
  {
    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetSourcesListIsLoaded(true);

    LOG(LOG_LEVEL_DEBUG,"RequestSourcesListFromServerTask::DoWork - [IsConnectedToInternet=FALSE] -> Exit function (entitle)");
    return;
  }

  BXBoxeeSources sourcesList;
  
  sourcesList.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  sourcesList.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

  std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.SourcesListUrl","http://app.boxee.tv/titles/sources");

  sourcesList.LoadFromURL(strUrl);
  m_taskHandler->CopySourcesList(sourcesList);
  LOG(LOG_LEVEL_DEBUG,"RequestSourcesListFromServerTask::DoWork - Exit function (entitle)");

  return;
}

//////////////////////////////////
// BXSourcesItem functions //
//////////////////////////////////

BXSourcesItem::BXSourcesItem(const std::string& type, const std::string& id, const std::string& name, const std::string& sourceId, const std::string& sourceName, const std::string& sourceType, const std::string& sourceGeo) : BXItem(type,id,name)
{
  m_sourceId = sourceId;
  m_sourceName = sourceName;
  m_sourceType = sourceType;
  m_sourceGeo = sourceGeo;
}

}

