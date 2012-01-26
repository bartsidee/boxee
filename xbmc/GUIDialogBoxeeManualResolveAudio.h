#pragma once

#ifndef GUIDIALOGBOXEEMANUALRESOLVEAUDIO_H_
#define GUIDIALOGBOXEEMANUALRESOLVEAUDIO_H_

#include "GUIDialog.h"
#include "FileItem.h"
#include "Thread.h"
#include "GUIDialogBoxBase.h"
#include "MetadataResolverMusic.h"

class CGetAlbumResultListBG : public IRunnable
{
public:
  CGetAlbumResultListBG(const CStdString& strTitle , CFileItemList* fileList);
  virtual ~CGetAlbumResultListBG() {}

  virtual void Run();
  
  CStdString            m_strTitle;
  vectorMetadata        m_vectorAlbums;
  CFileItemList*  m_pFileList;
};

class CGUIDialogBoxeeManualResolveAudio : public CGUIDialog
{

public:
  CGUIDialogBoxeeManualResolveAudio();
  virtual ~CGUIDialogBoxeeManualResolveAudio();
  
  static bool Show(CFileItemPtr pItem);
  
  virtual bool OnMessage(CGUIMessage &message);
  
protected:
  void Reset();
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

  bool GetList();
  bool AddAlbum(CFileItemPtr _selectedItem);

private:

  CFileItemPtr  m_unresolvedItem;
  CFileItemPtr  m_resolvedItem;

  BOXEE::BXMetadata m_resolvedMetadata;

  // Items returned as a result list from the server
  CFileItemList m_resultListItems;

  // Item that the user has selected
  CFileItemPtr m_selectedItem;

  bool m_bConfirmed;

};

#endif /* GUIDIALOGBOXEEMANUALRESOLVEAUDIO_H_ */
