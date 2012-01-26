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
#ifndef GUIDIALOGBOXEEQUICKTIP_H
#define GUIDIALOGBOXEEQUICKTIP_H

#include "GUIDialog.h"

/**
*/
class CGUIDialogBoxeeQuickTip : public CGUIDialog
{
public:
  CGUIDialogBoxeeQuickTip();
  virtual ~CGUIDialogBoxeeQuickTip();

  virtual bool OnMessage(CGUIMessage &message);
  virtual void OnInitWindow();
  
  static void ShowAndGetInput();
};

#endif // GUIDIALOGBOXEEQUICKTIP_H
