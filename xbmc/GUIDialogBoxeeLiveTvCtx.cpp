#include "GUIDialogBoxeeLiveTvCtx.h"

#ifdef HAS_DVB

#include <boost/foreach.hpp>
#include "BoxeeUtils.h"
#include "GUIWindowManager.h"
#include "GUIInfoManager.h"
#include "cores/dvb/dvbmanager.h"
#include "cores/dvb/epgstore.h"
#include "GUISettings.h"
#include "utils/log.h"
#include "GUIWindowBoxeeLiveTv.h"
#include "GUIDialogBoxeeLiveTvEditChannels.h"
#include "GUIDialogBoxeeLiveTvInfo.h"

#define CONTROL_LIST_EPG                 50
#define CONTROL_LABEL_EPG                49
#define CONTROL_LIST_ACTIONS             47
#define CONTROL_BUTTON_ACTION            461
#define CONTROL_BUTTON_EDIT_CHANNELS     12
#define CONTROL_BUTTON_ENABLE_SHARE      13
#define CONTROL_BUTTON_DISABLE_SHARE     14

CGUIDialogBoxeeLiveTvCtx::CGUIDialogBoxeeLiveTvCtx() : CGUIDialog(WINDOW_DIALOG_BOXEE_LIVETV_CTX, "boxee_livetv_context.xml")
{
}

CGUIDialogBoxeeLiveTvCtx::~CGUIDialogBoxeeLiveTvCtx()
{
}

void CGUIDialogBoxeeLiveTvCtx::OnInitWindow()
{
  g_infoManager.SetShowCodec(false);
  g_infoManager.SetShowInfo(false);

  CGUIDialog::OnInitWindow();

  // initialize visibility
  DVBManager::GetInstance().GetEpgServerLoader().NotifyEpgVisibilityChange(true);

  LoadTimeSlot();

  int selectedItem = LoadEPG();
  SET_CONTROL_FOCUS(CONTROL_LIST_EPG, selectedItem);
}

void CGUIDialogBoxeeLiveTvCtx::OnDeinitWindow(int nextWindowID)
{
  DVBManager::GetInstance().GetEpgServerLoader().NotifyEpgVisibilityChange(false);

  m_model.SetCurrentTimeSlot(0);

  SET_CONTROL_FOCUS(CONTROL_LIST_EPG,0);

  CGUIDialog::OnDeinitWindow(nextWindowID);
}

void CGUIDialogBoxeeLiveTvCtx::LoadTimeSlot()
{
  time_t startTime;
  time_t endTime;
  struct tm *loctime;

  m_model.GetCurrentTimeSlot(startTime, endTime);

  CStdString timeSlotStr = "[B]";
  if (startTime == 0)
  {
    timeSlotStr += "NOW";
  }
  else
  {
    loctime = localtime(&startTime);
    CDateTime startDateTime(*loctime);
    timeSlotStr += startDateTime.GetAsLocalizedTime("", false);
  }

  timeSlotStr += " - ";

  loctime = localtime(&endTime);
  CDateTime endDateTime(*loctime);
  timeSlotStr += endDateTime.GetAsLocalizedTime("", false);

  timeSlotStr += "[/B]";

  SET_CONTROL_LABEL(CONTROL_LABEL_EPG,timeSlotStr);
}

