//
// C++ Interface: GUIDialogBoxeeShare
//
// Description: 
//
//
// Author: Team XBMC <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef GUIDIALOGBOXEEFRIENDSMENU_H
#define GUIDIALOGBOXEEFRIENDSMENU_H

#include "GUIDialog.h"
#include "lib/libBoxee/bxfriendslist.h"
#include "PictureThumbLoader.h"

#include <vector>

/**
*/
class CGUIDialogBoxeeShare : public CGUIDialog
{
public:
  CGUIDialogBoxeeShare();
  virtual ~CGUIDialogBoxeeShare();

  virtual bool OnMessage(CGUIMessage &message);
  virtual void OnInitWindow();

  void SetItem(const CFileItem *item);
  
protected:
  CStdString GetShareText();

  CStdString   m_strText;
  CFileItem    m_item;
};

#endif
