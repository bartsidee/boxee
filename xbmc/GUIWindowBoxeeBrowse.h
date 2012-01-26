#ifndef GUIWINDOWBOXEEBROWSE_H_
#define GUIWINDOWBOXEEBROWSE_H_

#include "GUIWindow.h"
#include "linux/PlatformDefs.h"
#include "StdString.h"
#include "GUIViewControl.h"
#include "GUIBoxeeViewState.h"
#include "FileSystem/DirectoryHistory.h"
#include "BrowseWindowState.h"
#include "BoxeeBrowseMenuManager.h"

// View sort methods
#define VIEW_SORT_METHOD_NONE       "no-sort"
#define VIEW_SORT_METHOD_ATOZ       "title"
#define VIEW_SORT_METHOD_ZTOA       "title desc"
#define VIEW_SORT_METHOD_POPULARITY "popular"
#define VIEW_SORT_METHOD_RELEASE    "release"
#define VIEW_SORT_METHOD_RELEASE_REVERSE    "release asc"
#define VIEW_SORT_METHOD_DATE       "Date"
#define VIEW_SORT_METHOD_LAST_USED  "last-used"
#define VIEW_SORT_METHOD_USAGE      "usage"
#define VIEW_SORT_METHOD_NEWEST_FIRST "newestfirst"
#define VIEW_SORT_METHOD_NEWEST_LAST  "newestlast"

#define BROWSE_SETTINGS  8024

#define BROWSE_MENU_LIST_LEVEL_START_ID  47
#define BROWSE_MENU_LIST_LEVEL_END_ID    49

class CSelectBrowseMenuItemInfo
{
public:
  CStdString m_menuId;
  int m_selectedIndex;
  CStdString m_selectedLabel;

  void Reset();
  void Set(const CStdString& menuId, int selectedIndex, const CStdString& selectedLabel);
};

class CGUIWindowBoxeeBrowse : public CGUIWindow, public IBrowseWindowView
{
public:
  CGUIWindowBoxeeBrowse();
  CGUIWindowBoxeeBrowse(DWORD dwID, const CStdString &xmlFile);
  virtual ~CGUIWindowBoxeeBrowse();
  void InitializeWindowState();

  void SetWindowState(CBrowseWindowState* pWindowState);
  virtual bool HandleEmptyState();
  virtual void HandleLoadingTimeout(const CStdString& customMessage);

  // Implementation of IBrowseWindowView
  virtual void ShowItems(CFileItemList& list, bool append=false);

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);

  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

  virtual void OnWindowLoaded();
  virtual void OnWindowUnload();

  void GenerateAlphabetScrollbar(CFileItemList& list);

  // item is copied on purpose (it may be deleted as a result of window deinit)
  //virtual bool OnPlayMedia(CFileItem item);

  // ////////////////////////////////////
  // Functions called from XBMC infrastructure - GUIInfoManager
  virtual CFileItemPtr GetCurrentListItem(int offset = 0);
  virtual bool IsMediaWindow() const { return true; };
  int GetViewContainerID() const { return m_viewControl.GetCurrentControl(); };
  // The implementation of this function takes care of
  // redirecting the focus to the currently active view
  CGUIControl* GetFirstFocusableControl(int id);
  // ////////////////////////////////////

  // ALEX: Only one of those is needed, remove the redundant one
  void ClearView();
  void ClearFileItems();

  void SetWindowTitle(const CStdString& strTitle);

  void UpdateUIFlags();

  int GetSelectedItem() { return m_viewControl.GetSelectedItem(); };
  void SetSelectedItem(int item){ m_viewControl.SetSelectedItem(item); };

  CStdString m_strItemDescription;
  virtual CStdString GetItemDescription();

protected:

  CBrowseWindowState* m_windowState;

  void Refresh(bool bResetSelected = false);


  virtual bool OnClick(int iItem);
  bool GetClickedItem(int iItem, CFileItem& item);

  virtual bool OnBind(CGUIMessage& message);
  virtual void OnBack();

  virtual void OnLoadFailed(const CFileItem* pItem);
  virtual void OnItemLoaded(const CFileItem* pItem);

  virtual void ConfigureState(const CStdString& param);

  virtual bool UpdateWindowBrowseMenu(const CStdString& menuId = "");
  virtual void GetStartMenusStructure(std::list<CFileItemList>& browseMenuLevelList);

  virtual bool HandleClickOnBrowseMenu(const CFileItemPtr selectedFileItem);
  virtual void ResetBrowseMenuLevel(int menuListId = -1);

  //virtual void OnSearch();

  // Items that are sent to the view
  CFileItemList m_vecViewItems;

  // Remember the last selected item to restore window state
  int m_iLastControl;

  // View controller
  CGUIViewControl m_viewControl;

  // Index used to speed up item updating during background loading
  std::map< CStdString, CFileItemPtr > m_viewItemsIndex;

  int m_initSelectPosInBrowseMenu;

  bool m_hasBrowseMenu;

  void ApplyBrowseMenuFromStack();

private:

  bool InitWindowBrowseMenu();
  bool InitializeStartMenuStructure();
  bool GoForwardInWindowBrowseMenu(const CStdString& menuId);
  bool GoBackwardInWindowBrowseMenu();
  bool AddButtonItemsToBrowseMenu(const CFileItemList& browseMenu,int browseMenuListControlId,bool fromStack = false);

  void UpdateBrowseMenuData(const CFileItemList& browseMenu, int browseMenuListControlId);
  void UpdateTopBrowseMenuSelection();

  void ClearBrowseMenuStack();
  void AddBrowseMenuToStack(const CStdString& menuId);
  void PopBrowseMenuFromStack();

  bool IsEqualPaths(const CStdString& left, const CStdString& right);

  void UpdateIndexMaps();

  virtual void InitializeViewController();
  void PrepareViewItems(CFileItemList &items);//, CFileItemList& listAppendedTo = NULL);

  // Specifies when the next or previous page should be retrieved
  double m_iPageThreshold;

  bool m_bPageRequested;
  bool m_bAllItemsLoaded;

  int m_iMiscItems;

  CStdString m_currentMenuLevelStr;

  static std::stack<CStdString> m_browseMenuStack;
  static CSelectBrowseMenuItemInfo m_selectBrowseMenuItemInfo;

  bool m_shouldFocusToPanel;
};

#endif /*GUIWINDOWBOXEEBROWSE_H_*/
