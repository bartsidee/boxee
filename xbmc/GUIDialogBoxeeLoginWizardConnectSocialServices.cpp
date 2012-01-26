#include "GUIDialogBoxeeLoginWizardConnectSocialServices.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIWindowManager.h"
#include "GUIWebDialog.h"
#include "BoxeeUtils.h"
#include "BoxeeLoginWizardManager.h"
#include "LocalizeStrings.h"
#include "GUIDialogOK2.h"

#define SOCIAL_BUTTONS_GROUP_CONTROL_ID      7000
#define FACEBOOK_BUTTON_CONTROL_ID           8700
#define TWITTER_BUTTON_CONTROL_ID            8701
#define TUMBLR_BUTTON_CONTROL_ID             8702

#define ACTION_BUTTONS_GROUP_CONTROL_ID      8704
#define NEXT_BUTTON_CONTROL_ID               8705
#define SKIP_BUTTON_CONTROL_ID               8706



#define FACEBOOK_CONNECTED_WINDOW_PROPERTY    "facebook-connected"
#define TWITTER_CONNECTED_WINDOW_PROPERTY     "twitter-connected"
#define TUMBLR_CONNECTED_WINDOW_PROPERTY      "tumblr-connected"

CGUIDialogBoxeeLoginWizardConnectSocialServices::CGUIDialogBoxeeLoginWizardConnectSocialServices() : CGUIDialogBoxeeWizardBase(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_CONNECT_SOCIAL_SERVICES,"boxee_login_wizard_connect_social_services.xml","CGUIDialogBoxeeLoginWizardConnectSocialServices")
{
  m_isFacebookConnected = false;
  m_isTwitterConnected = false;
  m_isTumblrConnected = false;

}

CGUIDialogBoxeeLoginWizardConnectSocialServices::~CGUIDialogBoxeeLoginWizardConnectSocialServices()
{

}

void CGUIDialogBoxeeLoginWizardConnectSocialServices::OnInitWindow()
{
  CGUIDialogBoxeeWizardBase::OnInitWindow();

  if (!GetSocialServicesStatus())
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardConnectSocialServices::OnInitWindow - FAILED to get social services status from server. going to set action to NEXT and close dialog (blw)(digwiz)");
    m_actionChoseEnum = CActionChoose::NEXT;
    Close();
    return;
  }

  m_activeWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardConnectSocialServices::OnInitWindow - after getting social services status from server [isFacebookConnected=%d][isTwitterConnected=%d][isTumblrConnected=%d] (blw)(digwiz)",m_isFacebookConnected,m_isTwitterConnected,m_isTumblrConnected);

  m_activeWindow->SetProperty(FACEBOOK_CONNECTED_WINDOW_PROPERTY,m_isFacebookConnected);
  m_activeWindow->SetProperty(TWITTER_CONNECTED_WINDOW_PROPERTY,m_isTwitterConnected);
  m_activeWindow->SetProperty(TUMBLR_CONNECTED_WINDOW_PROPERTY,m_isTumblrConnected);
}

bool CGUIDialogBoxeeLoginWizardConnectSocialServices::OnAction(const CAction& action)
{
  return CGUIDialogBoxeeWizardBase::OnAction(action);
}

bool CGUIDialogBoxeeLoginWizardConnectSocialServices::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    if (HandleClick(message))
    {
      // continue to stay in this screen
      return true;
    }
  }
  break;
  }

  return CGUIDialogBoxeeWizardBase::OnMessage(message);
}

