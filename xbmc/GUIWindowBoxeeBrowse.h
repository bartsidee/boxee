#ifndef GUIWINDOWBOXEEBROWSE_H_
#define GUIWINDOWBOXEEBROWSE_H_

#include "GUIWindow.h"
#include "linux/PlatformDefs.h"
#include "StdString.h"
#include "GUIViewControl.h"
#include "GUIBoxeeViewState.h"
#include "FileSystem/DirectoryHistory.h"
#include "BrowseWindowConfiguration.h"
#include "BrowseWindowState.h"

class CGUIWindowBoxeeBrowse : public CGUIWindow
{
public:
  CGUIWindowBoxeeBrowse();
  CGUIWindowBoxeeBrowse(DWORD dwID, const CStdString &xmlFile);
  virtual ~CGUIWindowBoxeeBrowse();
  void InitializeWindowState();

  void SetWindowState(CBrowseWindowState* pWindowState);

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);

  virtual void OnInitWindow();
  virtual void OnWindowLoaded();
  virtual void OnWindowUnload();

  // item is copied on purpose (it may be deleted as a result of window deinit)
  virtual bool OnPlayMedia(CFileItem item);

  static void Show(const CStdString &strPath, const CStdString &strType, const CStdString &strLabel = "", const CStdString &strBackgroundImage = "",bool bResetHistory = true, const CStdString &strAppId = "");
  void ResetWindowState(const CStdString &strPath, const CStdString &strType, const CStdString &strLabel, const CStdString &strBackgroundImage, bool bResetHistory);

  CGUIBoxeeViewState* GetViewState() { return m_boxeeViewState.get(); }
  CStdString GetWindowStateType();
  const CFileItemList& GetItems();
  CStdString GetLocationPath();

  // Functions for preview funcitonality
  void Prefetch(const CStdString& strPath = "");
  int GetCurrentView();

  // Functions called from XBMC infrastructure
  virtual CFileItemPtr GetCurrentListItem(int offset = 0);
  virtual bool IsMediaWindow() const { return true; };
  int GetViewContainerID() const { return m_viewControl.GetCurrentControl(); };
  // The implementation of this function takes care of
  // redirecting the focus to the currently active view
  CGUIControl* GetFirstFocusableControl(int id);

  void ClearView();

  void SetAppData(const CStdString& strAppId, const CStdString& strAppName);
  
protected:

  CBrowseWindowState* m_windowState;

  void Refresh(bool bResetSelected = false);
  void UpdateFileList(bool bPreserveSelected = true);

  virtual bool OnClick(int iItem);
  bool GetClickedItem(int iItem, CFileItem& item);

  virtual bool OnBind(CGUIMessage& message);
  virtual void OnBack();
  virtual void OnLoaded();
  virtual void OnLoadFailed(const CFileItem* pItem);
  virtual void OnItemLoaded(const CFileItem* pItem);

  void ResetHistory();

  // Browse window items (the model)
  CFileItemList m_vecModelItems;
  // Items that are sent to the view
  CFileItemList m_vecViewItems;

  // Remember the last selected item to restore window state
  int m_iLastControl;

private:

  void UpdateHistory();
  void UpdatePath(CStdString strPath, bool bResetSelected);
  void UpdatePath(const CFileItem* pItem, bool bResetSelected);
  void UpdateLabel(const CFileItem* pItem);
  void UpdateIndexMaps();

  void ClearFileItems();

  void SetWindowTitle(const CStdString& strTitle);

  virtual void InitializeViewController();
  void SetViewItems(CFileItemList &items);

  bool HasAppData();
  void ResetAppData();

  void SetItemWithAppData(CFileItem& item);

  // Index used to speed up item updating during background loading
  std::map< CStdString, CFileItemPtr > m_itemsIndex;
  std::map< CStdString, CFileItemPtr > m_filteredItemsIndex;

  // View controller
  CGUIViewControl m_viewControl;
  std::auto_ptr<CGUIBoxeeViewState> m_boxeeViewState;

  std::vector<CBrowseWindowState*> m_history;

  // Holds the path of the item that is currently opened in the media action dialog
  // used to update the item when it is loaded by the background loader
  CStdString m_strInActionItemId;

  // if m_strAppId is set - then the browse screen was activated from an app. 
  CStdString m_strAppId;
  CStdString m_strAppName;
};

#endif /*GUIWINDOWBOXEEBROWSE_H_*/
