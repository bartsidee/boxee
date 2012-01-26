#ifndef GUIWINDOWBOXEEBROWSEPRODUCT_H_
#define GUIWINDOWBOXEEBROWSEPRODUCT_H_

#include "GUIWindowBoxeeBrowseTvShows.h"
#include "BoxeeServerDirectory.h"

using namespace BOXEE;

////////////////////
// CProductSource //
////////////////////

class CProductSourceType
{
public:
  enum ProductSourceTypeEnums
  {
    TV_SHOW=0,
    MOVIE=1,
    NUM_OF_PRODUCT_SOURCE_TYPES=2,
    UNKNOWN=3
  };
};

class CProductSource : public CBrowseWindowSource
{
public:
  CProductSource(const CStdString& name, const CStdString& strBasePath, int iWindowID);
  virtual ~CProductSource();
  virtual void AddStateParameters(std::map <CStdString, CStdString>& mapOptions);

  virtual void BindItems(CFileItemList& items);

  void SetProductId(const CStdString& productId);
  CStdString GetProductId();

  virtual CProductSourceType::ProductSourceTypeEnums GetProductSourceType() = 0;

protected:

  CStdString m_productId;
};

class CProductSourceMovie : public CProductSource
{
public:
  CProductSourceMovie(int iWindowID);
  virtual ~CProductSourceMovie();

  virtual CProductSourceType::ProductSourceTypeEnums GetProductSourceType();
};

class CProductSourceTvShow : public CProductSource
{
public:
  CProductSourceTvShow(int iWindowID);
  virtual ~CProductSourceTvShow();

  virtual CProductSourceType::ProductSourceTypeEnums GetProductSourceType();
};

/////////////////////////
// CProductWindowState //
/////////////////////////

class CProductWindowState : public CTvShowsWindowState
{
public:
  CProductWindowState(CGUIWindowBoxeeBrowse* pWindow);
  virtual ~CProductWindowState() {}
  virtual void SetCategory(const CStdString& strCategory);

  virtual bool OnBack();
};

//////////////////////////////////
// CGUIWindowBoxeeBrowseProduct //
//////////////////////////////////

class CGUIWindowBoxeeBrowseProduct : public CGUIWindowBoxeeBrowseTvShows
{
public:
  CGUIWindowBoxeeBrowseProduct();
	virtual ~CGUIWindowBoxeeBrowseProduct();

  virtual void OnInitWindow();
  virtual bool OnMessage(CGUIMessage& message);
};

#endif /*GUIWINDOWBOXEEBROWSEPRODUCT_H_*/
