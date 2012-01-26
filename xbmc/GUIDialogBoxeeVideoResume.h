/*
 * CGUIDialogBoxeeVideoResume.h
 *
 *  The purpose of this dialog is to present a notification that a Boxee update is available
 */

#ifndef CGUIDIALOGBOXEEVIDERESUME_H_
#define CGUIDIALOGBOXEEVIDERESUME_H_

#include "GUIDialog.h"

class CGUIDialogBoxeeVideoResume : public CGUIDialog
{
public:
  CGUIDialogBoxeeVideoResume();
  virtual ~CGUIDialogBoxeeVideoResume();
  virtual bool OnAction(const CAction& action);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void OnInitWindow();

  static int ShowAndGetInput(const std::vector<CStdString>& choices);

protected:

  int m_choiceIndex;

  std::vector<CStdString> m_choices;
};

#endif /* CGUIDIALOGBOXEEVIDERESUME_H_ */
