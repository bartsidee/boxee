
#ifndef CGUIDIALOGBOXEEPAYMENTUSERDATA_H_
#define CGUIDIALOGBOXEEPAYMENTUSERDATA_H_

#include "GUIDialog.h"
#include "Thread.h"
#include "lib/libBoxee/bxcurl.h"

class PostRequestJob : public IRunnable
{
public:
  PostRequestJob(const CStdString& url, const CStdString& strPostData);
  virtual ~PostRequestJob();

  virtual void Run();

  CStdString GetRespose();
  long GetRetCode();

private:

  BOXEE::BXCurl m_curl;
  CStdString m_url;
  CStdString m_strPostData;
  CStdString m_strResp;
  long m_retCode;
};

class CGUIDialogBoxeePaymentUserData : public CGUIDialog
{
public:
  CGUIDialogBoxeePaymentUserData(void);
  virtual ~CGUIDialogBoxeePaymentUserData(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);

  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

  bool IsTransactionSucceeded();
  bool IsQuitPaymentProcess();

  static bool Show(const CStdString& productId, bool& quitPaymentProcess);

private:

  bool OnClick(CGUIMessage& message);
  bool HandleClickOnGoOn();

  bool HandleTransactionWithServer();
  bool GetNonceFromServer(const CStdString& url);

  bool WaitForNonceFromServer(const CStdString& url, const CStdString& message, CStdString& nonceStr);

  void SetProductId(const CStdString& productId);

  bool m_bConfirmed;
  bool m_transactionSucceeded;
  bool m_quitPaymentProcess;

  CStdString m_productId;
  CStdString m_password;
  CStdString m_nonceStr;
};

#endif /* CGUIDIALOGBOXEEPAYMENTUSERDATA_H_ */
