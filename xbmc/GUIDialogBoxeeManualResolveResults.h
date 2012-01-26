#pragma once

#ifndef GUIDIALOGBOXEEMANUALRESOLVERESULTS_H_
#define GUIDIALOGBOXEEMANUALRESOLVERESULTS_H_

#include "GUIDialog.h"
#include "FileItem.h"
#include "FileSystem/FileCurl.h"
#include "GUIDialogBoxBase.h"
#include "Thread.h"

/*
class CGetResultListBG : public IRunnable
{
public:
  CGetResultListBG(const CStdString& strTitle, bool bIsMovie);
  virtual ~CGetResultListBG() {}
  virtual void Run();
  bool ParseResultListXml(const CStdString& strHtml, CFileItemList& items);
  CStdString m_strTitle;
  CFileItemList m_resultListItems;
  bool m_bIsMovie;
  bool m_bResult;

  XFILE::CFileCurl m_http;
};

class CGetDetailsBG : public IRunnable
{
public:
  CGetDetailsBG(CFileItemPtr videoItem, bool isMovie);
  virtual ~CGetDetailsBG() {}
  virtual void Run();

  CFileItemPtr m_VideoItem;
  bool m_bIsMovie;
};

*/

class CGUIDialogBoxeeManualResolveResults : public CGUIDialog
{

public:
  CGUIDialogBoxeeManualResolveResults();
  virtual ~CGUIDialogBoxeeManualResolveResults();
  
  static void Show(CFileItemPtr pItem);
  
  virtual bool OnMessage(CGUIMessage &message);
  //bool OnAction(const CAction& action);
  
protected:
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

private:

  CFileItemPtr m_VideoItem;
  
};

#endif /* GUIDIALOGBOXEEMANUALRESOLVERESULTS_H_ */
