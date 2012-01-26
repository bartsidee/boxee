#ifndef GUIWINDOWBOXEEBROWSETRACKS_H_
#define GUIWINDOWBOXEEBROWSETRACKS_H_

#include "GUIWindowBoxeeBrowse.h"

class CTracksSource : public CBrowseWindowSource
{
public:
  CTracksSource(int iWindowID);
  virtual ~CTracksSource();

  void AddStateParameters(std::map <CStdString, CStdString>& mapOptions);

  CStdString m_strAlbumId;
};

class CTracksWindowState : public CBrowseWindowState
{
public:
  CTracksWindowState(CGUIWindowBoxeeBrowse* pWindow);
  void InitState();
  virtual void SortItems(CFileItemList &items);

  void SetAlbumId(const CStdString& strAlbumId);

  void SetAlbumData(const CStdString& strAlbumId, const CStdString& strAlbumName, const CStdString& strAlbumThumb);

protected:

  virtual void UpdateFilters(const CStdString& strPath) {}

  CStdString m_strAlbumId;
  CStdString m_strAlbumName;
  CStdString m_strAlbumThumb;

  bool m_bHasShortcut;
};

class CGUIWindowBoxeeBrowseTracks : public CGUIWindowBoxeeBrowse
{
public:
  CGUIWindowBoxeeBrowseTracks();
	virtual ~CGUIWindowBoxeeBrowseTracks();
	
  virtual void OnInitWindow();
  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnBack();
  virtual bool OnBind(CGUIMessage& message);

  bool SetThumbFile(const CStdString& filename);
  bool ReloadThumb();
  bool UnloadThumb();

	bool OnShare();
  bool OnRescan();
  bool OnManualResolve();
  bool OnAddTrack();
  bool OnThumbChange();

protected:

  virtual void GetStartMenusStructure(std::list<CFileItemList>& browseMenuLevelList);

  virtual bool OnClick(int iItem);

private:
	CFileItemPtr m_albumItem;
	bool m_bResetPlaylist;
  //bool m_bIsRescanningAlbum;

};

#endif /*GUIWINDOWBOXEEBROWSETRACKS_H_*/
