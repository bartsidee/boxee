

#include "GUIDialogBoxeeCredits.h"

CGUIDialogBoxeeCredits::CGUIDialogBoxeeCredits(void)
    : CGUIDialog(WINDOW_DIALOG_BOXEE_CREDITS, "boxee_credits.xml")
{
}

CGUIDialogBoxeeCredits::~CGUIDialogBoxeeCredits(void)
{}

bool CGUIDialogBoxeeCredits::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      return true;
    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
      return CGUIDialog::OnMessage(message);
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}


