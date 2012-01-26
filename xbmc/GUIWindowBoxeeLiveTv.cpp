#include "GUIWindowBoxeeLiveTv.h"

#ifdef HAS_DVB

#include <boost/foreach.hpp>
#include "GUIWindowManager.h"
#include "Application.h"
#include "GUIInfoManager.h"
#include "cores/dvb/dvbmanager.h"
#include "cores/dvb/epgstore.h"
#include "LocalizeStrings.h"
#include "GUISettings.h"
#include "utils/log.h"
#include "guilib/GUIListContainer.h"
#include "utils/TimeUtils.h"
#include "CPUInfo.h"
#include "VideoReferenceClock.h"
#include "GUIDialogBoxeeLiveTvCtx.h"
#include "GUIDialogBoxeeShare.h"
#include "GUIDialogBoxeeExitVideo.h"

#ifdef HAS_INTEL_SMD
#include "IntelSMDGlobals.h"
#endif
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#ifdef __APPLE__
static CLinuxResourceCounter m_resourceCounter;
#endif

using namespace BOXEE;

#define REPORT_PROGRAM_WATCHED (3 * 60 * 1000)
#define SHOW_INFO_DURATION     (5)

#define BLUE_BAR                         9
#define LABEL_ROW1                       10
#define LABEL_ROW2                       11
#define LABEL_ROW3                       12
#define GROUP_QUICK_OSD                  30
#define GROUP_OSD                        40
#define GROUP_INFO                       50
#define LIST_OSD_BUTTONS                 9150
#define BUTTON_GUIDE                     9001
#define BUTTON_INFO                      9002
#define BUTTON_SHARE                     9003

//#define ENABLE_AUTO_CHANNEL_SWITCH // switch channels every AUTO_SWITCH_INTERVAL ms
#define AUTO_SWITCH_INTERVAL 10000 // ms

CGUIWindowBoxeeLiveTv::CGUIWindowBoxeeLiveTv() : CGUIWindow(WINDOW_BOXEE_LIVETV, "boxee_livetv_window.xml")
{
}

CGUIWindowBoxeeLiveTv::~CGUIWindowBoxeeLiveTv()
{
}

void CGUIWindowBoxeeLiveTv::OnInitWindow()
{
  CGUIWindow::OnInitWindow();

  CFileItem urlItem("start dvb");
  urlItem.SetProperty("FastPlayerStart", true);
  urlItem.SetProperty("DisableBoxeeUI", true);
  urlItem.m_strPath += "dvb://";
  g_application.PlayMedia(urlItem);

  g_graphicsContext.SetFullScreenVideo(true);
  g_Windowing.ClearBuffers(0, 0, 0, 1.0);

  m_reportToServer = g_guiSettings.GetBool("ota.share");
  if (m_reportToServer)
    m_programWatchedTimer.StartZero();
  else
    m_programWatchedTimer.Stop();

  m_osdTimer.StartZero();
#ifdef ENABLE_AUTO_CHANNEL_SWITCH
  m_autoChannelSwitch.StartZero();
#endif

  ShowQuickOSD();
}

void CGUIWindowBoxeeLiveTv::OnDeinitWindow(int nextWindowID)
{
  g_application.StopPlaying();
  g_graphicsContext.SetFullScreenVideo(false);

  g_windowManager.CloseDialogs(true);

  CGUIWindow::OnDeinitWindow(nextWindowID);
}

bool CGUIWindowBoxeeLiveTv::SwitchChannel(int channelId)
{
  DvbChannelPtr ch = DVBManager::GetInstance().GetChannels().GetChannelByIndex(channelId);
  if (ch.get() == NULL)
  {
    CLog::Log(LOGERROR, "CGUIWindowBoxeeLiveTv::SwitchChannel - invalid channel id [%d] (ltvc)", channelId);
    return false;
  }

  // save the selected channel inside the manager
  DVBManager::GetInstance().SetCurrentChannel(ch);

  CAction action;
  action.id = ACTION_CHANNEL_SWITCH;
  action.amount1 = channelId;

  if(g_application.m_pPlayer)
    g_application.m_pPlayer->OnAction(action);

  g_windowManager.GetWindow(g_windowManager.GetActiveWindow())->OnAction(action);

  return true;
}

bool CGUIWindowBoxeeLiveTv::SwitchRelativeChannel(int direction)
{
  DvbChannels& channels = DVBManager::GetInstance().GetChannels();
  int currentPlayingId = DVBManager::GetInstance().GetCurrentChannel()->GetIndex();
  int nextPlayingId = currentPlayingId;

  do
  {
    nextPlayingId += direction;

    if (nextPlayingId == (int) channels.Size())
      nextPlayingId = 0;

    if (nextPlayingId == -1)
      nextPlayingId = channels.Size() - 1;
  } while (!channels.GetChannelByIndex(nextPlayingId)->IsEnabled() && nextPlayingId != currentPlayingId);

  return SwitchChannel(nextPlayingId);
}

