#include "GUIDialogBoxeeMessageScroll.h"
#include "GUIWindowManager.h"
#include "FileItem.h"

#define ID_TEXTBOX     20
#define ID_BUTTON_OK   10

CGUIDialogBoxeeMessageScroll::CGUIDialogBoxeeMessageScroll() : CGUIDialogBoxBase(WINDOW_DIALOG_BOXEE_MESSAGE_SCROLL, "boxee_message_scroll.xml")
{

}

CGUIDialogBoxeeMessageScroll::~CGUIDialogBoxeeMessageScroll()
{

}

bool CGUIDialogBoxeeMessageScroll::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == ID_BUTTON_OK)
      {
        m_bConfirmed = true;
        Close();
        return true;
      }
    }
    break;
  }
  return CGUIDialogBoxBase::OnMessage(message);
}

void CGUIDialogBoxeeMessageScroll::ShowAndGetInput(const CStdString&  heading, const CStdString& line)
{
  CGUIDialogBoxeeMessageScroll* dialog = (CGUIDialogBoxeeMessageScroll*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_MESSAGE_SCROLL);
  if (!dialog)
  {
    return;
  }

  dialog->SetHeading( heading );

  CFileItemPtr itemPtr(new CFileItem(line));
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, dialog->GetID(), 5000, 0, 0, itemPtr);
  g_windowManager.SendThreadMessage(winmsg);

  dialog->DoModal();
}
