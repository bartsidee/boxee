#ifndef BOXEELOGINMANAGER_H_
#define BOXEELOGINMANAGER_H_

#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxobject.h"

#include <SDL/SDL.h>

#define NEW_USER_PROFILE_ID                           -100

class CBoxeeLoginStatusTypes
{
public:
  enum BoxeeLoginStatusEnums
  {
    LS_OK=0,
    LS_ERROR=1,
    LS_CREDENTIALS_ERROR=2,
    LS_NETWORK_ERROR=3,
    NUM_OF_LS=4
  };
};

class CBoxeeLoginManager
{
public:

  CBoxeeLoginManager();
  virtual ~CBoxeeLoginManager();

  bool Login();
  bool Logout();
  
  void SetInChangeUserProcess(bool inChangeUserProcess);
  bool IsInChangeUserProcess();

  void FinishSuccessfulLogin();

  static void SetProxyCreds(BOXEE::BXCredentials &creds);

  int CreateProfile(const CStdString& username, const BOXEE::BXObject& userObj);
  void UpdateProfile(int profileId, const CStdString& password, bool rememberPassword);

  static bool DoesThisUserAlreadyExist(const CStdString& username);
  static CStdString EncodePassword(CStdString password);

  static BOXEE::BXLoginStatus DoLogin(const CStdString& username, const CStdString& password, bool rememberPassword, int profileId = NEW_USER_PROFILE_ID);

private:

  static const char* CBoxeeLoginStatusEnumAsString(CBoxeeLoginStatusTypes::BoxeeLoginStatusEnums boxeeLoginStatusEnum);

  CBoxeeLoginStatusTypes::BoxeeLoginStatusEnums DoAutoLogin();
  CBoxeeLoginStatusTypes::BoxeeLoginStatusEnums DoUserLogin(int iItem);
  bool canTryAutoLogin();
  bool PostLoginActions();
  void HandleUpdateOnLogin();
  bool DoesThisUserAlreadyExist(CStdString& Username);

  bool m_inChangeUserProcess;
};

#endif /*BOXEELOGINMANAGER_H_*/
