#include "GUIDialogBoxeeLoginWizardAddNewUser.h"
#include "GUIWindowManager.h"
#include "BoxeeLoginManager.h"
#include "utils/log.h"
#include "GUIRadioButtonControl.h"
#include "GUIEditControl.h"
#include "Application.h"
#include "GUIDialogOK2.h"
#include "BoxeeLoginWizardManager.h"
#include "GUIWebDialog.h"
#include "GUIDialogProgress.h"
#include "RegExp.h"
#include "LocalizeStrings.h"
#include "md5.h"
#include "utils/Base64.h"
#include "lib/libBoxee/bxcredentials.h"
#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "lib/libBoxee/bxutils.h"

#ifdef _WIN32
#include "tools/XBMCTex/XTimeUtils.h"
#else
#include "XTimeUtils.h"
#endif

using namespace BOXEE;

#define FACEBOOK_CONNECT_BUTTON_CONTROL_ID               8700
#define NAME_EDIT_CONTROL_ID                             8701
#define EMAIL_EDIT_CONTROL_ID                            8702
#define PASSWORD_EDIT_CONTROL_ID                         8703

#define CHECK_DIALOG_FILEDS_DELAY                        15

#define USER_NAME_VALID_REGEXP_PATTERN                   "^[a-zA-Z0-9_\\.]+$"
#define MIN_USER_NAME_LENGTH                             1
#define MAX_USER_NAME_LENGTH                             45

#define EMAIL_VALID_REGEXP_PATTERN                       "^[^@]{1,64}@[\\w\\.-]{1,255}\\.[a-zA-Z]{2,}$"

#define MIN_PASSWORD_LENGTH                              6
#define MAX_PASSWORD_LENGTH                              45

#define BOXEE_XML_ELEMENT                                "boxee"
#define SUCCESS_XML_ELEMENT                              "success"
#define LOGIN_XML_ELEMENT                                "login"
#define OBJECT_XML_ELEMENT                               "object"
#define ERRORS_XML_ELEMENT                               "errors"

#define FACEBOOK_CONNECT_URL                             "http://app.boxee.tv/accountapi/connectservice?id=311"
#define FACEBOOK_CONNECT_GET_PARAMS_NUM_OF_RETRIES       1

#define FACEBOOK_PROBLE                                  "FACEBOOK_PROBLE"
#define NO_SERVICE                                       "NO_SERVICE"
#define USER_UNKNOWN                                     "USER_UNKNOWN"

CGUIDialogBoxeeLoginWizardAddNewUser::CGUIDialogBoxeeLoginWizardAddNewUser() : CGUIDialogBoxeeWizardBase(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_ADD_NEW_USER,"boxee_login_wizard_add_new_user.xml","CGUIDialogBoxeeLoginWizardAddNewUser")
{
  m_checkNameFieldDelay = 0;
  m_checkEmailDelay = 0;
  m_checkPasswordDelay = 0;

  m_validNameStr = "";
  m_isNameValid = false;

  m_validEmailStr = "";
  m_isEmailValid = false;

  m_validPasswordStr = "";
  m_isPasswordValid = false;
}

CGUIDialogBoxeeLoginWizardAddNewUser::~CGUIDialogBoxeeLoginWizardAddNewUser()
{

}

void CGUIDialogBoxeeLoginWizardAddNewUser::OnInitWindow()
{
  CGUIDialogBoxeeWizardBase::OnInitWindow();

  SetProperty("name-is-valid", false);
  SetProperty("email-is-valid", false);
  SetProperty("password-is-valid", false);

  m_checkNameFieldDelay = 0;
  m_checkEmailDelay = 0;
  m_checkPasswordDelay = 0;

  m_validNameStr = "";
  m_isNameValid = false;

  m_validEmailStr = "";
  m_isEmailValid = false;

  m_validPasswordStr = "";
  m_isPasswordValid = false;
}

bool CGUIDialogBoxeeLoginWizardAddNewUser::OnAction(const CAction& action)
{
  return CGUIDialogBoxeeWizardBase::OnAction(action);
}

