
#include "GUIDialogBoxeePaymentUserData.h"
#include "GUIWindowManager.h"
#include "log.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "lib/libBoxee/boxee.h"
#include "LocalizeStrings.h"
#include "GUIEditControl.h"
#include "GUIRadioButtonControl.h"
#include "GUIDialogOK2.h"
#include "lib/libBoxee/boxee.h"
#include "FileCurl.h"
#include "GUIDialogBoxeePaymentWaitForServerApproval.h"
#include "utils/md5.h"
#include "Util.h"

#define HIDDEN_CONTAINER                5000
#define CONTROL_TITLE_LABEL             6310
#define CONTROL_PASSWORD_EDIT           6320
#define GO_ON_BUTTON                    6331

#define MIN_PAYMENT_SERVER_ERROR_CODE   81000
#define MAX_PAYMENT_SERVER_ERROR_CODE   81099

CGUIDialogBoxeePaymentUserData::CGUIDialogBoxeePaymentUserData(void) : CGUIDialog(WINDOW_DIALOG_BOXEE_PAYMENT_USERDATA, "boxee_payment_userdata.xml")
{
  m_bConfirmed = false;
  m_password = "";
  m_quitPaymentProcess = false;
}

CGUIDialogBoxeePaymentUserData::~CGUIDialogBoxeePaymentUserData()
{

}

void CGUIDialogBoxeePaymentUserData::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  m_quitPaymentProcess = false;

  m_password = "";
  m_nonceStr = "";

  CGUIEditControl* passwordControl = (CGUIEditControl*)GetControl(CONTROL_PASSWORD_EDIT);
  if (!passwordControl)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::OnInitWindow - FAILED to get password control (pay)");
    Close();
    return;
  }

  passwordControl->SetLabel2(m_password);

  SET_CONTROL_FOCUS(CONTROL_PASSWORD_EDIT, 0);
}

void CGUIDialogBoxeePaymentUserData::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
}

bool CGUIDialogBoxeePaymentUserData::OnAction(const CAction& action)
{
  switch (action.id)
  {
  case ACTION_PARENT_DIR:
  case ACTION_PREVIOUS_MENU:
  {

    m_bConfirmed = false;
    Close();
    return true;
  }
  break;
  default:
  {
    // do nothing
  }
  break;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeePaymentUserData::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    return OnClick(message);
  }
  break;
  default:
  {
    // do nothing
  }
  break;
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogBoxeePaymentUserData::OnClick(CGUIMessage& message)
{
  int iControl = message.GetSenderId();

  //CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::OnClick - Enter function with [iControl=%d] (pay)",iControl);

  switch(iControl)
  {
  case GO_ON_BUTTON:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::OnClick - Handling click on [%d=GO_ON_BUTTON] (pay)",iControl);
    if (HandleClickOnGoOn())
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::OnClick - Call to HandleClickOnGoOn() returned TRUE -> going to close dialog (pay)");
      m_bConfirmed = true;
      Close();
      return true;
    }
    else
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::OnClick - Call to HandleClickOnGoOn() returned FALSE (pay)");
      return true;
    }
  }
  break;
  case CONTROL_PASSWORD_EDIT:
  {
    return true;
  }
  break;
  default:
  {
    CLog::Log(LOGWARNING,"CGUIDialogBoxeePaymentUserData::OnClick - UNKNOWN control [%d] was click (pay)",iControl);
    return true;
  }
  break;
  }
}

bool CGUIDialogBoxeePaymentUserData::HandleClickOnGoOn()
{
  CGUIEditControl* passwordControl = (CGUIEditControl*)GetControl(CONTROL_PASSWORD_EDIT);
  if (!passwordControl)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::HandleClickOnGoOn - FAILED to get password control (pay)");
    CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(55195));
    return false;
  }

  m_password = passwordControl->GetLabel2();

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::HandleClickOnGoOn - After set [password=%s] (pay)",m_password.c_str());

  if (m_password.IsEmpty())
  {
    CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(55131),g_localizeStrings.Get(55132));
    return false;
  }
  else
  {
    bool transactionSucceeded = HandleTransactionWithServer();
    if (transactionSucceeded || m_quitPaymentProcess)
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::HandleClickOnGoOn - Call to HandleTransactionWithServer() returned [%d]. Return TRUE. [quitPaymentProcess=%d] (pay)",transactionSucceeded,m_quitPaymentProcess);
      return true;
    }
    else
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::HandleClickOnGoOn - Call to HandleTransactionWithServer() returned [%d]. Return FALSE. [quitPaymentProcess=%d] (pay)",transactionSucceeded,m_quitPaymentProcess);
      return false;
    }
  }
}

