
#include "GUIDialogBoxeePaymentProducts.h"
#include "GUIWindowManager.h"
#include "log.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "lib/libBoxee/boxee.h"
#include "GUIDialogProgress.h"
#include "LocalizeStrings.h"
#include "GUIBaseContainer.h"
#include "GUIDialogBoxeePaymentTou.h"
#include "GUIDialogOK2.h"
#include "GUIListContainer.h"
#include "Util.h"

#define HIDDEN_CONTAINER                5000
#define CONTROL_HEADER_LABEL            6010
#define PRODUCTS_LIST                   6110
#define BACK_BUTTON                     6130
#define PURCHASE_LIST                   6150

CGUIDialogBoxeePaymentProducts::CGUIDialogBoxeePaymentProducts(void) : CGUIDialog(WINDOW_DIALOG_BOXEE_PAYMENT_PRODUCTS, "boxee_payment_products.xml")
{
  m_bConfirmed = false;
}

CGUIDialogBoxeePaymentProducts::~CGUIDialogBoxeePaymentProducts()
{

}

void CGUIDialogBoxeePaymentProducts::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentProducts::OnInitWindow - Handling item [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-boxeeoffer=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d][quality-lbl=%s][quality=%d][is-hd=%s][productsList=%s] (pay)",m_item.GetLabel().c_str(),m_item.m_strPath.c_str(),m_item.GetThumbnailImage().c_str(),m_item.GetProperty("link-title").c_str(),m_item.GetProperty("link-url").c_str(),m_item.GetProperty("link-boxeetype").c_str(),m_item.GetProperty("link-boxeeoffer").c_str(),m_item.GetProperty("link-type").c_str(),m_item.GetProperty("link-provider").c_str(),m_item.GetProperty("link-providername").c_str(),m_item.GetProperty("link-providerthumb").c_str(),m_item.GetProperty("link-countrycodes").c_str(),m_item.GetPropertyBOOL("link-countryrel"),m_item.GetProperty("quality-lbl").c_str(),m_item.GetPropertyInt("quality"),m_item.GetProperty("is-hd").c_str(),m_item.GetProperty("link-productslist").c_str());

  // Send the item to the special container to allow skin access
  CFileItemPtr itemPtr(new CFileItem(m_item));
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), HIDDEN_CONTAINER, 0, 0, itemPtr);
  g_windowManager.SendThreadMessage(winmsg);

  m_bConfirmed = false;

  CGUIMessage resetProductsListMsg(GUI_MSG_LABEL_RESET, GetID(), PRODUCTS_LIST);
  OnMessage(resetProductsListMsg);

  CGUIMessage resetPurchaseListMsg(GUI_MSG_LABEL_RESET, GetID(), PURCHASE_LIST);
  OnMessage(resetPurchaseListMsg);

  CFileItemList productsList;

  if (!GetProductsListFromServer(productsList))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentProducts::OnInitWindow - FAILED to get products from server (pay)");
    m_bConfirmed = false;
    CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(55105));
    Close();
    return;
  }

  CFileItemList purchaseList;

  for (int i=0; i<productsList.Size(); i++)
  {
    CFileItemPtr purchaseItem(new CFileItem(g_localizeStrings.Get(55100)));
    purchaseList.Add(purchaseItem);
  }

  CGUIMessage addProductsListMsg(GUI_MSG_LABEL_BIND, GetID(), PRODUCTS_LIST, 0, 0, &productsList);
  OnMessage(addProductsListMsg);

  CGUIMessage addPurchaseListMsg(GUI_MSG_LABEL_BIND, GetID(), PURCHASE_LIST, 0, 0, &purchaseList);
  OnMessage(addPurchaseListMsg);

  CStdString header;
  header.Format(g_localizeStrings.Get(55101).c_str(), m_item.GetProperty("link-medialabel").c_str(),m_item.GetProperty("link-providername").c_str());

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentProducts::OnInitWindow - Going to set [%s] as header (pay)",header.c_str());

  SET_CONTROL_LABEL(CONTROL_HEADER_LABEL,header);

  SET_CONTROL_FOCUS(PRODUCTS_LIST, 0);
}

void CGUIDialogBoxeePaymentProducts::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
}

