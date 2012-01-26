#ifndef GUIWINDOWBOXEEBROWSEPHOTOS_H_
#define GUIWINDOWBOXEEBROWSEPHOTOS_H_

#include "GUIWindowBoxeeBrowse.h"
#include "BrowseWindowFilter.h"

class CPhotosSource : public CBrowseWindowSource
{
public:
  CPhotosSource(int iWindowID);
  virtual ~CPhotosSource();
  void BindItems(CFileItemList& items);
  void SetStartingPath(const CStdString& strStartingPath);
private:
  CBrowseWindowPictureFilter m_filter;
};

class CPhotosWindowState : public CBrowseWindowStateWithHistory
{
public:
  CPhotosWindowState(CGUIWindowBoxeeBrowse* pWindow);

  void SetDefaultView();

  void OnPathChanged(CStdString strPath, bool bResetSelected=true);
  void FromHistory(CBrowseStateHistoryItem* historyItem);
  bool OnBack();
  void InitState();
  
  virtual void Refresh(bool bResetSelected = false);
  virtual CStdString GetItemSummary();
  
  void SetCurrentPath(const CStdString& path);
  CStdString GetCurrentPath();

  CFileItemList GetItems();

};

class CGUIWindowBoxeeBrowsePhotos : public CGUIWindowBoxeeBrowse
{
public:
  CGUIWindowBoxeeBrowsePhotos();
  virtual ~CGUIWindowBoxeeBrowsePhotos();

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);

  virtual void ConfigureState(const CStdString& param);
  virtual bool OnClick(int iItem);

  CStdString GetItemDescription();

protected:

  virtual void GetStartMenusStructure(std::list<CFileItemList>& browseMenuLevelList);
};

#endif