bool CGUIDialogBoxeeLoginWizardConnectSocialServices::HandleClick(CGUIMessage& message)
{
  int iControl = message.GetSenderId();

  switch(iControl)
  {
  case FACEBOOK_BUTTON_CONTROL_ID:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardConnectSocialServices::HandleClick - handling click on [Control=%d=FACEBOOK_BUTTON_CONTROL_ID] (blw)(digwiz)",iControl);
    return HandleClickOnFacebookButton();
  }
  break;
  case TWITTER_BUTTON_CONTROL_ID:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardConnectSocialServices::HandleClick - handling click on [Control=%d=TWITTER_BUTTON_CONTROL_ID] (blw)(digwiz)",iControl);
    return HandleClickOnTwitterButton();
  }
  break;
  case TUMBLR_BUTTON_CONTROL_ID:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardConnectSocialServices::HandleClick - handling click on [Control=%d=TUMBLR_BUTTON_CONTROL_ID] (blw)(digwiz)",iControl);
    return HandleClickOnTumblrButton();
  }
  break;
  case NEXT_BUTTON_CONTROL_ID:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardConnectSocialServices::HandleClick - handling click on [Control=%d=NEXT_BUTTON_CONTROL_ID] (blw)(digwiz)",iControl);
    return HandleClickOnNextButton();
  }
  break;
  case SKIP_BUTTON_CONTROL_ID:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardConnectSocialServices::HandleClick - handling click on [Control=%d=SKIP_BUTTON_CONTROL_ID] (blw)(digwiz)",iControl);
    return HandleClickOnSkipButton();
  }
  break;
  }

  return false;
}

bool CGUIDialogBoxeeLoginWizardConnectSocialServices::HandleClickOnFacebookButton()
{
  if (IsFacebookConnected())
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardConnectSocialServices::HandleClickOnFacebookButton - FACEBOOK is already enable (blw)(digwiz)");
    return true;
  }

  if (!ConnectSocialService(FACEBOOK_BUTTON_CONTROL_ID))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardConnectSocialServices::HandleClickOnFacebookButton - FAILED to connect FACEBOOK (blw)(digwiz)");
    return true;
  }

  m_isFacebookConnected = true;
  m_activeWindow->SetProperty(FACEBOOK_CONNECTED_WINDOW_PROPERTY,m_isFacebookConnected);

  return true;
}

bool CGUIDialogBoxeeLoginWizardConnectSocialServices::HandleClickOnTwitterButton()
{
  if (IsTwitterConnected())
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardConnectSocialServices::HandleClickOnTwitterButton - TWITTER is already enable (blw)(digwiz)");
    return true;
  }

  if (!ConnectSocialService(TWITTER_BUTTON_CONTROL_ID))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardConnectSocialServices::HandleClickOnTwitterButton - FAILED to connect TWITTER (blw)(digwiz)");
    return true;
  }

  m_isTwitterConnected = true;
  m_activeWindow->SetProperty(TWITTER_CONNECTED_WINDOW_PROPERTY,m_isTwitterConnected);

  return true;
}

bool CGUIDialogBoxeeLoginWizardConnectSocialServices::HandleClickOnTumblrButton()
{
  if (IsTumblrConnected())
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardConnectSocialServices::HandleClickOnTumblrButton - TUMBLR is already enable (blw)(digwiz)");
    return true;
  }

  if (!ConnectSocialService(TUMBLR_BUTTON_CONTROL_ID))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardConnectSocialServices::HandleClickOnTumblrButton - FAILED to connect TUMBLR (blw)(digwiz)");
    return true;
  }

  m_isTumblrConnected = true;
  m_activeWindow->SetProperty(TUMBLR_CONNECTED_WINDOW_PROPERTY,m_isTumblrConnected);

  return true;
}

bool CGUIDialogBoxeeLoginWizardConnectSocialServices::ConnectSocialService(int serviceButtonId)
{
#ifdef CANMORE
  CStdString url = m_servButtonToLinkMap[serviceButtonId];
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardConnectSocialServices::ConnectSocialService - open GUIWebDialog with [url=%s] (blw)(digwiz)",url.c_str());

  return CGUIWebDialog::ShowAndGetInput(url);
#else
  CStdString url = m_servButtonToExternalLinkMap[serviceButtonId];
  CStdString text = g_localizeStrings.Get(80001);
  text += " " + url;
  CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(10014),text);
  return false;