bool CGUIDialogBoxeeLoginWizardAddNewUser::OnMessage(CGUIMessage& message)
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

bool CGUIDialogBoxeeLoginWizardAddNewUser::HandleClick(CGUIMessage& message)
{
  int iControl = message.GetSenderId();

  switch(iControl)
  {
  case FACEBOOK_CONNECT_BUTTON_CONTROL_ID:
  {
#ifdef CANMORE
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddNewUser::HandleClick - handling click on [Control=%d=FACEBOOK_CONNECT_BUTTON_CONTROL_ID] (blw)(digwiz)",iControl);
    if (!OnCreateUserThruFacebook())
    {
      // ERROR log will be written in OnCreateUser
      return true;
    }
#else
    CStdString text = g_localizeStrings.Get(80004);
    CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(10014),text);
#endif
  }
  break;
  case DIALOG_WIZARD_BUTTON_NEXT:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddNewUser::HandleClick - handling click on [Control=%d=DIALOG_WIZARD_BUTTON_NEXT] (blw)(digwiz)",iControl);
    if (!OnCreateUser())
    {
      // ERROR log will be written in OnCreateUser
      return true;
    }
  }
  break;
  }

  return false;
}

bool CGUIDialogBoxeeLoginWizardAddNewUser::OnCreateUserThruFacebook()
{
  //CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddNewUser::OnCreateUserThruFacebook - NOT IMPLEMENTED (blw)(cnu)");
  //return false;

  /*
  // erez - FOR TESTING !!! //
  {
    CStdString strJson = "{\"error_code\":400,\"error_msg\":\"USER_UNKNOWN\"}";
    Json::Reader reader;
    Json::Value jResponse;

    CStdString errorCode;
    CStdString errorMsg;

    if (!reader.parse(strJson, jResponse))
    {
      printf("OnCreateUserThruFacebook - FAILED to parse json");
      return false;
    }

    Json::Value::Members keys = jResponse.getMemberNames();
    for (size_t i = 0; i < keys.size(); i++)
    {
      printf("[%zu/%zu] - [%s=%s]\n",i+1,keys.size(),keys[i].c_str(),jResponse.get(keys[i],"WTF").asString().c_str());
    }

    if (!jResponse.isMember("error_code"))
    {
      printf("FAILED to find [error_code] in request\n");
    }
    else
    {
      Json::Value val(jResponse.get("error_code",""));
      int errorCode = jResponse["error_code"].asInt();
    }

    if (!jResponse.isMember("error_msg"))
    {
      printf("FAILED to find [error_msg] in request\n");
    }
    else
    {
      errorMsg = jResponse["error_msg"].asString();
    }

    printf("[error_code=%s][error_msg=%s]\n",errorCode.c_str(),errorMsg.c_str());

    return false;
  }
  /////////////////////////////
  */

  CBoxeeLoginWizardManager::GetInstance().GetServerUserInfoByRef().Reset();

  if (!CGUIWebDialog::ShowAndGetInput(FACEBOOK_CONNECT_URL))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::OnCreateUserThruFacebook - FAILED to create new user thru facebook (blw)(cnu)");
    CGUIDialogOK2::ShowAndGetInput(53701, 53455);
    return false;
  }

  if (!GetSocialServiceParams(FACEBOOK_SERVICE_ID))
  {
    // ERROR log will be written in GetFacebookServiceParams
    CGUIDialogOK2::ShowAndGetInput(53701, 53455);
    return false;
  }

  ////////////////////////////////////////////////////
  // got user data from server -> set dialog fields //
  ////////////////////////////////////////////////////

  CServerUserInfo serverUserInfo = CBoxeeLoginWizardManager::GetInstance().GetServerUserInfo();

  CGUIEditControl* pNameEditControl = (CGUIEditControl*)GetControl(NAME_EDIT_CONTROL_ID);
  if (!pNameEditControl)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::OnCreateUserThruFacebook - FAILED to get NAME control (blw)(cnu)");
    CGUIDialogOK2::ShowAndGetInput(53701, 53455);
    return false;
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddNewUser::OnCreateUserThruFacebook - going to set [name=%s] (blw)(cnu)",serverUserInfo.m_name.c_str());
  pNameEditControl->SetLabel2(serverUserInfo.m_name);

  CGUIEditControl* pEmailEditControl = (CGUIEditControl*)GetControl(EMAIL_EDIT_CONTROL_ID);
  if (!pEmailEditControl)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::OnCreateUserThruFacebook - FAILED to get EMAIL control (blw)(cnu)");
    CGUIDialogOK2::ShowAndGetInput(53701, 53455);
    return false;
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddNewUser::OnCreateUserThruFacebook - going to set [email=%s] (blw)(cnu)",serverUserInfo.m_email.c_str());
  pEmailEditControl->SetLabel2(serverUserInfo.m_email);

  // tmp !!!
  CBoxeeLoginWizardManager::GetInstance().GetServerUserInfoByRef().m_serviceId = FACEBOOK_SERVICE_ID;

  return true;
}

