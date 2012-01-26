#include "GUIDialogBoxeeWizardBase.h"
#include "log.h"

CGUIDialogBoxeeWizardBase::CGUIDialogBoxeeWizardBase(int id, const CStdString& xmlFile, const CStdString& name) : CGUIDialog(id, xmlFile)
{
  m_name = name;
}

CGUIDialogBoxeeWizardBase::~CGUIDialogBoxeeWizardBase()
{

}

bool CGUIDialogBoxeeWizardBase::OnAction(const CAction &action)
{
  if (action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    m_actionChoseEnum = CActionChoose::BACK;
    Close();
    return true;
  }
  else if (action.id == ACTION_BUILT_IN_FUNCTION)
  {
    // don't allow during FTU
    CLog::Log(LOGDEBUG,"%s::OnAction - ACTION_BUILT_IN_FUNCTION is not allow (digwiz)",m_name.c_str());
    return true;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeeWizardBase::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    int senderId = message.GetSenderId();

    CLog::Log(LOGDEBUG,"%s::OnMessage - GUI_MSG_CLICKED - [buttonId=%d] (digwiz)",m_name.c_str(),senderId);

    switch (senderId)
    {
    case DIALOG_WIZARD_BUTTON_NEXT:
    {
      if (HandleClickNext())
      {
        m_actionChoseEnum = CActionChoose::NEXT;
        Close();
      }

      return true;
    }
    break;
    case DIALOG_WIZARD_BUTTON_BACK:
    {
      if (HandleClickBack())
      {
        m_actionChoseEnum = CActionChoose::BACK;
        Close();
      }

      return true;
    }
    break;
    }
  }
  break;
  }

  return CGUIDialog::OnMessage(message);
}

CActionChoose::ActionChooseEnums CGUIDialogBoxeeWizardBase::GetActionChose()
{
  return m_actionChoseEnum;
}

void CGUIDialogBoxeeWizardBase::SetActionChose(CActionChoose::ActionChooseEnums actionChoseEnum)
{
  m_actionChoseEnum = actionChoseEnum;
}

