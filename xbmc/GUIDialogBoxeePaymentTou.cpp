
#include "GUIDialogBoxeePaymentTou.h"
#include "GUIWindowManager.h"
#include "log.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "lib/libBoxee/boxee.h"
#include "GUIDialogProgress.h"
#include "LocalizeStrings.h"
#include "GUIBaseContainer.h"
#include "GUIDialogOK2.h"
#include "GUIDialogBoxeePaymentUserData.h"
#include "GUITextBox.h"
#include "GUIInfoTypes.h"

#define HIDDEN_CONTAINER                5000
#define BACK_BUTTON                     6130
#define CONTROL_TITLE_LABEL             6210
#define CONTROL_TEXT_LABEL              6211
#define CONTROL_TOU_TEXTBOX             6220
#define CONTROL_TOU_TEXTBOX_SCROLL      6221
#define CONTROL_TERMS_RADIO_BUTTON      6222
#define ACCEPT_BUTTON                   6231
#define DECLINE_BUTTON                  6232

CGUIDialogBoxeePaymentTou::CGUIDialogBoxeePaymentTou(void) : CGUIDialog(WINDOW_DIALOG_BOXEE_PAYMENT_TOU, "boxee_payment_tou.xml")
{
  m_bConfirmed = false;
  m_quitPaymentProcess = false;
  m_bTermsAccepted = false;
}

CGUIDialogBoxeePaymentTou::~CGUIDialogBoxeePaymentTou()
{

}

void CGUIDialogBoxeePaymentTou::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  if (!m_item.HasProperty("terms"))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentTou::OnInitWindow - FAILED to find TOU property in item (pay)");
    CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(55195));
    Close();
    return;
  }

  CStdString priceStr;
  if (!GetPriceLabel(priceStr))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentTou::OnInitWindow - FAILED to get build PriceLabel (pay)");
    CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(55195));
    Close();
    return;
  }

  CStdString billingTypeStr;
  if (!GetBillingTypeLabel(billingTypeStr))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentTou::OnInitWindow - FAILED to build BillingType (pay)");
    CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(55195));
    Close();
    return;
  }

  CStdString text;
  text.Format(g_localizeStrings.Get(55121).c_str(), priceStr.c_str(),billingTypeStr.c_str());

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentTou::OnInitWindow - Going to set [%s] as header (pay)",text.c_str());

  SET_CONTROL_LABEL(CONTROL_TEXT_LABEL,text);

  if (!m_item.GetProperty("terms").IsEmpty())
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentTou::OnInitWindow - Going to set TextBox [terms=%s] (pay)",m_item.GetProperty("terms").c_str());

    CGUITextBox* textbox = (CGUITextBox*)GetControl(CONTROL_TOU_TEXTBOX);
    if (!textbox)
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentTou::OnInitWindow - FAILED to get TextBox control (pay)");
      CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(55195));
      Close();
      return;
    }

    textbox->SetInfo(CGUIInfoLabel(m_item.GetProperty("terms")));

    CONTROL_ENABLE(CONTROL_TOU_TEXTBOX);
    CONTROL_ENABLE(CONTROL_TOU_TEXTBOX_SCROLL);
  }
  else
  {
    CONTROL_DISABLE(CONTROL_TOU_TEXTBOX);
    CONTROL_DISABLE(CONTROL_TOU_TEXTBOX_SCROLL);
  }

  m_bTermsAccepted = false;
  m_quitPaymentProcess = false;

  SET_CONTROL_SELECTED(GetID(), CONTROL_TERMS_RADIO_BUTTON, m_bTermsAccepted);

  SET_CONTROL_FOCUS(ACCEPT_BUTTON, 0);
}

void CGUIDialogBoxeePaymentTou::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
}

bool CGUIDialogBoxeePaymentTou::OnAction(const CAction& action)
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
  default:
  {
    // do nothing
  }
  break;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeePaymentTou::OnMessage(CGUIMessage& message)
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

bool CGUIDialogBoxeePaymentTou::OnClick(CGUIMessage& message)
{
  bool succeeded = false;

  int iControl = message.GetSenderId();

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentTou::OnClick - Enter function with [iControl=%d] (pay)",iControl);

  switch(iControl)
  {
  case CONTROL_TERMS_RADIO_BUTTON:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentTou::OnClick - Handling click on [%d=CONTROL_TERMS_RADIO_BUTTON] (pay)",iControl);

    succeeded = HandleClickOnAcceptTerms();
  }
  break;
  case ACCEPT_BUTTON:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentTou::OnClick - Handling click on [%d=ACCEPT_BUTTON] (pay)",iControl);

    succeeded = HandleClickOnAcceptButton();
  }
  break;
  case DECLINE_BUTTON:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentTou::OnClick - Handling click on [%d=DECLINE_BUTTON] (pay)",iControl);

    succeeded = HandleClickOnDeclineButton();
  }
  break;
  default:
  {
    CLog::Log(LOGWARNING,"CGUIDialogBoxeePaymentTou::OnClick - UNKNOWN control [%d] was click (pay)",iControl);
  }
  break;
  }

  return succeeded;
}

