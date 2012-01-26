#include "GUIWindowFirstTimeUseBackground.h"
#include "Key.h"
#include "log.h"

CGUIWindowFirstTimeUseBackground::CGUIWindowFirstTimeUseBackground(void) : CGUIWindow(WINDOW_FTU_BACKGROUND, "ftu_window_background.xml")
{

}

CGUIWindowFirstTimeUseBackground::~CGUIWindowFirstTimeUseBackground(void)
{

}

bool CGUIWindowFirstTimeUseBackground::OnAction(const CAction &action)
{
  switch (action.id)
  {
  case ACTION_PREVIOUS_MENU:
  case ACTION_PARENT_DIR:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowFirstTimeUseBackground::OnAction - ACTION_PREVIOUS_MENU or ACTION_PARENT_DIR is not allow (ftu)");
    return true;
  }
  break;
  case ACTION_BUILT_IN_FUNCTION:
  {
    // don't allow during FTU
    CLog::Log(LOGDEBUG,"CGUIWindowFirstTimeUseBackground::OnAction - ACTION_BUILT_IN_FUNCTION is not allow (ftu)");
    return true;
  }
  break;
  }

  return CGUIWindow::OnAction(action);
}