bool CGUIDialogBoxeeLoginWizardAddNewUser::GetSocialServiceParams(const CStdString& serviceId)
{
  CStdString strUrl = BOXEE::BXConfiguration::GetInstance().GetURLParam("Boxee.GetServiceParams","http://app.boxee.tv/accountapi/getserviceparams");
  strUrl += "?id=";
  strUrl += serviceId;

  Json::Value jResponse;
  int returnCode;

  int numOfRetries = FACEBOOK_CONNECT_GET_PARAMS_NUM_OF_RETRIES;
  bool isSucceeded = BOXEE::BXUtils::PerformJSONGetRequest(strUrl,jResponse,returnCode);

  /*
  // erez - FOR TESTING !!! //
  CStdString strJson = "{\"id\": \"100000358208233\",\"name\":\"Cookie Monro\",\"first_name\":\"Cookie\",\"last_name\":\"Monro\",\"link\":\"http:\\/\\/www.facebook.com\\/cookie.monro\",\"username\":\"cookie.monro\",\"birthday\":\"01\\/01\\/1981\",\"gender\":\"male\",\"email\":\"liel@boxee.tv\",\"timezone\":-4,\"locale\":\"en_US\",\"verified\":true,\"updated_time\":\"2011-10-03T15:46:24+0000\",\"access_token\":\"AAAAAKROdCKMBAOhxTZCDv1UeYU6ypWNQBni4K0YXuVWJQSxuuBLJo0psrkHuLtUOYwMynZBdH8TZCTU0FH47WmKRNyevpHEH2EHTMeLogZDZD\",\"code\":\"AQBGjcz-34WbyiN8CVVVd9twgg7JWbofgzdiUrGRHRY_-WTLA2EHcOktxsCiMtd8fn4vljSW13wSUt7zMlukrYL4CzEhH0DVtnIxSyFuxn-M7qtZaTmvf4KTGhN1212YrhDK3RARRc-4SeE7R9LbKiAksViRlPPvCGoSH6eM9PYMcQ9u0Jqwt74yQIv6euI1jfc\"}";
  Json::Reader reader;

  if (!reader.parse(strJson, jResponse))
  {
    printf("OnCreateUserThruFacebook - FAILED to parse json");
    return false;
  }

  bool isSucceeded = true;
  /////////////////////////////
  */

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddNewUser::GetSocialServiceParams - call PerformJSONGetRequest returned [isSucceeded=%d][returnCode=%d] . [serviceId=%s] (blw)(cnu)",isSucceeded,returnCode,serviceId.c_str());

  while (!isSucceeded)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::GetSocialServiceParams - FAILED to get service params. [serviceId=%s] (blw)(cnu)",serviceId.c_str());

    if (!jResponse.isMember("error_code"))
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::GetSocialServiceParams - FAILED to get [error_code] parameter from json response. [serviceId=%s] (blw)(cnu)",serviceId.c_str());
      return false;
    }

    int errorCode = jResponse["error_code"].asInt();

    if (errorCode != 400)
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::GetSocialServiceParams - FAILED to handle [error_code=%d]. [serviceId=%s] (blw)(cnu)",errorCode,serviceId.c_str());
      return false;
    }

    if (!jResponse.isMember("error_msg"))
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::GetSocialServiceParams - FAILED to get [error_msg] parameter from json response. [serviceId=%s] (blw)(cnu)",serviceId.c_str());
      return false;
    }

    CStdString errorMsg = jResponse["error_msg"].asString();

    if (errorMsg != FACEBOOK_PROBLE || (numOfRetries == 0))
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::GetSocialServiceParams - FAILED to get service params [error_msg=%s]. [serviceId=%s] (blw)(cnu)",errorMsg.c_str(),serviceId.c_str());
      return false;
    }

    /////////////////////////////////////////////////////////
    // received errorMsg FACEBOOK_PROBLE -> do another try //
    /////////////////////////////////////////////////////////

    numOfRetries--;
    isSucceeded = BOXEE::BXUtils::PerformJSONGetRequest(strUrl,jResponse,returnCode);
  }

  if (!ParseSocialServiceParams(serviceId,jResponse))
  {
    // ERROR log will be written in ParseSocialServiceParams
    return false;
  }

  return true;
}