void CGUIWindowBoxeeLiveTv::Render()
{
  CGUIWindow::Render();

  // If the elapsed time has passed and a channel is watched, notify the server
  if (m_reportToServer && m_programWatchedTimer.IsRunning() && m_programWatchedTimer.GetElapsedMilliseconds() > REPORT_PROGRAM_WATCHED)
  {
    // Notify the server
    DVBManager::GetInstance().ReportCurrentChannelToServerInBG();

    // Reset the timer, report back again in a few minutes
    m_programWatchedTimer.Reset();
  }

  if (m_osdTimer.IsRunning() && m_osdTimer.GetElapsedSeconds() > SHOW_INFO_DURATION)
  {
    SET_CONTROL_HIDDEN(GROUP_QUICK_OSD);
    SET_CONTROL_HIDDEN(GROUP_OSD);
    m_osdTimer.Stop();
  }

#ifdef ENABLE_AUTO_CHANNEL_SWITCH
  if(m_autoChannelSwitch.GetElapsedMilliseconds() > AUTO_SWITCH_INTERVAL)
  {
    SwitchRelativeChannel(1);
    m_autoChannelSwitch.StartZero();
  }
#endif

  if (!DVBManager::GetInstance().IsDongleInserted())
  {
    CLog::Log(LOGINFO, "CGUIWindowBoxeeLiveTv::Render: dongle disconnected. stopping playback");

    g_application.StopPlaying();
    Close();
  }
}

bool CGUIWindowBoxeeLiveTv::OnAction(const CAction &action)
{
  switch (action.id)
  {
    case ACTION_PREVIOUS_MENU:
    {
      if (GetControl(GROUP_INFO)->IsVisible())
      {
        SET_CONTROL_HIDDEN(GROUP_INFO);
      }
      else if (GetControl(GROUP_OSD)->IsVisible())
      {
        SET_CONTROL_HIDDEN(GROUP_OSD);
        SET_CONTROL_HIDDEN(GROUP_QUICK_OSD);
      }
      else
      {
        bool shouldClose = true;
        bool showExitDialog = g_guiSettings.GetBool("ota.showmessagewhenexit");
        if (showExitDialog && !CGUIDialogBoxeeExitVideo::ShowAndGetInput())
        {
          shouldClose = false;
        }

        if (shouldClose)
        {
          g_application.StopPlaying();
          g_windowManager.PreviousWindow();
        }
      }
      return true;
    }
    break;

    case ACTION_MOVE_UP:
    {
      if (GetControl(GROUP_INFO)->IsVisible())
      {
        return true;
      }
      else if (!GetControl(GROUP_OSD)->IsVisible())
      {
        return SwitchRelativeChannel(1);
      }
    }
    break;

    case ACTION_MOVE_DOWN:
    {
      if (GetControl(GROUP_INFO)->IsVisible())
      {
        return true;
      }
      else if (!GetControl(GROUP_OSD)->IsVisible())
      {
        return SwitchRelativeChannel(-1);
      }
    }
    break;

    case ACTION_MOVE_LEFT:
    case ACTION_MOVE_RIGHT:
    {
      if (GetControl(GROUP_INFO)->IsVisible())
      {
        return true;
      }
      else if (!GetControl(GROUP_OSD)->IsVisible())
      {
        ShowEPG();
        return true;
      }
    }
    break;

    case ACTION_SELECT_ITEM:
    {
      if (GetControl(GROUP_INFO)->IsVisible())
      {
        return true;
      }
      else if (!GetControl(GROUP_OSD)->IsVisible())
      {
        ShowOSD();
        return true;
      }
    }
    break;

    case ACTION_CHANNEL_SWITCH:
    {
      ShowQuickOSD();
      return true;
    }
    break;

    default:
    break;
  };

  return CGUIWindow::OnAction(action);
}

bool CGUIWindowBoxeeLiveTv::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() != GUI_MSG_CLICKED || message.GetParam1() != ACTION_SELECT_ITEM)
    return CGUIWindow::OnMessage(message);;

  switch (message.GetSenderId())
  {
  case BUTTON_GUIDE:
    ShowEPG();
    return true;
    break;

  case BUTTON_INFO:
    ShowInfo();
    return true;
    break;

  case BUTTON_SHARE:
    ShowShare();
    return true;
    break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowBoxeeLiveTv::ShowEPG()
{
  SET_CONTROL_HIDDEN(GROUP_QUICK_OSD);
  SET_CONTROL_HIDDEN(GROUP_OSD);
  m_osdTimer.Stop();

  CGUIDialogBoxeeLiveTvCtx* pDlgLiveTvCtx = (CGUIDialogBoxeeLiveTvCtx*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_LIVETV_CTX);
  if (!pDlgLiveTvCtx)
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeLiveTv::ShowOSD - FAILED to get BOXEE_LIVETV_CTX object (ltvc)");
    return;
  }

  pDlgLiveTvCtx->DoModal();
}

