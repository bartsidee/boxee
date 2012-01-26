/*
 * CGUIDialogBoxeeVideoQuality.h
 *
 *  The purpose of this dialog is to present a notification that a Boxee update is available
 */

#ifndef CGUIDIALOGBOXEEVIDEOQUALITY_H_
#define CGUIDIALOGBOXEEVIDEOQUALITY_H_

#include "GUIDialog.h"
#include "FileItem.h"

#define FULL_CVQ_DIALOG 0
#define LIST_CVQ_DIALOG 1
#define MAX_CVQ_DIALOG 2


class CGUIDialogBoxeeVideoQuality : public CGUIDialog
{
public:
  CGUIDialogBoxeeVideoQuality(void);
  virtual ~CGUIDialogBoxeeVideoQuality(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void OnInitWindow();
  virtual void Close(bool forceClose = false);
  
  bool IsCanceled(void) { return m_canceled; }
  void Reset();
  void Add(const CFileItem& item);
  void SetFocusedItem(int pos) {m_focusedItemPos = pos; }
  void ChangeDialogType(int dialogType);
  void LoadList();
  void ProccessItemSelected();
  int  GetSelectedItemPos();
  int  GetSavePerference() { return m_savePreference; }

protected:
  bool m_canceled;

  int m_dialogType;
  int m_selectedItemPos;
  int m_focusedItemPos;
  CFileItemList m_vecList;
  bool m_savePreference;

  int m_listControlId;
};

#endif /* CGUIDIALOGBOXEEVIDEOQUALITY_H_ */
