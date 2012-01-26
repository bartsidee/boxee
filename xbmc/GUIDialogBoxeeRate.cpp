

#include "GUIWindowManager.h"
#include "GUIDialogBoxeeRate.h"

#define BUTTON_LIKE 701
#define BUTTON_DONT_LIKE 702

CGUIDialogBoxeeRate::CGUIDialogBoxeeRate(void)
    : CGUIDialog(WINDOW_DIALOG_BOXEE_RATE, "boxee_rate.xml")
{
  m_bConfirmed = false;
  m_bLike = false;
}

CGUIDialogBoxeeRate::~CGUIDialogBoxeeRate()
{
}

bool CGUIDialogBoxeeRate::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == BUTTON_LIKE)
      {
        m_bLike = true;
        m_bConfirmed = true;
        Close();
        return true;
      }
      if (iControl == BUTTON_DONT_LIKE)
      {
        m_bLike = false;
        m_bConfirmed = true;
        Close();
        return true;
      }
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogBoxeeRate::OnAction(const CAction& action)
{
  if (action.id == ACTION_PREVIOUS_MENU)
  {
    m_bConfirmed = false;
    Close();
    return true;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeeRate::ShowAndGetInput(bool &bLike)
{
  CGUIDialogBoxeeRate *dialog = (CGUIDialogBoxeeRate *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_RATE);
  if (!dialog) return false;
  dialog->DoModal();
  bLike = dialog->m_bLike;
  return dialog->m_bConfirmed;
}


