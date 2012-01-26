#pragma once

#ifndef GUIDIALOGBOXEEMANUALRESOLVEEPISODE_H_
#define GUIDIALOGBOXEEMANUALRESOLVEEPISODE_H_

#include "GUIDialog.h"
#include "FileItem.h"

#include "GUIDialogBoxBase.h"

class CGUIDialogBoxeeManualResolveEpisode : public CGUIDialog
{

public:
  CGUIDialogBoxeeManualResolveEpisode();
  virtual ~CGUIDialogBoxeeManualResolveEpisode();
  
  static bool Show(CFileItemPtr pItem);
  
  virtual bool OnMessage(CGUIMessage &message);
  
protected:
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

private:

  CFileItemPtr m_videoItem;
  
  bool m_bConfirmed;
  
};

#endif /* GUIDIALOGBOXEEMANUALRESOLVEEPISODE_H_ */