bool CGUIDialogBoxeePaymentProducts::OnAction(const CAction& action)
{
  switch (action.id)
  {
  case ACTION_PARENT_DIR:
  case ACTION_PREVIOUS_MENU:
  {

    m_bConfirmed = false;
    Close();
    return true;
  }
  break;
  case ACTION_MOVE_UP:
  {
    if (GetFocusedControlID() == PRODUCTS_LIST)
    {
      CGUIListContainer* purchaseListControl = (CGUIListContainer*)GetControl(PURCHASE_LIST);
      if (purchaseListControl)
      {
        purchaseListControl->OnUp();
      }
    }
    else if (GetFocusedControlID() == PURCHASE_LIST)
    {
      CGUIListContainer* productListControl = (CGUIListContainer*)GetControl(PRODUCTS_LIST);
      if (productListControl)
      {
        productListControl->OnUp();
      }
    }
  }
  break;
  case ACTION_MOVE_DOWN:
  {
    if (GetFocusedControlID() == PRODUCTS_LIST)
    {
      CGUIListContainer* purchaseListControl = (CGUIListContainer*)GetControl(PURCHASE_LIST);
      if (purchaseListControl)
      {
        purchaseListControl->OnDown();
      }
    }
    else if (GetFocusedControlID() == PURCHASE_LIST)
    {
      CGUIListContainer* productListControl = (CGUIListContainer*)GetControl(PRODUCTS_LIST);
      if (productListControl)
      {
        productListControl->OnDown();
      }
    }
  }
  break;
  default:
  {
    // do nothing
  }
  break;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeePaymentProducts::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    return OnClick(message);
  }
  break;
  default:
  {
    // do nothing
  }
  break;
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogBoxeePaymentProducts::OnClick(CGUIMessage& message)
{
  bool succeeded = false;

  int iControl = message.GetSenderId();

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentProducts::OnClick - Enter function with [iControl=%d] (pay)",iControl);

  switch(iControl)
  {
  case PRODUCTS_LIST:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentProducts::OnClick - Handling click on [%d=PRODUCTS_LIST] (pay)",iControl);

    succeeded = HandleClickOnProductsList();
  }
  break;
  case PURCHASE_LIST:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentProducts::OnClick - Handling click on [%d=PURCHASE_LIST] (pay)",iControl);

    succeeded = HandleClickOnPurchaseList();
  }
  break;
  default:
  {
    CLog::Log(LOGWARNING,"CGUIDialogBoxeePaymentProducts::OnClick - UNKNOWN control [%d] was click (pay)",iControl);
  }
  break;
  }

  return succeeded;
}

bool CGUIDialogBoxeePaymentProducts::HandleClickOnProductsList()
{
  CGUIListContainer* productListControl = (CGUIListContainer*)GetControl(PRODUCTS_LIST);

  if (!productListControl)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentProducts::HandleClickOnProductsList - FAILED to get container [%d=PRODUCTS_LIST] container (pay)",PRODUCTS_LIST);
    CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(55195));
    return false;
}

  CGUIListItemPtr selectedButton = productListControl->GetSelectedItemPtr();
  if (!selectedButton.get())
{
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentProducts::HandleClickOnProductsList - FAILED to get the SelectedItem from container [%d=PRODUCTS_LIST] container (pay)",PRODUCTS_LIST);
    CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(55195));
    return false;
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentProducts::HandleClickOnProductsList - Click on item [product_id=%s][seller_id=%s][source_index=%s][vindicia_id=%s][name=%s][description=%s][price=%s][currency=%s][currency_symbol=%s][billing_cycle=%s][billing_type=%s][active=%s] (pay)",selectedButton->GetProperty("product_id").c_str(),selectedButton->GetProperty("seller_id").c_str(),selectedButton->GetProperty("source_index").c_str(),selectedButton->GetProperty("vindicia_id").c_str(),selectedButton->GetProperty("name").c_str(),selectedButton->GetProperty("description").c_str(),selectedButton->GetProperty("price").c_str(),selectedButton->GetProperty("currency").c_str(),selectedButton->GetProperty("currency_symbol").c_str(),selectedButton->GetProperty("billing_cycle").c_str(),selectedButton->GetProperty("billing_type").c_str(),selectedButton->GetProperty("active").c_str());

  if (selectedButton->GetProperty("product_id").IsEmpty())
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentProducts::HandleClickOnProductsList - Clicked item in [%d=PRODUCTS_LIST] doesn't have [product_id] property (pay)",PRODUCTS_LIST);
    CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(55195));
    return false;
  }

  CStdString productId = selectedButton->GetProperty("product_id");
  CStdString productName = selectedButton->GetLabel();

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentProducts::HandleClickOnProductsList - Going to close dialog and open WINDOW_BOXEE_BROWSE_PRODUCT with params [ProductId=%s][ProductName=%s] (pay)",productId.c_str(),productName.c_str());

  std::vector<CStdString> params;
  params.push_back(productId);
  params.push_back(productName);

    Close();

  g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_PRODUCT,params);

    return true;
  }

