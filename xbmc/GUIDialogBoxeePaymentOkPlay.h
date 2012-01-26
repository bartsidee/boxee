
#ifndef CGUIDIALOGBOXEEPAYMENTOKPLAY_H_
#define CGUIDIALOGBOXEEPAYMENTOKPLAY_H_

#include "GUIDialog.h"

class CGUIDialogBoxeePaymentOkPlay : public CGUIDialog
{
public:
  CGUIDialogBoxeePaymentOkPlay(void);
  virtual ~CGUIDialogBoxeePaymentOkPlay(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);

  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

  static bool Show();

private:

  bool OnClick(CGUIMessage& message);

  bool m_bConfirmed;
};

#endif /* CGUIDIALOGBOXEEPAYMENTOKPLAY_H_ */
