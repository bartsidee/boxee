
#include "GUIWindowBoxeeBrowsePhotos.h"
#include "FileSystem/BoxeeServerDirectory.h"
#include "GUIWindowManager.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "URL.h"
#include "Util.h"
#include "BoxeeUtils.h"
#include "utils/log.h"
#include "GUIUserMessages.h"
#include "LocalizeStrings.h"
#include "Application.h"

using namespace std;
using namespace BOXEE;

#define BUTTON_SHORTCUT 160

#define PHOTOS_WINDOW_SHORTCUT_COMMAND   "ActivateWindow(10491,%s)"

CPhotosWindowState::CPhotosWindowState(CGUIWindow* pWindow) : CLocalBrowseWindowState(pWindow)
{

}

CGUIWindowBoxeeBrowsePhotos::CGUIWindowBoxeeBrowsePhotos()
: CGUIWindowBoxeeBrowseWithPanel(WINDOW_BOXEE_BROWSE_PHOTOS, "boxee_browse_photos.xml")
{
  SetWindowState(new CPhotosWindowState(this));
}

CGUIWindowBoxeeBrowsePhotos::~CGUIWindowBoxeeBrowsePhotos()
{
  
}

void CGUIWindowBoxeeBrowsePhotos::OnInitWindow()
{
  CGUIWindowBoxeeBrowse::OnInitWindow();

  UpdateShortcutButton();
}

void CGUIWindowBoxeeBrowsePhotos::UpdateShortcutButton()
{
  CStdString label = "";

  bool bHasShortcut = ((CPhotosWindowState*)m_windowState)->UpdateHasShortcut(PHOTOS_WINDOW_SHORTCUT_COMMAND);

  if (bHasShortcut)
  {
    label = g_localizeStrings.Get(53716);
    SET_CONTROL_LABEL(BUTTON_SHORTCUT, label.ToUpper());
  }
  else
  {
    label = g_localizeStrings.Get(53715);
    SET_CONTROL_LABEL(BUTTON_SHORTCUT, label.ToUpper());
  }
}

bool CGUIWindowBoxeeBrowsePhotos::OnBind(CGUIMessage& message)
{
  if (message.GetPointer() && message.GetControlId() == 0)
  {
    UpdateShortcutButton();
    CONTROL_ENABLE(BUTTON_SHORTCUT);
  }

  return CGUIWindowBoxeeBrowse::OnBind(message);
}

bool CGUIWindowBoxeeBrowsePhotos::ProcessPanelMessages(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();

    if (iControl == BUTTON_SHORTCUT)
    {
      if (((CLocalBrowseWindowState*)m_windowState)->OnShortcut(PHOTOS_WINDOW_SHORTCUT_COMMAND))
      {
        // shortcut was added
        SET_CONTROL_LABEL(BUTTON_SHORTCUT, g_localizeStrings.Get(53716));
        g_application.m_guiDialogKaiToast.QueueNotification("", "", g_localizeStrings.Get(53737), 5000);
      }
      else
      {
        // shortcut was removed
        SET_CONTROL_LABEL(BUTTON_SHORTCUT, g_localizeStrings.Get(53715));
        g_application.m_guiDialogKaiToast.QueueNotification("", "", g_localizeStrings.Get(53739), 5000);
      }

      return true;
    }
  } // case GUI_MSG_CLICKED
  // else - break from switch and return false
  break;
  } // switch

  return CGUIWindowBoxeeBrowseWithPanel::ProcessPanelMessages(message);
}