bool CGUIDialogBoxeePaymentProducts::HandleClickOnPurchaseList()
  {
  CGUIListContainer* purchaseListControl = (CGUIListContainer*)GetControl(PURCHASE_LIST);

  if (!purchaseListControl)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentProducts::HandleClickOnPurchaseList - FAILED to get container [%d=PURCHASE_LIST] container (pay)",PURCHASE_LIST);
    CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(55195));
    return false;
  }

  CGUIListContainer* productListControl = (CGUIListContainer*)GetControl(PRODUCTS_LIST);

  if (!productListControl)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentProducts::HandleClickOnPurchaseList - FAILED to get container [%d=PRODUCTS_LIST] container (pay)",PRODUCTS_LIST);
    CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(55195));
    return false;
}

  int clickedItemIndex = purchaseListControl->GetSelectedItem();

  std::vector<CGUIListItemPtr>& productListItemsVec = productListControl->GetItemsByRef();

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentProducts::HandleClickOnPurchaseList - Click on [index=%d]. [ProductListSize=%d] (pay)",clickedItemIndex,(int)productListItemsVec.size());

  CGUIListItemPtr selectedButton;

  if (clickedItemIndex <= (int)productListItemsVec.size()-1)
{
    CGUIListItemPtr selectedButton = productListItemsVec[clickedItemIndex];
    if (!selectedButton.get())
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentProducts::HandleClickOnPurchaseList - FAILED to get the SelectedItem from container [%d=PRODUCTS_LIST] container (pay)",PRODUCTS_LIST);
      CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(55195));
      return false;
    }

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentProducts::HandleClickOnPurchaseList - For [index=%d] got product item [product_id=%s][seller_id=%s][source_index=%s][vindicia_id=%s][name=%s][description=%s][price=%s][currency=%s][currency_symbol=%s][billing_cycle=%s][billing_type=%s][active=%s] (pay)",clickedItemIndex,selectedButton->GetProperty("product_id").c_str(),selectedButton->GetProperty("seller_id").c_str(),selectedButton->GetProperty("source_index").c_str(),selectedButton->GetProperty("vindicia_id").c_str(),selectedButton->GetProperty("name").c_str(),selectedButton->GetProperty("description").c_str(),selectedButton->GetProperty("price").c_str(),selectedButton->GetProperty("currency").c_str(),selectedButton->GetProperty("currency_symbol").c_str(),selectedButton->GetProperty("billing_cycle").c_str(),selectedButton->GetProperty("billing_type").c_str(),selectedButton->GetProperty("active").c_str());

    bool quitPaymentProcess = false;
    if (CGUIDialogBoxeePaymentTou::Show(selectedButton,quitPaymentProcess))
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentProducts::HandleClickOnPurchaseList - dialog PaymentTou returned TRUE -> going to close dialog. [quitPaymentProcess=%d] (pay)",quitPaymentProcess);
      m_bConfirmed = !quitPaymentProcess;
  Close();
  return true;
}
    else
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentProducts::HandleClickOnPurchaseList - dialog PaymentTou returned FALSE. [quitPaymentProcess=%d] (pay)",quitPaymentProcess);
      return false;
    }
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentProducts::HandleClickOnPurchaseList - FAILED to get the SelectedItem from container [%d=PRODUCTS_LIST] container. [clickedItemIndex=%d][ProductListSize=%d] (pay)",PRODUCTS_LIST,clickedItemIndex,(int)productListItemsVec.size());
    CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(55195));
    return false;
  }
}

void CGUIDialogBoxeePaymentProducts::SetItem(CFileItemPtr item)
{
  m_item.Reset();
  m_item = *((CFileItem*)item.get());

  m_item.Dump();
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentProducts::SetItem - After set item [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-boxeeoffer=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d][quality-lbl=%s][quality=%d][is-hd=%s][productsList=%s] (pay)",m_item.GetLabel().c_str(),m_item.m_strPath.c_str(),m_item.GetThumbnailImage().c_str(),m_item.GetProperty("link-title").c_str(),m_item.GetProperty("link-url").c_str(),m_item.GetProperty("link-boxeetype").c_str(),m_item.GetProperty("link-boxeeoffer").c_str(),m_item.GetProperty("link-type").c_str(),m_item.GetProperty("link-provider").c_str(),m_item.GetProperty("link-providername").c_str(),m_item.GetProperty("link-providerthumb").c_str(),m_item.GetProperty("link-countrycodes").c_str(),m_item.GetPropertyBOOL("link-countryrel"),m_item.GetProperty("quality-lbl").c_str(),m_item.GetPropertyInt("quality"),m_item.GetProperty("is-hd").c_str(),m_item.GetProperty("link-productslist").c_str());
}

