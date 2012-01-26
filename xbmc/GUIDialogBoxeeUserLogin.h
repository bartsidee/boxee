#pragma once
#include "GUIDialog.h"
#include "lib/libBoxee/bxobject.h"
#include "GUIDialogOK.h"

#include "lib/libBoxee/boxee.h"

class CGUIDialogBoxeeUserLogin :
      public CGUIDialogBoxBase
{
public:
  CGUIDialogBoxeeUserLogin(void);
  virtual ~CGUIDialogBoxeeUserLogin(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual bool OnAction(const CAction &action);
  virtual void OnInitWindow();
  
  void SetUserName(const CStdString &username);
  void SetPassword(const CStdString &pass);
  void SetRememberPass(bool bRemember);
  CStdString GetUserName();
  CStdString GetPassword();
  bool GetRememberPassword();
  bool IsDeleteUserWasChoose();
  
protected: 
  CStdString m_userName;
  CStdString m_password;
  CStdString m_offline_password;
  bool m_bRememberPass;
  int m_profileId;
  bool m_passwordIsEncode;
  bool m_DeleteUserWasChoose;
};
