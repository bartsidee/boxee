/*
 * GUIDialogBoxeeSortDropdown.h
 *
 *  Created on: Nov 24, 2010
 *      Author: shayyizhak
 */

#ifndef GUIDIALOGBOXEESORTDROPDOWN_H_
#define GUIDIALOGBOXEESORTDROPDOWN_H_

#include "GUIDialog.h"
#include "FileItem.h"
#include "GUIDialogBoxeeDropdown.h"

class CGUIDialogBoxeeSortDropdown : public CGUIDialog
{
public:
  CGUIDialogBoxeeSortDropdown();
  virtual ~CGUIDialogBoxeeSortDropdown();

  static bool Show(CFileItemList& items, CStdString& value, float posX, float posY);
  static bool Show(CFileItemList& items, CStdString& value);

  virtual void OnInitWindow();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);

private:

  bool OnClick(CGUIMessage& message);
  bool GetClickedLabel(CGUIBaseContainer* subMenuList);

  bool SetControlPos(CGUIControl* pControl,float posX,float posY);

  CFileItemList m_items;
  bool m_bConfirmed;
  CStdString m_value;
  int m_type;
  bool LoadItems();
  int m_selectedSort;

  ICustomDropdownLabelCallback* m_getDropdownLabelCallback;
};


#endif /* GUIDIALOGBOXEESORTDROPDOWN_H_ */
