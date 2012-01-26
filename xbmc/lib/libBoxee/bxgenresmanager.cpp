//
// C++ Implementation: bxservicesmanager
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxgenresmanager.h"
#include "boxee.h"
#include "bxconfiguration.h"
#include "bxexceptions.h"
#include "logger.h"
#include "MetadataResolverVideo.h"
#include "../../Application.h"

namespace BOXEE
{

BXGenresManager::BXGenresManager()
{
  m_movieGenresListGuard = SDL_CreateMutex();
  m_tvGenresListGuard = SDL_CreateMutex();
  m_badWordsListGuard = SDL_CreateMutex();

  m_vecMovieGenres.clear();
  m_vecTvGenres.clear();
}

BXGenresManager::~BXGenresManager()
{
  SDL_DestroyMutex(m_movieGenresListGuard);
  SDL_DestroyMutex(m_tvGenresListGuard);
  SDL_DestroyMutex(m_badWordsListGuard);
}

bool BXGenresManager::Initialize()
{
  m_vecMovieGenres.clear();
  m_vecTvGenres.clear();

  return true;
}

bool BXGenresManager::UpdateGenres(unsigned long executionDelayInMS, bool repeat)
{
  bool retVal = true;

  // add task for loading MOVIE genres
  CRequestVideoGenresListFromServerTask* reqMovieGenresListTask = new CRequestVideoGenresListFromServerTask(this,"movie",0,false);

  if(reqMovieGenresListTask)
  {
    Boxee::GetInstance().GetBoxeeScheduleTaskManager().AddScheduleTask(reqMovieGenresListTask);
    retVal &= true;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"BXGenresManager::UpdateGenres - FAILED to allocate RequestVideoGenresListFromServerTask for [type=movie] (genres)");
    retVal &= false;
  }

  // add task for loading TV genres
  CRequestVideoGenresListFromServerTask* reqTvGenresListTask = new CRequestVideoGenresListFromServerTask(this,"tv",0,false);

  if(reqTvGenresListTask)
  {
    Boxee::GetInstance().GetBoxeeScheduleTaskManager().AddScheduleTask(reqTvGenresListTask);
    retVal &= true;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"BXGenresManager::UpdateGenres - FAILED to allocate RequestVideoGenresListFromServerTask for [type=tv] (genres)");
    retVal &= false;
  }

  // add task for loading bad words
  CRequestBadWordsListFromServerTask* reqBadWordsListTask = new CRequestBadWordsListFromServerTask(this,0,false);

  if(reqBadWordsListTask)
  {
    Boxee::GetInstance().GetBoxeeScheduleTaskManager().AddScheduleTask(reqBadWordsListTask);
    retVal &= true;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"BXGenresManager::UpdateGenres - FAILED to allocate CRequestBadWordsListFromServerTask for [type=tv] (genres)");
    retVal &= false;
  }

  return retVal;
}

void BXGenresManager::LockMovieGenresList()
{
  SDL_LockMutex(m_movieGenresListGuard);
}

void BXGenresManager::UnLockMovieGenresList()
{
  SDL_UnlockMutex(m_movieGenresListGuard);
}

void BXGenresManager::LockTvGenresList()
{
  SDL_LockMutex(m_tvGenresListGuard);
}

void BXGenresManager::UnLockTvGenresList()
{
  SDL_UnlockMutex(m_tvGenresListGuard);
}

void BXGenresManager::LockBadWordsList()
{
  SDL_LockMutex(m_badWordsListGuard);
}

void BXGenresManager::UnLockBadWordsList()
{
  SDL_UnlockMutex(m_badWordsListGuard);
}

void BXGenresManager::SetMovieGenres(const std::vector<GenreItem>& movieGenresVec)
{
  LockMovieGenresList();

  m_vecMovieGenres = movieGenresVec;

  UnLockMovieGenresList();
}

bool BXGenresManager::GetMovieGenres(std::vector<GenreItem>& movieGenresVec)
{
  LockMovieGenresList();

  movieGenresVec = m_vecMovieGenres;

  UnLockMovieGenresList();

  return true;
}

void BXGenresManager::SetTvGenres(const std::vector<GenreItem>& tvGenresVec)
{
  LockTvGenresList();

  m_vecTvGenres = tvGenresVec;

  UnLockTvGenresList();
}

bool BXGenresManager::GetTvGenres(std::vector<GenreItem>& tvGenresVec)
{
  LockTvGenresList();

  tvGenresVec = m_vecTvGenres;

  UnLockTvGenresList();

  return true;
}

void BXGenresManager::SetBadWords(const std::vector<std::string>& badWordsVec)
{
  LockBadWordsList();

  m_vecBadWords = badWordsVec;

  UnLockBadWordsList();

  if ((int)m_vecBadWords.size() > 0)
  {
    CMetadataResolverVideo::SetBadWords(m_vecBadWords);
  }
}

bool BXGenresManager::GetBadWords(std::vector<std::string>& badWordsVec)
{
  LockBadWordsList();

  badWordsVec = m_vecBadWords;

  UnLockBadWordsList();

  return true;
}

////////////////////////////////////////////////////
// RequestVideoGenresListFromServerTask functions //
////////////////////////////////////////////////////

