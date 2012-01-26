/*
* All Rights Reserved, Boxee.tv
*/

#ifndef GUIDIALOGBOXEEBROWSERCTX_H
#define GUIDIALOGBOXEEBROWSERCTX_H

#include "GUIDialogBoxeeCtx.h"
#include "GUIViewControl.h"
#include "GUILoaderDialog.h"
#include "Util.h"

class WebFavoriteJob : public IRunnable
{
public:
  WebFavoriteJob(const CStdString& url, const CStdString& urlTitle, bool bWebFavorite)
  {
    m_url = url;
    m_urlTitle = urlTitle;
    m_bWebFavorite = bWebFavorite;
    m_bJobResult = false;
  }

  WebFavoriteJob(const CStdString& url, const CStdString& urlTitle, bool bWebFavorite, unsigned long id)
  {
    m_url = url;
    m_urlTitle = urlTitle;
    m_bWebFavorite = bWebFavorite;
    m_bJobResult = false;
    m_id = id;
  }

  virtual ~WebFavoriteJob() { }
  virtual void Run();
  CStdString m_url;
  CStdString m_urlTitle;
  unsigned long m_id;
  bool m_bWebFavorite;
};



class CGUIDialogBoxeeBrowserCtx : public CGUIDialogBoxeeCtx
{
public:
  CGUIDialogBoxeeBrowserCtx();
  virtual bool OnMessage(CGUIMessage &message);
  virtual bool OnAction(const CAction &action);
  virtual void Update();
  virtual void Render();

  virtual void OnMoreInfo() ;

  virtual void SetItem(const CFileItem &item);

  void SetState();

protected:
  virtual void OnInitWindow();
  virtual void LoadFavorites();
  virtual void LoadHistory();
  virtual void OnAddWebFavorite();
  virtual void OnRemoveWebFavorite(CGUIListItemPtr ptrListItem);
  virtual void OnGoToURL(CGUIListItemPtr ptrListItem);
  
  virtual void OnGoToHistory();
  virtual void OnGoToFavorites();
  virtual void SetEmptyForFavorites();
  virtual void SetEmptyForHistory();
  virtual void SetFavoriteProperty();

  bool BrowseToAddressBarUrl();

  bool ManipulateAddressBar(const CStdString &strPath, bool SetHighlighted = false);

  bool  m_bMouseEnabled;
  bool  m_bBrowserClosed;
  CFileItemList* m_itemsFavorites;
  CFileItemList* m_itemsHistory;
};

#endif // GUIDIALOGBOXEEBROWSERCTX_H
