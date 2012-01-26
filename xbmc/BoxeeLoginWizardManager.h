#pragma once

#include "BoxeeSimpleDialogWizardManager.h"
#include "lib/libBoxee/boxee.h"

#define FACEBOOK_SERVICE_ID                   "311"
#define TWITTER_SERVICE_ID                    "413"
#define TUMBLR_SERVICE_ID                     "323"

class CGUIDialogBoxeeWizardBase;

class CServerUserInfo
{
public:

  void Reset();
  bool InitializeByJson(const Json::Value& jResponse);

  bool IsInitialize();

  CStdString m_id;
  CStdString m_name;
  CStdString m_firstName;
  CStdString m_lastName;
  CStdString m_link;
  CStdString m_username;
  CStdString m_birthday;
  CStdString m_gender;
  CStdString m_email;
  int m_timezone;
  CStdString m_locale;
  bool m_verified;
  CStdString m_updatedTime;
  CStdString m_accessToken;
  CStdString m_code;

  CStdString m_serviceId;

  bool m_isInitialize;
};

class CBoxeeLoginWizardManager : public CBoxeeSimpleDialogWizardManager
{
public:

  static CBoxeeLoginWizardManager& GetInstance();
  virtual ~CBoxeeLoginWizardManager();

  bool RunWizard(bool shouldEndWizardOnEmptyStack);

  CServerUserInfo GetServerUserInfo();
  CServerUserInfo& GetServerUserInfoByRef();

protected:

  virtual CGUIDialogBoxeeWizardBase* HandleNextAction(CGUIDialogBoxeeWizardBase* pDialog, bool& addCurrentDlgToStack);

  virtual bool OnWizardComplete();

private:

  CBoxeeLoginWizardManager();

  void IsAddingDialogToStack(CGUIDialogBoxeeWizardBase* pNextDialog,bool& addCurrentDlgToStack);

  bool m_isAddingNewUser;

  CServerUserInfo m_serverUserInfo;
};