bool CGUIDialogBoxeeLoginWizardAddNewUser::ParseSocialServiceParams(const CStdString& serviceId, const Json::Value& jResponse)
{
  Json::Value::Members keys = jResponse.getMemberNames();

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddNewUser::ParseSocialServiceParams - enter function. going to parse jResponse with [keysSize=%zu]. [serviceId=%s] (blw)(cnu)",keys.size(),serviceId.c_str());

  if (!CBoxeeLoginWizardManager::GetInstance().GetServerUserInfoByRef().InitializeByJson(jResponse))
  {
    // ERROR log will be written in InitializeByJson
    return false;
  }

  return true;
}

bool CGUIDialogBoxeeLoginWizardAddNewUser::SetDialogFields()
{


  return true;
}

bool CGUIDialogBoxeeLoginWizardAddNewUser::OnCreateUser()
{
  if (!CanExecuteLogin())
  {
    // ERROR log will be written in CanExecuteLogin
    return false;
  }

  CGUIDialogProgress* progress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (progress)
  {
    progress->StartModal();
    progress->Progress();
  }

  int profileId = -1;
  bool isLogin = false;

  CStdString strUrl = BXConfiguration::GetInstance().GetStringParam("Boxee.Create.User","https://secure.boxee.tv/api/register");

  ListHttpHeaders headers;
  headers.push_back("Content-Type: text/xml");

  BXXMLDocument doc;
  doc.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());

  CStdString postData;
  BuildCreateNewUserOnLoginPostData(postData);

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddNewUser::OnCreateUser - Going to create new user. [postData=%s] (blw)(cnu)",postData.c_str());

  bool isSucceeded = false;
  isSucceeded = doc.LoadFromURL(strUrl,headers,postData);

  if (progress)
  {
    progress->Close();
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddNewUser::OnCreateUser - Create new user returned [isSucceeded=%d][RetCode=%ld] (blw)(cnu)",isSucceeded,doc.GetLastRetCode());

  if (!isSucceeded)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::OnCreateUser - FAILED to create new user [username=%s]. Request from server returned FALSE (blw)(cnu)",m_validNameStr.c_str());
    CGUIDialogOK2::ShowAndGetInput(53701, 53455);
    return false;
  }

  BXObject newUserObj(false);
  CStdString errorMessage = "";

  if (!ParseServerCreatedNewUserResponse(doc, isLogin, newUserObj, errorMessage))
  {
    if (errorMessage.IsEmpty())
    {
      CGUIDialogOK2::ShowAndGetInput(53701, 53456);
    }
    else
    {
      CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701), errorMessage);
    }

    return false;
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddNewUser::OnCreateUser - New user [username=%s] was created on server. create local profile (blw)(cnu)",m_validNameStr.c_str());

  if (CBoxeeLoginWizardManager::GetInstance().GetServerUserInfoByRef().IsInitialize())
  {
    CServerUserInfo serverUserInfo = CBoxeeLoginWizardManager::GetInstance().GetServerUserInfo();

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddNewUser::OnCreateUser - New user [username=%s] was created thru social service [%s] -> update service (blw)(cnu)",m_validNameStr.c_str(),serverUserInfo.m_serviceId.c_str());

    CStdString strUrl = BXConfiguration::GetInstance().GetStringParam("Boxee.Update.Service.User","http://app.boxee.tv/accountapi/updateService");

    ListHttpHeaders headers;
    //headers.push_back("Content-Type: text/xml");

    BXXMLDocument doc;
    doc.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());

    CStdString postData = "bx_action=upd&service=";
    postData += serverUserInfo.m_serviceId;
    postData += "&format=json&access_token=";
    postData += serverUserInfo.m_accessToken;
    postData += "&user_id=";
    postData += serverUserInfo.m_username;
    postData += "&secret_skip=";
    postData += "S0M3S3c437";

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddNewUser::OnCreateUser - going to update new user service [id=%s]. [postData=%s] (blw)(cnu)",serverUserInfo.m_serviceId.c_str(),postData.c_str());

    bool isSucceeded = false;
    isSucceeded = doc.LoadFromURL(strUrl,headers,postData);

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddNewUser::OnCreateUser - update new user service [id=%s] returned [isSucceeded=%d][RetCode=%ld] (blw)(cnu)",serverUserInfo.m_serviceId.c_str(),isSucceeded,doc.GetLastRetCode());

    if (!isSucceeded)
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::OnCreateUser - FAILED to update new user [username=%s] with service [%s]. Request from server returned FALSE (blw)(cnu)",m_validNameStr.c_str(),serverUserInfo.m_serviceId.c_str());
      //CGUIDialogOK2::ShowAndGetInput(53701, 53472);
      //return false;
    }
  }

  if (!CreateNewUserProfile(newUserObj, profileId))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::OnCreateUser - FAILED to create profile for new user [username=%s] (blw)(cnu)",m_validNameStr.c_str());
    CGUIDialogOK2::ShowAndGetInput(53701, 53456);
    return false;
  }

  CProfile& p = g_settings.m_vecProfiles[profileId];

  BXCredentials credentials;
  credentials.SetUserName(p.getName());
  credentials.SetPassword(p.getLastLockCode());

  BOXEE::Boxee::GetInstance().PostLoginInitializations(credentials,newUserObj);

  if (isLogin)
  {
    g_application.SetOfflineMode(false);
  }

  g_application.GetBoxeeLoginManager().FinishSuccessfulLogin();

  return true;
}

