#pragma once
#include "GUIDialog.h"

#include "lib/libBoxee/boxee.h"

class CGUIDialogBoxeeLoggingIn : public CGUIDialog, public BOXEE::BoxeeListener
{
public:
  CGUIDialogBoxeeLoggingIn(void);
  virtual ~CGUIDialogBoxeeLoggingIn(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();
  
  void Reset();
  void SetName(const CStdString &strName);

  BOXEE::BXLoginStatus IsLoginSuccessful();
  const BOXEE::BXObject &GetUserObj() { return m_userObj; }
  void SetProfileToLoad(int nProfile);

protected:
  void OnLoginEnded(const BOXEE::BXCredentials &creds, BOXEE::BXLoginStatus bResult, const BOXEE::BXObject &userObj) ;
  void LoginDone(); 

  void UpdateProfileToLoad();
  
  bool m_bLoginOk;
  int  m_nProfileToLoad;
  BOXEE::BXLoginStatus m_eLoginStat;
  CStdString m_strName;
  BOXEE::BXObject m_userObj;
};
