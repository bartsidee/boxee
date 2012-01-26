#ifndef BROWSEWINDOWSTATE_H_
#define BROWSEWINDOWSTATE_H_

#include "linux/PlatformDefs.h"
#include "BrowseWindowFilter.h"
#include "FileSystem/DirectoryCache.h"

#define DEFAULT_PAGE_SIZE  48
#define DISABLE_PAGING   -1
#define CURRENT_PAGE_SIZE DISABLE_PAGING

class CGUIWindowBoxeeBrowse;
class CBrowseWindowState;

typedef enum
{
  WAITING_FOR_ITEMS = 0,
  GOT_ITEMS,
  NO_MORE_ITEMS
} SourceItemState;


class CBrowseWindowSource
{
public:
  CBrowseWindowSource(const CStdString& strSourceId, const CStdString& strPath, int iWindow);
  virtual ~CBrowseWindowSource();

  virtual void BindItems(CFileItemList& items);

  virtual void UpdateItem(const CFileItem* pItem);
  void UpdateIndexMap();

  virtual SourceItemState PopItem(CFileItemPtr& item);
  virtual SourceItemState PeekItem(CFileItemPtr& item);

  bool IsReady() { return m_bReady; }
  bool IsActive() { return m_bActive; }
  void Activate(bool bActive) { m_bActive = bActive; }
  void ClearCachedItems () { g_directoryCache.ClearDirectoriesThatIncludeUrl(m_strBasePath); }

  CStdString GetSourceId()    { return m_strSourceId;    }
  CStdString GetCurrentPath() { return m_strCurrentPath; }
  CStdString GetBasePath()    { return m_strBasePath;    }
  CBoxeeSort GetSortMethod()  { return m_sourceSort;  }

  void SetBasePath(const CStdString& value) { m_strBasePath = value; }
  void SetSourceId(const CStdString& value) { m_strSourceId = value; }

  bool IsPathRelevant(const CStdString& strPath);

  // Causes the sources to build a path and call ItemLoader to get items
  virtual void RequestItems(bool bUseCurPath = false);
  bool GetCredentials(CFileItemList& items);

  virtual void ClearItems();

  virtual void Reset();

  void SetPageSize(int pageSize) { m_iPageSize = pageSize; }
  int GetPageSize () { return m_iPageSize; }
  int GetItemCount() { return m_vecFileItems.Size(); }
  int GetRequestId() { return m_iRequestId; }

  // Set and Clear filter functions allow different sources to implement different filtering strategies
  virtual void SetFilter(const CStdString& strName, const CStdString strValue);
  virtual void ClearFilter(const CStdString& strName);

  virtual void SetSortMethod(const CBoxeeSort& bxSort);

  int GetTotalItemsCount();

  CFileItemList GetItemList(){ return m_vecFileItems; }

protected:
  virtual void AddStateParameters(std::map<CStdString, CStdString>& mapOptions);

  std::map<CStdString, CStdString> m_mapFilters;
  CBoxeeSort  m_sourceSort;

private:
  CStdString    m_strSourceId;
  CStdString    m_strBasePath;
  CStdString    m_strCurrentPath;

  std::map< CStdString, CFileItemPtr > m_mapFileItemsIndex;

  // Model file items
  CFileItemList m_vecFileItems;

  int           m_iMarker;
  int           m_iWindowID;

  bool          m_bActive;
  bool          m_bReady;
  bool          m_bDepleted;

  int           m_iStart;
  int           m_iPageSize;
  int           m_iTotalItemsCount;
  int           m_iRequestId;
};

typedef std::map<CStdString , CBrowseWindowSource* > SourcesMap;
typedef std::map< CStdString, CFileItemPtr > FileItemIndex;

// Interface for the browse window view (of the MVC model)
class IBrowseWindowView
{
public:
  virtual ~IBrowseWindowView() {}
  virtual void ShowItems(CFileItemList& items, bool append=false) = 0;
};

class CBrowseWindowSourceController
{
public:
  CBrowseWindowSourceController( IBrowseWindowView* pView , int pageSize = DISABLE_PAGING );
  virtual ~CBrowseWindowSourceController();

