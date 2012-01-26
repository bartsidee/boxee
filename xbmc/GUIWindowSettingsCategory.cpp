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
#include "GUIUserMessages.h"
#include "GUIWindowSettingsCategory.h"
#include "Application.h"
#include "utils/Builtins.h"
#include "KeyboardLayoutConfiguration.h"
#include "FileSystem/Directory.h"
#include "FileSystem/DirectoryCache.h"
#include "FileSystem/File.h"
#include "Util.h"
#include "GUISpinControlEx.h"
#include "GUISliderControl.h"
#include "GUIRadioButtonControl.h"
#include "GUIEditControl.h"
#include "GUILabelControl.h"
#include "GUIImage.h"
#include "utils/Weather.h"
#include "MusicDatabase.h"
#include "VideoDatabase.h"
#include "ProgramDatabase.h"
#include "ViewDatabase.h"
#include "XBAudioConfig.h"
#ifdef HAS_LCD
#include "utils/LCDFactory.h"
#endif
#include "visualizations/VisualisationFactory.h"
#include "PlayListPlayer.h"
#include "SkinInfo.h"
#include "GUIAudioManager.h"
#include "AudioContext.h"
#include "lib/libscrobbler/lastfmscrobbler.h"
#include "lib/libscrobbler/librefmscrobbler.h"
#include "GUIPassword.h"
#include "utils/GUIInfoManager.h"
#include "GUICheckMarkControl.h"
#include "GUIDialogGamepad.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogFileBrowser.h"
#include "GUIFontManager.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogYesNo2.h"
#include "GUIDialogOK.h"
#include "GUIDialogOK2.h"
#include "GUIDialogKaiToast.h"
#include "GUIWindowPrograms.h"
#include "MediaManager.h"
#include "utils/Network.h"
#ifdef HAS_WEB_SERVER
#include "lib/libGoAhead/WebServer.h"
#endif
#include "GUIControlGroupList.h"
#include "GUIWindowManager.h"
#ifdef _LINUX
#include "LinuxTimezone.h"
#include <dlfcn.h>
#if !defined(__APPLE__) 
#include "cores/AudioRenderers/ALSADirectSound.h"
#endif
#ifdef HAS_HAL
#include "HALManager.h"
#endif
#endif
#ifdef __APPLE__
#include "CoreAudio.h"
#include "XBMCHelper.h"
#include "CocoaInterface.h"
#endif
#include "GUIDialogAccessPoints.h"
#include "FileSystem/Directory.h"
#include "utils/ScraperParser.h"

#include "FileItem.h"
#include "GUIToggleButtonControl.h"
#include "FileSystem/SpecialProtocol.h"
#include "File.h"

#include "Zeroconf.h"
#include "PowerManager.h"

#ifdef _WIN32
#include "WIN32Util.h"
#include "WINDirectSound.h"

#define lstat stat
#include "DVDInputStreams\dvdnav\config.h"
#endif
#include <map>
#include "ScraperSettings.h"
#include "ScriptSettings.h"
#include "GUIDialogPluginSettings.h"
#include "Settings.h"
#include "AdvancedSettings.h"
#include "MouseStat.h"
#include "LocalizeStrings.h"
#include "LangInfo.h"
#include "StringUtils.h"
#include "WindowingFactory.h"
#include "GUISound.h"
#include "lib/libBoxee/boxee.h"
#include "KeyboardManager.h"
#include "bxconfiguration.h"
#include "BoxeeUtils.h"
#include "GUIDialogBoxeeNetworkNotification.h"
#include "HalServices.h"

#include "StringUtils.h"
#include "GUIWindowStateDatabase.h"
#include "GUIDialogSelect.h"

#ifdef HAS_EMBEDDED
#include "InitializeBoxManager.h"
#include "HalListenerImpl.h"
#include "BoxeeVersionUpdateManager.h"
#include "GUIDialogYesNo2.h"
#endif
#ifdef HAS_INTEL_SMD
// For format checking
#include <ismd_audio.h>
#include <ismd_audio_ac3.h>
#include <ismd_audio_ddplus.h>
#include <ismd_audio_truehd.h>
#include <ismd_audio_dts.h>
#include "cores/dvdplayer/DVDCodecs/DVDCodecs.h"
#include "IntelSMDGlobals.h"
#endif
#ifdef HAS_DVB
#include "dvbmanager.h"
#include "BoxeeOTAConfigurationManager.h"
#endif
#include "LicenseConfig.h"
#include "GUIWindowBoxeeMain.h"
#include "AppManager.h"
#include "BrowserService.h"
#include "BoxeeBrowseMenuManager.h"

using namespace std;
using namespace DIRECTORY;
using namespace XFILE;

#define CONTROL_GROUP_BUTTONS           0
#define CONTROL_GROUP_SETTINGS          1
#define CONTROL_SETTINGS_LABEL          2
#define CATEGORY_GROUP_ID               3
#define SETTINGS_GROUP_ID               5
#define CONTROL_DEFAULT_BUTTON          7
#define CONTROL_DEFAULT_RADIOBUTTON     8
#define CONTROL_DEFAULT_SPIN            9
#define CONTROL_DEFAULT_CATEGORY_BUTTON 10
#define CONTROL_DEFAULT_SEPARATOR       11
#define CONTROL_DEFAULT_EDIT            12
#define CONTROL_SETTINGS_GROUP_LABEL    92
#define CONTROL_START_BUTTONS           -100
#define CONTROL_START_CONTROL           -80

#define PREDEFINED_SCREENSAVERS          5

#define FEEDS_CATEGORY                   800

#define LINE_SIZE                       1024

#define FHOST_PATH                "/etc/hosts"
#define FHOST_TEMP_PATH           "/tmp/hostsTmp"
#define STAGING_LINE              "74.204.171.122   app.boxee.tv dir.boxee.tv res.boxee.tv\n"
#define HOSTS_LINE                "app.boxee.tv dir.boxee.tv res.boxee.tv\n"

#define SECONDS_TO_WAIT_FOR_INTERNET_CONNECTION           10

CGUIWindowSettingsCategory::CGUIWindowSettingsCategory(void)
    : CGUIWindow(WINDOW_SETTINGS_MYPICTURES, "SettingsCategory.xml")
{
  m_pOriginalSpin = NULL;
  m_pOriginalRadioButton = NULL;
  m_pOriginalButton = NULL;
  m_pOriginalCategoryButton = NULL;
  m_pOriginalImage = NULL;
  m_pOriginalEdit = NULL;
  // set the correct ID range...
  m_idRange = 8;
  m_iScreen = 0;
  // set the network settings so that we don't reset them unnecessarily
  m_iNetworkAssignment = -1;
  m_strErrorMessage = "";
  m_strOldTrackFormat = "";
  m_strOldTrackFormatRight = "";
  m_iSectionBeforeJump=-1;
  m_iControlBeforeJump=-1;
  m_iWindowBeforeJump=WINDOW_INVALID;
  m_returningFromSkinLoad = false;
  m_bBoxeeRemoteEnabled = false;
#ifdef HAS_DVB
  m_iOtaScanSection = -1;
#endif
#ifdef HAS_EMBEDDED
  m_iUpdateSection = -1;
  m_bDownloadingUpdate = false;
  m_bSmbServerEnable = false;
  m_smbPassword = "";
  m_smbWorkgroup = "";
  m_smbHostname = "";
  m_tzChosenCountryName = "";
  m_needToLoadTimezoneCitiesControl = false;
  m_tzChosenCountryName = "";
  m_tzChosenCityName = "";
  ReadTimezoneData();
  m_iNetworkInterface = -1;
#endif
}

CGUIWindowSettingsCategory::~CGUIWindowSettingsCategory(void)
{
  FreeControls();
    delete m_pOriginalEdit;
}

bool CGUIWindowSettingsCategory::OnAction(const CAction &action)
{
  if (action.id == ACTION_PREVIOUS_MENU)
  {
    g_windowManager.PreviousWindow();
    g_settings.Save();
    if (m_iWindowBeforeJump!=WINDOW_INVALID)
    {
      JumpToPreviousSection();
      return true;
    }
    m_lastControlID = 0; // don't save the control as we go to a different window each time
    return true;
  }
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowSettingsCategory::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
    {
      unsigned int iControl = message.GetSenderId();
      /*   if (iControl >= CONTROL_START_BUTTONS && iControl < CONTROL_START_BUTTONS + m_vecSections.size())
         {
          // change the setting...
          m_iSection = iControl-CONTROL_START_BUTTONS;
          CheckNetworkSettings();
          CreateSettings();
          return true;
         }*/
      for (unsigned int i = 0; i < m_vecSettings.size(); i++)
      {
        if (m_vecSettings[i]->GetID() == (int)iControl)
          OnClick(m_vecSettings[i]);
      }
    }
    break;
  case GUI_MSG_FOCUSED:
    {
      CGUIWindow::OnMessage(message);
      int focusedControl = GetFocusedControlID();
      if (focusedControl >= CONTROL_START_BUTTONS && focusedControl < (int)(CONTROL_START_BUTTONS + m_vecSections.size()) &&
          focusedControl - CONTROL_START_BUTTONS != m_iSection)
      {
        // changing section, check for updates
        CheckForUpdates();

        if (m_vecSections[focusedControl-CONTROL_START_BUTTONS]->m_strCategory == "masterlock")
        {
          if (!g_passwordManager.IsMasterLockUnlocked(true))
          { // unable to go to this category - focus the previous one
            SET_CONTROL_FOCUS(CONTROL_START_BUTTONS + m_iSection, 0);
            return false;
          }
        }
        m_iSection = focusedControl - CONTROL_START_BUTTONS;
        CheckNetworkSettings();

        CreateSettings();
      }
      return true;
    }
  case GUI_MSG_LOAD_SKIN:
    {
      // Do we need to reload the language file
      if (!m_strNewLanguage.IsEmpty())
      {
        LoadNewLanguage();
      }

      // Do we need to reload the skin font set
      if (!m_strNewSkinFontSet.IsEmpty())
      {
        g_guiSettings.SetString("lookandfeel.font", m_strNewSkinFontSet);
        g_settings.Save();
      }

      // Reload another skin
      if (!m_strNewSkin.IsEmpty())
      {
        g_guiSettings.SetString("lookandfeel.skin", m_strNewSkin);
        g_settings.Save();
      }

      // Reload a skin theme
      if (!m_strNewSkinTheme.IsEmpty())
      {
        g_guiSettings.SetString("lookandfeel.skintheme", m_strNewSkinTheme);
        // also set the default color theme
        CStdString colorTheme(m_strNewSkinTheme);
        CUtil::ReplaceExtension(colorTheme, ".xml", colorTheme);
        if (colorTheme.Equals("Textures.xml"))
          colorTheme = "defaults.xml";
        g_guiSettings.SetString("lookandfeel.skincolors", colorTheme);
        g_settings.Save();
      }

      // Reload a skin color
      if (!m_strNewSkinColors.IsEmpty())
      {
        g_guiSettings.SetString("lookandfeel.skincolors", m_strNewSkinColors);
        g_settings.Save();
      }

      if (IsActive())
        m_returningFromSkinLoad = true;
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      if (message.GetParam1() != WINDOW_INVALID && !m_returningFromSkinLoad)
      { // coming to this window first time (ie not returning back from some other window)
        // so we reset our section and control states
        m_iSection = 0;
        ResetControlStates();
      }
      m_returningFromSkinLoad = false;
      m_iScreen = (int)message.GetParam2() - (int)CGUIWindow::GetID();
	    m_bBoxeeRemoteEnabled = g_guiSettings.GetBool("boxee.irssremote");
#ifdef HAS_EMBEDDED
      CHalServicesFactory::GetInstance().GetSambaConfig(m_bSmbServerEnable, m_smbPassword, m_smbWorkgroup, m_smbHostname);

      g_guiSettings.SetBool("smbd.enable", m_bSmbServerEnable);
      g_guiSettings.SetString("smbd.password", m_smbPassword);
      g_guiSettings.SetString("smbd.workgroup", m_smbWorkgroup);
      m_tzChosenCountryName = g_guiSettings.GetString("timezone.country");
      m_tzChosenCityName = g_guiSettings.GetString("timezone.city");
      m_needToLoadTimezoneCitiesControl = false;
#endif

      return CGUIWindow::OnMessage(message);
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
    {
      // Hardware based stuff
      // TODO: This should be done in a completely separate screen
      // to give warning to the user that it writes to the EEPROM.

// dconti- remove stale xbox code
#if 0
      if (g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL_SPDIF || g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL_HDMI)
      {
        g_audioConfig.SetAC3Enabled(g_guiSettings.GetBool("audiooutput.ac3passthrough"));
        g_audioConfig.SetDTSEnabled(g_guiSettings.GetBool("audiooutput.dtspassthrough"));
        if (g_audioConfig.NeedsSave())
        { // should we perhaps show a dialog here?
          g_audioConfig.Save();
        }
      }
#endif

      CheckForUpdates();
      CheckNetworkSettings();
      CheckRuntimeSettings();
#ifdef HAS_EMBEDDED
      CheckTimezoneSettings();
#endif
      CGUIWindow::OnMessage(message);
      FreeControls();
      return true;
    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowSettingsCategory::SetupControls()
{
  // cleanup first, if necessary
  m_containRuntimeCategory = false;
  FreeControls();
  m_pOriginalSpin = (CGUISpinControlEx*)GetControl(CONTROL_DEFAULT_SPIN);
  m_pOriginalRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_DEFAULT_RADIOBUTTON);
  m_pOriginalCategoryButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_CATEGORY_BUTTON);
  m_pOriginalButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_BUTTON);
  m_pOriginalImage = (CGUIImage *)GetControl(CONTROL_DEFAULT_SEPARATOR);
  if (!m_pOriginalCategoryButton || !m_pOriginalSpin || !m_pOriginalRadioButton || !m_pOriginalButton)
    return ;
  m_pOriginalEdit = (CGUIEditControl *)GetControl(CONTROL_DEFAULT_EDIT);
  if (!m_pOriginalEdit || m_pOriginalEdit->GetControlType() != CGUIControl::GUICONTROL_EDIT)
  {
    delete m_pOriginalEdit;
    m_pOriginalEdit = new CGUIEditControl(*m_pOriginalButton);
  }
  m_pOriginalSpin->SetVisible(false);
  m_pOriginalRadioButton->SetVisible(false);
  m_pOriginalButton->SetVisible(false);
  m_pOriginalCategoryButton->SetVisible(false);
  m_pOriginalEdit->SetVisible(false);
  if (m_pOriginalImage) m_pOriginalImage->SetVisible(false);
  // setup our control groups...
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CATEGORY_GROUP_ID);
  if (!group)
      return;
  // get a list of different sections
  CSettingsGroup *pSettingsGroup = g_guiSettings.GetGroup(m_iScreen);
  if (!pSettingsGroup) return ;
  // update the screen string
  SET_CONTROL_LABEL(CONTROL_SETTINGS_LABEL, pSettingsGroup->GetLabelID());
  if (pSettingsGroup->GetTitleID() > 0)
  {
    SET_CONTROL_LABEL(CONTROL_SETTINGS_GROUP_LABEL, pSettingsGroup->GetTitleID());
  }
  // get the categories we need
  pSettingsGroup->GetCategories(m_vecSections);
  // run through and create our buttons...
  int j=0;
  for (unsigned int i = 0; i < m_vecSections.size(); i++)
  {
    if (m_vecSections[i]->m_labelID == 12360 && g_settings.m_iLastLoadedProfileIndex != 0)
      continue;
    CGUIButtonControl *pButton = NULL;
    if (m_pOriginalCategoryButton->GetControlType() == CGUIControl::GUICONTROL_TOGGLEBUTTON)
      pButton = new CGUIToggleButtonControl(*(CGUIToggleButtonControl *)m_pOriginalCategoryButton);
    else
      pButton = new CGUIButtonControl(*m_pOriginalCategoryButton);
    pButton->SetLabel(g_localizeStrings.Get(m_vecSections[i]->m_labelID));
    pButton->SetID(CONTROL_START_BUTTONS + j);
    pButton->SetVisible(true);
    pButton->AllocResources();
    group->AddControl(pButton);
    j++;
  }
  if (m_iSection < 0 || m_iSection >= (int)m_vecSections.size())
    m_iSection = 0;
  CreateSettings();
  // set focus correctly
  m_defaultControl = CONTROL_START_BUTTONS;
}

