/*
 * GUIDialogBoxeeApplicationAction.h
 *
 *  Created on: Feb 24, 2009
 *      Author: yuvalt
 */

#ifndef GUIDIALOGBOXEEAPPLICATIONACTION_H_
#define GUIDIALOGBOXEEAPPLICATIONACTION_H_

#include "GUIDialog.h"
#include "FileItem.h"

class CGUIDialogBoxeeApplicationAction :
      public CGUIDialog
{
public:
  CGUIDialogBoxeeApplicationAction();
  virtual ~CGUIDialogBoxeeApplicationAction();
  void SetAppItem(CFileItemPtr item);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);
  virtual void OnInitWindow();
  
  bool WasAppRemoved();

private:
  CFileItemPtr m_item;
  CFileItemList m_itemList;

  bool m_appWasRemoved;
};

#endif /* GUIDIALOGBOXEEAPPLICATIONACTION_H_ */
