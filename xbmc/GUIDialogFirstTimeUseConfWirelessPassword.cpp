#include "GUIDialogFirstTimeUseConfWirelessPassword.h"

#ifdef HAS_EMBEDDED

#include "GUIDialogFirstTimeUseWireless.h"
#include "GUIWindowManager.h"
#include "LocalizeStrings.h"
#include "GUILabelControl.h"
#include "GUIEditControl.h"
#include "GUIRadioButtonControl.h"
#include "GUIDialogOK2.h"
#include "log.h"

#define CONTROL_PASSWORD_LABEL 50
#define CONTROL_PASSWORD_EDIT 51
#define CONTROL_PASSWORD_SHOW 52

CGUIDialogFirstTimeUseConfWirelessPassword::CGUIDialogFirstTimeUseConfWirelessPassword() : CGUIDialogFirstTimeUseBase(WINDOW_DIALOG_FTU_CONF_WIRELESS_PASSWORD,"ftu_conf_wireless_password.xml","CGUIDialogFirstTimeUseConfWirelessPassword")
{
  m_password = "";
}

CGUIDialogFirstTimeUseConfWirelessPassword::~CGUIDialogFirstTimeUseConfWirelessPassword()
{

}

void CGUIDialogFirstTimeUseConfWirelessPassword::OnInitWindow()
{
  CGUIDialogFirstTimeUseBase::OnInitWindow();

  CGUILabelControl* passwordLabelCtrl = (CGUILabelControl*)GetControl(CONTROL_PASSWORD_LABEL);
  if (passwordLabelCtrl)
  {
    CStdString label;

    //  if (m_networkName.IsEmpty())
    //  {
    //    label = g_localizeStrings.Get(54619);
    //  }
    //  else
    //  {
    //    if (!m_isNetworkRequiresPassword)
    //    {
    //      CStdString str = g_localizeStrings.Get(54618);
    //      label.Format(str.c_str(), m_networkName);
    //    }
    //    else
    //    {
    //      CStdString str = g_localizeStrings.Get(54690);
    //      label.Format(str.c_str(), m_networkName);
    //    }
    //  }

    label = g_localizeStrings.Get(54619);
    passwordLabelCtrl->SetLabel(label);
  }

  m_password = "";
  ((CGUIEditControl*)GetControl(CONTROL_PASSWORD_EDIT))->SetLabel2(m_password);
}

bool CGUIDialogFirstTimeUseConfWirelessPassword::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    int senderId = message.GetSenderId();

    switch (senderId)
    {
    case CONTROL_PASSWORD_SHOW:
    {
      CGUIRadioButtonControl* pControl = (CGUIRadioButtonControl*)GetControl(CONTROL_PASSWORD_SHOW);
      if (pControl)
      {
        if (pControl->IsSelected())
        {
          ((CGUIEditControl*)GetControl(CONTROL_PASSWORD_EDIT))->SetInputType(CGUIEditControl::INPUT_TYPE_TEXT,0);
        }
        else
        {
          ((CGUIEditControl*)GetControl(CONTROL_PASSWORD_EDIT))->SetInputType(CGUIEditControl::INPUT_TYPE_PASSWORD,0);
        }
      }

      return true;
    }
    break;
    }
  }
  break;
  }

  return CGUIDialogFirstTimeUseBase::OnMessage(message);
}

bool CGUIDialogFirstTimeUseConfWirelessPassword::HandleClickNext()
{
  m_password = ((CGUIEditControl*)GetControl(CONTROL_PASSWORD_EDIT))->GetLabel2();

  //CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseConfWirelessPassword::HandleHitNextButton - After set [Password=%s] (initbox)",m_password.c_str());

  return true;
}

bool CGUIDialogFirstTimeUseConfWirelessPassword::HandleClickBack()
{
  // nothing to do

  return true;
}

CStdString CGUIDialogFirstTimeUseConfWirelessPassword::GetNetworkName()
{
  return m_networkName;
}

void CGUIDialogFirstTimeUseConfWirelessPassword::SetNetworkName(const CStdString& networkName, bool isNetworkRequiresPassword)
{
  m_networkName = networkName;
  m_isNetworkRequiresPassword = isNetworkRequiresPassword;
}

CStdString CGUIDialogFirstTimeUseConfWirelessPassword::GetPassword()
{
  return m_password;
}

void CGUIDialogFirstTimeUseConfWirelessPassword::SetPassword(const CStdString& password)
{
  m_password = password;
}

#endif