void CGUIWindowSettingsCategory::CreateRunTimeSettings()
{
  m_containRuntimeCategory = true;
  for (int i = 0; i < (int)m_vecSections[m_iSection]->m_vecSettings.size() ; i ++ )
  {
	  CSetting *pSetting = m_vecSections[m_iSection]->m_vecSettings[i];
    g_guiSettings.DeleteSetting(pSetting->GetSetting());
  }
  m_vecSections[m_iSection]->m_vecSettings.clear();

  switch (m_vecSections[m_iSection]->m_labelID)
  {
  // currently we only build the feed setting in runtime.
  // but probably in the future we will have to generate other category in run time
  // so, the purpose that in the future the xml parsing code will be shared for all
  // the categroies, and just the url string, and final adjustmnet will be specific to
  // the categroy
  case FEEDS_CATEGORY:
  {
    CSetting *pSetting = NULL;
    int iControlID = CONTROL_START_CONTROL;

    CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(SETTINGS_GROUP_ID);
    if (!group)
    {
      return;
    }

    // first check if the connect to Internet
    if (!g_application.IsConnectedToInternet())
    {
      g_guiSettings.AddString(0, "feeds.notconnected", 53756 , "", BUTTON_CONTROL_STANDARD);
      pSetting = g_guiSettings.GetSetting("feeds.notconnected");
      m_vecSections[m_iSection]->AddSetting(pSetting);
      AddSetting(pSetting, group->GetWidth(), iControlID);

      CGUIButtonControl *pButtonControl = (CGUIButtonControl *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      pButtonControl->SetHeight(200);
      return ;
    }

	  // build feeds settings
	  CLog::Log(LOGDEBUG,"CGUIWindowSettingsCategory::CreateRunTimeSettings - create feeds settings (frts)");

	  std::string strUrl = BOXEE::BXConfiguration::GetInstance().GetURLParam("Boxee.ApiFeedSettingsUrl","http://app.boxee.tv/api/feedsettings");

	  BOXEE::BXXMLDocument feedsSettingList;
  	bool succeeded = feedsSettingList.LoadFromURL(strUrl);

	  if (!succeeded)
	  {
	    CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::CreateRunTimeSettings - FAILED to LoadFromUrl (frts)");
	    return ;
	  }

	  if (feedsSettingList.GetDocument().Error())
	  {
	    CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::CreateRunTimeSettings - FAILED to LoadFromUrl (frts)");
	    return ;
	  }

	  CPluginSettings settings;
	  if (!settings.Load(feedsSettingList.GetDocument()))
	  {
	    CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::CreateRunTimeSettings - FAILED to load settings (frts)");
	    return ;
	  }

	  if (!settings.GetPluginRoot())
	  {
	    CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::CreateRunTimeSettings - FAILED to find <setting> tag (frts)");
	    return ;
	  }

	  TiXmlElement *xmlSetting = settings.GetPluginRoot()->FirstChildElement("setting");

    bool headerExist = false;
    bool headerWasSet = false;
    CStdString  headerStr;

    while (xmlSetting)
    {
      bool shouldAddSetting = false;
	    const char *type = xmlSetting->Attribute("type");
	    const char *id = xmlSetting->Attribute("id");
	    const char *value = xmlSetting->Attribute("value");
	    const char *label = xmlSetting->Attribute("label");

	    if ((type == NULL) || (id == NULL) || (label == NULL))
	    {
	      CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::CreateRunTimeSettings - feeds doesn't contain one of the following attribute : id, type, label - skip it (frts)");
	      xmlSetting = xmlSetting->NextSiblingElement("setting");
	      continue;
	    }

	    if (strcmpi(type, "bool") == 0 )
	    {
	      if (value == NULL)
	      {
	        CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::CreateRunTimeSettings - feeds doesn't contain one of the following attribute : id, type, label - skip it (frts)");
	        xmlSetting = xmlSetting->NextSiblingElement("setting");
	        continue;
	      }

	      g_guiSettings.AddBool(0, label, BoxeeUtils::TranslateStringById(label) , (stricmp(value, "true") == 0 ? true :  false));

	      shouldAddSetting = true;
	    }
	    else if (strcmpi(type, "text") == 0 )
	    {
	      if (value == NULL)
	      {
	        CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::CreateRunTimeSettings - feeds doesn't contain one of the following attribute : id, type, label - skip it (frts)");
	        xmlSetting = xmlSetting->NextSiblingElement("setting");
	        continue;
	      }

	      g_guiSettings.AddString(0, label, BoxeeUtils::TranslateStringById(label) , value, BUTTON_CONTROL_STANDARD);
	      shouldAddSetting = true;
	    }
	    else if (strcmpi(type, "header") == 0 )
	    {
	      headerStr = BoxeeUtils::TranslateStringById(label);
	      headerExist = true;
	    }
	    else
	    {
	      CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::CreateRunTimeSettings - feeds doesn't support [%s] type (frts)", type);
	      xmlSetting = xmlSetting->NextSiblingElement("setting");
	      continue;
	    }

	    if (shouldAddSetting)
	    {
	      if (headerExist && !headerWasSet)
	      {
	        //////////////////////////////////////////////////
	        // at least 1 line -> set header to the section //
	        //////////////////////////////////////////////////

	        g_guiSettings.AddString(0, "feeds.header", headerStr , "", BUTTON_CONTROL_STANDARD);
	        pSetting = g_guiSettings.GetSetting("feeds.header");
	        m_vecSections[m_iSection]->AddSetting(pSetting);
	        AddSetting(pSetting, group->GetWidth(), iControlID);
	        CGUIButtonControl *pButtonControl = (CGUIButtonControl *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
	        pButtonControl->SetEnabled(false);

	        headerWasSet = true;
	      }

	      pSetting = g_guiSettings.GetSetting(label);
	      pSetting->SetCustomData(id);
	      m_vecSections[m_iSection]->AddSetting(pSetting);

	      AddSetting(pSetting, group->GetWidth(), iControlID);

  	    if (pSetting->GetControlType() == BUTTON_CONTROL_STANDARD)
	      {
	        CGUIButtonControl *pButtonControl = (CGUIButtonControl *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
	        CSettingString *pSettingStr = (CSettingString *)pSetting;
          pButtonControl->SetLabel2(BoxeeUtils::TranslateStringById(pSettingStr->GetData()));
          pButtonControl->SetEnabled(false);
	      }
	    }

	    xmlSetting = xmlSetting->NextSiblingElement("setting");
	    iControlID ++;
    }

    // if only header exist then it is probabley the registeration message - display it in a large control
    if (headerExist && (m_vecSections[m_iSection]->m_vecSettings.size() == 0) && !headerWasSet)
    {
      g_guiSettings.AddString(0, "feeds.header", headerStr , "", BUTTON_CONTROL_STANDARD);
      pSetting = g_guiSettings.GetSetting("feeds.header");
      m_vecSections[m_iSection]->AddSetting(pSetting);
      AddSetting(pSetting, group->GetWidth(), iControlID);

      CGUIButtonControl *pButtonControl = (CGUIButtonControl *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      pButtonControl->SetHeight(200);
      pButtonControl->SetEnabled(false);
    }
    break;
    }
    default:
    {
      CLog::Log(LOGERROR,"Can't Build runtime settings for category [%s] (frts)", m_vecSections[m_iSection]->m_strCategory.c_str());
    }
    break;
  }
}

void CGUIWindowSettingsCategory::CreateSettings()
{
  FreeSettingsControls();

  if (m_vecSections[m_iSection]->m_runTime)
  {
    CreateRunTimeSettings();
    return;
  }

  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(SETTINGS_GROUP_ID);
  if (!group)
    return;
  vecSettings settings;
  if (!g_guiSettings.IsUsingCustomSettingsOrder())
  {
    g_guiSettings.GetSettingsGroup(m_vecSections[m_iSection]->m_strCategory, settings);
  }
  else
  {
    settings = m_vecSections[m_iSection]->m_vecSettings;
  }
  
  int services = 0;
  int iControlID = CONTROL_START_CONTROL;
  for (unsigned int i = 0; i < settings.size(); i++)
  {
    CSetting *pSetting = settings[i];
    CStdString strSetting = pSetting->GetSetting();

    if (strSetting.Equals("services.netflixclear") || strSetting.Equals("services.netflixesn"))
    {
      if (!XFILE::CFile::Exists("/data/.persistent/apps/nrd"))
      {
        continue;
      }
      else
      {
        services++;
      }
    }
    else if (strSetting.Equals("services.vuduclear"))
    {
      if (!XFILE::CFile::Exists("/data/.persistent/apps/vudu") || XFILE::CFile::Exists("/data/.persistent/apps/vudu/deactivate"))
      {
        continue;
      }
      else
      {
        services++;
      }
    }
    if (strSetting.Equals("services.spotifyclear"))
    {
      if (!XFILE::CFile::Exists("/data/.persistent/apps/spotify"))
      {
        continue;
      }
      else
      {
        services++;
      }
    }
    else if (strSetting.Equals("services.none"))
    {
      if (services > 0)
      {
        continue;
      }
    }

    AddSetting(pSetting, group->GetWidth(), iControlID);

    if (strSetting.Equals("services.none"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(false);
    }
    else if (strSetting.Equals("myprograms.ntscmode"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      for (int i = pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i++)
      {
        pControl->AddLabel(g_localizeStrings.Get(16106 + i), i);
      }
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("mymusic.visualisation"))
    {
      FillInVisualisations(pSetting, GetSetting(pSetting->GetSetting())->GetID());
    }
    else if (strSetting.Equals("musiclibrary.defaultscraper"))
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      FillInScrapers(pControl, g_guiSettings.GetString("musiclibrary.defaultscraper"), "music");
    }
    else if (strSetting.Equals("scrapers.moviedefault"))
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      FillInScrapers(pControl, g_guiSettings.GetString("scrapers.moviedefault"), "movies");
    }
    else if (strSetting.Equals("scrapers.tvshowdefault"))
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      FillInScrapers(pControl, g_guiSettings.GetString("scrapers.tvshowdefault"), "tvshows");
    }
    else if (strSetting.Equals("scrapers.musicvideodefault"))
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      FillInScrapers(pControl, g_guiSettings.GetString("scrapers.musicvideodefault"), "musicvideos");
    }
    else if (strSetting.Equals("karaoke.port0voicemask"))
    {
      FillInVoiceMasks(0, pSetting);
    }
    else if (strSetting.Equals("karaoke.port1voicemask"))
    {
      FillInVoiceMasks(1, pSetting);
    }
    else if (strSetting.Equals("karaoke.port2voicemask"))
    {
      FillInVoiceMasks(2, pSetting);
    }
    else if (strSetting.Equals("karaoke.port3voicemask"))
    {
      FillInVoiceMasks(3, pSetting);
    }
    else if (strSetting.Equals("debug.loglevel"))
    {
      FillInLogLevels(pSetting);
    }
    else if (strSetting.Equals("vpn.type"))
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      pControl->Clear();
      pControl->AddLabel(g_localizeStrings.Get(54738), 0); // PPTP
    }
    else if (strSetting.Equals("audiooutput.library"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(53268), AUDIO_LIBRARY_ALSA);
      pControl->AddLabel(g_localizeStrings.Get(53267), AUDIO_LIBRARY_PULSEAUDIO);
      pControl->SetValue(pSettingInt->GetData());
    }    
    else if (strSetting.Equals("audiooutput.mode"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(338), AUDIO_ANALOG);
      if (g_audioConfig.HasDigitalOutput())
      {
        pControl->AddLabel(g_localizeStrings.Get(339), AUDIO_DIGITAL_SPDIF);
#ifdef HAS_AUDIO_HDMI
        pControl->AddLabel(g_localizeStrings.Get(54123), AUDIO_DIGITAL_HDMI);
#endif
#ifdef HAS_INTEL_SMD
        pControl->AddLabel(g_localizeStrings.Get(54155), AUDIO_ALL_OUTPUTS);
#endif
      }
      pControl->SetValue(pSettingInt->GetData());
    }
#ifdef HAS_INTEL_SMD
    else if (strSetting.Equals("audiooutput.dd_truehd_drc"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(54158), DD_TRUEHD_DRC_AUTO);
      pControl->AddLabel(g_localizeStrings.Get(54159), DD_TRUEHD_DRC_OFF);
      pControl->AddLabel(g_localizeStrings.Get(54160), DD_TRUEHD_DRC_ON);

      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("audiooutput.dd_truehd_drc_percentage"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      if(g_guiSettings.GetInt("audiooutput.dd_truehd_drc") == DD_TRUEHD_DRC_ON)
      {
        pControl->SetEnabled(true);
      }

      for(int i = DD_TRUEHD_DRC_PRC_MIN; i <= DD_TRUEHD_DRC_PRC_MAX; i++)
      {
        CStdString label;
        label.Format("%d%s",i,"%");;
        pControl->AddLabel(label.c_str() , i);
      }
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("audiooutput.downmix"))
    {
      CSettingInt *pSettingInt = (CSettingInt*) pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *) GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel("DEFAULT", ISMD_AUDIO_DOWNMIX_DEFAULT);
      pControl->AddLabel("1_0", ISMD_AUDIO_DOWNMIX_1_0);
      pControl->AddLabel("1_0_LFE", ISMD_AUDIO_DOWNMIX_1_0_LFE);
      pControl->AddLabel("2_0", ISMD_AUDIO_DOWNMIX_2_0);
      pControl->AddLabel("2_0_NO_SCALE", ISMD_AUDIO_DOWNMIX_2_0_NO_SCALE);
      pControl->AddLabel("2_0_LFE", ISMD_AUDIO_DOWNMIX_2_0_LFE);
      pControl->AddLabel("2_0_LTRT", ISMD_AUDIO_DOWNMIX_2_0_LTRT);
      pControl->AddLabel("2_0_LTRT_NO_SCALE", ISMD_AUDIO_DOWNMIX_2_0_LTRT_NO_SCALE);
      pControl->AddLabel("2_0_DOLBY_PRO_LOGIC_II", ISMD_AUDIO_DOWNMIX_2_0_DOLBY_PRO_LOGIC_II);

      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("audiooutput.ddplus_output_config"))
    {
      CSettingInt *pSettingInt = (CSettingInt*) pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *) GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel("1_0_C", ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_1_0_C);
      pControl->AddLabel("2_0_LR", ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_2_0_LR);

      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("audiooutput.ddplus_lfe_channel_config"))
    {
      CSettingInt *pSettingInt = (CSettingInt*) pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *) GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel("NONE", ISMD_AUDIO_DDPLUS_LFE_CHANNEL_OUTPUT_NONE);
      pControl->AddLabel("ENABLED", ISMD_AUDIO_DDPLUS_LFE_CHANNEL_OUTPUT_ENABLED);
      pControl->AddLabel("DUAL", ISMD_AUDIO_DDPLUS_LFE_CHANNEL_OUTPUT_DUAL);

      pControl->SetValue(pSettingInt->GetData());
    }
#endif
    else if (strSetting.Equals("videooutput.aspect"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(21375), VIDEO_NORMAL);
      pControl->AddLabel(g_localizeStrings.Get(21376), VIDEO_LETTERBOX);
      pControl->AddLabel(g_localizeStrings.Get(21377), VIDEO_WIDESCREEN);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("cddaripper.encoder"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel("Lame", CDDARIP_ENCODER_LAME);
      pControl->AddLabel("Vorbis", CDDARIP_ENCODER_VORBIS);
      pControl->AddLabel("Wav", CDDARIP_ENCODER_WAV);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("cddaripper.quality"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(604), CDDARIP_QUALITY_CBR);
      pControl->AddLabel(g_localizeStrings.Get(601), CDDARIP_QUALITY_MEDIUM);
      pControl->AddLabel(g_localizeStrings.Get(602), CDDARIP_QUALITY_STANDARD);
      pControl->AddLabel(g_localizeStrings.Get(603), CDDARIP_QUALITY_EXTREME);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("lcd.type"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(351), LCD_TYPE_NONE);
#ifdef _LINUX
      pControl->AddLabel("LCDproc", LCD_TYPE_LCDPROC);
#endif
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("harddisk.aamlevel"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(21388), AAM_QUIET);
      pControl->AddLabel(g_localizeStrings.Get(21387), AAM_FAST);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("harddisk.apmlevel"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(21391), APM_HIPOWER);
      pControl->AddLabel(g_localizeStrings.Get(21392), APM_LOPOWER);
      pControl->AddLabel(g_localizeStrings.Get(21393), APM_HIPOWER_STANDBY);
      pControl->AddLabel(g_localizeStrings.Get(21394), APM_LOPOWER_STANDBY);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("system.targettemperature"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      for (int i = pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i++)
      {
        CTemperature temp=CTemperature::CreateFromCelsius(i);
        pControl->AddLabel(temp.ToString(), i);
      }
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("system.fanspeed") || strSetting.Equals("system.minfanspeed")) 
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      CStdString strPercentMask = g_localizeStrings.Get(14047);
      for (int i=pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i += pSettingInt->m_iStep)
      {
        CStdString strLabel;
        strLabel.Format(strPercentMask.c_str(), i*2);
        pControl->AddLabel(strLabel, i);
      }
      pControl->SetValue(int(pSettingInt->GetData()));
    }
    else if (strSetting.Equals("servers.webserverusername"))
    {
#ifdef HAS_WEB_SERVER
      // get password from the webserver if it's running (and update our settings)
      if (g_application.m_pWebServer)
      {
        ((CSettingString *)GetSetting(strSetting)->GetSetting())->SetData(g_application.m_pWebServer->GetUserName());
        g_settings.Save();
      }
#endif
    }
    else if (strSetting.Equals("servers.webserverpassword"))
    { 
#ifdef HAS_WEB_SERVER
      // get password from the webserver if it's running (and update our settings)
      if (g_application.m_pWebServer)
      {
        ((CSettingString *)GetSetting(strSetting)->GetSetting())->SetData(g_application.m_pWebServer->GetPassword());
        g_settings.Save();
      }
#endif
    }
#ifdef HAS_BOXEE_HAL
    else if (strSetting.Equals("network.assignment"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(787), ADDR_NONE);
      pControl->AddLabel(g_localizeStrings.Get(717), ADDR_STATIC);
      pControl->AddLabel(g_localizeStrings.Get(716), ADDR_DYNAMIC);
      pControl->SetValue(pSettingInt->GetData());
    }
#endif
    else if (strSetting.Equals("subtitles.style"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(738), FONT_STYLE_NORMAL);
      pControl->AddLabel(g_localizeStrings.Get(739), FONT_STYLE_BOLD);
      pControl->AddLabel(g_localizeStrings.Get(740), FONT_STYLE_ITALICS);
      pControl->AddLabel(g_localizeStrings.Get(741), FONT_STYLE_BOLD_ITALICS);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("subtitles.color"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      for (int i = SUBTITLE_COLOR_START; i <= SUBTITLE_COLOR_END; i++)
        pControl->AddLabel(g_localizeStrings.Get(760 + i), i);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("karaoke.fontcolors"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      for (int i = KARAOKE_COLOR_START; i <= KARAOKE_COLOR_END; i++)
        pControl->AddLabel(g_localizeStrings.Get(22040 + i), i);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("subtitles.height") || strSetting.Equals("karaoke.fontheight") )
    {
      FillInSubtitleHeights(pSetting);
    }
    else if (strSetting.Equals("subtitles.font") || strSetting.Equals("karaoke.font") )
    {
      FillInSubtitleFonts(pSetting);
    }
    else if (strSetting.Equals("subtitles.charset") || strSetting.Equals("locale.charset") || strSetting.Equals("karaoke.charset"))
    {
      FillInCharSets(pSetting);
    }
    else if (strSetting.Equals("subtitles.preferredlanguage") || strSetting.Equals("audioplayback.preferredlanguage"))
    {
      FillInPrefferedLanguage(pSetting, strSetting);
    }
    else if (strSetting.Equals("lookandfeel.font"))
    {
      FillInSkinFonts(pSetting);
    }
    else if (strSetting.Equals("lookandfeel.skin"))
    {
      FillInSkins(pSetting);
    }
    else if (strSetting.Equals("lookandfeel.soundskin"))
    {
      FillInSoundSkins(pSetting);
    }
    else if (strSetting.Equals("locale.language"))
    {
      FillInLanguages(pSetting);
    }
    else if (strSetting.Equals("locale.keyboard1"))
    {
      FillInKeyboards(pSetting,1);
    }
    else if (strSetting.Equals("locale.keyboard2"))
    {
      FillInKeyboards(pSetting,2);
    }
    else if (strSetting.Equals("locale.keyboard3"))
    {
      FillInKeyboards(pSetting,3);
    }
#ifdef _LINUX    
    else if (strSetting.Equals("locale.timezonecountry"))
    {
      CStdString myTimezoneCountry = g_timezone.GetOSConfiguredTimezone();
      int myTimezeoneCountryIndex = -1;
      
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      vector<CStdString> countries = g_timezone.GetCounties();
      size_t i=0;
      for (i=0; i < countries.size(); i++)
      {
        if (countries[i] == myTimezoneCountry)
           myTimezeoneCountryIndex = i;
        pControl->AddLabel(countries[i], i);
      }
      
      // myTimezoneCountry not always in countries.
      if (myTimezeoneCountryIndex == -1) {
        pControl->AddLabel(myTimezoneCountry, i);      
        pControl->SetValue(i);
      } else {
      pControl->SetValue(myTimezeoneCountryIndex);    
    }
    }
    else if (strSetting.Equals("locale.timezone"))
    {
      CStdString myTimezoneCountry = g_guiSettings.GetString("locale.timezonecountry");
      CStdString myTimezone = g_guiSettings.GetString("locale.timezone");
      int myTimezoneIndex = 0;
      
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->Clear();
      vector<CStdString> timezones = g_timezone.GetTimezonesByCountry(myTimezoneCountry);
      for (unsigned int i=0; i < timezones.size(); i++)
      {
        if (timezones[i] == myTimezone)
           myTimezoneIndex = i;
        pControl->AddLabel(timezones[i], i);
      }
      pControl->SetValue(myTimezoneIndex);   
    }    
#endif    
    else if (strSetting.Equals("videoscreen.resolution"))
    {
      FillInResolutions(pSetting, false);
    }
    else if (strSetting.Equals("videoscreen.vsync"))
    {
      FillInVSyncs(pSetting);
    }
    else if(strSetting.Equals("videoscreen.mode3d"))
    {
      FillInMode3D(pSetting);
    }
    else if(strSetting.Equals("videoplayback.interlacemode"))
    {
      FillInDeinterlacingPolicy(pSetting);
    }
    else if (strSetting.Equals("lookandfeel.skintheme"))
    {
      FillInSkinThemes(pSetting);
    }
    else if (strSetting.Equals("lookandfeel.skincolors"))
    {
      FillInSkinColors(pSetting);
    }
    else if (strSetting.Equals("menu.showsdefault"))
    {
      FillInShowsDefault(pSetting);
    }
    else if (strSetting.Equals("menu.moviesdefault"))
    {
      FillInMoviesDefault(pSetting);
    }
    else if (strSetting.Equals("menu.appsdefault"))
    {
      FillInAppsDefault(pSetting);
    }
    else if (strSetting.Equals("screensaver.mode"))
    {
      FillInScreenSavers(pSetting);
    }
    else if (strSetting.Equals("sort.showstarter"))
    {
      SetShowStarter(pSetting);
    }
    else if (strSetting.Equals("videoplayer.displayresolution") || strSetting.Equals("pictures.displayresolution"))
    {
      FillInResolutions(pSetting, true);
    }
    else if (strSetting.Equals("videoplayer.highqualityupscaling"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(13113), SOFTWARE_UPSCALING_DISABLED);
      pControl->AddLabel(g_localizeStrings.Get(13114), SOFTWARE_UPSCALING_SD_CONTENT);
      pControl->AddLabel(g_localizeStrings.Get(13115), SOFTWARE_UPSCALING_ALWAYS);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("videoplayer.upscalingalgorithm"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(13117), VS_SCALINGMETHOD_BICUBIC_SOFTWARE);
      pControl->AddLabel(g_localizeStrings.Get(13118), VS_SCALINGMETHOD_LANCZOS_SOFTWARE);
      pControl->AddLabel(g_localizeStrings.Get(13119), VS_SCALINGMETHOD_SINC_SOFTWARE);
      pControl->AddLabel(g_localizeStrings.Get(13120), VS_SCALINGMETHOD_VDPAU_HARDWARE);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("videolibrary.flattentvshows"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(20420), 0); // Never
      pControl->AddLabel(g_localizeStrings.Get(20421), 1); // One Season
      pControl->AddLabel(g_localizeStrings.Get(20422), 2); // Always
      pControl->SetValue(pSettingInt->GetData());
    }
#if defined (__APPLE__) || defined (_WIN32)
    else if (strSetting.Equals("videoscreen.displayblanking"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(13131), BLANKING_DISABLED);
      pControl->AddLabel(g_localizeStrings.Get(13132), BLANKING_ALL_DISPLAYS);
      pControl->SetValue(pSettingInt->GetData());
    }
#endif
#ifdef __APPLE__
    else if (strSetting.Equals("appleremote.mode"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(13610), APPLE_REMOTE_DISABLED);
      pControl->AddLabel(g_localizeStrings.Get(13611), APPLE_REMOTE_STANDARD);
      pControl->AddLabel(g_localizeStrings.Get(13612), APPLE_REMOTE_UNIVERSAL);
      pControl->AddLabel(g_localizeStrings.Get(13613), APPLE_REMOTE_MULTIREMOTE);
      pControl->SetValue(pSettingInt->GetData());
    }
#endif
    else if (strSetting.Equals("system.shutdownstate"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      if (!g_application.IsStandAlone())
      {
        pControl->AddLabel(g_localizeStrings.Get(13009), POWERSTATE_QUIT);
        pControl->AddLabel(g_localizeStrings.Get(13014), POWERSTATE_MINIMIZE);
      }

      if (g_powerManager.CanPowerdown())
      pControl->AddLabel(g_localizeStrings.Get(13005), POWERSTATE_SHUTDOWN);

      if (g_powerManager.CanHibernate())
      pControl->AddLabel(g_localizeStrings.Get(13010), POWERSTATE_HIBERNATE);

      if (g_powerManager.CanSuspend())
      pControl->AddLabel(g_localizeStrings.Get(13011), POWERSTATE_SUSPEND);

      pControl->SetValue(pSettingInt->GetData());
    }
#if defined(_LINUX) && !defined(__APPLE__)
    else if (strSetting.Equals("system.powerbuttonaction"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      
      pControl->AddLabel(g_localizeStrings.Get(231), POWERSTATE_NONE);
      pControl->AddLabel(g_localizeStrings.Get(12020), POWERSTATE_ASK);

      if (g_powerManager.CanPowerdown())
        pControl->AddLabel(g_localizeStrings.Get(13005), POWERSTATE_SHUTDOWN);

      if (g_powerManager.CanHibernate())
        pControl->AddLabel(g_localizeStrings.Get(13010), POWERSTATE_HIBERNATE);

      if (g_powerManager.CanSuspend())
        pControl->AddLabel(g_localizeStrings.Get(13011), POWERSTATE_SUSPEND);

      pControl->SetValue(pSettingInt->GetData());
    }
#endif
    else if (strSetting.Equals("system.ledcolour"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(13340), LED_COLOUR_NO_CHANGE);
      pControl->AddLabel(g_localizeStrings.Get(13341), LED_COLOUR_GREEN);
      pControl->AddLabel(g_localizeStrings.Get(13342), LED_COLOUR_ORANGE);
      pControl->AddLabel(g_localizeStrings.Get(13343), LED_COLOUR_RED);
      pControl->AddLabel(g_localizeStrings.Get(13344), LED_COLOUR_CYCLE);
      pControl->AddLabel(g_localizeStrings.Get(351), LED_COLOUR_OFF);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("system.leddisableonplayback") || strSetting.Equals("lcd.disableonplayback"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(106), LED_PLAYBACK_OFF);     // No
      pControl->AddLabel(g_localizeStrings.Get(13002), LED_PLAYBACK_VIDEO);   // Video Only
      pControl->AddLabel(g_localizeStrings.Get(475), LED_PLAYBACK_MUSIC);    // Music Only
      pControl->AddLabel(g_localizeStrings.Get(476), LED_PLAYBACK_VIDEO_MUSIC); // Video & Music
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("videoplayer.rendermethod"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
#ifdef HAS_XBOX_D3D
      pControl->AddLabel(g_localizeStrings.Get(13355), RENDER_LQ_RGB_SHADER);
      pControl->AddLabel(g_localizeStrings.Get(13356), RENDER_OVERLAYS);
      pControl->AddLabel(g_localizeStrings.Get(13357), RENDER_HQ_RGB_SHADER);
      pControl->AddLabel(g_localizeStrings.Get(21397), RENDER_HQ_RGB_SHADERV2);
#else
      pControl->AddLabel(g_localizeStrings.Get(13416), RENDER_METHOD_AUTO);
      pControl->AddLabel(g_localizeStrings.Get(13417), RENDER_METHOD_ARB);
      pControl->AddLabel(g_localizeStrings.Get(13418), RENDER_METHOD_GLSL);
      pControl->AddLabel(g_localizeStrings.Get(13419), RENDER_METHOD_SOFTWARE);
#endif
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("musicplayer.replaygaintype"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(351), REPLAY_GAIN_NONE);
      pControl->AddLabel(g_localizeStrings.Get(639), REPLAY_GAIN_TRACK);
      pControl->AddLabel(g_localizeStrings.Get(640), REPLAY_GAIN_ALBUM);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("lookandfeel.startupwindow"))
    {
      FillInStartupWindow(pSetting);
    }
    else if (strSetting.Equals("servers.ftpserveruser"))
    {
      FillInFTPServerUser(pSetting);
    }
    else if (strSetting.Equals("videoplayer.externaldvdplayer"))
    {
      CSettingString *pSettingString = (CSettingString *)pSetting;
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      if (pSettingString->GetData().IsEmpty())
        pControl->SetLabel2(g_localizeStrings.Get(20009));
    }
    else if (strSetting.Equals("locale.country"))
    {
      FillInRegions(pSetting);
    }
    else if (strSetting.Equals("locale.timeformat"))
    {
      FillInTimeFormat(pSetting);
    }
    else if (strSetting.Equals("locale.tempscale"))
    {
      FillInTempScale(pSetting);
    }
    else if (strSetting.Equals("network.interface"))
    {
       FillInNetworkInterfaces(pSetting);
    }
    else if (strSetting.Equals("audiooutput.audiodevice"))
    {
      FillInAudioDevices(pSetting);
    }
    else if (strSetting.Equals("audiooutput.passthroughdevice"))
    {
      FillInAudioDevices(pSetting,true);
    }
    else if (strSetting.Equals("audiooutput.controlmastervolume"))
    {
      CGUIRadioButtonControl *pControl = (CGUIRadioButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetSelected(g_guiSettings.GetBool("audiooutput.controlmastervolume"));
    }
    else if (strSetting.Equals("myvideos.resumeautomatically"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(106), RESUME_NO);
      pControl->AddLabel(g_localizeStrings.Get(107), RESUME_YES);
      pControl->AddLabel(g_localizeStrings.Get(12020), RESUME_ASK);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("videoplayer.synctype"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(13501), SYNC_DISCON);
      pControl->AddLabel(g_localizeStrings.Get(13502), SYNC_SKIPDUP);
      pControl->AddLabel(g_localizeStrings.Get(13503), SYNC_RESAMPLE);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("videoplayer.resamplequality"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(13506), RESAMPLE_LOW);
      pControl->AddLabel(g_localizeStrings.Get(13507), RESAMPLE_MID);
      pControl->AddLabel(g_localizeStrings.Get(13508), RESAMPLE_HIGH);
      pControl->AddLabel(g_localizeStrings.Get(13509), RESAMPLE_REALLYHIGH);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("weather.plugin"))
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      FillInWeatherPlugins(pControl, g_guiSettings.GetString("weather.plugin"));
    }
#ifdef HAS_EMBEDDED
    else if (strSetting.Equals("timezone.country"))
    {
      FillInTimezoneCountries(pSetting);
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      m_tzChosenCountryName = pControl->GetCurrentLabel();
    }
    else if (strSetting.Equals("timezone.city"))
    {
      FillInTimezoneCities(pSetting);

      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      if (pControl)
        pControl->SetEnabled(pControl->GetMaximum() > 1);
    }
    else if (strSetting.Equals("videoscreen.hdmioutput"))
    {
      FillInHDMIOutput(pSetting);
    }
    else if (strSetting.Equals("videoscreen.hdmipixeldepth"))
    {
      FillInHDMIPixelDepth(pSetting);
    }
    else if (strSetting.Equals("services.netflixplaypause"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(57200), NRDPP_DEFAULT);
      pControl->AddLabel(g_localizeStrings.Get(54867), NRDPP_NETFLIX);
      pControl->AddLabel(g_localizeStrings.Get(54868), NRDPP_PLAYPAUSE);
      pControl->SetValue(pSettingInt->GetData());
    }
#endif
    else if (strSetting.Equals("debug.syslogaddr"))
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      if (pControl)
        pControl->SetEnabled(g_guiSettings.GetBool("debug.syslogenabled"));
    }
    else if (strSetting.Equals("server.environment"))
    {
      CGUIButtonControl* pControl = (CGUIButtonControl *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      pControl->SetLabel2(g_guiSettings.GetString("server.environment").c_str());
    }
  }

  if (m_vecSections[m_iSection]->m_strCategory == "network")
  {
     NetworkInterfaceChanged();
  }
   
  // update our settings (turns controls on/off as appropriate)
  UpdateSettings();
}

void CGUIWindowSettingsCategory::UpdateKeyboard(int nKeyboard, CBaseSettingControl *pSettingControl)
{
  //
  // int order to stay consistent, not have duplicates and always have the English keyboard in place,
  // we first uninstall all keyboards, then set the new values again + English, re-add them and save.
  //
  std::vector<XBMC::Keyboard> keyboards;
  g_application.GetKeyboards().GetUserKeyboards(keyboards);
  
  for (size_t k=0; k<keyboards.size(); k++)
    g_application.GetKeyboards().UnInstallKeyboard(keyboards[k].GetLangPath());
  
  // make sure we have enough room to work directly on the required keyboard index
  while ((int)keyboards.size() <= nKeyboard)
    keyboards.push_back(XBMC::Keyboard());
    
  CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());

  if(m_availableLanguages.IsEmpty())
  {
    BoxeeUtils::GetAvailableLanguages(m_availableLanguages);
  }
  CFileItemPtr currLabelFile = m_availableLanguages.Get("lang_displayName",pControl->GetCurrentLabel());
  CStdString strLanguage = "";
  if (currLabelFile)
  {
    strLanguage = currLabelFile->GetProperty("lang_dirName");
  }
  if (pControl->GetValue() == 0) // None
    keyboards[nKeyboard] = XBMC::Keyboard(); // empty
  else if (strLanguage != ".svn" && strLanguage != pSettingString->GetData())
    keyboards[nKeyboard].Load(strLanguage);

  int nAdditionalKeyboards = 0;
  for (size_t k2=0; nAdditionalKeyboards <= 3 && k2<keyboards.size(); k2++)
  {
    XBMC::Keyboard &kb = keyboards[k2];
    if (!kb.GetLangPath().IsEmpty())
    {
      nAdditionalKeyboards++;
      g_application.GetKeyboards().InstallKeyboard(kb.GetLangPath());
    }
  }
  
  g_application.GetKeyboards().Save();  
}

void CGUIWindowSettingsCategory::UpdateSettings()
{
  for (unsigned int i = 0; i < m_vecSettings.size(); i++)
  {
    CBaseSettingControl *pSettingControl = m_vecSettings[i];
    pSettingControl->Update();
    CStdString strSetting = pSettingControl->GetSetting()->GetSetting();
    if (strSetting.Equals("videoscreen.testresolution"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        if (m_NewResolution == RES_INVALID || m_NewResolution == g_graphicsContext.GetVideoResolution())
          pControl->SetEnabled(false);
        else
          pControl->SetEnabled(true);
      }
    }
#ifdef HAS_EMBEDDED
    else if (strSetting.Equals("update.status"))
    {
      CBoxeeVersionUpdateJob versionUpdateJob = g_boxeeVersionUpdateManager.GetBoxeeVerUpdateJob();
      VERSION_UPDATE_JOB_STATUS verUpdateJobStatus = versionUpdateJob.GetVersionUpdateJobStatus();
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      CStdString label2;

      m_iUpdateSection = m_iSection;

      //
      // new version is ready
      //
      if(versionUpdateJob.acquireVersionUpdateJobForPerformUpdate())
      {
         CLog::Log(LOGDEBUG, "CGUIWindowSettingsCategory::UpdateSettings: New update is ready for installation [%s]", strSetting.c_str());
         pControl->SetEnabled(true);

         m_strUpdateStatus = g_localizeStrings.Get(53261);
      }
      else
      { 
        CLog::Log(LOGDEBUG, "CGUIWindowSettingsCategory::UpdateSettings: New update is not avaliable yet, job status is %s", versionUpdateJob.GetVersionUpdateJobStatusAsString(verUpdateJobStatus).c_str());

        pControl->SetEnabled(false);

        if(verUpdateJobStatus == VUJS_IDLE)
        { 
          if(m_strUpdateStatus == g_localizeStrings.Get(53252) || m_strUpdateStatus.empty())
          {
            m_strUpdateStatus = g_localizeStrings.Get(53252);

            CStdString strLastChecked = g_boxeeVersionUpdateManager.GetLastCheckedTime();
            if(!strLastChecked.empty())
            {
              pControl->SetLabel2(strLastChecked);
            }
            else
            {
              pControl->SetLabel2("");
            }
          }
        }
      }

      pControl->SetLabel(m_strUpdateStatus);
      
    }
    else if (strSetting.Equals("update.check.for.update"))
    {
      CBoxeeVersionUpdateJob versionUpdateJob = g_boxeeVersionUpdateManager.GetBoxeeVerUpdateJob();
      VERSION_UPDATE_JOB_STATUS verUpdateJobStatus = versionUpdateJob.GetVersionUpdateJobStatus();
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());

      //
      // new version is ready
      //
      if(versionUpdateJob.acquireVersionUpdateJobForPerformUpdate())
      {
         CLog::Log(LOGDEBUG, "CGUIWindowSettingsCategory::UpdateSettings: New update is ready for installation [%s]", strSetting.c_str());
         pControl->SetEnabled(false);
      }
      else
      {
        
        if(verUpdateJobStatus > VUJS_IDLE)
        {
          m_bDownloadingUpdate = true;
          pControl->SetEnabled(false);

          CLog::Log(LOGDEBUG, "CGUIWindowSettingsCategory::UpdateSettings - Update job is downloading update, status %s\n", versionUpdateJob.GetVersionUpdateJobStatusAsString(verUpdateJobStatus).c_str()); 
        }
        else
        {
          m_bDownloadingUpdate = false;
          pControl->SetEnabled(true);

          CLog::Log(LOGDEBUG, "CGUIWindowSettingsCategory::UpdateSettings - Update job is idle" );
        }
      }
    }
    else if (strSetting.Equals("videoscreen.hiresrendering"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled((g_graphicsContext.GetHeight() > 720));
    }
    else if (strSetting.Equals("smbd.password") || strSetting.Equals("smbd.workgroup"))
    {
      CGUIEditControl *pControl = (CGUIEditControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        pControl->SetEnabled(g_guiSettings.GetBool("smbd.enable"));
      }
    }
    else if (strSetting.Equals("vpn.operation"))
    {
      CGUIEditControl *pControl = (CGUIEditControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        pControl->SetEnabled(
            g_application.IsConnectedToInternet() &&
            g_guiSettings.GetString("vpn.server").length() > 0 &&
            g_guiSettings.GetString("vpn.account").length() > 0 &&
            g_guiSettings.GetString("vpn.password").length() > 0
        );
      }

      if (g_halListener.GetVpnConnected())
        pControl->SetLabel(g_localizeStrings.Get(54744));
      else
        pControl->SetLabel(g_localizeStrings.Get(54743));
    }
    else if (strSetting.Equals("timezone.country"))
    {
      // nothing to do
    }
    else if (strSetting.Equals("timezone.city"))
    {
      if (m_needToLoadTimezoneCitiesControl)
      {
        FillInTimezoneCities(pSettingControl->GetSetting());

        CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
        if (pControl)
        {
          pControl->SetEnabled(pControl->GetMaximum() > 1);
        }

        m_tzChosenCityName = pControl->GetCurrentLabel();
        m_needToLoadTimezoneCitiesControl = false;
      }
    }
#endif
    else if (strSetting.Equals("services.netflixesn"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(false);

      FILE* fp = fopen("/data/netflix/esn", "r");
      if (fp)
      {
        char esnSz[256];
        fgets(esnSz, sizeof(esnSz), fp);
        fclose(fp);
        pControl->SetLabel2(esnSz);
      }
      else
      {
        pControl->SetLabel2(g_localizeStrings.Get(13205));
      }
    }
    else if (strSetting.Equals("videoplayer.rendermethod"))
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        pControl->SetEnabled(!g_guiSettings.GetBool("videoplayer.hwaccel"));
      }
      CSettingInt *pSettingInt = (CSettingInt*)pSettingControl->GetSetting();
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("videoplayer.upscalingalgorithm"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        int value = g_guiSettings.GetInt("videoplayer.highqualityupscaling");
        
        if (value == SOFTWARE_UPSCALING_DISABLED)
          pControl->SetEnabled(false);
        else
          pControl->SetEnabled(true);
      }
    }
    else if (strSetting.Equals("debug.loglevel"))
    {
      CLog::m_logLevel = g_guiSettings.GetInt("debug.loglevel"); 
      CLog::m_showLogLine = (CLog::m_logLevel == LOGDEBUG);
    }
#ifdef HAVE_LIBVDPAU
    else if (strSetting.Equals("videoplayer.vdpauUpscalingLevel"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        int value1 = g_guiSettings.GetInt("videoplayer.upscalingalgorithm");
        int value2 = g_guiSettings.GetInt("videoplayer.highqualityupscaling");

        if (value1 == VS_SCALINGMETHOD_VDPAU_HARDWARE && value2 != SOFTWARE_UPSCALING_DISABLED)
          pControl->SetEnabled(true);
        else
          pControl->SetEnabled(false);
      }
    }
#endif
#if defined(__APPLE__) || defined(_WIN32)
    else if (strSetting.Equals("videoscreen.displayblanking"))
    {
      CGUISpinControlEx* pSpinControl = (CGUISpinControlEx*)GetControl(pSettingControl->GetID());
      if (pSpinControl)
      {
        int value = g_guiSettings.GetInt("videoscreen.resolution");
        
        // sanity. res can be a monitor that is no longer connected
        if (value >= (int)g_settings.m_ResInfo.size())
          value = RES_DESKTOP;
        
        // Note that actually enabling screen blanking requires a call to grahiccontext->SetVideoResolution,
        // which requires an acknowledge dialog. We should probably pair screen blanking with resolution changes,
        // trigger the test button, and clear the blanking if the user moves to Windowed mode
        if( -1 != g_settings.m_ResInfo[value].strMode.Find("Full Screen"))
        {
          // On W32 this string is "Full Screen" if we are not windowed
          // On Mac this string is "XRes x YRes - Full Screen" if we are not windowed.
          // This can also be "XRes x YRes - Full Screen #Z" in multimon setups on mac.
          pSpinControl->SetEnabled(true);
        }
        else
        {
          pSpinControl->SetEnabled(false);
      }
    }
    }
#endif
#ifdef __APPLE__
    else if (strSetting.Equals("appleremote.mode"))
    {
      bool cancelled;
      int remoteMode = g_guiSettings.GetInt("appleremote.mode");

      // if it's not disabled, start the event server or else apple remote won't work
      if ( remoteMode != APPLE_REMOTE_DISABLED )
      {
        g_guiSettings.SetBool("remoteevents.enabled", true);
        g_application.StartEventServer();
      }

      // if XBMC helper is running, prompt user before effecting change
      if ( g_xbmcHelper.IsRunning() && g_xbmcHelper.GetMode()!=remoteMode )
      {
        if (!CGUIDialogYesNo2::ShowAndGetInput(13144, 13145, -1, -1, cancelled, 10000))
        {
          // user declined, restore previous spinner state and appleremote mode
          CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
          g_guiSettings.SetInt("appleremote.mode", g_xbmcHelper.GetMode());
          pControl->SetValue(g_xbmcHelper.GetMode());
        }
        else
        {
          // reload configuration
          g_xbmcHelper.Configure();      
        }
      }
      else
      {
        // set new configuration.
        g_xbmcHelper.Configure();      
      }

      if (g_xbmcHelper.ErrorStarting() == true)
      {
        // inform user about error
        CGUIDialogOK::ShowAndGetInput(13620, 13621, 20022, 20022);

        // reset spinner to disabled state
        CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
        pControl->SetValue(APPLE_REMOTE_DISABLED);
      }
    }
    else if (strSetting.Equals("appleremote.alwayson"))
     {
       CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
       if (pControl)
       {
         int value = g_guiSettings.GetInt("appleremote.mode");
         if (value != APPLE_REMOTE_DISABLED)
           pControl->SetEnabled(true);
         else
           pControl->SetEnabled(false);
       }
     }
     else if (strSetting.Equals("appleremote.sequencetime"))
     {
       CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
       if (pControl)
       {
         int value = g_guiSettings.GetInt("appleremote.mode");
         if (value == APPLE_REMOTE_UNIVERSAL)
           pControl->SetEnabled(true);
         else
           pControl->SetEnabled(false);
       }
     }
     else if (strSetting.Equals("boxee.runatlogin"))
     {
       g_xbmcHelper.ConfigureStartup();
     }
#endif
#ifdef _WIN32
	 if (strSetting.Equals("boxee.irssremote"))
     {
       CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
       if (pControl)
       {
          bool cancelled;

	      // Check if the setting was changed and present the dialog
		  bool bCurrentSetting = g_guiSettings.GetBool("boxee.irssremote");
		  
		  if (bCurrentSetting != m_bBoxeeRemoteEnabled) 
		  {
            // Selection changed, notify the user
		    if (CGUIDialogYesNo::ShowAndGetInput(13148, 13149, 0, 0, -1, -1, cancelled, 10000))
			{
              m_bBoxeeRemoteEnabled = bCurrentSetting;
			}
			else
			{
  			  // Restore previous setting
			  g_guiSettings.SetBool("boxee.irssremote", m_bBoxeeRemoteEnabled);
			  
			}
			((CGUIRadioButtonControl*)pControl)->SetSelected(m_bBoxeeRemoteEnabled);
		  }
       }
     }
   else if (strSetting.Equals("boxee.runatlogin"))
   {
     CWIN32Util::RunAtLogin(g_guiSettings.GetBool("boxee.runatlogin"));
   }
#endif
    else if (strSetting.Equals("filelists.allowfiledeletion"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(!g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].filesLocked() || g_passwordManager.bMasterUser);
    }
    else if (strSetting.Equals("filelists.disableaddsourcebuttons"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteSources() || g_passwordManager.bMasterUser);
    }
    else if (strSetting.Equals("masterlock.startuplock") || strSetting.Equals("masterlock.enableshutdown") || strSetting.Equals("masterlock.automastermode"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE);
    }
    else if (strSetting.Equals("masterlock.loginlock"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && g_settings.bUseLoginScreen);
    }
    else if (strSetting.Equals("screensaver.uselock"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE                                    &&
                                         g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].getLockMode() != LOCK_MODE_EVERYONE &&
                                         !g_guiSettings.GetString("screensaver.mode").Equals("Black"));
    }
    else if (strSetting.Equals("upnp.musicshares") || strSetting.Equals("upnp.videoshares") || strSetting.Equals("upnp.pictureshares"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("upnp.server"));
    }
    else if (!strSetting.Equals("remoteevents.enabled")
             && strSetting.Left(13).Equals("remoteevents."))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("remoteevents.enabled"));
    }
    else if (strSetting.Equals("mymusic.clearplaylistsonend"))
    { // disable repeat and repeat one if clear playlists is enabled
      if (g_guiSettings.GetBool("mymusic.clearplaylistsonend"))
      {
        g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC, PLAYLIST::REPEAT_NONE);
        g_stSettings.m_bMyMusicPlaylistRepeat = false;
        g_settings.Save();
      }
    }
    else if (strSetting.Equals("cddaripper.quality"))
    { // only visible if we are doing non-WAV ripping
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("cddaripper.encoder") != CDDARIP_ENCODER_WAV);
    }
    else if (strSetting.Equals("cddaripper.bitrate"))
    { // only visible if we are ripping to CBR
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled((g_guiSettings.GetInt("cddaripper.encoder") != CDDARIP_ENCODER_WAV) &&
                                           (g_guiSettings.GetInt("cddaripper.quality") == CDDARIP_QUALITY_CBR));
    }
    else if (strSetting.Equals("audiooutput.ac3passthrough")     ||
             strSetting.Equals("audiooutput.dtspassthrough")     ||
             strSetting.Equals("audiooutput.passthroughdevice")  || 
             strSetting.Equals("audiooutput.truehdpassthrough")  ||
             strSetting.Equals("audiooutput.dtshdpassthrough")   ||
             strSetting.Equals("audiooutput.lpcm71passthrough")  ||
             strSetting.Equals("audiooutput.eac3passthrough"))
    { // only visible if we are in digital mode
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        if( g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL_HDMI )
        {
          pControl->SetEnabled( true );
        }
        else if( g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL_SPDIF )
        {
          pControl->SetEnabled( (strSetting.Equals("audiooutput.ac3passthrough") ||
                                 strSetting.Equals("audiooutput.dtspassthrough")) );
        }
        else
          pControl->SetEnabled( false );
      }