bool CGUIDialogBoxeePaymentProducts::GetProductsListFromServer(CFileItemList& productsList)
{
  CStdString productsListProp = m_item.GetProperty("link-productslist");

  if (productsListProp.IsEmpty())
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentProducts::GetProductsFromServer - FAILED to find ProductsList property in item [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-boxeeoffer=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d][quality-lbl=%s][quality=%d][is-hd=%s][productsList=%s] (pay)",m_item.GetLabel().c_str(),m_item.m_strPath.c_str(),m_item.GetThumbnailImage().c_str(),m_item.GetProperty("link-title").c_str(),m_item.GetProperty("link-url").c_str(),m_item.GetProperty("link-boxeetype").c_str(),m_item.GetProperty("link-boxeeoffer").c_str(),m_item.GetProperty("link-type").c_str(),m_item.GetProperty("link-provider").c_str(),m_item.GetProperty("link-providername").c_str(),m_item.GetProperty("link-providerthumb").c_str(),m_item.GetProperty("link-countrycodes").c_str(),m_item.GetPropertyBOOL("link-countryrel"),m_item.GetProperty("quality-lbl").c_str(),m_item.GetPropertyInt("quality"),m_item.GetProperty("is-hd").c_str(),m_item.GetProperty("link-productslist").c_str());
    return false;
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentProducts::GetProductsFromServer - Going to get from server ProductsList for [ProductsList=%s] (pay)",productsListProp.c_str());

  GetProductsListJob* pJob = new GetProductsListJob(productsListProp);
  if (CUtil::RunInBG(pJob,false) != JOB_SUCCEEDED)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentProducts::GetProductsFromServer - FAILED to get ProductList from server. Job returned FALSE (pay)");
    return false;
  }

  productsList = pJob->GetProductsList();

  delete pJob;

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentProducts::GetProductsFromServer - Exit function with [NumOfItems=%d] for [ProductsList=%s] (pay)",productsList.Size(),productsListProp.c_str());

  return true;
}

bool CGUIDialogBoxeePaymentProducts::Show(CFileItemPtr item)
{
  if (!item.get())
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentProducts::Show - Enter function with a NULL item (pay)");
    return false;
  }

  CGUIDialogBoxeePaymentProducts *dialog = (CGUIDialogBoxeePaymentProducts *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_PAYMENT_PRODUCTS);
  if (!dialog)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentProducts::Show - FAILED to get dialog to show (pay)");
    return false;
  }

  dialog->SetItem(item);
  dialog->DoModal();

  return dialog->m_bConfirmed;
}

GetProductsListJob::GetProductsListJob(const CStdString& productsListIds)
{
  m_productsListIds = productsListIds;
}

GetProductsListJob::~GetProductsListJob()
{

}

void GetProductsListJob::Run()
{
  CStdString strUrl = BOXEE::BXConfiguration::GetInstance().GetURLParam("Boxee.ProductsListUrl","http://app.boxee.tv/api/products?product_ids=");
  strUrl += m_productsListIds;

  BOXEE::BXXMLDocument xmlDoc;

  xmlDoc.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  xmlDoc.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

  if (!xmlDoc.LoadFromURL(strUrl))
  {
    CLog::Log(LOGERROR,"GetProductsListJob::Run - FAILED to get products from [strUrl=%s] (pay)",strUrl.c_str());
    return;
  }

  TiXmlElement* pRootElement = xmlDoc.GetDocument().RootElement();
  if (!pRootElement || strcmpi(pRootElement->Value(),"products") != 0)
  {
    CLog::Log(LOGERROR,"GetProductsListJob::Run - could not parse <products>. [pRootElement=%p][strUrl=%s] (pay)",pRootElement,strUrl.c_str());
    return;
  }

  TiXmlElement* productsChildElem = NULL;
  productsChildElem = pRootElement->FirstChildElement();

  while (productsChildElem)
  {
    if (strcmp(productsChildElem->Value(),"product") == 0)
    {
      CFileItemPtr productItem(new CFileItem());

      if (InitProductItem(productsChildElem,productItem))
      {
        CLog::Log(LOGDEBUG,"GetProductsListJob::Run - Adding item [product_id=%s][seller_id=%s][source_index=%s][vindicia_id=%s][name=%s][description=%s][price=%s][currency=%s][currency_symbol=%s][billing_cycle=%s][billing_type=%s][active=%s] (pay)",productItem->GetProperty("product_id").c_str(),productItem->GetProperty("seller_id").c_str(),productItem->GetProperty("source_index").c_str(),productItem->GetProperty("vindicia_id").c_str(),productItem->GetProperty("name").c_str(),productItem->GetProperty("description").c_str(),productItem->GetProperty("price").c_str(),productItem->GetProperty("currency").c_str(),productItem->GetProperty("currency_symbol").c_str(),productItem->GetProperty("billing_cycle").c_str(),productItem->GetProperty("billing_type").c_str(),productItem->GetProperty("active").c_str());

        m_productsList.Add(productItem);
      }
    }

    productsChildElem = productsChildElem->NextSiblingElement();
  }

  m_bJobResult = true;
}

