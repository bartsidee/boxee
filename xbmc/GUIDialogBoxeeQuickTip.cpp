#include "GUIDialogBoxeeQuickTip.h"
#include "GUIWindowManager.h"
#include "BoxeeUtils.h"
#include "log.h"

CGUIDialogBoxeeQuickTip::CGUIDialogBoxeeQuickTip() : CGUIDialog(WINDOW_DIALOG_BOXEE_QUICK_TIP, "DialogQuickTip.xml")
{

}


CGUIDialogBoxeeQuickTip::~CGUIDialogBoxeeQuickTip()
{

}

bool CGUIDialogBoxeeQuickTip::OnMessage(CGUIMessage &message)
{
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogBoxeeQuickTip::OnInitWindow()
{
  CGUIWindow::OnInitWindow();
}

void CGUIDialogBoxeeQuickTip::ShowAndGetInput()
{
  CGUIDialogBoxeeQuickTip* dialog = (CGUIDialogBoxeeQuickTip*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_QUICK_TIP);
  if (!dialog)
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeQuickTip::ShowAndGetInput - FAILED to get WINDOW_DIALOG_BOXEE_QUICK_TIP (login)");
    return;
  }

  if (strcmpi(BoxeeUtils::GetPlatformStr(),"dlink.dsm380") != 0)
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeQuickTip::ShowAndGetInput - [platform=%s] -> return (login)",BoxeeUtils::GetPlatformStr());
    return;
  }

  dialog->DoModal();
}