#if defined(_LINUX) && !defined(__APPLE__)
      if (g_guiSettings.GetInt("audiooutput.library") == AUDIO_LIBRARY_PULSEAUDIO && pControl)
        pControl->SetEnabled(false);
#endif
    }
    else if(strSetting.Equals("audiooutput.dd_truehd_drc_percentage"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        if(g_guiSettings.GetInt("audiooutput.dd_truehd_drc") == DD_TRUEHD_DRC_ON)
          pControl->SetEnabled( true );
        else
          pControl->SetEnabled(false);
      }
    }
    else if(strSetting.Equals("audiooutput.enable_audio_output_delay"))
    {
#ifdef HAS_INTEL_SMD
      g_IntelSMDGlobals.BuildAudioOutputs();
#endif
    }
    else if (strSetting.Equals("videoscreen.mode3d"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("videoscreen.tv3dfc"));
    }
    else if (strSetting.Equals("audiooutput.audiodevice") || strSetting.Equals("audiooutput.mode"))
    {
      // only visible if we are in digital mode
#if defined(_LINUX) && !defined(__APPLE__)
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("audiooutput.library") == AUDIO_LIBRARY_ALSA);
#endif
    }
    else if (strSetting.Equals("musicplayer.crossfadealbumtracks"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("musicplayer.crossfade") > 0);
    }
    else if (strSetting.Left(12).Equals("karaoke.port") || strSetting.Equals("karaoke.volume"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("karaoke.voiceenabled"));
    }
    else if (strSetting.Equals("system.fanspeed"))
    { // only visible if we have fancontrolspeed enabled
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("system.fanspeedcontrol"));
    }
    else if (strSetting.Equals("system.targettemperature") || strSetting.Equals("system.minfanspeed"))
    { // only visible if we have autotemperature enabled
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("system.autotemperature"));
    }
    else if (strSetting.Equals("servers.ftpserveruser") || strSetting.Equals("servers.ftpserverpassword"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("servers.ftpserver"));
    }
    else if (strSetting.Equals("servers.webserverusername"))
    {
      CGUIEditControl *pControl = (CGUIEditControl *)GetControl(pSettingControl->GetID());
      if (pControl)
        pControl->SetEnabled(g_guiSettings.GetBool("servers.webserver"));
    }
    else if (strSetting.Equals("servers.webserverpassword"))
    {
      CGUIEditControl *pControl = (CGUIEditControl *)GetControl(pSettingControl->GetID());
      if (pControl)
        pControl->SetEnabled(g_guiSettings.GetBool("servers.webserver"));
    }
    else if (strSetting.Equals("servers.webserverport"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("servers.webserver"));
    }
#ifdef HAS_BOXEE_HAL
    else if (strSetting.Equals("network.ipaddress") || strSetting.Equals("network.subnet") || strSetting.Equals("network.gateway") || strSetting.Equals("network.dns"))
    {
      bool enabled = false;
      CGUISpinControlEx* pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.assignment")->GetID());
      if (pControl1) 
        enabled = (pControl1->GetValue() == ADDR_STATIC);

      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl)
        pControl->SetEnabled(enabled);
    }
    else if (strSetting.Equals("network.assignment"))
    {
      CGUISpinControlEx* pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.assignment")->GetID());
      if (pControl1)
         pControl1->SetEnabled(true);
    }
    else if (strSetting.Equals("network.essid"))
    {
      // Get network information      
      CGUISpinControlEx *ifaceControl = (CGUISpinControlEx *)GetControl(GetSetting("network.interface")->GetID());
      int ifaceId = ifaceControl->GetValue();
      bool enabled = (ifaceId == WIRELESS_INTERFACE_ID);
               
      CGUISpinControlEx* pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.assignment")->GetID());
      if (pControl1) 
        enabled &= (pControl1->GetValue() != ADDR_NONE);

      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(enabled);
    }
