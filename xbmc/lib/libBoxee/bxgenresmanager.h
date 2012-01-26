// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxgenresmanager
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEGENRESMANAGER_H
#define BOXEEGENRESMANAGER_H

#include "bxscheduletaskmanager.h"
#include "bxboxeeservices.h"
#include <SDL/SDL.h>

namespace BOXEE
{

class GenreItem
{
public:
  GenreItem()
  {
    m_genreId = "all";
    m_genreText = m_genreId;
  };
  virtual ~GenreItem(){};

  std::string m_genreId;
  std::string m_genreText;
  std::string m_type;
};

class BXGenresManager
{
public:
  BXGenresManager();
  virtual ~BXGenresManager();

  bool Initialize();
  
  bool UpdateGenres(unsigned long executionDelayInMS, bool repeat);
  bool GetMovieGenres(std::vector<GenreItem>& movieGenresVec);
  bool GetTvGenres(std::vector<GenreItem>& tvGenresVec);
  bool GetBadWords(std::vector<std::string>& badWordsVec);

private:

  void LockMovieGenresList();
  void UnLockMovieGenresList();
  void SetMovieGenres(const std::vector<GenreItem>& vecMovieGenres);

  void LockTvGenresList();
  void UnLockTvGenresList();
  void SetTvGenres(const std::vector<GenreItem>& vecTvGenres);

  void LockBadWordsList();
  void UnLockBadWordsList();
  void SetBadWords(const std::vector<std::string>& vecBadWords);

  std::vector<GenreItem> m_vecMovieGenres;
  SDL_mutex* m_movieGenresListGuard;

  std::vector<GenreItem> m_vecTvGenres;
  SDL_mutex* m_tvGenresListGuard;
  
  std::vector<std::string> m_vecBadWords;
  SDL_mutex* m_badWordsListGuard;

  class CRequestVideoGenresListFromServerTask : public BOXEE::BoxeeScheduleTask
  {
  public:

    CRequestVideoGenresListFromServerTask(BXGenresManager* m_taskHandler, const std::string& genresType, unsigned long executionDelayInMS, bool repeat);
    virtual ~CRequestVideoGenresListFromServerTask();
    virtual void DoWork();

  private:

    BXGenresManager* m_taskHandler;
    std::string m_genresType;
  };

  class CRequestBadWordsListFromServerTask : public BOXEE::BoxeeScheduleTask
  {
  public:

    CRequestBadWordsListFromServerTask(BXGenresManager* m_taskHandler, unsigned long executionDelayInMS, bool repeat);
    virtual ~CRequestBadWordsListFromServerTask();
    virtual void DoWork();

  private:

    BXGenresManager* m_taskHandler;
  };
};

}

#endif
