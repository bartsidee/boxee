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

#include "system.h"
#include "LocalizeStrings.h"
#include "GUIDialogVideoSettings.h"
#include "GUIWindowManager.h"
#include "GUIDialogSelect.h"
#include "GUIButtonControl.h"
#include "GUIPassword.h"
#include "Util.h"
#include "MathUtils.h"
#include "GUISettings.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#include "VideoDatabase.h"
#include "GUIDialogYesNo.h"
#include "Settings.h"
#include "SkinInfo.h"
#include "Application.h"

#ifdef HAVE_LIBVDPAU
#include "cores/dvdplayer/DVDCodecs/Video/VDPAU.h"
#endif

CGUIDialogVideoSettings::CGUIDialogVideoSettings(void)
    : CGUIDialogSettings(WINDOW_DIALOG_VIDEO_OSD_SETTINGS, "custom_boxee_osd_advanced.xml"),m_advance(false)
{
}

CGUIDialogVideoSettings::~CGUIDialogVideoSettings(void)
{
}

#ifdef NEW_VIDEO_OSD
#define VIDEO_SETTINGS_VIEW_MODE          2
#define SUBTITLE_OFFSET_SLIDER            22
#define AUDIO_OFFSET_SLIDER               23
#else
#define VIDEO_SETTINGS_VIEW_MODE          2
#define SUBTITLE_OFFSET_SLIDER            22
#define AUDIO_OFFSET_SLIDER               23
#endif

const int g_entries[] = {630, 631, 632, 633, 634, 635, 636 };

void CGUIDialogVideoSettings::CreateSettings()
{
  m_usePopupSliders = g_SkinInfo.HasSkinFile("DialogSlider.xml");
  // clear out any old settings
  m_settings.clear();
#ifdef NEW_VIDEO_OSD

  AddSlider(SUBTITLE_OFFSET_SLIDER, 22006,&g_stSettings.m_currentVideoSettings.m_SubtitleDelay,-g_advancedSettings.m_videoSubsDelayRange, 0.25, g_advancedSettings.m_videoSubsDelayRange, FormatFloat);
  AddSlider(AUDIO_OFFSET_SLIDER, 297, &g_stSettings.m_currentVideoSettings.m_AudioDelay,-g_advancedSettings.m_videoAudioDelayRange, 1, g_advancedSettings.m_videoAudioDelayRange, FormatInteger);
  AddButton(VIDEO_SETTINGS_VIEW_MODE, 629,g_stSettings.m_currentVideoSettings.m_ViewMode,g_localizeStrings.Get(g_entries[g_stSettings.m_currentVideoSettings.m_ViewMode]));
#else
  AddSlider(SUBTITLE_OFFSET_SLIDER, 22006,&g_stSettings.m_currentVideoSettings.m_SubtitleDelay,-g_advancedSettings.m_videoSubsDelayRange, 0.25, g_advancedSettings.m_videoSubsDelayRange, FormatFloat);
  AddSlider(AUDIO_OFFSET_SLIDER, 297, &g_stSettings.m_currentVideoSettings.m_AudioDelay,-g_advancedSettings.m_videoAudioDelayRange, 0.01, g_advancedSettings.m_videoAudioDelayRange, FormatFloat);
  AddButton(VIDEO_SETTINGS_VIEW_MODE, 629,&g_stSettings.m_currentVideoSettings.m_ViewMode,g_localizeStrings.Get(g_entries[g_stSettings.m_currentVideoSettings.m_ViewMode]));
#endif
}

void CGUIDialogVideoSettings::SetupPage()
{
  CGUIDialogSettings::SetupPage();
}

void CGUIDialogVideoSettings::OnSettingChanged(SettingInfo &setting)
{
#ifdef NEW_VIDEO_OSD
  // check and update anything that needs it
#ifdef HAS_VIDEO_PLAYBACK
  if (setting.id == VIDEO_SETTINGS_VIEW_MODE)
  {
    if(OnViewModeChange())
    {
      int selectedLabel = g_stSettings.m_currentVideoSettings.m_ViewMode;
      selectedLabel = g_entries[selectedLabel];
      CStdString label = g_localizeStrings.Get(selectedLabel);
      setting.label2 = label;
    }
  }
  else if (setting.id == AUDIO_OFFSET_SLIDER)
  {
    if (g_application.m_pPlayer)
      g_application.m_pPlayer->SetAVDelay(g_stSettings.m_currentVideoSettings.m_AudioDelay);
  }
  else if (setting.id == SUBTITLE_OFFSET_SLIDER)
  {
    if (g_application.m_pPlayer)
      g_application.m_pPlayer->SetSubTitleDelay(g_stSettings.m_currentVideoSettings.m_SubtitleDelay);
  }
  else 
#endif
#ifdef HAS_XBOX_HARDWARE
#endif
#else
    if (setting.id == VIDEO_SETTINGS_VIEW_MODE)
    {
      if(OnViewModeChange())
      {
        int selectedLabel = g_stSettings.m_currentVideoSettings.m_ViewMode;
        selectedLabel = g_entries[selectedLabel];
        CStdString label = g_localizeStrings.Get(selectedLabel);
        setting.label2 = label;
      }
    }
    else if (setting.id == AUDIO_OFFSET_SLIDER)
    {
      if (g_application.m_pPlayer)
        g_application.m_pPlayer->SetAVDelay(g_stSettings.m_currentVideoSettings.m_AudioDelay);
    }
    else if (setting.id == SUBTITLE_OFFSET_SLIDER)
    {
      if (g_application.m_pPlayer)
        g_application.m_pPlayer->SetSubTitleDelay(g_stSettings.m_currentVideoSettings.m_SubtitleDelay);
    }
#endif
}

bool CGUIDialogVideoSettings::OnViewModeChange()
{
  CGUIDialogSelect *pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);

  if(pDlgSelect)
  {
    pDlgSelect->Reset();
    pDlgSelect->SetHeading(21443);

    for(size_t i = 0; i < 7; i++)
    {
      pDlgSelect->Add(g_localizeStrings.Get(g_entries[i]));
    }

    pDlgSelect->DoModal();

    if(pDlgSelect->IsConfirmed())
    {
      g_stSettings.m_currentVideoSettings.m_ViewMode = pDlgSelect->GetSelectedLabel();
      g_renderManager.SetViewMode(g_stSettings.m_currentVideoSettings.m_ViewMode);
      g_stSettings.m_currentVideoSettings.m_CustomZoomAmount = g_stSettings.m_fZoomAmount;
      g_stSettings.m_currentVideoSettings.m_CustomPixelRatio = g_stSettings.m_fPixelRatio;

      return true;
    }
  }
  return false;
}

CStdString CGUIDialogVideoSettings::FormatInteger(float value, float minimum)
{
  CStdString text;
  text.Format("%i", MathUtils::round_int(value));
  return text;
}

CStdString CGUIDialogVideoSettings::FormatFloat(float value, float minimum)
{
  CStdString text;
  text.Format("%2.2f", value);
  return text;
}

