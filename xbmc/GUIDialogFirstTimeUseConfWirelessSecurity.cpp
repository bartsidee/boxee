#include "GUIDialogFirstTimeUseConfWirelessSecurity.h"

#ifdef HAS_EMBEDDED

#include "HalServices.h"
#include "FileItem.h"
#include "log.h"

#define AUTH_NONE_LABEL "None"
#define AUTH_WEP_PASSPHRASE_LABEL "WEP 128-bit passphrase"
#define AUTH_WEP_KEY_LABEL "WEP 40-bit key"
#define AUTH_WPAPSK_LABEL "WPA Personal"
#define AUTH_WPA2PSK_LABEL "WPA2 Personal"



static int nofAuths = 5;
static const char* authStrs[] = { AUTH_NONE_LABEL, AUTH_WEP_PASSPHRASE_LABEL, AUTH_WEP_KEY_LABEL, AUTH_WPAPSK_LABEL, AUTH_WPA2PSK_LABEL };

#define LIST_CTRL 240

CGUIDialogFirstTimeUseConfWirelessSecurity::CGUIDialogFirstTimeUseConfWirelessSecurity() : CGUIDialogFirstTimeUseWithList(WINDOW_DIALOG_FTU_CONF_WIRELESS_SECURITY,"ftu_conf_wireless_security.xml","CGUIDialogFirstTimeUseConfWirelessSecurity")
{

}

CGUIDialogFirstTimeUseConfWirelessSecurity::~CGUIDialogFirstTimeUseConfWirelessSecurity()
{

}

void CGUIDialogFirstTimeUseConfWirelessSecurity::OnInitWindow()
{
  CGUIDialogFirstTimeUseWithList::OnInitWindow();

  SetAuth(AUTH_NONE);
}

bool CGUIDialogFirstTimeUseConfWirelessSecurity::FillListOnInit()
{
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), LIST_CTRL);
  OnMessage(msgReset);

  for (int i = 0; i < nofAuths; i++)
  {
    CFileItemPtr item(new CFileItem(authStrs[i]));
    m_listItems.Add(item);
  }

  // bind new items
  CGUIMessage messageBind(GUI_MSG_LABEL_BIND, GetID(), LIST_CTRL, 0, 0, &m_listItems);
  OnMessage(messageBind);

  return true;
}

bool CGUIDialogFirstTimeUseConfWirelessSecurity::HandleListChoice()
{
  SetAuth(CGUIDialogFirstTimeUseConfWirelessSecurity::GetAuthLabelAsEnum(m_selectedItem->GetLabel()));
  return true;
}

bool CGUIDialogFirstTimeUseConfWirelessSecurity::OnMessage(CGUIMessage& message)
{
  return CGUIDialogFirstTimeUseWithList::OnMessage(message);
}

void CGUIDialogFirstTimeUseConfWirelessSecurity::SetAuth(CHalWirelessAuthType auth)
{
  m_auth = auth;
}

CHalWirelessAuthType CGUIDialogFirstTimeUseConfWirelessSecurity::GetAuth()
{
  return m_auth;
}

bool CGUIDialogFirstTimeUseConfWirelessSecurity::HandleClickNext()
{
  return true;
}

bool CGUIDialogFirstTimeUseConfWirelessSecurity::HandleClickBack()
{
  // nothing to do

  return true;
}

CHalWirelessAuthType CGUIDialogFirstTimeUseConfWirelessSecurity::GetAuthLabelAsEnum(const CStdString& authLabel)
{
  if (authLabel == AUTH_NONE_LABEL)
  {
    return AUTH_NONE;
  }
  else if (authLabel == AUTH_WEP_PASSPHRASE_LABEL)
  {
    return AUTH_WEP_PASSPHRASE;
  }
  else if (authLabel == AUTH_WEP_KEY_LABEL)
  {
    return AUTH_WEP_KEY;
  }
  else if (authLabel == AUTH_WPAPSK_LABEL)
  {
    return AUTH_WPAPSK;
  }
  else if (authLabel == AUTH_WPA2PSK_LABEL)
  {
    return AUTH_WPA2PSK;
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseConfWirelessSecurity::GetAuthLabelAsEnum - FAILED to parse auth label [%s]. Return [AUthEnum=%d=AUTH_DONTCARE] (initbox)",authLabel.c_str(),AUTH_DONTCARE);
    return AUTH_DONTCARE;
  }
}

CStdString CGUIDialogFirstTimeUseConfWirelessSecurity::GetAuthAsString(CHalWirelessAuthType auth)
{
  switch(auth)
  {
  case AUTH_NONE:
  {
    return "AUTH_NONE";
  }
  break;
  case AUTH_DONTCARE:
  {
    return "AUTH_DONTCARE";
  }
  break;
  case AUTH_WEP_PASSPHRASE:
  {
    return "AUTH_WEP_PASSPHRASE";
  }
  break;
  case AUTH_WEP_KEY:
  {
    return "AUTH_WEP_KEY";
  }
  break;
  case AUTH_WPAPSK:
  {
    return "AUTH_WPAPSK";
  }
  break;
  case AUTH_WPA2PSK:
  {
    return "AUTH_WPA2PSK";
  }
  break;
  default:
  {
    return "UNKNOWN";
  }
  break;
  }
}

#endif
