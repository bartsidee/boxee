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
#include "GUIDialogAudioSubtitleSettings.h"
#include "GUIDialogBoxeeBrowseSubtitleSettings.h"
#include "GUIDialogFileBrowser.h"
#include "GUIPassword.h"
#include "Util.h"
#include "Application.h"
#include "VideoDatabase.h"
#include "XBAudioConfig.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogSelect.h"
#include "GUIWindowManager.h"
#include "GUILabelControl.h"
#include "FileSystem/Directory.h"
#include "FileSystem/File.h"
#include "URL.h"
#include "FileItem.h"
#include "SkinInfo.h"
#include "Settings.h"
#include "AdvancedSettings.h"
#include "GUISettings.h"
#include "LocalizeStrings.h"
#include "LangCodeExpander.h"

using namespace std;
using namespace XFILE;
using namespace DIRECTORY;

#ifdef HAS_VIDEO_PLAYBACK
extern void xbox_audio_switch_channel(int iAudioStream, bool bAudioOnAllSpeakers); //lowlevel audio
#endif

CGUIDialogAudioSubtitleSettings::CGUIDialogAudioSubtitleSettings(void)
    : CGUIDialogSettings(WINDOW_DIALOG_AUDIO_OSD_SETTINGS, "boxee_osd_audio_settings.xml")
{
}

CGUIDialogAudioSubtitleSettings::~CGUIDialogAudioSubtitleSettings(void)
{
}

#ifdef NEW_AUDIO_OSD

#define AUDIO_SETTINGS_STREAM             4
#define SUBTITLE_SETTINGS_STREAM          7
#define AUDIO_STREAM_LIST                 31
#define SUBTITLE_STREAM_LIST              30
#else
#define AUDIO_SETTINGS_STREAM             4
#define SUBTITLE_SETTINGS_STREAM          7
#define AUDIO_STREAM_LIST                 31
#define SUBTITLE_STREAM_LIST              30
#endif

// separator 7
#define AUDIO_SETTINGS_MAKE_DEFAULT      12

#define AUDIO_SETTINGS_TITLE           6

void CGUIDialogAudioSubtitleSettings::CreateSettings()
{
  m_usePopupSliders = g_SkinInfo.HasSkinFile("DialogSlider.xml");

  m_audioStreamsList.Clear();
  CGUIMessage msgAudioReset(GUI_MSG_LABEL_RESET, GetID(), AUDIO_STREAM_LIST);
  OnMessage(msgAudioReset);

  m_subtitleStreamsList.Clear();
  CGUIMessage msgSubtitleReset(GUI_MSG_LABEL_RESET, GetID(), SUBTITLE_STREAM_LIST);
  OnMessage(msgSubtitleReset);

  // clear out any old settings
  m_settings.clear();

  m_subtitleVisible = g_application.m_pPlayer->GetSubtitleVisible();
#ifdef NEW_AUDIO_OSD
  // create our settings
  AddSubtitleStreams(SUBTITLE_SETTINGS_STREAM);
  AddAudioStreams(AUDIO_SETTINGS_STREAM);
#else
  AddSubtitleStreams(SUBTITLE_SETTINGS_STREAM);
  AddAudioStreams(AUDIO_SETTINGS_STREAM);
#endif
}