BXGenresManager::CRequestVideoGenresListFromServerTask::CRequestVideoGenresListFromServerTask(BXGenresManager* taskHandler, const std::string& genresType, unsigned long executionDelayInMS, bool repeat):BoxeeScheduleTask("RequestVideoGneresList",executionDelayInMS,repeat)
{
  m_genresType = genresType;
  m_taskHandler = taskHandler;
}

BXGenresManager::CRequestVideoGenresListFromServerTask::~CRequestVideoGenresListFromServerTask()
{

}

void BXGenresManager::CRequestVideoGenresListFromServerTask::DoWork()
{
  LOG(LOG_LEVEL_DEBUG,"CRequestVideoGenresListFromServerTask::DoWork - Enter function (genres)");

  if (!g_application.IsConnectedToInternet())
  {
    LOG(LOG_LEVEL_DEBUG,"CRequestVideoGenresListFromServerTask::DoWork - [IsConnectedToInternet=FALSE] -> Exit function (genres)");
    return;
  }

  std::string strLink = BXConfiguration::GetInstance().GetStringParam("Boxee.Resolver.Server","http://res.boxee.tv");
  strLink += "/titles/genres?type=";
  strLink += m_genresType;

  BXXMLDocument xmlDoc;

  xmlDoc.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  xmlDoc.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

  if (!xmlDoc.LoadFromURL(strLink))
  {
    LOG(LOG_LEVEL_ERROR,"CRequestVideoGenresListFromServerTask::DoWork - FAILED to get genres from [strLink=%s] (genres)",strLink.c_str());
    return;
  }

  TiXmlElement *pRootElement = xmlDoc.GetDocument().RootElement();
  if (!pRootElement || strcmpi(pRootElement->Value(), "genres") != 0)
  {
    LOG(LOG_LEVEL_ERROR,"CRequestVideoGenresListFromServerTask::DoWork - could not parse genres. [pRootElement=%p] (genres)",pRootElement);
    return;
  }

  std::vector<GenreItem> genresVec;

  const TiXmlNode* pTag = 0;
  while ((pTag = pRootElement->IterateChildren(pTag)))
  {
    if (pTag && pTag->ValueStr() == "genre")
    {
      const TiXmlNode* pValue = pTag->FirstChild();

      if (pValue)
      {
        GenreItem genreItem;

        genreItem.m_type = m_genresType;
        genreItem.m_genreText = pValue->ValueStr();
        genreItem.m_genreId = std::string(((TiXmlElement*)pTag)->Attribute("id"));

        genresVec.push_back(genreItem);
      }
    }
  }

  if (m_genresType == "movie")
  {
    m_taskHandler->SetMovieGenres(genresVec);
  }
  else if (m_genresType == "tv")
  {
    m_taskHandler->SetTvGenres(genresVec);
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"CRequestVideoGenresListFromServerTask::DoWork - FAILED to set genres [size=%d] because of unknown genre type [genresType=%s] (genres)",(int)genresVec.size(),m_genresType.c_str());
    return;
  }

  return;
}

/////////////////////////////////////////////////
// RequestBadWordsListFromServerTask functions //
/////////////////////////////////////////////////

BXGenresManager::CRequestBadWordsListFromServerTask::CRequestBadWordsListFromServerTask(BXGenresManager* taskHandler, unsigned long executionDelayInMS, bool repeat):BoxeeScheduleTask("RequestVideoGneresList",executionDelayInMS,repeat)
{
  m_taskHandler = taskHandler;
}

BXGenresManager::CRequestBadWordsListFromServerTask::~CRequestBadWordsListFromServerTask()
{

}

void BXGenresManager::CRequestBadWordsListFromServerTask::DoWork()
{
  LOG(LOG_LEVEL_DEBUG,"CRequestBadWordsListFromServerTask::DoWork - Enter function (badwords)");

  if (!g_application.IsConnectedToInternet())
  {
    LOG(LOG_LEVEL_DEBUG,"CRequestBadWordsListFromServerTask::DoWork - [IsConnectedToInternet=FALSE] -> Exit function (badwords)");
    return;
  }

  std::string strLink = BXConfiguration::GetInstance().GetStringParam("Boxee.Resolver.Server","http://res.boxee.tv");
  strLink += "/title/badwords";

  BXXMLDocument xmlDoc;

  xmlDoc.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  xmlDoc.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

  if (!xmlDoc.LoadFromURL(strLink))
  {
    LOG(LOG_LEVEL_ERROR,"CRequestBadWordsListFromServerTask::DoWork - FAILED to get bad words from [strLink=%s] (badwords)",strLink.c_str());
    return;
  }

  TiXmlElement *pRootElement = xmlDoc.GetDocument().RootElement();
  if (!pRootElement || strcmpi(pRootElement->Value(), "badwords") != 0)
  {
    LOG(LOG_LEVEL_ERROR,"CRequestBadWordsListFromServerTask::DoWork - could not parse badwords. [pRootElement=%p] (badwords)",pRootElement);
    return;
  }

  std::vector<std::string> badWordsVec;

  const TiXmlNode* pTag = 0;
  while ((pTag = pRootElement->IterateChildren(pTag)))
  {
    if (pTag && pTag->ValueStr() == "badword")
    {
      const TiXmlNode* pValue = pTag->FirstChild();

      if (pValue)
      {
        badWordsVec.push_back(pValue->ValueStr());
      }
    }
  }

  m_taskHandler->SetBadWords(badWordsVec);

  return;
}

}
