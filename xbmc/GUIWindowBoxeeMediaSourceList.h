#ifndef GUI_WINDOW_BOXEE_MEDIA_SOURCE_LIST
#define GUI_WINDOW_BOXEE_MEDIA_SOURCE_LIST

#pragma once

#include <vector>
#include "GUIDialog.h"
#include "GUIDialog.h"
#include "Settings.h"
#include "FileItem.h"
#include "MediaSource.h"

class CGUIWindowBoxeeMediaSourceList : public CGUIDialog
{
public:
  CGUIWindowBoxeeMediaSourceList(void);
  virtual ~CGUIWindowBoxeeMediaSourceList(void);
  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  CStdString GetSelectedName();
  void Refresh();
  
private:

  void ProccessItemSelectedInControlSourceList();
  
  void LoadAllShares();
  bool m_sourcesExist;
  
  CFileItemList m_sources;
};

#endif
