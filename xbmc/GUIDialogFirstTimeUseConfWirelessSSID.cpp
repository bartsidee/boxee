#include "GUIDialogFirstTimeUseConfWirelessSSID.h"

#ifdef HAS_EMBEDDED

#include "GUIEditControl.h"
#include "log.h"

#define CONTROL_SSID_EDIT 51

CGUIDialogFirstTimeUseConfWirelessSSID::CGUIDialogFirstTimeUseConfWirelessSSID() : CGUIDialogFirstTimeUseBase(WINDOW_DIALOG_FTU_CONF_WIRELESS_SSID, "ftu_conf_wireless_ssid.xml","CGUIDialogFirstTimeUseConfWirelessSSID")
{
  m_ssid = "";
}

CGUIDialogFirstTimeUseConfWirelessSSID::~CGUIDialogFirstTimeUseConfWirelessSSID()
{
}

void CGUIDialogFirstTimeUseConfWirelessSSID::OnInitWindow()
{
  CGUIDialogFirstTimeUseBase::OnInitWindow();

  if (!m_ssid.IsEmpty())
  {
    ((CGUIEditControl*)GetControl(CONTROL_SSID_EDIT))->SetLabel2(m_ssid);
    CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseConfWireless::OnInitWindow - After set control with [m_ssid=%s] (initbox)",m_ssid.c_str());
  }
}

bool CGUIDialogFirstTimeUseConfWirelessSSID::OnMessage(CGUIMessage& message)
{
  return CGUIDialogFirstTimeUseBase::OnMessage(message);
}

bool CGUIDialogFirstTimeUseConfWirelessSSID::HandleClickNext()
{
  m_ssid = ((CGUIEditControl*)GetControl(CONTROL_SSID_EDIT))->GetLabel2();

  CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseConfWirelessSSID::HandleClickNext - After set [m_ssid=%s] (initbox)",m_ssid.c_str());

  return true;
}

bool CGUIDialogFirstTimeUseConfWirelessSSID::HandleClickBack()
{
  // nothing to do

  return true;
}

CStdString CGUIDialogFirstTimeUseConfWirelessSSID::GetSSID()
{
  return m_ssid;
}

void CGUIDialogFirstTimeUseConfWirelessSSID::SetSSID(const CStdString& ssid)
{
  m_ssid = ssid;
}

#endif

