#include "GUIDialogBoxeeLiveTvEditChannels.h"

#ifdef HAS_DVB
#include "Util.h"
#include <boost/foreach.hpp>
#include "Application.h"
#include "GUIWindowManager.h"
#include "cores/dvb/dvbmanager.h"
#include "utils/log.h"
#include "GUIListContainer.h"
#include "LiveTvModel.h"
#include "FileItem.h"
#include "GUIDialogProgress.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "lib/libBoxee/bxutils.h"
#include "LocalizeStrings.h"
#include "GUIEditControl.h"

using namespace BOXEE;

#define GROUP_CHANNEL_LINEUP             1
#define GROUP_EDIT_CHANNEL               2
#define CONTROL_LIST_CHANNELS            46
#define CONTROL_BUTTON_EDIT              61

#define CONTROL_EDIT_CHANNEL_NAME        21
#define CONTROL_LIST_SCHEDULES           22

CGUIDialogBoxeeLiveTvEditChannels::CGUIDialogBoxeeLiveTvEditChannels() : CGUIDialog(WINDOW_DIALOG_BOXEE_LIVETV_EDIT_CHANNELS, "boxee_livetv_edit_channels.xml")
{
  m_scheduleLoaded = false;
  m_selectedChannelItem = 0;
}

CGUIDialogBoxeeLiveTvEditChannels::~CGUIDialogBoxeeLiveTvEditChannels()
{
}

void CGUIDialogBoxeeLiveTvEditChannels::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  if (!LoadChannelsList())
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLiveTvEditChannels::OnInitWindow - FAILED to load channels (ltvc)");
    Close();
    return;
  }

  m_dirty = false;
}

void CGUIDialogBoxeeLiveTvEditChannels::OnDeinitWindow(int nextWindowID)
{
  if (m_dirty)
  {
#ifdef HAS_SERVER_OTA
    CGUIDialogProgress* progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    progress->StartModal();
    progress->Progress();

    if (!DVBManager::GetInstance().GetChannels().SaveChannelsToServer())
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeeLiveTvEditChannels::OnDeinitWindow - failed to save channels to server");
    }

    // Reload the EPG, in case the user changed channel info
    DVBManager::GetInstance().GetEpgServerLoader().RequestLoad();

    progress->Close();
#endif
  }

  CGUIDialog::OnDeinitWindow(nextWindowID);
}

bool CGUIDialogBoxeeLiveTvEditChannels::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_LABEL_BIND:
  {
    if (message.GetPointer() && message.GetControlId() == 0)
    {
      CFileItemList* items = (CFileItemList*)message.GetPointer();
      delete items;
      return true;
    }
  }
  break;

  case GUI_MSG_CLICKED:
  {
    if (message.GetSenderId() == CONTROL_LIST_CHANNELS)
    {
      return HandleClickedChannel();
    }
    else if (message.GetSenderId() == CONTROL_BUTTON_EDIT)
    {
      return HandleClickedEditChannelButton();
    }
    else if (message.GetSenderId() == CONTROL_LIST_SCHEDULES)
    {
      return HandleClickedEditChannelList();
    }
  }
  break;

  default:
  break;
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogBoxeeLiveTvEditChannels::OnAction(const CAction& action)
{
  switch (action.id)
  {
  case ACTION_PARENT_DIR:
  case ACTION_PREVIOUS_MENU:
  {
    CGUIBaseContainer* pControl = (CGUIBaseContainer*)GetControl(GROUP_CHANNEL_LINEUP);
    if (pControl && pControl->IsVisible())
    {
      Close();
    }
    else
    {
      CloseScheduleList();
    }

    return true;
  }
  break;

  default:
  break;
  }

  return CGUIDialog::OnAction(action);
}

