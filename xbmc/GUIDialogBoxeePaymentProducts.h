
#ifndef CGUIDIALOGBOXEEPAYMENTPRODUCTS_H_
#define CGUIDIALOGBOXEEPAYMENTPRODUCTS_H_

#include "GUIDialog.h"
#include "FileItem.h"
#include "Thread.h"

class GetProductsListJob : public IRunnable
{
public:
  GetProductsListJob(const CStdString& productsListIds);
  virtual ~GetProductsListJob();

  virtual void Run();

  CFileItemList GetProductsList();

private:

  bool InitProductItem(TiXmlElement* productChildElem, CFileItemPtr productItem);

  CStdString m_productsListIds;
  CFileItemList m_productsList;
};

class CGUIDialogBoxeePaymentProducts : public CGUIDialog
{
public:
  CGUIDialogBoxeePaymentProducts(void);
  virtual ~CGUIDialogBoxeePaymentProducts(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);

  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

  void SetItem(CFileItemPtr item);

  static bool Show(CFileItemPtr item);

private:

  bool OnClick(CGUIMessage& message);
  bool HandleClickOnProductsList();
  bool HandleClickOnPurchaseList();

  bool GetProductsListFromServer(CFileItemList& productsList);

  bool m_bConfirmed;

  CFileItem m_item;
};

#endif /* CGUIDIALOGBOXEEPAYMENTPRODUCTS_H_ */
