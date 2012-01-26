#ifndef GUIDIALOGBOXEEMAINMENU_H_
#define GUIDIALOGBOXEEMAINMENU_H_

#include "GUILoaderDialog.h"
#include "FileItem.h"

class CGUIDialogBoxeeMainMenu : public CGUILoaderDialog
{
public:
  CGUIDialogBoxeeMainMenu(void);
  virtual ~CGUIDialogBoxeeMainMenu();
  virtual bool OnMessage(CGUIMessage &message);
  bool OnAction(const CAction &action);

protected:

  virtual void OnInitWindow();
  
private:

  void ActivateWindow(unsigned int windowId, const CStdString& path = "", bool closeDialog = true);

  bool HandleShortcutListClick();
  void HandleShortcutManageButtonClick();

  bool m_manageButtonOn;
  bool m_moveShortcut;
};

#endif /*GUIDIALOGBOXEEMAINMENU_H_*/
