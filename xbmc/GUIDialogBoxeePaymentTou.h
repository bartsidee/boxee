
#ifndef CGUIDIALOGBOXEEPAYMENTTOU_H_
#define CGUIDIALOGBOXEEPAYMENTTOU_H_

#include "GUIDialog.h"
#include "FileItem.h"

class CGUIDialogProgress;

class CGUIDialogBoxeePaymentTou : public CGUIDialog
{
public:
  CGUIDialogBoxeePaymentTou(void);
  virtual ~CGUIDialogBoxeePaymentTou(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);

  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

  bool IsQuitPaymentProcess();

  static bool Show(CGUIListItemPtr item, bool& quitPaymentProcess);

private:

  void SetItem(CGUIListItemPtr item);

  bool OnClick(CGUIMessage& message);
  bool HandleClickOnAcceptTerms();
  bool HandleClickOnAcceptButton();
  bool HandleClickOnDeclineButton();

  bool GetPriceLabel(CStdString& priceStr);
  bool GetBillingTypeLabel(CStdString& billingTypeStr);

  void StartProgressDialog();
  void StopProgressDialog();

  bool m_bConfirmed;

  CFileItem m_item;

  CGUIDialogProgress* m_progress;

  bool m_bTermsAccepted;
  bool m_quitPaymentProcess;
};

#endif /* CGUIDIALOGBOXEEPAYMENTTOU_H_ */