#endif
    else if (strSetting.Equals("network.httpproxyserver")   || strSetting.Equals("network.httpproxyport") ||
             strSetting.Equals("network.httpproxyusername") || strSetting.Equals("network.httpproxypassword"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("network.usehttpproxy"));
      g_application.SetupHttpProxy();
    }
    else if (strSetting.Equals("network.save"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(true);
    }
    else if (strSetting.Equals("scrobbler.lastfmusername") || strSetting.Equals("scrobbler.lastfmpassword"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      if (pControl)
        pControl->SetEnabled(g_guiSettings.GetBool("scrobbler.lastfmsubmit") | g_guiSettings.GetBool("scrobbler.lastfmsubmitradio"));
    }
    else if (strSetting.Equals("scrobbler.librefmusername") || strSetting.Equals("scrobbler.librefmpassword"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("scrobbler.librefmsubmit"));
    }
    else if (strSetting.Equals("postprocessing.verticaldeblocklevel"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("postprocessing.verticaldeblocking") &&
                           g_guiSettings.GetBool("postprocessing.enable") &&
                           !g_guiSettings.GetBool("postprocessing.auto"));
    }
    else if (strSetting.Equals("postprocessing.horizontaldeblocklevel"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("postprocessing.horizontaldeblocking") &&
                           g_guiSettings.GetBool("postprocessing.enable") &&
                           !g_guiSettings.GetBool("postprocessing.auto"));
    }
    else if (strSetting.Equals("postprocessing.verticaldeblocking") || strSetting.Equals("postprocessing.horizontaldeblocking") || strSetting.Equals("postprocessing.autobrightnesscontrastlevels") || strSetting.Equals("postprocessing.dering"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("postprocessing.enable") &&
                           !g_guiSettings.GetBool("postprocessing.auto"));
    }
    else if (strSetting.Equals("postprocessing.auto"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("postprocessing.enable"));
    }
    else if (strSetting.Equals("VideoPlayer.InvertFieldSync"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("VideoPlayer.FieldSync"));
    }
    else if (strSetting.Equals("subtitles.color") || strSetting.Equals("subtitles.style") || strSetting.Equals("subtitles.charset"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(CUtil::IsUsingTTFSubtitles());
    }
    else if (strSetting.Equals("locale.charset"))
    { // TODO: Determine whether we are using a TTF font or not.
      //   CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      //   if (pControl) pControl->SetEnabled(g_guiSettings.GetString("lookandfeel.font").Right(4) == ".ttf");
    }
    else if (strSetting.Equals("screensaver.dimlevel"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("screensaver.mode") == "Dim");
    }
    else if (strSetting.Equals("screensaver.slideshowpath"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("screensaver.mode") == "SlideShow");
    }
    else if (strSetting.Equals("screensaver.slideshowshuffle"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("screensaver.mode") == "SlideShow" ||
                           g_guiSettings.GetString("screensaver.mode") == "Fanart Slideshow");
    }
    else if (strSetting.Equals("screensaver.preview")           ||
             strSetting.Equals("screensaver.usedimonpause")     ||
             strSetting.Equals("screensaver.usemusicvisinstead"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("screensaver.mode") != "None");
      if (strSetting.Equals("screensaver.usedimonpause") && g_guiSettings.GetString("screensaver.mode").Equals("Dim"))
        pControl->SetEnabled(false);
    }
    /*else if (strSetting.Equals("background.remark"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(false);
    }*/
    else if (strSetting.Equals("version.version"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(false);
      pControl->SetLabel2(g_infoManager.GetLabel(SYSTEM_BUILD_VERSION).c_str());
    }
    else if (strSetting.Equals("version.builddate"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(false);
      pControl->SetLabel2(g_infoManager.GetLabel(SYSTEM_BUILD_DATE).c_str());
    }
    else if (strSetting.Equals("version.ipaddress"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
#ifdef HAS_EMBEDDED
      pControl->SetEnabled(false);
      CHalEthernetInfo ethInfo;
      CHalWirelessInfo wirelessInfo;
      if (CHalServicesFactory::GetInstance().GetEthernetInfo(0, ethInfo) && ethInfo.ip_address != "0.0.0.0")
      {
        pControl->SetLabel2(ethInfo.ip_address.c_str());
      }
      else if (CHalServicesFactory::GetInstance().GetWirelessInfo(0, wirelessInfo) && wirelessInfo.ip_address != "0.0.0.0")
      {
        pControl->SetLabel2(wirelessInfo.ip_address.c_str());
      }
#else
      pControl->SetLabel2(g_infoManager.GetLabel(NETWORK_IP_ADDRESS).c_str());
#endif
    }
    else if (strSetting.Equals("version.totalmemory"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(false);
      pControl->SetLabel2(g_infoManager.GetLabel(SYSTEM_TOTAL_MEMORY).c_str());
    }
    else if (strSetting.Equals("version.screenresolution"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(false);
      pControl->SetLabel2(g_infoManager.GetLabel(SYSTEM_SCREEN_RESOLUTION).c_str());
    }
    else if (strSetting.Equals("version.macaddress"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(false);
      pControl->SetLabel2(g_infoManager.GetLabel(NETWORK_MAC_ADDRESS).c_str());
    }
#ifdef HAS_BOXEE_HAL
    else if (strSetting.Equals("version.wiredmacaddress"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(false);

      if (m_ethernetMacAddress == "")
      {
        CHalEthernetInfo info;
        if (CHalServicesFactory::GetInstance().GetEthernetInfo(0, info))
        {
          m_ethernetMacAddress = info.mac_address;
        }
      }

      pControl->SetLabel2(m_ethernetMacAddress);
    }
    else if (strSetting.Equals("version.wirelessmacaddress"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(false);

      if (m_wirelessMacAddress == "")
      {
        CHalWirelessInfo info;
        if (CHalServicesFactory::GetInstance().GetWirelessInfo(0, info))
        {
          m_wirelessMacAddress = info.mac_address;
        }
      }

      pControl->SetLabel2(m_wirelessMacAddress);
    }
    else if (strSetting.Equals("version.device") || strSetting.Equals("version.devicesn"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(false);

      if (m_device == "" || m_deviceSn == "")
      {
        CHalHardwareInfo info;
        if (CHalServicesFactory::GetInstance().GetHardwareInfo(info))
        {
          m_device.Format("%s", info.model.c_str());
          m_deviceSn = info.serialNumber;
        }
      }

      if (strSetting.Equals("version.devicesn"))
        pControl->SetLabel2(m_deviceSn);
      else if (strSetting.Equals("version.device"))
        pControl->SetLabel2(m_device);
    }
    else if (strSetting.Equals("version.firmware"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(false);

      CHalSoftwareInfo info;
      if (CHalServicesFactory::GetInstance().GetSoftwareInfo(info))
      {
        m_firmware.Format("%s", info.version.c_str());
      }

      pControl->SetLabel2(m_firmware);
    }
#endif
#ifdef HAS_DVB
    else if (strSetting.Equals("ota.reconfigure"))
    {
      m_iOtaScanSection = m_iSection;
    }
#endif
    else if (strSetting.Equals("system.leddisableonplayback"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      // LED_COLOUR_NO_CHANGE: we can't disable the LED on playback, 
      //                       we have no previos reference LED COLOUR, to set the LED colour back
      pControl->SetEnabled(g_guiSettings.GetInt("system.ledcolour") != LED_COLOUR_NO_CHANGE && g_guiSettings.GetInt("system.ledcolour") != LED_COLOUR_OFF);
    }
    else if (strSetting.Equals("musicfiles.trackformat"))
    {
      if (m_strOldTrackFormat != g_guiSettings.GetString("musicfiles.trackformat"))
      {
        CUtil::DeleteMusicDatabaseDirectoryCache();
        m_strOldTrackFormat = g_guiSettings.GetString("musicfiles.trackformat");
      }
    }
    else if (strSetting.Equals("musicfiles.trackformatright"))
    {
      if (m_strOldTrackFormatRight != g_guiSettings.GetString("musicfiles.trackformatright"))
      {
        CUtil::DeleteMusicDatabaseDirectoryCache();
        m_strOldTrackFormatRight = g_guiSettings.GetString("musicfiles.trackformatright");
      }
    }
#ifdef HAS_TIME_SERVER
    else if (strSetting.Equals("locale.timeserveraddress"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("locale.timeserver"));
    }
    else if (strSetting.Equals("locale.time") || strSetting.Equals("locale.date"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(!g_guiSettings.GetBool("locale.timeserver"));
      SYSTEMTIME curTime;
      GetLocalTime(&curTime);
      CStdString time;
      if (strSetting.Equals("locale.time"))
        time = g_infoManager.GetTime();
      else
        time = g_infoManager.GetDate();
      CSettingString *pSettingString = (CSettingString*)pSettingControl->GetSetting();
      pSettingString->SetData(time);
      pSettingControl->Update();
    }
#endif
    else if (strSetting.Equals("autodetect.nickname") || strSetting.Equals("autodetect.senduserpw"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("autodetect.onoff") && (g_settings.m_iLastLoadedProfileIndex == 0));
    }
    else if ( strSetting.Equals("autodetect.popupinfo"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("autodetect.onoff"));
    }
    else if (strSetting.Equals("videoplayer.externaldvdplayer"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("videoplayer.useexternaldvdplayer"));
    }
    else if (strSetting.Equals("cddaripper.path") || strSetting.Equals("mymusic.recordingpath") || strSetting.Equals("pictures.screenshotpath"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      if (pControl && g_guiSettings.GetString(strSetting, false).IsEmpty())
        pControl->SetLabel2("");
    }
    else if (strSetting.Equals("lcd.enableonpaused"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("lcd.disableonplayback") != LED_PLAYBACK_OFF && g_guiSettings.GetInt("lcd.type") != LCD_TYPE_NONE);
    }
    else if (strSetting.Equals("system.ledenableonpaused"))
    {
      // LED_COLOUR_NO_CHANGE: we can't enable LED on paused, 
      //                       we have no previos reference LED COLOUR, to set the LED colour back
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("system.leddisableonplayback") != LED_PLAYBACK_OFF && g_guiSettings.GetInt("system.ledcolour") != LED_COLOUR_OFF && g_guiSettings.GetInt("system.ledcolour") != LED_COLOUR_NO_CHANGE);
    }
#ifndef _LINUX
    else if (strSetting.Equals("lcd.backlight") || strSetting.Equals("lcd.disableonplayback"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("lcd.type") != LCD_TYPE_NONE);
    }
    else if (strSetting.Equals("lcd.contrast"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("lcd.type") != LCD_TYPE_NONE);
    }
#endif
    else if (strSetting.Equals("lookandfeel.enablemouse"))
    {
    }
    else if (strSetting.Equals("lookandfeel.rssedit"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(XFILE::CFile::Exists("special://home/scripts/RssTicker/default.py"));
    }
    else if (strSetting.Equals("musiclibrary.scrapersettings"))
    {
      CScraperParser parser;
      bool enabled=false;
      if (parser.Load("special://xbmc/system/scrapers/music/"+g_guiSettings.GetString("musiclibrary.defaultscraper")))
        enabled = parser.HasFunction("GetSettings");

      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(enabled);
    }
    else if (strSetting.Equals("videoplayer.synctype"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("videoplayer.usedisplayasclock"));
    }
    else if (strSetting.Equals("videoplayer.maxspeedadjust"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        bool enabled = (g_guiSettings.GetBool("videoplayer.usedisplayasclock")) && 
            (g_guiSettings.GetInt("videoplayer.synctype") == SYNC_RESAMPLE);
        pControl->SetEnabled(enabled);
      }  
    }      
    else if (strSetting.Equals("videoplayer.resamplequality"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        bool enabled = (g_guiSettings.GetBool("videoplayer.usedisplayasclock")) && 
            (g_guiSettings.GetInt("videoplayer.synctype") == SYNC_RESAMPLE);
        pControl->SetEnabled(enabled);
      }
    }
    else if (strSetting.Equals("filelists.filteradult"))
    {
      CProfile& prof = g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex];    
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) ((CGUIButtonControl *)pControl)->SetSelected(prof._bLockAdult);
    }
    else if (strSetting.Equals("weather.pluginsettings"))
    {
      // Create our base path
      CStdString basepath = "special://home/plugins/weather/" + g_guiSettings.GetString("weather.plugin");
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(!g_guiSettings.GetString("weather.plugin").IsEmpty() && CScriptSettings::SettingsExist(basepath));
    }
#ifdef HAS_ALSA
    else if (strSetting.Equals("audiooutput.custompassthrough"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL_SPDIF)
      {
        if (pControl) pControl->SetEnabled(g_guiSettings.GetString("audiooutput.passthroughdevice").Equals("custom"));
      }
      else
      {
        if (pControl) pControl->SetEnabled(false);
      }
    }
    else if (strSetting.Equals("audiooutput.customdevice"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetString("audiooutput.audiodevice").Equals("custom"));
    }
#endif
  }
}

void CGUIWindowSettingsCategory::UpdateRealTimeSettings()
{
  for (unsigned int i = 0; i < m_vecSettings.size(); i++)
  {
    CBaseSettingControl *pSettingControl = m_vecSettings[i];
    CStdString strSetting = pSettingControl->GetSetting()->GetSetting();
    if (strSetting.Equals("locale.time") || strSetting.Equals("locale.date"))
    {
#ifdef HAS_TIME_SERVER
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(!g_guiSettings.GetBool("locale.timeserver"));
#endif
      SYSTEMTIME curTime;
      GetLocalTime(&curTime);
      CStdString time;
      if (strSetting.Equals("locale.time"))
        time = g_infoManager.GetTime();
      else
        time = g_infoManager.GetDate();
      CSettingString *pSettingString = (CSettingString*)pSettingControl->GetSetting();
      pSettingString->SetData(time);
      pSettingControl->Update();
    }
  }
}

void CGUIWindowSettingsCategory::OnClick(CBaseSettingControl *pSettingControl)
{
  CStdString strSetting = pSettingControl->GetSetting()->GetSetting();
  if (strSetting.Equals("lookandfeel.rssedit"))
    CBuiltins::Execute("RunScript(special://home/scripts/RssTicker/default.py)");
  else if (strSetting.Equals("musiclibrary.scrapersettings") || strSetting.Equals("musiclibrary.defaultscraper"))
  {
    CMusicDatabase database;
    database.Open();
    SScraperInfo info;
    database.GetScraperForPath("musicdb://",info);
    if (!info.strPath.Equals(g_guiSettings.GetString("musiclibrary.defaultscraper")))
    {
      CScraperParser parser;
      parser.Load("special://xbmc/system/scrapers/music/"+g_guiSettings.GetString("musiclibrary.defaultscraper"));
      info.strPath = g_guiSettings.GetString("musiclibrary.defaultscraper");
      info.strContent = "albums";
      info.strTitle = parser.GetName();
    }
    if (info.settings.GetPluginRoot() || info.settings.LoadSettingsXML("special://xbmc/system/scrapers/music/"+info.strPath))
    {
      if (strSetting.Equals("musiclibrary.scrapersettings"))
        CGUIDialogPluginSettings::ShowAndGetInput(info);
    }
    database.SetScraperForPath("musicdb://",info);
  }

  // if OnClick() returns false, the setting hasn't changed or doesn't
  // require immediate update
  if (!pSettingControl->OnClick())
  {
    UpdateSettings();
    return;
  }

  OnSettingChanged(pSettingControl);
}

void CGUIWindowSettingsCategory::CheckForUpdates()
{
  for (unsigned int i = 0; i < m_vecSettings.size(); i++)
  {
    CBaseSettingControl *pSettingControl = m_vecSettings[i];
    if (pSettingControl->NeedsUpdate())
    {
      OnSettingChanged(pSettingControl);
      pSettingControl->Reset();
    }
  }
}

void CGUIWindowSettingsCategory::OnSettingChanged(CBaseSettingControl *pSettingControl)
{
  CStdString strSetting = pSettingControl->GetSetting()->GetSetting();

  if (strSetting.Equals("videoscreen.testresolution"))
  {
    TestAndSetNewResolution();
  }

  else if (strSetting.Equals("services.netflixclear"))
  {
    NetflixClear();
    CreateSettings();
  }

  else if (strSetting.Equals("services.vuduclear"))
  {
    VuduClear();
    CreateSettings();
  }

  else if (strSetting.Equals("services.spotifyclear"))
  {
    SpotifyClear();
    CreateSettings();
  }

#ifdef HAS_BOXEE_HAL
  else if (strSetting.Equals("vpn.operation"))
  {
    CVpnOperationBG* pJob = new CVpnOperationBG();
    if (CUtil::RunInBG(pJob) != JOB_SUCCEEDED)
    {
      CGUIDialogOK::ShowAndGetInput(51520, 0, 54748, 0);
    }
    else
    {
      // update user list received from server now
      BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().UpdateAllUserListsNow();
      CBoxeeBrowseMenuManager::GetInstance().ClearDynamicMenuButtons();
    }
  }
#endif

  // ok, now check the various special things we need to do
  else if (strSetting.Equals("mymusic.visualisation"))
  { // new visualisation choosen...
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    if (pControl->GetValue() == 0)
      pSettingString->SetData("None");
    else
      pSettingString->SetData( CVisualisation::GetCombinedName( pControl->GetCurrentLabel() ) );
  }
  /*else if (strSetting.Equals("musicfiles.repeat"))
  {
    g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC_TEMP, g_guiSettings.GetBool("musicfiles.repeat") ? PLAYLIST::REPEAT_ALL : PLAYLIST::REPEAT_NONE);
  }*/
  else if (strSetting.Equals("karaoke.port0voicemask"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("karaoke.port0voicemask", pControl->GetCurrentLabel());
    FillInVoiceMaskValues(0, g_guiSettings.GetSetting("karaoke.port0voicemask"));
  }
  else if (strSetting.Equals("karaoke.port1voicemask"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("karaoke.port1voicemask", pControl->GetCurrentLabel());
    FillInVoiceMaskValues(1, g_guiSettings.GetSetting("karaoke.port1voicemask"));
  }
  else if (strSetting.Equals("karaoke.port2voicemask"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("karaoke.port2voicemask", pControl->GetCurrentLabel());
    FillInVoiceMaskValues(2, g_guiSettings.GetSetting("karaoke.port2voicemask"));
  }
  else if (strSetting.Equals("karaoke.port2voicemask"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("karaoke.port3voicemask", pControl->GetCurrentLabel());
    FillInVoiceMaskValues(3, g_guiSettings.GetSetting("karaoke.port3voicemask"));
  }
  else if (strSetting.Equals("sort.showstarter"))
  {
    CGUIMessage refreshShowsMsg(GUI_MSG_UPDATE, WINDOW_SETTINGS_SYSTEM, 0);
    refreshShowsMsg.SetStringParam(strSetting);
    g_windowManager.SendThreadMessage(refreshShowsMsg,WINDOW_BOXEE_BROWSE_TVSHOWS);

    CGUIMessage refreshMovieMsg(GUI_MSG_UPDATE, WINDOW_SETTINGS_SYSTEM, 0);
    refreshMovieMsg.SetStringParam(strSetting);
    g_windowManager.SendThreadMessage(refreshMovieMsg,WINDOW_BOXEE_BROWSE_MOVIES);

    CGUIMessage refreshAlbumMsg(GUI_MSG_UPDATE, WINDOW_SETTINGS_SYSTEM, 0);
    refreshAlbumMsg.SetStringParam(strSetting);
    g_windowManager.SendThreadMessage(refreshAlbumMsg,WINDOW_BOXEE_BROWSE_ALBUMS);
  }
  else if (strSetting.Equals("filelists.filtergeoip2"))
  {
    CGUIRadioButtonControl *pControl = (CGUIRadioButtonControl*)GetControl(pSettingControl->GetID());

    bool cancelled = false;
    if (!pControl->IsSelected())
    {
      if (!CGUIDialogYesNo2::ShowAndGetInput(53295, 53297, 54643, 54642, cancelled, 0, 0))
      {
        pControl->SetSelected(true);
        g_guiSettings.SetBool("filelists.filtergeoip2", pControl->IsSelected());
      }
    }

    if (!cancelled)
    {
      CGUIMessage refreshMenuMsg(GUI_MSG_UPDATE, WINDOW_SETTINGS_SYSTEM, 0);
      refreshMenuMsg.SetStringParam(strSetting);
      g_windowManager.SendThreadMessage(refreshMenuMsg,WINDOW_DIALOG_BOXEE_BROWSE_MENU);

      CGUIMessage refreshShowMsg(GUI_MSG_UPDATE, WINDOW_SETTINGS_SYSTEM, 0);
      refreshShowMsg.SetStringParam(strSetting);
      g_windowManager.SendThreadMessage(refreshShowMsg,WINDOW_BOXEE_BROWSE_TVSHOWS);

      CGUIMessage refreshMovieMsg(GUI_MSG_UPDATE, WINDOW_SETTINGS_SYSTEM, 0);
      refreshMovieMsg.SetStringParam(strSetting);
      g_windowManager.SendThreadMessage(refreshMovieMsg,WINDOW_BOXEE_BROWSE_MOVIES);

      g_directoryCache.ClearSubPaths("boxee://");
    }
  }
  else if (strSetting.Equals("musiclibrary.cleanup"))
  {
    CMusicDatabase musicdatabase;
    musicdatabase.Clean();
    CUtil::DeleteMusicDatabaseDirectoryCache();
  }
  else if (strSetting.Equals("musiclibrary.defaultscraper"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    FillInScrapers(pControl, pControl->GetCurrentLabel(), "music");
  }
  else if (strSetting.Equals("scrapers.moviedefault"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    FillInScrapers(pControl, pControl->GetCurrentLabel(), "movies");
  }
  else if (strSetting.Equals("scrapers.tvshowdefault"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    FillInScrapers(pControl, pControl->GetCurrentLabel(), "tvshows");
  }
  else if (strSetting.Equals("scrapers.musicvideodefault"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    FillInScrapers(pControl, pControl->GetCurrentLabel(), "musicvideos");
  }
  else if (strSetting.Equals("videolibrary.cleanup"))
  {
    if (CGUIDialogYesNo::ShowAndGetInput(313, 333, 0, 0))
    {
      CVideoDatabase videodatabase;
      videodatabase.Open();
      videodatabase.CleanDatabase();
      videodatabase.Close();
    }
  }
  else if (strSetting.Equals("videolibrary.export") || strSetting.Equals("musiclibrary.export"))
  {
    int iHeading = 647;
    if (strSetting.Equals("musiclibrary.export"))
      iHeading = 20196;
    CStdString path(g_settings.GetDatabaseFolder());
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    bool singleFile;
    bool thumbs=false;
    bool overwrite=false;
    bool cancelled;
    singleFile = CGUIDialogYesNo::ShowAndGetInput(iHeading,20426,20427,-1,20428,20429,cancelled);
    if (cancelled)
      return;
    if (singleFile)
      thumbs = CGUIDialogYesNo::ShowAndGetInput(iHeading,20430,-1,-1,cancelled);
    if (cancelled)
      return;
    if (singleFile)
    overwrite = CGUIDialogYesNo::ShowAndGetInput(iHeading,20431,-1,-1,cancelled);
    if (cancelled)
      return;
    if (singleFile || CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(661), path, true))
    {
      if (strSetting.Equals("videolibrary.export"))
      {
      CUtil::AddFileToFolder(path, "videodb.xml", path);
      CVideoDatabase videodatabase;
      videodatabase.Open();
        videodatabase.ExportToXML(path,singleFile,thumbs,overwrite);
      videodatabase.Close();
    }
      else
      {
        CUtil::AddFileToFolder(path, "musicdb.xml", path);
        CMusicDatabase musicdatabase;
        musicdatabase.Open();
        musicdatabase.ExportToXML(path,singleFile,thumbs,overwrite);
        musicdatabase.Close();
  }
    }
  }
  else if (strSetting.Equals("karaoke.export") )
  {
    vector<CStdString> choices;
    choices.push_back(g_localizeStrings.Get(22034));
    choices.push_back(g_localizeStrings.Get(22035));

    int retVal = CGUIDialogContextMenu::ShowAndGetChoice(choices);
    if ( retVal > 0 )
    {
    CStdString path(g_settings.GetDatabaseFolder());
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(661), path, true))
    {
      CMusicDatabase musicdatabase;
      musicdatabase.Open();

        if ( retVal == 1 )
        {
          CUtil::AddFileToFolder(path, "karaoke.html", path);
          musicdatabase.ExportKaraokeInfo( path, true );
        }
        else
        {
          CUtil::AddFileToFolder(path, "karaoke.csv", path);
          musicdatabase.ExportKaraokeInfo( path, false );
        }
      musicdatabase.Close();
    }
  }
  }
  else if (strSetting.Equals("videolibrary.import"))
  {
    CStdString path(g_settings.GetDatabaseFolder());
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, "videodb.xml", g_localizeStrings.Get(651) , path))
    {
      CVideoDatabase videodatabase;
      videodatabase.Open();
      videodatabase.ImportFromXML(path);
      videodatabase.Close();
    }
  }
  else if (strSetting.Equals("musiclibrary.import"))
  {
    CStdString path(g_settings.GetDatabaseFolder());
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, "musicdb.xml", g_localizeStrings.Get(651) , path))
    {
      CMusicDatabase musicdatabase;
      musicdatabase.Open();
      musicdatabase.ImportFromXML(path);
      musicdatabase.Close();
    }
  }
  else if (strSetting.Equals("karaoke.importcsv"))
  {
    CStdString path(g_settings.GetDatabaseFolder());
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, "karaoke.csv", g_localizeStrings.Get(651) , path))
    {
      CMusicDatabase musicdatabase;
      musicdatabase.Open();
      musicdatabase.ImportKaraokeInfo(path);
      musicdatabase.Close();
    }
  }
  else if (strSetting.Equals("musicplayer.jumptoaudiohardware") || strSetting.Equals("videoplayer.jumptoaudiohardware"))
  {
    JumpToSection(WINDOW_SETTINGS_SYSTEM, "audiooutput");
  }
  else if (strSetting.Equals("musicplayer.jumptocache") || strSetting.Equals("videoplayer.jumptocache"))
  {
    JumpToSection(WINDOW_SETTINGS_SYSTEM, "cache");
  }
  else if (strSetting.Equals("weather.jumptolocale"))
  {
    JumpToSection(WINDOW_SETTINGS_APPEARANCE, "locale");
  }
  else if (strSetting.Equals("scrobbler.lastfmsubmit") || strSetting.Equals("scrobbler.lastfmsubmitradio") || strSetting.Equals("scrobbler.lastfmusername") || strSetting.Equals("scrobbler.lastfmpassword"))
  {
    CStdString strPassword=g_guiSettings.GetString("scrobbler.lastfmpassword");
    CStdString strUserName=g_guiSettings.GetString("scrobbler.lastfmusername");
    if ((g_guiSettings.GetBool("scrobbler.lastfmsubmit") || 
         g_guiSettings.GetBool("scrobbler.lastfmsubmitradio")) &&
         !strUserName.IsEmpty() && !strPassword.IsEmpty())
    {
      CLastfmScrobbler::GetInstance()->Init();
    }
    else
    {
      CLastfmScrobbler::GetInstance()->Term();
    }
  }
  else if (strSetting.Equals("scrobbler.librefmsubmit") || strSetting.Equals("scrobbler.librefmsubmitradio") || strSetting.Equals("scrobbler.librefmusername") || strSetting.Equals("scrobbler.librefmpassword"))
  {
    CStdString strPassword=g_guiSettings.GetString("scrobbler.librefmpassword");
    CStdString strUserName=g_guiSettings.GetString("scrobbler.librefmusername");
    if ((g_guiSettings.GetBool("scrobbler.librefmsubmit") || 
         g_guiSettings.GetBool("scrobbler.librefmsubmitradio")) &&
         !strUserName.IsEmpty() && !strPassword.IsEmpty())
    {
      CLibrefmScrobbler::GetInstance()->Init();
    }
    else
    {
      CLibrefmScrobbler::GetInstance()->Term();
    }
  }
  else if (strSetting.Left(22).Equals("MusicPlayer.ReplayGain"))
  { // Update our replaygain settings
    g_guiSettings.m_replayGain.iType = g_guiSettings.GetInt("musicplayer.replaygaintype");
    g_guiSettings.m_replayGain.iPreAmp = g_guiSettings.GetInt("musicplayer.replaygainpreamp");
    g_guiSettings.m_replayGain.iNoGainPreAmp = g_guiSettings.GetInt("musicplayer.replaygainnogainpreamp");
    g_guiSettings.m_replayGain.bAvoidClipping = g_guiSettings.GetBool("musicplayer.replaygainavoidclipping");
  }
  else if (strSetting.Equals("audiooutput.mode"))
  {
    if (!g_application.IsPlaying())
    {
#ifdef HAS_INTEL_SMD
      g_IntelSMDGlobals.BuildAudioOutputs();
#else
      CGUISoundPlayer::GetInstance().Deinitialize();
      CGUISoundPlayer::GetInstance().Initialize();
#endif
    }
  }
  else if (strSetting.Equals("audiooutput.audiodevice"))
  {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
      g_guiSettings.SetString("audiooutput.audiodevice", pControl->GetCurrentLabel());
  }
  else if (strSetting.Equals("audiooutput.controlmastervolume"))
  {
    CGUIRadioButtonControl *pControl = (CGUIRadioButtonControl*)GetControl(pSettingControl->GetID());
    g_guiSettings.SetBool("audiooutput.controlmastervolume", pControl->IsSelected());
  }
#if defined(_LINUX)
  else if (strSetting.Equals("audiooutput.passthroughdevice"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("audiooutput.passthroughdevice", pControl->GetCurrentLabel());
  }
#endif
#if 0
  // sanity check passthrough options
  else if( strSetting.Equals("audiooutput.ac3passthrough") &&
           g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL_HDMI )
  {
    if (!g_application.IsPlaying() && g_lic_settings.get_preferred_encoding() == AUDIO_VENDOR_DOLBY)
    {
      CGUISoundPlayer::GetInstance().Deinitialize();
      CGUISoundPlayer::GetInstance().Initialize();
    }
    CGUIButtonControl* pControl = (CGUIButtonControl*)GetControl(pSettingControl->GetID());
  }
  else if( strSetting.Equals("audiooutput.dtspassthrough") &&
           g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL_HDMI )
  {
    if (!g_application.IsPlaying() && g_lic_settings.get_preferred_encoding() == AUDIO_VENDOR_DTS)
    {
      CGUISoundPlayer::GetInstance().Deinitialize();
      CGUISoundPlayer::GetInstance().Initialize();
    }
  }
  else if (strSetting.Equals("audiooutput.dtspassthrough") || strSetting.Equals("audiooutput.ac3passthrough"))
  {
    if (!g_application.IsPlaying())
    {
      CGUISoundPlayer::GetInstance().Deinitialize();
      CGUISoundPlayer::GetInstance().Initialize();
    }
  }

#endif // HAS_INTEL_SMD
#ifdef HAS_LCD
  else if (strSetting.Equals("lcd.type"))
  {
#ifdef _LINUX
    g_lcd->Stop();
    CLCDFactory factory;
    delete g_lcd;
    g_lcd = factory.Create();
#endif
    g_lcd->Initialize();
  }
#ifndef _LINUX
  else if (strSetting.Equals("lcd.backlight"))
  {
    g_lcd->SetBackLight(((CSettingInt *)pSettingControl->GetSetting())->GetData());
  }
  else if (strSetting.Equals("lcd.contrast"))
  {
    g_lcd->SetContrast(((CSettingInt *)pSettingControl->GetSetting())->GetData());
  }
#endif
#endif
  else if (strSetting.Equals("servers.ftpserver"))
  {
    g_application.StopFtpServer();
    if (g_guiSettings.GetBool("servers.ftpserver"))
      g_application.StartFtpServer();
  }
  else if (strSetting.Equals("servers.ftpserverpassword"))
  {
   SetFTPServerUserPass();
  }
  else if (strSetting.Equals("servers.ftpserveruser"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("servers.ftpserveruser", pControl->GetCurrentLabel());
  }

  else if ( strSetting.Equals("servers.webserver") || strSetting.Equals("servers.webserverport") || 
            strSetting.Equals("servers.webserverusername") || strSetting.Equals("servers.webserverpassword"))
  {
    if (strSetting.Equals("servers.webserverport"))
    {
      CSettingString *pSetting = (CSettingString *)pSettingControl->GetSetting();
      // check that it's a valid port
      int port = atoi(pSetting->GetData().c_str());
      if (port <= 0 || port > 65535)
#ifndef _LINUX
        pSetting->SetData("80");
#else
        pSetting->SetData((geteuid() == 0)? "80" : "8080");
#endif
    }
#ifdef HAS_WEB_SERVER
    g_application.StopWebServer(true);
    if (g_guiSettings.GetBool("servers.webserver"))
    {
      g_application.StartWebServer();
      if (g_application.m_pWebServer)
      {
        if (strSetting.Equals("servers.webserverusername"))
          g_application.m_pWebServer->SetUserName(g_guiSettings.GetString("servers.webserverusername").c_str());
        else
          g_application.m_pWebServer->SetPassword(g_guiSettings.GetString("servers.webserverpassword").c_str());
      }
    }
#endif
  }
  else if (strSetting.Equals("airplay2.enable"))
  {
#ifdef HAS_AIRPLAY
    g_application.StopAirplayServer(true);

    if (g_guiSettings.GetBool("airplay2.enable"))
    {
      g_application.StartAirplayServer();
    }
#endif
  }
  else if (strSetting.Equals("servers.zeroconf"))
  {
#ifdef HAS_ZEROCONF
    //ifdef zeroconf here because it's only found in guisettings if defined
    CZeroconf::GetInstance()->Stop();
    if(g_guiSettings.GetBool("servers.zeroconf"))
      CZeroconf::GetInstance()->Start();
#endif
  }
  else if (strSetting.Equals("network.ipaddress"))
  {
    if (g_guiSettings.GetInt("network.assignment") == NETWORK_STATIC)
    {
      CStdString strDefault = g_guiSettings.GetString("network.ipaddress").Left(g_guiSettings.GetString("network.ipaddress").ReverseFind('.'))+".1";
      if (g_guiSettings.GetString("network.gateway").Equals("0.0.0.0"))
        g_guiSettings.SetString("network.gateway",strDefault);
      if (g_guiSettings.GetString("network.dns").Equals("0.0.0.0"))
        g_guiSettings.SetString("network.dns",strDefault);

    }
  }
    
  else if (strSetting.Equals("network.httpproxyport"))
  {
    CSettingString *pSetting = (CSettingString *)pSettingControl->GetSetting();
    // check that it's a valid port
    int port = atoi(pSetting->GetData().c_str());
    if (port <= 0 || port > 65535)
      pSetting->SetData("8080");
  }
  else if (strSetting.Equals("videoplayer.calibrate") || strSetting.Equals("videoscreen.guicalibration"))
  { // activate the video calibration screen
    g_windowManager.ActivateWindow(WINDOW_SCREEN_SIMPLE_CALIBRATION);
  }
  else if (strSetting.Equals("videoscreen.testpattern"))
  { // activate the test pattern
    g_windowManager.ActivateWindow(WINDOW_TEST_PATTERN);
  }
  else if (strSetting.Equals("videoscreen.testbadpixels"))
  { // activate the test for bad pixels
    g_windowManager.ActivateWindow(WINDOW_TEST_BAD_PIXELS_MANAGER);
  }
#ifdef HAS_GDL
  else if (strSetting.Equals("videoscreen.forceedid"))
  {
    bool bEnable = g_guiSettings.GetBool("videoscreen.forceedid");
    CStdString header = g_localizeStrings.Get(54873);
    CStdString line = g_localizeStrings.Get(51582);
    CStdString noLabel = g_localizeStrings.Get(222);
    CStdString yesLabel = g_localizeStrings.Get(51583);

    CGUIRadioButtonControl *pControl = (CGUIRadioButtonControl*)GetControl(pSettingControl->GetID());

    bool cancelled = false;

    if (!CGUIDialogYesNo2::ShowAndGetInput(header, line, noLabel, yesLabel, cancelled))
    {
      pControl->SetSelected(!bEnable);
      g_guiSettings.SetBool("videoscreen.forceedid", !bEnable);
    }
    else if (!cancelled)
    {
      CLog::Log(LOGNONE, "GUIWindowSettingsCategory forceedid is now %d. Restarting...", bEnable);
      ThreadMessage tMsg(TMSG_QUIT);
      g_application.getApplicationMessenger().SendMessage(tMsg,true);
    }
  }
  else if (strSetting.Equals("videoscreen.hdmioutput"))
  {
    g_Windowing.ConfigureHDMIOutput();
  }
  else if (strSetting.Equals("videoscreen.hdmipixeldepth"))
  {
    g_Windowing.ConfigureHDMIOutput();
  }
#endif
  else if (strSetting.Equals("videoplayer.externaldvdplayer"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CStdString path = pSettingString->GetData();
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    // TODO 2.0: Localize this
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, ".xbe", g_localizeStrings.Get(655), path))
      pSettingString->SetData(path);
  }
  else if (strSetting.Equals("subtitles.height"))
  {
    if (!CUtil::IsUsingTTFSubtitles())
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
      ((CSettingInt *)pSettingControl->GetSetting())->FromString(pControl->GetCurrentLabel());
    }
  }
  else if (strSetting.Equals("subtitles.font"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    pSettingString->SetData(pControl->GetCurrentLabel());
    FillInSubtitleHeights(g_guiSettings.GetSetting("subtitles.height"));
  }
  else if (strSetting.Equals("subtitles.charset"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString newCharset="DEFAULT";
    if (pControl->GetValue()!=0)
     newCharset = g_charsetConverter.getCharsetNameByLabel(pControl->GetCurrentLabel());
    if (newCharset != "" && (newCharset != pSettingString->GetData() || newCharset=="DEFAULT"))
    {
      pSettingString->SetData(newCharset);
      g_charsetConverter.reset();
    }
  }
  else if (strSetting.Equals("subtitles.enablesubtitlesbydefault"))
  {
    CGUIRadioButtonControl *pControl = (CGUIRadioButtonControl*)GetControl(pSettingControl->GetID());
    g_stSettings.m_defaultVideoSettings.m_SubtitleOn = pControl->IsSelected();
    g_settings.Save();
  }
  else if (strSetting.Equals("subtitles.preferredlanguage") || strSetting.Equals("audioplayback.preferredlanguage"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString newPreferredLanguage = pControl->GetCurrentLabel();
    pSettingString->SetData(newPreferredLanguage);
  }
  else if (strSetting.Equals("karaoke.fontheight"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    ((CSettingInt *)pSettingControl->GetSetting())->FromString(pControl->GetCurrentLabel());
  }
  else if (strSetting.Equals("karaoke.font"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    pSettingString->SetData(pControl->GetCurrentLabel());
    FillInSubtitleHeights(g_guiSettings.GetSetting("karaoke.fontheight"));
  }
  else if (strSetting.Equals("karaoke.charset"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString newCharset="DEFAULT";
    if (pControl->GetValue()!=0)
      newCharset = g_charsetConverter.getCharsetNameByLabel(pControl->GetCurrentLabel());
    if (newCharset != "" && (newCharset != pSettingString->GetData() || newCharset=="DEFAULT"))
    {
      pSettingString->SetData(newCharset);
      g_charsetConverter.reset();
    }
  }
  else if (strSetting.Equals("locale.charset"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString newCharset="DEFAULT";
    if (pControl->GetValue()!=0)
     newCharset = g_charsetConverter.getCharsetNameByLabel(pControl->GetCurrentLabel());
    if (newCharset != "" && (newCharset != pSettingString->GetData() || newCharset=="DEFAULT"))
    {
      pSettingString->SetData(newCharset);
      g_charsetConverter.reset();
    }
  }
  else if (strSetting.Equals("lookandfeel.font"))
  { // new font choosen...
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString strSkinFontSet = pControl->GetCurrentLabel();
    if (strSkinFontSet != ".svn" && strSkinFontSet != g_guiSettings.GetString("lookandfeel.font"))
    {
      m_strNewSkinFontSet = strSkinFontSet;
      g_application.DelayLoadSkin();
    }
    else
    { // Do not reload the language we are already using
      m_strNewSkinFontSet.Empty();
      g_application.CancelDelayLoadSkin();
    }
  }
  else if (strSetting.Equals("lookandfeel.skin"))
  { // new skin choosen...
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString strSkin = pControl->GetCurrentLabel();
    CStdString strSkinPath = g_settings.GetSkinFolder(strSkin);
    if (g_SkinInfo.Check(strSkinPath))
    {
      m_strErrorMessage.Empty();
      pControl->SettingsCategorySetSpinTextColor(pControl->GetButtonLabelInfo().textColor);
      if (strSkin != ".svn" && strSkin != g_guiSettings.GetString("lookandfeel.skin"))
      {
        m_strNewSkin = strSkin;
        g_application.DelayLoadSkin();
      }
      else
      { // Do not reload the skin we are already using
        m_strNewSkin.Empty();
        g_application.CancelDelayLoadSkin();
      }
    }
    else
    {
      m_strErrorMessage.Format("Incompatible skin. We require skins of version %0.2f or higher", g_SkinInfo.GetMinVersion());
      m_strNewSkin.Empty();
      g_application.CancelDelayLoadSkin();
      pControl->SettingsCategorySetSpinTextColor(pControl->GetButtonLabelInfo().disabledColor);
    }
  }
  else if (strSetting.Equals("lookandfeel.soundskin"))
  { // new sound skin choosen...
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    if (pControl->GetValue()==0)
      g_guiSettings.SetString("lookandfeel.soundskin", "OFF");
    else if (pControl->GetValue()==1)
      g_guiSettings.SetString("lookandfeel.soundskin", "SKINDEFAULT");
    else
      g_guiSettings.SetString("lookandfeel.soundskin", pControl->GetCurrentLabel());

    g_audioManager.Load();
  }
  else if (strSetting.Equals("lookandfeel.enablemouse"))
  {
    g_Mouse.SetEnabled(g_guiSettings.GetBool("lookandfeel.enablemouse"));
  }
  else if (strSetting.Equals("videoscreen.resolution"))
  { // new resolution choosen... - update if necessary
    int iControlID = pSettingControl->GetID();
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControlID);
    g_windowManager.SendMessage(msg);
    m_NewResolution = (RESOLUTION)msg.GetParam1();
    // reset our skin if necessary
    // delay change of resolution
    if (m_NewResolution == g_graphicsContext.GetVideoResolution())
    {
      m_NewResolution = RES_INVALID;
    }
  }
  else if (strSetting.Equals("videoscreen.vsync"))
  {
    int iControlID = pSettingControl->GetID();
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControlID);
    g_windowManager.SendMessage(msg);
// DXMERGE: This may be useful
//    g_videoConfig.SetVSyncMode((VSYNC)msg.GetParam1());
  }
  else if (strSetting.Equals("videoscreen.hiresrendering"))
  {
    bool enableLowRes = !g_guiSettings.GetBool("videoscreen.hiresrendering");
    g_graphicsContext.SetRenderLowresGraphics(enableLowRes);
  }
  else if (strSetting.Equals("locale.language"))
  {
    // new language chosen...

    CGUIDialogSelect *pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);

    pDlgSelect->SetHeading(54601);
    pDlgSelect->Reset();

    if(m_availableLanguages.IsEmpty())
    {
      BoxeeUtils::GetAvailableLanguages(m_availableLanguages);
    }

    for (int i = 0; i < m_availableLanguages.Size(); i++)
    {
      pDlgSelect->Add(m_availableLanguages[i]->GetProperty("lang_displayName"));
    }

    pDlgSelect->EnableButton(TRUE);
    pDlgSelect->SetButtonLabel(222); //'Cancel' button returns to weather settings
    pDlgSelect->DoModal();

    int selectedIndex = pDlgSelect->GetSelectedLabel();
    if (selectedIndex >= 0)
    {
      CStdString strNewLanguage = m_availableLanguages[selectedIndex]->GetProperty("lang_dirName");
      if (!strNewLanguage.Equals(g_guiSettings.GetString("locale.language")))
      {
#ifdef HAS_EMBEDDED
        CStdString header = g_localizeStrings.Get(57400);
        CStdString line;
        CStdString lineTmp = g_localizeStrings.Get(57401);
        line.Format(lineTmp.c_str(),m_availableLanguages[selectedIndex]->GetProperty("lang_displayName"));

        if (CGUIDialogYesNo2::ShowAndGetInput(header,line))
        {
#endif

          SetNewLanguage(strNewLanguage);

#ifdef HAS_EMBEDDED
          g_guiSettings.SetString("locale.language", m_strNewLanguage);
          g_settings.Save();

          CLog::Log(LOGDEBUG,"CGUIWindowSettingsCategory::OnSettingChanged - going to send TMSG_QUIT message after set [locale.language=%s] (lang)",g_guiSettings.GetString("locale.language").c_str());

          ThreadMessage tMsg(TMSG_QUIT);
          g_application.getApplicationMessenger().SendMessage(tMsg,true);
        }
#else

        g_application.ReloadSkin();

        CBoxeeBrowseMenuManager::GetInstance().Init(true);
#endif
      }
      else
      {
        CLog::Log(LOGDEBUG,"CGUIWindowSettingsCategory::OnSettingChanged - locale.language - since [NewLanguage=%s][%s=CurrentLanguage] not going to load the same language (lang)",strNewLanguage.c_str(),g_guiSettings.GetString("locale.language").c_str());
      }
    }
    else
    {
      CLog::Log(LOGDEBUG,"CGUIWindowSettingsCategory::OnSettingChanged - locale.language - got [%d] from select dialog (lang)",selectedIndex);
    }
  }
  else if (strSetting.Equals("locale.keyboard1"))
  { // new keyboard chosen...
    UpdateKeyboard(1,pSettingControl);
  }
  else if (strSetting.Equals("locale.keyboard2"))
  { // new keyboard chosen...
    UpdateKeyboard(2,pSettingControl);
  }
  else if (strSetting.Equals("locale.keyboard3"))
  { // new keyboard chosen...
    UpdateKeyboard(3,pSettingControl);
  }
  else if (strSetting.Equals("lookandfeel.skintheme"))
  { //a new Theme was chosen
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());

    CStdString strSkinTheme;

    if (pControl->GetValue() == 0) // Use default theme
      strSkinTheme = "SKINDEFAULT";
    else
      strSkinTheme = pControl->GetCurrentLabel() + ".xpr";

    if (strSkinTheme != pSettingString->GetData())
    {
      m_strNewSkinTheme = strSkinTheme;
      g_application.DelayLoadSkin();
    }
    else
    { // Do not reload the skin theme we are using
      m_strNewSkinTheme.Empty();
      g_application.CancelDelayLoadSkin();
    }
  }
  else if (strSetting.Equals("lookandfeel.skincolors"))
  { //a new color was chosen
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());

    CStdString strSkinColor;

    if (pControl->GetValue() == 0) // Use default colors
      strSkinColor = "SKINDEFAULT";
    else
      strSkinColor = pControl->GetCurrentLabel() + ".xml";

    if (strSkinColor != pSettingString->GetData())
    {
      m_strNewSkinColors = strSkinColor;
      g_application.DelayLoadSkin();
    }
    else
    { // Do not reload the skin colors we are using
      m_strNewSkinColors.Empty();
      g_application.CancelDelayLoadSkin();
    }
  }
  else if (strSetting.Equals("videoplayer.displayresolution"))
  {
    CSettingInt *pSettingInt = (CSettingInt *)pSettingControl->GetSetting();
    int iControlID = pSettingControl->GetID();
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControlID);
    g_windowManager.SendMessage(msg);
    pSettingInt->SetData(msg.GetParam1());
  }
  else if (strSetting.Equals("videoscreen.flickerfilter") || strSetting.Equals("videoscreen.soften"))
  { // reset display
    g_graphicsContext.SetVideoResolution(g_guiSettings.m_LookAndFeelResolution);
  }
  else if (strSetting.Equals("menu.showsdefault"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    int iValue = pControl->GetValue();
    CStdString strShowsDefault;
    if (iValue == 0)
      strShowsDefault = "favorite";
    else if (iValue == 1)
      strShowsDefault = "local";
    else if (iValue == 2)
      strShowsDefault = "all";

    CGUIWindowStateDatabase sdb;
    sdb.SetDefaultCategory(WINDOW_BOXEE_BROWSE_TVSHOWS, strShowsDefault);

  }
  else if (strSetting.Equals("menu.moviesdefault"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    int iValue = pControl->GetValue();
    CStdString strMoviesDefault;
    if (iValue == 0)
      strMoviesDefault = "local";
    else if (iValue == 1)
      strMoviesDefault = "all";

    CGUIWindowStateDatabase sdb;
    sdb.SetDefaultCategory(WINDOW_BOXEE_BROWSE_MOVIES, strMoviesDefault);
  }
  else if (strSetting.Equals("menu.appsdefault"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    int iValue = pControl->GetValue();
    CStdString strAppsDefault;
    if (iValue == 0)
      strAppsDefault = "favorite";
    else if (iValue == 1)
      strAppsDefault = "all";

    CGUIWindowStateDatabase sdb;
    sdb.SetDefaultCategory(WINDOW_BOXEE_BROWSE_APPS, strAppsDefault);
  }
  else if (strSetting.Equals("screensaver.mode"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    int iValue = pControl->GetValue();
    CStdString strScreenSaver;
    if (iValue == 0)
      strScreenSaver = "None";
    else if (iValue == 1)
      strScreenSaver = "Dim";
    else if (iValue == 2)
      strScreenSaver = "Black";
    else if (iValue == 3)
      strScreenSaver = "SlideShow"; // PictureSlideShow
    else if (iValue == 4)
      strScreenSaver = "Fanart Slideshow"; //Fanart Slideshow
    else
      strScreenSaver = pControl->GetCurrentLabel() + ".xbs";
    pSettingString->SetData(strScreenSaver);
  }
  else if (strSetting.Equals("screensaver.preview"))
  {
    g_application.ActivateScreenSaver(true);
  }
  else if (strSetting.Equals("screensaver.slideshowpath"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CStdString path = pSettingString->GetData();

    VECSOURCES localPictureSources;

    CMediaSource boxeeScreensaverMediaSource;
    boxeeScreensaverMediaSource.strPath = "special://xbmc/media/boxee_screen_saver";
    boxeeScreensaverMediaSource.strName = "Boxee Screensaver";
    localPictureSources.push_back(boxeeScreensaverMediaSource);

    // Get only the local sources from the PictureSources
    for(size_t i=0;i<g_settings.m_pictureSources.size();i++)
    {
      localPictureSources.push_back(g_settings.m_pictureSources[i]);
    }

	  CStdString devicePath;

#if defined(_LINUX) && !defined(__APPLE__) && !defined(HAS_EMBEDDED)
    // Add user home directory
    devicePath = getenv("HOME");
    CMediaSource homeMediaSource;
    homeMediaSource.strPath = devicePath;
    homeMediaSource.strName = "Home";
    localPictureSources.push_back(homeMediaSource);
  
    // Add root directory
    devicePath = "/";
    CMediaSource rootMediaSource;
    rootMediaSource.strPath = devicePath;
    rootMediaSource.strName = "Computer Filesystem"; 
    localPictureSources.push_back(rootMediaSource);
#elif defined(_WIN32) 
	VECSOURCES localDrives;
	g_mediaManager.GetLocalDrives(localDrives);
	for(size_t i=0;i<localDrives.size();i++)
	{
      CMediaSource mediaSource = localDrives[i];
	  localPictureSources.push_back(mediaSource);
	}
#endif  
    
    if (CGUIDialogFileBrowser::ShowAndGetDirectory(localPictureSources, g_localizeStrings.Get(pSettingString->m_iHeadingString), path))
    {
      pSettingString->SetData(path);
    }
  }

  /*else if (strSetting.Equals("background.imagefolder"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CStdString path = pSettingString->GetData();
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);

    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(pSettingString->m_iHeadingString), path))
    {
      pSettingString->SetData(path);

      CSettingString * setting = (CSettingString *)(GetSetting("background.imagefile")->GetSetting());
      setting->SetData("");
      g_settings.ResetSkinSetting("CustomBG");
      g_settings.SetSkinString(g_settings.TranslateSkinString("CustomBGFolder"), path);
      g_settings.SetSkinBool(g_settings.TranslateSkinBool("EnableCustomBGFolder"), true);
    }
    else
    {
      g_settings.ResetSkinSetting("CustomBGFolder");
      g_settings.ResetSkinSetting("EnableCustomBGFolder");
      g_settings.Save();
  }
  }
  else if (strSetting.Equals("background.imagefile"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CStdString path = pSettingString->GetData();
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);

    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, "", g_localizeStrings.Get(pSettingString->m_iHeadingString), path, true))
    {
      pSettingString->SetData(path);

      CSettingString * setting = (CSettingString *)(GetSetting("background.imagefolder")->GetSetting());
      setting->SetData("");
      g_settings.ResetSkinSetting("CustomBGFolder");
      g_settings.SetSkinString(g_settings.TranslateSkinString("CustomBG"), path);
      g_settings.SetSkinBool(g_settings.TranslateSkinBool("EnableCustomBG"), true);
    }
    else
    {
      g_settings.ResetSkinSetting("CustomBG");
      g_settings.ResetSkinSetting("EnableCustomBG");
      g_settings.Save();
  }
  }
  else if (strSetting.Equals("background.reset"))
  {
    g_settings.ResetSkinSetting("EnableCustomBG");
    g_settings.ResetSkinSetting("EnableCustomBGFolder");

    CSettingString * setting = (CSettingString *)(GetSetting("background.imagefile")->GetSetting());
    setting->SetData("");

    CSettingString * setting1 = (CSettingString *)(GetSetting("background.imagefolder")->GetSetting());
    setting1->SetData("");
  }*/


  else if (strSetting.Equals("pictures.screenshotpath") || strSetting.Equals("mymusic.recordingpath") || strSetting.Equals("cddaripper.path") || strSetting.Equals("subtitles.custompath"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CStdString path = g_guiSettings.GetString(strSetting,false);
    VECSOURCES shares;

    g_mediaManager.GetNetworkLocations(shares);
    g_mediaManager.GetLocalDrives(shares);

    UpdateSettings();
    bool bWriteOnly = true;

    if (strSetting.Equals("subtitles.custompath"))
    {
      bWriteOnly = false;
      shares = g_settings.m_videoSources;
      g_mediaManager.GetLocalDrives(shares);
    }
    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(pSettingString->m_iHeadingString), path, bWriteOnly))
    {
      pSettingString->SetData(path);
    }
  }
  else if (strSetting.Left(22).Equals("MusicPlayer.ReplayGain"))
  { // Update our replaygain settings
    g_guiSettings.m_replayGain.iType = g_guiSettings.GetInt("musicplayer.replaygaintype");
    g_guiSettings.m_replayGain.iPreAmp = g_guiSettings.GetInt("musicplayer.replaygainpreamp");
    g_guiSettings.m_replayGain.iNoGainPreAmp = g_guiSettings.GetInt("musicplayer.replaygainnogainpreamp");
    g_guiSettings.m_replayGain.bAvoidClipping = g_guiSettings.GetBool("musicplayer.replaygainavoidclipping");
  }
  else if (strSetting.Equals("locale.country"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());

    const CStdString& strRegion=pControl->GetCurrentLabel();
    
    g_langInfo.SetCurrentRegion(strRegion);
    g_guiSettings.SetString("locale.country", strRegion);
    CWeather::GetInstance().Refresh(); // need to reset our weather, as temperatures need re-translating.
  }

  else if (strSetting.Equals("locale.tempscale"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());

    const CStdString& tempUnit=pControl->GetCurrentLabel();

    SetTempUnit(tempUnit);
  }
  else if (strSetting.Equals("locale.timeformat"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    const CStdString& timeFormat=pControl->GetCurrentLabel();

    SetTimeFormat(timeFormat);
  }

#ifdef HAS_TIME_SERVER
  else if (strSetting.Equals("locale.timeserver") || strSetting.Equals("locale.timeserveraddress"))
  {
    g_application.StopTimeServer();
    if (g_guiSettings.GetBool("locale.timeserver"))
      g_application.StartTimeServer();
  }
#endif
  else if (strSetting.Equals("locale.time"))
  {
    SYSTEMTIME curTime;
    GetLocalTime(&curTime);
    if (CGUIDialogNumeric::ShowAndGetTime(curTime, g_localizeStrings.Get(14066)))
    { // yay!
      SYSTEMTIME curDate;
      GetLocalTime(&curDate);
      CUtil::SetSysDateTimeYear(curDate.wYear, curDate.wMonth, curDate.wDay, curTime.wHour, curTime.wMinute);
    }
  }
  else if (strSetting.Equals("locale.date"))
  {
    SYSTEMTIME curDate;
    GetLocalTime(&curDate);
    if (CGUIDialogNumeric::ShowAndGetDate(curDate, g_localizeStrings.Get(14067)))
    { // yay!
      SYSTEMTIME curTime;
      GetLocalTime(&curTime);
      CUtil::SetSysDateTimeYear(curDate.wYear, curDate.wMonth, curDate.wDay, curTime.wHour, curTime.wMinute);
    }
  }

  else if (strSetting.Equals("smb.winsserver") || strSetting.Equals("smb.workgroup") )
  {
    if (g_guiSettings.GetString("smb.winsserver") == "0.0.0.0")
      g_guiSettings.SetString("smb.winsserver", "");

    /* okey we really don't need to restarat, only deinit samba, but that could be damn hard if something is playing*/
    //TODO - General way of handling setting changes that require restart

    if (CGUIDialogYesNo2::ShowAndGetInput(14038, 54765))
    {
      g_application.getApplicationMessenger().RestartApp();
    }
  }
  else if(strSetting.Equals("smb.agressivescan2"))
  {
    CGUIEditControl *pControl = (CGUIEditControl *)GetControl(pSettingControl->GetID());
    if (pControl)
    {
      pControl->SetEnabled(g_guiSettings.GetBool("smb.agressivescan2"));
    }

    CBrowserService* pBrowser = g_application.GetBrowserService();
    if(pBrowser)
      pBrowser->Refresh();
  }
  else if (strSetting.Equals("upnp.client"))
  {
#ifdef HAS_UPNP
    if (g_guiSettings.GetBool("upnp.client"))
      g_application.StartUPnPClient();
    else
      g_application.StopUPnPClient();
#endif
  }
  else if (strSetting.Equals("upnp.server"))
  {
#ifdef HAS_UPNP
    if (g_guiSettings.GetBool("upnp.server"))
      g_application.StartUPnPServer();
    else
      g_application.StopUPnPServer();
#endif
  }
  else if (strSetting.Equals("upnp.renderer"))
  {
#ifdef HAS_UPNP
    if (g_guiSettings.GetBool("upnp.renderer"))
      g_application.StartUPnPRenderer();
    else
      g_application.StopUPnPRenderer();
#endif
  }
  else if (strSetting.Equals("remoteevents.enabled"))
  {
#ifdef HAS_EVENT_SERVER
    if (g_guiSettings.GetBool("remoteevents.enabled"))
      g_application.StartEventServer();
    else
    {
      if (!g_application.StopEventServer(true, true))
      {
        g_guiSettings.SetBool("remoteevents.enabled", true);
        CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
        if (pControl) pControl->SetEnabled(true);
      }
    }
#endif
  }
  else if (strSetting.Equals("remoteevents.allinterfaces"))
  {
#ifdef HAS_EVENT_SERVER
    if (g_guiSettings.GetBool("remoteevents.enabled"))
    {
      if (g_application.StopEventServer(true, true))
        g_application.StartEventServer();
      else
      {
        g_guiSettings.SetBool("remoteevents.enabled", true);
        CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
        if (pControl) pControl->SetEnabled(true);
      }
    }
#endif
  }
  else if (strSetting.Equals("remoteevents.initialdelay") || 
           strSetting.Equals("remoteevents.continuousdelay"))    
  {
#ifdef HAS_EVENT_SERVER
    if (g_guiSettings.GetBool("remoteevents.enabled"))
    {
      g_application.RefreshEventServer();
    }
#endif      
  }
  else if (strSetting.Equals("upnp.musicshares"))
  {
    CStdString filename;
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "upnpserver.xml", filename);
    CStdString strDummy;
    g_settings.LoadUPnPXml(filename);
    if (CGUIDialogFileBrowser::ShowAndGetSource(strDummy,false,&g_settings.m_UPnPMusicSources,"upnpmusic"))
      g_settings.SaveUPnPXml(filename);
    else
      g_settings.LoadUPnPXml(filename);
  }
  else if (strSetting.Equals("upnp.videoshares"))
  {
    CStdString filename;
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "upnpserver.xml", filename);
    CStdString strDummy;
    g_settings.LoadUPnPXml(filename);
    if (CGUIDialogFileBrowser::ShowAndGetSource(strDummy,false,&g_settings.m_UPnPVideoSources,"upnpvideo"))
      g_settings.SaveUPnPXml(filename);
    else
      g_settings.LoadUPnPXml(filename);
  }
  else if (strSetting.Equals("upnp.pictureshares"))
  {
    CStdString filename;
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "upnpserver.xml", filename);
    CStdString strDummy;
    g_settings.LoadUPnPXml(filename);
    if (CGUIDialogFileBrowser::ShowAndGetSource(strDummy,false,&g_settings.m_UPnPPictureSources,"upnppictures"))
      g_settings.SaveUPnPXml(filename);
    else
      g_settings.LoadUPnPXml(filename);
  }
  else if (strSetting.Equals("masterlock.lockcode"))
  {
    // Now Prompt User to enter the old and then the new MasterCode!
    if(g_passwordManager.SetMasterLockMode())
    {
      // We asked for the master password and saved the new one!
      // Nothing todo here
    }
  }
#ifdef HAS_BOXEE_HAL
  else if (strSetting.Equals("network.interface"))
  {
     NetworkInterfaceChanged();
  }
  else if (strSetting.Equals("network.save"))
  { 
     CHalAddrType iAssignment = ADDR_NONE;
     CStdString sIPAddress;
     CStdString sNetworkMask;
     CStdString sDefaultGateway;
     CStdString sWirelessNetwork;
     CStdString sWirelessKey;
     CHalWirelessAuthType iAuthType;
     CStdString sDns;

     CGUISpinControlEx *ifaceControl = (CGUISpinControlEx *)GetControl(GetSetting("network.interface")->GetID());
     int ifaceId = ifaceControl->GetValue();

   
     // Update controls with information
     CGUISpinControlEx* pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.assignment")->GetID());
     if (pControl1) iAssignment = (CHalAddrType) pControl1->GetValue();
     CGUIButtonControl* pControl2 = (CGUIButtonControl *)GetControl(GetSetting("network.ipaddress")->GetID());
     if (pControl2) sIPAddress = pControl2->GetLabel2();         
     pControl2 = (CGUIButtonControl *)GetControl(GetSetting("network.subnet")->GetID());
     if (pControl2) sNetworkMask = pControl2->GetLabel2();         
     pControl2 = (CGUIButtonControl *)GetControl(GetSetting("network.gateway")->GetID());
     if (pControl2) sDefaultGateway = pControl2->GetLabel2();         
     pControl2 = (CGUIButtonControl *)GetControl(GetSetting("network.dns")->GetID());
     if (pControl2) sDns = pControl2->GetLabel2();         
     pControl2 = (CGUIButtonControl *)GetControl(GetSetting("network.essid")->GetID());
     if (pControl2) sWirelessNetwork = pControl2->GetLabel2();         
     sWirelessKey = m_networkKey;
     iAuthType = m_networkEnc;

     if (ifaceId == WIRELESS_INTERFACE_ID && sWirelessNetwork.length() == 0 && iAssignment != ADDR_NONE)
     {
       CGUIDialogOK::ShowAndGetInput(0, 54671, 0, 0);
     }
     else if (iAssignment == ADDR_STATIC && !CUtil::IsValidIp(sIPAddress))
     {
       CGUIDialogOK2::ShowAndGetInput(257,54653);
     }
     else if (iAssignment == ADDR_STATIC && !CUtil::IsValidIp(sNetworkMask))
     {
       CGUIDialogOK2::ShowAndGetInput(257,54653);
     }
     else if (iAssignment == ADDR_STATIC && !CUtil::IsValidIp(sDns))
     {
       CGUIDialogOK2::ShowAndGetInput(257,54653);
     }
     else if (iAssignment == ADDR_STATIC && !CUtil::IsValidIp(sDefaultGateway))
     {
       CGUIDialogOK2::ShowAndGetInput(257,54653);
     }
     else
     {
       bool done = true;

       CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
       pDlgProgress->SetLine(0, "");
       pDlgProgress->SetLine(1, g_localizeStrings.Get(784));
       pDlgProgress->SetLine(2, "");
       pDlgProgress->StartModal();
       pDlgProgress->Progress();

       if (ifaceId == WIRED_INTERFACE_ID)
       {
         CHalEthernetConfig config;
         config.addr_type = iAssignment;
         config.dns = sDns;
         config.gateway = sDefaultGateway;
         config.ip_address = sIPAddress;
         config.netmask = sNetworkMask;
         CHalServicesFactory::GetInstance().SetEthernetConfig(0, config);
       }
       else
       {
         CHalWirelessConfig config;
         config.addr_type = iAssignment;
         config.dns = sDns;
         config.gateway = sDefaultGateway;
         config.ip_address = sIPAddress;
         config.netmask = sNetworkMask;
         config.password = sWirelessKey;
         config.ssid = sWirelessNetwork;
         config.authType = iAuthType;
         CHalServicesFactory::GetInstance().SetWirelessConfig(0, config);
       }

       pDlgProgress->Close();
       if (iAssignment != ADDR_NONE)
       {
         CWaitNetworkUpBG* pJob = new CWaitNetworkUpBG(ifaceId);
         done = (CUtil::RunInBG(pJob) == JOB_SUCCEEDED);
       }

       if (ifaceId == WIRED_INTERFACE_ID)
       {
         CHalEthernetInfo info;
         CHalServicesFactory::GetInstance().GetEthernetInfo(0, info);

         GetSetting("network.ipaddress")->GetSetting()->FromString(info.ip_address);
         GetSetting("network.subnet")->GetSetting()->FromString(info.netmask);
         GetSetting("network.gateway")->GetSetting()->FromString(info.gateway);
         GetSetting("network.dns")->GetSetting()->FromString(info.dns);
       }
       else
       {
         CHalWirelessInfo info;
         CHalServicesFactory::GetInstance().GetWirelessInfo(0, info);

         GetSetting("network.ipaddress")->GetSetting()->FromString(info.ip_address);
         GetSetting("network.subnet")->GetSetting()->FromString(info.netmask);
         GetSetting("network.gateway")->GetSetting()->FromString(info.gateway);
         GetSetting("network.dns")->GetSetting()->FromString(info.dns);
         GetSetting("network.essid")->GetSetting()->FromString(info.ssid);
       }

       if (!done)
         CGUIDialogOK::ShowAndGetInput(0, 786, 0, 0);
       else if (iAssignment == ADDR_NONE)
          CGUIDialogOK::ShowAndGetInput(0, 788, 0, 0);
       else
          CGUIDialogOK::ShowAndGetInput(0, 785, 0, 0);

       if(m_iNetworkInterface != ifaceId)
       {
         CBrowserService* pBrowser = g_application.GetBrowserService();
         if(pBrowser)
           pBrowser->Refresh();

         m_iNetworkInterface = ifaceId;
       }
     }

  }
  else if (strSetting.Equals("network.essid"))
  {
    CGUIButtonControl* essControl = (CGUIButtonControl *)GetControl(GetSetting("network.essid")->GetID());

    CGUIDialogAccessPoints *dialog = (CGUIDialogAccessPoints *)g_windowManager.GetWindow(WINDOW_DIALOG_ACCESS_POINTS);
    if (dialog)
    {
      if (essControl)
        dialog->SetEssId(essControl->GetLabel2());
      dialog->SetAuth(m_networkEnc);
      dialog->DoModal();
       
      if (dialog->WasItemSelected())
      {
        if (essControl)
          essControl->SetLabel2(dialog->GetEssId());

        m_networkEnc = dialog->GetAuth();

        if (dialog->GetAuth() != AUTH_NONE)
        {
          m_networkKey = dialog->GetPassword();
        }
        else
        {
          m_networkKey = "";
        }
      }
    }
  }
#endif
#ifdef _LINUX
  else if (strSetting.Equals("locale.timezonecountry"))
  {
     CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
     CStdString strCountry = pControl->GetCurrentLabel();
     g_timezone.SetTimezone(strCountry);
     g_guiSettings.SetString("locale.timezonecountry", strCountry.c_str());
    }
  else  if (strSetting.Equals("locale.timezone"))
  {
     CGUISpinControlEx *tzControl = (CGUISpinControlEx *)GetControl(GetSetting("locale.timezone")->GetID());
     g_timezone.SetTimezone(tzControl->GetLabel());
     g_guiSettings.SetString("locale.timezone", tzControl->GetLabel().c_str());
  }
#endif
//Boxee
  else  if (strSetting.Equals("audiooutput.mode"))
  {
#ifdef HAS_ALSA
    if (g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL_SPDIF)
      g_guiSettings.SetString("audiooutput.audiodevice", "iec958");
    else
      g_guiSettings.SetString("audiooutput.audiodevice", "default");
#endif
  }
//end Boxee
  else if (strSetting.Equals("lookandfeel.skinzoom"))
  {
    g_windowManager.SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_WINDOW_RESIZE);
  }
  else if (strSetting.Equals("videolibrary.flattentvshows") ||
           strSetting.Equals("videolibrary.removeduplicates"))
  {
    CUtil::DeleteVideoDatabaseDirectoryCache();
  }
  else if (strSetting.Equals("filelists.filteradult"))
  {    
    bool shouldSave = false;
    bool resetToTrue = false;
    
    CProfile& prof = g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex];
    if (prof._bLockAdult)
    {
      resetToTrue = true;
      if (CGUIDialogYesNo2::ShowAndGetInput(53295, 53291))
      {
        if (prof.getAdultLockCode() != "" && prof.getAdultLockCode() != "d41d8cd98f00b204e9800998ecf8427e" /* empty string as md5 */)
        {
          CStdString heading = "Enter Password";
          CStdString password = prof.getAdultLockCode();
          int rc = CGUIDialogKeyboard::ShowAndVerifyPassword(password, heading, 0); 
          if (rc == 0)
          {
            prof._bLockAdult = false;
            shouldSave = true;
            resetToTrue = false;
          }
          else if (rc == 1)
          {
            CGUIDialogYesNo2::ShowAndGetInput(257, 53294);
          }
        }          
        else
        {
          prof._bLockAdult = false;
          shouldSave = true;
          resetToTrue = false;
        }
      }
    }
    else
    {
      prof._bLockAdult = true;
      shouldSave = true;
    }
    
    if (resetToTrue)
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting("filelists.filteradult")->GetID());
      pControl->SetSelected(true);
      return;
    }
    else
    {
      BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().UpdateAppsCategoriesListNow();
      CBoxeeBrowseMenuManager::GetInstance().ClearDynamicMenuButtons();
    }
    
    if (shouldSave)
    {
      g_settings.SaveProfiles(PROFILES_FILE);
    }
  }
  else if (strSetting.Equals("filelists.setadultcode"))
  {
    CProfile& prof = g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex];
    if (prof.getAdultLockCode() != "" && prof.getAdultLockCode() != "d41d8cd98f00b204e9800998ecf8427e" /* empty string as md5 */)
    {
      CStdString heading = "Enter Password";
      CStdString password = prof.getAdultLockCode();
      int rc = CGUIDialogKeyboard::ShowAndVerifyPassword(password, heading, 0); 
      if (rc == 1)
      {
        CGUIDialogOK::ShowAndGetInput(257, 53294, 0 ,0);
        return;
      }
      else if (rc == -1)
      {
        return;
      }
    }

    CStdString newPassword;
    if (CGUIDialogKeyboard::ShowAndVerifyNewPassword(newPassword, g_localizeStrings.Get(12340), true))
    {
      prof.setAdultLockCode(newPassword);
      g_settings.SaveProfiles(PROFILES_FILE);
    }
    else
    {
      return;
    }
  }
  else if (strSetting.Equals("erase.db"))
  {
    CLog::Log(LOGDEBUG,"CGUIWindowSettingsCategory::OnSettingChanged - will remove database and delete thumbnails");
    g_application.DeleteDatabaseAndThumbnails();
  }
  else if (strSetting.Equals("erase.settings"))
  {
    CLog::Log(LOGDEBUG,"CGUIWindowSettingsCategory::OnSettingChanged - will delete GuiSettings.xml file");

    CStdString header = g_localizeStrings.Get(51578);
    CStdString line = g_localizeStrings.Get(51579) + g_localizeStrings.Get(51582);
    CStdString noLabel = g_localizeStrings.Get(222);
    CStdString yesLabel = g_localizeStrings.Get(51583);

    if (CGUIDialogYesNo2::ShowAndGetInput(header,line,noLabel,yesLabel))
    {
      //delete GuiSettings.xml file
      CFile settings;
      settings.Delete("special://masterprofile/guisettings.xml");

      CLog::Log(LOGDEBUG,"CGUIWindowSettingsCategory::OnSettingChanged - going to send TMSG_QUIT message");

      //sent quit msg
      ThreadMessage tMsg(TMSG_QUIT);
      g_application.getApplicationMessenger().SendMessage(tMsg,true);
    }
  }
  else if (strSetting.Equals("erase.thumb"))
  {
    CLog::Log(LOGDEBUG,"CGUIWindowSettingsCategory::OnSettingChanged - [CleanOldThumbnails] -> Going to call g_application.RemoveOldThumbnails with [force=TRUE] (rot)");
    g_application.RemoveOldThumbnails(true,true);
	}
#ifdef HAS_EMBEDDED
  else if (strSetting.Equals("update.status"))
  {
    CBoxeeVersionUpdateJob versionUpdateJob = g_boxeeVersionUpdateManager.GetBoxeeVerUpdateJob();
    //
    // new version is ready - start to update
    //
    if(versionUpdateJob.acquireVersionUpdateJobForPerformUpdate())
    {
      CBoxeeVersionUpdateManager::HandleUpdateVersionButton(false);
    }
  }
  else if (strSetting.Equals("update.check.for.update"))
  {
    bool hasNewUpdate = false;
    CStdString versionUpdateBuildNum;
    int retval = 0;

    CGUIDialogProgress* progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

    progress->StartModal();
	
    retval = g_boxeeVersionUpdateManager.CheckForUpdate(hasNewUpdate, versionUpdateBuildNum);
 
    progress->Close();

    if(retval == 0 && hasNewUpdate == true)
    {
       CLog::Log(LOGINFO,"CGUIWindowSettingsCategory::OnSettingChanged - Has new update version %s", versionUpdateBuildNum.c_str());
       
       bool bCanceled = false;

       if(CGUIDialogYesNo2::ShowAndGetInput(g_localizeStrings.Get(53240), g_localizeStrings.Get(53254), bCanceled) && !bCanceled)
       {
         g_boxeeVersionUpdateManager.StartUpdate();
         CLog::Log(LOGINFO,"CGUIWindowSettingsCategory::OnSettingChanged - Going to run update job");
       }
       else
       {
         CLog::Log(LOGINFO,"CGUIWindowSettingsCategory::OnSettingChanged - Download of update was canceled");
       }
    }
    else
    {
      CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53240), g_localizeStrings.Get(53249));

      CLog::Log(LOGINFO,"CGUIWindowSettingsCategory::OnSettingChanged - No new update avaliable, last checked %s", g_boxeeVersionUpdateManager.GetLastCheckedTime().c_str()); 
    }
  }
  else if (strSetting.Equals("timezone.country"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    m_tzChosenCountryName = pControl->GetCurrentLabel();
    m_needToLoadTimezoneCitiesControl = true;
  }
  else if (strSetting.Equals("timezone.city"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    m_tzChosenCityName = pControl->GetCurrentLabel();
  }
  else if (strSetting.Equals("ftu.run"))
  {
    CInitializeBoxManager::GetInstance().Run(true);
  }
#endif
  else if (strSetting.Equals("videoplayer.hwaccel"))
  {
    g_guiSettings.SetInt("videoplayer.rendermethod", RENDER_METHOD_AUTO);
  }

  else if (strSetting.Equals("debug.syslogenabled"))
  {
    CGUIEditControl *pControl = (CGUIEditControl *)GetControl(GetSetting("debug.syslogaddr")->GetID());
    if (pControl)
        pControl->SetEnabled(g_guiSettings.GetBool("debug.syslogenabled"));

    CLog::ResetSyslogServer();
  }
  else if (strSetting.Equals("debug.syslogaddr"))
  {

    struct hostent *host = gethostbyname(g_guiSettings.GetString("debug.syslogaddr").c_str());
    if (host == NULL)
    {
      if (g_guiSettings.GetString("debug.syslogaddr").size() > 0)
      {
        CGUIDialogOK2::ShowAndGetInput(54094, 54111);
      }
    }

    CLog::ResetSyslogServer();
  }
  else if (strSetting.Equals("browser.clearcookies"))
  {
    if (CGUIDialogYesNo2::ShowAndGetInput(51925, 54898))
    {
      CStdString cookieFile(_P("special://profile/browser/cookies.dat"));
      CLog::Log(LOGINFO, "Deleting browser cookies file '%s'\n", cookieFile.c_str());
      XFILE::CFile::Delete(cookieFile);
    }
  }
  else if (strSetting.Equals("browser.clearlocalstorage"))
  {
    if (CGUIDialogYesNo2::ShowAndGetInput(51925, 90211))
        {
          CStdString localStorageDir(_P("special://profile/browser/localstorage/"));
          CLog::Log(LOGINFO, "Deleting browser local storage '%s'\n", localStorageDir.c_str());
          CUtil::WipeDir(localStorageDir);
        }
  }
  else if (strSetting.Equals("browser.clearcache"))
  {
    if (CGUIDialogYesNo2::ShowAndGetInput(51925, 54899))
    {
#ifdef CANMORE
      CStdString cacheDir("/tmp/browser_cache/");
#else
      CStdString cacheDir(_P("special://profile/browser/"));
#endif

    // delete sub dirs and not the dir itself as it may contain
    // the cookie file that we don't want to delete here

      CLog::Log(LOGINFO, "Deleting browser cache dir '%s'\n", cacheDir.c_str());

      CFileItemList items;
      CDirectory::GetDirectory(cacheDir, items);
      for (int i=0; i < items.Size(); i++)
      {
        CFileItemPtr pItem = items[i];
        struct stat fileStat;
        int rc = lstat(pItem->m_strPath.c_str(), &fileStat);  // TODO: Note that on Windows links would not be handled properly - since this is actually stat().
        if (rc == 0 && S_ISDIR(fileStat.st_mode))
        {
          CUtil::WipeDir(pItem->m_strPath);
          CLog::Log(LOGINFO, "%s was deleted", pItem->m_strPath.c_str());
        }
      }
    }
  }
  else if (strSetting.Equals("browser.clearflashsharedobjects"))
  {
    if (CGUIDialogYesNo2::ShowAndGetInput(51925, 54779))
    {
      CStdString cacheDir("/data/.macromedia/");

    // delete sub dirs and not the dir itself as it may contain
    // the cookie file that we don't want to delete here

      CLog::Log(LOGINFO, "Deleting Flash Shared Objects '%s'\n", cacheDir.c_str());

      CFileItemList items;
      CDirectory::GetDirectory(cacheDir, items);
      for (int i=0; i < items.Size(); i++)
      {
        CFileItemPtr pItem = items[i];
        struct stat fileStat;
        int rc = -1;
        rc = lstat(pItem->m_strPath.c_str(), &fileStat);
        CUtil::WipeDir(pItem->m_strPath);
        CLog::Log(LOGINFO, "%s was deleted", pItem->m_strPath.c_str());
      }
    }
  }
#ifdef HAS_DVB
  else if (strSetting.Equals("ota.reconfigure"))
  {
    if (CGUIWindowBoxeeMain::RunOnBoardingWizardIfNeeded(true))
    {
      g_windowManager.ActivateWindow(WINDOW_BOXEE_LIVETV);
    }
  }
#endif
  else if (strSetting.Equals("server.environment2"))
  {
    //CStdString sett = g_guiSettings.GetSetting("server.environment2");
    CStdString header = g_localizeStrings.Get(90104);
    CStdString line = g_localizeStrings.Get(90105) + g_localizeStrings.Get(51582);
    CStdString noLabel = g_localizeStrings.Get(222);
    CStdString yesLabel = g_localizeStrings.Get(51583);

    //only if user selected yes
    if (CGUIDialogYesNo2::ShowAndGetInput(header,line,noLabel,yesLabel))
    {
      CStdString strHostInput = g_guiSettings.GetString("server.environment2");

      XFILE::CFile host;
      XFILE::CFile hostTmp;
      bool skipLine = false;

      if (!hostTmp.OpenForWrite(FHOST_TEMP_PATH,true))
      {
        CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::OnSettingChanged - server.environment - FAILED to open temp file [%s]",FHOST_TEMP_PATH);
      }
      else
      {
        if (!host.Open(FHOST_PATH))
        {
          CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::OnSettingChanged - server.environment - FAILED to open file [%s]",FHOST_PATH);
          hostTmp.Close();
        }
        else
        {
          //open /etc/hosts
          char line[1024];
          while(host.ReadString(line,1024))
          {
            CStdString strLine(line);

            if(strLine.IsEmpty())
            {
              continue;
            }

            //if switch to production and found #server in /etc/hosts
            //dont copy ip line to new etc/hosts file
            if(skipLine)
            {
              skipLine = false;
              continue;
            }

            //found staging line in file
            if(strLine.Find("#server") != -1)
            {
              skipLine = true;
              continue;
            }
            //write line to new /etc/hosts file
            strLine += "\n";
            hostTmp.Write(strLine ,strLine.size());
          }
          //switch to staging
          //needs to write to /etc/hosts to work with new ip
          if(!strHostInput.IsEmpty())
          {
            CStdString serverLine = "#server\n";
            serverLine += strHostInput;
            serverLine += " ";
            serverLine += HOSTS_LINE;

            hostTmp.Write(serverLine,serverLine.size());
          }

          host.Close();
          hostTmp.Close();

          //overwrite etc/hosts file to match the current environment
          CFile::Cache(FHOST_TEMP_PATH,FHOST_PATH);
          hostTmp.Delete(FHOST_TEMP_PATH);
        }
      }
      //restart boxee
      ThreadMessage tMsg(TMSG_QUIT);
      g_application.getApplicationMessenger().SendMessage(tMsg,true);
    }
  }
  else if (strSetting.Equals("server.environment"))
  {
    CGUIDialogSelect *pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);

    pDlgSelect->SetHeading(90103);
    pDlgSelect->Reset();

    pDlgSelect->Add(g_localizeStrings.Get(90101));  //Staging
    pDlgSelect->Add(g_localizeStrings.Get(90102));  //Production
    pDlgSelect->DoModal();

    CStdString selectedLabel = pDlgSelect->GetSelectedLabelText();

    //only if a different environment was selected
    if(!selectedLabel.Equals(g_guiSettings.GetString("server.environment")))
    {
      //open are you sure dialog
      CStdString header = g_localizeStrings.Get(90104);
      CStdString line = g_localizeStrings.Get(90105) + g_localizeStrings.Get(51582);
      CStdString noLabel = g_localizeStrings.Get(222);
      CStdString yesLabel = g_localizeStrings.Get(51583);

      //only if user selected yes
      if (CGUIDialogYesNo2::ShowAndGetInput(header,line,noLabel,yesLabel))
      {
        bool isStaging = false;
        XFILE::CFile host;
        XFILE::CFile hostTmp;
        bool foundServer = false;
        bool skipLine = false;

        isStaging = (pDlgSelect->GetSelectedLabel() == STAGING_ON);

        g_guiSettings.SetString("server.environment",selectedLabel);
        g_settings.Save();

        if (!hostTmp.OpenForWrite(FHOST_TEMP_PATH,true))
        {
          CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::OnSettingChanged - server.environment - FAILED to open temp file [%s]",FHOST_TEMP_PATH);
        }
        else
        {
          if (!host.Open(FHOST_PATH))
          {
            CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::OnSettingChanged - server.environment - FAILED to open file [%s]",FHOST_PATH);
            hostTmp.Close();
          }
          else
          {
            //open /etc/hosts
            char line[1024];
            while(host.ReadString(line,1024))
            {
              CStdString strLine(line);

              if(strLine.IsEmpty())
              {
                continue;
              }

              //if switch to production and found #server in /etc/hosts
              //dont copy ip line to new etc/hosts file
              if(skipLine)
              {
                skipLine = false;
                continue;
              }

              //found staging line in file
              if(strLine.Find("#server") != -1)
              {
                foundServer = true;
                if(!isStaging)
                {
                  skipLine = true;
                  continue;
                }
              }
              //write line to new /etc/hosts file
              hostTmp.Write(strLine,strLine.size());
            }

            //switch to staging
            //needs to write to /etc/hosts to work with new ip
            if(!foundServer && isStaging)
            {
              CStdString serverLine = "#server\n";
              serverLine += STAGING_LINE;

              hostTmp.Write(serverLine,serverLine.size());
            }

            host.Close();
            hostTmp.Close();

            //overwrite etc/hosts file to match the current environment
            CFile::Cache(FHOST_TEMP_PATH,FHOST_PATH);

            hostTmp.Delete(FHOST_TEMP_PATH);
          }
        }
        //restart boxee
        ThreadMessage tMsg(TMSG_QUIT);
        g_application.getApplicationMessenger().SendMessage(tMsg,true);
      }
    }

    pDlgSelect->Close();
  }
  else if (strSetting.Equals("weather.settings"))
  {
    CAppManager::GetInstance().Launch("app://accuweather/launch?settings=1");
    return;
  }

  UpdateSettings();
}

void CGUIWindowSettingsCategory::FreeControls()
{
  // clear the category group
  CGUIControlGroupList *control = (CGUIControlGroupList *)GetControl(CATEGORY_GROUP_ID);
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }
  m_vecSections.clear();
  FreeSettingsControls();
}

void CGUIWindowSettingsCategory::FreeSettingsControls()
{
  // clear the settings group
  CGUIControlGroupList *control = (CGUIControlGroupList *)GetControl(SETTINGS_GROUP_ID);
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }

  for(int i = 0; (size_t)i < m_vecSettings.size(); i++)
  {
    delete m_vecSettings[i];
  }
  m_vecSettings.clear();
}

void CGUIWindowSettingsCategory::AddSetting(CSetting *pSetting, float width, int &iControlID)
{
  if (!pSetting->IsVisible()) return;  // not displayed in current session
  CBaseSettingControl *pSettingControl = NULL;
  CGUIControl *pControl = NULL;
  if (pSetting->GetControlType() == CHECKMARK_CONTROL)
  {
    pControl = new CGUIRadioButtonControl(*m_pOriginalRadioButton);
    if (!pControl) return ;
    ((CGUIRadioButtonControl *)pControl)->SetLabel(pSetting->GetLabelStr());
    pControl->SetWidth(width);
    pSettingControl = new CRadioButtonSettingControl((CGUIRadioButtonControl *)pControl, iControlID, pSetting);
  }
  else if (pSetting->GetControlType() == SPIN_CONTROL_FLOAT || pSetting->GetControlType() == SPIN_CONTROL_INT_PLUS || pSetting->GetControlType() == SPIN_CONTROL_TEXT || pSetting->GetControlType() == SPIN_CONTROL_INT)
  {
    pControl = new CGUISpinControlEx(*m_pOriginalSpin);
    if (!pControl) return ;
    pControl->SetWidth(width);
    ((CGUISpinControlEx *)pControl)->SetText(pSetting->GetLabelStr());
    pSettingControl = new CSpinExSettingControl((CGUISpinControlEx *)pControl, iControlID, pSetting);
  }
  else if (pSetting->GetControlType() == SEPARATOR_CONTROL && m_pOriginalImage)
  {
    pControl = new CGUIImage(*m_pOriginalImage);
    if (!pControl) return;
    pControl->SetWidth(width);
    pSettingControl = new CSeparatorSettingControl((CGUIImage *)pControl, iControlID, pSetting);
  }
  else if (pSetting->GetControlType() == EDIT_CONTROL_INPUT ||
           pSetting->GetControlType() == EDIT_CONTROL_HIDDEN_INPUT ||
           pSetting->GetControlType() == EDIT_CONTROL_NUMBER_INPUT ||
           pSetting->GetControlType() == EDIT_CONTROL_IP_INPUT)
  {
    pControl = new CGUIEditControl(*m_pOriginalEdit);
    if (!pControl) return ;
    ((CGUIEditControl *)pControl)->SettingsCategorySetTextAlign(XBFONT_CENTER_Y);
    ((CGUIEditControl *)pControl)->SetLabel(pSetting->GetLabelStr());
    pControl->SetWidth(width);
    pSettingControl = new CEditSettingControl((CGUIEditControl *)pControl, iControlID, pSetting);
  }
  else if (pSetting->GetControlType() != SEPARATOR_CONTROL) // button control
  {
    pControl = new CGUIButtonControl(*m_pOriginalButton);
    if (!pControl) return ;
    ((CGUIButtonControl *)pControl)->SettingsCategorySetTextAlign(XBFONT_CENTER_Y);
    ((CGUIButtonControl *)pControl)->SetLabel(pSetting->GetLabelStr());
    pControl->SetWidth(width);
    pSettingControl = new CButtonSettingControl((CGUIButtonControl *)pControl, iControlID, pSetting);
  }
  if (!pControl) return;
  pControl->SetID(iControlID++);
  pControl->SetVisible(true);
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(SETTINGS_GROUP_ID);
  if (group)
  {
    pControl->AllocResources();
    group->AddControl(pControl);
    m_vecSettings.push_back(pSettingControl);
  }
}

void CGUIWindowSettingsCategory::Render()
{
  // update realtime changeable stuff
  UpdateRealTimeSettings();
  // update alpha status of current button
  bool bAlphaFaded = false;
  CGUIControl *control = GetFirstFocusableControl(CONTROL_START_BUTTONS + m_iSection);
  if (control && !control->HasFocus())
  {
    if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
    {
      control->SetFocus(true);
      ((CGUIButtonControl *)control)->SetAlpha(0x80);
      bAlphaFaded = true;
    }
    else if (control->GetControlType() == CGUIControl::GUICONTROL_TOGGLEBUTTON)
    {
      control->SetFocus(true);
      ((CGUIButtonControl *)control)->SetSelected(true);
      bAlphaFaded = true;
    }
  }
  CGUIWindow::Render();
  if (bAlphaFaded)
  {
    control->SetFocus(false);
    if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
      ((CGUIButtonControl *)control)->SetAlpha(0xFF);
    else
      ((CGUIButtonControl *)control)->SetSelected(false);
  }
  // render the error message if necessary
  if (m_strErrorMessage.size())
  {
    CGUIFont *pFont = g_fontManager.GetFont("font13");
    float fPosY = g_graphicsContext.GetHeight() * 0.8f;
    float fPosX = g_graphicsContext.GetWidth() * 0.5f;
    CGUITextLayout::DrawText(pFont, fPosX, fPosY, 0xffffffff, 0, m_strErrorMessage, XBFONT_CENTER_X);
  }
#ifdef HAS_EMBEDDED
  UpdateDownloadStatus();
#endif
#ifdef HAS_DVB
  UpdateOtaStatus();
#endif
}

#ifdef HAS_EMBEDDED
void CGUIWindowSettingsCategory::CheckTimezoneSettings()
{
  CLog::Log(LOGDEBUG,"CGUIWindowSettingsCategory::CheckTimezoneSettings timezone city current %s new %s\n", g_guiSettings.GetString("timezone.city").c_str(), m_tzChosenCityName.c_str());
  CLog::Log(LOGDEBUG,"CGUIWindowSettingsCategory::CheckTimezoneSettings timezone country current %s new %s\n", g_guiSettings.GetString("timezone.country").c_str(), m_tzChosenCountryName.c_str());

  if (m_tzChosenCityName != g_guiSettings.GetString("timezone.city") || m_tzChosenCountryName != g_guiSettings.GetString("timezone.country"))
  {
    g_guiSettings.SetString("timezone.country",m_tzChosenCountryName);
    g_guiSettings.SetString("timezone.city",m_tzChosenCityName);

    CStdString cityName = m_tzChosenCityName;
    cityName.Replace(" ","_");

    CStdString timezonePath = m_tzCityToPathMap[cityName];
    if (timezonePath.IsEmpty())
    {
      CLog::Log(LOGDEBUG,"CGUIWindowSettingsCategory::CheckTimezoneSettings - FAILED to get timezone path for [ChosenCityName=%s] (tz)",m_tzChosenCityName.c_str());
    }
    else if (!CHalServicesFactory::GetInstance().SetTimezone(timezonePath))
    {
      CLog::Log(LOGDEBUG,"CGUIWindowSettingsCategory::CheckTimezoneSettings - FAILED to set Timezone to [timezonePath=%s] (tz)",timezonePath.c_str());
    }
  }
}
#endif

void CGUIWindowSettingsCategory::CheckNetworkSettings()
{
#ifdef HAS_EMBEDDED
  if (m_bSmbServerEnable != g_guiSettings.GetBool("smbd.enable") ||
      m_smbPassword != g_guiSettings.GetString("smbd.password") ||
      m_smbWorkgroup != g_guiSettings.GetString("smbd.workgroup") ||
      m_smbHostname != g_guiSettings.GetString("server.hostname"))
  {
    m_bSmbServerEnable = g_guiSettings.GetBool("smbd.enable");
    m_smbPassword = g_guiSettings.GetString("smbd.password");
    m_smbWorkgroup = g_guiSettings.GetString("smbd.workgroup");
    m_smbHostname = g_guiSettings.GetString("server.hostname");

    if (m_bSmbServerEnable)
    {
      CLog::Log(LOGDEBUG,"CGUIWindowSettingsCategory::CheckNetworkSettings - Going to run SMB server. [SmbServerEnable=%d][HostName=%s][password=%s][smbWorkgroup=%s] (smbd)",m_bSmbServerEnable,m_smbHostname.c_str(),g_guiSettings.GetString("smbd.password").c_str(),m_smbWorkgroup.c_str());
      g_application.StartSmbServer();
    }
    else
    {
      CLog::Log(LOGDEBUG,"CGUIWindowSettingsCategory::CheckNetworkSettings - Going to stop SMB server. [SmbServerEnable=%d][HostName=%s][password=%s][smbWorkgroup=%s] (smbd)",m_bSmbServerEnable,m_smbHostname.c_str(),g_guiSettings.GetString("smbd.password").c_str(),m_smbWorkgroup.c_str());
      g_application.StopSmbServer();
    }
  }

//  if (m_smbHostname != g_guiSettings.GetString("smbd.hostname"))
//  {
//    m_smbHostname = g_guiSettings.GetString("smbd.hostname");
//    CLog::Log(LOGDEBUG,"CGUIWindowSettingsCategory::CheckNetworkSettings - hostname changed [m_smbHostname=%s] (smbd)", m_smbHostname.c_str());
//    g_application.SetHostname();
//  }
#endif

  // Check if the user accidentally set the proxy to true but did not enter the host
  if (g_guiSettings.GetBool("network.usehttpproxy") && g_guiSettings.GetString("network.httpproxyserver").IsEmpty())
  {
    CGUIDialogOK2::ShowAndGetInput(54093, 714);
    g_guiSettings.SetBool("network.usehttpproxy", false);
    return;
  }

//  else if (strSetting.Equals("network.httpproxyserver")   || strSetting.Equals("network.httpproxyport") ||
//              strSetting.Equals("network.httpproxyusername") || strSetting.Equals("network.httpproxypassword"))
//     {
//       CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
//       if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("network.usehttpproxy"));
//       g_application.SetupHttpProxy();
//     }


  if (!g_application.IsStandAlone())
    return;

  // check if our network needs restarting (requires a reset, so check well!)
  if (m_iNetworkAssignment == -1)
  {
    // nothing to do here, folks - move along.
    return ;
  }
  // we need a reset if:
  // 1.  The Network Assignment has changed OR
  // 2.  The Network Assignment is STATIC and one of the network fields have changed
  if (m_iNetworkAssignment != g_guiSettings.GetInt("network.assignment") ||
      (m_iNetworkAssignment == NETWORK_STATIC && (
         m_strNetworkIPAddress != g_guiSettings.GetString("network.ipaddress") ||
         m_strNetworkSubnet != g_guiSettings.GetString("network.subnet") ||
         m_strNetworkGateway != g_guiSettings.GetString("network.gateway") ||
         m_strNetworkDNS != g_guiSettings.GetString("network.dns"))))
  {
/*    // our network settings have changed - we should prompt the user to reset XBMC
    if (CGUIDialogYesNo2::ShowAndGetInput(14038, 54765))
    {
      // reset settings
      g_application.getApplicationMessenger().RestartApp();
      // Todo: aquire new network settings without restart app!
    }
    else*/

    // update our settings variables    
    m_iNetworkAssignment = g_guiSettings.GetInt("network.assignment");
    m_strNetworkIPAddress = g_guiSettings.GetString("network.ipaddress");
    m_strNetworkSubnet = g_guiSettings.GetString("network.subnet");
    m_strNetworkGateway = g_guiSettings.GetString("network.gateway");
    m_strNetworkDNS = g_guiSettings.GetString("network.dns");

    // replace settings
    /*   g_guiSettings.SetInt("network.assignment", m_iNetworkAssignment);
       g_guiSettings.SetString("network.ipaddress", m_strNetworkIPAddress);
       g_guiSettings.SetString("network.subnet", m_strNetworkSubnet);
       g_guiSettings.SetString("network.gateway", m_strNetworkGateway);
       g_guiSettings.SetString("network.dns", m_strNetworkDNS);*/
  }
}

void CGUIWindowSettingsCategory::CheckRuntimeSettings()
{
  if (g_application.m_bStop)
  {
	return;
  }
  if (m_containRuntimeCategory)
  {
    for (unsigned int i = 0; i < m_vecSections.size(); i++)
    {
      if (m_vecSections[i]->m_runTime)
      {
    	switch (m_vecSections[i]->m_labelID)
    	{
    	case FEEDS_CATEGORY:
    	{
    	  ApplyPersonalFeeds();
    	  break;
    	}
    	default:
    	{

    	}
    	}

      }
    }
  }

}


void CGUIWindowSettingsCategory::FillInSubtitleHeights(CSetting *pSetting)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  if (CUtil::IsUsingTTFSubtitles())
  { // easy - just fill as per usual
    CStdString strLabel;
    for (int i = pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i += pSettingInt->m_iStep)
    {
      if (pSettingInt->m_iFormat > -1)
      {
        CStdString strFormat = g_localizeStrings.Get(pSettingInt->m_iFormat);
        strLabel.Format(strFormat, i);
      }
      else
        strLabel.Format(pSettingInt->m_strFormat, i);
      pControl->AddLabel(strLabel, i);
    }
    pControl->SetValue(pSettingInt->GetData());
  }
        }

void CGUIWindowSettingsCategory::FillInSubtitleFonts(CSetting *pSetting)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  int iCurrentFont = 0;
  int iFont = 0;

  // find TTF fonts
  {
    CFileItemList items;
    if (CDirectory::GetDirectory("special://xbmc/media/Fonts/", items))
    {
      for (int i = 0; i < items.Size(); ++i)
      {
        CFileItemPtr pItem = items[i];

        if (!pItem->m_bIsFolder)
        {

          if ( !CUtil::GetExtension(pItem->GetLabel()).Equals(".ttf") ) continue;
          if (pItem->GetLabel().Equals(pSettingString->GetData(), false))
            iCurrentFont = iFont;

          pControl->AddLabel(pItem->GetLabel(), iFont++);
        }

      }
    }
  }
  
  pControl->SetValue(iCurrentFont);
}

void CGUIWindowSettingsCategory::FillInSkinFonts(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();

  int iSkinFontSet = 0;

  m_strNewSkinFontSet.Empty();

  RESOLUTION res;
  CStdString strPath = g_SkinInfo.GetSkinPath("Font.xml", &res);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(strPath))
  {
    CLog::Log(LOGERROR, "Couldn't load %s", strPath.c_str());
    return ;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();

  CStdString strValue = pRootElement->Value();
  if (strValue != CStdString("fonts"))
  {
    CLog::Log(LOGERROR, "file %s doesnt start with <fonts>", strPath.c_str());
    return ;
  }

  const TiXmlNode *pChild = pRootElement->FirstChild();
  strValue = pChild->Value();
  if (strValue == "fontset")
  {
    while (pChild)
    {
      strValue = pChild->Value();
      if (strValue == "fontset")
      {
        const char* idAttr = ((TiXmlElement*) pChild)->Attribute("id");
        const char* unicodeAttr = ((TiXmlElement*) pChild)->Attribute("unicode");

        bool isUnicode=(unicodeAttr && stricmp(unicodeAttr, "true") == 0);

        bool isAllowed=true;
        if (g_langInfo.ForceUnicodeFont() && !isUnicode)
          isAllowed=false;

        if (idAttr != NULL && isAllowed)
        {
          pControl->AddLabel(idAttr, iSkinFontSet);
          if (strcmpi(idAttr, g_guiSettings.GetString("lookandfeel.font").c_str()) == 0)
            pControl->SetValue(iSkinFontSet);
          iSkinFontSet++;
        }
      }
      pChild = pChild->NextSibling();
    }

  }
  else
  {
    // Since no fontset is defined, there is no selection of a fontset, so disable the component
    pControl->AddLabel(g_localizeStrings.Get(13278), 1);
    pControl->SetValue(1);
    pControl->SetEnabled(false);
  }
}

void CGUIWindowSettingsCategory::FillInSkins(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  pControl->SetShowRange(true);

  m_strNewSkin.Empty();

  //find skins...
  CFileItemList items;
  CDirectory::GetDirectory("special://xbmc/skin/", items);
  if (!CSpecialProtocol::XBMCIsHome())
    CDirectory::GetDirectory("special://home/skin/", items);

  int iCurrentSkin = 0;
  int iSkin = 0;
  vector<CStdString> vecSkins;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (pItem->m_bIsFolder)
    {
      if (strcmpi(pItem->GetLabel().c_str(), ".svn") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "fonts") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "media") == 0) continue;
      //   if (g_SkinInfo.Check(pItem->m_strPath))
      //   {
      vecSkins.push_back(pItem->GetLabel());
      //   }
    }
  }

  sort(vecSkins.begin(), vecSkins.end(), sortstringbyname());
  for (unsigned int i = 0; i < vecSkins.size(); ++i)
  {
    CStdString strSkin = vecSkins[i];
    if (strcmpi(strSkin.c_str(), g_guiSettings.GetString("lookandfeel.skin").c_str()) == 0)
    {
      iCurrentSkin = iSkin;
    }
    pControl->AddLabel(strSkin, iSkin++);
  }
  pControl->SetValue(iCurrentSkin);
  return ;
}

void CGUIWindowSettingsCategory::FillInSoundSkins(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  //pControl->SetShowRange(true);

  m_strNewSkin.Empty();

  //find skins...
  CFileItemList items;
  CDirectory::GetDirectory("special://xbmc/sounds/", items);
  if (!CSpecialProtocol::XBMCIsHome())
    CDirectory::GetDirectory("special://home/sounds/", items);

  int iCurrentSoundSkin = 0;
  int iSoundSkin = 0;
  vector<CStdString> vecSoundSkins;
  int i;
  for (i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (pItem->m_bIsFolder)
    {
      if (strcmpi(pItem->GetLabel().c_str(), ".svn") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "fonts") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "media") == 0) continue;
      vecSoundSkins.push_back(pItem->GetLabel());
    }
  }

  pControl->AddLabel(g_localizeStrings.Get(474), iSoundSkin++); // Off
  pControl->AddLabel(g_localizeStrings.Get(54755), iSoundSkin++); // Skin Default

  if (g_guiSettings.GetString("lookandfeel.soundskin")=="SKINDEFAULT")
    iCurrentSoundSkin=1;

  /* 051010 - currently we are using only 2 options
  sort(vecSoundSkins.begin(), vecSoundSkins.end(), sortstringbyname());
  for (i = 0; i < (int) vecSoundSkins.size(); ++i)
  {
    CStdString strSkin = vecSoundSkins[i];
    if (strcmpi(strSkin.c_str(), g_guiSettings.GetString("lookandfeel.soundskin").c_str()) == 0)
    {
      iCurrentSoundSkin = iSoundSkin;
    }
    pControl->AddLabel(strSkin, iSoundSkin++);
  }
  */

  pControl->SetValue(iCurrentSoundSkin);
  return ;
}

