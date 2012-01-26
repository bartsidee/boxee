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
#include "GUIUserMessages.h"
#include "../../guilib/GUIWindowManager.h"
#include "utils/log.h"
#include "../../Util.h"
#include "BoxeeBrowseMenuManager.h"

#define ENTITLEMENTS_DELIMITER ";"
#define OFFER_DELIMITER        ","

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
    LOG(LOG_LEVEL_ERROR,"BXSourcesManager::UpdateSourcesList - FAILED to allocate RequestSourcesListFromServerTask (source)");
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
  
  //we need to check for any changes
  bool vectorChanged = false;

  if (m_sourcesList.GetNumOfSources() != sourcesList.GetNumOfSources())
  {
    // if they are different in size, there was a change
    vectorChanged = true;
  }

  LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::CopyServicesList - going to copy sourcesList [size=%d]. [currSize=%d][vectorChanged=%d] (source)",sourcesList.GetNumOfSources(),m_sourcesList.GetNumOfSources(),vectorChanged);

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
	  for (unsigned int j = 0; j < vecTokens.size(); j++)
	  {
		  if (vecTokens[j] == "movie")
		  {
			  m_movieSourcesList.AddSource(bxSource);
		  }
		  else if (vecTokens[j] == "tv")
		  {
			  m_tvSourcesList.AddSource(bxSource);
		  }
		  else
		  {
			  LOG(LOG_LEVEL_ERROR,"BXSourcesManager::CopySourcesList - Unknown source type %s (source)", vecTokens[j].c_str());
		  }
	  }
  }

  if (vectorChanged)
  {
    //send message to the menu to update
    CLog::Log(LOGDEBUG,"BXSourcesManager::CopySourcesList - there was a change in the vector. sending update to the menu (source)");

    CBoxeeBrowseMenuManager::GetInstance().ClearDynamicMenuButtons("mn_library_movies_providers");
    CBoxeeBrowseMenuManager::GetInstance().ClearDynamicMenuButtons("mn_library_shows_providers");
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
    LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::GetSourcesList - In offline mode. Going to return FALSE (source)");
    return false;
  }

  bool sourcesListWasLoaded = false;
  
  LockSourcesList();

  sourcesListWasLoaded = m_sourcesList.IsLoaded();
  
  while (!sourcesListWasLoaded)
  {
    // SourcesList ISN'T loaded yet -> UnLock the SourcesList and wait for it to load
    
    UnLockSourcesList();

    LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::GetSourcesList - SourcesList is not loaded yet. Going to try again in [%dms] (source)",DELAY_FOR_CHECK_FEED_LOADED);

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

  LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::GetSourcesList - Exit function. After set [SourcesListSize=%d] (source)",sourcesList.GetNumOfSources());

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
    std::string sourceThumb = obj.GetValue("source_thumb");
    std::string sourcePremium = obj.GetValue("source_premium");
    std::string sourceOffer = obj.GetValue("source_offer");

    BXSourcesItem sourcesItem(type, id, name, sourceId, sourceName, sourceType, sourceGeo, sourceThumb, sourcePremium, sourceOffer);
    
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
    LOG(LOG_LEVEL_ERROR,"BXSourcesManager::GetSourcesIds - Call to GetSources returned [SourcesListSize=%d] (source)",numOfSources);
    return false;
  }

  for(int i=0; i<numOfSources; i++)
  {
    BXObject obj = sources.GetSource(i);

    sourcesIdsVec.push_back(obj.GetID());
  }

  LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::GetSourcesIds - Going to return  [sourcesIdsVecSize=%d] (source)",sourcesIdsVec.size());

  return true;
}