void CGUIDialogBoxeeLiveTvEditChannels::CloseScheduleList()
{
  // Get the channel that was selected in the first screen
  CGUIBaseContainer* pContainer = (CGUIBaseContainer*)GetControl(CONTROL_LIST_CHANNELS);
  CGUIListItemPtr clickedChannel = pContainer->GetSelectedItemPtr();
  DvbChannelPtr channel = DVBManager::GetInstance().GetChannels().GetChannelByIndex(clickedChannel->GetPropertyInt("channel-id"));
  CStdString oldChannelName = channel->GetChannelLabel().Trim();

  // Get the channel information as the user typed or selected it
  pContainer = (CGUIBaseContainer*)GetControl(CONTROL_LIST_SCHEDULES);
  clickedChannel = pContainer->GetSelectedItemPtr();
  CStdString newChannelNumber = clickedChannel->GetProperty("ChannelNumber");
  CStdString newChannelName =  ((CGUIEditControl*)GetControl(CONTROL_EDIT_CHANNEL_NAME))->GetLabel2();

//  printf("****** NUMBER: |%s| |%s|\n", channel->GetChannelNumber().c_str(), newChannelNumber.c_str());
//  printf("****** NAME: |%s| |%s|\n", oldChannelName.c_str(), newChannelName.c_str());

  bool somethingChanged = false;
  if (channel->GetChannelNumber() != newChannelNumber)
  {
    // If the user clicked on the "Unknown schedule", revert to original values
    if (clickedChannel->HasProperty("OriginalChannelInfo"))
    {
      newChannelNumber = "";
    }

    channel->SetChannelManualNumber(newChannelNumber);
    channel->SetChannelManualName(newChannelName);

    somethingChanged = true;
  }
  else if (oldChannelName != newChannelName)
  {
    channel->SetChannelManualName(newChannelName);
    somethingChanged = true;
  }

  if (somethingChanged)
  {
    m_dirty = true;
    LoadChannelsList();
  }

  SET_CONTROL_HIDDEN(GROUP_EDIT_CHANNEL);
  SET_CONTROL_VISIBLE(GROUP_CHANNEL_LINEUP);
  SET_CONTROL_FOCUS(CONTROL_LIST_CHANNELS, m_selectedChannelItem);
}

bool CGUIDialogBoxeeLiveTvEditChannels::LoadChannelsList()
{
  LiveTvModel model;
  LiveTvModelChannelsType channels = model.GetChannels();

  CFileItemList channelsList;

  CStdString currentChannelNumber = DVBManager::GetInstance().GetCurrentChannel()->GetChannelNumber();
  int selectedItem = 0;

  foreach (LiveTvModelChannel channel, channels)
  {
    CStdString label = channel.number;
    if (!label.IsEmpty())
        label += " ";
    label += channel.label;

    CFileItemPtr channelItem(new CFileItem(label));
    channelItem->SetProperty("ChannelName", channel.label);
    channelItem->SetProperty("ChannelEnabled", channel.enabled);
    channelItem->SetProperty("channel-id", channel.id);

    if (channel.number == currentChannelNumber)
      selectedItem = channelsList.Size();

    channelsList.Add(channelItem);
  }

  CGUIMessage msgBind(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_CHANNELS, 0,0, &channelsList);
  OnMessage(msgBind);

  SET_CONTROL_FOCUS(CONTROL_LIST_CHANNELS, selectedItem);

  return true;
}

bool CGUIDialogBoxeeLiveTvEditChannels::HandleClickedChannel()
{
  CGUIBaseContainer* pContainer = (CGUIBaseContainer*)GetControl(CONTROL_LIST_CHANNELS);
  if (!pContainer)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLiveTvEditChannels::HandleClickedChannel - FAILED to get CONTROL_LIST_CHANNELS object (ltvc)");
    return true;
  }

  CGUIListItemPtr clickedChannel = pContainer->GetSelectedItemPtr();
  clickedChannel->SetProperty("ChannelEnabled", !clickedChannel->GetPropertyBOOL("ChannelEnabled"));

  DvbChannelPtr channel = DVBManager::GetInstance().GetChannels().GetChannelByIndex(clickedChannel->GetPropertyInt("channel-id"));
  channel->SetEnabled(!channel->IsEnabled());

  m_dirty = true;

  return true;
}