void CGUIWindowSettingsCategory::FillInCharSets(CSetting *pSetting)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  int iCurrentCharset = 0;
  vector<CStdString> vecCharsets = g_charsetConverter.getCharsetLabels();

  CStdString strCurrentCharsetLabel="DEFAULT";
  if (pSettingString->GetData()!="DEFAULT")
    strCurrentCharsetLabel = g_charsetConverter.getCharsetLabelByName(pSettingString->GetData());

  sort(vecCharsets.begin(), vecCharsets.end(), sortstringbyname());

  vecCharsets.insert(vecCharsets.begin(), g_localizeStrings.Get(13278)); // "Default"

  bool bIsAuto=(pSettingString->GetData()=="DEFAULT");

  for (int i = 0; i < (int) vecCharsets.size(); ++i)
  {
    CStdString strCharsetLabel = vecCharsets[i];

    if (!bIsAuto && strCharsetLabel == strCurrentCharsetLabel)
      iCurrentCharset = i;

    pControl->AddLabel(strCharsetLabel, i);
  }

  pControl->SetValue(iCurrentCharset);
}

void CGUIWindowSettingsCategory::FillInPrefferedLanguage(CSetting *pSetting, CStdString id)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();
  int iCurrentSubtitlesPreferredLang = 0;
  CStdString strCurrentSubtitlesPreferredLang = g_guiSettings.GetString(id);

  for (int i = 0; i < (int)g_settings.m_subtitleLangsVec.size(); ++i)
  {
    CStdString strSubtitleLang = g_settings.m_subtitleLangsVec[i];
    pControl->AddLabel(strSubtitleLang, i);

    CLog::Log(LOGDEBUG,"CGUIWindowSettingsCategory::FillInLanguage - %s [%d/%d] - [%s] (sl)",id.c_str(), i+1,(int)g_settings.m_subtitleLangsVec.size(),strSubtitleLang.c_str());

    if (strCurrentSubtitlesPreferredLang == strSubtitleLang)
    {
      iCurrentSubtitlesPreferredLang = i;
    }
  }

  pControl->SetValue(iCurrentSubtitlesPreferredLang);
}

