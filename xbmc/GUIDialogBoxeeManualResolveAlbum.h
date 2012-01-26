#pragma once

#ifndef GUIDIALOGBOXEEMANUALRESOLVEALBUM_H_
#define GUIDIALOGBOXEEMANUALRESOLVEALBUM_H_

#include "GUIDialog.h"
#include "FileItem.h"
#include "FileSystem/FileCurl.h"
#include "GUIDialogBoxBase.h"
#include "Thread.h"

class CGUIDialogBoxeeManualResolveAlbum : public CGUIDialog
{

public:
  CGUIDialogBoxeeManualResolveAlbum();
  virtual ~CGUIDialogBoxeeManualResolveAlbum();
  
  static bool Show(CFileItemPtr pItem);
  
  virtual bool OnMessage(CGUIMessage &message);
  //bool OnAction(const CAction& action);
  
protected:
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

private:

  CFileItemPtr m_albumItem;
  bool m_bConfirmed;

};

#endif /* GUIDIALOGBOXEEMANUALRESOLVEALBUM_H_ */