int CGUIDialogBoxeeLiveTvCtx::LoadEPG()
{
  LiveTvModelChannelsType channels = m_model.GetChannels();
  LiveTvModelProgramsType programs = m_model.GetPrograms();

  int currentChannelIndex = DVBManager::GetInstance().GetCurrentChannel()->GetIndex();

  time_t startSlotTime;
  time_t endSlotTime;
  m_model.GetCurrentTimeSlot(startSlotTime, endSlotTime);

  m_listEPG.Clear();

  int selectedItem = 0;

  for (int channelIndex = 0; channelIndex < (int) channels.size(); channelIndex++)
  {
    LiveTvModelChannel& channelInfo = channels[channelIndex];

    if (!channelInfo.enabled)
      continue;

    bool isActive = (currentChannelIndex == channelIndex);

    std::vector<LiveTvModelProgram>& programsForChannel = programs[channelIndex];

    if (programsForChannel.size() > 0)
    {
      bool firstProgram = true;
      foreach (LiveTvModelProgram program, programsForChannel)
      {
        time_t startTime = (time_t) program.info.start;
        struct tm* startLocaltime = localtime(&startTime);
        CDateTime startDateTime(*startLocaltime);

        time_t endTime = (time_t) program.info.end;
        struct tm* endLocaltime = localtime(&endTime);
        CDateTime endDateTime(*endLocaltime);

        CFileItemPtr channelItem(new CFileItem(program.info.title));
        channelItem->SetProperty("ChannelName", channelInfo.label);
        channelItem->SetProperty("ChannelNumber", channelInfo.number);
        channelItem->SetProperty("channel-id", channelInfo.id);
        channelItem->SetProperty("hasepg", true);
        channelItem->SetProperty("rating", program.info.rating);
        channelItem->SetProperty("hasrating", program.info.rating.length() > 0);
        channelItem->SetProperty("issubitem", !firstProgram);
        channelItem->SetProperty("program-id", program.info.id);

        channelItem->SetProperty("starttime", startDateTime.GetAsLocalizedTime("", false));
        channelItem->SetProperty("endtime", endDateTime.GetAsLocalizedTime("", false));
        channelItem->SetProperty("isnow", program.isNow);
        channelItem->SetProperty("isnew", program.info.isNew);
        channelItem->SetProperty("isactive", isActive);

        channelItem->SetProperty("show-synopsis", program.info.synopsis);
        channelItem->SetProperty("show-title", program.info.title);
        channelItem->SetProperty("episode-title", program.info.episodeTitle);
        channelItem->SetProperty("episode-number", program.info.episodeNumber);
        channelItem->SetProperty("season-number", program.info.seasonNumber);

        // In case there are multiple shows on the selected channels, we only want
        // the first one to be active
        if (isActive)
        {
          selectedItem = m_listEPG.Size();
          isActive = false;
        }

        if (program.isNow && program.social.watching)
        {
          int numOfFriendsWatching = program.social.friends.size();

          channelItem->SetProperty("friendswatching", true);
          channelItem->SetProperty("watching-label", program.social.label);

          if (numOfFriendsWatching == 1)
          {
            channelItem->SetProperty("userthumb-1", program.social.friends[0].thumbSmallUrl);
          }
          else if (numOfFriendsWatching == 2)
          {
            channelItem->SetProperty("userthumb-1", program.social.friends[0].thumbSmallUrl);
            channelItem->SetProperty("userthumb-2", program.social.friends[1].thumbSmallUrl);
          }
        }

        m_listEPG.Add(channelItem);

        firstProgram = false;
      }
    }
    else
    {
      // Sorry, no programs for this channel
      CFileItemPtr channelItem(new CFileItem(channelInfo.label));
      channelItem->SetProperty("ChannelName", channelInfo.label);
      channelItem->SetProperty("ChannelNumber", channelInfo.number);
      channelItem->SetProperty("channel-id", channelInfo.id);
      channelItem->SetProperty("hasepg", false);
      channelItem->SetProperty("isactive", isActive);

      if (isActive)
      {
        selectedItem = m_listEPG.Size();
        isActive = false;
      }

      m_listEPG.Add(channelItem);
    }
  }

  CGUIMessage msgBind(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_EPG, 0, 0, &m_listEPG);
  OnMessage(msgBind);

  SetProperty("HasNext", m_model.HasNextTimeSlot());
  SetProperty("HasPrev", m_model.HasPrevTimeSlot());

  return selectedItem;
}

void CGUIDialogBoxeeLiveTvCtx::HandleClickedChannel()
{
  int selectedItem;
  CONTROL_GET_SELECTED_ITEM(GetID(), CONTROL_LIST_EPG, selectedItem);

  CFileItemPtr pSelectedItem = m_listEPG.Get(selectedItem);

  // Show info if it's a future show and has epg
  if (pSelectedItem->GetPropertyBOOL("hasepg") && !pSelectedItem->GetPropertyBOOL("isnow"))
  {
    CGUIDialogBoxeeLiveTvInfo* pInfo = (CGUIDialogBoxeeLiveTvInfo*) g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_LIVETV_INFO);
    pInfo->SetProgram(pSelectedItem);
    pInfo->DoModal();
    return;
  }

  // Switch channel if it's not the active one.
  if (!pSelectedItem->GetPropertyBOOL("isactive"))
  {
    CGUIWindowBoxeeLiveTv::SwitchChannel(pSelectedItem->GetPropertyInt("channel-id"));
  }

  // Close the dialog
  Close();
}