void CGUIWindowSettingsCategory::FillInVisualisations(CSetting *pSetting, int iControlID)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  if (!pSetting) return;
  int iWinID = g_windowManager.GetActiveWindow();
  {
    CGUIMessage msg(GUI_MSG_LABEL_RESET, iWinID, iControlID);
    g_windowManager.SendMessage(msg);
  }
  vector<CStdString> vecVis;
  //find visz....
  CFileItemList items;
  CDirectory::GetDirectory("special://xbmc/visualisations/", items);
  if (!CSpecialProtocol::XBMCIsHome())
    CDirectory::GetDirectory("special://home/visualisations/", items);

  CVisualisationFactory visFactory;
  CStdString strExtension;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->m_bIsFolder)
    {
      const char *visPath = (const char*)pItem->m_strPath;

      CUtil::GetExtension(pItem->m_strPath, strExtension);
      if (strExtension == ".vis")  // normal visualisation
      {
#ifdef _LINUX
        void *handle = dlopen( _P(visPath).c_str(), RTLD_LAZY );
        if (!handle)
          continue;
        dlclose(handle);
#endif
        CStdString strLabel = pItem->GetLabel();
        vecVis.push_back( CVisualisation::GetFriendlyName( strLabel ) );
      }
      else if ( strExtension == ".mvis" )  // multi visualisation with sub modules
      {
        CVisualisation* vis = visFactory.LoadVisualisation( visPath );
        if ( vis )
        {
          map<string, string> subModules;
          map<string, string>::iterator iter;
          string moduleName, path;
          CStdString visName = pItem->GetLabel();
          visName = visName.Mid(0, visName.size() - 5);

          // get list of sub modules from the visualisation
          vis->GetSubModules( subModules );

          for ( iter=subModules.begin() ; iter!=subModules.end() ; iter++ )
          {
            // each pair of the map is of the format 'module name' => 'module path'
            moduleName = iter->first;
            vecVis.push_back( CVisualisation::GetFriendlyName( visName.c_str(), moduleName.c_str() ).c_str() );
            CLog::Log(LOGDEBUG, "Module %s for visualisation %s", moduleName.c_str(), visPath);
          }

          delete vis;
        }
      }
    }
  }

  CStdString strDefaultVis = pSettingString->GetData();
  if (!strDefaultVis.Equals("None"))
    strDefaultVis = CVisualisation::GetFriendlyName( strDefaultVis );

  sort(vecVis.begin(), vecVis.end(), sortstringbyname());

  // add the "disabled" setting first
  int iVis = 0;
  int iCurrentVis = 0;
  {
    CGUIMessage msg(GUI_MSG_LABEL_ADD, iWinID, iControlID, iVis++);
    msg.SetLabel(231);
    g_windowManager.SendMessage(msg);
  }
  for (int i = 0; i < (int) vecVis.size(); ++i)
  {
    CStdString strVis = vecVis[i];

    if (strcmpi(strVis.c_str(), strDefaultVis.c_str()) == 0)
      iCurrentVis = iVis;

    {
      CGUIMessage msg(GUI_MSG_LABEL_ADD, iWinID, iControlID, iVis++);
      msg.SetLabel(strVis);
      g_windowManager.SendMessage(msg);
    }
  }
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, iWinID, iControlID, iCurrentVis);
    g_windowManager.SendMessage(msg);
  }
}

void CGUIWindowSettingsCategory::FillInVoiceMasks(DWORD dwPort, CSetting *pSetting)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetShowRange(true); // show the range
  int iCurrentMask = 0;
  int iMask = 0;
  vector<CStdString> vecMask;

  //find masks in xml...
  TiXmlDocument xmlDoc;
  CStdString fileName = "special://xbmc/system/voicemasks.xml";
  if ( !xmlDoc.LoadFile(fileName) ) return ;
  TiXmlElement* pRootElement = xmlDoc.RootElement();
  CStdString strValue = pRootElement->Value();
  if ( strValue != "VoiceMasks") return ;
  if (pRootElement)
  {
    const TiXmlNode *pChild = pRootElement->FirstChild("Name");
    while (pChild)
    {
      if (pChild->FirstChild())
      {
        CStdString strName = pChild->FirstChild()->Value();
        vecMask.push_back(strName);
      }
      pChild = pChild->NextSibling("Name");
    }
  }
  xmlDoc.Clear();


  CStdString strDefaultMask = pSettingString->GetData();

  sort(vecMask.begin(), vecMask.end(), sortstringbyname());
//  CStdString strCustom = "Custom";
  CStdString strNone = "None";
//  vecMask.insert(vecMask.begin(), strCustom);
  vecMask.insert(vecMask.begin(), strNone);
  for (int i = 0; i < (int) vecMask.size(); ++i)
  {
    CStdString strMask = vecMask[i];

    if (strcmpi(strMask.c_str(), strDefaultMask.c_str()) == 0)
      iCurrentMask = iMask;

    pControl->AddLabel(strMask, iMask++);
  }

  pControl->SetValue(iCurrentMask);
}

void CGUIWindowSettingsCategory::FillInVoiceMaskValues(DWORD dwPort, CSetting *pSetting)
{
  CStdString strCurMask = g_guiSettings.GetString(pSetting->GetSetting());
  if (strCurMask.CompareNoCase("None") == 0 || strCurMask.CompareNoCase("Custom") == 0 )
  {
#define XVOICE_MASK_PARAM_DISABLED (-1.0f)
    g_stSettings.m_karaokeVoiceMask[dwPort].energy = XVOICE_MASK_PARAM_DISABLED;
    g_stSettings.m_karaokeVoiceMask[dwPort].pitch = XVOICE_MASK_PARAM_DISABLED;
    g_stSettings.m_karaokeVoiceMask[dwPort].whisper = XVOICE_MASK_PARAM_DISABLED;
    g_stSettings.m_karaokeVoiceMask[dwPort].robotic = XVOICE_MASK_PARAM_DISABLED;
    return;
  }

  //find mask values in xml...
  TiXmlDocument xmlDoc;
  CStdString fileName = "special://xbmc/system/voicemasks.xml";
  if ( !xmlDoc.LoadFile( fileName ) ) return ;
  TiXmlElement* pRootElement = xmlDoc.RootElement();
  CStdString strValue = pRootElement->Value();
  if ( strValue != "VoiceMasks") return ;
  if (pRootElement)
  {
    const TiXmlNode *pChild = pRootElement->FirstChild("Name");
    while (pChild)
    {
      CStdString strMask = pChild->FirstChild()->Value();
      if (strMask.CompareNoCase(strCurMask) == 0)
      {
        for (int i = 0; i < 4;i++)
        {
          pChild = pChild->NextSibling();
          if (pChild)
          {
            CStdString strValue = pChild->Value();
            if (strValue.CompareNoCase("fSpecEnergyWeight") == 0)
            {
              if (pChild->FirstChild())
              {
                CStdString strName = pChild->FirstChild()->Value();
                g_stSettings.m_karaokeVoiceMask[dwPort].energy = (float) atof(strName.c_str());
              }
            }
            else if (strValue.CompareNoCase("fPitchScale") == 0)
            {
              if (pChild->FirstChild())
              {
                CStdString strName = pChild->FirstChild()->Value();
                g_stSettings.m_karaokeVoiceMask[dwPort].pitch = (float) atof(strName.c_str());
              }
            }
            else if (strValue.CompareNoCase("fWhisperValue") == 0)
            {
              if (pChild->FirstChild())
              {
                CStdString strName = pChild->FirstChild()->Value();
                g_stSettings.m_karaokeVoiceMask[dwPort].whisper = (float) atof(strName.c_str());
              }
            }
            else if (strValue.CompareNoCase("fRoboticValue") == 0)
            {
              if (pChild->FirstChild())
              {
                CStdString strName = pChild->FirstChild()->Value();
                g_stSettings.m_karaokeVoiceMask[dwPort].robotic = (float) atof(strName.c_str());
              }
            }
          }
        }
        break;
      }
      pChild = pChild->NextSibling("Name");
    }
  }
  xmlDoc.Clear();
}

void CGUIWindowSettingsCategory::FillInLogLevels(CSetting *pSetting)
{  
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();
  
  pControl->AddLabel(g_localizeStrings.Get(54090), LOGDEBUG);  
  pControl->AddLabel(g_localizeStrings.Get(54091), LOGINFO);  
  pControl->AddLabel(g_localizeStrings.Get(54092), LOGNOTICE);  
  pControl->AddLabel(g_localizeStrings.Get(54093), LOGWARNING);  
  pControl->AddLabel(g_localizeStrings.Get(54094), LOGERROR);  
  pControl->AddLabel(g_localizeStrings.Get(54095), LOGSEVERE);  
  pControl->AddLabel(g_localizeStrings.Get(54096), LOGFATAL);  
  pControl->AddLabel(g_localizeStrings.Get(54097), LOGNONE); 
  
  pControl->SetValue(CLog::m_logLevel);
}

void CGUIWindowSettingsCategory::FillInResolutions(CSetting *pSetting, bool playbackSetting)
{
#ifdef __APPLE__
  g_Windowing.UpdateResolutions();
#endif
  
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

#ifndef HAS_EMBEDDED
  pControl->AddLabel(g_settings.m_ResInfo[RES_WINDOW].strMode, RES_WINDOW);  
#endif
  pControl->AddLabel(g_settings.m_ResInfo[RES_DESKTOP].strMode, RES_DESKTOP);
  size_t maxRes = g_settings.m_ResInfo.size();
#ifdef _WIN32
  if (g_Windowing.GetNumScreens())
    maxRes = std::min(maxRes, (size_t)RES_DESKTOP + g_Windowing.GetNumScreens());
#endif
  for (size_t i = RES_CUSTOM ; i < maxRes; i++)
  {
    pControl->AddLabel(g_settings.m_ResInfo[i].strMode, i);
  }

  if (pSettingInt->GetData() >= (int)g_settings.m_ResInfo.size())
    pControl->SetValue(RES_DESKTOP);
  else 
  pControl->SetValue(pSettingInt->GetData());
  
  m_NewResolution = g_graphicsContext.GetVideoResolution();
}

