/*
 * GUIDialogBoxeeVideoInfo.cpp
 *
 *  Created on: Jan 17, 2011
 *      Author: shayyizhak
 */

#include "GUIDialogBoxeeTechInfo.h"
#include "GUIWindowManager.h"
#include "GUIBaseContainer.h"
#include "utils/log.h"
#include "GUIControlGroupList.h"
#include "GUIToggleButtonControl.h"
#include "GUIListContainer.h"
#include "Application.h"
#include "GUIInfoManager.h"
#include "CPUInfo.h"
#include "VideoReferenceClock.h"
#include "utils/TimeUtils.h"
#include "LocalizeStrings.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#ifdef HAS_INTEL_SMD
#include "cores/IntelSMDGlobals.h"
#endif

#define ITEMS_LIST 600
#define GROUP_LIST 500
#define TOGGLE_BUTTON_CONTROL 8020
#define BOTTOM_IMAGE_CONTROL  8021
#define LIST_ITEM_HEIGHT 54
#define LIST_POSY_OFFSET -8
#define LIST_POSX_OFFSET -196
#define TOGGLE_BUTTON_POSY_OFFSET 0
#define TOGGLE_BUTTON_POSX_OFFSET 0
#define LIST_HEIGHT_OFFSET 0
#define LABEL1                9101
#define LABEL2                9102
#define LABEL3                9103



CGUIDialogBoxeeTechInfo::CGUIDialogBoxeeTechInfo()
: CGUIDialog(WINDOW_DIALOG_TECH_INFO, "boxee_tech_info.xml")
{
}


CGUIDialogBoxeeTechInfo::~CGUIDialogBoxeeTechInfo()
{
}

void CGUIDialogBoxeeTechInfo::OnInitWindow()
{
  CGUIDialog::OnInitWindow();
}

