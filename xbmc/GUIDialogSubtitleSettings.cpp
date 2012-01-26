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
#include "GUIDialogSubtitleSettings.h"
#include "GUIDialogFileBrowser.h"
#include "GUIPassword.h"
#include "Util.h"
#include "Application.h"
#include "VideoDatabase.h"
#include "XBAudioConfig.h"
#include "GUIDialogYesNo.h"
#include "FileSystem/Directory.h"
#include "FileSystem/File.h"
#include "URL.h"
#include "FileItem.h"
#include "SkinInfo.h"
#include "Settings.h"
#include "AdvancedSettings.h"
#include "GUISettings.h"
#include "LocalizeStrings.h"
#include "LangInfo.h"
#include "CharsetConverter.h"
#include "DVDPlayer.h"
#include "utils/TimeUtils.h"

using namespace std;
using namespace XFILE;
using namespace DIRECTORY;

#ifdef HAS_VIDEO_PLAYBACK
extern void xbox_audio_switch_channel(int iAudioStream, bool bAudioOnAllSpeakers); //lowlevel audio
#endif

CGUIDialogSubtitleSettings::CGUIDialogSubtitleSettings(void)
    : CGUIDialogSettings(WINDOW_DIALOG_SUBTITLE_OSD_SETTINGS, "VideoOSDSettings.xml"),m_reloadSubtitle (0)
{
}

CGUIDialogSubtitleSettings::~CGUIDialogSubtitleSettings(void)
{
}

// separator 7
#define SUBTITLE_SETTINGS_ENABLE          1
#define SUBTITLE_SETTINGS_DELAY           2
#define SUBTITLE_SETTINGS_STREAM          3
#define SUBTITLE_SETTINGS_BROWSER         4
#define SUBTITLE_SETTINGS_CHARSET         5

void CGUIDialogSubtitleSettings::CreateSettings()
{
  m_usePopupSliders = g_SkinInfo.HasSkinFile("DialogSlider.xml");

  // clear out any old settings
  m_settings.clear();

  m_reloadSubtitle = 0;
  // create our settings
  m_subtitleVisible = g_application.m_pPlayer->GetSubtitleVisible();
  AddBool(SUBTITLE_SETTINGS_ENABLE, 13397, &m_subtitleVisible);
  AddSlider(SUBTITLE_SETTINGS_DELAY, 22006, &g_stSettings.m_currentVideoSettings.m_SubtitleDelay, -g_advancedSettings.m_videoSubsDelayRange, 0.1f, g_advancedSettings.m_videoSubsDelayRange, FormatDelay);
  AddSubtitleStreams(SUBTITLE_SETTINGS_STREAM);
  AddButton(SUBTITLE_SETTINGS_BROWSER,13250);

  m_vecCharsetLabels.clear();
  m_vecCharsetLabels =  g_charsetConverter.getCharsetLabels();
  sort(m_vecCharsetLabels.begin(), m_vecCharsetLabels.end(), sortstringbyname());

  CStdString charsetLabel = g_charsetConverter.getCharsetLabelByName(g_langInfo.GetSubtitleCharSet());

  g_stSettings.m_currentVideoSettings.m_Charset = getCharsetPosByLabel(charsetLabel);

  if (g_stSettings.m_currentVideoSettings.m_Charset != -1)
  {
    AddSpin(SUBTITLE_SETTINGS_CHARSET, 735, m_vecCharsetLabels , &g_stSettings.m_currentVideoSettings.m_Charset);
  }

}

void CGUIDialogSubtitleSettings::AddSubtitleStreams(unsigned int id)
{
  SettingInfo setting;

  setting.id = id;
  setting.name = g_localizeStrings.Get(462);
  setting.type = SettingInfo::SPIN;
  setting.min = 0;
  setting.data = &m_subtitleStream;
  m_subtitleStream = g_application.m_pPlayer->GetSubtitle();

  if(m_subtitleStream < 0) m_subtitleStream = 0;

  // get the number of audio strams for the current movie
  setting.max = (float)g_application.m_pPlayer->GetSubtitleCount() - 1;

  // cycle through each subtitle and add it to our entry list
  for (int i = 0; i <= setting.max; ++i)
  {
    CStdString strItem;
    CStdString strName;
    g_application.m_pPlayer->GetSubtitleName(i, strName);
    if (strName.length() == 0)
      strName = "Unnamed";

    strItem.Format("%s (%i/%i)", strName.c_str(), i + 1, (int)setting.max + 1);

    setting.entry.push_back(strItem);
  }

  if (setting.max < 0)
  { // no subtitle streams - just add a "None" entry
    m_subtitleStream = 0;
    setting.max = 0;
    setting.entry.push_back(g_localizeStrings.Get(231).c_str());
  }
  m_settings.push_back(setting);
}

