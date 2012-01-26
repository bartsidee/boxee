#pragma once

#include "GUIDialogBoxeeWizardBase.h"
#include "lib/libBoxee/bxxmldocument.h"
#include "lib/libBoxee/bxobject.h"
#include "../lib/libjson/include/json/value.h"

class CGUIDialogBoxeeLoginWizardAddNewUser : public CGUIDialogBoxeeWizardBase
{
public:

  CGUIDialogBoxeeLoginWizardAddNewUser();
  virtual ~CGUIDialogBoxeeLoginWizardAddNewUser();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);

  virtual void Render();

  CStdString GetUserName();
  CStdString GetEmail();

protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

private:

  bool HandleClick(CGUIMessage& message);

  bool OnCreateUserThruFacebook();
  bool GetSocialServiceParams(const CStdString& serviceId);
  bool ParseSocialServiceParams(const CStdString& serviceId, const Json::Value& jResponse);
  bool OnCreateUser();
  void BuildCreateNewUserOnLoginPostData(CStdString& postData);
  void BuildNewUserUpdateServiceDataOnLoginPostData(CStdString& postData);
  bool ParseServerCreatedNewUserResponse(BOXEE::BXXMLDocument& doc, bool& isLogin, BOXEE::BXObject& newUserObj, CStdString& errorMessage);
  bool CreateNewUserProfile(const BOXEE::BXObject& newUserObj, int& profileId);
  CStdString EncodePassword(CStdString password);

  bool SetDialogFields();

  bool OnLoginButton();
  bool CanExecuteLogin();
  void checkDialogFields();

  bool DoLogin();

  CStdString m_validNameStr;
  bool m_isNameValid;
  int m_checkNameFieldDelay;
  void checkNameField();
  bool IsNameValid(const CStdString& username);

  CStdString m_validEmailStr;
  bool m_isEmailValid;
  int m_checkEmailDelay;
  void checkEmailField();
  bool IsEmailValid(const CStdString& email);

  CStdString m_validPasswordStr;
  bool m_isPasswordValid;
  int m_checkPasswordDelay;
  void checkPasswordField();
  bool IsPasswordValid(const CStdString& password);
};
