#ifndef GUIDIALOGBOXEEBROWSEMENU_H_
#define GUIDIALOGBOXEEBROWSEMENU_H_

#include "GUILoaderDialog.h"
#include "FileItem.h"
#include "lib/libBoxee/bxgenresmanager.h"
#include "lib/libBoxee/bxsourcesmanager.h"
#include "WatchDog.h"
#include "utils/SingleLock.h"
#include "lib/libBoxee/bxtrailersmanager.h"
#include "lib/libBoxee/bxappboxmanager.h"

class CGUIButtonControl;

#define BROWSE_MENU_BUTTON_SEARCH                 11011

class CGUIDialogBoxeeBrowseMenu : public CGUIDialog
{
public:
  CGUIDialogBoxeeBrowseMenu(void);
  virtual ~CGUIDialogBoxeeBrowseMenu();

  virtual bool OnMessage(CGUIMessage &message);
  bool OnAction(const CAction &action);

  virtual void Render();

  virtual void Close(bool forceClose = false);

  static void OpenSearchWithAction(const CAction &action);

  void ReloadControl (CStdString strControlId);

  CStdString GetSearchTerm();

protected:

  virtual void OnInitWindow();

  void UpdateMenu(bool bRebuild);

  void OnClick(int iClickedButton);

private:

  bool ReloadDynamicRows();
  bool AdjustButtonsAndSearchBar();

  void ActivateWindow(unsigned int windowId, const CStdString& path = "", bool closeDialog = true);

  void ResetCurrentParameters();

  bool CanOpenBrowseMenu();

  void ResetAction();
  void SetAction(const CAction &action);

  CAction m_startAction;

  static int s_buttonId;
  bool m_bInitialized;
  unsigned int m_downloadCounter;

  // The following members are used to track the current state of the menu

  // Index of the currently selected row
  int m_iCurrentButtonId;
  int m_iCurrentWindowId;

  CCriticalSection m_initCriticalSection;

  bool m_openInSearch;
};

#endif
