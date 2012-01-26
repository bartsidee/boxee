
#include "GUIWindowBoxeeBrowseProduct.h"
#include "utils/log.h"
#include "GUIWindowManager.h"
#include "Util.h"
#include "LocalizeStrings.h"

using namespace std;
using namespace BOXEE;

////////////////////
// CProductSource //
////////////////////

CProductSource::CProductSource(const CStdString& strName, const CStdString& strBasePath, int iWindowID) : CBrowseWindowSource(strName, strBasePath, iWindowID)
{

}

CProductSource::~CProductSource()
{

}

void CProductSource::AddStateParameters(std::map <CStdString, CStdString>& mapOptions)
{
  mapOptions["remote"] = "true";
  mapOptions["product"] = m_productId;

  CBrowseWindowSource::AddStateParameters(mapOptions);
}

void CProductSource::BindItems(CFileItemList& items)
{
  CLog::Log(LOGDEBUG,"CProductSource::BindItems - [SourceType=%d] - Enter function with [ItemListSize=%d] (pay)",GetProductSourceType(),items.Size());

  items.Sort(SORT_METHOD_LABEL, SORT_ORDER_DESC);
  return CBrowseWindowSource::BindItems(items);
}

void CProductSource::SetProductId(const CStdString& productId)
{
  m_productId = productId;
}

CStdString CProductSource::GetProductId()
{
  return m_productId;
}

CProductSourceMovie::CProductSourceMovie(int iWindowID) : CProductSource("ProductSourceMovie", "boxee://movies/movies/", iWindowID)
{

}

CProductSourceMovie::~CProductSourceMovie()
{

}

CProductSourceType::ProductSourceTypeEnums CProductSourceMovie::GetProductSourceType()
{
  return CProductSourceType::MOVIE;
}

CProductSourceTvShow::CProductSourceTvShow(int iWindowID) : CProductSource("ProductSourceMovie", "boxee://tvshows/tv/", iWindowID)
{

}

CProductSourceTvShow::~CProductSourceTvShow()
{

}

CProductSourceType::ProductSourceTypeEnums CProductSourceTvShow::GetProductSourceType()
{
  return CProductSourceType::TV_SHOW;
}

/////////////////////////
// CProductWindowState //
/////////////////////////

CProductWindowState::CProductWindowState(CGUIWindowBoxeeBrowse* pWindow) : CTvShowsWindowState(pWindow)
{
  m_sourceController.RemoveAllSources();
  m_sourceController.AddSource(new CProductSourceMovie(m_pWindow->GetID()));
}

void CProductWindowState::SetCategory(const CStdString& strCategory)
{

}

bool CProductWindowState::OnBack()
{
  CLog::Log(LOGDEBUG,"CProductWindowState::OnBack - Enter function (pay)");
  g_windowManager.PreviousWindow();
  return true;
}

//////////////////////////////////
// CGUIWindowBoxeeBrowseProduct //
//////////////////////////////////

CGUIWindowBoxeeBrowseProduct::CGUIWindowBoxeeBrowseProduct() : CGUIWindowBoxeeBrowseTvShows(WINDOW_BOXEE_BROWSE_PRODUCT, "boxee_browse_product.xml")
{
  SetWindowState(new CProductWindowState(this));
}

CGUIWindowBoxeeBrowseProduct::~CGUIWindowBoxeeBrowseProduct()
{

}

void CGUIWindowBoxeeBrowseProduct::OnInitWindow()
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseProduct::OnInitWindow - Enter function (pay)");

  g_windowManager.CloseDialogs(true);

  return CGUIWindowBoxeeBrowseTvShows::OnInitWindow();
}

bool CGUIWindowBoxeeBrowseProduct::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseProduct::OnMessage - GUI_MSG_WINDOW_INIT - Enter function. [NumOfStringParam=%d] (pay)",(int)message.GetNumStringParams());

    // ProductBrowse screen has only one source
    SourcesMap mapSources = ((CProductWindowState*)m_windowState)->GetSources();

    CProductSource* productSource = (CProductSource*)mapSources.begin()->second;
    if (!productSource)
    {
      CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseProduct::OnInitWindow - FAILED to get the ProductSource (pay)");
      Close();
      return false;
    }

    CStdString productId = message.GetStringParam(0);
    if (productId.IsEmpty())
    {
      CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseProduct::OnInitWindow - FAILED to get productId from message. [ProductId=%s] (pay)",productId.c_str());
      Close();
      return false;
    }

    productSource->SetProductId(productId);

    CStdString productName = message.GetStringParam(1);
    if (productName.IsEmpty())
    {
      CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseProduct::OnInitWindow - FAILED to get productName from message. [ProductName=%s] (pay)",productName.c_str());
      Close();
      return false;
    }

    CStdString packageName = productName;
    packageName += " ";
    packageName += g_localizeStrings.Get(55111);
    SetProperty("package-name",packageName);

    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseProduct::OnMessage - GUI_MSG_WINDOW_INIT - After set source ProductId to [%s] (pay)",productSource->GetProductId().c_str());
  }
  break;
  }

  return CGUIWindowBoxeeBrowseTvShows::OnMessage(message);
}

