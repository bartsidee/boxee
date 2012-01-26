/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIWindowVisualisation.h"
#include "GUIVisualisationControl.h"
#include "Application.h"
#include "GUIDialogMusicOSD.h"
#include "GUIUserMessages.h"
#include "utils/GUIInfoManager.h"
#include "ButtonTranslator.h"
#include "GUIDialogVisualisationPresetList.h"
#include "GUIWindowManager.h"
#include "Settings.h"
#include "PlayList.h"
#include "PictureThumbLoader.h"
#include "AdvancedSettings.h"
#include "MouseStat.h"
#include "utils/log.h"

#include "GUIWindowMusicInfo.h"
#include "GUIDialogBoxeeMusicCtx.h"

#ifdef HAS_EMBEDDED
#include "ItemLoader.h"
#endif 

using namespace MUSIC_INFO;
using namespace PLAYLIST;

#define TRANSISTION_COUNT   50  // 1 second
#define TRANSISTION_LENGTH 200  // 4 seconds
#define START_FADE_LENGTH  100  // 2 seconds on startup

#define CONTROL_VIS          2

//Boxee
#define CONTROL_OSD_INFO    100
#define DEFAULT_FPS         60
//end Boxee

CGUIWindowVisualisation::CGUIWindowVisualisation(void)
    : CGUIWindow(WINDOW_VISUALISATION, "MusicVisualisation.xml")
{
  m_initTimer = 0;
  m_lockedTimer = 0;
  m_bShowPreset = false;
}

CGUIWindowVisualisation::~CGUIWindowVisualisation(void)
{
}

void CGUIWindowVisualisation::ShowOSD()
{
  CGUIDialogBoxeeMusicCtx* pDlgInfo = (CGUIDialogBoxeeMusicCtx*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_MUSIC_CTX);
  CPlayList& pl = g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist());
  int nSong = g_playlistPlayer.GetCurrentSong();
  CFileItem *pItem = NULL;
  if ( nSong < pl.size() && nSong >= 0)
    pItem = pl[nSong].get();

  if (pItem == NULL)
    pItem = &g_application.CurrentFileItem();

  if (pDlgInfo && pItem )
  {
    if (pItem->GetThumbnailImage().IsEmpty())
      pItem->SetThumbnailImage(g_infoManager.GetImage(MUSICPLAYER_COVER,0));
    pDlgInfo->SetItem(*pItem);
    pDlgInfo->DoModal();
  }
  g_infoManager.SetShowInfo(false);
}

