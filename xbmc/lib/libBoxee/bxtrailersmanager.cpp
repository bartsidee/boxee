//
// C++ Implementation: bxservicesmanager
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxtrailersmanager.h"
#include "boxee.h"
#include "bxconfiguration.h"
#include "bxexceptions.h"
#include "logger.h"
#include "MetadataResolverVideo.h"
#include "../../Application.h"
#include "GUIUserMessages.h"
#include "../../guilib/GUIWindowManager.h"
#include "../../LangInfo.h"
#include "BoxeeBrowseMenuManager.h"

namespace BOXEE
{

BXTrailerSectionsManager::BXTrailerSectionsManager()
{
  m_movieTrailersSectionListGuard = SDL_CreateMutex();
}

BXTrailerSectionsManager::~BXTrailerSectionsManager()
{
  SDL_DestroyMutex(m_movieTrailersSectionListGuard);
}

bool BXTrailerSectionsManager::Initialize()
{
  m_vecMovieTrailerSections.clear();
  
  return true;
}

bool BXTrailerSectionsManager::UpdateTrailerSections(unsigned long executionDelayInMS, bool repeat)
{
  bool retVal = true;

  CRequestVideoTrailerSectionsListFromServerTask* reqMovieTrailerSectionsListTask = new CRequestVideoTrailerSectionsListFromServerTask(this,0,false);

  if(reqMovieTrailerSectionsListTask)
  {
    Boxee::GetInstance().GetBoxeeScheduleTaskManager().AddScheduleTask(reqMovieTrailerSectionsListTask);
    retVal &= true;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"BXTrailerSectionsManager::UpdateTrailers - FAILED to allocate CRequestVideoTrailerSectionsListFromServerTask (trailer)");
    retVal &= false;
  }

  return retVal;
}

void BXTrailerSectionsManager::LockMovieTrailerSectionsList()
{
  SDL_LockMutex(m_movieTrailersSectionListGuard);
}

void BXTrailerSectionsManager::UnLockMovieTrailerSectionsList()
{
  SDL_UnlockMutex(m_movieTrailersSectionListGuard);
}

void BXTrailerSectionsManager::SetMovieTrailerSections(const std::vector<TrailerSectionItem>& vecMovieTrailers)
{
  bool vectorChanged = false;

  //we need to check for any changes
  LockMovieTrailerSectionsList();

  if (m_vecMovieTrailerSections.size() != vecMovieTrailers.size())
  {
    // if they are different in size, there was a change
    vectorChanged = true;
  }

  m_vecMovieTrailerSections = vecMovieTrailers;

  UnLockMovieTrailerSectionsList();

  if (vectorChanged)
  {
    //send message to the menu to update
    CLog::Log(LOGDEBUG,"BXTrailerSectionsManager::SetMovieTrailers, there was a change in the vector, sending update to the menu (trailers update)");

    CBoxeeBrowseMenuManager::GetInstance().ClearDynamicMenuButtons("mn_library_movies_trailers");
  }

}

bool BXTrailerSectionsManager::GetMovieTrailerSections(std::vector<TrailerSectionItem>& movieTrailersVec)
{
  LockMovieTrailerSectionsList();

  movieTrailersVec = m_vecMovieTrailerSections;

  UnLockMovieTrailerSectionsList();

  return true;
}


/////////////////////////////////////////////////////
// RequestVideoTrailerListFromServerTask functions //
/////////////////////////////////////////////////////

BXTrailerSectionsManager::CRequestVideoTrailerSectionsListFromServerTask::CRequestVideoTrailerSectionsListFromServerTask(BXTrailerSectionsManager* taskHandler, unsigned long executionDelayInMS, bool repeat):BoxeeScheduleTask("RequestVideoGneresList",executionDelayInMS,repeat)
{
  m_taskHandler = taskHandler;
}

BXTrailerSectionsManager::CRequestVideoTrailerSectionsListFromServerTask::~CRequestVideoTrailerSectionsListFromServerTask()
{

}

void BXTrailerSectionsManager::CRequestVideoTrailerSectionsListFromServerTask::DoWork()
{
  LOG(LOG_LEVEL_DEBUG,"CRequestVideoTrailersListFromServerTask::DoWork - Enter function (trailer)");

  if (!g_application.ShouldConnectToInternet())
  {
    LOG(LOG_LEVEL_DEBUG,"CRequestVideoTrailersListFromServerTask::DoWork - [ShouldConnectToInternet=FALSE] -> Exit function (trailer)");
    return;
  }

  std::string strLink = BXConfiguration::GetInstance().GetStringParam("Boxee.Resolver.Server","http://res.boxee.tv");
  strLink += "/titles/trailersections";

  if (!g_langInfo.GetLanguageCode().IsEmpty())
  {
    strLink += "?lang=";
    strLink += g_langInfo.GetLanguageCode().c_str();
  }

  BXXMLDocument xmlDoc;

  xmlDoc.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  xmlDoc.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

  if (!xmlDoc.LoadFromURL(strLink))
  {
    LOG(LOG_LEVEL_ERROR,"CRequestVideoTrailersListFromServerTask::DoWork - FAILED to get genres from [strLink=%s] (trailer)",strLink.c_str());
    return;
  }

  TiXmlElement *pRootElement = xmlDoc.GetDocument().RootElement();
  if (!pRootElement || strcmpi(pRootElement->Value(), "sections") != 0)
  {
    LOG(LOG_LEVEL_ERROR,"CRequestVideoTrailersListFromServerTask::DoWork - could not parse genres. [pRootElement=%p] (trailer)",pRootElement);
    return;
  }

  std::vector<TrailerSectionItem> trailerVec;

  const TiXmlNode* pTag = 0;
  while ((pTag = pRootElement->IterateChildren(pTag)))
  {
    if (pTag && pTag->ValueStr() == "section")
    {
      const TiXmlNode* pValue = pTag->FirstChild();

      if (pValue)
      {
        TrailerSectionItem trailerItem;
        const char* attribute = ((TiXmlElement*)pTag)->Attribute("id");
        if (attribute)
          trailerItem.m_Id = std::string(attribute);

        attribute = ((TiXmlElement*)pTag)->Attribute("path");
        if (attribute)
          trailerItem.m_strPath = std::string(attribute);

        trailerItem.m_Text = pValue->ValueStr();

        LOG(LOG_LEVEL_DEBUG,"CRequestVideoTrailersListFromServerTask::DoWork - added new section: id=[%s], text=[%s], path=[%s] (haim)",trailerItem.m_Id.c_str(), trailerItem.m_Text.c_str(), trailerItem.m_strPath.c_str());

        trailerVec.push_back(trailerItem);
      }
    }
  }

  m_taskHandler->SetMovieTrailerSections(trailerVec);

  return;
}

}
