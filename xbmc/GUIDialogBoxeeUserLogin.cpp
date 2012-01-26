
#include "GUIWindowManager.h"
#include "GUIDialogBoxeeUserLogin.h"
#include "Settings.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogKeyboard.h"
#include "GUIButtonControl.h"
#include "GUIEditControl.h"
#include "GUIRadioButtonControl.h"
#include "boxee.h"
#include "Util.h"
#include "utils/md5.h"
#include "utils/Base64.h"
#include "GUIDialogOK.h"
#include "GUIDialogBusy.h"
#include "GUIDialogProgress.h"
#include "Util.h"
#include "FileSystem/Directory.h"
#include "Application.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogBoxeeLoggingIn.h"
#include "GUIWindowLoginScreen.h"
#include "utils/log.h"
#include "GUILabelControl.h"

#define CONTROL_USERID         9001
#define CONTROL_PASSWORD       9002
#define CONTROL_REMEMBER       9003
#define CONTROL_OK             9004
#define CONTROL_CANCEL         9005
#define CONTROL_DELETE         9006
#define CONTROL_USERID_LABEL   9010

using namespace std;

CGUIDialogBoxeeUserLogin::CGUIDialogBoxeeUserLogin(void)
    : CGUIDialogBoxBase(WINDOW_DIALOG_BOXEE_USER_LOGIN, "boxee_user_login.xml")
{
   m_password = "";
   m_offline_password = "";
   m_userName = "";
   m_bRememberPass = false;
   m_passwordIsEncode = false;
   m_DeleteUserWasChoose = false;
}

CGUIDialogBoxeeUserLogin::~CGUIDialogBoxeeUserLogin(void)
{}

bool CGUIDialogBoxeeUserLogin::OnAction(const CAction &action)
{
  // don't allow any built in actions to act here.
  // this forces only navigation type actions to be performed.
  if (action.id == ACTION_BUILT_IN_FUNCTION)
  {
    return true;  // pretend we handled it
  }

  return CGUIDialogBoxBase::OnAction(action);
}

bool CGUIDialogBoxeeUserLogin::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
     int iControl = message.GetSenderId();
     const CGUIControl* control = GetControl(iControl);
     
     if (iControl == CONTROL_REMEMBER)
     {
       CGUIDialogBoxBase::OnMessage(message);
       m_bRememberPass = ((CGUIRadioButtonControl*) control)->IsSelected();
       return true;
     }
     else if (iControl == CONTROL_OK)
     {
       m_userName = ((CGUIEditControl*)GetControl(CONTROL_USERID))->GetLabel2();
       m_password = m_offline_password = ((CGUIEditControl*)GetControl(CONTROL_PASSWORD))->GetLabel2();
       if (!(m_userName.IsEmpty() || m_password.IsEmpty()))
       {
         Close();
         m_bConfirmed = true;
         return true;
       }
       else
       { 
         if(m_userName.IsEmpty())
         {
           // Case of empty UserName field
           CGUIDialogOK::ShowAndGetInput(20068,51058,20022,20022);
         }
         else
         {
           // Case of empty Password field
           CGUIDialogOK::ShowAndGetInput(20068,51059,20022,20022);
         }
       }
     }
     else if (iControl == CONTROL_CANCEL)
     {
       m_bConfirmed = false;
       Close();
       return true;
     }
     else if (iControl == CONTROL_DELETE)
     {
       CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUserLogin::OnMessage - In GUI_MSG_CLICKED. CONTROL_DELETE was clicked (login)");
       
       m_bConfirmed = false;
       m_DeleteUserWasChoose = true;
       Close();
       return true;
     }
  }

  return CGUIDialogBoxBase::OnMessage(message);
}

bool CGUIDialogBoxeeUserLogin::IsDeleteUserWasChoose()
{
  return m_DeleteUserWasChoose;   
}

void CGUIDialogBoxeeUserLogin::SetRememberPass(bool bRemember)
{
   m_bRememberPass = bRemember;
}

void CGUIDialogBoxeeUserLogin::OnInitWindow()
{
  CGUIDialogBoxBase::OnInitWindow();

  ((CGUILabelControl*)GetControl(CONTROL_USERID_LABEL))->SetLabel(m_userName);
  ((CGUIEditControl*)GetControl(CONTROL_USERID))->SetLabel2(m_userName);

  if (m_userName != "")
  {
    SET_CONTROL_HIDDEN(CONTROL_USERID);
    SET_CONTROL_VISIBLE(CONTROL_USERID_LABEL);
    SET_CONTROL_VISIBLE(CONTROL_DELETE);

    SET_CONTROL_FOCUS(CONTROL_PASSWORD, 1);
  }
  else
  {
    SET_CONTROL_HIDDEN(CONTROL_USERID_LABEL);
    SET_CONTROL_VISIBLE(CONTROL_USERID);
    SET_CONTROL_HIDDEN(CONTROL_DELETE);
    
    SET_CONTROL_FOCUS(CONTROL_USERID, 1);
  }
  
  if (!m_password.IsEmpty())
  {
    ((CGUIRadioButtonControl*) GetControl(CONTROL_REMEMBER))->SetSelected(true);
    ((CGUIEditControl*) GetControl(CONTROL_PASSWORD))->SetLabel2(m_password);
    m_bRememberPass = true;
    m_passwordIsEncode = true;
  }
  else
  {
    ((CGUIRadioButtonControl*) GetControl(CONTROL_REMEMBER))->SetSelected(false);
    m_bRememberPass = false;
    m_passwordIsEncode = false;
  }
  
  m_DeleteUserWasChoose = false;
}

void CGUIDialogBoxeeUserLogin::SetPassword(const CStdString &pass)
{
   if (pass == "-" || pass.IsEmpty())
   {
     m_password = "";
   }
   else
   {
     m_password = pass;
   }
}

void CGUIDialogBoxeeUserLogin::SetUserName(const CStdString &username)
{
   m_userName = username;
}

CStdString CGUIDialogBoxeeUserLogin::GetUserName()
{
   return m_userName.ToLower();
}

CStdString CGUIDialogBoxeeUserLogin::GetPassword()
{
  CStdString password = m_password;
  
  if(!m_passwordIsEncode)
  {
    XBMC::MD5 md5;
    unsigned char md5hash[16];
    md5.append((unsigned char *)m_password.c_str(), (int)m_password.size());
    md5.getDigest(md5hash);

    password = CBase64::Encode(md5hash, 16);
  }
  
  return password;
}

bool CGUIDialogBoxeeUserLogin::GetRememberPassword()
{
  return m_bRememberPass;
}
