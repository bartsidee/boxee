#ifndef GUIWINDOWBOXEE_BROWSE_APPS_H_
#define GUIWINDOWBOXEE_BROWSE_APPS_H_

#include "GUIWindowBoxeeBrowseWithPanel.h"

class CAppsWindowState : public CBrowseWindowState
{
public:
  CAppsWindowState(CGUIWindow* pWindow);

  virtual void SetApplicationType(const CStdString& strType);
  virtual void SortItems(CFileItemList &items);

  CStdString CreatePath();

  void Reset();

  virtual bool OnBack();
  bool OnMyApps();
  bool OnAllApps();
  bool OnRepositories();

  void SetState(int state);
  int GetState();

protected:

  virtual void UpdateFilters(const CStdString& strPath){}

  CStdString m_strApplicationType;
  int m_iState;

  int m_iPreviousState;
  CBoxeeSort m_savedSort;

};


class CGUIWindowBoxeeBrowseApps : public CGUIWindowBoxeeBrowseWithPanel
{
public:

  CGUIWindowBoxeeBrowseApps();
  virtual ~CGUIWindowBoxeeBrowseApps();

  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  virtual bool OnAction(const CAction &action);
  virtual bool ProcessPanelMessages(CGUIMessage& message);

  void SetWindowLabel(int controlId, const CStdString windowLabel);

  bool IsInMyAppsState();

  void FromRepositories();

protected:

  virtual bool OnClick(int iItem);

private:

  bool HandleBtnApplicationType();
  bool FillDropdownWithApplicationTypes(CFileItemList& applicationTypes);

  bool m_isRepository;

  bool m_itemWasLaunch;

};

#endif /*GUIWINDOWBOXEE_BROWSE_APPS_H_*/