bool CGUIWindowVisualisation::OnAction(const CAction &action)
{
  switch (action.id)
  {
  case ACTION_SHOW_INFO:
  {
    ShowOSD();
    return true;
  }
  break;
  case ACTION_PARENT_DIR:
  {
    g_windowManager.PreviousWindow();
    return true;
  }
  break;
  case ACTION_SHOW_GUI:
#ifdef HAS_EMBEDDED
    if(g_application.IsPlaying())
    {
      g_application.GetItemLoader().Resume();
    }
#endif
    // save the settings
    g_settings.Save();
    g_windowManager.PreviousWindow();
    return true;
    break;

  case ACTION_VIS_PRESET_LOCK:
  { // show the locked icon + fall through so that the vis handles the locking
    CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
    g_windowManager.SendMessage(msg);
    if (msg.GetPointer())
    {
      CVisualisation *pVis = (CVisualisation *)msg.GetPointer();
      char** pPresets=NULL;
      int currpreset=0, numpresets=0;
      bool locked;

      pVis->GetPresets(&pPresets,&currpreset,&numpresets,&locked);
      if (numpresets == 1 || !pPresets)
        return true;
    }
    if (!m_bShowPreset)
    {
      m_lockedTimer = START_FADE_LENGTH;
      g_infoManager.SetShowCodec(true);
    }
  }
  break;
  case ACTION_VIS_PRESET_SHOW:
  {
    if (!m_lockedTimer || m_bShowPreset)
      m_bShowPreset = !m_bShowPreset;
    g_infoManager.SetShowCodec(m_bShowPreset);
    return true;
  }
  break;

  case ACTION_DECREASE_RATING:
  case ACTION_INCREASE_RATING:
  {
    // actual action is taken care of in CApplication::OnAction()
    m_initTimer = g_advancedSettings.m_songInfoDuration * (int)g_infoManager.GetFPS();
    g_infoManager.SetShowInfo(true);
  }
  break;
  // TODO: These should be mapped to it's own function - at the moment it's overriding
  // the global action of fastforward/rewind and OSD.
  /*  case KEY_BUTTON_Y:
    g_application.m_CdgParser.Pause();
    return true;
    break;

    case ACTION_ANALOG_FORWARD:
    // calculate the speed based on the amount the button is held down
    if (action.amount1)
    {
      float AVDelay = g_application.m_CdgParser.GetAVDelay();
      g_application.m_CdgParser.SetAVDelay(AVDelay - action.amount1 / 4.0f);
      return true;
    }
    break;*/

  case ACTION_NEXT_ITEM:
  case ACTION_PREV_ITEM:
  {
    //////////////////////////////////////////////////////////////////////////////////////////////////
    // In case of SHOUTCAST we want to disable the options of ACTION_NEXT_ITEM and ACTION_PREV_ITEM //
    //////////////////////////////////////////////////////////////////////////////////////////////////

    CPlayList& playlist = g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist());
    int currentSongIndex = g_playlistPlayer.GetCurrentSong();

    CFileItem *pItem = NULL;
    if ((currentSongIndex < playlist.size()) && (currentSongIndex >= 0))
    {
      pItem = playlist[currentSongIndex].get();
    }

    if (pItem == NULL)
    {
      pItem = &g_application.CurrentFileItem();
    }

    if(pItem)
    {
      if((pItem->m_strPath).Left(8) == "shout://")
      {
        CLog::Log(LOGDEBUG,"CGUIWindowVisualisation::OnAction - Not handling action [action.id=%d] in SHOUTCAST [path=%s]",action.id,(pItem->m_strPath).c_str());
        return true;
      }
    }
  }
  break;
  }
  // default action is to send to the visualisation first
  CGUIVisualisationControl *pVisControl = (CGUIVisualisationControl *)GetControl(CONTROL_VIS);
  if (pVisControl && pVisControl->OnAction(action))
    return true;
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowVisualisation::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_PLAYBACK_STARTED:
    {
      CGUIDialogBoxeeMusicCtx* pDlgInfo = (CGUIDialogBoxeeMusicCtx*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_MUSIC_CTX);
      if(!pDlgInfo->IsActive())
      {
        CGUIVisualisationControl *pVisControl = (CGUIVisualisationControl *)GetControl(CONTROL_VIS);
        if (pVisControl)
          return pVisControl->OnMessage(message);
      }
      else
      {
        pDlgInfo->DoModal();
      }

    }
    break;
  case GUI_MSG_GET_VISUALISATION:
    {
//      message.SetControlID(CONTROL_VIS);
      CGUIVisualisationControl *pVisControl = (CGUIVisualisationControl *)GetControl(CONTROL_VIS);
      if (pVisControl)
        message.SetPointer(pVisControl->GetVisualisation());
      return true;
    }
    break;
  case GUI_MSG_VISUALISATION_ACTION:
    {
      // message.SetControlID(CONTROL_VIS);
      CGUIVisualisationControl *pVisControl = (CGUIVisualisationControl *)GetControl(CONTROL_VIS);
      if (pVisControl)
        return pVisControl->OnMessage(message);
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
    {
      g_windowManager.CloseDialogs(true);
      
      /*
      if (IsActive()) // save any changed settings from the OSD
        g_settings.Save();
      // check and close any OSD windows
      CGUIDialogMusicOSD *pOSD = (CGUIDialogMusicOSD *)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_OSD);
      if (pOSD && pOSD->IsDialogRunning()) pOSD->Close(true);
      CGUIDialogVisualisationPresetList *pList = (CGUIDialogVisualisationPresetList *)g_windowManager.GetWindow(WINDOW_DIALOG_VIS_PRESET_LIST);
      if (pList && pList->IsDialogRunning()) pList->Close(true);      
//Boxee
//Boxee uses different windows
      CGUIWindowMusicInfo *pOSD2 = (CGUIWindowMusicInfo *)g_windowManager.GetWindow(WINDOW_MUSIC_INFO);
      if (pOSD2 && pOSD2->IsDialogRunning()) pOSD2->Close(true);
      CGUIDialogBoxeeMusicCtx *pList2 = (CGUIDialogBoxeeMusicCtx *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_MUSIC_CTX);
      if (pList2 && pList2->IsDialogRunning()) pList2->Close(true);
//end Boxee
      */      
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      // Switching to visualization, closes all dialogs
      g_windowManager.CloseDialogs(true);
      /*
//Boxee
      CGUIWindowMusicInfo* pDlgInfo = (CGUIWindowMusicInfo*)g_windowManager.GetWindow(WINDOW_MUSIC_INFO);
      pDlgInfo->Close(); // make sure info dialog is closed
//end Boxee

      */
 
      // check whether we've come back here from a window during which time we've actually
      // stopped playing music
      if (message.GetParam1() == WINDOW_INVALID && !g_application.IsPlayingAudio())
      { // why are we here if nothing is playing???
        g_windowManager.PreviousWindow();
        return true;
      }

      // hide or show the preset button(s)
      g_infoManager.SetShowCodec(m_bShowPreset);
      g_infoManager.SetShowInfo(true);  // always show the info initially.
      CGUIWindow::OnMessage(message);
      if (g_infoManager.GetCurrentSongTag())
        m_tag = *g_infoManager.GetCurrentSongTag();

      if (g_stSettings.m_bMyMusicSongThumbInVis)
      { // always on
        m_initTimer = 0;
      }
      else
      {
      // start display init timer (fade out after 3 secs...)
        m_initTimer = g_advancedSettings.m_songInfoDuration * (int)g_infoManager.GetFPS();
      }

      ShowOSD();

      return true;
    }
  }
  return CGUIWindow::OnMessage(message);
}

