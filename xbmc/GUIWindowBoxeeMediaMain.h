#pragma once
#include "GUIWindow.h"
#include "FileItem.h"
#include "PictureThumbLoader.h"

#define GUI_MSG_REFRESH_APPS (GUI_MSG_USER + 150)

class CGUIWindowBoxeeMediaMain : public CGUIWindow, public IBackgroundLoaderObserver
{
public:
  CGUIWindowBoxeeMediaMain(DWORD id, const char *xmlFile);
  virtual ~CGUIWindowBoxeeMediaMain(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);
  virtual bool IsMediaWindow() const { return true; }
  virtual bool HasListItems() const { return true; }

  virtual std::string GetType() = 0;
  
  virtual void OnItemLoaded(CFileItem* pItem) ;

protected:
  virtual void OnInitWindow();
  virtual void LoadMediaItems(CFileItemList &mediaItems) = 0;
  virtual void LoadAppItems(CFileItemList &appItems) = 0;
  virtual void OnMediaItemClick(CFileItem *pItem) = 0;
  virtual void OnMediaItemPlay(CFileItem *pItem) ;
  virtual void OnMediaItemPopup(CFileItem *pItem) = 0;
  virtual void OnAppsClick(CFileItem *pItem) = 0;
  virtual void OnAppsPopup(CFileItem *pItem) ;
  virtual void OnMoreApps() = 0;
  virtual void OnMoreMedia() = 0;
  virtual const VECSOURCES &GetShares() = 0;

  virtual CBackgroundInfoLoader *GetLoader();

  bool OnClick(int iControl, int iAction);
  void LoadMediaItems();
  void Cleanup();
  void UpdateItemInList(const CFileItem *pItem, CFileItemList &list);

  CBackgroundInfoLoader *m_Loader; 

  CFileItemList m_listMediaItems;
  CFileItemList m_listMediaItemsSmall;
  CFileItemList m_listApps;
  CFileItemList m_listAppsSmall;
};
