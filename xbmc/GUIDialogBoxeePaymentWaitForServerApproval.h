
#ifndef CGUIDIALOGBOXEEPAYMENTWAITFORSEVERAPPROVAL_H_
#define CGUIDIALOGBOXEEPAYMENTWAITFORSEVERAPPROVAL_H_

#include "GUIDialog.h"
#include "lib/libBoxee/bxbgprocess.h"
#include "FileCurl.h"

class CGUIDialogProgress;

class CGUIDialogBoxeePaymentWaitForServerApproval : public CGUIDialog
{
public:
  CGUIDialogBoxeePaymentWaitForServerApproval(void);
  virtual ~CGUIDialogBoxeePaymentWaitForServerApproval(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);

  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

  virtual void Render();

  bool IsQuitTransaction();
  bool IsConfirmed();

  void SetEmail(const CStdString& email);
  void SetUrl(const CStdString& url);

  void GetNonceStr(CStdString& nonceStr);

private:

  bool OnClick(CGUIMessage& message);

  bool m_bConfirmed;

  bool m_quitTransaction;

  CStdString m_email;
  CStdString m_url;
  CStdString m_nonceStr;

  class CGetNonceJob : public BOXEE::BXBGJob
  {
  public:
    CGetNonceJob(const CStdString& url);
    virtual ~CGetNonceJob();
    virtual void DoWork();

    void GetNonceStr(CStdString& nonceStr);
    bool IsNonceSet();

    bool IsRuning();

    void Stop();

  private:

    CStdString m_url;
    CStdString m_nonceStr;
    bool m_nonceWasSet;
    bool m_isRunning;
    bool m_stop;
  };

  CGetNonceJob* m_getNonceJob;
  BOXEE::BXBGProcess m_processor;
};

#endif /* CGUIDIALOGBOXEEPAYMENTWAITFORSEVERAPPROVAL_H_ */
