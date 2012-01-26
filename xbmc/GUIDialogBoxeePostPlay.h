//
// C++ Interface: GUIDialogBoxeePostPlay
//
// Description: 
//
//
// Author: Team Boxee <>, (C) 2009
//
#ifndef GUIDIALOGBOXEEPOSTPLAY_H
#define GUIDIALOGBOXEEPOSTPLAY_H

#include "GUIDialog.h"

/**
*/
class CGUIDialogBoxeePostPlay : public CGUIDialog
{
public:
  CGUIDialogBoxeePostPlay();
  virtual ~CGUIDialogBoxeePostPlay();

  virtual bool OnMessage(CGUIMessage &message);
  virtual void OnInitWindow();

  void SetItem(CFileItem *pItem);
  void Reset();
  
protected:

  bool HandleClickOnShareButton();
  bool HandleClickOnPlayNextButton();
  bool HandleClickOnRemoveFromQueueButton();

  CFileItem    m_item;
};

#endif