  virtual void Refresh();

  void UpdateIndexMaps();

  // Removes all previous sources and sets the one provided as parameter
  bool SetNewSource(CBrowseWindowSource* source);
  void ActivateSource(const CStdString& strSourceId, bool bActivate, bool bReset);
  void ActivateAllSources(bool bActivate, bool bReset);
  bool AddSource (CBrowseWindowSource* source);
  bool RemoveSource(const CStdString& strSourceId);

  bool IsPathRelevant(CFileItemList& items);
  bool IsPathRelevant(const CStdString& strPath, int requestId);
  // Called from the view (window) when items arrive via BIND message
  void BindItems(CFileItemList& items);

  // Default implementation show all ready sources
  virtual void ShowReadySources();

  // Update item in the model by the one loaded by the background loader
  void UpdateItem(const CFileItem* pItem);

  // Merge items from two or more sources before passing them to the view
  SourceItemState MergeSources();

  void ClearItems();
  void ClearCachedSources();

  int GetPageSize() { return m_iPageSize; }
  void SetPageSize(int pageSize){ m_iPageSize = pageSize; }

  int SourceCount () { return m_vecSourcesItems.Size(); }

  int RemoveAllSources();
  SourcesMap GetSources() { return m_sources; }
  void SetSources(SourcesMap mapSources) { m_sources = mapSources; }

  CBrowseWindowSource* GetSourceById(const CStdString& strSourceId);

  SourceItemState GetNextPage();

  void SetFilter(const CStdString& strName, const CStdString strValue);
  void ClearFilter(const CStdString& strName);

  virtual void SetSortMethod(const CBoxeeSort& bxSort);
  CBoxeeSort GetSortMethod() { return m_currentSort; }


  int GetTotalItemsCount() { return m_iTotalItemsCount; }

protected:
  IBrowseWindowView* m_pView;

  SourcesMap m_sources;

  CFileItemList m_vecSourcesItems;

  CFileItemList m_vecCurrentPageItems;
  CFileItemList m_vecCurrentViewItems;
  FileItemIndex m_mapViewItemsIndexByBoxeeId;

  CBoxeeSort m_currentSort;
  FileItemIndex m_mapViewItemsIndexByItemId;

  int m_iPageSize;
  bool m_bPendingBind;

  int m_iTotalItemsCount;
};

class CBrowseWindowState
{
public:

  CBrowseWindowState(CGUIWindowBoxeeBrowse* pWindow);
  virtual ~CBrowseWindowState();

  // Initialize state
  virtual void InitState();
  virtual void SaveState();
  virtual bool LoadState();

  bool IsPathRelevant(const CStdString& strPath, int requestId) { return m_sourceController.IsPathRelevant(strPath,requestId); }

  // Refresh reloads all sources and sends the results to the view
  virtual void Refresh(bool bResetSelected = false);

  // Processes item that was clicked in the view
  virtual bool OnClick(CFileItem& item);

  // Process back call
  virtual bool OnBack();

  // This callback is invoked when the window path is updated
  // Only single path can be updated using this function
  virtual void OnPathChanged(CStdString strPath, bool bResetSelected = true);

  // Callback for background loader, passes the updated item to the controller
  virtual void OnItemLoaded(const CFileItem* pItem);

  // ///////////////////////////////
  // Controller functions
  virtual void OnBind(CFileItemList& items);

  SourcesMap GetSources() { return m_sourceController.GetSources(); }
  void SetSources(SourcesMap mapSources) { m_sourceController.SetSources(mapSources); }

  bool IsPathRelevant(const CStdString& strPath);
  CBrowseWindowSourceController& GetController() { return m_sourceController; }

  void ClearItems() { m_sourceController.ClearItems(); }
  void ClearCachedSources() { m_sourceController.ClearCachedSources(); }
  /////////////////////////////////////////////////////////////////////////////

  // //////////////////////////////
  // Defaults
  virtual void SetDefaultView();
  virtual void SetDefaultCategory();
  virtual void SetDefaultSort();
  void ResetDefaults();
  ////////////////////////////////////////////////////////////////////////////

