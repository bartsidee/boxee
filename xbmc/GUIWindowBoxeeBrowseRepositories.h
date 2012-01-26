#ifndef GUIWINDOWBOXEE_BROWSE_REPOSITORIES_H_
#define GUIWINDOWBOXEE_BROWSE_REPOSITORIES_H_

#include "GUIWindowBoxeeBrowse.h"

class CRepositoriesWindowState : public CBrowseWindowState
{
public:
  CRepositoriesWindowState(CGUIWindowBoxeeBrowse* pWindow);

};

class CGUIWindowBoxeeBrowseRepositories : public CGUIWindowBoxeeBrowse
{
public:
  CGUIWindowBoxeeBrowseRepositories();
	virtual ~CGUIWindowBoxeeBrowseRepositories();
	
  virtual void OnInitWindow();
  virtual bool OnMessage(CGUIMessage& message);

  void ShowItems(CFileItemList& list, bool append=false);
protected:

  /*
   * Handles click on specific item in the container
   */
  virtual bool OnClick(int iItem);

  virtual void GetStartMenusStructure(std::list<CFileItemList>& browseMenuLevelList);

private:

  bool ProcessMenuClick(CGUIMessage& message);

  bool HandleMenuAll();
  bool HandleMenuAdd();
  bool HandleMenuManage();

  static const char* ControlIdAsString(int controlId);

  bool m_manageButtonOn;
};

#endif /*GUIWINDOWBOXEE_BROWSE_APPS_H_*/
