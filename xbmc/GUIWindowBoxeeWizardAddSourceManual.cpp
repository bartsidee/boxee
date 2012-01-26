
#include "GUIWindowBoxeeWizardAddSourceManual.h"
#include "Application.h"
#include "GUIEditControl.h"
#include "GUIDialogKeyboard.h"
#include "utils/log.h"

#define CONTROL_HOST      9001
#define CONTROL_USER      9002
#define CONTROL_PASSWORD  9003
#define CONTROL_OK        9004
#define CONTROL_CANCEL    9005

CGUIWindowBoxeeWizardAddSourceManual::CGUIWindowBoxeeWizardAddSourceManual(void)
    : CGUIDialog(WINDOW_BOXEE_WIZARD_ADD_SOURCE_MANUAL, "boxee_wizard_add_source_manual.xml")
{
}

CGUIWindowBoxeeWizardAddSourceManual::~CGUIWindowBoxeeWizardAddSourceManual(void)
{
}

bool CGUIWindowBoxeeWizardAddSourceManual::OnAction(const CAction &action)
{
  if(action.id == ACTION_PREVIOUS_MENU)
  {
    m_IsConfirmed = false;
    Close();
    return true;
  }
  // don't allow any built in actions to act here.
  // this forces only navigation type actions to be performed.
  else if (action.id == ACTION_BUILT_IN_FUNCTION)
  {
    return true;  // pretend we handled it
  }
  
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowBoxeeWizardAddSourceManual::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {    
    DWORD senderId = message.GetSenderId();

    if(senderId == CONTROL_OK)
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddFolder::OnMessage - Enter GUI_MSG_CLICKED case with [SenderId=CONTROL_OK] (msmk)");

      CGUIEditControl* textButton = (CGUIEditControl*) GetControl(CONTROL_HOST);
      m_host = textButton->GetLabel2();
      textButton = (CGUIEditControl*) GetControl(CONTROL_USER);
      m_user = textButton->GetLabel2();
      textButton = (CGUIEditControl*) GetControl(CONTROL_PASSWORD);
      m_password = textButton->GetLabel2();      
      
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeWizardAddSourceManual::OnAction - In [iControl=%d=CONTROL_OK]. After set [m_host=%s][m_user=%s][m_password=%s] (was)(msmk)",senderId,m_host.c_str(),m_user.c_str(),m_password.c_str());
      
      m_IsConfirmed = true;
      Close();
      return true;       
    }
    else if(senderId == CONTROL_CANCEL)
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddFolder::OnMessage - Enter GUI_MSG_CLICKED case with [SenderId=CONTROL_CANCEL] (msmk)");

      m_IsConfirmed = false;
      Close();
      return true;
    }
  }
  break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIWindowBoxeeWizardAddSourceManual::OnInitWindow()
{
  CGUIWindow::OnInitWindow();
}  

CStdString CGUIWindowBoxeeWizardAddSourceManual::GetURL()
{
   CStdString result = "smb://";
   if (m_user != "")
   {
      result += m_user;
      if (m_password != "")
         result += ":" + m_password;
      result += "@";
   }
   result += m_host;

   CLog::Log(LOGDEBUG,"CGUIWindowBoxeeWizardAddSourceManual::GetURL - Exit function and return [result=%s]. [m_user=%s][m_password=%s][m_host=%s] (was)",result.c_str(),m_user.c_str(),m_password.c_str(),m_host.c_str());

   return result;
}

bool CGUIWindowBoxeeWizardAddSourceManual::IsConfirmed()
{
   return m_IsConfirmed;
}