bool BXSourcesManager::IsInSources(const std::string& productsList)
{
  if (productsList.empty())
  {
    LOG(LOG_LEVEL_WARNING,"BXSourcesManager::IsInSources - Enter with an EMPTY productsList therefore going to return FALSE. [productsList=%s] (source)",productsList.c_str());
    return false;
  }

  bool sourcesListWasLoaded = false;

  LockSourcesList();

  sourcesListWasLoaded = m_sourcesList.IsLoaded();

  while (!sourcesListWasLoaded)
  {
    // SourcesList ISN'T loaded yet -> UnLock the SourcesList and wait for it to load

    UnLockSourcesList();

    LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::IsInSources - SourcesList is not loaded yet. Going to try again in [%dms] (source)",DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockSourcesList();

    sourcesListWasLoaded = m_sourcesList.IsLoaded();
  }

  int numOfSources = m_sourcesList.GetNumOfSources();

  LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::IsInSources - Enter function with [productsList=%s][SourcesListSize=%d] (source)",productsList.c_str(),numOfSources);

  if(numOfSources < 1)
  {
    UnLockSourcesList();
    LOG(LOG_LEVEL_WARNING,"BXSourcesManager::IsInSources - [SourcesListSize=%d] therefore going to return FALSE (source)",numOfSources);
    return false;
  }

  std::set<CStdString> productsSet;
  int numOfProducts = BoxeeUtils::StringTokenize(productsList,productsSet,ENTITLEMENTS_DELIMITER,true,true);
  if (numOfProducts < 1)
  {
    UnLockSourcesList();
    LOG(LOG_LEVEL_WARNING,"BXSourcesManager::IsInSources - Tokenize of [productsList=%s] return [size=%d] therefore going to return FALSE (source)",productsList.c_str(),numOfProducts);
    return false;
  }

  for (int i=0; i < numOfSources; i++)
  {
    BXObject product = m_sourcesList.GetSource(i);

    std::string productId = product.GetValue("product_id");
    if(productsSet.find(productId) != productsSet.end())
    {
      UnLockSourcesList();
      LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::IsInSources - [productId=%s] WAS found in the SourcesList. Exit function and return TRUE (source)",productId.c_str());
      return true;
    }
  }

  UnLockSourcesList();
  LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::IsInSources - None of [productsList=%s] was found in the SourcesList. Exit function and return FALSE (source)",productsList.c_str());
  return false;
}

/////////////////////
// ExcludedSources //
/////////////////////

bool BXSourcesManager::UpdateExcludedSources(unsigned long executionDelayInMS, bool repeat)
{
  RequestExcludedSourcesFromServerTask* reqExcludedSourcesTask = new RequestExcludedSourcesFromServerTask(this,executionDelayInMS,repeat);

  if(!reqExcludedSourcesTask)
  {
    LOG(LOG_LEVEL_ERROR,"BXSourcesManager::UpdateExcludedSources - FAILED to allocate RequestExcludedSourcesFromServerTask (cf)");
    return false;
  }

  if(executionDelayInMS == 0)
  {
    // In case the request is for immediate execution -> Set the status of the list to NOT LOADED in order
    // for get function to wait for update

    LockExcludedSources();

    m_isExcludedSourcesLoaded = false;

    UnLockExcludedSources();
  }

  Boxee::GetInstance().GetBoxeeScheduleTaskManager().AddScheduleTask(reqExcludedSourcesTask);
  return true;
}

void BXSourcesManager::LockExcludedSources()
{
  SDL_LockMutex(m_excludedSourcesGuard);
}

void BXSourcesManager::UnLockExcludedSources()
{
  SDL_UnlockMutex(m_excludedSourcesGuard);
}

void BXSourcesManager::SetExcludedSourcesLoaded(bool isLoaded)
{
  LockSourcesList();

  m_isExcludedSourcesLoaded = isLoaded;

  UnLockSourcesList();
}

void BXSourcesManager::SetExcludedSources(const std::string& excludedSources)
{
  LockExcludedSources();

  m_excludedSources = excludedSources;
  m_isExcludedSourcesLoaded = true;

  UnLockExcludedSources();
}