bool CGUIDialogBoxeeTechInfo::OnAction(const CAction& action)
{
  if (action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    Close();
    return true;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeeTechInfo::OnMessage(CGUIMessage& message)
{
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogBoxeeTechInfo::Show()
{
  CGUIDialogBoxeeTechInfo *dialog = (CGUIDialogBoxeeTechInfo *)g_windowManager.GetWindow(WINDOW_DIALOG_TECH_INFO);

  if (dialog)
  {
    dialog->DoModal();
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeTechInfo::Show");
  }
}


void CGUIDialogBoxeeTechInfo::Render()
{
  if (!LoadItems())
  {
    Close();
    return;
  }
  CGUIDialog::Render();

}

bool CGUIDialogBoxeeTechInfo::LoadItems()
{
  if (g_application.GetPlaySpeed() != 1)
    g_infoManager.SetDisplayAfterSeek();
  //if (m_bShowCurrentTime)
  //  g_infoManager.SetDisplayAfterSeek();

  //m_bLastRender = true;
  if (!g_application.m_pPlayer) return false;

  /*
  if( g_application.m_pPlayer->IsCaching() )
  {
    g_infoManager.SetDisplayAfterSeek(0); //Make sure these stuff aren't visible now
    SET_CONTROL_VISIBLE(LABEL_BUFFERING);
  }
  else
  {
    SET_CONTROL_HIDDEN(LABEL_BUFFERING);
  }
  */

   // show audio codec info
  CStdString strAudio, strVideo, strGeneral;
  g_application.m_pPlayer->GetAudioInfo(strAudio);
  strAudio = FormatAudioInfo(strAudio);
  {
    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL1);
    msg.SetLabel(strAudio);
    OnMessage(msg);
  }
  // show video codec info
  g_application.m_pPlayer->GetVideoInfo(strVideo);
  strVideo = FormatVideoInfo(strVideo);
  {
    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL2);
    msg.SetLabel(strVideo);
    OnMessage(msg);
  }
  // show general info
  g_application.m_pPlayer->GetGeneralInfo(strGeneral);
  {
    CStdString strGeneralFPS;
/*
#ifdef __APPLE__
    // We show CPU usage for the entire process, as it's arguably more useful.
    double dCPU = m_resourceCounter.GetCPUUsage();
    CStdString strCores;
    strCores.Format("cpu: %.0f%%", dCPU);
#else
*/
    CStdString strCores = g_cpuInfo.GetCoresUsageString();
//#endif
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
#ifdef HAS_INTEL_SMD
    unsigned int videoCur, videoMax, audioCur, audioMax;
    videoCur = videoMax = audioCur = audioMax = 0;

    ismd_port_handle_t audio_input;
    audio_input = g_IntelSMDGlobals.GetAudioDevicePort(g_IntelSMDGlobals.GetPrimaryAudioDevice());

    g_IntelSMDGlobals.GetPortStatus(g_IntelSMDGlobals.GetVidDecInput(), videoCur, videoMax);
    g_IntelSMDGlobals.GetPortStatus(audio_input, audioCur, audioMax);

    strGeneralFPS.Format("%s\nW( DVDPlayer: %.2f SMD Audio: %.2f %d/%d SMD Video %.2f %d/%d %s ) %s"
                             , strGeneral.c_str()
                             , (float)(g_application.m_pPlayer->GetTime() / 1000.0)
                             ,  g_IntelSMDGlobals.IsmdToDvdPts(g_IntelSMDGlobals.GetAudioCurrentTime())/1000000
                             ,  audioCur, audioMax
                             ,  g_IntelSMDGlobals.IsmdToDvdPts(g_IntelSMDGlobals.GetVideoCurrentTime())/1000000
                             ,  videoCur, videoMax
                             , strCores.c_str()
                             , strClock.c_str() );
#endif

    strGeneralFPS = FormatGeneralInfo(strGeneralFPS);
    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL3);
    msg.SetLabel(strGeneralFPS);
    OnMessage(msg);
  }
  //----------------------
  // ViewMode Information
  //----------------------
  /*
  if (m_bShowViewModeInfo && CTimeUtils::GetTimeMS() - m_dwShowViewModeTimeout > 2500)
  {
    m_bShowViewModeInfo = false;
  }
  if (m_bShowViewModeInfo)
  {
    {
      // get the "View Mode" string
      CStdString strTitle = g_localizeStrings.Get(629);
      CStdString strMode = g_localizeStrings.Get(630 + g_stSettings.m_currentVideoSettings.m_ViewMode);
      CStdString strInfo;
      strInfo.Format("%s : %s", strTitle.c_str(), strMode.c_str());
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL1);
      msg.SetLabel(strInfo);
      OnMessage(msg);
    }
    // show sizing information
    CRect SrcRect, DestRect;
    float fAR;
    g_application.m_pPlayer->GetVideoRect(SrcRect, DestRect);
    g_application.m_pPlayer->GetVideoAspectRatio(fAR);
    {
      CStdString strSizing;
      strSizing.Format("Sizing: (%i,%i)->(%i,%i) (Zoom x%2.2f) AR:%2.2f:1 (Pixels: %2.2f:1)",
                       (int)SrcRect.Width(), (int)SrcRect.Height(),
                       (int)DestRect.Width(), (int)DestRect.Height(), g_stSettings.m_fZoomAmount, fAR*g_stSettings.m_fPixelRatio, g_stSettings.m_fPixelRatio);
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL2);
      msg.SetLabel(strSizing);
      OnMessage(msg);
    }
    // show resolution information
    int iResolution = g_graphicsContext.GetVideoResolution();
    {
      CStdString strStatus;
      strStatus.Format("%s %ix%i@%.2fHz %s",
        g_localizeStrings.Get(13287), g_settings.m_ResInfo[iResolution].iWidth,
        g_settings.m_ResInfo[iResolution].iHeight, g_settings.m_ResInfo[iResolution].fRefreshRate,
        g_settings.m_ResInfo[iResolution].strMode.c_str());
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL3);
      msg.SetLabel(strStatus);
      OnMessage(msg);
    }
  }

  // TTF subs should render 1:1
  TransformMatrix m;
  g_graphicsContext.PushTransform(m, true);
  //RenderTTFSubtitles();
  g_graphicsContext.PopTransform();

  g_graphicsContext.ApplyGuiTransform();

  if (m_timeCodeShow && m_timeCodePosition != 0)
  {
    if ( (CTimeUtils::GetTimeMS() - m_timeCodeTimeout) >= 2500)
    {
      m_timeCodeShow = false;
      m_timeCodePosition = 0;
    }
    CStdString strDispTime = "hh:mm";

    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL1);
    for (int count = 0; count < m_timeCodePosition; count++)
    {
      if (m_timeCodeStamp[count] == -1)
        strDispTime[count] = ':';
      else
        strDispTime[count] = (char)m_timeCodeStamp[count] + 48;
    }
    strDispTime += "/" + g_infoManager.GetLabel(PLAYER_DURATION) + " [" + g_infoManager.GetLabel(PLAYER_TIME) + "]"; // duration [ time ]
    msg.SetLabel(strDispTime);
    OnMessage(msg);
  }
  */

  //CGUIWindow::Render();

  g_graphicsContext.RestoreGuiTransform();

  return true;
}