void CGUIDialogBoxeeLoginWizardAddNewUser::BuildCreateNewUserOnLoginPostData(CStdString& postData)
{
  postData = "<boxee><register>";
  postData += "<email>";
  postData += m_validEmailStr;
  postData += "</email>";

  postData += "<password>";
  CStdString validPasswordStr = m_validPasswordStr;
  validPasswordStr.Replace("&","&amp;");
  validPasswordStr.Replace("\"","&quot;");
  validPasswordStr.Replace("'","&apos;");
  validPasswordStr.Replace(">","&gt;");
  validPasswordStr.Replace("<","&lt;");
  validPasswordStr.Replace(" ","&nbsp;");
  postData += validPasswordStr;
  postData += "</password>";

  postData += "<username>";
  postData += m_validNameStr;
  postData += "</username>";

  CStdString firstName = m_validNameStr;
  CStdString lastName = "";

  size_t spacePos = m_validNameStr.find(" ");
  if (spacePos != std::string::npos)
  {
    firstName = m_validNameStr.substr(0,spacePos);
    lastName = m_validNameStr.substr(spacePos+1);
  }

  postData += "<first_name>";
  postData += firstName;
  postData += "</first_name>";

  postData += "<last_name>";
  postData += lastName;
  postData += "</last_name>";

  postData += "</register></boxee>";
}

