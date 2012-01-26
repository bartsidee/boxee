#ifndef GUIWINDOWBOXEEBROWSETVSHOWS_H_
#define GUIWINDOWBOXEEBROWSETVSHOWS_H_

#include "GUIWindowBoxeeBrowseWithPanel.h"
#include "BoxeeServerDirectory.h"
#include "lib/libBoxee/bxgenresmanager.h"
#include "lib/libBoxee/bxsourcesmanager.h"

using namespace BOXEE;

class CTvShowsWindowState : public CBrowseWindowState
{
public:
  CTvShowsWindowState(CGUIWindow* pWindow);
  virtual ~CTvShowsWindowState() {}

  virtual CStdString CreatePath();
  virtual CStdString AddGuiStateParameters(const CStdString& _strPath);

  virtual void Reset();

  // Filters
  void SetGenre(const CStdString& strGenre);
  CStdString GetGenre() { return m_strGenre; }

  void SetSource(const CStdString& strSourceId, const CStdString& strSourceName);
  CStdString GetSource() { return m_strSource; }

  void SetFree(bool bFree);

  void SetUnwatchedFilter(bool bOn);

  virtual void ResetFilters();
  void UpdateFilters(const CStdString& strPath) {}

  virtual void SortItems(CFileItemList &items);

  virtual bool OnBack();

  virtual void OnUnwatched();
  virtual void OnFree();

  void RestoreWindowState();

  bool m_bInShow;

protected:

  // filters
  CStdString m_strGenre;
  CStdString m_strSource;

  bool m_bFree;
  bool m_bUnwatched;
};

class CMyShowsWindowState : public  CTvShowsWindowState
{
public:
  CMyShowsWindowState(CGUIWindow* pWindow);
  virtual ~CMyShowsWindowState() {}
  void Reset();
  virtual CStdString AddGuiStateParameters(const CStdString& _strPath);
};

class CAllShowsWindowState : public  CTvShowsWindowState
{
public:
  CAllShowsWindowState(CGUIWindow* pWindow);
  virtual ~CAllShowsWindowState() {}
  void Reset();
  virtual CStdString AddGuiStateParameters(const CStdString& _strPath);
};


class CGUIWindowBoxeeBrowseTvShows : public CGUIWindowBoxeeBrowseWithPanel
{
public:
  CGUIWindowBoxeeBrowseTvShows();
  CGUIWindowBoxeeBrowseTvShows(DWORD dwID, const CStdString &xmlFile);
  virtual ~CGUIWindowBoxeeBrowseTvShows();
	
  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);
  virtual bool ProcessPanelMessages(CGUIMessage& message);


  virtual bool OnClick(int iItem);
  virtual void Render();
  void SetVideoCounters(bool bOn);

protected:

  virtual void SetGenres();
  virtual void SetSources();

  void FillGenresList(CFileItemList& genres);
  void FillSourcesList(CFileItemList& sources);

  std::vector<GenreItem> m_vecGenres;
  std::vector<BXSourcesItem> m_vecSources;

  CTvShowsWindowState* myShowsState;
  CTvShowsWindowState* allShowsState;
  int                  m_renderCount;



};


#endif /*GUIWINDOWBOXEEBROWSETVSHOWS_H_*/
