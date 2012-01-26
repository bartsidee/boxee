#include "GUIDialogFirstTimeUseConfWireless.h"
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
#define AUTOMATIC_CONF 281
#define MANUALLY_CONF 282

CGUIDialogFirstTimeUseConfWireless::CGUIDialogFirstTimeUseConfWireless() : CGUIDialogFirstTimeUseBase(WINDOW_DIALOG_FTU_CONF_WIRELESS_PASSWORD,"ftu_conf_wireless.xml","CGUIDialogFirstTimeUseConfWireless")
{
  m_buttonSelected = 0;
  m_password = "";
}

CGUIDialogFirstTimeUseConfWireless::~CGUIDialogFirstTimeUseConfWireless()
{

}

void CGUIDialogFirstTimeUseConfWireless::OnInitWindow()
{
  CGUIDialogFirstTimeUseBase::OnInitWindow();

  if (m_buttonSelected != 0)
  {
    switch(m_buttonSelected)
    {
    case AUTOMATIC_CONF:
    {
      SetSelectAUTOMATIC();
    }
    break;
    case MANUALLY_CONF:
    {
      SetSelectMANUALLY();
    }
    break;
    }
  }

  // TODO: check if need to show password

  CStdString wirelessNetworkName = "";
  CGUIDialogFirstTimeUseWireless* pWirelessDialog = (CGUIDialogFirstTimeUseWireless*)g_windowManager.GetWindow(WINDOW_DIALOG_FTU_WIRELESS);
  if (pWirelessDialog)
  {
    wirelessNetworkName = pWirelessDialog->GetSelectedWirelessName();
  }

  CStdString label;
  CStdString str = g_localizeStrings.Get(54690);

  label.Format(str.c_str(), wirelessNetworkName);

  ((CGUILabelControl*)GetControl(CONTROL_PASSWORD_LABEL))->SetLabel(label);

  if (!m_password.IsEmpty())
  {
    ((CGUIEditControl*)GetControl(CONTROL_PASSWORD_EDIT))->SetLabel2(m_password);
    CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseConfWireless::OnInitWindow - After set control with [m_password=%s] (initbox)",m_password.c_str());
  }
}

bool CGUIDialogFirstTimeUseConfWireless::OnMessage(CGUIMessage& message)
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
        //((CGUIEditControl*)GetControl(CONTROL_PASSWORD_EDIT))->
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
    case AUTOMATIC_CONF:
    {
      SetSelectAUTOMATIC();
      return true;
    }
    break;
    case MANUALLY_CONF:
    {
      SetSelectMANUALLY();
      return true;
    }
    break;
    }
  }
  break;
  }

  return CGUIDialogFirstTimeUseBase::OnMessage(message);
}

bool CGUIDialogFirstTimeUseConfWireless::HandleClickNext()
{
  if (m_buttonSelected == 0)
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseConfWireless::HandleHitNextButton - No buton was chose (initbox)");

    CGUIDialogOK2::ShowAndGetInput(54679,54668);

    SET_CONTROL_FOCUS(AUTOMATIC_CONF, 0);

    return false;
  }

  m_password = ((CGUIEditControl*)GetControl(CONTROL_PASSWORD_EDIT))->GetLabel2();

  CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseConfWireless::HandleHitNextButton - After set [Password=%s] (initbox)",m_password.c_str());

  return true;
}

bool CGUIDialogFirstTimeUseConfWireless::HandleClickBack()
{
  // nothing to do

  return true;
}

void CGUIDialogFirstTimeUseConfWireless::SetSelectAUTOMATIC()
{
  SET_CONTROL_SELECTED(GetID(), AUTOMATIC_CONF, true);
  SET_CONTROL_SELECTED(GetID(), MANUALLY_CONF, false);
  m_buttonSelected = AUTOMATIC_CONF;
}

void CGUIDialogFirstTimeUseConfWireless::SetSelectMANUALLY()
{
  SET_CONTROL_SELECTED(GetID(), AUTOMATIC_CONF, false);
  SET_CONTROL_SELECTED(GetID(), MANUALLY_CONF, true);
  m_buttonSelected = MANUALLY_CONF;
}

int CGUIDialogFirstTimeUseConfWireless::GetChoiceSelected()
{
  return m_buttonSelected;
}

CStdString CGUIDialogFirstTimeUseConfWireless::GetPassword()
{
  return m_password;
}
