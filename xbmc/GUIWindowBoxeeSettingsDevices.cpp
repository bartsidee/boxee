#include "GUIWindowBoxeeSettingsDevices.h"
#include "LocalizeStrings.h"
#include "log.h"
#include "Application.h"
#include "BoxeeDeviceManager.h"
#include "GUIDialogOK2.h"
#include "GUIDialogYesNo2.h"
#include "GUIUserMessages.h"

#define CONTROL_DEVICE_BUTTON  51
#define CONTROL_DEVICE_LIST    56

#define DEVICE_ICON_PROPERTY_NAME  "deviceicon"

CGUIWindowBoxeeSettingsDevices::CGUIWindowBoxeeSettingsDevices() : CGUIDialog(WINDOW_BOXEE_SETTINGS_DEVICES, "boxee_settings_devices.xml")
{

}

CGUIWindowBoxeeSettingsDevices::~CGUIWindowBoxeeSettingsDevices()
{

}

void CGUIWindowBoxeeSettingsDevices::OnInitWindow()
{
  CGUIWindow::OnInitWindow();

  SET_CONTROL_FOCUS(CONTROL_DEVICE_BUTTON,0);

  LoadPairedDevicesList();
}

bool CGUIWindowBoxeeSettingsDevices::OnAction(const CAction &action)
{
  switch (action.id)
  {
  case ACTION_PREVIOUS_MENU:
  case ACTION_PARENT_DIR:
  {
    Close();
    return true;
  }
  }

  return CGUIWindow::OnAction(action);
}

bool CGUIWindowBoxeeSettingsDevices::OnMessage(CGUIMessage &message)
{
  switch(message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();

    if(iControl == CONTROL_DEVICE_LIST)
    {
      return HandleClickOnPairedDeviceList();
    }
  }
  break;
  case GUI_MSG_UPDATE:
  {
    LoadPairedDevicesList();
  }
  break;
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIWindowBoxeeSettingsDevices::HandleClickOnPairedDeviceList()
{
  // Get selected index from the list
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_DEVICE_LIST);
  OnMessage(msg);

  int selectedIndex = msg.GetParam1();

  if (selectedIndex < 0 || selectedIndex > m_pairedDeviceList.Size() - 1)
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeSettingsDevices::HandleClickOnPairedDeviceList - GUI_MSG_CLICKED - FAILED to get [selectedIndex=%d]. [ListSize=%d] (bspd)",selectedIndex,m_pairedDeviceList.Size());
    return true;
  }

  CFileItemPtr m_selectedItem = m_pairedDeviceList.Get(selectedIndex);
  CStdString deviceLabel = m_selectedItem->GetLabel();
  CStdString deviceId = m_selectedItem->GetProperty("deviceid");

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeSettingsDevices::HandleClickOnPairedDeviceList - GUI_MSG_CLICKED - clicked on device [label=%s][id=%s] in CONTROL_DEVICE_LIST (bspd)",deviceLabel.c_str(),deviceId.c_str());

  if (deviceId.IsEmpty())
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeSettingsDevices::HandleClickOnPairedDeviceList - GUI_MSG_CLICKED - got device with an EMPTY deviceId. [label=%s][id=%s] (bspd)",deviceLabel.c_str(),deviceId.c_str());
    return true;
  }

  CStdString message;
  CStdString messageStr = g_localizeStrings.Get(55515);
  message.Format(messageStr.c_str(),deviceLabel);

  if (CGUIDialogYesNo2::ShowAndGetInput(g_localizeStrings.Get(55510), message, g_localizeStrings.Get(222), g_localizeStrings.Get(53405)))
  {
    if (!g_application.GetBoxeeDeviceManager().UnPairDevice(deviceId))
    {
      CStdString message;
      CStdString messageStr = g_localizeStrings.Get(55516);
      message.Format(messageStr.c_str(),deviceLabel);

      CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(55510), message);
    }
    else
    {
      LoadPairedDevicesList();
    }
  }

  return true;
}

void CGUIWindowBoxeeSettingsDevices::LoadPairedDevicesList()
{
  m_pairedDeviceList.Clear();
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_DEVICE_LIST);
  OnMessage(msgReset);

  const std::map<CStdString, CBoxeeDeviceItem>& pairedDeviceMap = g_application.GetBoxeeDeviceManager().GetPairedDeviceMap();
  size_t numOfPairedDevices = pairedDeviceMap.size();

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeSettingsDevices::OnInitWindow - got [NumOfPairedDevice=%zu] (bspd)",numOfPairedDevices);

  int counter = 0;
  std::map<CStdString, CBoxeeDeviceItem>::const_iterator it = pairedDeviceMap.begin();
  while (it != pairedDeviceMap.end())
  {
    const CBoxeeDeviceItem& deviceItem = it->second;
    CFileItemPtr item(new CFileItem());
    item->SetLabel(deviceItem.GetDeviceLabel());
    item->SetProperty("deviceid",deviceItem.GetDeviceId());
    item->SetProperty(DEVICE_ICON_PROPERTY_NAME,deviceItem.GetDeviceTypeAsString());

    counter++;
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeSettingsDevices::OnInitWindow - [%d/%zu] - adding device [label=%s] (bspd)",counter,numOfPairedDevices,item->GetLabel().c_str());

    m_pairedDeviceList.Add(item);

    it++;
  }

  if (m_pairedDeviceList.Size() > 0)
  {
    m_pairedDeviceList.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);

    // bind paired devices
    CGUIMessage message2(GUI_MSG_LABEL_BIND, GetID(), CONTROL_DEVICE_LIST, 0, 0, &m_pairedDeviceList);
    OnMessage(message2);

    SET_CONTROL_FOCUS(CONTROL_DEVICE_LIST,0);
  }
}