bool CGUIDialogBoxeePaymentUserData::HandleTransactionWithServer()
{
  CStdString url = "http://app.boxee.tv/product/";
  url += m_productId;
  url += "/subscribe";

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::HandleTransactionWithServer - Call GetNonceFromServer() with [url=%s] (pay)",url.c_str());

  if (!GetNonceFromServer(url))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::HandleTransactionWithServer - FAILED to get nonce from server (pay)");

    if (!m_quitPaymentProcess)
    {
      CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(55195));
    }

    return false;
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::HandleTransactionWithServer - Call GetNonceFromServer() with [url=%s] returned TRUE (pay)",url.c_str());

  CStdString signature = m_productId;

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::HandleTransactionWithServer - 1 - [signature=%s]. [productId=%s] (pay)",signature.c_str(),m_productId.c_str());

  signature += m_nonceStr;

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::HandleTransactionWithServer - 2 - [signature=%s]. [nonceStr=%s] (pay)",signature.c_str(),m_nonceStr.c_str());

  CStdString temp = BOXEE::Boxee::GetInstance().GetCredentials().GetUserName();
  temp += ":boxee:";
  temp += m_password;
  CStdString passwordMD5;
  XBMC::MD5 md5ForPassword;
  md5ForPassword.append(temp);
  md5ForPassword.getDigest(passwordMD5);
  passwordMD5.ToLower();
  signature += passwordMD5;

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::HandleTransactionWithServer - 3 - [signature=%s]. [password=%s][passwordMD5=%s] (pay)",signature.c_str(),m_password.c_str(),passwordMD5.c_str());

  CStdString signatureMD5;
  XBMC::MD5 md5ForSignature;
  md5ForSignature.append(signature);
  md5ForSignature.getDigest(signatureMD5);
  signatureMD5.ToLower();

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::HandleTransactionWithServer - 4 - [signature=%s][signatureMD5=%s] (pay)",signature.c_str(),signatureMD5.c_str());

  CStdString strPostData = "signature=";
  strPostData += signatureMD5;

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::HandleTransactionWithServer - Going to do transaction to url [%s] with data [%s] (pay)",url.c_str(),strPostData.c_str());

  PostRequestJob* pJob = new PostRequestJob(url,strPostData);
  if (CUtil::RunInBG(pJob,false) != JOB_SUCCEEDED)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::HandleTransactionWithServer - FAILED to finish transaction action. Job returned FALSE (pay)");
    CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(55135));
    return false;
  }

  CStdString strResp = pJob->GetRespose();
  long retCode = pJob->GetRetCode();

  delete pJob;

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::HandleTransactionWithServer - For [%s] got response length [%d]. [RetCode=%ld] (pay)",url.c_str(),(int)strResp.size(),retCode);

  if (retCode != 200)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::HandleTransactionWithServer - FAILED to finish transaction action (pay)");
    CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(55135));
    return false;
  }
  else
  {
    BOXEE::BXXMLDocument xmlDoc;
    if (!xmlDoc.LoadFromString(strResp))
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::HandleTransactionWithServer - FAILED to load server response [%s]. [url=%s][IsRespEmpty=%d] (pay)",strResp.c_str(),url.c_str(),strResp.IsEmpty());
      CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(55135));
      return false;
    }

    TiXmlElement* pRootElement = xmlDoc.GetDocument().RootElement();
    if (!pRootElement)
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::HandleTransactionWithServer - FAILED to get RootElement. [url=%s] (pay)",url.c_str());
      CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(55135));
      return false;
    }

    if (strcmpi(pRootElement->Value(),"response") != 0)
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::HandleTransactionWithServer - Root element is NOT <response>. [<%s>][url=%s] (pay)",pRootElement->Value(),url.c_str());
      CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(55135));
      return false;
}

    TiXmlElement* pErrorCodeElement = pRootElement->FirstChildElement("errorCode");
    if (pErrorCodeElement)
    {
      CStdString errorCode = pErrorCodeElement->GetText();

      CStdString errorMsg = "";
      TiXmlElement* pErrorMsgElement = pRootElement->FirstChildElement("errorMsg");
      if (pErrorMsgElement)
      {
        errorMsg = pErrorMsgElement->GetText();
        CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::HandleTransactionWithServer - Server response contain an ERROR. [ErrorCode=%s][ErrorMsg=%s] (pay)",errorCode.c_str(),errorMsg.c_str());
      }
      else
      {
        CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::HandleTransactionWithServer - Server response contain an ERROR. [ErrorCode=%s] (pay)",errorCode.c_str());
      }

      CStdString errorCodeStr = g_localizeStrings.Get(81000);

      CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::HandleTransactionWithServer - Set to general error [errorCodeStr=%s]. [ErrorCode=%s][ErrorMsg=%s] (pay)",errorCodeStr.c_str(),errorCode.c_str(),errorMsg.c_str());

      long lErrorCode = atol(errorCode.c_str());

      CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::HandleTransactionWithServer - [str=%s] -> [long=%ld]. [errorCodeStr=%s][ErrorCode=%s][ErrorMsg=%s] (pay)",errorCode.c_str(),lErrorCode,errorCodeStr.c_str(),errorCode.c_str(),errorMsg.c_str());

      if (lErrorCode >= MIN_PAYMENT_SERVER_ERROR_CODE || lErrorCode <= MAX_PAYMENT_SERVER_ERROR_CODE)
      {
        errorCodeStr = g_localizeStrings.Get(lErrorCode);

        CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::HandleTransactionWithServer - [str=%s] -> [long=%ld]. [errorCodeStr=%s][ErrorCode=%s][ErrorMsg=%s] (pay)",errorCode.c_str(),lErrorCode,errorCodeStr.c_str(),errorCode.c_str(),errorMsg.c_str());
      }

      if (errorCodeStr.IsEmpty())
      {
        CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::HandleTransactionWithServer - FAILED to get string error for [ErrorCode=%s]. Set a general error (pay)",errorCode.c_str());
        CStdString errorCodeStr = g_localizeStrings.Get(81000);
      }

      CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),errorCodeStr);
      return false;
    }
    else
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::GetNonceFromServer - Transaction action succeeded. Update user EntitlementsList (pay)");
      BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().UpdateUserEntitlementsNow();
      return true;
    }
  }
}

