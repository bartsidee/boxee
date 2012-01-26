/*
 * CGUIDialogBoxeeUpdateProgress.cpp
 *
 */


#include "GUIDialogBoxeeUpdateProgress.h"
#include "utils/log.h"

#define CONTROL_LABEL  500

using namespace std;

CGUIDialogBoxeeUpdateProgress::CGUIDialogBoxeeUpdateProgress(void) : CGUIDialog(WINDOW_DIALOG_BOXEE_UPDATE_PROGRESS, "boxee_update_progress.xml")
{
  
}

CGUIDialogBoxeeUpdateProgress::~CGUIDialogBoxeeUpdateProgress(void)
{
  
}

bool CGUIDialogBoxeeUpdateProgress::OnAction(const CAction &action)
{
  // don't allow any built in actions to act here.
  // this forces only navigation type actions to be performed.
  if (action.id == ACTION_BUILT_IN_FUNCTION)
  {
    return true;  // pretend we handled it
  }
  else if (action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    // This dialog should not be closed because we are during update and it will be closed when Boxee will be shutdown
    //Close();
    return true;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeeUpdateProgress::OnMessage(CGUIMessage &message)
{
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogBoxeeUpdateProgress::OnInitWindow()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUpdateProgress::OnInitWindow - Enter function (update)");

  CGUIDialog::OnInitWindow();
}

void CGUIDialogBoxeeUpdateProgress::SetLabel(const CStdString& label)
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUpdateProgress::SetLabel - Enter function. Going to set label to [%s] (update)",label.c_str());

  SET_CONTROL_LABEL(CONTROL_LABEL,label);
}

