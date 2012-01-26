#ifndef CGUIDIALOGBOXEEPAIR_H_
#define CGUIDIALOGBOXEEPAIR_H_

#include "GUIDialog.h"
#include "BoxeeDeviceManager.h"

class CGUIDialogBoxeePair : public CGUIDialog
{
public:

  CGUIDialogBoxeePair(void);
  virtual ~CGUIDialogBoxeePair(void);

  virtual void OnInitWindow();

  virtual bool OnAction(const CAction& action);
  virtual bool OnMessage(CGUIMessage& message);

  virtual void Render();

  void SetDeviceItem(CBoxeeDeviceItem* deviceItem);
  CBoxeeDeviceItem GetDeviceItem();

  bool PairDevice(const CStdString& deviceId, const CStdString& code);

  void Reset();

protected:

  CStdString m_pairStr;
  CBoxeeDeviceItem m_deviceItem;
  CStdString m_passcodeStr;
};

#endif /* CGUIDIALOGBOXEEPAIR_H_ */
