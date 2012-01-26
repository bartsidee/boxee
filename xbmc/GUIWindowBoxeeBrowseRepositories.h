#ifndef GUIWINDOWBOXEE_BROWSE_REPOSITORIES_H_
#define GUIWINDOWBOXEE_BROWSE_REPOSITORIES_H_

#include "GUIWindowBoxeeBrowse.h"

class CGUIWindowBoxeeBrowseRepositories : public CGUIWindowBoxeeBrowse
{
public:
  CGUIWindowBoxeeBrowseRepositories();
	virtual ~CGUIWindowBoxeeBrowseRepositories();
	
  virtual void OnInitWindow();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void OnBack();

protected:

  /*
   * Handles click on specific item in the container
   */
  virtual bool OnClick(int iItem);

private:

  /**
   * Creates the updated path that will be sent to BoxeeServerDirectory
   * according to the current state of the buttons
   */
  virtual CStdString CreatePath();

  bool ProcessMenuClick(CGUIMessage& message);

  bool HandleMenuAll();
  bool HandleMenuAdd();
  bool HandleMenuManage();

  static const char* ControlIdAsString(int controlId);

  CStdString m_strPath;
  bool m_manageButtonOn;
};

#endif /*GUIWINDOWBOXEE_BROWSE_APPS_H_*/
