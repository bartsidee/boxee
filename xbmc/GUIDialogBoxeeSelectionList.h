/*
 * CGUIDialogBoxeeBoxeeSelectionList.h
 *
 *  The purpose of this dialog is to be an abstraction for other dialogs for selecting items from a list
 */

#ifndef CGUIDIALOGBOXEESELECTIONLST_H_
#define CGUIDIALOGBOXEESELECTIONLST_H_

#include "GUIDialog.h"
#include "FileItem.h"

class CGUIDialogBoxeeSelectionList : public CGUIDialog
{
public:
  CGUIDialogBoxeeSelectionList(void);
  virtual ~CGUIDialogBoxeeSelectionList(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void OnInitWindow();
  virtual void Close(bool forceClose = false);

  bool IsCanceled(void) { return m_canceled; }
  void Reset();
  void Set(const CFileItemList& item) { m_vecList = item; }
  void SetTitle (const CStdString& title) { m_strDialogTitle = title;}
  void SetFocusedItem(int pos) {m_focusedItemPos = pos; }
  void LoadList();
  void ProccessItemSelected();
  int  GetSelectedItemPos();

protected:
  bool m_canceled;

  int m_selectedItemPos;
  int m_focusedItemPos;
  CFileItemList m_vecList;
  CStdString m_strDialogTitle;
};

#endif /* CGUIDIALOGBOXEESELECTIONLST_H_ */
