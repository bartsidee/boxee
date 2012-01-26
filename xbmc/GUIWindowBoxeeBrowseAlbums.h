#ifndef GUIWINDOWBOXEEBROWSEALBUMS_H_
#define GUIWINDOWBOXEEBROWSEALBUMS_H_

#include "GUIWindowBoxeeBrowseWithPanel.h"

class CAlbumsWindowState : public CBrowseWindowState
{
public:
  CAlbumsWindowState(CGUIWindow* pWindow);

  virtual void SortItems(CFileItemList &items);

  CStdString CreatePath();
  CStdString AddGuiStateParameters(const CStdString& strPath);

  void Reset();

  virtual bool OnBack();
  bool OnAlbums();
  bool OnArtist(const CStdString& strArtist);
  bool OnArtists();

  void SetArtist(const CStdString& strArtist);
  void SetGenre(const CStdString& strGenre);

  bool m_bInTracks;
  int m_iSelectedArtist;

protected:

  virtual void UpdateFilters(const CStdString& strPath) {}

  CStdString m_strGenre;
  CStdString m_strArtist;
  int m_iState;
};


class CGUIWindowBoxeeBrowseAlbums : public CGUIWindowBoxeeBrowseWithPanel
{
public:
  CGUIWindowBoxeeBrowseAlbums();
  virtual ~CGUIWindowBoxeeBrowseAlbums();

  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);
  virtual bool ProcessPanelMessages(CGUIMessage& message);
  virtual void Render();
  void SetAudioCounters(bool bOn);

protected:
  virtual bool OnClick(int iItem);

  void FillGenresList(CFileItemList& genres);
  std::vector<CStdString> m_vecGenres;
  int                     m_renderCount;

};


#endif /*GUIWINDOWBOXEEBROWSEALBUMS_H_*/