void CGUIDialogSubtitleSettings::OnSettingChanged(SettingInfo &setting)
{
  if (setting.id == SUBTITLE_SETTINGS_ENABLE)
  {
    g_stSettings.m_currentVideoSettings.m_SubtitleOn = m_subtitleVisible;
    g_application.m_pPlayer->SetSubtitleVisible(g_stSettings.m_currentVideoSettings.m_SubtitleOn);
    if (!g_stSettings.m_currentVideoSettings.m_SubtitleCached && g_stSettings.m_currentVideoSettings.m_SubtitleOn)
    {
      g_application.Restart(true); // cache subtitles
      Close();
    }
  }
  else if (setting.id == SUBTITLE_SETTINGS_DELAY)
  {
    g_application.m_pPlayer->SetSubTitleDelay(g_stSettings.m_currentVideoSettings.m_SubtitleDelay);
  }
  else if (setting.id == SUBTITLE_SETTINGS_STREAM && setting.max > 0)
  {
    g_stSettings.m_currentVideoSettings.m_SubtitleStream = m_subtitleStream;
    g_application.m_pPlayer->SetSubtitle(m_subtitleStream);
  }
  else if (setting.id == SUBTITLE_SETTINGS_BROWSER)
  {
    CStdString strPath;
    if (CUtil::IsInRAR(g_application.CurrentFileItem().m_strPath) || CUtil::IsInZIP(g_application.CurrentFileItem().m_strPath))
    {
      CURL url(g_application.CurrentFileItem().m_strPath);
      strPath = url.GetHostName();
    }
    else
      strPath = g_application.CurrentFileItem().m_strPath;

    CStdString strMask = ".utf|.utf8|.utf-8|.sub|.srt|.smi|.rt|.txt|.ssa|.aqt|.jss|.ass|.idx|.rar|.zip";
    if (g_application.GetCurrentPlayer() == EPC_DVDPLAYER)
      strMask = ".srt|.rar|.zip|.ifo|.smi|.sub|.idx|.ass|.ssa|.txt";
    VECSOURCES shares(g_settings.m_videoSources);
    if (g_stSettings.iAdditionalSubtitleDirectoryChecked != -1 && !g_guiSettings.GetString("subtitles.custompath").IsEmpty())
    {
      CMediaSource share;
      std::vector<CStdString> paths;
      CStdString strPath1;
      CUtil::GetDirectory(strPath,strPath1);
      paths.push_back(strPath1);
      strPath1 = g_guiSettings.GetString("subtitles.custompath");
      paths.push_back(g_guiSettings.GetString("subtitles.custompath"));
      share.FromNameAndPaths("video",g_localizeStrings.Get(21367),paths);
      shares.push_back(share);
      strPath = share.strPath;
      CUtil::AddSlashAtEnd(strPath);
    }
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares,strMask,g_localizeStrings.Get(293),strPath,false,true)) // "subtitles"
    {
      CStdString strExt;
      CUtil::GetExtension(strPath,strExt);
      if (strExt.CompareNoCase(".idx") == 0 || strExt.CompareNoCase(".sub") == 0)
      {
        // Playback could end and delete m_pPlayer while dialog is up so make sure it's valid
       	if (g_application.m_pPlayer)
        {
          if (CFile::Cache(strPath,"special://temp/subtitle"+strExt))
          {
            CStdString strPath2;
            CStdString strPath3;
            if (strExt.CompareNoCase(".idx") == 0)
            {
              CUtil::ReplaceExtension(strPath,".sub",strPath2);
              strPath3 = "special://temp/subtitle.sub";
            }
            else
            {
              CUtil::ReplaceExtension(strPath,".idx",strPath2);
              if (!CFile::Exists(strPath2) && (CUtil::IsInRAR(strPath2) || CUtil::IsInZIP(strPath2)))
              {
                CStdString strFileName = CUtil::GetFileName(strPath);
                CUtil::GetDirectory(strPath,strPath3);
                CUtil::GetParentPath(strPath3,strPath2);
                CUtil::AddFileToFolder(strPath2,strFileName,strPath2);
                CUtil::ReplaceExtension(strPath2,".idx",strPath2);
              }
              strPath3 = "special://temp/subtitle.idx";
            }
            if (CFile::Exists(strPath2))
              CFile::Cache(strPath2,strPath3);
            else
            {
              CFileItemList items;
              CStdString strDir,strFileNameNoExtNoCase;
              CUtil::Split(strPath,strDir,strPath3);
              CUtil::ReplaceExtension(strPath3,".",strFileNameNoExtNoCase);
              strFileNameNoExtNoCase.ToLower();
              CUtil::GetDirectory(strPath,strDir);
              CDirectory::GetDirectory(strDir,items,".rar|.zip",false);
              vector<CStdString> vecExts;
              for (int i=0;i<items.Size();++i)
                CUtil::CacheRarSubtitles(vecExts,items[i]->m_strPath,strFileNameNoExtNoCase,"");
            }
            g_stSettings.m_currentVideoSettings.m_SubtitleOn = true;

            if(g_application.m_pPlayer->AddSubtitle("special://temp/subtitle.idx"))
            {
              m_subtitleStream = g_application.m_pPlayer->GetSubtitleCount() - 1;
              g_application.m_pPlayer->SetSubtitle(m_subtitleStream);
              g_application.m_pPlayer->SetSubtitleVisible(true);
            }

            Close();
          }
        }
      }
      else
      {
        m_subtitleStream = g_application.m_pPlayer->GetSubtitleCount();
        CStdString strExt;
        CUtil::GetExtension(strPath,strExt);
        if (CFile::Cache(strPath,"special://temp/subtitle.browsed"+strExt))
        {
          g_stSettings.m_currentVideoSettings.m_SubtitleOn = true;
          g_application.m_pPlayer->SetSubtitleVisible(true);
          g_application.m_pPlayer->AddSubtitle("special://temp/subtitle.browsed"+strExt);
          g_application.m_pPlayer->SetSubtitle(m_subtitleStream);
        }

        Close();
      }
      g_stSettings.m_currentVideoSettings.m_SubtitleCached = true;
    }
  }
  else if (setting.id == SUBTITLE_SETTINGS_CHARSET)
  {
    CStdString charSetName = g_charsetConverter.getCharsetNameByLabel(m_vecCharsetLabels[g_stSettings.m_currentVideoSettings.m_Charset]);
    if (!charSetName.Equals(g_langInfo.GetSubtitleCharSet()))
    {
      m_reloadSubtitle = CTimeUtils::GetFrameTime() + 2000;
    }
    else
    {
      m_reloadSubtitle = 0;
    }

  }
}

