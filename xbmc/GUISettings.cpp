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

#include "GUISettings.h"
#include <limits.h>
#include "Settings.h"
#include "GUIDialogFileBrowser.h"
#ifdef HAS_XBOX_HARDWARE
#include "utils/FanController.h"
#endif
#include "XBAudioConfig.h"
#include "MediaManager.h"
#ifdef _LINUX
#include "linux/LinuxTimezone.h"
#endif
#include "utils/Network.h"
#include "Application.h"
#include "utils/SystemInfo.h"
#include "FileSystem/SpecialProtocol.h"
#include "Util.h"
#include "AdvancedSettings.h"
#include "LocalizeStrings.h"
#include "StringUtils.h"
#include "utils/log.h"
#include "tinyXML/tinyxml.h"
#include "FileSystem/File.h"

using namespace std;

// String id's of the masks
#define MASK_MINS   14044
#define MASK_SECS   14045
#define MASK_MS    14046
#define MASK_PERCENT 14047
#define MASK_KBPS   14048
#define MASK_KB    14049
#define MASK_DB    14050

#define MAX_RESOLUTIONS 128

#define TEXT_OFF 351

class CGUISettings g_guiSettings;

struct sortsettings
{
  bool operator()(const CSetting* pSetting1, const CSetting* pSetting2)
  {
    return pSetting1->GetOrder() < pSetting2->GetOrder();
  }
};

CStdString CSetting::GetLabelStr()
{
  if (m_useILabel)
  {
    return g_localizeStrings.Get(m_iLabel);
  }

  return m_labelStr;
}

void CSettingBool::FromString(const CStdString &strValue)
{
  m_bData = (strValue == "true");
}

CStdString CSettingBool::ToString()
{
  return m_bData ? "true" : "false";
}

CSettingSeparator::CSettingSeparator(int iOrder, const char *strSetting)
    : CSetting(iOrder, strSetting, 0, SEPARATOR_CONTROL)
{
}

CSettingFloat::CSettingFloat(int iOrder, const char *strSetting, int iLabel, float fData, float fMin, float fStep, float fMax, int iControlType)
    : CSetting(iOrder, strSetting, iLabel, iControlType)
{
  m_fData = fData;
  m_fMin = fMin;
  m_fStep = fStep;
  m_fMax = fMax;
}

void CSettingFloat::FromString(const CStdString &strValue)
{
  SetData((float)atof(strValue.c_str()));
}

CStdString CSettingFloat::ToString()
{
  CStdString strValue;
  strValue.Format("%f", m_fData);
  return strValue;
}

CSettingInt::CSettingInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
    : CSetting(iOrder, strSetting, iLabel, iControlType)
{
  m_iData = iData;
  m_iMin = iMin;
  m_iMax = iMax;
  m_iStep = iStep;
  m_iFormat = -1;
  m_iLabelMin = -1;
  if (strFormat)
    m_strFormat = strFormat;
  else
    m_strFormat = "%i";
}

CSettingInt::CSettingInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, int iFormat, int iLabelMin)
    : CSetting(iOrder, strSetting, iLabel, iControlType)
{
  m_iData = iData;
  m_iMin = iMin;
  m_iMax = iMax;
  m_iStep = iStep;
  m_iLabelMin = iLabelMin;
  m_iFormat = iFormat;
  if (m_iFormat < 0)
    m_strFormat = "%i";
}

void CSettingInt::FromString(const CStdString &strValue)
{
  SetData(atoi(strValue.c_str()));
}

CStdString CSettingInt::ToString()
{
  CStdString strValue;
  strValue.Format("%i", m_iData);
  return strValue;
}

void CSettingHex::FromString(const CStdString &strValue)
{
  int iHexValue;
  if (sscanf(strValue, "%x", (unsigned int *)&iHexValue))
    SetData(iHexValue);
}

CStdString CSettingHex::ToString()
{
  CStdString strValue;
  strValue.Format("%x", m_iData);
  return strValue;
}

