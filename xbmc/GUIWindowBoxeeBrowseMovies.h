#ifndef GUIWINDOWBOXEEBROWSEMOVIES_H_
#define GUIWINDOWBOXEEBROWSEMOVIES_H_

#include "GUIWindowBoxeeBrowseTvShows.h"
#include "BoxeeServerDirectory.h"
#include "lib/libBoxee/bxtrailersmanager.h"

using namespace BOXEE;

class CRemoteMoviesSource : public CBrowseWindowSource
{

public:
  CRemoteMoviesSource(int iWindowID);
  virtual ~CRemoteMoviesSource();
  virtual void AddStateParameters(std::map <CStdString, CStdString>& mapOptions);

};

class CMovieStoreSource : public CBrowseWindowSource
{

public:
  CMovieStoreSource(int iWindowID);
  virtual ~CMovieStoreSource();
  virtual void AddStateParameters(std::map <CStdString, CStdString>& mapOptions);

};

class CLocalMoviesSource : public CBrowseWindowSource
{
public:
  CLocalMoviesSource(int iWindowID);
  virtual ~CLocalMoviesSource();
  virtual void AddStateParameters(std::map <CStdString, CStdString>& mapOptions);

};

class CTrailersSource : public CBrowseWindowSource
{

public:
  CTrailersSource(int iWindowID);
  virtual ~CTrailersSource();
  virtual void AddStateParameters(std::map <CStdString, CStdString>& mapOptions);

};

////////////////////////////////////////////////////////////

class CMoviesWindowState : public CTvShowsWindowState
{
public:
  CMoviesWindowState(CGUIWindowBoxeeBrowse* pWindow);
  virtual ~CMoviesWindowState() {}
  virtual void SetCategory(const CStdString& strCategory);
  virtual void SetTrailerSection(const TrailerSectionItem& section);

  virtual CStdString GetItemSummary();

  void SetStore(const CStdString& strStoreId);
  void SetSource(const CStdString& strSourcePath);
  void OnBind(CFileItemList& items);
private:
  TrailerSectionItem m_selectedSection;
};

// /////////////////////////////////////////////////////////

class CGUIWindowBoxeeBrowseMovies : public CGUIWindowBoxeeBrowseTvShows
{
public:
  CGUIWindowBoxeeBrowseMovies();
  virtual ~CGUIWindowBoxeeBrowseMovies();

  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnInitWindow();

protected:

  //void FillStoreList(CFileItemList& store);

  virtual void ConfigureState(const CStdString& param);
  virtual void GetStartMenusStructure(std::list<CFileItemList>& browseMenuLevelList);
};

#endif /*GUIWINDOWBOXEEBROWSEMOVIES_H_*/