bool CGUIDialogBoxeeLoginWizardAddNewUser::ParseServerCreatedNewUserResponse(BOXEE::BXXMLDocument& doc, bool& isLogin, BOXEE::BXObject& newUserObj, CStdString& errorMessage)
{
  const TiXmlElement* pRootElement = doc.GetDocument().RootElement();
  if (!pRootElement)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::ParseServerCreatedNewUserResponse - FAILED to get root element (blw)(cnu)");
    return false;
  }

  if (strcmp(pRootElement->Value(),BOXEE_XML_ELEMENT) != 0)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::ParseServerCreatedNewUserResponse - Root element isn't <boxee>. <%s> (blw)(cnu)",pRootElement->Value());
    return false;
  }

  bool foundSucceessElement = false;
  bool isSucceess = false;

  const TiXmlNode* pTag = NULL;
  while ((pTag = pRootElement->IterateChildren(pTag)))
  {
    CStdString elementName = pTag->ValueStr();

    if (elementName == SUCCESS_XML_ELEMENT)
    {
      ///////////////////////
      // case of <success> //
      ///////////////////////

      foundSucceessElement = true;

      const TiXmlNode* pValue = pTag->FirstChild();

      if (pValue)
      {
        CStdString succeessValue = pValue->ValueStr();

        if (succeessValue.Equals("1"))
        {
          CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddNewUser::ParseServerCreatedNewUserResponse - Server returned <success=%s=1> for [SelectedUsername=%s] (blw)(cnu)",succeessValue.c_str(),m_validNameStr.c_str());
          isSucceess = true;
        }
        else
        {
          CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::ParseServerCreatedNewUserResponse - Server returned <success=%s> for [SelectedUsername=%s] (blw)(cnu)",succeessValue.c_str(),m_validNameStr.c_str());
        }
      }
      else
      {
        CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::ParseServerCreatedNewUserResponse - FAILED to get <success> element value. [SelectedUsername=%s] (blw)(cnu)",m_validNameStr.c_str());
      }
    }
    else if (elementName == LOGIN_XML_ELEMENT)
    {
      /////////////////////
      // case of <login> //
      /////////////////////

      if (!foundSucceessElement)
      {
        CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::ParseServerCreatedNewUserResponse - There wasn't <success> before <login> for [SelectedUsername=%s] (blw)(cnu)",m_validNameStr.c_str());
        return false;
      }

      const TiXmlNode* pValue = pTag->FirstChild();

      if (pValue)
      {
        CStdString loginValue = pValue->ValueStr();

        if (loginValue.Equals("1"))
        {
          CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddNewUser::ParseServerCreatedNewUserResponse - Server returned <login=%s=1> for [SelectedUsername=%s] (blw)(cnu)",loginValue.c_str(),m_validNameStr.c_str());
          isLogin = true;
        }
        else
        {
          CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddNewUser::ParseServerCreatedNewUserResponse - Server returned <login=%s> for [SelectedUsername=%s] (blw)(cnu)",loginValue.c_str(),m_validNameStr.c_str());
        }
      }
      else
      {
        CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::ParseServerCreatedNewUserResponse - FAILED to get <login> element value. [SelectedUsername=%s] (blw)(cnu)",m_validNameStr.c_str());
        return false;
      }
    }
    else if (elementName == OBJECT_XML_ELEMENT)
    {
      ///////////////////////
      // case of <object> //
      ///////////////////////

      if (!isSucceess)
      {
        CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::ParseServerCreatedNewUserResponse - <object> element won't be parse because server FAILED to create new user. [SelectedUsername=%s] (blw)(cnu)",m_validNameStr.c_str());
        return false;
      }

      newUserObj.FromXML(pTag);
      newUserObj.SetValid(true);
    }
    else if (elementName == ERRORS_XML_ELEMENT)
    {
      ///////////////////////
      // case of <errors> //
      ///////////////////////

      if (isSucceess)
      {
        CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::ParseServerCreatedNewUserResponse - Received <errors> element while <success=%d>. [SelectedUsername=%s] (blw)(cnu)",isSucceess,m_validNameStr.c_str());
        return false;
      }

      const TiXmlNode* pErrorTag = NULL;
      while ((pErrorTag = pTag->IterateChildren(pErrorTag)))
      {
        const TiXmlNode* pErrorValue = pErrorTag->FirstChild();

        if (pErrorValue)
        {
          errorMessage = pErrorValue->ValueStr();
          CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::ParseServerCreatedNewUserResponse - Parse error message [%s]. [SelectedUsername=%s] (blw)(cnu)",errorMessage.c_str(),m_validNameStr.c_str());
          return false;
        }
      }

      CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::ParseServerCreatedNewUserResponse - FAILED to parse any error message [%s]. [SelectedUsername=%s] (blw)(cnu)",errorMessage.c_str(),m_validNameStr.c_str());
      return false;
    }
  }

  return true;
}

