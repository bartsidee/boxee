#pragma once

#ifndef GUIDIALOGBOXEEMANUALRESOLVEDETAILS_H_
#define GUIDIALOGBOXEEMANUALRESOLVEDETAILS_H_

#include "GUIDialog.h"
#include "FileItem.h"

#include "GUIDialogBoxBase.h"

class CGUIDialogBoxeeManualResolveDetails : public CGUIDialog
{

public:
  CGUIDialogBoxeeManualResolveDetails();
  virtual ~CGUIDialogBoxeeManualResolveDetails();
  
  static void Show(CFileItemPtr pItem);
  
  virtual bool OnMessage(CGUIMessage &message);
  //bool OnAction(const CAction& action);
  
protected:
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

private:

  CFileItemPtr m_VideoItem;
  
};

#endif /* GUIDIALOGBOXEEMANUALRESOLVEDETAILS_H_ */