void CGUIWindowSettingsCategory::FillInVSyncs(CSetting *pSetting)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();
#ifndef __APPLE__
  pControl->AddLabel(g_localizeStrings.Get(13101) , VSYNC_DRIVER);
#endif
  pControl->AddLabel(g_localizeStrings.Get(13106) , VSYNC_DISABLED);
  pControl->AddLabel(g_localizeStrings.Get(13107) , VSYNC_VIDEO);
  pControl->AddLabel(g_localizeStrings.Get(13108) , VSYNC_ALWAYS);

  pControl->SetValue(pSettingInt->GetData());
}

void CGUIWindowSettingsCategory::FillInMode3D(CSetting *pSetting)
{
  CSettingInt *pSettingInt = (CSettingInt*) pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *) GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  pControl->AddLabel(g_localizeStrings.Get(54881), MODE_3D_NONE);
  pControl->AddLabel(g_localizeStrings.Get(54882), MODE_3D_SBS);
  pControl->AddLabel(g_localizeStrings.Get(54883), MODE_3D_OU);

  pControl->SetValue(pSettingInt->GetData());
}

void CGUIWindowSettingsCategory::FillInDeinterlacingPolicy(CSetting *pSetting)
{
#ifdef HAS_INTEL_SMD
  CSettingInt *pSettingInt = (CSettingInt*) pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *) GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  pControl->AddLabel(g_localizeStrings.Get(57301), INTERLACE_MODE_AUTO);
  pControl->AddLabel(g_localizeStrings.Get(57302), INTERLACE_MODE_VIDEO);
  pControl->AddLabel(g_localizeStrings.Get(57303), INTERLACE_MODE_FILM);
  pControl->AddLabel(g_localizeStrings.Get(57304), INTERLACE_MODE_SPATIAL_ONLY);
  pControl->AddLabel(g_localizeStrings.Get(57305), INTERLACE_MODE_TOP_FIELD_ONLY);
  pControl->AddLabel(g_localizeStrings.Get(57306), INTERLACE_MODE_SCALE_ONLY);
  pControl->AddLabel(g_localizeStrings.Get(57307), INTERLACE_MODE_NEVER);

  pControl->SetValue(pSettingInt->GetData());
#endif
}

void CGUIWindowSettingsCategory::GetLanguages(std::vector<CStdString> &vecLanguage)
{
  //find languages...
  CFileItemList items;
  CDirectory::GetDirectory("special://xbmc/language/", items);

  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (pItem->m_bIsFolder)
    {
      if (strcmpi(pItem->GetLabel().c_str(), ".svn") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "fonts") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "media") == 0) continue;
      vecLanguage.push_back(pItem->GetLabel());
    }
  }

  sort(vecLanguage.begin(), vecLanguage.end(), sortstringbyname());
}

void CGUIWindowSettingsCategory::FillInTimezoneCountries(CSetting *pSetting)
{
#ifdef HAS_EMBEDDED
  if (m_tzCountryToCodeMap.empty())
  {
    CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::FillInTimezoneCountries - FAILED to fill countries because CountriesMap is empty (tz)");
    return;
  }

  CStdStringArray countries;
  std::map<CStdString, CStdString>::iterator it = m_tzCountryToCodeMap.begin();
  while(it != m_tzCountryToCodeMap.end())
  {
    countries.push_back(it->first);
    it++;
  }

  sort(countries.begin(), countries.end(), sortstringbyname());

  int lineCounter = 0;
  int selectedCountry = 0;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  for (int i=0; i<(int)countries.size(); i++)
  {
    lineCounter++;
    CStdString countryName = countries[i];
    if (countryName == m_tzChosenCountryName)
    {
      selectedCountry = lineCounter;
    }

    pControl->AddLabel(countries[i], lineCounter);
  }

  pControl->SetValue(selectedCountry);
#endif
}

void CGUIWindowSettingsCategory::FillInTimezoneCities(CSetting *pSetting)
{
#ifdef HAS_EMBEDDED
  if (m_tzCodeToCityMultimap.empty())
  {
    CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::FillInTimezoneCities - FAILED to fill cities because CitiesMap is empty (tz)");
    return;
  }

  CStdString chosenCountryCode = m_tzCountryToCodeMap[m_tzChosenCountryName];
  if (m_tzCodeToCityMultimap.find(chosenCountryCode) == m_tzCodeToCityMultimap.end())
  {
    CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::FillInTimezoneCities - FAILED to fill cities because CitiesMap has no entries for [chosenCountryCode=%s]. [ChosenCountryName=%s] (tz)",chosenCountryCode.c_str(),m_tzChosenCountryName.c_str());
    return;
  }

  int lineCounter = 0;
  int selectedCity = 0;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  std::multimap<CStdString, CStdString>::iterator it;
  pair<std::multimap<CStdString, CStdString>::iterator,std::multimap<CStdString, CStdString>::iterator> ret;
  ret = m_tzCodeToCityMultimap.equal_range(chosenCountryCode);
  for (it=ret.first; it!=ret.second; ++it)
  {
    lineCounter++;
    CStdString cityName = it->second;

    cityName.Replace("_"," ");

    if (cityName == m_tzChosenCityName)
    {
      selectedCity = lineCounter;
    }

    CLog::Log(LOGDEBUG,"CGUIWindowSettingsCategory::FillInTimezoneCities - [%d] - Going to add [cityName=%s] to SpinControl. [chosenCountryCode=%s][ChosenCountryName=%s] (tz)",lineCounter,cityName.c_str(),chosenCountryCode.c_str(),m_tzChosenCountryName.c_str());

    pControl->AddLabel(cityName, lineCounter);
  }

  pControl->SetValue(selectedCity);
#endif
}

void CGUIWindowSettingsCategory::FillInHDMIOutput(CSetting *pSetting)
{
#ifdef HAS_EMBEDDED
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  CStdString label;

  for (int i = HDMI_OUTPUT_RGB_LOW; i < HDMI_OUTPUT_COUNT; i++)
  {
    switch(i)
    {
    case HDMI_OUTPUT_RGB_LOW:
      label = ("RGB Low");
      break;
    case HDMI_OUTPUT_RGB_HI:
      label = ("RGB High");
      break;
    case HDMI_OUTPUT_YUV_422:
      label = ("YUV 422");
      break;
    case HDMI_OUTPUT_YUV_444:
      label = ("YUV 444");
      break;
    }

    pControl->AddLabel(label, i);
  }

  pControl->SetValue(g_guiSettings.GetInt("videoscreen.hdmioutput"));
#endif
}

void CGUIWindowSettingsCategory::FillInHDMIPixelDepth(CSetting *pSetting)
{
#ifdef HAS_EMBEDDED
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  CStdString label;

  for (int i = HDMI_PIXEL_DEPTH_24; i < HDMI_PIXEL_DEPTH_COUNT; i++)
  {
    switch(i)
    {
    case HDMI_PIXEL_DEPTH_24:
      label = ("24 Bit");
      break;
    case HDMI_PIXEL_DEPTH_30:
      label = ("30 Bit");
      break;
    case HDMI_PIXEL_DEPTH_36:
      label = ("36 Bit");
      break;
    }

    pControl->AddLabel(label, i);
  }

  pControl->SetValue(g_guiSettings.GetInt("videoscreen.hdmipixeldepth"));
#endif
}

void CGUIWindowSettingsCategory::FillInLanguages(CSetting *pSetting)
{
  CGUIButtonControl* pControl = (CGUIButtonControl *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  m_strNewLanguage.Empty();

  if(m_availableLanguages.IsEmpty())
  {
    BoxeeUtils::GetAvailableLanguages(m_availableLanguages);
  }

  for (int i = 0; i < m_availableLanguages.Size(); i++)
  {
    CStdString strLanguage = m_availableLanguages[i]->GetProperty("lang_dirName");
    if (strcmpi(strLanguage.c_str(), g_guiSettings.GetString("locale.language").c_str()) == 0)
    {
      pControl->SetLabel2(m_availableLanguages[i]->GetProperty("lang_displayName"));
      break;
    }
  }
}

void CGUIWindowSettingsCategory::FillInKeyboards(CSetting *pSetting, int nKeyboard)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();
  
  int iCurrentLang = 0;
  int iLanguage = 1;

  if(m_availableLanguages.IsEmpty())
  {
    BoxeeUtils::GetAvailableLanguages(m_availableLanguages);
  }

  pControl->AddLabel(g_localizeStrings.Get(54070), 0);
  
  std::vector<XBMC::Keyboard> userKeyboards;
  g_application.GetKeyboards().GetUserKeyboards(userKeyboards);

  CStdString curr;
  if (userKeyboards.size() > (size_t)nKeyboard)
    curr = userKeyboards[nKeyboard].GetLangPath();

  for(int i = 0; i < m_availableLanguages.Size(); ++i)
  {
    CStdString strLanguage = m_availableLanguages[i]->GetProperty("lang_dirName");
    CStdString strLangDisplay = m_availableLanguages[i]->GetProperty("lang_displayName");

    if (strcmpi(strLanguage.c_str(), curr.c_str()) == 0)
      iCurrentLang = iLanguage;
  
    XBMC::Keyboard kb;
    if (strLanguage.find("English") == CStdString::npos && strLanguage.find("english") == CStdString::npos && kb.Load(strLanguage))
    {
      pControl->AddLabel(strLangDisplay, iLanguage++);
    }
  }
  
  pControl->SetValue(iCurrentLang);
}

void CGUIWindowSettingsCategory::FillInShowsDefault(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  pControl->AddLabel(g_localizeStrings.Get(54834), 0); // Favorites
  pControl->AddLabel(g_localizeStrings.Get(54835), 1); // Files
  pControl->AddLabel(g_localizeStrings.Get(54836), 2); // All

  CStdString strSelected;
  CGUIWindowStateDatabase sdb;
  sdb.GetDefaultCategory(WINDOW_BOXEE_BROWSE_TVSHOWS, strSelected);

  if (strSelected == "favorite")
    pControl->SetValue(0);
  else if (strSelected == "local")
    pControl->SetValue(1);
  else if (strSelected == "all")
    pControl->SetValue(2);
}

void CGUIWindowSettingsCategory::FillInMoviesDefault(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  pControl->AddLabel(g_localizeStrings.Get(54835), 0); // Files
  pControl->AddLabel(g_localizeStrings.Get(54836), 1); // All

  CStdString strSelected;
  CGUIWindowStateDatabase sdb;
  sdb.GetDefaultCategory(WINDOW_BOXEE_BROWSE_MOVIES, strSelected);

  if (strSelected == "local")
    pControl->SetValue(0);
  else if (strSelected == "all")
    pControl->SetValue(1);
}

void CGUIWindowSettingsCategory::FillInAppsDefault(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  pControl->AddLabel(g_localizeStrings.Get(54834), 0); // Favorites
  pControl->AddLabel(g_localizeStrings.Get(54836), 1); // All

  CStdString strSelected;
  CGUIWindowStateDatabase sdb;
  sdb.GetDefaultCategory(WINDOW_BOXEE_BROWSE_APPS, strSelected);

  if (strSelected == "favorite")
    pControl->SetValue(0);
  else if (strSelected == "all")
    pControl->SetValue(1);
}

void CGUIWindowSettingsCategory::SetShowStarter(CSetting* pSetting)
{
  CGUICheckMarkControl *pControl = (CGUICheckMarkControl *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  //pControl->Clear();

  CStdString strValue;
  CGUIWindowStateDatabase sdb;
  sdb.GetUserSetting("showstarter", strValue);

  if (strValue=="true")
    pControl->SetSelected(true);
  else
    pControl->SetSelected(false);
}

void CGUIWindowSettingsCategory::FillInScreenSavers(CSetting *pSetting)
{ // Screensaver mode
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  pControl->AddLabel(g_localizeStrings.Get(351), 0); // Off
  pControl->AddLabel(g_localizeStrings.Get(352), 1); // Dim
  pControl->AddLabel(g_localizeStrings.Get(353), 2); // Black
  pControl->AddLabel(g_localizeStrings.Get(108), 3); // PictureSlideShow
  //pControl->AddLabel(g_localizeStrings.Get(20425), 4); // Fanart Slideshow

  //find screensavers ....
  CFileItemList items;
  CDirectory::GetDirectory( "special://xbmc/screensavers/", items);
  if (!CSpecialProtocol::XBMCIsHome())
    CDirectory::GetDirectory("special://home/screensavers/", items);

  int iCurrentScr = -1;
  vector<CStdString> vecScr;
  int i = 0;
  for (i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->m_bIsFolder)
    {
      CStdString strExtension;
      CUtil::GetExtension(pItem->m_strPath, strExtension);
      if (strExtension == ".xbs")
      {
#ifdef _LINUX
        void *handle = dlopen(_P(pItem->m_strPath).c_str(), RTLD_LAZY);
        if (!handle)
        {
          CLog::Log(LOGERROR, "FillInScreensavers: Unable to load %s, reason: %s", pItem->m_strPath.c_str(), dlerror());
          continue;
        }
        dlclose(handle);
#endif
        CStdString strLabel = pItem->GetLabel();
        vecScr.push_back(strLabel.Mid(0, strLabel.size() - 4));
      }
    }
  }

  CStdString strDefaultScr = pSettingString->GetData();
  CStdString strExtension;
  CUtil::GetExtension(strDefaultScr, strExtension);
  if (strExtension == ".xbs")
    strDefaultScr.Delete(strDefaultScr.size() - 4, 4);

  sort(vecScr.begin(), vecScr.end(), sortstringbyname());
  for (i = 0; i < (int) vecScr.size(); ++i)
  {
    CStdString strScr = vecScr[i];

    if (strcmpi(strScr.c_str(), strDefaultScr.c_str()) == 0)
      iCurrentScr = i + PREDEFINED_SCREENSAVERS;

    pControl->AddLabel(strScr, i + PREDEFINED_SCREENSAVERS);
  }

  // if we can't find the screensaver previously configured
  // then fallback to turning the screensaver off.
  if (iCurrentScr < 0)
  {
    if (strDefaultScr == "Dim")
      iCurrentScr = 1;
    else if (strDefaultScr == "Black")
      iCurrentScr = 2;
    else if (strDefaultScr == "SlideShow") // PictureSlideShow
      iCurrentScr = 3;
    else if (strDefaultScr == "Fanart Slideshow") // Fanart slideshow
      iCurrentScr = 4;
    else
    {
      iCurrentScr = 0;
      pSettingString->SetData("None");
    }
  }
  pControl->SetValue(iCurrentScr);
}

void CGUIWindowSettingsCategory::FillInFTPServerUser(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  pControl->SetShowRange(true);

#ifdef HAS_FTP_SERVER
  int iDefaultFtpUser = 0;

  CStdString strFtpUser1; int iUserMax;
  // Get FTP XBOX Users and list them !
  if (CUtil::GetFTPServerUserName(0, strFtpUser1, iUserMax))
  {
    for (int i = 0; i < iUserMax; i++)
    {
      if (CUtil::GetFTPServerUserName(i, strFtpUser1, iUserMax))
        pControl->AddLabel(strFtpUser1.c_str(), i);
      if (strFtpUser1.ToLower() == "xbox") iDefaultFtpUser = i;
    }
    pControl->SetValue(iDefaultFtpUser);
    CUtil::GetFTPServerUserName(iDefaultFtpUser, strFtpUser1, iUserMax);
    g_guiSettings.SetString("servers.ftpserveruser", strFtpUser1.c_str());
    pControl->Update();
  }
  else { //Set "None" if there is no FTP User found!
    pControl->AddLabel(g_localizeStrings.Get(231).c_str(), 0);
    pControl->SetValue(0);
    pControl->Update();
  }
#endif
}
bool CGUIWindowSettingsCategory::SetFTPServerUserPass()
{
#ifdef HAS_FTP_SERVER
  // TODO: Read the FileZilla Server XML and Set it here!
  // Get GUI USER and pass and set pass to FTP Server
  CStdString strFtpUserName, strFtpUserPassword;
  strFtpUserName      = g_guiSettings.GetString("servers.ftpserveruser");
  strFtpUserPassword  = g_guiSettings.GetString("servers.ftpserverpassword");
  if(strFtpUserPassword.size()!=0)
  {
    if (CUtil::SetFTPServerUserPassword(strFtpUserName, strFtpUserPassword))
    {
      // todo! ERROR check! if something goes wrong on SetPW!
      // PopUp OK and Display: FTP Server Password was set succesfull!
      CGUIDialogOK::ShowAndGetInput(728, 0, 1247, 0);
    }
    return true;
  }
  else
  {
    // PopUp OK and Display: FTP Server Password is empty! Try Again!
    CGUIDialogOK::ShowAndGetInput(728, 0, 12358, 0);
  }
#endif
  return true;
}

void CGUIWindowSettingsCategory::FillInRegions(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();

  int iCurrentRegion=0;
  CStdStringArray regions;
  g_langInfo.GetRegionNames(regions);

  CStdString strCurrentRegion=g_langInfo.GetCurrentRegion();

  sort(regions.begin(), regions.end(), sortstringbyname());

  for (int i = 0; i < (int) regions.size(); ++i)
  {
    const CStdString& strRegion = regions[i];

    if (strRegion == strCurrentRegion)
      iCurrentRegion = i;

    pControl->AddLabel(strRegion, i);
  }

  pControl->SetValue(iCurrentRegion);
}

CBaseSettingControl *CGUIWindowSettingsCategory::GetSetting(const CStdString &strSetting)
{
  for (unsigned int i = 0; i < m_vecSettings.size(); i++)
  {
    if (m_vecSettings[i]->GetSetting()->GetSetting() == strSetting)
      return m_vecSettings[i];
  }
  CLog::Log(LOGERROR,"Failed GettingSetting %s!\n", strSetting.c_str());
  return NULL;
}

void CGUIWindowSettingsCategory::JumpToSection(int windowId, const CStdString &section)
{
  // grab our section
  CSettingsGroup *pSettingsGroup = g_guiSettings.GetGroup(windowId - WINDOW_SETTINGS_MYPICTURES);
  if (!pSettingsGroup) return;
  // get the categories we need
  vecSettingsCategory categories;
  pSettingsGroup->GetCategories(categories);
  // iterate through them and check for the required section
  int iSection = -1;
  for (unsigned int i = 0; i < categories.size(); i++)
    if (categories[i]->m_strCategory.Equals(section))
      iSection = i;
  if (iSection == -1) return;

  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0, 0, 0);
  OnMessage(msg);
  m_iSectionBeforeJump=m_iSection;
  m_iControlBeforeJump=m_lastControlID;
  m_iWindowBeforeJump=GetID();

  m_iSection=iSection;
  m_lastControlID=CONTROL_START_CONTROL;
  CGUIMessage msg1(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, windowId);
  OnMessage(msg1);
  for (unsigned int i=0; i<m_vecSections.size(); ++i)
    CONTROL_DISABLE(CONTROL_START_BUTTONS+i);
}

void CGUIWindowSettingsCategory::JumpToPreviousSection()
{
  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0, 0, 0);
  OnMessage(msg);
  m_iSection=m_iSectionBeforeJump;
  m_lastControlID=m_iControlBeforeJump;
  CGUIMessage msg1(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, m_iWindowBeforeJump);
  OnMessage(msg1);

  m_iSectionBeforeJump=-1;
  m_iControlBeforeJump=-1;
  m_iWindowBeforeJump=WINDOW_INVALID;
}

void CGUIWindowSettingsCategory::FillInSkinThemes(CSetting *pSetting)
{
  // There is a default theme (just Textures.xpr)
  // any other *.xpr files are additional themes on top of this one.
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  CStdString strSettingString = g_guiSettings.GetString("lookandfeel.skintheme");

  m_strNewSkinTheme.Empty();

  // Clear and add. the Default Label
  pControl->Clear();
  pControl->SetShowRange(true);
  pControl->AddLabel(g_localizeStrings.Get(15109), 0); // "SKINDEFAULT"! The standart Textures.xpr will be used!

  // find all *.xpr in this path
  CStdString strDefaultTheme = pSettingString->GetData();

  // Search for Themes in the Current skin!
  vector<CStdString> vecTheme;
  CUtil::GetSkinThemes(vecTheme);

  // Remove the .xpr extension from the Themes
  CStdString strExtension;
  CUtil::GetExtension(strSettingString, strExtension);
  if (strExtension == ".xpr") strSettingString.Delete(strSettingString.size() - 4, 4);
  // Sort the Themes for GUI and list them
  int iCurrentTheme = 0;
  for (int i = 0; i < (int) vecTheme.size(); ++i)
  {
    CStdString strTheme = vecTheme[i];
    // Is the Current Theme our Used Theme! If yes set the ID!
    if (strTheme.CompareNoCase(strSettingString) == 0 )
      iCurrentTheme = i + 1; // 1: #of Predefined Theme [Label]
    pControl->AddLabel(strTheme, i + 1);
  }
  // Set the Choosen Theme
  pControl->SetValue(iCurrentTheme);
}

void CGUIWindowSettingsCategory::FillInSkinColors(CSetting *pSetting)
{
  // There is a default theme (just defaults.xml)
  // any other *.xml files are additional color themes on top of this one.
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  CStdString strSettingString = g_guiSettings.GetString("lookandfeel.skincolors");

  m_strNewSkinColors.Empty();

  // Clear and add. the Default Label
  pControl->Clear();
  pControl->SetShowRange(true);
  pControl->AddLabel(g_localizeStrings.Get(15109), 0); // "SKINDEFAULT"! The standard defaults.xml will be used!

  // Search for colors in the Current skin!
  vector<CStdString> vecColors;

  CStdString strPath;
  CUtil::AddFileToFolder(g_SkinInfo.GetBaseDir(),"colors",strPath);

  CFileItemList items;
  CDirectory::GetDirectory(PTH_IC(strPath), items, ".xml");
  // Search for Themes in the Current skin!
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->m_bIsFolder && pItem->GetLabel().CompareNoCase("defaults.xml") != 0)
    { // not the default one
      CStdString strLabel = pItem->GetLabel();
      vecColors.push_back(strLabel.Mid(0, strLabel.size() - 4));
    }
  }
  sort(vecColors.begin(), vecColors.end(), sortstringbyname());

  // Remove the .xml extension from the Themes
  if (CUtil::GetExtension(strSettingString) == ".xml")
    CUtil::RemoveExtension(strSettingString);

  int iCurrentColor = 0;
  for (int i = 0; i < (int) vecColors.size(); ++i)
  {
    CStdString strColor = vecColors[i];
    // Is the Current Theme our Used Theme! If yes set the ID!
    if (strColor.CompareNoCase(strSettingString) == 0 )
      iCurrentColor = i + 1; // 1: #of Predefined Theme [Label]
    pControl->AddLabel(strColor, i + 1);
  }
  // Set the Choosen Theme
  pControl->SetValue(iCurrentColor);
}

void CGUIWindowSettingsCategory::FillInStartupWindow(CSetting *pSetting)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  const vector<CSkinInfo::CStartupWindow> &startupWindows = g_SkinInfo.GetStartupWindows();

  // TODO: How should we localize this?
  // In the long run there is no way to do it really without the skin having some
  // translation information built in to it, which isn't really feasible.

  // Alternatively we could lookup the strings in the english strings file to get
  // their id and then get the string from that

  // easier would be to have the skinner use the "name" as the label number.

  // eg <window id="0">513</window>

  bool currentSettingFound(false);
  for (vector<CSkinInfo::CStartupWindow>::const_iterator it = startupWindows.begin(); it != startupWindows.end(); it++)
  {
    CStdString windowName((*it).m_name);
    if (StringUtils::IsNaturalNumber(windowName))
      windowName = g_localizeStrings.Get(atoi(windowName.c_str()));
    int windowID((*it).m_id);
    pControl->AddLabel(windowName, windowID);
    if (pSettingInt->GetData() == windowID)
      currentSettingFound = true;
  }

  // ok, now check whether our current option is one of these
  // and set it's value
  if (!currentSettingFound)
  { // nope - set it to the "default" option - the first one
    pSettingInt->SetData(startupWindows[0].m_id);
  }
  pControl->SetValue(pSettingInt->GetData());
}

void CGUIWindowSettingsCategory::OnInitWindow()
{
  if (g_application.IsStandAlone())
  {
#ifndef __APPLE__
    m_iNetworkAssignment = g_guiSettings.GetInt("network.assignment");
    m_strNetworkIPAddress = g_guiSettings.GetString("network.ipaddress");
    m_strNetworkSubnet = g_guiSettings.GetString("network.subnet");
    m_strNetworkGateway = g_guiSettings.GetString("network.gateway");
    m_strNetworkDNS = g_guiSettings.GetString("network.dns");
#endif
  }
  m_strOldTrackFormat = g_guiSettings.GetString("musicfiles.trackformat");
  m_strOldTrackFormatRight = g_guiSettings.GetString("musicfiles.trackformatright");
  m_NewResolution = RES_INVALID;
  SetupControls();
  CGUIWindow::OnInitWindow();
}

void CGUIWindowSettingsCategory::FillInViewModes(CSetting *pSetting, int windowID)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->AddLabel("Auto", DEFAULT_VIEW_AUTO);
  bool found(false);
  int foundType = 0;
  CGUIWindow *window = g_windowManager.GetWindow(windowID);
  if (window)
  {
    window->Initialize();
    for (int i = 50; i < 60; i++)
    {
      CGUIBaseContainer *control = (CGUIBaseContainer *)window->GetControl(i);
      if (control)
      {
        int type = (control->GetType() << 16) | i;
        pControl->AddLabel(control->GetLabel(), type);
        if (type == pSettingInt->GetData())
          found = true;
        else if ((type >> 16) == (pSettingInt->GetData() >> 16))
          foundType = type;
      }
    }
    window->ClearAll();
  }
  if (!found)
    pSettingInt->SetData(foundType ? foundType : (DEFAULT_VIEW_AUTO));
  pControl->SetValue(pSettingInt->GetData());
}

void CGUIWindowSettingsCategory::FillInSortMethods(CSetting *pSetting, int windowID)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  CFileItemList items("C:");
  CGUIViewState *state = CGUIViewState::GetViewState(windowID, items);
  if (state)
  {
    bool found(false);
    vector< pair<int,int> > sortMethods;
    state->GetSortMethods(sortMethods);
    for (unsigned int i = 0; i < sortMethods.size(); i++)
    {
      pControl->AddLabel(g_localizeStrings.Get(sortMethods[i].second), sortMethods[i].first);
      if (sortMethods[i].first == pSettingInt->GetData())
        found = true;
    }
    if (!found && sortMethods.size())
      pSettingInt->SetData(sortMethods[0].first);
  }
  pControl->SetValue(pSettingInt->GetData());
  delete state;
}

void CGUIWindowSettingsCategory::FillInScrapers(CGUISpinControlEx *pControl, const CStdString& strSelected, const CStdString& strContent)
{
  CFileItemList items;
  if (strContent.Equals("music"))
    CDirectory::GetDirectory("special://xbmc/system/scrapers/music",items,".xml",false);
  else
    CDirectory::GetDirectory("special://xbmc/system/scrapers/video",items,".xml",false);
  int j=0;
  int k=0;
  pControl->Clear();
  for ( int i=0;i<items.Size();++i)
  {
    if (items[i]->m_bIsFolder)
      continue;

    CScraperParser parser;
    if (parser.Load(items[i]->m_strPath))
    {
      if (parser.GetContent() != strContent && !strContent.Equals("music"))
        continue;

      if (parser.GetName().Equals(strSelected)|| CUtil::GetFileName(items[i]->m_strPath).Equals(strSelected))
      {
        if (strContent.Equals("music")) // native strContent would be albums or artists but we're using the same scraper for both
          g_guiSettings.SetString("musiclibrary.defaultscraper", CUtil::GetFileName(items[i]->m_strPath));
        else if (strContent.Equals("movies"))
          g_guiSettings.SetString("scrapers.moviedefault", CUtil::GetFileName(items[i]->m_strPath));
        else if (strContent.Equals("tvshows"))
          g_guiSettings.SetString("scrapers.tvshowdefault", CUtil::GetFileName(items[i]->m_strPath));
        else if (strContent.Equals("musicvideos"))
          g_guiSettings.SetString("scrapers.musicvideodefault", CUtil::GetFileName(items[i]->m_strPath));
        k = j;
      }
      pControl->AddLabel(parser.GetName(),j++);
    }
  }
  pControl->SetValue(k);
}

void CGUIWindowSettingsCategory::FillInNetworkInterfaces(CSetting *pSetting)
{
#ifdef HAS_BOXEE_HAL
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();
  pControl->AddLabel(g_localizeStrings.Get(54669), WIRED_INTERFACE_ID);
  pControl->AddLabel(g_localizeStrings.Get(54679), WIRELESS_INTERFACE_ID);

  CHalWirelessConfig wirelessConfig;
  CHalServicesFactory::GetInstance().GetWirelessConfig(0, wirelessConfig);
  if (wirelessConfig.addr_type != ADDR_NONE)
  {
    pControl->SetValue(WIRELESS_INTERFACE_ID);
  }
#endif
}

void CGUIWindowSettingsCategory::FillInAudioDevices(CSetting* pSetting, bool Passthrough)
{
#ifdef __APPLE__  
  if (Passthrough)
    return;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  CoreAudioDeviceList deviceList;
  CCoreAudioHardware::GetOutputDevices(&deviceList);

  if (CCoreAudioHardware::GetDefaultOutputDevice())
    pControl->AddLabel("Default Output Device", 0); // This will cause FindAudioDevice to fall back to the system default as configured in 'System Preferences'
  int activeDevice = 0;
  
  CStdString deviceName;
  for (int i = pControl->GetMaximum(); !deviceList.empty(); i++)
  {
    CCoreAudioDevice device(deviceList.front());
    pControl->AddLabel(device.GetName(deviceName, device.GetId()), i);
    
    if (g_guiSettings.GetString("audiooutput.audiodevice").Equals(deviceName))
      activeDevice = i; // Tag this one
    
    deviceList.pop_front();
  }
  pControl->SetValue(activeDevice);
#elif defined(HAS_ALSA)
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  std::vector<CStdString> cardList;
  CALSADirectSound::GetSoundCards(cardList);
  if (cardList.size()==0)
  {
    pControl->AddLabel("Error - no devices found", 0);
  }
  else
  {
    cardList.insert(cardList.begin(), "default");
    cardList.insert(cardList.begin(), "custom");
    std::vector<CStdString>::const_iterator iter = cardList.begin();
    for (int i=0; iter != cardList.end(); iter++)
    {
      CStdString cardName = *iter;
      pControl->AddLabel(cardName, i);

      if (Passthrough)
      {
        if (g_guiSettings.GetString("audiooutput.passthroughdevice").Equals(cardName))
          pControl->SetValue(i);
      }
      else
      {
        if (g_guiSettings.GetString("audiooutput.audiodevice").Equals(cardName))
          pControl->SetValue(i);
    }

      i++;
  }
  }
#elif defined(_WIN32)
  if (Passthrough)
    return;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();
  CWDSound p_dsound;
  std::vector<DSDeviceInfo > deviceList = p_dsound.GetSoundDevices();
  std::vector<DSDeviceInfo >::const_iterator iter = deviceList.begin();
  for (int i=0; iter != deviceList.end(); i++)
  {
    DSDeviceInfo dev = *iter;
    pControl->AddLabel(dev.strDescription, i);
  
    if (g_guiSettings.GetString("audiooutput.audiodevice").Equals(dev.strDescription))
        pControl->SetValue(i);

    ++iter;
  }
#endif
}

