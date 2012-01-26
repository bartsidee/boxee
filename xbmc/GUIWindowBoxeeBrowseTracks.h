#ifndef GUIWINDOWBOXEEBROWSETRACKS_H_
#define GUIWINDOWBOXEEBROWSETRACKS_H_

#include "GUIWindowBoxeeBrowse.h"

class CTracksWindowState : public CBrowseWindowState
{
public:
  CTracksWindowState(CGUIWindow* pWindow);
  void InitState(const CStdString& strPath);
  virtual void SortItems(CFileItemList &items);
  CStdString CreatePath();

  bool HasShortcut();
  bool OnShortcut();

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

  bool ProcessPanelMessages(CGUIMessage& message);

	bool OnShare();

protected:

	virtual bool OnClick(int iItem);

private:

	CFileItemPtr m_albumItem;
	bool m_bResetPlaylist;

};

#endif /*GUIWINDOWBOXEEBROWSETRACKS_H_*/
