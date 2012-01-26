#pragma once

#ifndef GUIDIALOGBOXEEMANUALRESOLVEMOVIE_H_
#define GUIDIALOGBOXEEMANUALRESOLVEMOVIE_H_

#include "GUIDialog.h"
#include "FileItem.h"
#include "FileSystem/FileCurl.h"
#include "GUIDialogBoxBase.h"
#include "Thread.h"

class CGUIDialogBoxeeManualResolveMovie : public CGUIDialog
{

public:
  CGUIDialogBoxeeManualResolveMovie();
  virtual ~CGUIDialogBoxeeManualResolveMovie();
  
  static bool Show(CFileItemPtr pItem, CFileItemList& videoParts);
  
  virtual bool OnMessage(CGUIMessage &message);
  //bool OnAction(const CAction& action);
  
protected:
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

private:

  CFileItemPtr m_videoItem;
  CFileItemList m_videoParts;
  
  bool m_bConfirmed;

};

#endif /* GUIDIALOGBOXEEMANUALRESOLVEMOVIE_H_ */