bool CGUIDialogBoxeePaymentTou::HandleClickOnAcceptTerms()
{
  m_bTermsAccepted = !m_bTermsAccepted;
  SET_CONTROL_SELECTED(GetID(), CONTROL_TERMS_RADIO_BUTTON, m_bTermsAccepted);
  return true;
}

bool CGUIDialogBoxeePaymentTou::HandleClickOnAcceptButton()
{
  if (!m_bTermsAccepted)
  {
    CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(55123),g_localizeStrings.Get(55124));
    return false;
  }
  else
  {
    if (CGUIDialogBoxeePaymentUserData::Show(m_item.GetProperty("product_id"),m_quitPaymentProcess))
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentTou::OnClick - dialog PaymentUserData returned TRUE -> going to close dialog. [quitPaymentProcess=%d] (pay)",m_quitPaymentProcess);
      m_bConfirmed = true;
      Close();
      return true;
    }
    else
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentTou::OnClick - dialog PaymentUserData returned FALSE. [quitPaymentProcess=%d] (pay)",m_quitPaymentProcess);
      return false;
    }
  }
}

bool CGUIDialogBoxeePaymentTou::HandleClickOnDeclineButton()
{
  m_bConfirmed = false;
  Close();
  return true;
}

bool CGUIDialogBoxeePaymentTou::IsQuitPaymentProcess()
{
  return m_quitPaymentProcess;
}

void CGUIDialogBoxeePaymentTou::SetItem(CGUIListItemPtr item)
{
  m_item.Reset();
  m_item = *((CGUIListItem*)item.get());
  //CLog::Log(LOGDEBUG,"CGUIDialogBoxeePaymentsProducts::SetItem - After set item [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-boxeeoffer=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d][quality-lbl=%s][quality=%d][is-hd=%s][productsList=%s] (pay)",m_item.GetLabel().c_str(),m_item.m_strPath.c_str(),m_item.GetThumbnailImage().c_str(),m_item.GetProperty("link-title").c_str(),m_item.GetProperty("link-url").c_str(),m_item.GetProperty("link-boxeetype").c_str(),m_item.GetProperty("link-boxeeoffer").c_str(),m_item.GetProperty("link-type").c_str(),m_item.GetProperty("link-provider").c_str(),m_item.GetProperty("link-providername").c_str(),m_item.GetProperty("link-providerthumb").c_str(),m_item.GetProperty("link-countrycodes").c_str(),m_item.GetPropertyBOOL("link-countryrel"),m_item.GetProperty("quality-lbl").c_str(),m_item.GetPropertyInt("quality"),m_item.GetProperty("is-hd").c_str(),m_item.GetProperty("link-productslist").c_str());
}

bool CGUIDialogBoxeePaymentTou::Show(CGUIListItemPtr item, bool& quitPaymentProcess)
{
  if (!item.get())
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentTou::Show - Enter function with a NULL item (pay)");
    return false;
  }

  CGUIDialogBoxeePaymentTou *dialog = (CGUIDialogBoxeePaymentTou *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_PAYMENT_TOU);
  if (!dialog)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentTou::Show - FAILED to get dialog to show (pay)");
    return false;
  }

  dialog->SetItem(item);
  dialog->DoModal();

  quitPaymentProcess = dialog->IsQuitPaymentProcess();

  return dialog->m_bConfirmed;
}

bool CGUIDialogBoxeePaymentTou::GetPriceLabel(CStdString& priceStr)
{
  priceStr = "";

  if (!m_item.GetProperty("currency_symbol").IsEmpty())
  {
    priceStr = m_item.GetProperty("currency_symbol");
  }
  else if (m_item.GetProperty("currency").IsEmpty())
  {
    priceStr = m_item.GetProperty("currency");
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentTou::Show - FAILED to get [currency] or [currency_symbol] property from item (pay)");
    return false;
  }

  if (!m_item.GetProperty("price").IsEmpty())
  {
    priceStr += m_item.GetProperty("price");
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePaymentTou::Show - FAILED to get [price] property from item (pay)");
    return false;
  }

  return true;
}

bool CGUIDialogBoxeePaymentTou::GetBillingTypeLabel(CStdString& billingTypeStr)
{
  if (!m_item.GetProperty("billing_type").IsEmpty())
  {
    billingTypeStr = m_item.GetProperty("billing_type");
    return true;
  }

  return false;
}

void CGUIDialogBoxeePaymentTou::StartProgressDialog()
{
  m_progress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

  if (m_progress)
  {
    m_progress->StartModal();
    m_progress->Progress();
  }
}

void CGUIDialogBoxeePaymentTou::StopProgressDialog()
{
  if (m_progress)
  {
    m_progress->Close();
  }
}