void CGUIDialogAudioSubtitleSettings::AddAudioStreams(unsigned int id)
{
  SettingInfo setting;
  setting.id = id;
  setting.name = g_localizeStrings.Get(460);
  setting.type = SettingInfo::BUTTON;
  setting.min = 0;
  setting.data = &m_audioStream;
  // get the number of audio strams for the current movie
  setting.max = (float)g_application.m_pPlayer->GetAudioStreamCount() - 1;
  m_audioStream = g_application.m_pPlayer->GetAudioStream();
  setting.label2 = "";

  if( m_audioStream < 0 ) m_audioStream = 0;

  // check if we have a single, stereo stream, and if so, allow us to split into
  // left, right or both
  if (!setting.max)
  {
    CStdString strAudioInfo;
    g_application.m_pPlayer->GetAudioInfo(strAudioInfo);
    int iNumChannels = atoi(strAudioInfo.Right(strAudioInfo.size() - strAudioInfo.Find("chns:") - 5).c_str());
    CStdString strAudioCodec = strAudioInfo.Mid(7, strAudioInfo.Find(") VBR") - 5);
    bool bDTS = strstr(strAudioCodec.c_str(), "DTS") != 0;
    bool bAC3 = strstr(strAudioCodec.c_str(), "AC3") != 0;
    if (iNumChannels == 2 && !(bDTS || bAC3))
    { // ok, enable these options
/*      if (g_stSettings.m_currentVideoSettings.m_AudioStream == -1)
      { // default to stereo stream
        g_stSettings.m_currentVideoSettings.m_AudioStream = 0;
      }*/
      setting.max = 2;
      m_audioStream = -g_stSettings.m_currentVideoSettings.m_AudioStream - 1;
      for (int i = 0; i <= setting.max; i++)
      {
        CStdString label = g_localizeStrings.Get(13320 + i);
        CFileItemPtr audioStream ( new CFileItem(label)  );
        if(i == m_audioStream)
        {
          audioStream->SetProperty("IsSelected",true);
        }
        else
        {
          audioStream->SetProperty("IsSelected",false);
        }
        m_audioStreamsList.Add(audioStream);
        setting.entry.push_back(label);
      }
      m_settings.push_back(setting);

      CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), AUDIO_STREAM_LIST, 0, 0, &m_audioStreamsList);
      OnMessage(msg);
      return;
    }
  }

  // cycle through each audio stream and add it to our list control
  for (int i = 0; i <= setting.max; ++i)
  {
    CStdString strItem;
    CStdString strName;
    CStdString strDetails;
    g_application.m_pPlayer->GetAudioStreamLang(i, strName);
    g_application.m_pPlayer->GetAudioStreamName(i, strDetails);

    CStdString language;
    g_LangCodeExpander.Lookup(language, strName);
    strName = language;

    if (strName.length() == 0)
      strName = "Unknown";

    if( CStdString(strName).ToLower() == CStdString(strDetails).ToLower())
      strDetails = "";

    CStdString sep = (!strName.IsEmpty() && !strDetails.IsEmpty()) ? " - " : "";

    strItem.Format(" %s%s%s ", strName.c_str(), sep.c_str(), strDetails.c_str());
    setting.entry.push_back(strItem);

    CFileItemPtr audioStream ( new CFileItem(strItem)  );

    if(i == m_audioStream)
    {
      audioStream->SetProperty("IsSelected",true);
      CStdString label2;
      label2.Format("%s ", strName.c_str());
      setting.label2 = label2;
      SetAudioTitle();
    }
    else
    {
      audioStream->SetProperty("IsSelected",false);
    }

    m_audioStreamsList.Add(audioStream);
  }

  if( setting.max < 0 )
  {
    CStdString label = g_localizeStrings.Get(231);
    CFileItemPtr audioStream ( new CFileItem(label)  );
    m_audioStreamsList.Add(audioStream);

    setting.max = 0;
    setting.label2 = "None";
    setting.entry.push_back(g_localizeStrings.Get(231).c_str());
  }

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), AUDIO_STREAM_LIST, 0, 0, &m_audioStreamsList);
  OnMessage(msg);
  SetItemSelected(AUDIO_STREAM_LIST,m_audioStream);
  m_settings.push_back(setting);

}

