#ifndef GUI_WINDOW_BOXEE_MEDIA_SOURCES
#define GUI_WINDOW_BOXEE_MEDIA_SOURCES

#pragma once

#include "GUIDialog.h"
#include "FileItem.h"
#include "MediaSource.h"


#define SOURCE_LOCAL   1
#define SOURCE_NETWORK 2
#define SOURCE_NETWORK_APPLICATIONS 3
#define MANUALLY_ADD_SOURCE 4

class CGUIWindowBoxeeMediaSources : public CGUIDialog
{
public:
  CGUIWindowBoxeeMediaSources(void);
  virtual ~CGUIWindowBoxeeMediaSources(void);
  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void Render();

  int getSelectedSource();
  void Refresh();
  
  void UpdateScanLabel(const CStdString& path);
  void UpdateResolveLabel(const CStdString& path);

private:

  CStdString GetScanResolvePathToShow(const CStdString& path);
  
  void OpenGUIWindow(int windowID);
  void LoadAllShares();
  void ProccessItemSelectedInControlSourceList();
  
  int    m_selectedSource;
  bool m_sourcesExist;
  int  m_renderCount;

  CFileItemList m_sources;

};


#endif
