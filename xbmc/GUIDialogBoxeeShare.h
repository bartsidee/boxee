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
#ifndef GUIDIALOGBOXEESHARE_H
#define GUIDIALOGBOXEESHARE_H

#include "GUIDialog.h"
#include "../lib/libjson/include/json/value.h"
#include "Thread.h"

/**
*/
class CGUIDialogBoxeeShare : public CGUIDialog
{
public:
  CGUIDialogBoxeeShare();
  virtual ~CGUIDialogBoxeeShare();

  virtual bool OnMessage(CGUIMessage &message);
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

  void SetItem(const CFileItem *item);
  void UpdateShareDialog();

protected:
  
  CStdString GetDefaultShareText();
  bool GetDefaultMusicShareText(CStdString& defaultMusicShareText);
  bool GetDefaultTvShowShareText(CStdString& defaultTvShowShareText);

  CStdString    m_strText;
  CFileItem     m_item;
  CFileItemList m_servicesList;
  Json::Value   m_jsonServiceList;
};

#endif