CStdString CGUIDialogBoxeeTechInfo::FormatAudioInfo (CStdString strAudio)
{
  size_t pos;
  pos = strAudio.find("(") + 2;
  strAudio = strAudio.substr (pos);
  pos = strAudio.find(")");
  strAudio = strAudio.substr (0, pos) + strAudio.substr (pos+1);
  pos = strAudio.find("P(") + 3;
  strAudio = strAudio.substr (0, pos-3) + strAudio.substr (pos);
  pos = strAudio.find(")");
  strAudio = strAudio.substr (0, pos);

  // Remove "audio" tag"
  pos = strAudio.find(":") + 2;
  strAudio = strAudio.substr (pos);

  // Replace \n with ","
  pos = strAudio.find("\n");
  strAudio = strAudio.substr(0, pos-1) + ", " + strAudio.substr(pos+2);

  // Remove 4th tag
  strAudio = removeNItem(4, strAudio);

  // Remove 5th tag
  strAudio = removeNItem(4, strAudio);

  // Remove 6th tag
  strAudio = removeNItem(4, strAudio);

  return strAudio;
}

CStdString CGUIDialogBoxeeTechInfo::FormatVideoInfo (CStdString strVideo)
{
  size_t pos;
  pos = strVideo.find("(") + 2;
  strVideo = strVideo.substr (pos);
  pos = strVideo.find(")");
  strVideo = strVideo.substr (0, pos) + strVideo.substr (pos+1);
  pos = strVideo.find("P(") + 3;
  strVideo = strVideo.substr (0, pos-3) + strVideo.substr (pos);
  pos = strVideo.find(")");
  strVideo = strVideo.substr (0, pos);

  // Remove "video" tag"
  pos = strVideo.find(":") + 2;
  strVideo = strVideo.substr (pos);

  // Replace \n with ",\n"
  pos = strVideo.find("\n");
  strVideo = strVideo.substr(0, pos-1) + ",\n" + strVideo.substr(pos+2);

  // Remove 1st tag
  strVideo = removeNItem(1, strVideo);

  // strVideo 2nd tag
  strVideo = removeNItem(1, strVideo);

  // Remove 4th tag
  strVideo = removeNItem(2, strVideo);

  return strVideo;
}

CStdString CGUIDialogBoxeeTechInfo::FormatGeneralInfo (CStdString strGeneral)
{
  size_t pos;
  pos = strGeneral.find("(") + 2;
  strGeneral = strGeneral.substr (pos);
  pos = strGeneral.find(")");
  strGeneral = strGeneral.substr (0, pos) + strGeneral.substr (pos+1);
  pos = strGeneral.find("W(") + 3;
  strGeneral = strGeneral.substr (0, pos-3) + strGeneral.substr (pos);
  pos = strGeneral.find(")");
  strGeneral = strGeneral.substr (0, pos);

  // Remove "aq" tag"
  pos = strGeneral.find(":") + 2;
  strGeneral = strGeneral.substr (pos);

  // Replace \n with ",\n"
  pos = strGeneral.find("\n");
  strGeneral = strGeneral.substr(0, pos-1) + ",\n" + strGeneral.substr(pos+2);

  // Remove 1st tag
  strGeneral = removeNItem(1, strGeneral);

  // strVideo 3nd tag
  strGeneral = removeNItem(2, strGeneral);

  return strGeneral;
}

CStdString CGUIDialogBoxeeTechInfo::removeNItem(int n, CStdString str)
{
  size_t pos;
  CStdString tempStr;
  for (int i=1; i<n; i++)
  {
    pos = str.find(",");
    tempStr = tempStr.c_str() + str.substr(0, pos+2);
    str = str.substr(pos+2);
    //printf ("CGUIDialogBoxeeTechInfo::removeNItem - str: %s, tempStr: %s\n",str.c_str(), tempStr.c_str());
  }
  pos = str.find(",");
  str = str.substr(pos+2);
  tempStr = tempStr + str;

  return tempStr;
}