#endif
}

bool CGUIDialogBoxeeLoginWizardConnectSocialServices::HandleClickOnNextButton()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardConnectSocialServices::HandleClickOnNextButton - enter function (blw)");

  m_actionChoseEnum = CActionChoose::NEXT;
  Close();
  return true;
}

bool CGUIDialogBoxeeLoginWizardConnectSocialServices::HandleClickOnSkipButton()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardConnectSocialServices::HandleClickOnSkipButton - enter function (blw)");

  m_actionChoseEnum = CActionChoose::NEXT;
  Close();
  return true;
}

bool CGUIDialogBoxeeLoginWizardConnectSocialServices::GetSocialServicesStatus()
{
  int retCode;
  Job_Result jobResult = BoxeeUtils::GetShareServicesJson(m_jsonServiceList,retCode);

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardConnectSocialServices::GetSocialServicesStatus - call to get SocialServices status from server returned [jobResult=%d] (blw)(digwiz)",jobResult);

  if (jobResult != JOB_SUCCEEDED)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardConnectSocialServices::GetSocialServicesStatus - FAILED to get SocialServices status from server. [jobResult=%d] (blw)(digwiz)",jobResult);
    return false;
  }

  m_servicesList.Clear();
  m_servButtonToLinkMap.clear();
  m_servButtonToExternalLinkMap.clear();
  BoxeeUtils::ParseJsonShareServicesToFileItems(m_jsonServiceList,m_servicesList);

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardConnectSocialServices::GetSocialServicesStatus - after parse SocialServices to FIleItemList. [NumOfSocialServices=%d] (blw)(digwiz)",m_servicesList.Size());

  for (int i=0; i<m_servicesList.Size(); i++)
  {
    CFileItemPtr item = m_servicesList.Get(i);

    CStdString serviceId = item->GetProperty("serviceId");

    if (serviceId == FACEBOOK_SERVICE_ID)
    {
      m_isFacebookConnected = item->GetPropertyBOOL("enable");
      m_servButtonToLinkMap[FACEBOOK_BUTTON_CONTROL_ID] = item->GetProperty("connect");
      m_servButtonToExternalLinkMap[FACEBOOK_BUTTON_CONTROL_ID] = item->GetProperty("link");
    }
    else if (serviceId == TWITTER_SERVICE_ID)
    {
      m_isTwitterConnected = item->GetPropertyBOOL("enable");
      m_servButtonToLinkMap[TWITTER_BUTTON_CONTROL_ID] = item->GetProperty("connect");
      m_servButtonToExternalLinkMap[TWITTER_BUTTON_CONTROL_ID] = item->GetProperty("link");
    }
    else if (serviceId == TUMBLR_SERVICE_ID)
    {
      m_isTumblrConnected = item->GetPropertyBOOL("enable");
      m_servButtonToLinkMap[TUMBLR_BUTTON_CONTROL_ID] = item->GetProperty("connect");
      m_servButtonToExternalLinkMap[TUMBLR_BUTTON_CONTROL_ID] = item->GetProperty("link");
    }
  }

  return true;
}

bool CGUIDialogBoxeeLoginWizardConnectSocialServices::HandleClickNext()
{
  return true;
}

bool CGUIDialogBoxeeLoginWizardConnectSocialServices::HandleClickBack()
{
  return true;
}

bool CGUIDialogBoxeeLoginWizardConnectSocialServices::IsFacebookConnected()
{
  return m_isFacebookConnected;
}

bool CGUIDialogBoxeeLoginWizardConnectSocialServices::IsTwitterConnected()
{
  return m_isTwitterConnected;
}

bool CGUIDialogBoxeeLoginWizardConnectSocialServices::IsTumblrConnected()
{
  return m_isTumblrConnected;
}

