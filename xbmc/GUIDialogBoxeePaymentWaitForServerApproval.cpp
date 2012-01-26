
#include "GUIDialogBoxeePaymentWaitForServerApproval.h"
#include "GUIWindowManager.h"
#include "log.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "lib/libBoxee/boxee.h"
#include "LocalizeStrings.h"
#include "GUIEditControl.h"
#include "GUIRadioButtonControl.h"
#include "GUIDialogOK2.h"
#include "lib/libBoxee/bxcurl.h"
#include "lib/libBoxee/boxee.h"
#include "GUIDialogProgress.h"
#include "FileCurl.h"
#include "bxutils.h"

#define HIDDEN_CONTAINER                5000
#define CONTROL_TITLE_LABEL             6410
#define CONTROL_MESSAGE_LABEL           6411
#define DO_LATER_BUTTON                 6431

#define CHECK_NONCE_IN_SEC              5
#define MAX_WAIT_FOR_NONCE_IN_SEC       600  // 10 min

CGUIDialogBoxeePaymentWaitForServerApproval::CGUIDialogBoxeePaymentWaitForServerApproval(void) : CGUIDialog(WINDOW_DIALOG_BOXEE_PAYMENT_WAIT_FOR_SERVER_APPROVAL, "boxee_payment_wait_for_server_approval.xml")
{
  m_bConfirmed = true;
  m_quitTransaction = false;
  m_email = "";
  m_nonceStr = "";
  m_getNonceJob = NULL;
}

CGUIDialogBoxeePaymentWaitForServerApproval::~CGUIDialogBoxeePaymentWaitForServerApproval()
{

}

void CGUIDialogBoxeePaymentWaitForServerApproval::OnInitWindow()
{
  if (m_email.IsEmpty())
  {
    m_bConfirmed = false;
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentWaitForServerApproval::OnInitWindow - Enter function with EMPTY email [%s]. After set [Confirmed=%d] (pay)",m_email.c_str(),m_bConfirmed);
    Close();
    return;
  }

  if (m_url.IsEmpty())
  {
    m_bConfirmed = false;
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentWaitForServerApproval::OnInitWindow - Enter function with EMPTY url [%s]. After set [Confirmed=%d] (pay)",m_url.c_str(),m_bConfirmed);
    Close();
    return;
  }

  CGUIDialog::OnInitWindow();

  m_nonceStr = "";
  m_quitTransaction = false;

  m_getNonceJob  = new CGetNonceJob(m_url);

  m_processor.Start(1);
  m_processor.QueueJob(m_getNonceJob);

  CStdString message;
  CStdString email = "[COLOR green]";
  email += m_email;
  email += "[/COLOR]";
  message.Format(g_localizeStrings.Get(55143).c_str(), email.c_str());

  SET_CONTROL_LABEL(CONTROL_MESSAGE_LABEL,message);

  SET_CONTROL_FOCUS(DO_LATER_BUTTON, 0);
}

void CGUIDialogBoxeePaymentWaitForServerApproval::OnDeinitWindow(int nextWindowID)
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentWaitForServerApproval::OnDeinitWindow - Enter function. [nextWindowID=%d] (pay)",nextWindowID);

  m_getNonceJob->Stop();
  m_processor.Stop();

  m_getNonceJob->GetNonceStr(m_nonceStr);

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentWaitForServerApproval::OnDeinitWindow - After set [nonceStr=%s]. [JobIsNonceSet=%d] (pay)",m_nonceStr.c_str(),m_getNonceJob->IsNonceSet());

  delete m_getNonceJob;

  CGUIDialog::OnDeinitWindow(nextWindowID);

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentWaitForServerApproval::OnDeinitWindow - Exit function. [nextWindowID=%d] (pay)",nextWindowID);
}