bool CGUIDialogBoxeePaymentUserData::GetNonceFromServer(const CStdString& url)
{
  // set PostData to NOT be empty in order for action to be POST
  CStdString strPostData = "submit=subscribe";

  PostRequestJob* pJob = new PostRequestJob(url,strPostData);
  if (CUtil::RunInBG(pJob,false) != JOB_SUCCEEDED)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::GetNonceFromServer - FAILED to nonce from server. Job returned FALSE (pay)");
    return false;
  }

  CStdString strResp = pJob->GetRespose();
  long retCode = pJob->GetRetCode();

  delete pJob;

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::GetNonceFromServer - For [%s] got response length [%d]. [RetCode=%ld] (pay)",url.c_str(),(int)strResp.size(),retCode);

  if (retCode != 200)
  {
    m_quitPaymentProcess = true;
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::GetNonceFromServer - Got [RetCode=%ld] from server. After set [quitPaymentProcess=%d]. [url=%s] (pay)",retCode,m_quitPaymentProcess,url.c_str());
    return false;
  }

    BOXEE::BXXMLDocument xmlDoc;
    if (!xmlDoc.LoadFromString(strResp))
    {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::GetNonceFromServer - FAILED to load server response [%s]. [url=%s][IsRespEmpty=%d] (pay)",strResp.c_str(),url.c_str(),strResp.IsEmpty());
      return false;
    }

  TiXmlElement* pRootElement = xmlDoc.GetDocument().RootElement();
  if (!pRootElement)
    {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::GetNonceFromServer - FAILED to get RootElement. [url=%s] (pay)",url.c_str());
      return false;
    }

  if (strcmpi(pRootElement->Value(),"response") != 0)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::GetNonceFromServer - Root element is NOT <response>. [<%s>][url=%s] (pay)",pRootElement->Value(),url.c_str());
    return false;
  }

  TiXmlElement* pNonceElement = pRootElement->FirstChildElement("nonce");
  if (!pNonceElement)
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::GetNonceFromServer - Server response doesn't include <nonce> (pay)");

    TiXmlElement* pEmailElement = pRootElement->FirstChildElement("email");
    if (!pEmailElement)
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::GetNonceFromServer - Server response doesn't include <email> (pay)");
      return false;
    }

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::GetNonceFromServer - parse <email> from server response (pay)");

    CStdString email = pEmailElement->GetText();

    if (!WaitForNonceFromServer(url,email,m_nonceStr))
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::GetNonceFromServer - FAILED to wait for <nonce> from server (pay)");
      return false;
    }
  }
  else
  {
    m_nonceStr = pNonceElement->GetText();
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::GetNonceFromServer - parse <nonce> from server response. [nonce=%s] (pay)",m_nonceStr.c_str());
  }

  if (m_nonceStr.IsEmpty())
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::GetNonceFromServer - FAILED to get nonce [nonceStr=%s]. [url=%s] (pay)",m_nonceStr.c_str(),url.c_str());
    return false;
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::GetNonceFromServer - After set [nonceStr=%s] (pay)",m_nonceStr.c_str());

  return true;
}

