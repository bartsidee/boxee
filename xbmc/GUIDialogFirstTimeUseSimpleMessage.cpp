#include "GUIDialogFirstTimeUseSimpleMessage.h"

#ifdef HAS_EMBEDDED

#include "GUILabelControl.h"
#include "Application.h"
#include "LocalizeStrings.h"
#include "log.h"
#include "InitializeBoxManager.h"

#define SIMPLE_MESSAGE_LABEL_CONTROL 259

CGUIDialogFirstTimeUseSimpleMessage::CGUIDialogFirstTimeUseSimpleMessage() : CGUIDialogFirstTimeUseBase(WINDOW_DIALOG_FTU_SIMPLE_MESSAGE,"ftu_simple_message.xml","CGUIDialogFirstTimeUseSimpleMessage")
{

}

CGUIDialogFirstTimeUseSimpleMessage::~CGUIDialogFirstTimeUseSimpleMessage()
{

}

void CGUIDialogFirstTimeUseSimpleMessage::OnInitWindow()
{
  CGUIDialogFirstTimeUseBase::OnInitWindow();

  ((CGUILabelControl*)GetControl(SIMPLE_MESSAGE_LABEL_CONTROL))->SetLabel(m_message);
}

bool CGUIDialogFirstTimeUseSimpleMessage::OnAction(const CAction &action)
{
  if (action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    // don't allow back in this dialog

    return true;
  }

  return CGUIDialogFirstTimeUseBase::OnAction(action);
}

bool CGUIDialogFirstTimeUseSimpleMessage::OnMessage(CGUIMessage& message)
{
  return CGUIDialogFirstTimeUseBase::OnMessage(message);
}

bool CGUIDialogFirstTimeUseSimpleMessage::HandleClickNext()
{
  // nothing to do

  return true;
}

bool CGUIDialogFirstTimeUseSimpleMessage::HandleClickBack()
{
  // nothing to do

  return true;
}

void CGUIDialogFirstTimeUseSimpleMessage::SetMessage(const CStdString& message)
{
  m_message = message;
}

#endif

