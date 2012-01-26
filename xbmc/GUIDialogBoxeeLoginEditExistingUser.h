#pragma once

#include "GUIDialog.h"

class CGUIDialogBoxeeLoginEditExistingUser : public CGUIDialog
{
public:

  CGUIDialogBoxeeLoginEditExistingUser();
  virtual ~CGUIDialogBoxeeLoginEditExistingUser();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);

  virtual void Render();

  void SetUsername(const CStdString& username);
  CStdString GetUsername();

  void SetUserId(const CStdString& userId);
  CStdString GetUserId();

  void SetPassword(const CStdString& password);
  CStdString GetPassword();

  void SetRememberPassword(bool rememberPassword);
  bool IsRememberPassword();

  void SetUserProfileIndex(int userProfileIndex);
  int GetUserProfileIndex();

  void SetThumb(const CStdString& thumb);
  CStdString GetThumb();

  static bool Show(int userProfileIndex, const CStdString& username, const CStdString& userId, CStdString& password, bool& rememberPassword, const CStdString& thumb);

protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

private:

  bool HandleClick(CGUIMessage& message);

  bool CanExecuteLogin();
  bool OnLoginButton();

  bool OnRemoveButton();

  bool IsPasswordFiledEmpty();

  int m_checkPasswordFieldDelay;
  bool m_needToCheckEmptyPassword;

  CStdString m_username;
  CStdString m_userId;
  CStdString m_password;
  int m_userProfileIndex;
  bool m_rememberPassword;
  CStdString m_thumb;
};
