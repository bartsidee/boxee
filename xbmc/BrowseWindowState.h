#ifndef BROWSEWINDOWSTATE_H_
#define BROWSEWINDOWSTATE_H_

#include "GUIWindow.h"
#include "linux/PlatformDefs.h"
#include "StdString.h"
#include "GUIViewControl.h"
#include "GUIBoxeeViewState.h"
#include "FileSystem/DirectoryHistory.h"
#include "BrowseWindowConfiguration.h"

class CBrowseWindowStateNode
{
  std::vector<CStdString> m_vecActions;
};

class CBrowseWindowStateMachine
{
  std::vector<CBrowseWindowStateNode> m_vecStates;
  std::stack<CBrowseWindowStateNode> m_stateStack;
};

class CBrowseWindowState
{
public:

  CBrowseWindowState(CGUIWindow* pWindow);
  virtual ~CBrowseWindowState();

  virtual CStdString CreatePath();

  virtual void Reset();

  virtual CStdString GetCurrentPath();
  virtual void SetPath(const CStdString &strInitialPath);
  virtual void SetSelectedItem(int iSelectedItem);
  virtual int GetSelectedItem();
  virtual void SetBackgroundImage(const CStdString& strImage);
  virtual CStdString GetBackgroundImage();
  virtual void SetLabel(const CStdString& strLabel);
  virtual CStdString GetLabel();
  virtual void InitState(const CStdString& strPath);
  virtual void UpdateWindowProperties();

  // Function specific to the window state of CGUIWindowBoxeeBrowse
  // It uses type (such as "vide", "music", "pictures" etc... to initialize state configuration
  void UpdateConfigurationByType(const CStdString& strType);
  CStdString GetWindowStateType();

  // This callback is invoked when the window path is updated
  virtual void OnPathChanged(CStdString strPath, bool bResetSelected = true);
  virtual void OnLoading();
  virtual void OnLoaded();
  virtual void OnLoadFailed();

  // Performs all necessary transformations (Sort, Filter etc...) on the provided set of model items
  virtual void ProcessItems(const CFileItemList& vecModelItems, CFileItemList& vecViewItems);

  virtual bool OnBack() { return false; }

  // Sort functionality (TODO: Move to separate module)
  virtual bool OnSort();
  virtual void SetSort(const CBoxeeSort& sort);
  virtual void SortItems(CFileItemList &items);

  // Search functionality
  virtual void SetSearchType(const CStdString& strSearchType);
  virtual CStdString GetSearchString();
  virtual bool OnSearchStart();
  virtual bool OnSearchEnd();
  virtual bool InSearchMode();
  virtual void RestoreWindowState();
  virtual void SaveWindowState();


  CBrowseWindowState* ToHistory();
  void FromHistory(CBrowseWindowState*);

protected:

  void SetPathToDisplay();
  int m_displayPathMaxLength;

  CGUIWindow* m_pWindow;
  CStdString m_strInitialPath;

  // Sort data members
  CBoxeeSort m_sort;
  std::vector<CBoxeeSort> m_vecSortMethods;

  // Search data members
  int m_iSearchDepth;
  CStdString m_strSearchString;
  CStdString m_strSearchType;

  virtual void UpdateFilters(const CStdString& strPath);

  CBrowseWindowConfiguration m_configuration;
};

#endif /*BROWSEWINDOWSTATE_H_*/
