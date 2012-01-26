#include "GUIDialogFirstTimeUseConfNetwork.h"

#ifdef HAS_EMBEDDED

#include "GUIEditControl.h"
#include "GUIDialogOK2.h"
#include "utils/RegExp.h"
#include "log.h"

#define DEFAULT_IP_ADDRESS ""
#define DEFAULT_NETMASK ""
#define DEFAULT_GATEWAY ""
#define DEFAULTL_DNS ""

#define IP_REGEXP "^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$"
#define CONTROL_IP_ADDRESS_EDIT 501
#define CONTROL_NETMASK_EDIT 511
#define CONTROL_GATEWAY_EDIT 521
#define CONTROL_DNS_EDIT 531

CGUIDialogFirstTimeUseConfNetwork::CGUIDialogFirstTimeUseConfNetwork() : CGUIDialogFirstTimeUseBase(WINDOW_DIALOG_FTU_CONF_NETWORK,"ftu_conf_network.xml","CGUIDialogFirstTimeUseConfNetwork")
{
  m_ipAddress = DEFAULT_IP_ADDRESS;
  m_netmask = DEFAULT_NETMASK;
  m_gateway = DEFAULT_GATEWAY;
  m_dns = DEFAULTL_DNS;
}

CGUIDialogFirstTimeUseConfNetwork::~CGUIDialogFirstTimeUseConfNetwork()
{

}

void CGUIDialogFirstTimeUseConfNetwork::OnInitWindow()
{
  CGUIDialogFirstTimeUseBase::OnInitWindow();

  ((CGUIEditControl*)GetControl(CONTROL_IP_ADDRESS_EDIT))->SetInputType(CGUIEditControl::INPUT_TYPE_IPADDRESS,0);
  ((CGUIEditControl*)GetControl(CONTROL_IP_ADDRESS_EDIT))->SetLabel2(m_ipAddress);
  CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseConfNetwork::OnInitWindow - After set control with [IpAddress=%s] (initbox)",m_ipAddress.c_str());

  ((CGUIEditControl*)GetControl(CONTROL_NETMASK_EDIT))->SetInputType(CGUIEditControl::INPUT_TYPE_IPADDRESS,0);
  ((CGUIEditControl*)GetControl(CONTROL_NETMASK_EDIT))->SetLabel2(m_netmask);
  CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseConfNetwork::OnInitWindow - After set control with [Netmask=%s] (initbox)",m_netmask.c_str());

  ((CGUIEditControl*)GetControl(CONTROL_GATEWAY_EDIT))->SetInputType(CGUIEditControl::INPUT_TYPE_IPADDRESS,0);
  ((CGUIEditControl*)GetControl(CONTROL_GATEWAY_EDIT))->SetLabel2(m_gateway);
  CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseConfNetwork::OnInitWindow - After set control with [Gateway=%s] (initbox)",m_gateway.c_str());

  ((CGUIEditControl*)GetControl(CONTROL_DNS_EDIT))->SetInputType(CGUIEditControl::INPUT_TYPE_IPADDRESS,0);
  ((CGUIEditControl*)GetControl(CONTROL_DNS_EDIT))->SetLabel2(m_dns);
  CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseConfNetwork::OnInitWindow - After set control with [DNS=%s] (initbox)",m_dns.c_str());

  SET_CONTROL_FOCUS(CONTROL_IP_ADDRESS_EDIT, 0);
}

bool CGUIDialogFirstTimeUseConfNetwork::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    int senderId = message.GetSenderId();

    CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseConfNetwork::OnMessage - GUI_MSG_CLICKED - [buttonId=%d] (initbox)",senderId);

    switch (senderId)
    {
    case BUTTON_NEXT:
    {
      if (!ValidateFields())
      {
        CGUIDialogOK2::ShowAndGetInput(257,54653);
        return true;
      }
    }
    break;
    }
  }
  }

  return CGUIDialogFirstTimeUseBase::OnMessage(message);
}

bool CGUIDialogFirstTimeUseConfNetwork::ValidateFields()
{
  CRegExp reg;
  reg.RegComp(IP_REGEXP);
  CStdString editStr = "";

  // check IpAddress value
  editStr = ((CGUIEditControl*)GetControl(CONTROL_IP_ADDRESS_EDIT))->GetLabel2();
  if (reg.RegFind(editStr.c_str()) == -1)
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseConfNetwork::ValidateFields - [ipaddress=%s] is NOT valid (initbox)",editStr.c_str());
    return false;
  }

  // check IpAddress value
  editStr = ((CGUIEditControl*)GetControl(CONTROL_NETMASK_EDIT))->GetLabel2();
  if (reg.RegFind(editStr.c_str()) == -1)
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseConfNetwork::ValidateFields - [netmask=%s] is NOT valid (initbox)",editStr.c_str());
    return false;
  }

  // check IpAddress value
  editStr = ((CGUIEditControl*)GetControl(CONTROL_GATEWAY_EDIT))->GetLabel2();
  if (reg.RegFind(editStr.c_str()) == -1)
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseConfNetwork::ValidateFields - [gateway=%s] is NOT valid (initbox)",editStr.c_str());
    return false;
  }

  // check IpAddress value
  editStr = ((CGUIEditControl*)GetControl(CONTROL_DNS_EDIT))->GetLabel2();
  if (reg.RegFind(editStr.c_str()) == -1)
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseConfNetwork::ValidateFields - [dns=%s] is NOT valid (initbox)",editStr.c_str());
    return false;
  }

  return true;
}

bool CGUIDialogFirstTimeUseConfNetwork::HandleClickNext()
{
  m_ipAddress = ((CGUIEditControl*)GetControl(CONTROL_IP_ADDRESS_EDIT))->GetLabel2();
  CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseConfNetwork::HandleHitNextButton - After set [IpAddress=%s] (initbox)",m_ipAddress.c_str());

  m_netmask = ((CGUIEditControl*)GetControl(CONTROL_NETMASK_EDIT))->GetLabel2();
  CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseConfNetwork::HandleHitNextButton - After set [Netmask=%s] (initbox)",m_netmask.c_str());

  m_gateway = ((CGUIEditControl*)GetControl(CONTROL_GATEWAY_EDIT))->GetLabel2();
  CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseConfNetwork::HandleHitNextButton - After set [Gateway=%s] (initbox)",m_gateway.c_str());

  m_dns = ((CGUIEditControl*)GetControl(CONTROL_DNS_EDIT))->GetLabel2();
  CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseConfNetwork::HandleHitNextButton - After set [DNS=%s] (initbox)",m_dns.c_str());

  return true;
}

bool CGUIDialogFirstTimeUseConfNetwork::HandleClickBack()
{
  // nothing to do

  return true;
}

CStdString CGUIDialogFirstTimeUseConfNetwork::GetIpAddress()
{
  return m_ipAddress;
}

CStdString CGUIDialogFirstTimeUseConfNetwork::GetNetmask()
{
  return m_netmask;
}

CStdString CGUIDialogFirstTimeUseConfNetwork::GetGateway()
{
  return m_gateway;
}

CStdString CGUIDialogFirstTimeUseConfNetwork::GetDNS()
{
  return m_dns;
}

#endif