CSettingString::CSettingString(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
    : CSetting(iOrder, strSetting, iLabel, iControlType)
{
  m_strData = strData;
  m_bAllowEmpty = bAllowEmpty;
  m_iHeadingString = iHeadingString;
}
CSettingString::CSettingString(int iOrder, const char *strSetting, CStdString labelStr, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
    : CSetting(iOrder, strSetting, 0, iControlType, false, labelStr)
{
  m_strData = strData;
  m_bAllowEmpty = bAllowEmpty;
  m_iHeadingString = iHeadingString;
}

void CSettingString::FromString(const CStdString &strValue)
{
  m_strData = strValue;
}

CStdString CSettingString::ToString()
{
  return m_strData;
}

CSettingPath::CSettingPath(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
    : CSettingString(iOrder, strSetting, iLabel, strData, iControlType, bAllowEmpty, iHeadingString)
{
}

void CSettingsGroup::GetCategories(vecSettingsCategory &vecCategories)
{
  if (g_guiSettings.IsUsingCustomSettingsOrder())
  {
    vecCategories = m_vecCategories;
  }
  else
  {
    vecCategories.clear();
    for (unsigned int i = 0; i < m_vecCategories.size(); i++)
    {
      vecSettings settings;
      // check whether we actually have these settings available.
      g_guiSettings.GetSettingsGroup(m_vecCategories[i]->m_strCategory, settings);
      if (settings.size())
        vecCategories.push_back(m_vecCategories[i]);
    }
  }
}

// Settings are case sensitive
CGUISettings::CGUISettings(void)
{
}

void CGUISettings::GetInfoPage(CStdString *str, CStdString params) {
  std::map<CStdString, CSetting*>::iterator nitr;
  for(nitr = settingsMap.begin(); nitr != settingsMap.end(); ++nitr) {
    *str += nitr->first + "  :  " + nitr->second->ToString() + "\n";
  }
}

void CGUISettings::Initialize()
{
  ZeroMemory(&m_replayGain, sizeof(ReplayGainSettings));
  g_application.m_infoPage.Register(INFOPAGE_GUISETTINGS, "GUISettings", this);

  // Pictures settings
  AddGroup(0, 1);
  AddCategory(0, "pictures", 16000);
  AddBool(0, "pictures.showvideos", 22022, false);
  AddBool(0, "pictures.generatethumbs",13360,true);
  AddSeparator(0,"pictures.sep1");
  AddBool(0, "pictures.useexifrotation", 20184, true);
  AddBool(0, "pictures.usetags", 258, true);
  // FIXME: hide this setting until it is properly respected. In the meanwhile, default to AUTO.
  //AddInt(8, "pictures.displayresolution", 169, (int)RES_AUTORES, (int)RES_HDTV_1080i, 1, (int)CUSTOM+MAX_RESOLUTIONS, SPIN_CONTROL_TEXT);
  AddInt(0, "pictures.displayresolution", 169, (int)RES_AUTORES, (int)RES_AUTORES, 1, (int)RES_AUTORES, SPIN_CONTROL_TEXT);
  AddSeparator(1,"pictures.sep2");
  AddPath(2,"pictures.screenshotpath",20004,"special://home/screenshots",BUTTON_CONTROL_PATH_INPUT,false,657);

  AddCategory(0, "slideshow", 108);
  AddInt(1, "slideshow.staytime", 12378, 5, 1, 1, 100, SPIN_CONTROL_INT_PLUS, MASK_SECS);
  AddInt(2, "slideshow.transistiontime", 225, 2500, 100, 100, 10000, SPIN_CONTROL_INT_PLUS, MASK_MS);
  AddBool(3, "slideshow.displayeffects", 12379, true);
  AddBool(0, "slideshow.shuffle", 13319, false);

  // parental control settings
  AddGroup(1, 0);
  AddCategory(1, "parental.control", 801);
  AddSeparator(0, "parental.sep1");

  // My Weather settings
  AddGroup(2, 8);
  AddCategory(2, "weather", 16000);
  AddString(1, "weather.areacode1", 14019, "USNY0996 - New York, NY", BUTTON_CONTROL_STANDARD);
  AddString(0, "weather.areacode2", 14020, "UKXX0085 - London, United Kingdom", BUTTON_CONTROL_STANDARD);
  AddString(0, "weather.areacode3", 14021, "JAXX0085 - Tokyo, Japan", BUTTON_CONTROL_STANDARD);
  AddSeparator(0, "weather.sep1");
  AddString(0, "weather.plugin", 23000, "", SPIN_CONTROL_TEXT, true);
  AddSeparator(0, "weather.sep2");
  AddString(0, "weather.jumptolocale", 20026, "", BUTTON_CONTROL_STANDARD);

  // My Music Settings
  AddGroup(3, 2);
  AddCategory(3, "mymusic", 16000);
#ifdef HAS_EMBEDDED
  AddString(1, "mymusic.visualisation", 250, "opengl_spectrum.vis", SPIN_CONTROL_TEXT);
#elif defined(_LINUX)
  AddString(1, "mymusic.visualisation", 250, "ProjectM.vis", SPIN_CONTROL_TEXT);
#elif defined(_WIN32)
#ifdef HAS_DX
  AddString(1, "mymusic.visualisation", 250, "MilkDrop.vis", SPIN_CONTROL_TEXT);
#else
  AddString(1, "mymusic.visualisation", 250, "ProjectM.vis", SPIN_CONTROL_TEXT);
#endif
#endif
  AddSeparator(2, "mymusic.sep1");
  AddBool(3, "mymusic.autoplaynextitem", 489, true);
  //AddBool(4, "musicfiles.repeat", 488, false);
  AddBool(0, "mymusic.clearplaylistsonend",239,false);
  AddSeparator(0, "mymusic.sep2");
  AddString(0,"mymusic.recordingpath",20005,"select writable folder",BUTTON_CONTROL_PATH_INPUT,false,657);

  AddCategory(0,"musiclibrary",14022);
  AddBool(0, "musiclibrary.albumartistsonly", 13414, false);
  AddSeparator(0,"musiclibrary.sep1");
  AddBool(0,"musiclibrary.downloadinfo", 20192, false);
  AddString(0, "musiclibrary.defaultscraper", 20194, "Allmusic", SPIN_CONTROL_TEXT);
  AddString(0, "musiclibrary.scrapersettings", 21417, "", BUTTON_CONTROL_STANDARD);
  AddBool(0, "musiclibrary.updateonstartup", 22000, false);
  AddBool(0, "musiclibrary.backgroundupdate", 22001, false);
  AddSeparator(0,"musiclibrary.sep2");
  AddString(0, "musiclibrary.cleanup", 334, "", BUTTON_CONTROL_STANDARD);
  AddString(0, "musiclibrary.export", 20196, "", BUTTON_CONTROL_STANDARD);
  AddString(0, "musiclibrary.import", 20197, "", BUTTON_CONTROL_STANDARD);

  AddCategory(3, "musicplayer", 16003);
  AddString(0, "musicplayer.jumptoaudiohardware", 16001, "", BUTTON_CONTROL_STANDARD);
  AddSeparator(0, "musicplayer.sep1");
  AddInt(0, "musicplayer.replaygaintype", 638, REPLAY_GAIN_ALBUM, REPLAY_GAIN_NONE, 1, REPLAY_GAIN_TRACK, SPIN_CONTROL_TEXT);
  AddInt(0, "musicplayer.replaygainpreamp", 641, 89, 77, 1, 101, SPIN_CONTROL_INT_PLUS, MASK_DB);
  AddInt(0, "musicplayer.replaygainnogainpreamp", 642, 89, 77, 1, 101, SPIN_CONTROL_INT_PLUS, MASK_DB);
  AddBool(0, "musicplayer.replaygainavoidclipping", 643, false);
  AddSeparator(0, "musicplayer.sep2");
  AddInt(0, "musicplayer.crossfade", 13314, 0, 0, 1, 10, SPIN_CONTROL_INT_PLUS, MASK_SECS, TEXT_OFF);
  AddBool(0, "musicplayer.crossfadealbumtracks", 13400, true);
  AddSeparator(0, "musicplayer.sep3");
  AddString(0, "musicplayer.jumptocache", 439, "", BUTTON_CONTROL_STANDARD);

  AddCategory(0, "musicfiles", 744);
  AddBool(0, "musicfiles.usetags", 258, true);
  AddString(0, "musicfiles.trackformat", 13307, "[%N. ]%A - %T", EDIT_CONTROL_INPUT, false, 16016);
  AddString(0, "musicfiles.trackformatright", 13387, "%D", EDIT_CONTROL_INPUT, false, 16016);
  // advanced per-view trackformats.
  AddString(0, "musicfiles.nowplayingtrackformat", 13307, "", EDIT_CONTROL_INPUT, false, 16016);
  AddString(0, "musicfiles.nowplayingtrackformatright", 13387, "", EDIT_CONTROL_INPUT, false, 16016);
  AddString(0, "musicfiles.librarytrackformat", 13307, "", EDIT_CONTROL_INPUT, false, 16016);
  AddString(0, "musicfiles.librarytrackformatright", 13387, "", EDIT_CONTROL_INPUT, false, 16016);
  AddSeparator(0, "musicfiles.sep1");
  AddBool(0, "musicfiles.usecddb", 227, true);
  AddBool(0, "musicfiles.findremotethumbs", 14059, true);

  AddCategory(0, "scrobbler", 15221);
  AddBool(0, "scrobbler.lastfmsubmit", 15201, false);
  AddBool(0, "scrobbler.lastfmsubmitradio", 15250, false);
  AddString(0,"scrobbler.lastfmusername", 15202, "", EDIT_CONTROL_INPUT, false, 15202);
  AddString(0,"scrobbler.lastfmpassword", 15203, "", EDIT_CONTROL_HIDDEN_INPUT, false, 15203);
  AddSeparator(0, "scrobbler.sep1");
  AddBool(0, "scrobbler.librefmsubmit", 15217, false);
  AddString(0, "scrobbler.librefmusername", 15218, "", EDIT_CONTROL_INPUT, false, 15218);
  AddString(0, "scrobbler.librefmpassword", 15219, "", EDIT_CONTROL_HIDDEN_INPUT, false, 15219);

  AddCategory(0, "cddaripper", 620);
  AddString(0, "cddaripper.path", 20000, "select writable folder", BUTTON_CONTROL_PATH_INPUT, false, 657);
  AddString(0, "cddaripper.trackformat", 13307, "[%N. ]%T - %A", EDIT_CONTROL_INPUT, false, 16016);
  AddInt(0, "cddaripper.encoder", 621, CDDARIP_ENCODER_LAME, CDDARIP_ENCODER_LAME, 1, CDDARIP_ENCODER_WAV, SPIN_CONTROL_TEXT);
  AddInt(0, "cddaripper.quality", 622, CDDARIP_QUALITY_CBR, CDDARIP_QUALITY_CBR, 1, CDDARIP_QUALITY_EXTREME, SPIN_CONTROL_TEXT);
  AddInt(0, "cddaripper.bitrate", 623, 192, 128, 32, 320, SPIN_CONTROL_INT_PLUS, MASK_KBPS);

#ifdef HAS_KARAOKE
  AddCategory(3, "karaoke", 13327);
  AddBool(0, "karaoke.enabled", 13323, true);
  // auto-popup the song selector dialog when the karaoke song was just finished and playlist is empty.
  AddBool(0, "karaoke.autopopupselector", 22037, false);
  AddSeparator(0, "karaoke.sep1");
  AddString(0, "karaoke.font", 22030, "boxee.ttf", SPIN_CONTROL_TEXT);
  AddInt(0, "karaoke.fontheight", 22031, 36, 16, 2, 74, SPIN_CONTROL_TEXT); // use text as there is a disk based lookup needed
  AddInt(0, "karaoke.fontcolors", 22032, KARAOKE_COLOR_START, KARAOKE_COLOR_START, 1, KARAOKE_COLOR_END, SPIN_CONTROL_TEXT);
  AddString(0, "karaoke.charset", 22033, "DEFAULT", SPIN_CONTROL_TEXT);
  AddSeparator(0,"karaoke.sep2");
  AddString(0, "karaoke.export", 22038, "", BUTTON_CONTROL_STANDARD);
  AddString(0, "karaoke.importcsv", 22036, "", BUTTON_CONTROL_STANDARD);
#endif

  // System settings
  AddGroup(4, 13000);

  AddCategory(4, "version", 51950);
  AddString(0,   "version.version", 51950, "", BUTTON_CONTROL_STANDARD);
  AddString(0,   "version.builddate", 51949, "", BUTTON_CONTROL_STANDARD);
  AddString(0,   "version.ipaddress", 150, "", BUTTON_CONTROL_STANDARD);
  AddString(0,   "version.totalmemory", 51952, "", BUTTON_CONTROL_STANDARD);
  AddString(0,   "version.screenresolution", 13287, " ", BUTTON_CONTROL_STANDARD);
  AddString(0,   "version.macaddress", 149, " ", BUTTON_CONTROL_STANDARD);

  // advanced only configuration
  AddBool(1, "system.debuglogging", 20191, false);
  AddSeparator(2, "system.sep1");
  AddInt(3, "system.shutdowntime", 357, 0, 0, 5, 120, SPIN_CONTROL_INT_PLUS, MASK_MINS, TEXT_OFF);
  // In standalone mode we default to another.
  if (g_application.IsStandAlone())
    AddInt(4, "system.shutdownstate", 13008, 0, 1, 1, 5, SPIN_CONTROL_TEXT);
  else
    AddInt(4, "system.shutdownstate", 13008, POWERSTATE_QUIT, 0, 1, 5, SPIN_CONTROL_TEXT);
#if defined(_LINUX) && !defined(__APPLE__)
  AddInt(5, "system.powerbuttonaction", 13015, POWERSTATE_NONE, 0, 1, 5, SPIN_CONTROL_TEXT);
#endif

#ifdef HAS_LCD
  AddCategory(4, "lcd", 448);
#ifdef _LINUX
  AddInt(2, "lcd.type", 4501, LCD_TYPE_NONE, LCD_TYPE_NONE, 1, LCD_TYPE_LCDPROC, SPIN_CONTROL_TEXT);
#endif
#ifndef _LINUX // xbmc's lcdproc can't control backlight/contrast yet ..
  AddInt(4, "lcd.backlight", 463, 80, 0, 5, 100, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
  AddInt(5, "lcd.contrast", 465, 100, 0, 5, 100, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
  AddSeparator(6, "lcd.sep1");
#endif
  AddInt(7, "lcd.disableonplayback", 20310, LED_PLAYBACK_OFF, LED_PLAYBACK_OFF, 1, LED_PLAYBACK_VIDEO_MUSIC, SPIN_CONTROL_TEXT);
  AddBool(8, "lcd.enableonpaused", 20312, true);
#endif

#ifdef __APPLE__
  if (CSysInfo::IsAppleTV())
  {
    AddCategory(0, "appleremote", 13600);
    AddInt(0, "appleremote.mode", 13601, APPLE_REMOTE_STANDARD, APPLE_REMOTE_DISABLED, 1, APPLE_REMOTE_UNIVERSAL, SPIN_CONTROL_TEXT);
    AddBool(0, "appleremote.alwayson", 13602, false);
    AddInt(0, "appleremote.sequencetime", 13603, 500, 50, 50, 1000, SPIN_CONTROL_INT_PLUS, MASK_MS, TEXT_OFF);    
  }
  else
  {
    AddCategory(4, "appleremote", 13600);
  AddInt(1, "appleremote.mode", 13601, APPLE_REMOTE_STANDARD, APPLE_REMOTE_DISABLED, 1, APPLE_REMOTE_MULTIREMOTE, SPIN_CONTROL_TEXT);
    AddBool(2, "appleremote.alwayson", 13602, false);
    AddInt(3, "appleremote.sequencetime", 13603, 500, 50, 50, 1000, SPIN_CONTROL_INT_PLUS, MASK_MS, TEXT_OFF);    
  }
#endif

  AddCategory(4, "boxee", 51500);

#ifdef __APPLE__
  if (CSysInfo::IsAppleTV())
  {
    AddBool(0, "boxee.runatlogin",51502,false);
  }
  else
  {
    AddBool(1, "boxee.runatlogin",51502,false);    
  }
#endif

#ifdef _WIN32
  AddBool(1, "boxee.runatlogin",51502,false);
#endif

#if defined _WIN32
  AddBool(2, "boxee.irssremote", 51523, false);
#endif

  AddBool(3, "boxee.showpostplay", 51524, true);

  AddCategory(4, "autorun", 447);
  if (CSysInfo::IsAppleTV())
  {
    AddBool(0, "autorun.dvd", 240, true);
    AddBool(0, "autorun.vcd", 241, true);
    AddBool(0, "autorun.cdda", 242, true);
  }
  else
  {
    AddBool(1, "autorun.dvd", 240, true);
    AddBool(2, "autorun.vcd", 241, true);
    AddBool(3, "autorun.cdda", 242, true);    
  }
  
  AddCategory(4, "cache", 439);
  AddInt(0, "cache.harddisk", 14025, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(0, "cache.sep1");
  AddInt(0, "cachevideo.dvdrom", 14026, 2048, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(0, "cachevideo.lan", 14027, 2048, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(0, "cachevideo.internet", 14028, 4096, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(0, "cache.sep2");
  AddInt(0, "cacheaudio.dvdrom", 14030, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(0, "cacheaudio.lan", 14031, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(0, "cacheaudio.internet", 14032, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(0, "cache.sep3");
  AddInt(0, "cachedvd.dvdrom", 14034, 2048, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(0, "cachedvd.lan", 14035, 2048, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(0, "cache.sep4");
  AddInt(0, "cacheunknown.internet", 14060, 4096, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);

  //AddCategory(4, "cache", 439);
  AddInt(0, "cache.harddisk", 14025, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(0, "cache.sep1");
  AddInt(0, "cachevideo.dvdrom", 14026, 2048, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(0, "cachevideo.lan", 14027, 2048, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(0, "cachevideo.internet", 14028, 4096, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(0, "cache.sep2");
  AddInt(0, "cacheaudio.dvdrom", 14030, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(0, "cacheaudio.lan", 14031, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(0, "cacheaudio.internet", 14032, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(0, "cache.sep3");
  AddInt(0, "cachedvd.dvdrom", 14034, 2048, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(0, "cachedvd.lan", 14035, 2048, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(0, "cache.sep4");
  AddInt(0, "cacheunknown.internet", 14060, 4096, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);

  AddCategory(4, "audiooutput", 772);
#if defined(_LINUX) && !defined(__APPLE__)
  AddInt(0, "audiooutput.library", 53266, AUDIO_LIBRARY_ALSA, AUDIO_LIBRARY_ALSA, 1, AUDIO_LIBRARY_PULSEAUDIO, SPIN_CONTROL_TEXT);
#endif
  AddInt(3, "audiooutput.mode", 337, AUDIO_ANALOG, AUDIO_ANALOG, 1, AUDIO_DIGITAL, SPIN_CONTROL_TEXT);  
  AddBool(4, "audiooutput.ac3passthrough", 364, true);
  AddBool(5, "audiooutput.dtspassthrough", 254, true);
#if defined(_WIN32)
  //AddBool(6, "audiooutput.eac3passthrough", 54120, false); // don't show as this doesn't work yet.
  AddBool(7, "audiooutput.truehdpassthrough", 54121, false);
  AddBool(8, "audiooutput.dtshdpassthrough", 54122, false);
#endif
  
#ifdef __APPLE__
  AddString(6, "audiooutput.audiodevice", 545, "Default", SPIN_CONTROL_TEXT);
  //AddString(7, "audiooutput.passthroughdevice", 546, "S/PDIF", BUTTON_CONTROL_INPUT);
  AddBool(7, "audiooutput.downmixmultichannel", 548, true);
#elif defined(_LINUX)
  AddSeparator(6, "audiooutput.sep1");
  AddString(7, "audiooutput.audiodevice", 545, "default", SPIN_CONTROL_TEXT);
  AddString(8, "audiooutput.customdevice", 1300, "", EDIT_CONTROL_INPUT);
  AddSeparator(9, "audiooutput.sep2");
  AddString(10, "audiooutput.passthroughdevice", 546, "iec958", SPIN_CONTROL_TEXT);
  AddString(11, "audiooutput.custompassthrough", 1301, "", EDIT_CONTROL_INPUT);
  AddSeparator(12, "audiooutput.sep3");
  AddBool(13, "audiooutput.downmixmultichannel", 548, true);
#elif defined(_WIN32)
  AddString(9, "audiooutput.audiodevice", 545, "Default", SPIN_CONTROL_TEXT);
  AddBool(10, "audiooutput.downmixmultichannel", 548, true);
  AddBool(11,"audiooutput.controlmastervolume", 549, true);
#endif

  AddCategory(4, "debug", 54080);
  AddInt(1,  "debug.loglevel", 54081, LOGINFO, LOGDEBUG, 1, LOGNONE, SPIN_CONTROL_TEXT);  
  AddBool(2, "debug.showinfoline",54082,false);
    
  AddCategory(4, "masterlock", 12360);
  AddString(1, "masterlock.lockcode"       , 20100, "-", BUTTON_CONTROL_STANDARD);
  AddSeparator(2, "masterlock.sep1");
  AddBool(4, "masterlock.startuplock"      , 20076,false);
  AddBool(5, "masterlock.enableshutdown"   , 12362,false);
  AddBool(6, "masterlock.automastermode"   , 20101,false);
  AddSeparator(7,"masterlock.sep2" );
  AddBool(8, "masterlock.loginlock",20116,true);
  // hidden masterlock settings
  AddInt(0,"masterlock.maxretries", 12364, 3, 3, 1, 100, SPIN_CONTROL_TEXT);

  AddCategory(4, "advanced", 16005);

  // video settings
  AddGroup(5, 3);
  AddCategory(5, "myvideos", 16000);
  AddBool(0, "myvideos.treatstackasfile", 20051, true);
  AddInt(0, "myvideos.resumeautomatically", 12017, RESUME_ASK, RESUME_NO, 1, RESUME_ASK, SPIN_CONTROL_TEXT);
  AddBool(1,"myvideos.pausewhenexit",12026, false);
  AddBool(2, "myvideos.autothumb",12024, true);
  AddInt(3, "myvideos.videosize", 12025, 50, 0, 10, 700, SPIN_CONTROL_INT);
  AddBool(0, "myvideos.cleanstrings", 20418, false);
  AddBool(0, "myvideos.extractflags",20433,false);

  AddCategory(0, "videolibrary", 14022);

  AddBool(0, "videolibrary.hideplots", 20369, false);
  AddInt(0, "videolibrary.flattentvshows", 20412, 1, 0, 1, 2, SPIN_CONTROL_TEXT);
  AddSeparator(0, "videolibrary.sep1");
  AddBool(0, "videolibrary.updateonstartup", 22000, false);
  AddBool(0, "videolibrary.backgroundupdate", 22001, false);
  AddSeparator(0, "videolibrary.sep2");
  AddString(0, "videolibrary.cleanup", 334, "", BUTTON_CONTROL_STANDARD);
  AddString(0, "videolibrary.export", 647, "", BUTTON_CONTROL_STANDARD);
  AddString(0, "videolibrary.import", 648, "", BUTTON_CONTROL_STANDARD);
  
#ifndef HAS_EMBEDDED
  AddCategory(5, "videoplayer", 16005);
#endif
  AddString(0, "videoplayer.calibrate", 214, "", BUTTON_CONTROL_STANDARD);
  AddString(0, "videoplayer.jumptoaudiohardware", 16001, "", BUTTON_CONTROL_STANDARD);
  AddSeparator(3, "videoplayer.sep1");
#if defined(HAVE_LIBVDPAU) || defined(HAS_DX)
  AddBool(2, "videoplayer.hwaccel", 53269, true);
#else
  AddBool(2, "videoplayer.hwaccel", 53269, false);
#endif
#ifdef HAVE_LIBVDPAU  
  AddInt(1, "videoplayer.rendermethod", 13415, RENDER_METHOD_AUTO, RENDER_METHOD_AUTO, 1, RENDER_METHOD_VDPAU, SPIN_CONTROL_TEXT);
#elif defined(__APPLE__)
  bool bATV = g_sysinfo.IsAppleTV();
  AddInt(1, "videoplayer.rendermethod", 13415, bATV?RENDER_METHOD_ARB:RENDER_METHOD_AUTO, RENDER_METHOD_AUTO, 1, RENDER_METHOD_SOFTWARE, SPIN_CONTROL_TEXT);
#elif defined(HAS_DX) || defined(HAS_EMBEDDED)
  AddInt(0, "videoplayer.rendermethod", 13415, RENDER_METHOD_AUTO, RENDER_METHOD_AUTO, 1, RENDER_METHOD_AUTO, SPIN_CONTROL_TEXT);
#else
  AddInt(1, "videoplayer.rendermethod", 13415, RENDER_METHOD_AUTO, RENDER_METHOD_AUTO, 1, RENDER_METHOD_SOFTWARE, SPIN_CONTROL_TEXT);
#endif
  AddInt(0, "videoplayer.displayresolution", 169, (int)RES_DESKTOP, (int)RES_WINDOW, 1, (int)RES_CUSTOM+MAX_RESOLUTIONS, SPIN_CONTROL_TEXT);
  AddBool(0, "videoplayer.adjustrefreshrate", 170, false);
#ifdef HAVE_LIBVDPAU
  AddBool(0, "videoplayer.strictbinding", 13120, false);
  AddBool(0, "videoplayer.vdpau_allow_xrandr", 13122, false);
#endif
#ifdef HAS_GL
  AddSeparator(0, "videoplayer.sep1.5");
  AddInt(0, "videoplayer.highqualityupscaling", 13112, SOFTWARE_UPSCALING_DISABLED, SOFTWARE_UPSCALING_DISABLED, 1, SOFTWARE_UPSCALING_ALWAYS, SPIN_CONTROL_TEXT);
  AddInt(0, "videoplayer.upscalingalgorithm", 13116, VS_SCALINGMETHOD_BICUBIC_SOFTWARE, VS_SCALINGMETHOD_BICUBIC_SOFTWARE, 1, VS_SCALINGMETHOD_SINC_SOFTWARE, SPIN_CONTROL_TEXT);
#ifdef HAVE_LIBVDPAU
  AddInt(0, "videoplayer.vdpauUpscalingLevel", 13121, 0, 0, 1, 9, SPIN_CONTROL_INT_PLUS, -1, TEXT_OFF);
  AddBool(11, "videoplayer.vdpaustudiolevel", 13122, true);
#endif
#endif

#ifndef HAS_EMBEDDED
  AddInt(7, "videoplayer.aspecterror", 22021, 3, 0, 1, 20, SPIN_CONTROL_INT_PLUS, MASK_PERCENT, TEXT_OFF);
#else
  AddInt(0, "videoplayer.aspecterror", 22021, 3, 0, 1, 20, SPIN_CONTROL_INT_PLUS, MASK_PERCENT, TEXT_OFF);
#endif
  AddSeparator(0, "videoplayer.sep2");
  AddString(0, "videoplayer.jumptocache", 439, "", BUTTON_CONTROL_STANDARD);
  AddSeparator(0, "videoplayer.sep3");
  AddInt(0, "videoplayer.dvdplayerregion", 21372, 0, 0, 1, 8, SPIN_CONTROL_INT_PLUS, -1, TEXT_OFF);
  AddBool(0, "videoplayer.dvdautomenu", 21882, false);
  AddBool(0, "videoplayer.editdecision", 22003, false);

  AddSeparator(18, "videoplayer.sep4");
#ifndef HAS_EMBEDDED
  AddBool(19, "videoplayer.usedisplayasclock", 13510, false);
  AddInt(20, "videoplayer.synctype", 13500, SYNC_DISCON, SYNC_DISCON, 1, SYNC_RESAMPLE, SPIN_CONTROL_TEXT);
  AddFloat(21, "videoplayer.maxspeedadjust", 13504, 5.0f, 0.0f, 0.1f, 10.0f);
  AddInt(22, "videoplayer.resamplequality", 13505, RESAMPLE_MID, RESAMPLE_LOW, 1, RESAMPLE_REALLYHIGH, SPIN_CONTROL_TEXT);
#else
  AddBool(0, "videoplayer.usedisplayasclock", 13510, false);
  AddInt(0, "videoplayer.synctype", 13500, SYNC_DISCON, SYNC_DISCON, 1, SYNC_RESAMPLE, SPIN_CONTROL_TEXT);
  AddFloat(0, "videoplayer.maxspeedadjust", 13504, 5.0f, 0.0f, 0.1f, 10.0f);
  AddInt(0, "videoplayer.resamplequality", 13505, RESAMPLE_MID, RESAMPLE_LOW, 1, RESAMPLE_REALLYHIGH, SPIN_CONTROL_TEXT);
#endif

  AddSeparator(23, "videoplayer.sep5");
  AddBool(0, "videoplayer.teletextenabled", 23050, true);

  AddCategory(5, "subtitles", 287);
  AddString(1, "subtitles.font", 288, "boxee.ttf", SPIN_CONTROL_TEXT);
  AddInt(2, "subtitles.height", 289, 30, 16, 2, 74, SPIN_CONTROL_TEXT); // use text as there is a disk based lookup needed
  AddInt(3, "subtitles.style", 736, FONT_STYLE_BOLD, FONT_STYLE_NORMAL, 1, FONT_STYLE_BOLD_ITALICS, SPIN_CONTROL_TEXT);
  AddInt(4, "subtitles.color", 737, 1 /* default white */, SUBTITLE_COLOR_START, 1, SUBTITLE_COLOR_END, SPIN_CONTROL_TEXT);
  AddString(5, "subtitles.charset", 735, "DEFAULT", SPIN_CONTROL_TEXT);
  AddSeparator(0, "subtitles.sep1");
  AddBool(0, "subtitles.searchrars", 13249, false);
  AddSeparator(0,"subtitles.sep2");
  AddString(0, "subtitles.custompath", 21366, "special://masterprofile/subtitles", BUTTON_CONTROL_PATH_INPUT, false, 657);

  // Don't add the category - makes them hidden in the GUI
  //AddCategory(5, "postprocessing", 14041);
  AddBool(0, "postprocessing.enable", 286, false);
  AddBool(0, "postprocessing.auto", 307, true); // only has effect if PostProcessing.Enable is on.
  AddBool(0, "postprocessing.verticaldeblocking", 308, false);
  AddInt(0, "postprocessing.verticaldeblocklevel", 308, 0, 0, 1, 100, SPIN_CONTROL_INT);
  AddBool(0, "postprocessing.horizontaldeblocking", 309, false);
  AddInt(0, "postprocessing.horizontaldeblocklevel", 309, 0, 0, 1, 100, SPIN_CONTROL_INT);
  AddBool(0, "postprocessing.autobrightnesscontrastlevels", 310, false);
  AddBool(0, "postprocessing.dering", 311, false);

  AddCategory(5, "scrapers", 21412);
  AddString(0, "scrapers.moviedefault", 21413, "tmdb.xml", SPIN_CONTROL_TEXT);
  AddString(0, "scrapers.tvshowdefault", 21414, "tvdb.xml", SPIN_CONTROL_TEXT);
  AddString(0, "scrapers.musicvideodefault", 21415, "mtv.xml", SPIN_CONTROL_TEXT);
  AddSeparator(0,"scrapers.sep2");
  AddBool(0, "scrapers.langfallback", 21416, true);

  AddCategory(5, "advanced", 16005);

  // network settings
  AddGroup(6, 705);
  AddCategory(6, "network", 705);

  if (g_application.IsStandAlone())
  {
#ifndef __APPLE__
    AddString(1, "network.interface",775,"", SPIN_CONTROL_TEXT);
    AddInt(2, "network.assignment", 715, NETWORK_DHCP, NETWORK_DHCP, 1, NETWORK_DISABLED, SPIN_CONTROL_TEXT);
    AddString(3, "network.ipaddress", 719, "0.0.0.0", EDIT_CONTROL_IP_INPUT);
    AddString(4, "network.subnet", 720, "255.255.255.0", EDIT_CONTROL_IP_INPUT);
    AddString(5, "network.gateway", 721, "0.0.0.0", EDIT_CONTROL_IP_INPUT);
    AddString(6, "network.dns", 722, "0.0.0.0", EDIT_CONTROL_IP_INPUT);
    AddString(7, "network.dnssuffix", 22002, "", EDIT_CONTROL_INPUT, true);
    AddString(8, "network.essid", 776, "0.0.0.0", BUTTON_CONTROL_STANDARD);
    AddInt(9, "network.enc", 778, ENC_NONE, ENC_NONE, 1, ENC_WPA2, SPIN_CONTROL_TEXT);
    AddString(10, "network.key", 777, "0.0.0.0", EDIT_CONTROL_INPUT);
#ifndef _WIN32
    AddString(11, "network.save", 779, "", BUTTON_CONTROL_STANDARD);
#endif
    AddSeparator(12, "network.sep1");
#endif
  }
  AddBool(13, "network.usehttpproxy", 708, false);
  AddString(14, "network.httpproxyserver", 706, "", EDIT_CONTROL_INPUT);
  AddString(15, "network.httpproxyport", 707, "8080", EDIT_CONTROL_NUMBER_INPUT, false, 707);
  AddString(16, "network.httpproxyusername", 709, "", EDIT_CONTROL_INPUT);
  AddString(17, "network.httpproxypassword", 710, "", EDIT_CONTROL_HIDDEN_INPUT,true,733);

  AddSeparator(18, "network.sep2");
  AddBool(0, "network.enableinternet", 14054, true);

#if defined(HAS_FTP_SERVER) || defined (HAS_WEB_SERVER)
  AddCategory(6, "servers", 14036);
#endif

#ifdef HAS_FTP_SERVER
  AddBool(1,  "servers.ftpserver",        167, true);
  AddString(3,"servers.ftpserverpassword",1246, "xbox", EDIT_CONTROL_HIDDEN_INPUT, true, 1246);
  AddBool(4,  "servers.ftpautofatx",      771, true);
  AddString(2,"servers.ftpserveruser",    1245, "xbox", SPIN_CONTROL_TEXT);
#endif
#if defined(HAS_FTP_SERVER) && defined(HAS_WEB_SERVER)
  AddSeparator(5, "servers.sep1");
#endif
#ifdef HAS_WEB_SERVER
  AddBool(6,  "servers.webserver",        263, true);
  AddString(7,"servers.webserverport",    730, "8800", EDIT_CONTROL_NUMBER_INPUT, false, 730);
  AddString(0,"servers.webserverusername",1048, "boxee", EDIT_CONTROL_INPUT);
  AddString(9,"servers.webserverpassword",733, "", EDIT_CONTROL_HIDDEN_INPUT, true, 733);
#endif

  // zeroconf publishing
#ifdef HAS_ZEROCONF
  AddBool(0, "servers.zeroconf", 1260, true);
#endif

  AddCategory(6, "smb", 1200);
  AddBool  (0, "smb.client",      1201, true);
  AddString(1, "smb.username",    1203,   "", EDIT_CONTROL_INPUT, true, 1203);
  AddString(2, "smb.password",    1204,   "", EDIT_CONTROL_HIDDEN_INPUT, true, 1204);
#ifndef _WIN32
  AddString(3, "smb.winsserver",  1207,   "",  EDIT_CONTROL_IP_INPUT);
  AddString(4, "smb.workgroup",   1202,   "WORKGROUP", EDIT_CONTROL_INPUT, false, 1202);
#endif
#ifdef _LINUX
  AddBool  (0, "smb.mountshares", 1208,   false);
#endif
  AddBool  (0, "smb.singleconnection", 1209,   false);

  AddCategory(0, "upnp", 20110);
  AddBool(0,     "upnp.client", 20111, true);
  AddBool(0,     "upnp.renderer", 21881, false);
  AddSeparator(0,"upnp.sep1");
  AddBool(0,     "upnp.server", 21360, false);  
  AddString(0,   "upnp.musicshares", 21361, "", BUTTON_CONTROL_STANDARD);
  AddString(0,   "upnp.videoshares", 21362, "", BUTTON_CONTROL_STANDARD);
  AddString(0,   "upnp.pictureshares", 21363, "", BUTTON_CONTROL_STANDARD);

  // remote events settings
#ifdef HAS_EVENT_SERVER
  AddCategory(0, "remoteevents", 790);
  AddBool(0,  "remoteevents.enabled",         791, true);
  if (CSysInfo::IsAppleTV())
    AddString(0,"remoteevents.port",            792, "9777", EDIT_CONTROL_NUMBER_INPUT, false, 792);
  else
    AddString(0,"remoteevents.port",            792, "9770", EDIT_CONTROL_NUMBER_INPUT, false, 792);
  AddInt(0,   "remoteevents.portrange",       793, 10, 1, 1, 100, SPIN_CONTROL_INT);
  AddInt(0,   "remoteevents.maxclients",      797, 20, 1, 1, 100, SPIN_CONTROL_INT);
  AddSeparator(0,"remoteevents.sep1");
  AddBool(0,  "remoteevents.allinterfaces",   794, false);
  AddSeparator(0,"remoteevents.sep2");
  AddInt(0,   "remoteevents.initialdelay",    795, 750, 5, 5, 10000, SPIN_CONTROL_INT);
  AddInt(0,   "remoteevents.continuousdelay", 796, 25, 5, 5, 10000, SPIN_CONTROL_INT);
#endif

  // appearance settings
  AddGroup(7, 480);
  AddCategory(7,"lookandfeel", 14037);
  AddString(0, "lookandfeel.skin",166,DEFAULT_SKIN, SPIN_CONTROL_TEXT);
  AddString(0, "lookandfeel.skintheme",15111,"SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddString(0, "lookandfeel.skincolors",14078, "SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddString(0, "lookandfeel.font",13303,"Default", SPIN_CONTROL_TEXT);
  AddInt(0, "lookandfeel.skinzoom",20109, 0, -20, 2, 20, SPIN_CONTROL_INT, MASK_PERCENT);
  AddInt(0, "lookandfeel.startupwindow",512,1, WINDOW_HOME, 1, WINDOW_PYTHON_END, SPIN_CONTROL_TEXT);
  AddSeparator(0, "lookandfeel.sep1");
  AddString(8, "lookandfeel.soundskin",15108,"SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddSeparator(10, "lookandfeel.sep2");
  AddBool(0, "lookandfeel.enablerssfeeds",13305,  true);
  AddString(0, "lookandfeel.rssedit", 21435, "", BUTTON_CONTROL_STANDARD);
  AddSeparator(0, "lookandfeel.sep3");
  AddBool(12, "lookandfeel.enablemouse", 21369, true);

  AddCategory(7, "locale", 20026);
  AddString(1, "locale.country", 20026, "USA", SPIN_CONTROL_TEXT);
  AddString(2, "locale.language",248,"english", SPIN_CONTROL_TEXT);
  AddString(3, "locale.charset",802,"DEFAULT", SPIN_CONTROL_TEXT); // charset is set by the language file
  AddSeparator(4, "locale.sep1");
  AddString(5, "locale.keyboard1",54071,"n/a", SPIN_CONTROL_TEXT);
  AddString(6, "locale.keyboard2",54072,"n/a", SPIN_CONTROL_TEXT);
  AddString(7, "locale.keyboard3",54073,"n/a", SPIN_CONTROL_TEXT);
#ifndef _LINUX
  AddString(8, "locale.time", 14065, "", BUTTON_CONTROL_MISC_INPUT);
  AddString(9, "locale.date", 14064, "", BUTTON_CONTROL_MISC_INPUT);
#else
  // Disable, because user can not change them.
  AddString(0, "locale.time", 14065, "", BUTTON_CONTROL_MISC_INPUT);
  AddString(0, "locale.date", 14064, "", BUTTON_CONTROL_MISC_INPUT);
  AddString(0, "locale.timezone", 14081, g_timezone.GetOSConfiguredTimezone(), SPIN_CONTROL_TEXT);
  AddString(0, "locale.timezonecountry", 14080, g_timezone.GetCountryByTimezone(g_timezone.GetOSConfiguredTimezone()), SPIN_CONTROL_TEXT);
#endif
#ifdef HAS_TIME_SERVER
  AddSeparator(14, "locale.sep2");
  AddBool(15, "locale.timeserver", 168, false);
  AddString(16, "locale.timeserveraddress", 731, "pool.ntp.org", EDIT_CONTROL_INPUT);
#endif
  AddString(17, "locale.timeformat",14096,"12h", SPIN_CONTROL_TEXT);
  AddString(18, "locale.tempscale",14097,"F", SPIN_CONTROL_TEXT);

  AddCategory(7, "videoscreen", 131);
  if (CSysInfo::IsAppleTV())
  {
    AddInt(0, "videoscreen.resolution",169,(int)RES_DESKTOP, (int)RES_WINDOW, 1, (int)RES_CUSTOM+MAX_RESOLUTIONS, SPIN_CONTROL_TEXT);
    AddString(0, "videoscreen.testresolution",13109,"", BUTTON_CONTROL_STANDARD);    
  }
  else
  {
    AddInt(1, "videoscreen.resolution",169,(int)RES_DESKTOP, (int)RES_WINDOW, 1, (int)RES_CUSTOM+MAX_RESOLUTIONS, SPIN_CONTROL_TEXT);
    AddString(2, "videoscreen.testresolution",13109,"", BUTTON_CONTROL_STANDARD);    
  }

#if defined (__APPLE__) || defined(_WIN32) 
  if (CSysInfo::IsAppleTV())
  {
    AddInt(0, "videoscreen.displayblanking", 13130, BLANKING_DISABLED, BLANKING_DISABLED, 1, BLANKING_ALL_DISPLAYS, SPIN_CONTROL_TEXT);
  }
  else
  {
    AddInt(5, "videoscreen.displayblanking", 13130, BLANKING_DISABLED, BLANKING_DISABLED, 1, BLANKING_ALL_DISPLAYS, SPIN_CONTROL_TEXT);    
  }
#endif

  AddString(6, "videoscreen.guicalibration",214,"", BUTTON_CONTROL_STANDARD);
#ifdef HAS_GL
  AddString(7, "videoscreen.testpattern",226,"", BUTTON_CONTROL_STANDARD);
#else
  AddString(0, "videoscreen.testpattern",226,"", BUTTON_CONTROL_STANDARD);
#endif
#if defined(_WIN32) || defined (__APPLE__)
  // We prefer a fake fullscreen mode (window covering the screen rather than dedicated fullscreen)
  // as it works nicer with switching to other applications. However on some systems vsync is broken
  // when we do this (eg non-Aero on ATI in particular) and on others (AppleTV) we can't get XBMC to
  // the front
  bool fakeFullScreen = true;
  bool showSetting = false;  // never use real full screen for now. black screen on movie playback.
  //if (g_sysinfo.IsAeroDisabled())
  //  fakeFullScreen = false;
#if defined (__APPLE__)
  if (g_sysinfo.IsAppleTV())
  {
    fakeFullScreen = false;
  }
  showSetting = false;
#endif
  AddBool(showSetting ? 2 : 0, "videoscreen.fakefullscreen", 14083, fakeFullScreen);
  AddSeparator(4, "videoscreen.sep1");
#endif
  AddInt(8, "videoscreen.vsync", 13105, DEFAULT_VSYNC, VSYNC_DISABLED, 1, VSYNC_ALWAYS, SPIN_CONTROL_TEXT);
  AddCategory(7, "filelists", 14018);
  AddBool(0, "filelists.hideparentdiritems", 13306, true);
  AddBool(2, "filelists.hideextensions", 497, false);
  AddBool(0, "filelists.ignorethewhensorting", 13399, false);
  AddBool(0, "filelists.unrollarchives",516, false);
  AddSeparator(0, "filelists.sep1");
  AddBool(0, "filelists.allowfiledeletion", 14071, false);
  AddBool(0, "filelists.disableaddsourcebuttons", 21382,  true);
  AddSeparator(9, "filelists.sep2");
  AddBool(10, "filelists.showhidden", 21330, false);
  AddSeparator(11, "filelists.sep3");
  AddBool(12, "filelists.filtergeoip", 53293, true);
  AddSeparator(13, "filelists.sep4");
  AddBool(14, "filelists.filteradult", 53290, true);
  AddString(15, "filelists.setadultcode", 53292, "", BUTTON_CONTROL_STANDARD);      

  AddCategory(7, "screensaver", 360);
  AddString(1, "screensaver.mode", 356, "SlideShow", SPIN_CONTROL_TEXT);
  AddString(2, "screensaver.preview", 1000, "", BUTTON_CONTROL_STANDARD);
  AddInt(3, "screensaver.time", 355, 5, 1, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_MINS);
  AddBool(4, "screensaver.usemusicvisinstead", 13392, true);
  AddBool(0, "screensaver.usedimonpause", 22014, true);
  AddBool(0, "screensaver.uselock",20140,false);
  // Note: Application.cpp might hide powersaving settings if not supported.
  AddSeparator(6, "screensaver.sep_powersaving");
  AddInt(7, "screensaver.powersavingtime", 1450, 0, 0, 5, 4 * 60, SPIN_CONTROL_INT_PLUS, MASK_MINS, TEXT_OFF);
  AddSeparator(8, "screensaver.sep1");
  AddInt(9, "screensaver.dimlevel", 362, 20, 0, 10, 80, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
#ifndef HAS_EMBEDDED
  AddPath(10, "screensaver.slideshowpath", 774, "special://xbmc/media/boxee_screen_saver", BUTTON_CONTROL_PATH_INPUT, false, 657);
#else
  AddPath(0, "screensaver.slideshowpath", 774, "special://xbmc/media/boxee_screen_saver", BUTTON_CONTROL_PATH_INPUT, false, 657);
#endif

  AddCategory(7, "window", 0);
  AddInt(0, "window.width",  0, 854, 10, 1, INT_MAX, SPIN_CONTROL_INT);
  AddInt(0, "window.height", 0, 480, 10, 1, INT_MAX, SPIN_CONTROL_INT);

  AddPath(0,"system.playlistspath",20006,"set default",BUTTON_CONTROL_PATH_INPUT,false);

  AddCategory(7, "background", 53130);
  AddPath(0,   "background.imagefile", 51946, "", BUTTON_CONTROL_PATH_INPUT, false, 657);
  AddPath(0,   "background.imagefolder", 51947, "", BUTTON_CONTROL_PATH_INPUT, false, 657);
  AddString(0,   "background.remark", 53131, "", BUTTON_CONTROL_STANDARD);

  // My Weather settings
  AddGroup(2, 8);
  AddCategory(7, "weather", 8); 
  AddString(1, "weather.areacode1", 14019, "USNY0996 - New York, NY", BUTTON_CONTROL_STANDARD);
  AddString(0, "weather.areacode2", 14020, "UKXX0085 - London, United Kingdom", BUTTON_CONTROL_STANDARD);
  AddString(0, "weather.areacode3", 14021, "CAXX0343 - Ottawa, Canada", BUTTON_CONTROL_STANDARD);
  AddSeparator(0, "weather.sep1");
  AddString(0, "weather.jumptolocale", 20026, "", BUTTON_CONTROL_STANDARD);

  AddCategory(7, "advanced", 16005);

  AddString(0, "background.reset", 51957, "", BUTTON_CONTROL_STANDARD);

  AddString(0, "personal.feeds", 801, "", BUTTON_CONTROL_STANDARD);

  AddPath(0,   "erase.db", 51944, "", BUTTON_CONTROL_STANDARD);
  AddPath(0,   "erase.thumb", 51956, "", BUTTON_CONTROL_STANDARD);


}

CGUISettings::~CGUISettings(void)
{
  Clear();
}

void CGUISettings::AddGroup(int groupID, int labelID)
{
  CSettingsGroup *pGroup = new CSettingsGroup(groupID, labelID);
  if (pGroup)
    settingsGroups.push_back(pGroup);
}

CSettingsCategory* CGUISettings::AddCategory(int groupID, const char *strSetting, int labelID)
{
  for (unsigned int i = 0; i < settingsGroups.size(); i++)
  {
    if (settingsGroups[i]->GetGroupID() == groupID)
      return settingsGroups[i]->AddCategory(CStdString(strSetting).ToLower(), labelID);
  }
  
  return NULL;
}

CSettingsGroup *CGUISettings::GetGroup(int groupID)
{
  for (unsigned int i = 0; i < settingsGroups.size(); i++)
  {
    if (settingsGroups[i]->GetGroupID() == groupID)
      return settingsGroups[i];
  }
  CLog::Log(LOGDEBUG, "Error: Requested setting group (%i) was not found.  "
                      "It must be case-sensitive",
            groupID);
  return NULL;
}

CSettingsGroup *CGUISettings::GetGroupByWindowId(int windowId)
{
  for (unsigned int i = 0; i < settingsGroups.size(); i++)
  {
    if (settingsGroups[i]->GetWindowID() == windowId)
      return settingsGroups[i];
  }
  CLog::Log(LOGDEBUG, "Error: Requested setting group by window (%d) was not found.", windowId);
  return NULL;
}

void CGUISettings::AddSeparator(int iOrder, const char *strSetting)
{
  CSettingSeparator *pSetting = new CSettingSeparator(iOrder, CStdString(strSetting).ToLower());
  if (!pSetting) return;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddBool(int iOrder, const char *strSetting, int iLabel, bool bData, int iControlType)
{
  CSettingBool* pSetting = new CSettingBool(iOrder, CStdString(strSetting).ToLower(), iLabel, bData, iControlType);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}
void CGUISettings::AddBool(int iOrder, const char *strSetting, CStdString labelStr, bool bData, int iControlType)
{
  CSettingBool* pSetting = new CSettingBool(iOrder, CStdString(strSetting).ToLower(), labelStr, bData, iControlType);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}
bool CGUISettings::GetBool(const char *strSetting) const
{
  ASSERT(settingsMap.size());
  constMapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  { // old category
    return ((CSettingBool*)(*it).second)->GetData();
  }
  // Assert here and write debug output
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  return false;
}

void CGUISettings::SetBool(const char *strSetting, bool bSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  { // old category
    ((CSettingBool*)(*it).second)->SetData(bSetting);
    return ;
  }
  // Assert here and write debug output
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::ToggleBool(const char *strSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  { // old category
    ((CSettingBool*)(*it).second)->SetData(!((CSettingBool *)(*it).second)->GetData());
    return ;
  }
  // Assert here and write debug output
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::AddFloat(int iOrder, const char *strSetting, int iLabel, float fData, float fMin, float fStep, float fMax, int iControlType)
{
  CSettingFloat* pSetting = new CSettingFloat(iOrder, CStdString(strSetting).ToLower(), iLabel, fData, fMin, fStep, fMax, iControlType);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

float CGUISettings::GetFloat(const char *strSetting) const
{
  ASSERT(settingsMap.size());
  constMapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    return ((CSettingFloat *)(*it).second)->GetData();
  }
  // Assert here and write debug output
  //ASSERT(false);
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  return 0.0f;
}

void CGUISettings::SetFloat(const char *strSetting, float fSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    ((CSettingFloat *)(*it).second)->SetData(fSetting);
    return ;
  }
  // Assert here and write debug output
  ASSERT(false);
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::LoadMasterLock(TiXmlElement *pRootElement)
{
  std::map<CStdString,CSetting*>::iterator it = settingsMap.find("masterlock.enableshutdown");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
  it = settingsMap.find("masterlock.maxretries");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
  it = settingsMap.find("masterlock.automastermode");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
  it = settingsMap.find("masterlock.startuplock");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
    it = settingsMap.find("autodetect.nickname");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
}


void CGUISettings::AddInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
{
  CSettingInt* pSetting = new CSettingInt(iOrder, CStdString(strSetting).ToLower(), iLabel, iData, iMin, iStep, iMax, iControlType, strFormat);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, int iFormat, int iLabelMin/*=-1*/)
{
  CSettingInt* pSetting = new CSettingInt(iOrder, CStdString(strSetting).ToLower(), iLabel, iData, iMin, iStep, iMax, iControlType, iFormat, iLabelMin);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddHex(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
{
  CSettingHex* pSetting = new CSettingHex(iOrder, CStdString(strSetting).ToLower(), iLabel, iData, iMin, iStep, iMax, iControlType, strFormat);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

int CGUISettings::GetInt(const char *strSetting) const
{
  ASSERT(settingsMap.size());
  constMapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    return ((CSettingInt *)(*it).second)->GetData();
  }
  // Assert here and write debug output
  CLog::Log(LOGERROR,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  //ASSERT(false);
  return 0;
}

void CGUISettings::SetInt(const char *strSetting, int iSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    ((CSettingInt *)(*it).second)->SetData(iSetting);
    if (stricmp(strSetting, "videoscreen.resolution") == 0)
      g_guiSettings.m_LookAndFeelResolution = (RESOLUTION)iSetting;
    return ;
  }
  // Assert here and write debug output
  ASSERT(false);
}

void CGUISettings::AddString(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
{
  CSettingString* pSetting = new CSettingString(iOrder, CStdString(strSetting).ToLower(), iLabel, strData, iControlType, bAllowEmpty, iHeadingString);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}
void CGUISettings::AddString(int iOrder, const char *strSetting,  CStdString strLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
{
  CSettingString* pSetting = new CSettingString(iOrder, CStdString(strSetting).ToLower(), strLabel, strData, iControlType, bAllowEmpty, iHeadingString);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddPath(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
{
  CSettingPath* pSetting = new CSettingPath(iOrder, CStdString(strSetting).ToLower(), iLabel, strData, iControlType, bAllowEmpty, iHeadingString);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

const CStdString &CGUISettings::GetString(const char *strSetting, bool bPrompt) const
{
  constMapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    CSettingString* result = ((CSettingString *)(*it).second);
    if (result->GetData() == "select folder")
    {
      CStdString strData = "";
      if (bPrompt)
      {
        VECSOURCES shares;
        g_mediaManager.GetLocalDrives(shares);
        if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares,g_localizeStrings.Get(result->GetLabel()),strData,false))
        {
          result->SetData(strData);
          g_settings.Save();
        }
        else
          return StringUtils::EmptyString;
      }
      else
        return StringUtils::EmptyString;
    }
    if (result->GetData() == "select writable folder")
    {
      CStdString strData = "";
      if (bPrompt)
      {
        VECSOURCES shares;
        g_mediaManager.GetLocalDrives(shares);
        if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares,g_localizeStrings.Get(result->GetLabel()),strData,true))
        {
          result->SetData(strData);
          g_settings.Save();
        }
        else
          return StringUtils::EmptyString;
      }
      else
        return StringUtils::EmptyString;
    }
    return result->GetData();
  }
  // Assert here and write debug output
  
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  //ASSERT(false);
  // hardcoded return value so that compiler is happy
  return ((CSettingString *)(*settingsMap.begin()).second)->GetData();
}

void CGUISettings::SetString(const char *strSetting, const char *strData)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    ((CSettingString *)(*it).second)->SetData(strData);
    return ;
  }
  // Assert here and write debug output
  CLog::Log(LOGERROR,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  ASSERT(false);
}

void CGUISettings::DeleteSetting(const char *strSetting)
{
  settingsMap.erase(CStdString(strSetting).ToLower());
}

CSetting *CGUISettings::GetSetting(const char *strSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
    return (*it).second;
  else
    return NULL;
}

// get all the settings beginning with the term "strGroup"
void CGUISettings::GetSettingsGroup(const char *strGroup, vecSettings &settings)
{
  vecSettings unorderedSettings;
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
  {
    if ((*it).first.Left(strlen(strGroup)).Equals(strGroup) && (*it).second->GetOrder() > 0 && !(*it).second->IsAdvanced())
      unorderedSettings.push_back((*it).second);
  }
  // now order them...
  sort(unorderedSettings.begin(), unorderedSettings.end(), sortsettings());

  // remove any instances of 2 separators in a row
  bool lastWasSeparator(false);
  for (vecSettings::iterator it = unorderedSettings.begin(); it != unorderedSettings.end(); it++)
  {
    CSetting *setting = *it;
    // only add separators if we don't have 2 in a row
    if (setting->GetType() == SETTINGS_TYPE_SEPARATOR)
    {
      if (!lastWasSeparator)
        settings.push_back(setting);
      lastWasSeparator = true;
    }
    else
    {
      lastWasSeparator = false;
      settings.push_back(setting);
    }
  }
}

void CGUISettings::LoadXML(TiXmlElement *pRootElement, bool hideSettings /* = false */)
{ // load our stuff...
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
  {
    LoadFromXML(pRootElement, it, hideSettings);
  }
  
  // setup logging...
  CLog::m_logLevel    = GetInt("debug.loglevel");
  CLog::m_showLogLine = GetBool("debug.showinfoline");
  
  // Get hardware based stuff...
  CLog::Log(LOGNOTICE, "Getting hardware information now...");
  if (GetInt("audiooutput.mode") == AUDIO_DIGITAL && !g_audioConfig.HasDigitalOutput())
    SetInt("audiooutput.mode", AUDIO_ANALOG);
  // FIXME: Check if the hardware supports it (if possible ;)
  //SetBool("audiooutput.ac3passthrough", g_audioConfig.GetAC3Enabled());
  //SetBool("audiooutput.dtspassthrough", g_audioConfig.GetDTSEnabled());
  CLog::Log(LOGINFO, "Using %s output", GetInt("audiooutput.mode") == AUDIO_ANALOG ? "analog" : "digital");
  CLog::Log(LOGINFO, "AC3 pass through is %s", GetBool("audiooutput.ac3passthrough") ? "enabled" : "disabled");
  CLog::Log(LOGINFO, "DTS pass through is %s", GetBool("audiooutput.dtspassthrough") ? "enabled" : "disabled");
#if defined(_LINUX) && !defined(__APPLE__)
  CLog::Log(LOGINFO, "Audio library is %s", GetInt("audiooutput.library") == AUDIO_LIBRARY_ALSA ? "ALSA" : "PulseAudio");
#endif
  
  g_guiSettings.m_LookAndFeelResolution = (RESOLUTION)GetInt("videoscreen.resolution");
#ifdef __APPLE__
  // trap any previous vsync by driver setting, does not exist on OSX
  if (GetInt("videoscreen.vsync") == VSYNC_DRIVER)
  {
    SetInt("videoscreen.vsync", VSYNC_ALWAYS);
  }
#endif
 // DXMERGE: This might have been useful?
 // g_videoConfig.SetVSyncMode((VSYNC)GetInt("videoscreen.vsync"));
  CLog::Log(LOGNOTICE, "Checking resolution %i", g_guiSettings.m_LookAndFeelResolution);
  if (
    (g_guiSettings.m_LookAndFeelResolution == RES_AUTORES) ||
    (!g_graphicsContext.IsValidResolution(g_guiSettings.m_LookAndFeelResolution))
  )
  {
    RESOLUTION newRes = RES_DESKTOP;
    if (g_guiSettings.m_LookAndFeelResolution == RES_AUTORES)
    {
      //"videoscreen.resolution" will stay at RES_AUTORES, m_LookAndFeelResolution will be the real mode
      CLog::Log(LOGNOTICE, "Setting RES_AUTORESolution mode %i", newRes);
      g_guiSettings.m_LookAndFeelResolution = newRes;
    }
    else
    {
      CLog::Log(LOGNOTICE, "Setting safe mode %i", newRes);
      SetInt("videoscreen.resolution", newRes);
    }
  }
  
  // Move replaygain settings into our struct
  m_replayGain.iPreAmp = GetInt("musicplayer.replaygainpreamp");
  m_replayGain.iNoGainPreAmp = GetInt("musicplayer.replaygainnogainpreamp");
  m_replayGain.iType = GetInt("musicplayer.replaygaintype");
  m_replayGain.bAvoidClipping = GetBool("musicplayer.replaygainavoidclipping");

#if defined(_LINUX) && !defined(__APPLE__)
  CStdString timezone = GetString("locale.timezone");
#if 0 // Boxee: since the user cannot change the timezone, always take it from the OS
  if(timezone == "0" || timezone.IsEmpty())
#endif
  {
    timezone = g_timezone.GetOSConfiguredTimezone();
    SetString("locale.timezone", timezone);
  }
  g_timezone.SetTimezone(timezone);
#endif  
}

void CGUISettings::LoadFromXML(TiXmlElement *pRootElement, mapIter &it, bool advanced /* = false */)
{
  CStdStringArray strSplit;
  StringUtils::SplitString((*it).first, ".", strSplit);
  if (strSplit.size() > 1)
  {
    const TiXmlNode *pChild = pRootElement->FirstChild(strSplit[0].c_str());
    if (pChild)
    {
      const TiXmlElement *pGrandChild = pChild->FirstChildElement(strSplit[1].c_str());
      if (pGrandChild && pGrandChild->FirstChild())
      {
        CStdString strValue = pGrandChild->FirstChild()->Value();
        if (strValue.size() )
        {
          if (strValue != "-")
          { // update our item
            if ((*it).second->GetType() == SETTINGS_TYPE_PATH)
            { // check our path
              int pathVersion = 0;
              pGrandChild->Attribute("pathversion", &pathVersion);
              strValue = CSpecialProtocol::ReplaceOldPath(strValue, pathVersion);
            }
            (*it).second->FromString(strValue);
            if (advanced)
              (*it).second->SetAdvanced();
          }
        }
      }
    }
  }
}

void CGUISettings::SaveXML(TiXmlNode *pRootNode)
{
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
  {
    // don't save advanced settings
    CStdString first = (*it).first;
    if ((*it).second->IsAdvanced())
      continue;

    CStdStringArray strSplit;
    StringUtils::SplitString((*it).first, ".", strSplit);
    if (strSplit.size() > 1)
    {
      TiXmlNode *pChild = pRootNode->FirstChild(strSplit[0].c_str());
      if (!pChild)
      { // add our group tag
        TiXmlElement newElement(strSplit[0].c_str());
        pChild = pRootNode->InsertEndChild(newElement);
      }

      if (pChild)
      { // successfully added (or found) our group
        TiXmlElement newElement(strSplit[1]);
        if ((*it).second->GetControlType() == SETTINGS_TYPE_PATH)
          newElement.SetAttribute("pathversion", CSpecialProtocol::path_version);
        TiXmlNode *pNewNode = pChild->InsertEndChild(newElement);
        if (pNewNode)
        {
          TiXmlText value((*it).second->ToString());
          pNewNode->InsertEndChild(value);
        }
      }
    }
  }
}

void CGUISettings::Clear()
{
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
    delete (*it).second;
  settingsMap.clear();
  for (unsigned int i = 0; i < settingsGroups.size(); i++)
    delete settingsGroups[i];
  settingsGroups.clear();
}

void CGUISettings::LoadCustomSettingsOrder()
{
  int separator = 1;
  char separatorStr[10];
  
  m_customSettingsOrder = false;
  
  std::vector<CSettingsGroup *> mySettingsGroups;
  
  // load the xml file
  TiXmlDocument xmlDoc;
  CStdString fileName = "special://xbmc/system/settingsmap.xml";
  fileName = _P(fileName);
 
  if (!XFILE::CFile::Exists(fileName))
  {
    CLog::Log(LOGINFO, "No settingsmap.xml to load (%s). Using default settings.", fileName.c_str());
    return;
  }
 
  if (!xmlDoc.LoadFile(fileName))
  {
    CLog::Log(LOGERROR, "%s, Line %d: %s", fileName.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return;
  }

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (strcmpi(pRootElement->Value(), "settingsmap") != 0)
  {
    CLog::Log(LOGERROR, "%s: Doesn't contain <settingsmap>", fileName.c_str());
    return;
  }

  const TiXmlElement *pGroup = pRootElement->FirstChildElement("group");
  while (pGroup)
  {
    // Read group
    const char* groupId = pGroup->Attribute("id");
    const char* groupWindow = pGroup->Attribute("window");
    if (groupId == NULL || groupWindow == NULL)
    {
      CLog::Log(LOGERROR, "%s: <group> has no id or window", fileName.c_str());
      continue;
    }
    
    // Create group object
    CSettingsGroup* group = new CSettingsGroup(atoi(groupId), atoi(groupWindow));
    
    const TiXmlElement *pCategory = pGroup->FirstChildElement("category");
    while (pCategory)
    {
      const char* categoryId = pCategory->Attribute("id");
      const char* categoryLabel = pCategory->Attribute("label");
      if (categoryId == NULL || categoryLabel == NULL)
      {
        CLog::Log(LOGERROR, "%s: <category> has no id or label for group %s", fileName.c_str(), groupId);
        pGroup = pGroup->NextSiblingElement("group");
        continue;

      }      
      const char* catPlatform = pCategory->Attribute("platform");
      if (catPlatform != NULL && !CUtil::MatchesPlatform(catPlatform))
      {
    	  pCategory = pCategory->NextSiblingElement();
          continue;
      }
      bool runtime = false;
      CStdString runTimeStr = pCategory->Attribute("runtime");
      runTimeStr.ToLower();

      if (runTimeStr.Equals("true"))
      {
    	CLog::Log(LOGDEBUG,"category %s will be build in run time ", categoryLabel);
    	runtime = true;
      }
      // Create category object
      CSettingsCategory* category = group->AddCategory(categoryId, atoi(categoryLabel), runtime);
      
      const TiXmlElement *pSetting = pCategory->FirstChildElement();
      while (pSetting)
      {
        if (stricmp(pSetting->Value(), "setting") == 0)
        {
          const char* settingId = pSetting->Attribute("id");
          const char* platform = pSetting->Attribute("platform");
          
          if (settingId == NULL)
          {
            CLog::Log(LOGERROR, "%s: <setting> has no id in category %s", fileName.c_str(), categoryId);
            pSetting = pSetting->NextSiblingElement();
            continue;
          }
          
          if (platform != NULL && !CUtil::MatchesPlatform(platform))
          {
              pSetting = pSetting->NextSiblingElement();
              continue;
          }
      
          if (settingsMap.find(settingId) == settingsMap.end())
          {
            CLog::Log(LOGERROR, "%s: <setting> %s was not found", fileName.c_str(), settingId);
            pSetting = pSetting->NextSiblingElement();
            continue;
          }
          
          category->AddSetting(settingsMap[settingId]);
        } 
        else if (stricmp(pSetting->Value(), "separator") == 0)
        {
          sprintf(separatorStr, "sep%d", separator);
          category->AddSetting(new CSettingSeparator(separator, separatorStr));
          separator++;
        }
        
        pSetting = pSetting->NextSiblingElement();
      }
      
      pCategory = pCategory->NextSiblingElement("category");
    }
    
    mySettingsGroups.push_back(group);
    
    pGroup = pGroup->NextSiblingElement("group");
  }
  
  // Now replace the actual vector of groups
  settingsGroups.clear();
  settingsGroups = mySettingsGroups;
  
  m_customSettingsOrder = true;
}

void CGUISettings::SetResolution(RESOLUTION res)
{
  CStdString mode;
  if (res == RES_DESKTOP)
    mode = "DESKTOP";
  else if (res == RES_WINDOW)
    mode = "WINDOW";
  else if (res >= RES_CUSTOM && res < (RESOLUTION)g_settings.m_ResInfo.size())
  {
    const RESOLUTION_INFO &info = g_settings.m_ResInfo[res];
    mode.Format("%1i%05i%05i%09.5f", info.iScreen, info.iWidth, info.iHeight, info.fRefreshRate);
  }
  else
  {
    CLog::Log(LOGWARNING, "%s, setting invalid resolution %i", __FUNCTION__, res);
    mode = "DESKTOP";
  }
  SetString("videoscreen.screenmode", mode);
}
