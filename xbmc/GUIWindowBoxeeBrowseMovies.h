#ifndef GUIWINDOWBOXEEBROWSEMOVIES_H_
#define GUIWINDOWBOXEEBROWSEMOVIES_H_

#include "GUIWindowBoxeeBrowseTvShows.h"
#include "BoxeeServerDirectory.h"

using namespace BOXEE;

class CAllMoviesWindowState : public CAllShowsWindowState
{
public:
  CAllMoviesWindowState(CGUIWindow* pWindow);
  virtual ~CAllMoviesWindowState() {}
  virtual CStdString CreatePath();
};

class CMyMoviesWindowState : public CMyShowsWindowState
{
public:
  CMyMoviesWindowState(CGUIWindow* pWindow);
  virtual ~CMyMoviesWindowState() {}
  virtual CStdString CreatePath();
};

class CGUIWindowBoxeeBrowseMovies : public CGUIWindowBoxeeBrowseTvShows
{
public:
  CGUIWindowBoxeeBrowseMovies();
	virtual ~CGUIWindowBoxeeBrowseMovies();

protected:

	virtual void SetSources();
	virtual void SetGenres();
};

#endif /*GUIWINDOWBOXEEBROWSEMOVIES_H_*/