bool CGUIDialogBoxeeLiveTvEditChannels::HandleClickedEditChannelButton()
{
  m_scheduleLoaded = LoadAvailableChannelList();
  if (!m_scheduleLoaded)
  {
    // TBD ERROR MSG
    return false;
  }

  CGUIBaseContainer* pContainer = (CGUIBaseContainer*)GetControl(CONTROL_LIST_CHANNELS);
  if (!pContainer)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLiveTvEditChannels::HandleClickedChannel - FAILED to get CONTROL_LIST_CHANNELS object (ltvc)");
    return true;
  }

  m_selectedChannelItem = pContainer->GetSelectedItem();
  CGUIListItemPtr clickedChannel = pContainer->GetSelectedItemPtr();
  DvbChannelPtr channel = DVBManager::GetInstance().GetChannels().GetChannelByIndex(clickedChannel->GetPropertyInt("channel-id"));
  if (channel)
  {
    SET_CONTROL_LABEL2(CONTROL_EDIT_CHANNEL_NAME, channel->GetChannelLabel().Trim());
  }

  CFileItemList schedulesList;
  int selectedItem = 0;

  CFileItemPtr unknownSchedule(new CFileItem(g_localizeStrings.Get(90321)));
  unknownSchedule->SetProperty("ChannelNumber", channel->GetVirtualChannel());
  unknownSchedule->SetProperty("ChannelName", channel->GetServiceName());
  unknownSchedule->SetProperty("OriginalChannelInfo", true);
  schedulesList.Add(unknownSchedule);

  foreach (Schedule schedule, m_schedules)
  {
    CStdString label;
    label.Format("%s %s", schedule.first.c_str(), schedule.second.c_str());
    CFileItemPtr scheduleItem(new CFileItem(label));
    scheduleItem->SetProperty("ChannelNumber", schedule.first);
    scheduleItem->SetProperty("ChannelName", schedule.second);

    if (schedule.first == channel->GetChannelNumber())
      selectedItem = schedulesList.Size();

    schedulesList.Add(scheduleItem);
  }

  CGUIMessage msgBind(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_SCHEDULES, 0,0, &schedulesList);
  OnMessage(msgBind);

  CGUIMessage winmsg(GUI_MSG_UNMARK_ALL_ITEMS, GetID(), CONTROL_LIST_SCHEDULES);
  OnMessage(winmsg);

  CGUIMessage winmsg1(GUI_MSG_MARK_ITEM, GetID(),CONTROL_LIST_SCHEDULES, selectedItem);
  OnMessage(winmsg1);

  SET_CONTROL_FOCUS(CONTROL_LIST_SCHEDULES, selectedItem);

  SET_CONTROL_HIDDEN(GROUP_CHANNEL_LINEUP);
  SET_CONTROL_VISIBLE(GROUP_EDIT_CHANNEL);
  SET_CONTROL_FOCUS(CONTROL_EDIT_CHANNEL_NAME, 0);

  return true;
}

bool CGUIDialogBoxeeLiveTvEditChannels::HandleClickedEditChannelList()
{
  CGUIBaseContainer* pContainer = (CGUIBaseContainer*)GetControl(CONTROL_LIST_SCHEDULES);
  if (!pContainer)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLiveTvEditChannels::HandleClickedEditChannelList - FAILED to get CONTROL_LIST_CHANNELS object (ltvc)");
    return true;
  }

  CGUIListItemPtr clickedChannel = pContainer->GetSelectedItemPtr();
  int selctedItemIndex = pContainer->GetSelectedItem();

  CGUIMessage winmsg(GUI_MSG_UNMARK_ALL_ITEMS, GetID(), CONTROL_LIST_SCHEDULES);
  OnMessage(winmsg);

  CGUIMessage winmsg1(GUI_MSG_MARK_ITEM, GetID(),CONTROL_LIST_SCHEDULES,selctedItemIndex);
  OnMessage(winmsg1);

  SET_CONTROL_LABEL2(CONTROL_EDIT_CHANNEL_NAME, clickedChannel->GetProperty("ChannelName"));

  return true;
}

bool CGUIDialogBoxeeLiveTvEditChannels::ScheduleComparator(const Schedule &left, const Schedule &right)
{
  return (CUtil::VersionCompare(left.first, right.first) < 0);
}

bool CGUIDialogBoxeeLiveTvEditChannels::LoadAvailableChannelList()
{
  if (m_scheduleLoaded)
    return true;

  CStdString strUrl = BOXEE::BXConfiguration::GetInstance().GetURLParam("Boxee.ApiGetAvailableChannels", "http://app.boxee.tv/epg/channels/");
  Json::Value jResponse;
  int returnCode;

  if(BOXEE::BXUtils::PerformJSONGetRequestInBG(strUrl,jResponse,returnCode) != JOB_SUCCEEDED)
  {
    return false;
  }

  Json::Value jStations = jResponse["stations"];

  if (!jStations.isArray())
  {
    return false;
  }

  if (jStations.size() == 0)
  {
    return false;
  }

  for (size_t j = 0; j < jStations.size(); j++)
  {
    Json::Value station = jStations[(int) j];
    m_schedules.push_back(Schedule(station["channel"].asString(), station["call_sign"].asString()));
  }

  std::sort(m_schedules.begin(), m_schedules.end(), CGUIDialogBoxeeLiveTvEditChannels::ScheduleComparator);

  return true;

}

#endif

