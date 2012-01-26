#pragma once

#ifndef GUIDIALOGBOXEEMANUALRESOLVEADDFILES_H_
#define GUIDIALOGBOXEEMANUALRESOLVEADDFILES_H_

#include "GUIDialog.h"
#include "FileItem.h"

#include "GUIDialogBoxBase.h"

class CGUIDialogBoxeeManualResolveAddFiles : public CGUIDialog
{

public:
  CGUIDialogBoxeeManualResolveAddFiles();
  virtual ~CGUIDialogBoxeeManualResolveAddFiles();
  
  static void Show(CFileItemPtr pItem, CFileItemList& items);
  
  virtual bool OnMessage(CGUIMessage &message);
  bool OnAction(const CAction& action);
  
protected:
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

  void GetDirectory(const CStdString& strFolderPath);

  void GetParts(CFileItemList& items)
  {
    items = m_partItems;
  }

private:

  void OnPartListClick();
  
  CFileItemPtr m_videoItem;
  CFileItemList m_fileItems;
  CFileItemList m_partItems;
  CStdString m_currentFolder;
  bool m_moveShortcut;

};

#endif /* GUIDIALOGBOXEEMANUALRESOLVEADDFILES_H_ */
