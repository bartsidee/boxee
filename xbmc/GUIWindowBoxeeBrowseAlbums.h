#ifndef GUIWINDOWBOXEEBROWSEALBUMS_H_
#define GUIWINDOWBOXEEBROWSEALBUMS_H_

#include "GUIWindowBoxeeBrowse.h"

#define MUSIC_THUMB_VIEW  53
#define MUSIC_LINE_VIEW   54

class CAlbumsSource : public CBrowseWindowSource
{
public:
  CAlbumsSource(int iWindowID);
  virtual ~CAlbumsSource();
  void BindItems(CFileItemList& items);

};

class CArtistsSource : public CBrowseWindowSource
{
public:
  CArtistsSource(int iWindowID);
  virtual ~CArtistsSource();

};

class CArtistSource : public CBrowseWindowSource
{
public:
  CArtistSource(const CStdString& path ,int iWindowID);
  virtual ~CArtistSource();

};

class CAlbumsWindowState : public CBrowseWindowState
{
public:
  CAlbumsWindowState(CGUIWindowBoxeeBrowse* pWindow);

  virtual void Refresh(bool bResetSelected = false);
  virtual CStdString GetItemSummary();
  //virtual CBrowseWindowState* Clone();

  virtual bool OnBack();

  bool OnArtist(CFileItem& artistItem);

  void SetArtist(const CStdString& strArtist);
  void SetGenre(const CStdString& strGenre);
  void SetCategory(const CStdString &strCategory);

  void SetDefaultView();
  void SetDefaultCategory();

  // The purpose of the m_iSelectedArtist is to remember the selected artist after we
  // return back from showing the albums of a specific artist
  int m_iSelectedArtist;

protected:

  CStdString m_strGenre;
  CStdString m_strArtist;

private:

  bool OnArtists();
  bool OnAlbums();

  int m_iSavedView;
};


class CGUIWindowBoxeeBrowseAlbums : public CGUIWindowBoxeeBrowse
{
public:
  CGUIWindowBoxeeBrowseAlbums();
  virtual ~CGUIWindowBoxeeBrowseAlbums();

  void ConfigureState(const CStdString& param);
  void ShowItems(CFileItemList& list, bool append);

  virtual bool OnMessage(CGUIMessage& message);

protected:

  virtual void GetStartMenusStructure(std::list<CFileItemList>& browseMenuLevelList);

  virtual bool OnClick(int iItem);

};


#endif /*GUIWINDOWBOXEEBROWSEALBUMS_H_*/
