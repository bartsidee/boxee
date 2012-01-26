
#include "GUIDialogBoxeeGetFacebookExtraCredential.h"

#define CONTROL_BUTTON_YES 11
#define CONTROL_BUTTON_NO  10


CGUIDialogBoxeeGetFacebookExtraCredential::CGUIDialogBoxeeGetFacebookExtraCredential(void)
    : CGUIDialogBoxBase(WINDOW_DIALOG_BOXEE_GET_FACEBOOK_EXTRA_CRED, "boxee_get_facebook_extra_cred.xml")
{
}

CGUIDialogBoxeeGetFacebookExtraCredential::~CGUIDialogBoxeeGetFacebookExtraCredential()
{
}

void CGUIDialogBoxeeGetFacebookExtraCredential::OnInitWindow()
{
  m_bConfirmed = false;
  CGUIDialogBoxBase::OnInitWindow();
}

bool CGUIDialogBoxeeGetFacebookExtraCredential::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    if (message.GetSenderId() == CONTROL_BUTTON_YES)
    {
      m_bConfirmed = true;
      Close();
      return true;
    }
    if (message.GetSenderId() == CONTROL_BUTTON_NO)
    {
      m_bConfirmed = false;
      Close();
      return true;
    }

  }
  break;
  }
  return CGUIDialogBoxBase::OnMessage(message);
}

bool CGUIDialogBoxeeGetFacebookExtraCredential::OnAction(const CAction& action)
{
  return CGUIDialogBoxBase::OnAction(action);
}

void CGUIDialogBoxeeGetFacebookExtraCredential::SetDialogText(const CStdString& heading, const CStdString& line, const CStdString& yesLabel, const CStdString& noLabel)
{
  SetHeading(heading);
  SetLine(0,line);
  SetChoice(1,yesLabel);
  SetChoice(0,noLabel);
}