void CGUIDialogAudioSubtitleSettings::AddSubtitleStreams(unsigned int id)
{
  SettingInfo setting;

  setting.id = id;
  setting.name = g_localizeStrings.Get(55301);
  setting.type = SettingInfo::BUTTON;
  setting.min = 0;
  setting.data = &m_subtitleStream;
  m_subtitleStream = g_application.m_pPlayer->GetSubtitle();
  setting.label2 = "";

  // get the number of subtitles streams for the current movie
  setting.max = (float)g_application.m_pPlayer->GetSubtitleCount() - 1;

  //add Off...
  CStdString offLabel = g_localizeStrings.Get(90214);
  CFileItemPtr subtitleOff ( new CFileItem(offLabel)  );
  if(!m_subtitleVisible)
  {
    m_subtitleStream = -1;
    subtitleOff->SetProperty("IsSelected",true);
  }
  else
  {
    subtitleOff->SetProperty("IsSelected",false);
  }
  m_subtitleStreamsList.Add(subtitleOff);

  // cycle through each subtitle and add it to our entry list
  for (int i = 0; i <= setting.max; ++i)
  {
    CStdString strItem = "";
    CStdString strName = "";
    CStdString strDetails = "";
    g_application.m_pPlayer->GetSubtitleLang(i, strName);
    g_application.m_pPlayer->GetSubtitleName(i, strDetails);

    CStdString language;
    g_LangCodeExpander.Lookup(language, strName);
    strName = language;

    if (strName.length() == 0)
      strName = "Unknown";

    if( CStdString(strName).ToLower() == CStdString(strDetails).ToLower())
      strDetails = "";

    CStdString sep = (!strName.IsEmpty() && !strDetails.IsEmpty()) ? " - " : "";

    strItem.Format("%s%s%s ", strName.c_str(), sep.c_str(), strDetails.c_str());
    setting.entry.push_back(strItem);

    CFileItemPtr subtitleStream ( new CFileItem(strItem)  );

    if(i == m_subtitleStream)
    {
      subtitleStream->SetProperty("IsSelected",true);
      CStdString label2;
      label2.Format("%s ", strName.c_str());
      setting.label2 = label2;
    }
    else
    {
      subtitleStream->SetProperty("IsSelected",false);
    }

    m_subtitleStreamsList.Add(subtitleStream);
  }

  //add Browse...
  m_browseSubtitleIndex = (int)setting.max + 2;
  CStdString browseLabel = g_localizeStrings.Get(20153);
  CFileItemPtr subtitleStream ( new CFileItem(browseLabel)  );
  m_subtitleStreamsList.Add(subtitleStream);

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), SUBTITLE_STREAM_LIST, 0, 0, &m_subtitleStreamsList);
  OnMessage(msg);
  SetItemSelected(SUBTITLE_STREAM_LIST,m_subtitleStream + 1);

  m_settings.push_back(setting);
}

void CGUIDialogAudioSubtitleSettings::OnSettingChanged(SettingInfo &setting)
{
#ifdef NEW_AUDIO_OSD
  // check and update anything that needs it

  else if (setting.id == AUDIO_SETTINGS_STREAM)
  {
    // first check if it's a stereo track that we can change between stereo, left and right
    if (g_application.m_pPlayer->GetAudioStreamCount() == 1)
    {
      if (setting.max == 2)
      { // we're in the case we want - call the code to switch channels etc.
        // update the screen setting...
        g_stSettings.m_currentVideoSettings.m_AudioStream = -1 - m_audioStream;
        // call monkeyh1's code here...
        //bool bAudioOnAllSpeakers = (g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL) && g_stSettings.m_currentVideoSettings.m_OutputToAllSpeakers;
        return;
      }
    }
    // only change the audio stream if a different one has been asked for
    if (g_application.m_pPlayer->GetAudioStream() != m_audioStream)
    {
      g_stSettings.m_currentVideoSettings.m_AudioStream = m_audioStream;
      g_application.m_pPlayer->SetAudioStream(m_audioStream);    // Set the audio stream to the one selected
    }
  }

#else
  if (setting.id == AUDIO_SETTINGS_STREAM)
  {
    // first check if it's a stereo track that we can change between stereo, left and right
    if (g_application.m_pPlayer->GetAudioStreamCount() == 1)
    {
      if (setting.max == 2)
      { // we're in the case we want - call the code to switch channels etc.
        // update the screen setting...
        g_stSettings.m_currentVideoSettings.m_AudioStream = -1 - m_audioStream;
        // call monkeyh1's code here...
        //bool bAudioOnAllSpeakers = (g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL) && g_stSettings.m_currentVideoSettings.m_OutputToAllSpeakers;
        return;
      }
    }

    if(setting.entry.size() > 1)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), AUDIO_STREAM_LIST);
      OnMessage(msg);
      int newItemSelected = msg.GetParam1();

      // only change the audio stream if a different one has been asked for
      if (g_application.m_pPlayer->GetAudioStream() != newItemSelected)
      {
        g_stSettings.m_currentVideoSettings.m_AudioStream = newItemSelected;
        g_application.m_pPlayer->SetAudioStream(newItemSelected);

        m_audioStreamsList.Get(m_audioStream)->SetProperty("IsSelected",false);
        m_audioStreamsList.Get(newItemSelected)->SetProperty("IsSelected",true);

        CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), AUDIO_STREAM_LIST, 0, 0, &m_audioStreamsList);
        OnMessage(msg);
        SetItemSelected(AUDIO_STREAM_LIST,newItemSelected);

        m_audioStream = newItemSelected;


        CStdString m_audioStreamString;
        CStdString m_label2;
        g_application.m_pPlayer->GetAudioStreamLang(m_audioStream, m_audioStreamString);
        g_LangCodeExpander.Lookup(m_label2, m_audioStreamString);

        setting.label2.Format(" %s (%i/%i) ", m_label2, m_audioStream+1, setting.entry.size());
      }
    }
  }
  else if (setting.id == SUBTITLE_SETTINGS_STREAM)
   {
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), SUBTITLE_STREAM_LIST);
    OnMessage(msg);
    int selectedSubtitle = msg.GetParam1();

    m_subtitleStreamsList.Get(m_subtitleStream + 1)->SetProperty("IsSelected",false);
    m_subtitleStreamsList.Get(selectedSubtitle)->SetProperty("IsSelected",true);

    CGUIMessage msgBind(GUI_MSG_LABEL_BIND, GetID(), SUBTITLE_STREAM_LIST, 0, 0, &m_subtitleStreamsList);
    OnMessage(msgBind);
    SetItemSelected(SUBTITLE_STREAM_LIST,selectedSubtitle);

    //handle selecting Off
    if(selectedSubtitle == 0)
    {
      m_subtitleStream = selectedSubtitle - 1;
      m_subtitleVisible = false;
      SetSubtitleVisibility();
      return;
    }

    //handle selecting browse
    else if(selectedSubtitle == m_browseSubtitleIndex)
    {
      OnBrowseSubtitleSelect();
      return;
    }

    else if(setting.entry.size() > 0)
    {
      if(!m_subtitleVisible)
      {
        m_subtitleVisible = true;
        SetSubtitleVisibility();
      }

      m_subtitleStream = selectedSubtitle - 1;
      g_stSettings.m_currentVideoSettings.m_SubtitleStream = m_subtitleStream;
      g_application.m_pPlayer->SetSubtitle(m_subtitleStream);

      CStdString m_langStreamString;
      CStdString m_label2;
      g_application.m_pPlayer->GetSubtitleLang(m_subtitleStream, m_langStreamString);
      g_LangCodeExpander.Lookup(m_label2, m_langStreamString);

      setting.label2.Format("%s (%i/%i) ", m_label2, m_subtitleStream+1, setting.entry.size());
    }
   }