void CGUIDialogSubtitleSettings::ReloadSubtitle()
{
	CStdString charSetName = g_charsetConverter.getCharsetNameByLabel(m_vecCharsetLabels[g_stSettings.m_currentVideoSettings.m_Charset]);
    g_guiSettings.SetString("subtitles.charset", charSetName);
    g_charsetConverter.reset();
    g_application.m_pPlayer->RestartSubtitleStream();
}

void CGUIDialogSubtitleSettings::Render()
{
  m_volume = g_stSettings.m_nVolumeLevel * 0.01f;
  if (g_application.m_pPlayer)
  {
    // these settings can change on the fly
    UpdateSetting(SUBTITLE_SETTINGS_ENABLE);
    UpdateSetting(SUBTITLE_SETTINGS_DELAY);
  }

  if (m_reloadSubtitle &&  CTimeUtils::GetFrameTime() >= m_reloadSubtitle)
  {
    m_reloadSubtitle = 0;
    ReloadSubtitle();
  }
  CGUIDialogSettings::Render();
}

CStdString CGUIDialogSubtitleSettings::FormatDelay(float value, float interval)
{
  CStdString text;
  if (fabs(value) < 0.5f*interval)
    text = "0.000s";
  else if (value < 0)
    text.Format(g_localizeStrings.Get(22004).c_str(), fabs(value));
  else
    text.Format(g_localizeStrings.Get(22005).c_str(), value);
  return text;
}




unsigned int CGUIDialogSubtitleSettings::getCharsetPosByLabel(const CStdString& charsetName)
{
  for (unsigned int i = 0; i < m_vecCharsetLabels.size(); i++)
  {
    if (m_vecCharsetLabels[i].Equals(charsetName))
    {
      return i;
    }
  }

  return -1;
}

CStdString CGUIDialogSubtitleSettings::getCharsetLabelByPos(int pos)
{
  CStdString empty = "";

  if( (int)m_vecCharsetLabels.size() > pos)
  {
    return m_vecCharsetLabels[pos];
  }
  return empty;
}