bool CGUIDialogBoxeeLoginWizardAddNewUser::CreateNewUserProfile(const BXObject& newUserObj, int& profileId)
{
  if (!newUserObj.IsValid())
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::CreateNewUserProfile - Enter function with a INVALID user object. [username=%s] (cnu)",m_validNameStr.c_str());
    return false;
  }

  profileId = g_application.GetBoxeeLoginManager().CreateProfile(newUserObj.GetID(), newUserObj);

  if (profileId > 0)
  {
    g_settings.LoadProfile(profileId);

    CStdString password = m_validPasswordStr;
    password = CBoxeeLoginManager::EncodePassword(password);
    g_application.GetBoxeeLoginManager().UpdateProfile(profileId,password,true);

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddNewUser::CreateNewUserProfile - After create profile for [username=%s]. [profileId=%d] (cnu)",m_validNameStr.c_str(),profileId);

    return true;
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::CreateNewUserProfile - FAILED to create profile for [username=%s]. [profileId=%d] (cnu)",m_validNameStr.c_str(),profileId);
    return false;
  }
}

bool CGUIDialogBoxeeLoginWizardAddNewUser::CanExecuteLogin()
{
  const CStdString username = ((CGUIEditControl*)GetControl(NAME_EDIT_CONTROL_ID))->GetLabel2();
  if (CBoxeeLoginManager::DoesThisUserAlreadyExist(username))
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardAddNewUser::CanExecuteLogin - user [%s] already exist on this machine (blw)",username.c_str());
    CGUIDialogOK2::ShowAndGetInput(20068,51603);
    return false;
  }

  bool isConnectedToInternet = g_application.IsConnectedToInternet();
  if(!isConnectedToInternet)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardAddNewUser::CanExecuteLogin - can't add user since [isConnectedToInternet=%d] (blw)",isConnectedToInternet);
    CGUIDialogOK2::ShowAndGetInput(53743, 53744);
    return false;
  }

  return true;
}

void CGUIDialogBoxeeLoginWizardAddNewUser::Render()
{
  checkDialogFields();

  CGUIDialogBoxeeWizardBase::Render();
}

bool CGUIDialogBoxeeLoginWizardAddNewUser::HandleClickNext()
{
  return true;
}

bool CGUIDialogBoxeeLoginWizardAddNewUser::HandleClickBack()
{
  return true;
}

void CGUIDialogBoxeeLoginWizardAddNewUser::checkDialogFields()
{
  checkNameField();
  checkEmailField();
  checkPasswordField();
}

void CGUIDialogBoxeeLoginWizardAddNewUser::checkNameField()
{
  m_checkNameFieldDelay++;

  if (m_checkNameFieldDelay > CHECK_DIALOG_FILEDS_DELAY)
  {
    m_checkNameFieldDelay = 0;

    CGUIEditControl* pNameEditControl = (CGUIEditControl*)GetControl(NAME_EDIT_CONTROL_ID);
    if (pNameEditControl)
    {
      CStdString name = pNameEditControl->GetLabel2();

      if (!m_validNameStr.Equals(name))
      {
        if (!name.IsEmpty() && IsNameValid(name))
        {
          m_validNameStr = name;
          m_isNameValid = true;
          SetProperty("name-is-valid", true);

          /*
          if (m_isNameValid && m_isEmailValid && m_isPasswordValid)
          {
            CONTROL_ENABLE(DIALOG_WIZARD_BUTTON_NEXT);
          }
          */
        }
        else
        {
          if (m_isNameValid)
          {
            // name valid status was change

            m_validNameStr = "";
            m_isNameValid = false;
            SetProperty("name-is-valid", false);
          }
        }
      }
    }
  }
}

