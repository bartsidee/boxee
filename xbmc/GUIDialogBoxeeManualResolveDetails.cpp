
#include "GUIDialogBoxeeManualResolveDetails.h"
#include "GUIWindowManager.h"
#include "Util.h"
#include "utils/log.h"
#include "LocalizeStrings.h"

using namespace BOXEE;

CGUIDialogBoxeeManualResolveDetails::CGUIDialogBoxeeManualResolveDetails() :
  CGUIDialog(WINDOW_DIALOG_BOXEE_MANUAL_DETAILS, "boxee_manual_resolve_details.xml")
  {
  }

CGUIDialogBoxeeManualResolveDetails::~CGUIDialogBoxeeManualResolveDetails() {
}

void CGUIDialogBoxeeManualResolveDetails::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  // Send the item to the special container to allow skin acceess 
  CFileItemPtr itemPtr(new CFileItem(*m_VideoItem.get()));
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), 5000, 0, 0, itemPtr);
  g_windowManager.SendThreadMessage(winmsg);

}

void CGUIDialogBoxeeManualResolveDetails::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
}

bool CGUIDialogBoxeeManualResolveDetails::OnMessage(CGUIMessage& message)
{
  return CGUIDialog::OnMessage(message);
}

