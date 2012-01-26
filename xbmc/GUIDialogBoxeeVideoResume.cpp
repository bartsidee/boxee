/*
 * CGUIDialogBoxeeVideoResume
 *
 */
#include "GUIDialogBoxeeVideoResume.h"
#include "GUIWindowManager.h"
#include "FileItem.h"
#include "log.h"

#define CONTROL_LIST     11

CGUIDialogBoxeeVideoResume::CGUIDialogBoxeeVideoResume() : CGUIDialog(WINDOW_DIALOG_BOXEE_VIDEO_RESUME, "boxee_video_resume.xml")
{
  m_choiceIndex = -1;
}

CGUIDialogBoxeeVideoResume::~CGUIDialogBoxeeVideoResume()
{

}

bool CGUIDialogBoxeeVideoResume::OnAction(const CAction& action)
{
  switch (action.id)
  {
  case ACTION_PARENT_DIR:
  case ACTION_PREVIOUS_MENU:
  {
    m_choiceIndex = -1;
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeVideoResume::OnAction - set [choiceIndex=%d]. [NumOfChoices=%d] (resume)",m_choiceIndex,(int)m_choices.size());
    Close();
    return true;
  }
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeeVideoResume::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    int iControl = message.GetSenderId();

    if (iControl == CONTROL_LIST)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST);
      g_windowManager.SendMessage(msg);
      m_choiceIndex = msg.GetParam1();

      if ((m_choiceIndex < 0) || (m_choiceIndex >= (int)m_choices.size()))
      {
        CLog::Log(LOGERROR,"CGUIDialogBoxeeVideoResume::OnMessage - GUI_MSG_CLICKED - Index of item clicked is [%d] while [NumOfChoices=%d] (resume)",m_choiceIndex,(int)m_choices.size());
        m_choiceIndex = -1;
        Close();
        return false;
      }

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeVideoResume::OnMessage - GUI_MSG_CLICKED - set [choiceIndex=%d]. [NumOfChoices=%d] (resume)",m_choiceIndex,(int)m_choices.size());

      Close();
      return true;
    }
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogBoxeeVideoResume::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  m_choiceIndex = -1;

  if ((int)m_choices.size() < 1)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeVideoResume::OnInitWindow - There are no label to add. [NumOfChoices=%d] (resume)",(int)m_choices.size());
    Close();
    return;
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeVideoResume::OnInitWindow - going to add [NumOfChoices=%d] (resume)",(int)m_choices.size());

  for (int i=0; i<(int)m_choices.size(); i++)
  {
    CFileItemPtr choice(new CFileItem(m_choices[i]));
    CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_LIST, 0, 0, choice);
    OnMessage(winmsg);
  }

  SET_CONTROL_FOCUS(CONTROL_LIST, 0);
}

int CGUIDialogBoxeeVideoResume::ShowAndGetInput(const std::vector<CStdString>& choices)
{
  CGUIDialogBoxeeVideoResume *dialog = (CGUIDialogBoxeeVideoResume*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_VIDEO_RESUME);
  if (!dialog)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeVideoResume::ShowAndGetInput - FAILED to get WINDOW_DIALOG_BOXEE_VIDEO_RESUME (resume)");
    return false;
  }

  dialog->m_choices = choices;

  dialog->DoModal();

  return dialog->m_choiceIndex;
}
