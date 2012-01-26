/*
 * CGUIDialogBoxeeUpdateMessage.h
 *
 *  The purpose of this dialog is to present a notification that a Boxee update is available
 */

#ifndef CGUIDIALOGBOXEEUPDATEMESSAGE_H_
#define CGUIDIALOGBOXEEUPDATEMESSAGE_H_

#include "GUIDialogBoxBase.h"
#include "BoxeeVersionUpdateManager.h"

class CGUIDialogBoxeeUpdateMessage : public CGUIDialogBoxBase
{
public:
  CGUIDialogBoxeeUpdateMessage(void);
  virtual ~CGUIDialogBoxeeUpdateMessage(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual bool OnAction(const CAction &action);
  virtual void OnInitWindow();
  
  void InLoginScreen(bool inLoginScreen);

protected:
  
  void SetDialogUpdateLabelAndButtons(bool inLoginScreen,VERSION_UPDATE_FORCE isVersionUpdateForce);

  bool m_inLoginScreen;

};

#endif /* CGUIDIALOGBOXEEMEDIAACTION_H_ */
