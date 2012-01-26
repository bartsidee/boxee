/*
 * CGUIDialogBoxeeUpdateProgress.h
 *
 *  The purpose of this dialog is to present a notification that a Boxee update is available
 */

#ifndef CGUIDIALOGBOXEEUPDATEPROGRESS_H_
#define CGUIDIALOGBOXEEUPDATEPROGRESS_H_

#include "GUIDialog.h"

class CGUIDialogBoxeeUpdateProgress : public CGUIDialog
{
public:
  CGUIDialogBoxeeUpdateProgress(void);
  virtual ~CGUIDialogBoxeeUpdateProgress(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual bool OnAction(const CAction &action);
  virtual void OnInitWindow();
  
  void SetLabel(const CStdString& label);
  
protected:
  
};

#endif /* CGUIDIALOGBOXEEUPDATEPROGRESS_H_ */