  // ///////////////////////////////
  // View functions
  void SetCurrentView(int iCurrentView);
  int  GetCurrentView();
  void SetPageSize(int iPageSize);
  virtual void SetSelectedItem(int iSelectedItem);
  virtual int GetSelectedItem();
  ///////////////////////////////////////////////////////////////////////////

  // Only used in SimpleApp and TVEpisodes, consider removing from base
  virtual void SetLabel(const CStdString& strLabel);
  virtual CStdString GetLabel();

  // Sort functionality (TODO: Move to separate module)
  virtual bool OnSort();
  virtual void SetSort(const CBoxeeSort& sort);
  CBoxeeSort GetSort() { return m_sort; }
  //virtual void SortItems(CFileItemList &items);
  CBoxeeSort* GetSortById(const CStdString& id);

  // Search functionality ///////////////////////////
  /*
  virtual void SetSearchType(const CStdString& strSearchType);
  virtual CStdString GetSearchString();
  virtual bool OnSearchStart();
  virtual bool OnSearchEnd();
  virtual bool InSearchMode();
  */
  // //////////////////////////////

  virtual CStdString GetItemSummary() { return ""; };

  // Filters ////////////////////////////
  void SetFilter(const CStdString& strName, const CStdString strValue);
  void ClearFilter(const CStdString& strName);

  ///////////////////////////////////////////////////////////////////////////
  // Paging

  SourceItemState GetNextPage();

  ///////////////////////////////////
  // Categories

  virtual void SetCategory(const CStdString& strCategory);
  virtual CStdString GetCategory () { return m_strSelectedCategory; }

  int GetTotalItemCount() { return m_sourceController.GetTotalItemsCount(); }

protected:
  
  // Methods for saving and loading the state from database

	// Window associated with this state
  CGUIWindowBoxeeBrowse* m_pWindow;

  // Controller holds the sources of the window
  CBrowseWindowSourceController m_sourceController;

	CStdString m_strTitle;
	CStdString m_strLabel;

	// Currently active window view
	int m_iCurrentView;

	// Currently selected item in the view
	int m_iSelectedItem;

  void SetPathToDisplay(const CStdString& strPath);

  // Sort data members
  CBoxeeSort m_sort;
  CBoxeeSort* m_storedSort;
  std::vector<CBoxeeSort> m_vecSortMethods; //needs to be saved to the database

  // Search data members
  int m_iSearchDepth;
  CStdString m_strSearchString;
  CStdString m_strSearchType;

	CBrowseWindowAllowFilter* m_browseWindowAllowFilter;

private:
  CStdString m_strSelectedCategory;
  bool m_bIsInitialized;

};

////////////////////////////////////
// History related

class CBrowseStateHistorySource
{
public:
  CStdString m_strBasePath;
  CStdString m_strSourceId;
};

class CBrowseStateHistoryItem
{
public:
  std::vector<CBrowseStateHistorySource> m_vecSources;
  int m_iSelectedItem;
  int m_level;
};

class CBrowseWindowStateHistory
{
public:
    static void PushState(int id, CBrowseStateHistoryItem* state);
    static CBrowseStateHistoryItem* PopState(int id);
    static bool IsEmpty(int id);
    static int Size(int id);
    static void ResetHistory(int id);

private:
    static std::map<int, std::stack<CBrowseStateHistoryItem*> > m_history;
    static CCriticalSection m_historyLock;
};

class CBrowseWindowStateWithHistory : public CBrowseWindowState
{
public:
  CBrowseWindowStateWithHistory(CGUIWindowBoxeeBrowse* pWindow);
  virtual ~CBrowseWindowStateWithHistory();
  void UpdateHistory();
  void ResetHistory();

  virtual CBrowseStateHistoryItem* ToHistory();
  virtual void FromHistory(CBrowseStateHistoryItem* historyItem);

  virtual bool OnBack();
  virtual bool OnClick(CFileItem& item);

  virtual void OnPathChanged(CStdString strPath, bool bResetSelected = true);

};

#endif /*BROWSEWINDOWSTATE_H_*/