bool CGUIDialogBoxeePaymentUserData::WaitForNonceFromServer(const CStdString& url, const CStdString& email, CStdString& nonceStr)
{
  if (email.IsEmpty())
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::WaitForNonceFromServer - Enter function with an empty email. [email=%s] (pay)",email.c_str());
    return false;
  }

  CGUIDialogBoxeePaymentWaitForServerApproval *dialog = (CGUIDialogBoxeePaymentWaitForServerApproval *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_PAYMENT_WAIT_FOR_SERVER_APPROVAL);
  if (!dialog)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::WaitForNonceFromServer - FAILED to get dialog to show (pay)");
    return false;
  }

  dialog->SetEmail(email);
  dialog->SetUrl(url);
  dialog->DoModal();

  m_quitPaymentProcess = dialog->IsQuitTransaction();

  if (dialog->IsConfirmed())
  {
    dialog->GetNonceStr(nonceStr);
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::WaitForNonceFromServer - dialog PaymentWaitForServerApproval returned TRUE and set [nonce=%s]. [quitPaymentProcess=%d] (pay)",nonceStr.c_str(),m_quitPaymentProcess);
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentUserData::WaitForNonceFromServer - dialog PaymentWaitForServerApproval returned FALSE. [quitPaymentProcess=%d] (pay)",m_quitPaymentProcess);
    return false;
  }
}

bool CGUIDialogBoxeePaymentUserData::IsQuitPaymentProcess()
{
  return m_quitPaymentProcess;
}

void CGUIDialogBoxeePaymentUserData::SetProductId(const CStdString& productId)
{
  m_productId = productId;
}

bool CGUIDialogBoxeePaymentUserData::Show(const CStdString& productId, bool& quitPaymentProcess)
{
  CGUIDialogBoxeePaymentUserData *dialog = (CGUIDialogBoxeePaymentUserData *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_PAYMENT_USERDATA);
  if (!dialog)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::Show - FAILED to get dialog to show (pay)");
    return false;
  }

  if (productId.IsEmpty())
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentUserData::Show - Enter function with EMPTY ProductId. [productId=%s] (pay)",productId.c_str());
    return false;
  }

  dialog->SetProductId(productId);
  dialog->DoModal();

  quitPaymentProcess = dialog->IsQuitPaymentProcess();

  return dialog->m_bConfirmed;
}

PostRequestJob::PostRequestJob(const CStdString& url, const CStdString& strPostData)
{
  m_url = url;
  m_strPostData = strPostData;
  m_strResp = "";
  m_retCode = 0;
}

PostRequestJob::~PostRequestJob()
{

}

void PostRequestJob::Run()
{
  m_strResp = "";
  m_retCode = 0;

  m_strResp = m_curl.HttpPostString(m_url.c_str(),m_strPostData);
  m_retCode = m_curl.GetLastRetCode();

  m_bJobResult = true;
}

CStdString PostRequestJob::GetRespose()
{
  return m_strResp;
}

long PostRequestJob::GetRetCode()
{
  return m_retCode;
}

