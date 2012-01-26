#include "GUIDialogFirstTimeUseBase.h"

#ifdef HAS_EMBEDDED

#include "log.h"

CGUIDialogFirstTimeUseBase::CGUIDialogFirstTimeUseBase(int id, const CStdString& xmlFile, const CStdString& name) : CGUIDialog(id, xmlFile)
{
  m_name = name;
}

CGUIDialogFirstTimeUseBase::~CGUIDialogFirstTimeUseBase()
{

}

bool CGUIDialogFirstTimeUseBase::OnAction(const CAction &action)
{
  if (action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    m_actionChoseEnum = CActionChose::BACK;
    Close();
    return true;
  }
  else if (action.id == ACTION_BUILT_IN_FUNCTION)
  {
    // don't allow during FTU
    CLog::Log(LOGDEBUG,"%s::OnAction - ACTION_BUILT_IN_FUNCTION is not allow (ftu)",m_name.c_str());
    return true;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogFirstTimeUseBase::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    int senderId = message.GetSenderId();

    CLog::Log(LOGDEBUG,"%s::OnMessage - GUI_MSG_CLICKED - [buttonId=%d] (initbox)",m_name.c_str(),senderId);

    switch (senderId)
    {
    case BUTTON_NEXT:
    {
      if (HandleClickNext())
      {
        m_actionChoseEnum = CActionChose::NEXT;
        Close();
      }

      return true;
    }
    break;
    case BUTTON_BACK:
    {
      if (HandleClickBack())
      {
        m_actionChoseEnum = CActionChose::BACK;
        Close();
      }

      return true;
    }
    break;
    }
  }
  }

  return CGUIDialog::OnMessage(message);
}

CActionChose::ActionChoseEnums CGUIDialogFirstTimeUseBase::GetActionChose()
{
  return m_actionChoseEnum;
}

void CGUIDialogFirstTimeUseBase::SetActionChose(CActionChose::ActionChoseEnums actionChoseEnum)
{
  m_actionChoseEnum = actionChoseEnum;
}

#endif

