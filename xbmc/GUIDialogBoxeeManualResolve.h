#pragma once

#ifndef GUIDIALOGBOXEEMANUALRESOLVE_H_
#define GUIDIALOGBOXEEMANUALRESOLVE_H_

#include "GUIDialog.h"
#include "FileItem.h"
#include "Thread.h"
#include "GUIDialogBoxBase.h"
#include "FileSystem/FileCurl.h"
#include "MetadataResolverVideo.h"

class CLoadVideoFileContext : public IRunnable
{
public:
  CLoadVideoFileContext (CVideoFileContext& videoFileContext,CFileItemPtr& videoItem,CFileItemList& videoParts);
  virtual ~CLoadVideoFileContext() {}
  virtual void Run();

  CVideoFileContext& m_videoFileContext;
  CFileItemPtr&     m_videoItem;
  CFileItemList&    m_videoParts;
};

class CAddVideoBG : public IRunnable
{
public:
  CAddVideoBG (const BOXEE::BXMetadata& resolvedVideoMetadata,const CVideoFileContext& videoFileContext,const CFileItemList& videoParts);
  virtual ~CAddVideoBG() {}
  virtual void Run();
  bool         AddVideo();

  BOXEE::BXMetadata m_resolvedVideoMetadata;
  CVideoFileContext m_videoFileContext;
  CFileItemList     m_videoParts;
};

class CGetResultListBG : public IRunnable
{
public:
  CGetResultListBG(const CStdString& strTitle, CFileItemList& items);
  virtual ~CGetResultListBG() {}
  virtual void Run();
  bool ParseResultListXml(const CStdString& strHtml, CFileItemList& items);
  CStdString m_strTitle;
  
  CFileItemList& m_items;
  XFILE::CFileCurl m_http;
};

class CGetDetailsBG : public IRunnable
{
public:
  CGetDetailsBG(CVideoFileContext& context, BOXEE::BXMetadata* pMetadata);
  virtual ~CGetDetailsBG() {}
  virtual void Run();

  CVideoFileContext& m_context;
  BOXEE::BXMetadata* m_pMetadata;

};

class CSendConfirmationBG : public IRunnable
{
public:
  CSendConfirmationBG(CVideoFileContext& context);
  virtual ~CSendConfirmationBG() {}
  virtual void Run();

  CVideoFileContext& m_context;
  XFILE::CFileCurl m_http;
};

class CGUIDialogBoxeeManualResolve : public CGUIDialog
{

public:
  CGUIDialogBoxeeManualResolve();
  virtual ~CGUIDialogBoxeeManualResolve();
  
  static bool Show(CFileItemPtr pItem);
  
  virtual bool OnMessage(CGUIMessage &message);
  virtual bool OnAction(const CAction &action);
  
protected:
  void Reset();

  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  
  bool GetList();



private:
  bool IdentifyWithNFO();
  bool ResolveVideo();
  
  CFileItemPtr m_unresolvedVideoItem;
  CFileItemPtr m_resolvedVideoItem;
  
  BOXEE::BXMetadata m_resovedVideoMetadata;
  BOXEE::BXMetadata m_episodeResolvedVideoMetadata;

  CVideoFileContext m_videoFileContext;
  CVideoFileContext m_episodeVideoFileContext;

  // Items returned as a result list from the server
  CFileItemList m_resultListItems;
  
  // Item that the user has selected
  CFileItemPtr m_selectedItem;
  
  CFileItemList m_videoParts;
  
  bool m_bConfirmed;
  bool m_bUseNFO;
  bool m_bSearched;

};

#endif /* GUIDIALOGBOXEEMANUALRESOLVE_H_ */
