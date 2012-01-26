/*
 * GUIDialogFirstTimeUseMenuCust.h
 *
 *  Created on: Nov 29, 2010
 *      Author: shayyizhak
 */

#ifndef GUIDIALOGFIRSTTIMEUSEMENUCUST_H_
#define GUIDIALOGFIRSTTIMEUSEMENUCUST_H_

#include "GUIDialog.h"

class CGUIDialogFirstTimeUseMenuCust : public CGUIDialog
{
public:
  CGUIDialogFirstTimeUseMenuCust(void);
  virtual ~CGUIDialogFirstTimeUseMenuCust(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual bool OnAction(const CAction &action);
  virtual void OnInitWindow();

};

#endif /* GUIDIALOGFIRSTTIMEUSEMENUCUST_H_ */
