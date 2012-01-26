#ifndef GUIWINDOWBOXEEBROWSETVSHOWS_H_
#define GUIWINDOWBOXEEBROWSETVSHOWS_H_

#include "GUIWindowBoxeeBrowse.h"
#include "BoxeeServerDirectory.h"
#include "lib/libBoxee/bxgenresmanager.h"
#include "lib/libBoxee/bxsourcesmanager.h"
#include "GUIDialogBoxeeSortDropdown.h"

using namespace BOXEE;

class CRemoteTVShowsSource : public CBrowseWindowSource
{
public:
  CRemoteTVShowsSource(int iWindowID);
  virtual ~CRemoteTVShowsSource();

  void AddStateParameters(std::map <CStdString, CStdString>& mapOptions);
};

class CTVShowsStoreSource : public CBrowseWindowSource
{
public:
  CTVShowsStoreSource(int iWindowID);
  virtual ~CTVShowsStoreSource();

  void AddStateParameters(std::map <CStdString, CStdString>& mapOptions);
};

class CLocalTVShowsSource : public CBrowseWindowSource
{
public:
  CLocalTVShowsSource(int iWindowID);
  virtual ~CLocalTVShowsSource();

  void AddStateParameters(std::map <CStdString, CStdString>& mapOptions);
  void BindItems(CFileItemList& items);
};

class CTVShowsSubscriptionsSource : public CBrowseWindowSource
{
public:
  CTVShowsSubscriptionsSource(int iWindowID);
  virtual ~CTVShowsSubscriptionsSource();

  void AddStateParameters(std::map <CStdString, CStdString>& mapOptions);
};

class CTvShowsWindowState : public CBrowseWindowState
{
public:
  CTvShowsWindowState(CGUIWindowBoxeeBrowse* pWindow);
  virtual ~CTvShowsWindowState() {}

  virtual void Refresh(bool bResetSelected=false);

  //virtual bool OnBack();

  virtual void SetCategory(const CStdString& strCategory);
  void ClearCachedSources();

  // Filters
  virtual void ResetFilters();

  void SetGenre(const GenreItem& genre);
  GenreItem GetGenre() { return m_currentGenre; }

  void SetUnwatched(bool unwatched);

  void SetStore(const CStdString& strStoreId);

  void SetDefaultView();
  void SetDefaultCategory();

  // Flag that indicates that we are returning back from episodes screen of a specific tv show
  int   m_iLastSelectedItem;

  virtual CStdString GetItemSummary();

protected:
  // filters
  GenreItem  m_currentGenre;
  //CStdString m_strSource;
  CStdString  m_strStoreId;
  CStdString  m_strStoreName;
  bool        m_bUnwatched;

  CFileItemList m_storeList;
};

///////////////////////////////////////////////////////////////////////////
// TVshows browse window implementation
class CGUIWindowBoxeeBrowseTvShows : public CGUIWindowBoxeeBrowse
{
public:
  CGUIWindowBoxeeBrowseTvShows();
  CGUIWindowBoxeeBrowseTvShows(DWORD dwID, const CStdString &xmlFile);
  virtual ~CGUIWindowBoxeeBrowseTvShows();

  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);

  virtual bool OnClick(int iItem);

  void UpdateVideoCounters(bool bOn);

  virtual void ShowItems(CFileItemList& list, bool append);
  virtual bool HandleEmptyState();

  // Set of functions that update UI flags and properties
  void UpdateUIGenre(const CStdString& strValue);

protected:

  virtual void ConfigureState(const CStdString& param);
  virtual void GetStartMenusStructure(std::list<CFileItemList>& browseMenuLevelList);

  //void FillGenresList(CFileItemList& genres);
  //void FillReadyToWatchList(CFileItemList& ready);

  // Counter used to update local media resolver status every 120 sec
  int m_renderCount;
};


#endif /*GUIWINDOWBOXEEBROWSETVSHOWS_H_*/
