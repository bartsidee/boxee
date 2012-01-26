#ifndef GUI_DIALOG_BOXEE_USER_PASSWORD
#define GUI_DIALOG_BOXEE_USER_PASSWORD

#pragma once

#include <vector>
#include "GUIDialog.h"

class CGUIDialogBoxeeUserPassword : public CGUIDialog
{
public:
  CGUIDialogBoxeeUserPassword(void);
  virtual ~CGUIDialogBoxeeUserPassword(void);
  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  bool IsConfirmed();  
  void SetUser(const CStdString& user) { m_user = user; }
  void SetPassword(const CStdString& password) { m_password = password; }
  const CStdString& GetUser() { return m_user; }
  const CStdString& GetPassword() { return m_password; }
  static bool ShowAndGetUserAndPassword(CStdString& strUser, CStdString& strPassword, const CStdString& strURL);
  
private:
  CStdString m_user;
  CStdString m_password;
  bool m_IsConfirmed;  
};

#endif
