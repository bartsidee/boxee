#ifndef GUIWINDOWBOXEE_BROWSE_SHORTCUTS_H_
#define GUIWINDOWBOXEE_BROWSE_SHORTCUTS_H_

#include "GUIWindowBoxeeBrowse.h"

class CGUIWindowBoxeeBrowseShortcuts : public CGUIWindowBoxeeBrowse
{
public:
  CGUIWindowBoxeeBrowseShortcuts();
	virtual ~CGUIWindowBoxeeBrowseShortcuts();
	
  virtual void OnInitWindow();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);

protected:

  /*
   * Handles click on specific item in the container
   */
  virtual bool OnClick(int iItem);

  virtual void SortItems(CFileItemList &items);

private:

  /**
   * Creates the updated path that will be sent to BoxeeServerDirectory
   * according to the current state of the buttons
   */
  virtual CStdString CreatePath();

  bool ProcessMenuClick(CGUIMessage& message);

  bool HandleMenuManage();

  static const char* ControlIdAsString(int controlId);

  CStdString m_strPath;
  bool m_manageButtonOn;
  //bool m_removeShortcut;
  bool m_moveShortcut;
};

#endif /*GUIWINDOWBOXEE_BROWSE_SHORTCUTS_H_*/