std::string BXSourcesManager::GetExcludedSources()
{
  LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::GetExcludedSources - Enter function (cf)");

  std::string excludedSources;

  if(BOXEE::Boxee::GetInstance().IsInOfflineMode())
  {
    LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::GetExcludedSources - In offline mode. Going to return [%s] (cf)",excludedSources.c_str());
    return excludedSources;
  }

  LockExcludedSources();

  while (!m_isExcludedSourcesLoaded)
  {
    // ExcludedSources ISN'T loaded yet -> UnLock the ExcludedSources and wait for it to load

    UnLockExcludedSources();

    LOG(LOG_LEVEL_DEBUG,"BXSourcesManager::GetExcludedSources - ExcludedSources is not loaded yet. Going to try again in [%dms] (cf)",DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockExcludedSources();
  }

  excludedSources = m_excludedSources;

  UnLockExcludedSources();

  return excludedSources;
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
  LOG(LOG_LEVEL_DEBUG,"RequestSourcesListFromServerTask::DoWork - Enter function (source)");

  if (!g_application.ShouldConnectToInternet())
  {
    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetSourcesListIsLoaded(true);

    LOG(LOG_LEVEL_DEBUG,"RequestSourcesListFromServerTask::DoWork - [ShouldConnectToInternet=FALSE] -> Exit function (source)");
    return;
  }

  BXBoxeeSources sourcesList;
  
  sourcesList.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  sourcesList.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

  std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.SourcesListUrl","http://app.boxee.tv/titles/sources");

  sourcesList.LoadFromURL(strUrl);

  bool isLoaded = m_taskHandler->m_sourcesList.IsLoaded();
  long lastRetCode = sourcesList.GetLastRetCode();

  LOG(LOG_LEVEL_DEBUG,"RequestSourcesListFromServerTask::DoWork - After LoadFromURL. [lastRetCode=%d][isLoaded=%d][size=%d][currSize=%d] (source)",lastRetCode,isLoaded,sourcesList.GetNumOfSources(),m_taskHandler->m_sourcesList.GetNumOfSources());

  if (!isLoaded || lastRetCode == 200)
  {
    ////////////////////////////////////////////
    // copy return result from the server if: //
    // a) recommendationsList isn't loaded    //
    // b) the server returned 200             //
    ////////////////////////////////////////////

    m_taskHandler->CopySourcesList(sourcesList);
  }

  LOG(LOG_LEVEL_DEBUG,"RequestSourcesListFromServerTask::DoWork - Exit function (source)");

  return;
}

////////////////////////////////////////////////////
// RequestExcludedSourcesFromServerTask functions //
////////////////////////////////////////////////////

BXSourcesManager::RequestExcludedSourcesFromServerTask::RequestExcludedSourcesFromServerTask(BXSourcesManager* taskHandler, unsigned long executionDelayInMS, bool repeat):BoxeeScheduleTask("RequestSourcesList",executionDelayInMS,repeat)
{
  m_taskHandler = taskHandler;
}

BXSourcesManager::RequestExcludedSourcesFromServerTask::~RequestExcludedSourcesFromServerTask()
{

}

void BXSourcesManager::RequestExcludedSourcesFromServerTask::DoWork()
{
  LOG(LOG_LEVEL_DEBUG,"RequestExcludedSourcesFromServerTask::DoWork - Enter function (cf)");

  if (!g_application.ShouldConnectToInternet())
  {
    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetExcludedSourcesLoaded(true);

    LOG(LOG_LEVEL_DEBUG,"RequestExcludedSourcesFromServerTask::DoWork - [ShouldConnectToInternet=FALSE] -> Exit function (cf)");
    return;
  }

  BXXMLDocument doc;
  doc.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());
  doc.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.ExcludedSourcesUrl","http://app.boxee.tv/api/excluded_sources");
  if (!doc.LoadFromURL(strUrl))
  {
    CLog::Log(LOGERROR,"RequestExcludedSourcesFromServerTask::DoWork - FAILED to get excluded sources (cf)");

    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetExcludedSourcesLoaded(true);

    return;
  }

  TiXmlElement* root = doc.GetRoot();

  if(!root)
  {
    CLog::Log(LOGERROR,"RequestExcludedSourcesFromServerTask::DoWork - FAILED to get root from BXXMLDocument of the excluded channel response (cf)");

    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetExcludedSourcesLoaded(true);

    return;
  }

  if((strcmp(root->Value(),"boxee") != 0))
  {
    CLog::Log(LOGERROR,"RequestExcludedSourcesFromServerTask::DoWork - FAILED to parse excluded channel response because the root tag ISN'T <boxee> (cf)");

    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetExcludedSourcesLoaded(true);

    return;
  }

  TiXmlElement* childElem = NULL;
  childElem = root->FirstChildElement();

  while (childElem)
  {
    if (strcmp(childElem->Value(), "excluded_sources") == 0)
    {
      CStdString excludedSources(childElem->GetText());
      CLog::Log(LOGDEBUG,"RequestExcludedSourcesFromServerTask::DoWork - <excluded_sources> tag was found. [excludedSourcesStr=%s] (cf)",excludedSources.c_str());

      m_taskHandler->SetExcludedSources(excludedSources);
      return;
    }

    childElem = childElem->NextSiblingElement();
  }

  LOG(LOG_LEVEL_DEBUG,"RequestExcludedSourcesFromServerTask::DoWork - Exit function (cf)");

  // set loaded to true so Get() functions won't wait forever
  m_taskHandler->SetExcludedSourcesLoaded(true);

  return;
}

/////////////////////////////
// BXSourcesItem functions //
/////////////////////////////

BXSourcesItem::BXSourcesItem(const std::string& type, const std::string& id, const std::string& name, const std::string& sourceId, const std::string& sourceName, const std::string& sourceType, const std::string& sourceGeo, const std::string& sourceThumb, const std::string& sourcePremium, const std::string& sourceOffer) : BXItem(type,id,name)
{
  m_sourceId = sourceId;
  m_sourceName = sourceName;
  m_sourceType = sourceType;
  m_sourceGeo = sourceGeo;
  m_sourceThumb = sourceThumb;
  m_sourcePremium = sourcePremium;
  m_sourceOffer = sourceOffer;
}

}

