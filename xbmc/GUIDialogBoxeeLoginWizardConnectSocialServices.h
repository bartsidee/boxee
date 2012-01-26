#pragma once

#include "GUIDialogBoxeeWizardBase.h"
#include "FileItem.h"
#include "lib/libjson/include/json/value.h"

class CGUIDialogBoxeeLoginWizardConnectSocialServices : public CGUIDialogBoxeeWizardBase
{
public:

  CGUIDialogBoxeeLoginWizardConnectSocialServices();
  virtual ~CGUIDialogBoxeeLoginWizardConnectSocialServices();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);

  bool IsFacebookConnected();
  bool IsTwitterConnected();
  bool IsTumblrConnected();

protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

private:

  bool HandleClick(CGUIMessage& message);

  bool HandleClickOnFacebookButton();
  bool HandleClickOnTwitterButton();
  bool HandleClickOnTumblrButton();

  bool HandleClickOnNextButton();
  bool HandleClickOnSkipButton();

  bool GetSocialServicesStatus();

  bool ConnectSocialService(int serviceButtonId);

  CFileItemList m_servicesList;
  Json::Value m_jsonServiceList;

  bool m_isFacebookConnected;
  bool m_isTwitterConnected;
  bool m_isTumblrConnected;

  CGUIWindow* m_activeWindow;

  std::map<int,CStdString> m_servButtonToLinkMap;
  std::map<int,CStdString> m_servButtonToExternalLinkMap;
};