void CGUIWindowBoxeeLiveTv::ShowQuickOSD()
{
  SET_CONTROL_VISIBLE(GROUP_QUICK_OSD);
  m_osdTimer.StartZero();
}

void CGUIWindowBoxeeLiveTv::ShowOSD()
{
  SET_CONTROL_VISIBLE(GROUP_QUICK_OSD);
  SET_CONTROL_VISIBLE(GROUP_OSD);
  SET_CONTROL_FOCUS(LIST_OSD_BUTTONS, 0);
  m_osdTimer.StartZero();
}

void CGUIWindowBoxeeLiveTv::ShowInfo()
{
  SET_CONTROL_VISIBLE(GROUP_INFO);
}

void CGUIWindowBoxeeLiveTv::ShowShare()
{
  CGUIDialogBoxeeShare *pShare = (CGUIDialogBoxeeShare *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_SHARE);

  if (pShare)
  {
    LiveTvModelProgram currentProgram = DVBManager::GetInstance().GetCurrentProgram();
    CStdString shareLabel = currentProgram.info.title;
    if (currentProgram.info.episodeTitle.length())
    {
      shareLabel += ": ";
      shareLabel += currentProgram.info.episodeTitle;
    }

    CFileItem shareItem(shareLabel);
    shareItem.SetProperty("livetv", true);
    if (currentProgram.info.IsInfoFromServer())
      shareItem.SetProperty("program_id", currentProgram.info.id);
    pShare->SetItem(&shareItem);
    pShare->DoModal();
  }
}

void CGUIWindowBoxeeLiveTv::RenderFullScreen()
{
  g_graphicsContext.Clear();

  g_graphicsContext.ApplyGuiTransform();
  Render();
  RenderCodecInfo();

  g_graphicsContext.RestoreGuiTransform();
}

void CGUIWindowBoxeeLiveTv::RenderCodecInfo()
{
  bool bShowCodec = g_infoManager.GetBool(PLAYER_SHOWCODEC);

  if (bShowCodec)
  {
    SET_CONTROL_VISIBLE(LABEL_ROW1);
    SET_CONTROL_VISIBLE(LABEL_ROW2);
    SET_CONTROL_VISIBLE(LABEL_ROW3);
    SET_CONTROL_VISIBLE(BLUE_BAR);
  }
  else
  {
    SET_CONTROL_HIDDEN(LABEL_ROW1);
    SET_CONTROL_HIDDEN(LABEL_ROW2);
    SET_CONTROL_HIDDEN(LABEL_ROW3);
    SET_CONTROL_HIDDEN(BLUE_BAR);
  }

  if (bShowCodec && g_application.m_pPlayer)
  {
    // show audio codec info
    CStdString strAudio, strVideo, strGeneral;
    g_application.m_pPlayer->GetAudioInfo(strAudio);
    {
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW1);
      msg.SetLabel(strAudio);
      OnMessage(msg);
    }
    // show video codec info
    g_application.m_pPlayer->GetVideoInfo(strVideo);
    {
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW2);
      msg.SetLabel(strVideo);
      OnMessage(msg);
    }
    // show general info
    g_application.m_pPlayer->GetGeneralInfo(strGeneral);
    {
      CStdString strGeneralFPS;
      CStdString strCores = g_cpuInfo.GetCoresUsageString();

      int missedvblanks;
      int    refreshrate;
      double clockspeed;
      CStdString strClock;

      if (g_VideoReferenceClock.GetClockInfo(missedvblanks, clockspeed, refreshrate))
        strClock.Format("S( refresh:%i missed:%i speed:%+.3f%% %s )"
                       , refreshrate
                       , missedvblanks
                       , clockspeed - 100.0
                       , g_renderManager.GetVSyncState().c_str());

      strGeneralFPS.Format("%s\nW( fps:%02.2f %s ) %s"
                         , strGeneral.c_str()
                         , g_infoManager.GetFPS()
                         , strCores.c_str(), strClock.c_str() );

      CStdString playerTimeStr;
      __int64 playerTime = g_application.m_pPlayer->GetTime();
      StringUtils::MilisecondsToTimeString((int)playerTime, playerTimeStr);

      static CStdString strDvb;
      static Uint32 lastCheck = 0;
      static DvbTunerTechnicalInfo info;

      Uint32 now = SDL_GetTicks();
      if (now - lastCheck > 1000)
      {
        lastCheck = now;

        std::vector<DvbTunerPtr> tuners = DVBManager::GetInstance().GetTuners();
        if (tuners.size())
          info = tuners[0]->GetTechnicalInfo();
      }

      strDvb.Format("Status: 0x%x Signal: %lu SNR: %lu BER: %lu UCB: %lu",
          info.fe_status, info.signal, info.snr, info.ber, info.ucb);

      strGeneralFPS.Format("LiveTV: %s %s %s"
                               , playerTimeStr.c_str()
                               , strCores.c_str()
                               , strDvb.c_str());

      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW3);
      msg.SetLabel(strGeneralFPS);
      OnMessage(msg);
    }
  }
}
#endif