bool CGUIDialogBoxeeLiveTvCtx::HandleClick(CGUIMessage& message)
{
  //see what was clicked and decide what to do with it
  switch (message.GetSenderId())
  {
    case CONTROL_LIST_EPG:
    {
      HandleClickedChannel();
      return true;
    }
    break;

    case CONTROL_BUTTON_EDIT_CHANNELS:
    {
      CGUIDialogBoxeeLiveTvEditChannels* pDlgLTEC = (CGUIDialogBoxeeLiveTvEditChannels*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_LIVETV_EDIT_CHANNELS);
      if (!pDlgLTEC)
      {
        CLog::Log(LOGERROR,"CGUIDialogBoxeeLiveTvCtx::HandleClick - CONTROL_BUTTON_EDIT_CHANNELS - FAILED to get LIVETV_EDIT_CHANNELS object (ltvc)");
        return true;
      }

      pDlgLTEC->DoModal();

      RefreshEPG(0);

      return true;
    }
    break;

    case CONTROL_BUTTON_ENABLE_SHARE:
    case CONTROL_BUTTON_DISABLE_SHARE:
    {
      g_guiSettings.SetBool("ota.share", !g_guiSettings.GetBool("ota.share"));
      g_settings.Save();
      return true;
    }
    break;

    default:
    {
      // do nothing
    }
    break;
  }

  return false;
}

bool CGUIDialogBoxeeLiveTvCtx::RefreshEPG(int direction)
{
  bool hasSlot = true;
  if (direction > 0)
    hasSlot = m_model.SetNextTimeSlot();
  else if (direction < 0)
    hasSlot = m_model.SetPrevTimeSlot();

  if (!hasSlot)
    return false;

  // Remember the channel that we are on
  int selectedItem;
  CONTROL_GET_SELECTED_ITEM(GetID(), CONTROL_LIST_EPG, selectedItem);
  CFileItemPtr pSelectedItem = m_listEPG.Get(selectedItem);
  CStdString currentCHannel = pSelectedItem->GetProperty("ChannelName");

  LoadEPG();
  LoadTimeSlot();

  for (int i = 0; i < m_listEPG.Size(); i++)
  {
    if (m_listEPG[i]->GetProperty("ChannelName") == currentCHannel)
    {
      selectedItem = i;
      break;
    }
  }

  SET_CONTROL_FOCUS(CONTROL_LIST_EPG, selectedItem);

  return true;
}

bool CGUIDialogBoxeeLiveTvCtx::OnAction(const CAction& action)
{
  switch (action.id)
  {
    case ACTION_MOVE_RIGHT:
    {
      if (!GetControl(CONTROL_LIST_ACTIONS)->HasFocus())
      {
        return RefreshEPG(1);
      }
    }
    break;

    case ACTION_MOVE_LEFT:
    {
      if (RefreshEPG(-1))
      {
        return true;
      }
    }
    break;
  };

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeeLiveTvCtx::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_LABEL_BIND:
  {
    if (message.GetPointer() && message.GetControlId() == 0)
    {
      CFileItemList *items = (CFileItemList *)message.GetPointer();
      delete items;
      return true;
    }
  }
  break;

  case GUI_MSG_CLICKED:
  {
    if (HandleClick(message))
    {
      return true;
    }
  }
  break;

  default:
  break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogBoxeeLiveTvCtx::Render()
{
  CGUIDialog::Render();

  time_t now = time(NULL);
  time_t startSlot;
  time_t endSlot;
  m_model.GetCurrentTimeSlot(startSlot, endSlot);

  if (now > endSlot)
  {
    m_model.SetCurrentTimeSlot(0);
    RefreshEPG(0);
  }
}

#endif