bool GetProductsListJob::InitProductItem(TiXmlElement* productElem, CFileItemPtr productItem)
{
  if (!productElem)
  {
    CLog::Log(LOGERROR,"GetProductsListJob::GetProductsFromServer - Enter function with a NULL element (pay)");
    return false;
  }

  bool foundTouElement = false;
  TiXmlElement* productChildElem = NULL;
  productChildElem = productElem->FirstChildElement();

  while (productChildElem)
  {
    if (strcmp(productChildElem->Value(),"product_id") == 0)
    {
      productItem->SetProperty("product_id",productChildElem->GetText());
    }
    else if (strcmp(productChildElem->Value(),"seller_id") == 0)
    {
      productItem->SetProperty("seller_id",productChildElem->GetText());
    }
    else if (strcmp(productChildElem->Value(),"source_index") == 0)
    {
      productItem->SetProperty("source_index",productChildElem->GetText());
    }
    else if (strcmp(productChildElem->Value(),"vindicia_id") == 0)
    {
      productItem->SetProperty("vindicia_id",productChildElem->GetText());
    }
    else if (strcmp(productChildElem->Value(),"name") == 0)
    {
      productItem->SetProperty("name",productChildElem->GetText());
      productItem->SetLabel(productChildElem->GetText());
    }
    else if (strcmp(productChildElem->Value(),"description") == 0)
    {
      productItem->SetProperty("description",productChildElem->GetText());
    }
    else if (strcmp(productChildElem->Value(),"price") == 0)
    {
      productItem->SetProperty("price",productChildElem->GetText());
    }
    else if (strcmp(productChildElem->Value(),"currency") == 0)
    {
      productItem->SetProperty("currency",productChildElem->GetText());
    }
    else if (strcmp(productChildElem->Value(),"currency_symbol") == 0)
    {
      productItem->SetProperty("currency_symbol",productChildElem->GetText());
    }
    else if (strcmp(productChildElem->Value(),"billing_cycle") == 0)
    {
      productItem->SetProperty("billing_cycle",productChildElem->GetText());
    }
    else if (strcmp(productChildElem->Value(),"billing_type") == 0)
    {
      productItem->SetProperty("billing_type",productChildElem->GetText());
    }
    else if (strcmp(productChildElem->Value(),"active") == 0)
    {
      productItem->SetProperty("active",productChildElem->GetText());
    }
    else if (strcmp(productChildElem->Value(),"source") == 0)
    {
      TiXmlElement* sourceChildElem = NULL;
      sourceChildElem = productChildElem->FirstChildElement();

      while (sourceChildElem)
      {
        if (strcmp(sourceChildElem->Value(),"terms") == 0)
        {
          foundTouElement = true;
          productItem->SetProperty("terms",sourceChildElem->GetText());
        }

        sourceChildElem = sourceChildElem->NextSiblingElement();
      }
    }

    productChildElem = productChildElem->NextSiblingElement();
  }

  if (!foundTouElement)
  {
    CLog::Log(LOGERROR,"GetProductsListJob::GetProductsFromServer - Product DOESN'T have a <terms> element. [name=%s][product_id=%s][foundTouElement=%d] (pay)",productItem->GetProperty("name").c_str(),productItem->GetProperty("product_id").c_str(),foundTouElement);
    return false;
  }

  return true;
}

CFileItemList GetProductsListJob::GetProductsList()
{
  return m_productsList;
  }

