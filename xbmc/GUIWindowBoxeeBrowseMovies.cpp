
#include "GUIWindowBoxeeBrowseMovies.h"
#include "FileSystem/BoxeeServerDirectory.h"
#include "GUIWindowManager.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "URL.h"
#include "Util.h"
#include "BoxeeUtils.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "GUISettings.h"
#include "GUIDialogBoxeeSearch.h"
#include "boxee.h"

using namespace std;
using namespace BOXEE;

CAllMoviesWindowState::CAllMoviesWindowState(CGUIWindow* pWindow) : CAllShowsWindowState(pWindow)
{
  m_strSearchType = "movies";
}

CStdString CAllMoviesWindowState::CreatePath()
{
  CStdString strPath = "boxee://movies/movies";
  strPath = AddGuiStateParameters(strPath);
  return strPath;
}

CMyMoviesWindowState::CMyMoviesWindowState(CGUIWindow* pWindow) : CMyShowsWindowState(pWindow)
{
  m_strSearchType = "movies";
}

CStdString CMyMoviesWindowState::CreatePath()
{
  CStdString strPath = "boxee://movies/movies";
  strPath = AddGuiStateParameters(strPath);
  return strPath;
}

// /////////////////////////////////////////////////////////

CGUIWindowBoxeeBrowseMovies::CGUIWindowBoxeeBrowseMovies()
: CGUIWindowBoxeeBrowseTvShows(WINDOW_BOXEE_BROWSE_MOVIES, "boxee_browse_movies.xml")
{
  myShowsState = new CMyMoviesWindowState(this);
  allShowsState = new CAllMoviesWindowState(this);

  SetWindowState(myShowsState);
}

CGUIWindowBoxeeBrowseMovies::~CGUIWindowBoxeeBrowseMovies()
{
}

void CGUIWindowBoxeeBrowseMovies::SetGenres()
{
  BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetMovieGenres(m_vecGenres);
}

void CGUIWindowBoxeeBrowseMovies::SetSources()
{
  BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetMovieSources(m_vecSources);
}