bool CGUIDialogBoxeeLoginWizardAddNewUser::IsNameValid(const CStdString& username)
{
  if (username.length() > MAX_USER_NAME_LENGTH)
  {
    return false;
  }

  CStdString strToCheck = username;
  strToCheck.ToLower();

  CStdString strRegExpr = USER_NAME_VALID_REGEXP_PATTERN;

  CRegExp reg;
  reg.RegComp(strRegExpr);
  int pos = reg.RegFind(strToCheck);

  if (pos != -1)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void CGUIDialogBoxeeLoginWizardAddNewUser::checkEmailField()
{
  if (m_isNameValid)
  {
    m_checkEmailDelay++;

    if (m_checkEmailDelay > CHECK_DIALOG_FILEDS_DELAY)
    {
      m_checkEmailDelay = 0;

      CGUIEditControl* pEmailEditControl = (CGUIEditControl*)GetControl(EMAIL_EDIT_CONTROL_ID);
      if (pEmailEditControl)
      {
        CStdString email = pEmailEditControl->GetLabel2();

        if (!m_validEmailStr.Equals(email))
        {
          if (!email.IsEmpty() && IsEmailValid(email))
          {
            m_validEmailStr = email;
            m_isEmailValid = true;
            SetProperty("email-is-valid", true);

            /*
            if (m_isNameValid && m_isEmailValid && m_isPasswordValid)
            {
              CONTROL_ENABLE(DIALOG_WIZARD_BUTTON_NEXT);
            }
            */
          }
          else
          {
            if (m_isEmailValid)
            {
              // email valid status was change

              m_validEmailStr = "";
              m_isEmailValid = false;
              SetProperty("email-is-valid", false);
            }
          }
        }
      }
    }
  }
}

bool CGUIDialogBoxeeLoginWizardAddNewUser::IsEmailValid(const CStdString& email)
{
  if (email.length() > 45)
  {
    return false;
  }

  //CStdString strToCheck = "*" + email + "*";
  CStdString strToCheck = email;
  strToCheck.ToLower();

  CStdString strRegExpr = EMAIL_VALID_REGEXP_PATTERN;

  CRegExp reg;
  reg.RegComp(strRegExpr);
  int pos = reg.RegFind(strToCheck);

  if (pos != -1)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void CGUIDialogBoxeeLoginWizardAddNewUser::checkPasswordField()
{
  if (m_isNameValid && m_isEmailValid)
  {
    m_checkPasswordDelay++;

    if (m_checkPasswordDelay > CHECK_DIALOG_FILEDS_DELAY)
    {
      m_checkPasswordDelay = 0;

      CGUIEditControl* pPasswordEditControl = (CGUIEditControl*)GetControl(PASSWORD_EDIT_CONTROL_ID);
      if (pPasswordEditControl)
      {
        CStdString password = pPasswordEditControl->GetLabel2();

        if (!m_validPasswordStr.Equals(password))
        {
          if (!password.IsEmpty() && IsPasswordValid(password))
          {
            m_validPasswordStr = password;
            m_isPasswordValid = true;
            SetProperty("password-is-valid", true);
          }
          else
          {
            if (m_isPasswordValid)
            {
              m_validPasswordStr = "";
              m_isPasswordValid = false;
              SetProperty("password-is-valid", false);
            }
          }
        }
      }
    }
  }
}

bool CGUIDialogBoxeeLoginWizardAddNewUser::IsPasswordValid(const CStdString& password)
{
  if (password.length() < MIN_PASSWORD_LENGTH || password.length() > MAX_PASSWORD_LENGTH)
  {
    return false;
  }

  return true;
}

CStdString CGUIDialogBoxeeLoginWizardAddNewUser::GetUserName()
{
  return m_validNameStr;
}

CStdString CGUIDialogBoxeeLoginWizardAddNewUser::GetEmail()
{
  return m_validEmailStr;
}

