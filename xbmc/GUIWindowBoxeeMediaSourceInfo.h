#ifndef GUI_WINDOW_BOXEE_MEDIA_SOURCE_INFO
#define GUI_WINDOW_BOXEE_MEDIA_SOURCE_INFO

#pragma once

#include "GUIDialog.h"
#include "AppDescriptor.h"
#include "Thread.h"
#include "BoxeeMediaSourceList.h"

class GetRSSInfoBG : public IRunnable
{
public:
  GetRSSInfoBG(const CStdString& url)
  {
    m_url = url;
    m_bResult = false;
  }
  
  virtual void Run();
  CStdString m_url;
  bool m_bResult;
  CStdString m_title;
  CStdString m_thumbnail;
};

class CGUIWindowBoxeeMediaSourceInfo : public CGUIDialog
{
public:
  CGUIWindowBoxeeMediaSourceInfo(void);
  virtual ~CGUIWindowBoxeeMediaSourceInfo(void);
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  void SetEditedSource(CStdString sourceName);   
  void SetAddSource(CStdString sourcePath);
  void SetEnableLocationEdit(bool enableLocationEdit);
  void SetSourceThumbPath(CStdString thumbPath);
  void SetAppDescriptor(CAppDescriptor& appDesc);

private:
  CStdString shortenURL(CStdString path);
  CStdString ManipulateSourceDisplayName();
  bool IsOnlyPictureSourceSelected();

  CStdString m_editedSourceName;
  CBoxeeMediaSource  m_editedSource;
  CStdString m_sourcePath;
  CStdString m_sourceThumbPath;
  bool m_enableLocationEdit;
  CAppDescriptor m_appDesc;
};

#endif
