// Copyright © 2011 BOXEE. All rights reserved.
//
// C++ Interface: bxtrailersmanager
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEETRAILERSECTIONSMANAGER_H
#define BOXEETRAILERSECTIONSMANAGER_H

#include "bxscheduletaskmanager.h"
#include "bxboxeeservices.h"
#include <SDL/SDL.h>

#define DEFAULT_SECTION "LIBRARY" //should be the same as id of the default section the server gives us
#define GENRE_SECTION   "GENRES"
#define BOXOFFICE_TOP "BOXOFFICE_TOP"

namespace BOXEE
{

class TrailerSectionItem
{
public:
  TrailerSectionItem()
  {
    m_Id = DEFAULT_SECTION;
    m_Text = m_Id;
    m_strPath="";
  };
  virtual ~TrailerSectionItem(){};

  std::string m_Id;
  std::string m_Text;
  std::string m_strPath;
};

class BXTrailerSectionsManager
{
public:
  BXTrailerSectionsManager();
  virtual ~BXTrailerSectionsManager();

  bool Initialize();
  
  bool UpdateTrailerSections(unsigned long executionDelayInMS, bool repeat);
  bool GetMovieTrailerSections(std::vector<TrailerSectionItem>& movieTrailerSectionsVec);

private:

  void LockMovieTrailerSectionsList();
  void UnLockMovieTrailerSectionsList();
  void SetMovieTrailerSections(const std::vector<TrailerSectionItem>& vecMovieTrailerSection);

  std::vector<TrailerSectionItem> m_vecMovieTrailerSections;
  SDL_mutex* m_movieTrailersSectionListGuard;

  class CRequestVideoTrailerSectionsListFromServerTask : public BOXEE::BoxeeScheduleTask
  {
  public:

    CRequestVideoTrailerSectionsListFromServerTask(BXTrailerSectionsManager* m_taskHandler, unsigned long executionDelayInMS, bool repeat);
    virtual ~CRequestVideoTrailerSectionsListFromServerTask();
    virtual void DoWork();

  private:
    BXTrailerSectionsManager* m_taskHandler;
  };
};

}

#endif
