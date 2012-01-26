#ifndef GUIWINDOWBOXEE_BROWSE_APPBOX_H_
#define GUIWINDOWBOXEE_BROWSE_APPBOX_H_

#include "GUIWindowBoxeeBrowseWithPanel.h"
#include "GUIWindowBoxeeBrowseApps.h"

class CAppBoxWindowState : public CAppsWindowState
{
public:
  CAppBoxWindowState(CGUIWindow* pWindow);
};

class CGUIWindowBoxeeBrowseAppBox : public CGUIWindowBoxeeBrowseWithPanel
{
public:
  CGUIWindowBoxeeBrowseAppBox();
	virtual ~CGUIWindowBoxeeBrowseAppBox();
	
  virtual void OnInitWindow();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);

protected:

  /*
   * Handles click on specific item in the container
   */
  virtual bool OnClick(int iItem);

  virtual void EnableAllControls();
  virtual void DisableAllControls();

private:
  
  bool ProcessPanelMessages(CGUIMessage& message);

  bool HandleBtnApplicationType();
  bool FillDropdownWithApplicationTypes(CFileItemList& applicationTypes);

  bool HandleMenuRepositories();
  bool HandleMenuSearch();
  bool HandleMenuMyApps();

  static const char* ControlIdAsString(int controlId);

};

#endif /*GUIWINDOWBOXEE_BROWSE_APPBOX_H_*/
