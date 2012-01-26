#ifndef GUIDIALOGBOXEEDROPDOWN_H
#define GUIDIALOGBOXEEDROPDOWN_H

#include "GUIDialog.h"
#include "FileItem.h"

class CGUIBaseContainer;

class ICustomDropdownLabelCallback
{
public:
  virtual void OnGetDropdownLabel(CStdString& label) = 0;
  virtual ~ICustomDropdownLabelCallback(){}

  CStdString m_customTitle;
};

class CGUIDialogBoxeeDropdown : public CGUIDialog
{
public:
  CGUIDialogBoxeeDropdown(int id = WINDOW_DIALOG_BOXEE_DROPDOWN, const CStdString &xmlFile = "boxee_dropdown.xml");
  virtual ~CGUIDialogBoxeeDropdown();

  virtual void OnInitWindow();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);

  void SetTitle(const CStdString& title);

  virtual bool Show(CFileItemList& items, const CStdString& title, CStdString& value, int type = 1, ICustomDropdownLabelCallback* cdlCallback = NULL);
  virtual bool Show(CFileItemList& items, const CStdString& title, CStdString& value, float posX, float posY, int type = 1, ICustomDropdownLabelCallback* cdlCallback = NULL);

private:

  bool OnClick(CGUIMessage& message);
  bool GetClickedLabel(CGUIBaseContainer* subMenuList);

  CFileItemList m_items;
  bool m_bConfirmed;
  CStdString m_value;
  int m_type;
  void LoadItems();

  CStdString m_title;

  ICustomDropdownLabelCallback* m_getDropdownLabelCallback;
};

class CGUIDialogBoxeeBrowserDropdown : public CGUIDialogBoxeeDropdown
{
public:
  CGUIDialogBoxeeBrowserDropdown();
  virtual ~CGUIDialogBoxeeBrowserDropdown();
};

#endif //GUIDIALOGBOXEEDROPDOWN_H