#endif
}

void CGUIDialogAudioSubtitleSettings::SetItemSelected(int controlList, int item)
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), controlList, item);
  OnMessage(msg);
}


void CGUIDialogAudioSubtitleSettings::SetSubtitleVisibility()
{
  g_stSettings.m_currentVideoSettings.m_SubtitleOn = m_subtitleVisible;
  g_application.m_pPlayer->SetSubtitleVisible(g_stSettings.m_currentVideoSettings.m_SubtitleOn);
  if (!g_stSettings.m_currentVideoSettings.m_SubtitleCached && g_stSettings.m_currentVideoSettings.m_SubtitleOn)
  {
    g_application.Restart(true); // cache subtitles
    Close();
  }
}

void CGUIDialogAudioSubtitleSettings::OnBrowseSubtitleSelect()
{
  CGUIDialogBoxeeBrowseSubtitleSettings *pDialog = (CGUIDialogBoxeeBrowseSubtitleSettings*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_BROWSE_SUBTITLES_SETTINGS);

  if (pDialog)
  {
    Close();
    pDialog->DoModal();
  }
}

void CGUIDialogAudioSubtitleSettings::Render()
{
  CGUIDialogSettings::Render();
}

CStdString CGUIDialogAudioSubtitleSettings::FormatDecibel(float value, float interval)
{
  CStdString text;
  text.Format("%2.1f dB", value);
  return text;
}

CStdString CGUIDialogAudioSubtitleSettings::FormatDelay(float value, float interval)
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

void CGUIDialogAudioSubtitleSettings::SetAudioTitle()
{
  CGUILabelControl* pControl = (CGUILabelControl *) GetControl(AUDIO_SETTINGS_TITLE);
  if(!pControl) return;
  CStdString audioName;
  g_application.m_pPlayer->GetAudioStreamName(m_audioStream, audioName);
  audioName.Trim();
  if(!audioName.Equals("Unknown") && !audioName.Equals("(Invalid)"))
  {
    pControl->SetLabel(audioName);
  }
  else
  {
    pControl->SetLabel("");
  }
}