bool CGUIDialogBoxeePaymentWaitForServerApproval::OnAction(const CAction& action)
{
  switch (action.id)
  {
  case ACTION_PARENT_DIR:
  case ACTION_PREVIOUS_MENU:
  {
    m_bConfirmed = false;
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentWaitForServerApproval::OnAction - Handling action [%d]. After set [Confirmed=%d] and going to call Close() (pay)",action.id,m_bConfirmed);
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

bool CGUIDialogBoxeePaymentWaitForServerApproval::OnMessage(CGUIMessage& message)
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

bool CGUIDialogBoxeePaymentWaitForServerApproval::OnClick(CGUIMessage& message)
{
  bool succeeded = false;

  int iControl = message.GetSenderId();

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentWaitForServerApproval::OnClick - Enter function with [iControl=%d] (pay)",iControl);

  switch(iControl)
  {
  case DO_LATER_BUTTON:
  {
    m_bConfirmed = false;
    m_quitTransaction = true;
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentWaitForServerApproval::OnClick - Handling click on [%d=DO_LATER_BUTTON] - going to call Close(). [Confirmed=%d][quitTransaction=%d] (pay)",DO_LATER_BUTTON,m_bConfirmed,m_quitTransaction);
    Close();
    return true;
  }
  break;
  default:
  {
    CLog::Log(LOGWARNING,"CGUIDialogBoxeePaymentWaitForServerApproval::OnClick - UNKNOWN control [%d] was click (pay)",iControl);
  }
  break;
  }

  return succeeded;
}

void CGUIDialogBoxeePaymentWaitForServerApproval::Render()
{
  if (!m_getNonceJob->IsRuning())
  {
    m_bConfirmed = m_getNonceJob->IsNonceSet();
    Close();
    return;
  }

  CGUIDialog::Render();
}

bool CGUIDialogBoxeePaymentWaitForServerApproval::IsQuitTransaction()
{
  return m_quitTransaction;
}

bool CGUIDialogBoxeePaymentWaitForServerApproval::IsConfirmed()
{
  return m_bConfirmed;
}

void CGUIDialogBoxeePaymentWaitForServerApproval::SetEmail(const CStdString& email)
{
  m_email = email;
}

void CGUIDialogBoxeePaymentWaitForServerApproval::SetUrl(const CStdString& url)
{
  m_url = url;
}

void CGUIDialogBoxeePaymentWaitForServerApproval::GetNonceStr(CStdString& nonceStr)
{
  nonceStr = m_nonceStr;
}

//////////////////
// CGetNonceJob //
//////////////////

CGUIDialogBoxeePaymentWaitForServerApproval::CGetNonceJob::CGetNonceJob(const CStdString& url) : BXBGJob("CGetNonceJob",false)
{
  m_url = url;
  m_nonceStr = "";
  m_nonceWasSet = false;
  m_isRunning = false;
  m_stop = false;
}

CGUIDialogBoxeePaymentWaitForServerApproval::CGetNonceJob::~CGetNonceJob()
{

}

void CGUIDialogBoxeePaymentWaitForServerApproval::CGetNonceJob::DoWork()
{
  m_isRunning = true;
  m_nonceWasSet = false;

  CLog::Log(LOGDEBUG,"CGetNonceJob::DoWork - Enter function (pay)");

  long retCode;

  BOXEE::BXCurl curl;
  CStdString strResp;

  // set PostData to NOT be empty in order for action to be POST
  CStdString strPostData = "submit=subscribe";

  CStdString url = m_url;
  url += "?retry=";

  CLog::Log(LOGDEBUG,"CGetNonceJob::DoWork - Going to check for <nonce>. [url=%s][stop=%d] (pay)",url.c_str(),m_stop);

  int loopCounter = -1;
  int requestCounter = -1;

  while (!m_stop)
  {
    loopCounter++;

    if (loopCounter % CHECK_NONCE_IN_SEC == 0)
    {
      requestCounter++;
      CStdString requestCounterStr = BOXEE::BXUtils::IntToString(requestCounter);
      url += requestCounterStr;

      strResp = curl.HttpPostString(url.c_str(),strPostData);

      retCode = curl.GetLastRetCode();

      CLog::Log(LOGDEBUG,"CGetNonceJob::DoWork - [%d] - For [%s] got response length [%d]. [RetCode=%ld][requestCounter=%d][requestCounterStr=%s][stop=%d] (pay)",loopCounter,url.c_str(),(int)strResp.size(),retCode,requestCounter,requestCounterStr.c_str(),m_stop);

      if (m_stop)
    {
      continue;
    }

      if (retCode == 200 && (int)strResp.size() > 0)
    {
      BOXEE::BXXMLDocument xmlDoc;
      if (!xmlDoc.LoadFromString(strResp))
      {
          CLog::Log(LOGERROR,"CGetNonceJob::DoWork - [%d] - FAILED to load server response. [url=%s][IsRespEmpty=%d][stop=%d] (pay)",loopCounter,url.c_str(),strResp.IsEmpty(),m_stop);
      }
      else
      {
          TiXmlElement* pRootElement = xmlDoc.GetDocument().RootElement();
          if (!pRootElement)
        {
            CLog::Log(LOGERROR,"CGetNonceJob::DoWork - [%d] - FAILED to get RootElement. [url=%s][stop=%d] (pay)",loopCounter,url.c_str(),m_stop);
        }
        else
        {
            if (strcmpi(pRootElement->Value(),"response") != 0)
            {
              CLog::Log(LOGERROR,"CGetNonceJob::DoWork - [%d] - Root element is NOT <response>. [<%s>][url=%s][stop=%d] (pay)",loopCounter,pRootElement->Value(),url.c_str(),m_stop);
            }
            else
              {
              TiXmlElement* pNonceElement = pRootElement->FirstChildElement("nonce");
              if (pNonceElement)
              {
                m_nonceStr = pNonceElement->GetText();
                m_nonceWasSet = true;
                m_stop = true;
                CLog::Log(LOGDEBUG,"CGetNonceJob::DoWork - [%d] - After get [nonceStr=%s]. [stop=%d] (pay)",loopCounter,m_nonceStr.c_str(),m_stop);
                continue;
              }
              else
              {
                CLog::Log(LOGDEBUG,"CGetNonceJob::DoWork - [%d] - Server response doesn't include <nonce>. [stop=%d] (pay)",loopCounter,m_stop);
            }
          }
        }
      }
    }
    }

    if (loopCounter > MAX_WAIT_FOR_NONCE_IN_SEC)
    {
      CLog::Log(LOGDEBUG,"CGetNonceJob::DoWork - [%d] - Wait for more the %dsec. Going to exit (pay)",loopCounter,MAX_WAIT_FOR_NONCE_IN_SEC);
      m_stop = true;
      continue;
    }

    if (!m_stop)
    {
      Sleep(1000);
        }
      }

  CLog::Log(LOGDEBUG,"CGetNonceJob::DoWork - [%d] - Exit function. [nonceStr=%s][stop=%d] (pay)",loopCounter,m_nonceStr.c_str(),m_stop);

  m_isRunning = false;
    }

void CGUIDialogBoxeePaymentWaitForServerApproval::CGetNonceJob::GetNonceStr(CStdString& nonceStr)
{
  nonceStr = m_nonceStr;
}

bool CGUIDialogBoxeePaymentWaitForServerApproval::CGetNonceJob::IsNonceSet()
{
  return m_nonceWasSet;
}

bool CGUIDialogBoxeePaymentWaitForServerApproval::CGetNonceJob::IsRuning()
    {
  return m_isRunning;
    }

void CGUIDialogBoxeePaymentWaitForServerApproval::CGetNonceJob::Stop()
{
  m_stop = true;
  }