#if defined(HAS_ALSA)
void CGUIWindowSettingsCategory::GenSoundLabel(const CStdString& device, const CStdString& card, const int labelValue, CGUISpinControlEx* pControl, bool Passthrough)
{
  CStdString deviceString(device);

  printf("device %s\n", deviceString.c_str());
  if (!device.Equals("custom"))
  {
    deviceString.AppendFormat(":CARD=%s", card.c_str());
    printf("device after %s\n", deviceString.c_str());
  }

  if (CALSADirectSound::SoundDeviceExists(deviceString.c_str()) || 
       !device.Equals("default") ||
       !device.Equals("custom"))
  {
    pControl->AddLabel(deviceString.c_str(), labelValue);
    if (Passthrough)
    {
      if (g_guiSettings.GetString("audiooutput.passthroughdevice").Equals(deviceString))
        pControl->SetValue(labelValue);
    }
    else
    {
      if (g_guiSettings.GetString("audiooutput.audiodevice").Equals(deviceString))
        pControl->SetValue(labelValue);
    }
  }
}
#endif //defined(_LINUX) && !defined(__APPLE__)

void CGUIWindowSettingsCategory::FillInWeatherPlugins(CGUISpinControlEx *pControl, const CStdString& strSelected)
{
  int j=0;
  int k=0;
  pControl->Clear();
  // add our disable option
  pControl->AddLabel(g_localizeStrings.Get(13611), j++);

  CFileItemList items;
  if (CDirectory::GetDirectory("special://home/plugins/weather/", items, "/", false))
  {
    for (int i=0; i<items.Size(); ++i)
    {    
      // create the full path to the plugin
      CStdString plugin;
      CStdString pluginPath = items[i]->m_strPath;
      // remove slash at end so we can use the plugins folder as plugin name
      CUtil::RemoveSlashAtEnd(pluginPath);
      // add default.py to our plugin path to create the full path
      CUtil::AddFileToFolder(pluginPath, "default.py", plugin);
      if (XFILE::CFile::Exists(plugin))
      {
        // is this the users choice
        if (CUtil::GetFileName(pluginPath).Equals(strSelected))
          k = j;
        // we want to use the plugins folder as name
        pControl->AddLabel(CUtil::GetFileName(pluginPath), j++);
      }
    }
  }
  pControl->SetValue(k);
}

void CGUIWindowSettingsCategory::FillInTimeFormat(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  int iCurrentFormat = 0;

  CStdString strCurrentTemp =g_langInfo.TimeFormatToPattern(g_langInfo.GetTimeFormat());
  CStdStringArray formats;
  g_langInfo.GetTimeFormats(formats);

  for (int i = 0; i < (int) formats.size(); ++i)
  {
    const CStdString& strFormat = formats[i];
    pControl->AddLabel(formats[i], i);

    if (strFormat == strCurrentTemp)
    {
      iCurrentFormat = i;
    }
  }

  pControl->SetValue(iCurrentFormat);
}
void CGUIWindowSettingsCategory::FillInTempScale(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  int iCurrentUnit = 0;
  CStdStringArray units;
  units.push_back("F");
  units.push_back("C");

// g_langInfo.GetTempUnits(units);

  CStdString strCurrentUnit =g_langInfo.TempUnitToStr(g_langInfo.GetTempUnit());

  for (int i = 0; i < (int) units.size(); ++i)
  {
	const CStdString& strUnit = units[i];

    pControl->AddLabel(strUnit, i);
    if (strUnit == strCurrentUnit)
      iCurrentUnit = i;

  }
  pControl->SetValue(iCurrentUnit);
}

void CGUIWindowSettingsCategory::NetworkInterfaceChanged(void)
{
#ifdef HAS_BOXEE_HAL
  CHalAddrType iAssignment;
  CHalWirelessAuthType iAuthType = AUTH_DONTCARE;
  CStdString sIPAddress;
  CStdString sNetworkMask;
  CStdString sDefaultGateway;
  CStdString sWirelessNetwork;
  CStdString sWirelessKey;
  CStdString sDNS;
  bool bIsWireless;

  // Get network information
  if (!GetSetting("network.interface"))
  {
    return;
  }

  CGUISpinControlEx *ifaceControl = (CGUISpinControlEx *)GetControl(GetSetting("network.interface")->GetID());
  if (!ifaceControl)
  {
    return;
  }

  int val = ifaceControl->GetValue();

  if(m_iNetworkInterface == -1)
    m_iNetworkInterface = val;

  if (val == WIRED_INTERFACE_ID)
  {
    bIsWireless = false;
    CHalEthernetConfig config;
    CHalServicesFactory::GetInstance().GetEthernetConfig(0, config);
    sIPAddress = config.ip_address;
    sNetworkMask = config.netmask;
    sDefaultGateway = config.gateway;
    sDNS = config.dns;
    iAssignment = config.addr_type;
  }
  else
  {
    bIsWireless = true;
    CHalWirelessConfig config;
    CHalServicesFactory::GetInstance().GetWirelessConfig(0, config);
    sIPAddress = config.ip_address;
    sNetworkMask = config.netmask;
    sDefaultGateway = config.gateway;
    sDNS = config.dns;
    iAssignment = config.addr_type;
    sWirelessNetwork = config.ssid;
    m_networkKey = sWirelessKey = config.password;
    iAuthType = config.authType;
  }

  // Update controls with information
  CGUISpinControlEx* pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.assignment")->GetID());
  if (pControl1) pControl1->SetValue(iAssignment);

  GetSetting("network.dns")->GetSetting()->FromString(sDNS);

  if (iAssignment == ADDR_STATIC)
  {
    GetSetting("network.ipaddress")->GetSetting()->FromString(sIPAddress);
    GetSetting("network.subnet")->GetSetting()->FromString(sNetworkMask);
    GetSetting("network.gateway")->GetSetting()->FromString(sDefaultGateway);
  }
  else if (iAssignment == ADDR_NONE)
  {
    GetSetting("network.ipaddress")->GetSetting()->FromString("");
    GetSetting("network.subnet")->GetSetting()->FromString("");
    GetSetting("network.gateway")->GetSetting()->FromString("");
  }
  else
  {
    if (val == WIRED_INTERFACE_ID)
    {
      CHalEthernetInfo info;
      CHalServicesFactory::GetInstance().GetEthernetInfo(0, info);
      GetSetting("network.ipaddress")->GetSetting()->FromString(info.ip_address);
      GetSetting("network.subnet")->GetSetting()->FromString(info.netmask);
      GetSetting("network.gateway")->GetSetting()->FromString(info.gateway);
      GetSetting("network.dns")->GetSetting()->FromString(info.dns);
    }
    else
    {
      bIsWireless = true;
      CHalWirelessInfo info;
      CHalServicesFactory::GetInstance().GetWirelessInfo(0, info);
      ((CGUIButtonControl *)GetControl(GetSetting("network.ipaddress")->GetID()))->SetLabel2(info.ip_address);
      ((CGUIButtonControl *)GetControl(GetSetting("network.subnet")->GetID()))->SetLabel2("mooki");
      ((CGUIButtonControl *)GetControl(GetSetting("network.gateway")->GetID()))->SetLabel2(info.gateway);
      ((CGUIButtonControl *)GetControl(GetSetting("network.dns")->GetID()))->SetLabel2(info.dns);
      ((CGUIButtonControl *)GetControl(GetSetting("network.essid")->GetID()))->SetLabel2(info.ssid);

      GetSetting("network.ipaddress")->GetSetting()->FromString(info.ip_address);
      GetSetting("network.subnet")->GetSetting()->FromString(info.netmask);
      GetSetting("network.gateway")->GetSetting()->FromString(info.gateway);
      GetSetting("network.dns")->GetSetting()->FromString(info.dns);
      GetSetting("network.essid")->GetSetting()->FromString(info.ssid);

    }
  }

  if (bIsWireless)
  {
    GetSetting("network.essid")->GetSetting()->FromString(sWirelessNetwork);
    m_networkEnc = iAuthType;
  }
  else
  {
    GetSetting("network.essid")->GetSetting()->FromString("");
    m_networkEnc = AUTH_DONTCARE;
  }
#endif
}

void CGUIWindowSettingsCategory::ApplyPersonalFeeds()
{
  CStdString xmlStr = "<settings>";
  CStdString id;
  CStdString type;
  CStdString value;

  for (int i = 0; i < (int)m_vecSections[m_iSection]->m_vecSettings.size() ; i ++ )
  {
	CSetting *pSetting = m_vecSections[m_iSection]->m_vecSettings[i];
    if (pSetting->GetControlType() == BUTTON_CONTROL_STANDARD)
    {
      type = "text";
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      if (!pControl)
      {
       	CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::ApplyPersonalFeeds control doesnt exist %s ", pSetting->GetLabelStr().c_str());
       	continue;
      }
      value = pControl->GetLabel2();

    }
    else if (pSetting->GetControlType() ==  CHECKMARK_CONTROL)
    {
      type = "bool";
      CGUIRadioButtonControl *pControl = (CGUIRadioButtonControl *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      if (!pControl)
      {
       	CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::ApplyPersonalFeeds control doesnt exist %s ", pSetting->GetLabelStr().c_str());
       	continue;
      }
      value = pControl->IsSelected() ? "true" : "false";

    } else
    {
    	CLog::Log(LOGDEBUG,"dont generate setting %s to the server ", pSetting->GetLabelStr().c_str());
    	continue;
    }
    id = pSetting->GetCustomData();
    xmlStr += "<setting id=\"";
    xmlStr += id;
    xmlStr += "\" type=\"";
    xmlStr += type;
    xmlStr += "\" value=\"";
    xmlStr += value ;
    xmlStr += "\"/>";
  }
  xmlStr += "</settings>";

  CLog::Log(LOGDEBUG,"xmlString %s",xmlStr.c_str());

  std::string strUrl = BOXEE::BXConfiguration::GetInstance().GetURLParam("Boxee.ApiFeedSettingsUrl","http://app.boxee.tv/api/feedsettings");
  BOXEE::ListHttpHeaders headers;
  headers.push_back("Content-Type: text/xml");

  BOXEE::BXXMLDocument    postUrl;
  postUrl.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  postUrl.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());
  strUrl = postUrl.LoadFromURL(strUrl,headers, xmlStr.c_str());

}

void CGUIWindowSettingsCategory::SetNewLanguage(CStdString newLanguage)
{
  // no lock because is is currently called only from this window and FTU
  m_strNewLanguage = newLanguage;
}

CStdString CGUIWindowSettingsCategory::GetNewLanguage()
{
  return m_strNewLanguage;
}

bool CGUIWindowSettingsCategory::TestAndSetNewResolution(RESOLUTION newResolution)
{
  bool newResWasSet = true;

  if (newResolution != RES_INVALID)
  {
    m_NewResolution = newResolution;
  }

  RESOLUTION lastRes = g_graphicsContext.GetVideoResolution();

  g_guiSettings.SetInt("videoscreen.resolution", m_NewResolution);
  g_guiSettings.SetInt("videoplayer.displayresolution", m_NewResolution);
  g_guiSettings.SetInt("pictures.displayresolution", m_NewResolution);
  g_graphicsContext.SetVideoResolution(m_NewResolution, TRUE);
  g_guiSettings.m_LookAndFeelResolution = m_NewResolution;
  bool cancelled = false;
  if (!CGUIDialogYesNo::ShowAndGetInput(13110, 13111, 20022, 20022, -1, -1, cancelled, 10000, 0))
  {
    g_guiSettings.SetInt("videoscreen.resolution", lastRes);
    g_guiSettings.SetInt("videoplayer.displayresolution", lastRes);
    g_guiSettings.SetInt("pictures.displayresolution", lastRes);
    g_graphicsContext.SetVideoResolution(lastRes, TRUE);
    g_guiSettings.m_LookAndFeelResolution = lastRes;

    newResWasSet = false;
  }

  int width = g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iWidth;
  int height = g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iHeight;
  float refresh = g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].fRefreshRate;
  bool interlace = (g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].dwFlags & D3DPRESENTFLAG_INTERLACED);

  g_guiSettings.SetInt("videoscreen.width", width);
  g_guiSettings.SetInt("videoscreen.height", height);
  g_guiSettings.SetFloat("videoscreen.refresh", refresh);
  g_guiSettings.SetBool("videoscreen.interlace", interlace);

  //printf("#@#@ Res was set: width %d height %d refresh %f interlace %d\n", width, height, refresh, interlace);

  CGUIMessage msg(GUI_MSG_SETFOCUS,g_windowManager.GetActiveWindow(),CONTROL_START_CONTROL,0);
  CGUIWindow* pWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
  pWindow->OnMessage(msg);

  return newResWasSet;
}

bool CGUIWindowSettingsCategory::LoadNewLanguage()
{
  g_guiSettings.SetString("locale.language", m_strNewLanguage);
  g_settings.Save();

  CStdString strLangInfoPath;
  strLangInfoPath.Format("special://xbmc/language/%s/langinfo.xml", m_strNewLanguage.c_str());

  g_langInfo.Load(strLangInfoPath);

  if (g_langInfo.ForceUnicodeFont() && !g_fontManager.IsFontSetUnicode())
  {
    CLog::Log(LOGINFO, "Language needs a ttf font, loading first ttf font available");
    CStdString strFontSet;
    if (g_fontManager.GetFirstFontSetUnicode(strFontSet))
    {
      m_strNewSkinFontSet=strFontSet;
    }
    else
    {
      CLog::Log(LOGERROR, "No ttf font found but needed: %s", strFontSet.c_str());
    }
  }

  g_charsetConverter.reset();

#ifdef _XBOX
  CStdString strKeyboardLayoutConfigurationPath;
  strKeyboardLayoutConfigurationPath.Format("special://xbmc/language/%s/keyboardmap.xml", m_strNewLanguage.c_str());
  CLog::Log(LOGINFO, "load keyboard layout configuration info file: %s", strKeyboardLayoutConfigurationPath.c_str());
  g_keyboardLayoutConfiguration.Load(strKeyboardLayoutConfigurationPath);
#endif

  CStdString strLanguagePath;
  strLanguagePath.Format("special://xbmc/language/%s/strings.xml", m_strNewLanguage.c_str());

  g_localizeStrings.Load(strLanguagePath);

  // also tell our weather to reload, as this must be localized
  CWeather::GetInstance().Refresh();

  BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().UpdateGenresListNow();

  CGUIMessage refreshMenuMsg(GUI_MSG_UPDATE, WINDOW_INVALID, 0);
  refreshMenuMsg.SetStringParam("initMenu");
  g_windowManager.SendThreadMessage(refreshMenuMsg,WINDOW_DIALOG_BOXEE_BROWSE_MENU);

  return true;
}

void CGUIWindowSettingsCategory::SetTempUnit(const CStdString& tempUnit)
{
  g_langInfo.SetTempUnit(tempUnit);
  g_guiSettings.SetString("locale.tempscale", tempUnit);
  CWeather::GetInstance().Refresh(); // need to reset our weather, as temperatures need re-translating.
}

void CGUIWindowSettingsCategory::SetTimeFormat(const CStdString& timeFormat)
{
  g_langInfo.SetTimeFormatFromPatterns(timeFormat);
  g_guiSettings.SetString("locale.timeformat", timeFormat);
  CWeather::GetInstance().Refresh(); // need to reset our weather, as temperatures need re-translating.
}

#ifdef HAS_DVB
void CGUIWindowSettingsCategory::UpdateOtaStatus()
{
  static int refreshCounter = 0;
  bool bOtaSectionVisible = (m_iOtaScanSection == m_iSection);

  ++refreshCounter;

  if (!bOtaSectionVisible || (refreshCounter % 60 != 1))
  {
    return;
  }

  CBaseSettingControl *otaStatusSetting = GetSetting("ota.reconfigure");
  if (otaStatusSetting == NULL)
  {
    CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::UpdateOtaStatus - FAILED to get setting [ota.rescan]");
    return;
  }

  CGUIButtonControl *pStatusControl = (CGUIButtonControl *)GetControl(otaStatusSetting->GetID());
  if (pStatusControl == NULL)
  {
    CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::UpdateOtaStatus - FAILED to get CGUIButtonControl for setting [ota.rescan]");
    return;
  }

  CStdString status;
  if (g_guiSettings.GetBool("ota.selectedcable"))
  {
    status += g_localizeStrings.Get(58030);
  }
  else
  {
    status += g_localizeStrings.Get(58019);
  }

  if (g_guiSettings.GetString("ota.zipcode") != "")
  {
    status += " \xc2\xb7 ";
    status += g_guiSettings.GetString("ota.zipcode");
  }
  else if (g_guiSettings.GetString("ota.countrycode") != "")
  {
    status += " \xc2\xb7 ";
    status += g_guiSettings.GetString("ota.countrycode").c_str();
  }

  status += " \xc2\xb7 ";

  if (DVBManager::GetInstance().IsScanning())
  {
    status += g_localizeStrings.Get(58054);
  }
  else if (DVBManager::GetInstance().GetChannels().Size() == 0 || !g_guiSettings.GetBool("ota.scanned"))
  {
    status += g_localizeStrings.Get(58053);
  }
  else
  {
    CStdString str;
    str.Format(g_localizeStrings.Get(58055).c_str(), DVBManager::GetInstance().GetChannels().Size());
    status += str;
  }

  pStatusControl->SetLabel2(status);
}
#endif

#ifdef HAS_EMBEDDED
void CGUIWindowSettingsCategory::UpdateDownloadStatus()
{
  static int downloadCounter = 0;
  CBoxeeVersionUpdateJob versionUpdateJob = g_boxeeVersionUpdateManager.GetBoxeeVerUpdateJob();
  //VERSION_UPDATE_JOB_STATUS versionUpdateJobStatus = versionUpdateJob.GetVersionUpdateJobStatus();
  bool bUpdateSectionVisible = (m_iUpdateSection == m_iSection);

  ++downloadCounter;

  if(!bUpdateSectionVisible || !m_bDownloadingUpdate || (downloadCounter % 60 != 0))
  {
    return;
  }

  CDownloadInfo downloadnfo;
  g_boxeeVersionUpdateManager.GetDownloadInfo(downloadnfo);

  double percent = downloadnfo.m_CurrentDownloadProgress;

  VERSION_UPDATE_DOWNLOAD_STATUS status = downloadnfo.m_Status;   
  switch(status)
  {
  case VUDS_IDLE:
  case VUDS_PRE_DOWNLOADING:
  {
    m_strUpdateStatus = g_localizeStrings.Get(54735);
  }
  break;
  case VUDS_DOWNLOADING:
  {
    if ((int)percent == 0)
    {
      m_strUpdateStatus = g_localizeStrings.Get(54735);
    }
    else
    {
      m_strUpdateStatus = g_localizeStrings.Get(53264);
    }
  }
  break;
  case VUDS_POST_DOWNLOADING:
  {
    m_strUpdateStatus = g_localizeStrings.Get(54654);
  }
  break;
  case VUDS_FINISHED:
  {
    downloadCounter = 0;
    m_bDownloadingUpdate = false;

    CLog::Log(LOGDEBUG,"CGUIWindowSettingsCategory::UpdateDownloadStatus - [downloadCounter=%d] - [status=%d=VUDS_FINISHED] -> Close (initbox)(update)",downloadCounter,(int)status);
  }
  break;
  case VUDS_FAILED:
  {
    downloadCounter = 0;
    m_bDownloadingUpdate = false;

    m_strUpdateStatus = g_localizeStrings.Get(54656);

    CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::UpdateDownloadStatus - [downloadCounter=%d] - [status=%d=VUDS_FAILED] -> Close (initbox)(update)",downloadCounter,(int)status);
  }
  break;
  default:
  {
    CLog::Log(LOGINFO,"CGUIWindowSettingsCategory::UpdateDownloadStatus - [downloadCounter=%d] - UNKNOWN [status=%d] -> not handling (initbox)(update)",downloadCounter,(int)status);
  }
  break;
  }

  CBaseSettingControl *updateStatusSetting = GetSetting("update.status");
  if (updateStatusSetting == NULL)
  {
    CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::UpdateDownloadStatus - FAILED to get setting [update.status] (initbox)(update)");
    return;
  }

  CGUIButtonControl *pStatusControl = (CGUIButtonControl *)GetControl(updateStatusSetting->GetID());

  if (pStatusControl == NULL)
  {
    CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::UpdateDownloadStatus - FAILED to get CGUIButtonControl for setting [update.status] (initbox)(update)");
    return;
  }

  if(status == VUDS_DOWNLOADING)
  {
    CStdString percentStr = "%d%%";
    CStdString percentLabel;
    percentLabel.Format(percentStr.c_str(),(int)percent);
    
    pStatusControl->SetLabel2(percentLabel);
  }
  else
  {
    UpdateSettings();
    pStatusControl->SetLabel2("");
  }
 
  pStatusControl->SetLabel(m_strUpdateStatus);
}

void CGUIWindowSettingsCategory::ReadTimezoneData()
{
  ReadTimezoneCountries();
  ReadTimezoneCities();
}

void CGUIWindowSettingsCategory::ReadTimezoneCountries()
{
  // read timezones countries from countries.txt
  CStdString fileName = "special://xbmc/system/countries.txt";
  fileName = _P(fileName);

  if (!XFILE::CFile::Exists(fileName))
  {
    CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::ReadTimezoneCountries - FAILED to open file [%s]. File doesn't exist. (tz)",fileName.c_str());
    return;
  }

  FILE* pFile = fopen(fileName, "r");
  if (!pFile)
  {
    CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::ReadTimezoneCountries - FAILED to open file [%s] (tz)",fileName.c_str());
    return;
  }

  m_tzCountryToCodeMap.clear();
  m_tzCodeToCountryMap.clear();
  int lineCounter = 0;

  CStdString strLine;
  char line[LINE_SIZE];
  memset(line, 0, LINE_SIZE);

  while(fgets(line, LINE_SIZE, pFile))
  {
    lineCounter++;
    strLine = line;
    StringUtils::RemoveCRLF(strLine);

    int firstCommaPos = strLine.Find(",");
    if (firstCommaPos == -1)
    {
      CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::ReadTimezoneCountries - [%d] - FAILED to find comma in line [%s]. [%s] (tz)",lineCounter,strLine.c_str(),fileName.c_str());
      continue;
    }

    CStdString countryCode = strLine.substr(0,firstCommaPos);
    CStdString countryName = strLine.substr(firstCommaPos+1);

    countryName.TrimLeft("\"");
    countryName.TrimRight("\"");

    //CLog::Log(LOGDEBUG,"CGUIWindowSettingsCategory::ReadTimezoneCountries - [%d] - Going to add [countryName=%s] to CountriesMap. [countryCode=%s] (tz)",lineCounter,countryName.c_str(),countryCode.c_str());

    m_tzCountryToCodeMap[countryName] = countryCode;
    m_tzCodeToCountryMap[countryCode] = countryName;

    memset(line, 0, LINE_SIZE);
  }
}

void CGUIWindowSettingsCategory::ReadTimezoneCities()
{
  // read timezones cities from timezones.txt
  CStdString fileName = "special://xbmc/system/timezones.txt";
  fileName = _P(fileName);

  if (!XFILE::CFile::Exists(fileName))
  {
    CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::ReadTimezoneCities - FAILED to open file [%s]. File doesn't exist. (tz)",fileName.c_str());
    return;
  }

  FILE* pFile = fopen(fileName, "r");
  if (!pFile)
  {
    CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::ReadTimezoneCities - FAILED to open file [%s] (tz)",fileName.c_str());
    return;
  }

  m_tzCodeToCityMultimap.clear();
  int lineCounter = 0;

  CStdString strLine;
  char line[LINE_SIZE];
  memset(line, 0, LINE_SIZE);

  while(fgets(line, LINE_SIZE, pFile))
  {
    lineCounter++;
    strLine = line;
    StringUtils::RemoveCRLF(strLine);

    vector<CStdString> tokens;
    CUtil::Tokenize(strLine,tokens,",");
    if ((int)tokens.size() < 3)
    {
      CLog::Log(LOGERROR,"CGUIWindowSettingsCategory::ReadTimezoneCities - [%d] - FAILED to find comma in line [%s]. [%s] (tz)",lineCounter,strLine.c_str(),fileName.c_str());
      continue;
    }

    CStdString countryCode = tokens[0];
    CStdString cityName = tokens[1];
    CStdString timezonePath = tokens[2];

//    CLog::Log(LOGDEBUG,"CGUIWindowSettingsCategory::ReadTimezoneCities - [%d] - Going to add [countryCode=%s][cityName=%s][timezonePath=%s] to CitiesMap (tz)",
//        lineCounter,countryCode.c_str(),cityName.c_str(),timezonePath.c_str());

    m_tzCodeToCityMultimap.insert(pair<CStdString,CStdString>(countryCode,cityName));
    m_tzCityToPathMap[cityName] = timezonePath;

    memset(line, 0, LINE_SIZE);
  }
}

CStdString CGUIWindowSettingsCategory::GetCountryByCode(const CStdString& countryCode)
{
  CStdString countryName = "";
  if (m_tzCodeToCountryMap.find(countryCode) != m_tzCodeToCountryMap.end())
  {
    countryName = m_tzCodeToCountryMap[countryCode];
  }

  return countryName;
}

bool CGUIWindowSettingsCategory::IsTimezoneCityExist(const CStdString& cityName)
{
  if (m_tzCityToPathMap.find(cityName) != m_tzCityToPathMap.end())
  {
    return true;
  }

  return false;
}
#endif

void CGUIWindowSettingsCategory::NetflixClear()
{
#ifdef HAS_EMBEDDED
  if (CGUIDialogYesNo2::ShowAndGetInput(51925, 54889))
  {
    system("/bin/rm -rf /.persistent/apps/nrd");
  }
#endif
}

void CGUIWindowSettingsCategory::SpotifyClear()
{
#ifdef HAS_EMBEDDED
  if (CGUIDialogYesNo2::ShowAndGetInput(51925, 54891))
  {
    system("/bin/rm -rf /.persistent/apps/spotify");
  }
#endif
}

void CGUIWindowSettingsCategory::VuduClear()
{
#ifdef HAS_EMBEDDED
  if (CGUIDialogYesNo2::ShowAndGetInput(51925, 54889))
  {
    creat("/.persistent/apps/vudu/deactivate", 0644);
  }
#endif
}

#ifdef HAS_BOXEE_HAL
CWaitNetworkUpBG::CWaitNetworkUpBG(int ifaceId) : m_ifaceId(ifaceId)
{

}

void CWaitNetworkUpBG::Run()
{
  time_t startTime = time(NULL);
  m_bJobResult = false;

  while (!m_bJobResult && startTime + 30 > time(NULL))
  {
    m_bJobResult = CWaitNetworkUpBG::IsNetworkUp(m_ifaceId);

    if (!m_bJobResult)
    {
      Sleep(1000);
    }
  }
}

bool CWaitNetworkUpBG::IsNetworkUp(int ifaceId)
{
  bool isNetworkUp = false;

  if (ifaceId == WIRED_INTERFACE_ID)
  {
    CHalEthernetInfo info;
    CHalServicesFactory::GetInstance().GetEthernetInfo(0, info);
    isNetworkUp = (info.running && info.link_up && info.ip_address != "0.0.0.0");
  }
  else
  {
    CHalWirelessInfo info;
    CHalServicesFactory::GetInstance().GetWirelessInfo(0, info);
    isNetworkUp = (info.connected && info.ip_address != "0.0.0.0");
  }

  return isNetworkUp;
}

CVpnOperationBG::CVpnOperationBG()
{

}

void CVpnOperationBG::Run()
{
  bool doHangup = g_halListener.GetVpnConnected();

  if (doHangup)
  {
    m_bJobResult = CHalServicesFactory::GetInstance().VpnHangup();

    if (m_bJobResult)
    {
      g_halListener.SetVpnConnected(false);
    }
  }
  else
  {
    CHALVpnConfig vpnConfig;
    vpnConfig.tunnelType = CHalVpnPPTP;
    vpnConfig.host = g_guiSettings.GetString("vpn.server");
    vpnConfig.password = g_guiSettings.GetString("vpn.password");
    vpnConfig.username = g_guiSettings.GetString("vpn.account");
    if (g_guiSettings.GetBool("vpn.encryption"))
      vpnConfig.encType = CHalVpnMppeForce;
    else
      vpnConfig.encType = CHalVpnMppeAuto;

    CHalServicesFactory::GetInstance().VpnSetConfig(vpnConfig);
    m_bJobResult = CHalServicesFactory::GetInstance().VpnDial();

    if (m_bJobResult)
    {
      g_halListener.SetVpnConnected(true);
    }
  }

  bool isConnectedToInternet = g_application.IsConnectedToInternet(true);
  int numOfSecWaited = 0;

  while (!isConnectedToInternet && (numOfSecWaited < SECONDS_TO_WAIT_FOR_INTERNET_CONNECTION))
  {
    sleep(1);
    numOfSecWaited++;
    isConnectedToInternet = g_application.IsConnectedToInternet(true);
  }

  if (isConnectedToInternet)
  {
    BoxeeUtils::RefreshCountryCode();

    g_directoryCache.ClearSubPaths("boxee://");
    g_directoryCache.ClearSubPaths("apps://");

    CGUIMessage refreshMovieWinMsg(GUI_MSG_UPDATE, 0, 0);
    g_windowManager.SendThreadMessage(refreshMovieWinMsg,WINDOW_BOXEE_BROWSE_MOVIES);

    CGUIMessage refreshTvShowWinMsg(GUI_MSG_UPDATE, 0, 0);
    g_windowManager.SendThreadMessage(refreshTvShowWinMsg,WINDOW_BOXEE_BROWSE_TVSHOWS);

    CGUIMessage refreshAppsWinMsg(GUI_MSG_UPDATE, 0, 0);
    g_windowManager.SendThreadMessage(refreshAppsWinMsg,WINDOW_BOXEE_BROWSE_APPS);
  }
  else
  {
    CLog::Log(LOGWARNING,"CGUIWindowSettingsCategory::CVpnOperationBG::Run - exit function after %s vpn with [isConnectedToInternet=%d=FALSE] (vpn)",doHangup ? "DISCONNECT" : "CONNECT",isConnectedToInternet);
  }
}
#endif