bool CGUIWindowVisualisation::OnMouse(const CPoint &point)
{
  if (g_Mouse.bClick[MOUSE_RIGHT_BUTTON])
  { // no control found to absorb this click - go back to GUI
    CAction action;
    action.id = ACTION_SHOW_GUI;
    OnAction(action);
    return true;
  }
  if (g_Mouse.bClick[MOUSE_LEFT_BUTTON])
  { // no control found to absorb this click - toggle the track INFO
    CAction action;
    action.id = ACTION_PAUSE;
    return g_application.OnAction(action);
  }
  if (g_Mouse.HasMoved())
  { // movement - toggle the OSD
    CGUIDialogBoxeeMusicCtx* pDlgInfo = (CGUIDialogBoxeeMusicCtx*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_MUSIC_CTX);
    if (pDlgInfo)
    {
      pDlgInfo->SetAutoClose(3000);
      pDlgInfo->DoModal();
    }
  }
  return true;
}

void CGUIWindowVisualisation::Render()
{
  g_application.ResetScreenSaver();
  // check for a tag change
  const CMusicInfoTag* tag = g_infoManager.GetCurrentSongTag();
  if (tag && *tag != m_tag)
  { // need to fade in then out again
    m_tag = *tag;

    // fade in
    if((int)g_infoManager.GetFPS() == 0)
    {
      m_initTimer  = g_advancedSettings.m_songInfoDuration * DEFAULT_FPS;
    }
    else
    {
      m_initTimer = g_advancedSettings.m_songInfoDuration * (int)g_infoManager.GetFPS();
    }

    g_infoManager.SetShowInfo(true);
  }
  if (m_initTimer)
  {
    m_initTimer--;
    if (!m_initTimer && !g_stSettings.m_bMyMusicSongThumbInVis)
    { // reached end of fade in, fade out again
      g_infoManager.SetShowInfo(false);
    }
  }
  // show or hide the locked texture
  if (m_lockedTimer)
  {
    m_lockedTimer--;
    if (!m_lockedTimer && !m_bShowPreset)
      g_infoManager.SetShowCodec(false);
  }
  CGUIWindow::Render();
}
