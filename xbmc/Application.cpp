
/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://xbmc.org
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

#include "Application.h"
#include "utils/Builtins.h"
#include "Splash.h"
#include "KeyboardLayoutConfiguration.h"
#include "LangInfo.h"
#include "Util.h"
#include "Picture.h"
#include "TextureManager.h"
#include "cores/dvdplayer/DVDFileInfo.h"
#include "PlayListPlayer.h"
#include "Autorun.h"
#ifdef HAS_LCD
#include "utils/LCDFactory.h"
#endif
#include "GUIControlProfiler.h"
#include "LangCodeExpander.h"
#include "utils/GUIInfoManager.h"
#include "PlayListFactory.h"
#include "GUIFontManager.h"
#include "GUIColorManager.h"
#include "GUITextLayout.h"
#include "SkinInfo.h"
#ifdef HAS_PYTHON
#include "lib/libPython/XBPython.h"
#endif
#include "ButtonTranslator.h"
#include "GUIAudioManager.h"
#ifdef HAS_LASTFM
#include "lib/libscrobbler/lastfmscrobbler.h"
#include "lib/libscrobbler/librefmscrobbler.h"
#endif
#include "GUIPassword.h"
#include "ApplicationMessenger.h"
#include "SectionLoader.h"
#include "cores/DllLoader/DllLoaderContainer.h"
#include "GUIUserMessages.h"
#include "FileSystem/DirectoryCache.h"
#include "FileSystem/StackDirectory.h"
#include "FileSystem/SpecialProtocol.h"
#include "FileSystem/DllLibCurl.h"
#include "FileSystem/CMythSession.h"
#include "FileSystem/PluginDirectory.h"
#include "FileSystem/BoxeeServerDirectory.h"
#ifdef HAS_FILESYSTEM_SAP
#include "FileSystem/SAPDirectory.h"
#endif
#ifdef HAS_FILESYSTEM_HTSP
#include "FileSystem/HTSPDirectory.h"
#endif
#include "utils/TuxBoxUtil.h"
#include "utils/SystemInfo.h"
#include "utils/TimeUtils.h"
#include "ApplicationRenderer.h"
#include "GUILargeTextureManager.h"
#include "LastFmManager.h"
#include "SmartPlaylist.h"
#include "FileSystem/RarManager.h"
#include "PlayList.h"
#include "WindowingFactory.h"
#include "PowerManager.h"
#include "DPMSSupport.h"
#include "Settings.h"
#include "AdvancedSettings.h"
#include "LocalizeStrings.h"
#include "CPUInfo.h"

#include "RssSourceManager.h"
#include "ItemLoader.h"

#include "KeyboardStat.h"
#include "MouseStat.h"

#if defined(FILESYSTEM) && !defined(_LINUX)
#include "FileSystem/FileDAAP.h"
#endif
#ifdef HAS_UPNP
#include "UPnP.h"
#include "FileSystem/UPnPDirectory.h"
#endif
#if defined(_LINUX) && defined(HAS_FILESYSTEM_SMB)
#include "FileSystem/SMBDirectory.h"
#endif
#ifndef _BOXEE_
#include "PartyModeManager.h"
#endif
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#ifdef HAS_KARAOKE
#include "karaoke/karaokelyricsmanager.h"
#include "karaoke/GUIDialogKaraokeSongSelector.h"
#include "karaoke/GUIWindowKaraokeLyrics.h"
#endif
#include "AudioContext.h"
#include "GUIFontTTF.h"
#include "utils/Network.h"
#include "xbox/IoSupport.h"
#include "Zeroconf.h"
#include "ZeroconfBrowser.h"
#ifndef _LINUX
#include "utils/Win32Exception.h"
#else
#include <signal.h>
#endif
#ifdef HAS_WEB_SERVER
#include "lib/libGoAhead/XBMChttp.h"
#include "lib/libGoAhead/WebServer.h"
#endif
#ifdef HAS_FTP_SERVER
#include "lib/libfilezilla/xbfilezilla.h"
#endif
#ifdef HAS_TIME_SERVER
#include "utils/Sntp.h"
#endif
#ifdef HAS_EVENT_SERVER
#include "utils/EventServer.h"
#endif
#ifdef HAS_DBUS_SERVER
#include "utils/DbusServer.h"
#endif
#ifdef HAS_JSONRPC
#include "lib/libjsonrpc/JSONRPC.h"
#include "lib/libjsonrpc/TCPServer.h"
#endif
#ifdef HAS_AIRPLAY
#include "utils/AirPlayServer.h"
#endif
#ifdef HAS_AIRTUNES
#include "utils/AirTunesServer.h"
#endif

// Windows includes
#include "GUIWindowManager.h"
#include "GUIWindowHome.h"
#include "GUIStandardWindow.h"
#include "GUIWindowSettings.h"
#include "GUIWindowFileManager.h"
#include "GUIWindowSettingsCategory.h"
#ifndef _BOXEE_
#include "GUIWindowMusicPlaylist.h"
#include "GUIWindowMusicSongs.h"
#include "GUIWindowMusicNav.h"
#include "GUIWindowMusicPlaylistEditor.h"
#endif
#include "GUIWindowMusicInfo.h"
#ifndef _BOXEE_
#include "GUIWindowVideoInfo.h"
#include "GUIWindowVideoFiles.h"
#include "GUIWindowVideoNav.h"
#include "GUIWindowVideoPlaylist.h"
#endif
#include "GUIWindowSettingsProfile.h"
#ifdef HAS_GL
#include "GUIWindowTestPatternGL.h"
#endif
#ifdef HAS_GLES
#include "GUIWindowTestPatternGLES.h"
#endif
#ifdef HAS_DX
#include "GUIWindowTestPatternDX.h"
#endif
#include "GUIWindowSettingsScreenCalibration.h"
#ifndef _BOXEE_
#include "GUIWindowPrograms.h"
#include "GUIWindowPictures.h"
#include "GUIWindowScripts.h"
#endif
#include "GUIWindowLoginScreen.h"
#include "GUIWindowVisualisation.h"
#include "GUIWindowSystemInfo.h"
#include "GUIWindowScreensaver.h"
#include "GUIWindowSlideShow.h"
#include "GUIWindowStartup.h"
#include "GUIWindowFullScreen.h"
#include "GUIWindowOSD.h"
#include "GUIWindowMusicOverlay.h"
#include "GUIWindowVideoOverlay.h"
// Dialog includes
#include "GUIDialogMusicOSD.h"
#include "GUIDialogVisualisationSettings.h"
#include "GUIDialogVisualisationPresetList.h"
#include "GUIWindowScriptsInfo.h"
#include "GUIDialogNetworkSetup.h"
#include "GUIDialogMediaSource.h"
#include "GUIDialogVideoSettings.h"
#include "GUIDialogAudioSubtitleSettings.h"
#include "GUIDialogBoxeeBrowseSubtitleSettings.h"
#include "GUIDialogBoxeeBrowseLocalSubtitleSettings.h"
#include "GUIDialogSubtitleSettings.h"
#include "GUIDialogVideoBookmarks.h"
#include "GUIDialogProfileSettings.h"
#include "GUIDialogLockSettings.h"
#include "GUIDialogContentSettings.h"
#include "GUIDialogVideoScan.h"
#include "GUIWebDialog.h"
#include "GUIDialogBusy.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"
#include "GUIDialogProgress.h"
#include "GUIDialogSelect.h"
#include "GUIDialogBoxeeEject.h"
#include "GUIDialogBoxeeWatchLaterGetStarted.h"
#include "GUIDialogBoxeeMakeBoxeeSocial.h"
#include "GUIDialogBoxeeGetFacebookExtraCredential.h"
#include "GUIDialogFileStacking.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogGamepad.h"
#include "GUIDialogSubMenu.h"
#include "GUIDialogFavourites.h"
#include "GUIDialogButtonMenu.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogMusicScan.h"
#include "GUIDialogPlayerControls.h"
#include "GUIDialogSongInfo.h"
#include "GUIDialogSmartPlaylistEditor.h"
#include "GUIDialogSmartPlaylistRule.h"
#include "GUIDialogPictureInfo.h"
#include "GUIDialogPluginSettings.h"
#include "GUIDialogFirstTimeUseMenuCust.h"
#ifdef HAS_LINUX_NETWORK
#include "GUIDialogAccessPoints.h"
#include "GUIDialogWirelessAuthentication.h"
#endif
#include "GUIDialogFullScreenInfo.h"
#include "GUIDialogTeletext.h"
#include "GUIDialogSlider.h"
#include "cores/dlgcache.h"
#include "GUIDialogBoxeeVideoQuality.h"
#include "GUIDialogBoxeeSelectionList.h"
#include "GUIDialogBoxeeVideoResume.h"
#include "GUIWindowManager.h"

// boxee stuff
#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxvideodatabase.h"
#include "lib/libBoxee/bxaudiodatabase.h"
#include "lib/libBoxee/bxmediadatabase.h"
#include "lib/libBoxee/bxcurl.h"
#include "GUIDialogBoxeeShare.h"
#include "GUIDialogBoxeePostPlay.h"
#include "GUIDialogBoxeeRate.h"
#include "BoxeeUtils.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "lib/libBoxee/bxmediadatabase.h"
#include "lib/libBoxee/bxuserprofiledatabase.h"
#include "lib/libBoxee/bxutils.h"
#include "GUIDialogBoxeeMediaAction.h"
#include "GUIDialogBoxeeMessageScroll.h"
#include "GUIDialogBoxeeManualResolve.h"
#include "GUIDialogBoxeeManualResolveMovie.h"
#include "GUIDialogBoxeeManualResolveEpisode.h"
#include "GUIDialogBoxeeManualResolveAddFiles.h"
#include "GUIDialogBoxeeManualResolveDetails.h"
#include "GUIDialogBoxeeManualResolvePartAction.h"
#include "GUIDialogBoxeeManualResolveAudio.h"
#include "GUIDialogBoxeeManualResolveAlbum.h"
#ifdef HAS_WIDGETS
#include "GUIDialogWidgets.h"
#endif
#include "GUIWindowBoxeeMediaMain.h"
#include "GUIDialogBoxeeVideoCtx.h"
#include "GUIDialogBoxeeMusicCtx.h"
#include "GUIDialogBoxeePictureCtx.h"
#include "GUIDialogBoxeeAppCtx.h"
#include "GUIDialogBoxeeBrowserCtx.h"
#include "GUIDialogBoxeeUserLogin.h"
#include "GUIDialogBoxeeLoggingIn.h"
#include "GUIDialogBoxeeCredits.h"
#include "GUIDialogBoxeeUserPassword.h"
#ifdef XBMC_STANDALONE
#ifdef HAS_XRANDR
#include "GUIWindowBoxeeWizardResolution.h"
#endif
#include "GUIWindowBoxeeWizardAudio.h"
#include "GUIWindowBoxeeWizardNetwork.h"
#include "GUIWindowBoxeeWizardNetworkManual.h"
#include "GUIWindowBoxeeWizardTimezone.h"
#include "GUIWindowBoxeeWizardAddSource.h"
#include "GUIWindowBoxeeWizardSourceManager.h"
#include "GUIWindowBoxeeWizardSourceName.h"
#endif
#include "GUIWindowBoxeeWizardAddSourceManual.h"
#include "GUIDialogBoxeeBuffering.h"
#include "MetadataResolver.h"
#include "MetadataResolverVideo.h"
#include "MetadataResolverMusic.h"

#include "GUIWindowStateDatabase.h"
#include "GUIWindowBoxeeBrowse.h"
#include "GUIWindowBoxeeBrowseLocal.h"
#include "GUIWindowBoxeeBrowseTvShows.h"
#include "GUIWindowBoxeeBrowseMovies.h"
#include "GUIWindowBoxeeBrowseTvEpisodes.h"
#include "GUIWindowBoxeeBrowseAlbums.h"
#include "GUIWindowBoxeeBrowseTracks.h"
#include "GUIWindowBoxeeBrowseRepositories.h"
#include "GUIWindowBoxeeBrowseApps.h"
//#include "GUIWindowBoxeeBrowseAppBox.h"
#include "GUIWindowBoxeeBrowseSimpleApp.h"
#include "GUIWindowBoxeeBrowseProduct.h"
#include "GUIWindowBoxeeBrowseQueue.h"
#include "GUIWindowBoxeeBrowseDiscover.h"
#include "GUIWindowBoxeeBrowseHistory.h"
#include "GUIWindowBoxeeBrowsePhotos.h"
#include "GUIWindowBoxeeMain.h"
#include "GUIDialogBoxeeMainMenu.h"
#include "GUIDialogBoxeeBrowseMenu.h"
#include "GUIWindowBoxeeMediaInfo.h"
#include "BoxeeBrowseMenuManager.h"

#include "GUIWindowBoxeeLoginPrompt.h"
#include "GUIDialogBoxeeChapters.h"

#include "GUIWindowBoxeeMediaSources.h"
#include "GUIWindowBoxeeMediaSourceInfo.h"
#include "GUIWindowBoxeeMediaSourceList.h"
#include "GUIWindowBoxeeMediaSourceAddFolder.h"
#include "GUIWindowBoxeeMediaSourceAddShare.h"
#include "GUIWindowBoxeeApplicationSettings.h"
#include "GUIWindowBoxeeAlbumInfo.h"
#include "GUIDialogOK2.h"
#include "GUIDialogYesNo2.h"
#include "GUIDialogBoxeeUpdateMessage.h"
#include "GUIDialogBoxeeUpdateProgress.h"
#include "GUIDialogBoxeeApplicationAction.h"
#include "GUIDialogBoxeeRssFeedInfo.h"
#include "GUIDialogBoxeeDropdown.h"
#include "GUIDialogBoxeeSortDropdown.h"
#include "GUIDialogBoxeeTechInfo.h"
#include "GUIDialogBoxeeSearch.h"
#include "GUIDialogBoxeeGlobalSearch.h"
#include "GUIDialogBoxeeShortcutAction.h"
#include "GUIWindowSettingsScreenSimpleCalibration.h"
#include "DllLibid3tag.h"
#include "ffmpeg/libavcodec/avcodec.h"
#include "cores/ffmpeg/DllAvCodec.h"
#include "cores/ffmpeg/DllAvFormat.h"
#include "cores/ffmpeg/DllSwScale.h"
#include "cores/ffmpeg/DllPostProc.h"
#include "AppManager.h"
#include "BrowserService.h"
#include "GUIDialogBoxeeNetworkNotification.h"
#include "cores/AudioRenderers/AudioUtils.h"
#include "MusicInfoTagLoaderFactory.h"
#include "GUIDialogBoxeeExitVideo.h"
#include "GUIDialogBoxeeQuickTip.h"
#include "GUIDialogBoxeeSeekBar.h"
#include "GUIDialogBoxeePair.h"
#include "GUIWindowBoxeeSettingsDevices.h"
#include "GUIDialogBoxeeChannelFilter.h"

#ifdef HAS_EMBEDDED
#include "InitializeBoxManager.h"
#include "GUIDialogFirstTimeUseLang.h"
#include "GUIDialogFirstTimeUseWelcome.h"
#include "GUIDialogFirstTimeUseWireless.h"
#include "GUIDialogFirstTimeUseConfWirelessPassword.h"
#include "GUIDialogFirstTimeUseConfWirelessSecurity.h"
#include "GUIDialogFirstTimeUseConfWirelessSSID.h"
#include "GUIDialogFirstTimeUseConfNetwork.h"
#include "GUIDialogFirstTimeUseNetworkMessage.h"
#include "GUIDialogFirstTimeUseUpdateMessage.h"
#include "GUIDialogFirstTimeUseUpdateProgress.h"
#include "GUIDialogFirstTimeUseSimpleMessage.h"
#include "GUIWindowFirstTimeUseBackground.h"
#include "GUIWindowFirstTimeUseCalibration.h"
#endif

#ifdef HAS_INTEL_SMD
#include "IntelSMDGlobals.h"
#endif

#ifdef HAS_DVB
#include "GUIDialogBoxeeOTAConfiguration.h"
#include "GUIDialogBoxeeOTALocationConfiguration.h"
#include "GUIDialogBoxeeOTAFacebookConnect.h"
#include "GUIDialogBoxeeOTANoChannels.h"
#include "GUIDialogBoxeeLiveTvScan.h"
#include "BoxeeOTAConfigurationManager.h"
#endif

#include "GUIWindowTestBadPixelsManager.h"
#include "GUIWindowTestBadPixels.h"

#include "bxoemconfiguration.h"
#include "BoxeeApiHttpServer.h"
#include "utils/AnnouncementManager.h"

// Payments UI
#include "GUIDialogBoxeePaymentProducts.h"
#include "GUIDialogBoxeePaymentTou.h"
#include "GUIDialogBoxeePaymentUserData.h"
#include "GUIDialogBoxeePaymentOkPlay.h"
#include "GUIDialogBoxeePaymentWaitForServerApproval.h"

// Login Wizard
#include "GUIDialogBoxeeLoginWizardChooseUserToAddType.h"
#include "GUIDialogBoxeeLoginWizardAddExistingUser.h"
#include "GUIDialogBoxeeLoginWizardAddNewUser.h"
#include "GUIDialogBoxeeLoginWizardNewUserDetails.h"
#include "GUIDialogBoxeeLoginWizardTOU.h"
#include "GUIDialogBoxeeLoginWizardConnectSocialServices.h"
#include "GUIDialogBoxeeLoginWizardMenuCust.h"
#include "GUIDialogBoxeeLoginWizardConfirmation.h"
#include "GUIDialogBoxeeLoginWizardQuickTip.h"
#include "GUIDialogBoxeeLoginWizardQuickTipAirPlay.h"
#include "GUIDialogBoxeeLoginEditExistingUser.h"

// end boxee

#include "FileSystem/SMBDirectory.h"
#include "FileSystem/FileCurl.h"

#ifdef HAS_PERFORMACE_SAMPLE
#include "utils/PerformanceSample.h"
#else
#define MEASURE_FUNCTION
#endif

#if defined(HAS_SDL) && defined(_WIN32)
#include <SDL/SDL_syswm.h>
#endif
#ifdef _WIN32
#include <shlobj.h>
#include "win32util.h"
#endif
#ifdef HAS_XRANDR
#include "XRandR.h"
#endif
#ifdef __APPLE__
#include "CocoaInterface.h"
#include "XBMCHelper.h"
#endif
#ifdef HAVE_LIBVDPAU
#include "cores/dvdplayer/DVDCodecs/Video/VDPAU.h"
#endif

#ifdef _LINUX
#include <sys/utsname.h>
#endif

#ifdef _WIN32
#include "win32/RemoteWrapper.h"
#endif

#include "cores/dlgcache.h"

#ifdef HAS_DVD_DRIVE
#include "lib/libcdio/logging.h"
#endif

#ifdef HAS_HAL
#include "linux/HALManager.h"
#endif

#include "MediaManager.h"

#include "GUISound.h"

#ifdef _LINUX
#include "XHandle.h"
#endif

#ifdef HAS_LIRC
#include "common/LIRC.h"
#endif
#ifdef HAS_IRSERVERSUITE
#include "common/IRServerSuite/IRServerSuite.h"
#endif
#ifdef HAS_REMOTECONTROL
#include "common/IntelCERemote.h"
#endif
#ifdef HAS_BOXEE_HAL
#include "HalServices.h"
#include "HalListenerImpl.h"
#endif

#ifdef HAS_DVB
#include "xbmc/cores/dvb/dvbmanager.h"
#include "GUIWindowBoxeeLiveTv.h"
#include "GUIDialogBoxeeLiveTvCtx.h"
#include "GUIDialogBoxeeLiveTvEditChannels.h"
#include "GUIDialogBoxeeLiveTvInfo.h"
#endif

#ifdef CANMORE
#include <gdl.h>
#endif

#include "VersionInfo.h"
#include "Variant.h"

// #define INFO_PAGE 3

#define HTTP_SERVER_PORT 9876

using namespace std;
using namespace XBMC;
using namespace XFILE;
using namespace DIRECTORY;
#ifdef HAS_DVD_DRIVE
using namespace MEDIA_DETECT;
#endif
using namespace PLAYLIST;
using namespace VIDEO;
using namespace MUSIC_INFO;
#ifdef HAS_EVENT_SERVER
using namespace EVENTSERVER;
#endif
#ifdef HAS_DBUS_SERVER
using namespace DBUSSERVER;
#endif
#ifdef HAS_JSONRPC
using namespace JSONRPC;
#endif

// boxee stuff
using namespace BOXEE;
// end boxee

#ifdef __APPLE__
// better define it in a .cpp that in a .mm - for initialization purposes
CWinSystemOSXGL g_Windowing;
#endif

// uncomment this if you want to use release libs in the debug build.
// Atm this saves you 7 mb of memory
#define USE_RELEASE_LIBS

#if defined(_WIN32)
#if defined(_DEBUG) && !defined(USE_RELEASE_LIBS)
  #if defined(HAS_FILESYSTEM)
    #pragma comment (lib,"../../xbmc/lib/libXBMS/libXBMSd.lib")    // SECTIONNAME=LIBXBMS
    #pragma comment (lib,"../../xbmc/lib/libxdaap/libxdaapd.lib") // SECTIONNAME=LIBXDAAP
    #pragma comment (lib,"../../xbmc/lib/libRTV/libRTVd_win32.lib")
  #endif
  #pragma comment (lib,"../../xbmc/lib/libGoAhead/goahead_win32d.lib") // SECTIONNAME=LIBHTTP
  #pragma comment (lib,"../../xbmc/lib/sqLite/libSQLite3_win32d.lib")
  #pragma comment (lib,"../../xbmc/lib/libshout/libshout_win32d.lib" )
  #pragma comment (lib,"../../xbmc/lib/libcdio/libcdio_win32d.lib" )
  #pragma comment (lib,"../../xbmc/lib/libiconv/libiconvd.lib")
  #pragma comment (lib,"../../xbmc/lib/libfribidi/libfribidid.lib")
  #pragma comment (lib,"../../xbmc/lib/libpcre/libpcred.lib")
  #pragma comment (lib,"../../xbmc/lib/libsamplerate/libsamplerate_win32d.lib")
#else
  #ifdef HAS_FILESYSTEM
    #pragma comment (lib,"../../xbmc/lib/libXBMS/libXBMS.lib")
    #pragma comment (lib,"../../xbmc/lib/libxdaap/libxdaap.lib")
    #pragma comment (lib,"../../xbmc/lib/libRTV/libRTV_win32.lib")
  #endif
  #pragma comment (lib,"../../xbmc/lib/libGoAhead/goahead_win32.lib")
  #pragma comment (lib,"../../xbmc/lib/sqLite/libSQLite3_win32.lib")
  #pragma comment (lib,"../../xbmc/lib/libshout/libshout_win32.lib" )
  #pragma comment (lib,"../../xbmc/lib/libcdio/libcdio_win32.lib" )
  #pragma comment (lib,"../../xbmc/lib/libiconv/libiconv.lib")
  #pragma comment (lib,"../../xbmc/lib/libfribidi/libfribidi.lib")
  #pragma comment (lib,"../../xbmc/lib/libpcre/libpcre.lib")
  #pragma comment (lib,"../../xbmc/lib/libsamplerate/libsamplerate_win32.lib")
 #endif
 #endif

#define MAX_FFWD_SPEED 5
#define HOME_SCREEN_REFRESH_INTERVAL_IN_MIN 5
#define CHECK_REMOVE_OLD_THUMBNAILS_INTERVAL_IN_DAYS 1
#define MAX_TIME_THUMB_CAN_EXIST_IN_DAYS 7

#define ITEMS_HISTORY_FILE_PATH "special://profile/playbackhistory.dmp"
#define BROWSER_HISTORY_FILE_PATH "special://profile/browserhistory.dmp"

#ifdef HAS_PERFORMANCETIMER
#define PERFORMANCETIMER_BEGIN int64_t performanceTimer = CurrentHostCounter();
#define PERFORMANCETIMER_SPLIT { int64_t performanceTimerNow = CurrentHostCounter();\
  printf("%s:%d elapsed: %llu\n", __FUNCTION__, __LINE__, performanceTimerNow - performanceTimer);\
  performanceTimer = performanceTimerNow; }
#else
#define PERFORMANCETIMER_BEGIN
#define PERFORMANCETIMER_SPLIT
#endif

#define NUM_THUMB_CREATOR_THREADS 2

CStdString g_LoadErrorStr;

// logger method for boxee library
#include "lib/libBoxee/logger.h"
void BoxeeLoggerFunction(const char *szMsg) {
  CLog::Log(LOGDEBUG,"Boxee: %s", szMsg);
}

CPlayScheduler::CPlayScheduler()
{
  m_pSem = SDL_CreateSemaphore(0);
}

CPlayScheduler::~CPlayScheduler()
{
  if(m_pSem)
      SDL_DestroySemaphore(m_pSem);
}

void CPlayScheduler::Stop()
{
  m_bStop = true;
  SDL_SemPost(m_pSem);
}

void CPlayScheduler::Schedule(CBackgroundPlayer *player)
{
  {
    CSingleLock lock(m_lock);
    m_queue.push_front(player); 
  }

  SDL_SemPost(m_pSem);

#if defined(HAS_EMBEDDED) && !defined(__APPLE__)
  sched_yield();
#endif
}


void CPlayScheduler::Process()
{
  while(!m_bStop)
  {
    CBackgroundPlayer* player = NULL;

    CSingleLock lock(m_lock);
    if(!m_queue.empty())
    {
      player = m_queue.front();
      m_queue.pop_front();

      while (m_queue.size() > 0)
      {
        CBackgroundPlayer* p = m_queue.front();

        m_queue.pop_front();
        delete p;
      }

    }
    lock.Leave();

    if(player)
    {
      player->Create(false);
#if defined(HAS_EMBEDDED) && !defined(__APPLE__)
      sched_yield();
#endif 
      int timeout = 50;
      while(!player->WaitForThreadExit(timeout))
      {
        if(SDL_SemValue(m_pSem) > 0)
        {
           g_application.StopPlaying();
           timeout = INFINITE;
        }
      }

      delete player;
    }

    SDL_SemWait(m_pSem);
  }
}

CBackgroundPlayer::CBackgroundPlayer(const CFileItem &item, int iPlayList) : m_iPlayList(iPlayList)
{
  m_item = new CFileItem;
  *m_item = item;
}

CBackgroundPlayer::~CBackgroundPlayer()
{
}

void CBackgroundPlayer::Process()
{
  g_application.PlayMediaSync(*m_item, m_iPlayList);
}
//extern IDirectSoundRenderer* m_pAudioDecoder;
CApplication::CApplication(void) : m_itemCurrentFile(new CFileItem), m_progressTrackingItem(new CFileItem), m_BoxeeItemsHistory(ITEMS_HISTORY_FILE_PATH,200), m_BoxeeBrowserHistory(BROWSER_HISTORY_FILE_PATH,50)
{
//Boxee
  m_bVerbose = false;
  m_bDrawControlBorders = false;
  m_pPythonManager = NULL;
  m_homeScreenTimerOn = false;
  m_userLoggedIn = false;
  m_inSlideshowScreensaver = false;
  m_pBrowserService = NULL;
//end Boxee
  m_iPlaySpeed = 1;
#ifdef HAS_WEB_SERVER
  m_pWebServer = NULL;
  m_pXbmcHttp = NULL;
  m_prevMedia="";
#endif
  m_pFileZilla = NULL;
  m_pPlayer = NULL;
  m_bInactive = false;
  m_bScreenSave = false;
  m_dpms = NULL;
  m_dpmsIsActive = false;
  m_iScreenSaveLock = 0;
  //m_dwSaverTick = CTimeUtils::GetTimeMS();
#ifdef __APPLE__
  m_dwOSXscreensaverTicks = CTimeUtils::GetTimeMS();
#endif
  m_skinReloadTime = 0;
  m_bInitializing = true;
  m_eForcedNextPlayer = EPC_NONE;
  m_strPlayListFile = "";
  m_nextPlaylistItem = -1;
  m_bPlaybackStarting = false;

#ifdef HAS_GLX
  XInitThreads();
#endif

  //true while we in IsPaused mode! Workaround for OnPaused, which must be add. after v2.0
  m_bIsPaused = false;

  /* for now always keep this around */
#ifdef HAS_KARAOKE
  m_pKaraokeMgr = new CKaraokeLyricsManager();
#endif
  m_currentStack = new CFileItemList;

#ifdef HAS_SDL
  m_frameCount = 0;
  m_frameMutex = SDL_CreateMutex();
  m_frameCond = SDL_CreateCond();
#endif

  m_bPresentFrame = false;
  m_bPlatformDirectories = true;

  m_bStandalone = false;
  m_bEnableLegacyRes = false;
  m_bRunResumeJobs = false;
  m_bSystemScreenSaverEnable = false;
  m_infoPage.Register(INFOPAGE_APPLICATION, "CApplication", this);
  m_bAllSettingsLoaded = false;

#ifdef HAS_EMBEDDED
  m_bEnableFTU = true;
  m_wasPlaying = true;
  m_wasConnectedToInternet = false;
  m_dpmsWasActive = true;
  m_wasSuspended = true;
  SetLeds();
#endif

  m_renderingEnabled = true;

  m_playScheduler.Create();
  m_previosMouseLocation.x = m_previosMouseLocation.y = 0.0;
}

#ifdef HAS_EMBEDDED
void CApplication::SetLeds()
{
  if (IsPlaying() != m_wasPlaying || IsConnectedToInternet() != m_wasConnectedToInternet ||
      m_dpmsIsActive != m_dpmsWasActive || m_wasSuspended != g_powerManager.IsSuspended())
  {
    m_wasSuspended = g_powerManager.IsSuspended();
    m_wasPlaying = IsPlaying();
    m_wasConnectedToInternet = IsConnectedToInternet();
    m_dpmsWasActive = m_dpmsIsActive;
    CHalServicesFactory::GetInstance().SetLEDState(m_wasPlaying, m_wasConnectedToInternet, m_dpmsWasActive | m_wasSuspended);
  }
}
#endif

CApplication::~CApplication(void)
{
  delete m_currentStack;

#ifdef HAS_KARAOKE
    delete m_pKaraokeMgr;
#endif

#ifdef HAS_SDL
    if (m_frameMutex)
    SDL_DestroyMutex(m_frameMutex);

  if (m_frameCond)
    SDL_DestroyCond(m_frameCond);
#endif
  delete m_dpms;
}

bool CApplication::OnEvent(XBMC_Event& newEvent)
{

  switch(newEvent.type)
  {
    case XBMC_QUIT:
      if (!g_application.m_bStop)
        g_application.getApplicationMessenger().Quit();
      break;
    case XBMC_KEYDOWN:
    case XBMC_KEYUP:
      g_Keyboard.HandleEvent(newEvent);
      g_application.ProcessKeyboard();
      break;
    case XBMC_MOUSEBUTTONDOWN:
    case XBMC_MOUSEBUTTONUP:
    case XBMC_MOUSEMOTION:
        g_Mouse.HandleEvent(newEvent);
        g_application.ProcessMouse();
        g_Mouse.ResetClicks();
      break;
    case XBMC_VIDEORESIZE:
      if (g_application.m_bInitializing)
        break;
      
      if(g_advancedSettings.m_fullScreen )
      {
        if(g_guiSettings.m_LookAndFeelResolution != RES_CUSTOM)
          g_guiSettings.SetInt("videoscreen.resolution", RES_DESKTOP);
      }
      else
      {
        g_Windowing.SetWindowResolution(newEvent.resize.w, newEvent.resize.h);
        g_graphicsContext.SetVideoResolution(RES_WINDOW, true);
        g_guiSettings.SetInt("window.width", newEvent.resize.w);
        g_guiSettings.SetInt("window.height", newEvent.resize.h);

        if(g_guiSettings.m_LookAndFeelResolution != RES_CUSTOM)
          g_guiSettings.SetInt("videoscreen.resolution", RES_WINDOW);
      }

      g_settings.Save();
      
      break;
  }
  return true;
  }

// This function does not return!
void CApplication::FatalErrorHandler(bool WindowSystemInitialized, bool MapDrives, bool InitNetwork)
{
  // XBMC couldn't start for some reason...
  // g_LoadErrorStr should contain the reason
  fprintf(stderr, "Fatal error encountered, aborting\n");
  fprintf(stderr, "Error log at %sxbmc.log\n", g_stSettings.m_logFolder.c_str());
#ifdef _WIN32
	MessageBox(NULL, g_LoadErrorStr.c_str(), "Fatal Error" , MB_OK|MB_ICONERROR);
#endif
  abort();
  }

#ifndef _LINUX
LONG WINAPI CApplication::UnhandledExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
  PCSTR pExceptionString = "Unknown exception code";

#define STRINGIFY_EXCEPTION(code) case code: pExceptionString = #code; break

  switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
  {
    STRINGIFY_EXCEPTION(EXCEPTION_ACCESS_VIOLATION);
    STRINGIFY_EXCEPTION(EXCEPTION_ARRAY_BOUNDS_EXCEEDED);
    STRINGIFY_EXCEPTION(EXCEPTION_BREAKPOINT);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_DENORMAL_OPERAND);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_DIVIDE_BY_ZERO);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_INEXACT_RESULT);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_INVALID_OPERATION);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_OVERFLOW);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_STACK_CHECK);
    STRINGIFY_EXCEPTION(EXCEPTION_ILLEGAL_INSTRUCTION);
    STRINGIFY_EXCEPTION(EXCEPTION_INT_DIVIDE_BY_ZERO);
    STRINGIFY_EXCEPTION(EXCEPTION_INT_OVERFLOW);
    STRINGIFY_EXCEPTION(EXCEPTION_INVALID_DISPOSITION);
    STRINGIFY_EXCEPTION(EXCEPTION_NONCONTINUABLE_EXCEPTION);
    STRINGIFY_EXCEPTION(EXCEPTION_SINGLE_STEP);
  }
#undef STRINGIFY_EXCEPTION

  g_LoadErrorStr.Format("%s (0x%08x)\n at 0x%08x",
                        pExceptionString, ExceptionInfo->ExceptionRecord->ExceptionCode,
                        ExceptionInfo->ExceptionRecord->ExceptionAddress);

  CLog::Log(LOGFATAL, "%s", g_LoadErrorStr.c_str());

  return ExceptionInfo->ExceptionRecord->ExceptionCode;
}
#endif

extern "C" void __stdcall init_emu_environ();
extern "C" void __stdcall update_emu_environ();

#ifdef _LINUX
void signal_handler(int v)
{
  std::string s((v==SIGSEGV)?"(SIGSEGV)":
  (v==SIGINT)?"(SIGINT)":
  (v==SIGABRT)?"(SIGABRT)":
  (v==SIGFPE)?"(SIGFPE)": "(unknown)");
  printf("signal %s (%d) caught.\n",s.c_str(), v);

#ifdef HAS_EMBEDDED
  if (v == SIGINT)
  {
    signal(SIGINT, SIG_DFL);
  }
#endif

  raise(v);
}
#endif

//
// Utility function used to copy files from the application bundle
// over to the user data directory in Application Support/XBMC.
//
static void CopyUserDataIfNeeded(const CStdString &strPath, const CStdString &file)
{
  CStdString destPath = CUtil::AddFileToFolder(strPath, file);
  if (!CFile::Exists(destPath))
  {
    // need to copy it across
    CStdString srcPath = CUtil::AddFileToFolder("special://xbmc/userdata/", file);
    CFile::Cache(PTH_IC(srcPath), destPath);
  }
}

void CApplication::Preflight()
    {      
  // run any platform preflight scripts.
#ifdef __APPLE__
  CStdString install_path;
  
  CUtil::GetHomePath(install_path);
  setenv("XBMC_HOME", install_path.c_str(), 0);
  install_path += "/tools/osx/preflight";
  system(install_path.c_str());
#endif 
      }

HRESULT CApplication::Create(HWND hWnd)
{
  g_guiSettings.Initialize();  // Initialize default Settings
  g_settings.Initialize(); //Initialize default AdvancedSettings

  m_bSystemScreenSaverEnable = g_Windowing.IsSystemScreenSaverEnabled();
  g_Windowing.EnableSystemScreenSaver(false);

#ifdef _LINUX
  tzset();   // Initialize timezone information variables
#endif

  // Grab a handle to our thread to be used later in identifying the render thread.
  m_threadID = CThread::GetCurrentThreadId();

#ifndef _LINUX
  //floating point precision to 24 bits (faster performance)
  _controlfp(_PC_24, _MCW_PC);

  /* install win32 exception translator, win32 exceptions
   * can now be caught using c++ try catch */
  win32_exception::install_handler();
#endif

  CProfile *profile;

  // only the InitDirectories* for the current platform should return
  // non-null (if at all i.e. to set a profile)
  // putting this before the first log entries saves another ifdef for g_stSettings.m_logFolder
  profile = InitDirectoriesLinux();
  if (!profile)
    profile = InitDirectoriesOSX();
  if (!profile)
    profile = InitDirectoriesWin32();
  if (profile)
  {
    profile->setName("Master user");
    profile->setLockMode(LOCK_MODE_EVERYONE);
    profile->setLockCode("");
    profile->setLastLockCode("");
    profile->setDate("");
    g_settings.m_vecProfiles.push_back(*profile);
    delete profile;
  }

  CLog::Log(LOGNOTICE, "-----------------------------------------------------------------------");
#if defined(__APPLE__)
  CLog::Log(LOGNOTICE, "Starting BOXEE, Platform: Mac OS X.  Built on %s (%s)", __DATE__, SVN_REV);
#elif defined(_LINUX)
  CLog::Log(LOGNOTICE, "Starting BOXEE, Platform: GNU/Linux.  Built on %s (%s)", __DATE__, SVN_REV);
#elif defined(_WIN32) 
  CLog::Log(LOGNOTICE, "Starting BOXEE, Platform: %s.  Built on %s (%s, compiler %i)",g_sysinfo.GetKernelVersion().c_str(), __DATE__, SVN_REV, _MSC_VER);
  CLog::Log(LOGNOTICE, g_cpuInfo.getCPUModel().c_str());
  CLog::Log(LOGNOTICE, CWIN32Util::GetResInfoString());
  CLog::Log(LOGNOTICE, "Running with %s rights", (CWIN32Util::IsCurrentUserLocalAdministrator() == TRUE) ? "administrator" : "restricted");
#endif
  CSpecialProtocol::LogPaths();

  char szXBEFileName[1024];
  CIoSupport::GetXbePath(szXBEFileName);
  CLog::Log(LOGNOTICE, "The executable running is: %s", szXBEFileName);
  CLog::Log(LOGNOTICE, "Log File is located: %sboxee.log", g_stSettings.m_logFolder.c_str());
  CLog::Log(LOGNOTICE, "-----------------------------------------------------------------------");

#ifdef USE_MINI_DUMPS
  CStdString logFile;
  logFile.Format("%sboxee.log", g_stSettings.m_logFolder.c_str());
  g_MiniDumps.AddLogFile(logFile.c_str());
#endif

  g_guiSettings.LoadCustomSettingsOrder();

  CStdString strExecutablePath;
  CUtil::GetHomePath(strExecutablePath);

  // if we are running from DVD our UserData location will be TDATA
  if (CUtil::IsDVD(strExecutablePath))
  {
    // TODO: Should we copy over any UserData folder from the DVD?
    if (!CFile::Exists("special://masterprofile/guisettings.xml")) // first run - cache userdata folder
    {
      CFileItemList items;
      CUtil::GetRecursiveListing("special://xbmc/userdata",items,"");
      for (int i=0;i<items.Size();++i)
          CFile::Cache(items[i]->m_strPath,"special://masterprofile/"+CUtil::GetFileName(items[i]->m_strPath));
    }
    g_settings.m_vecProfiles[0].setDirectory("special://masterprofile/");
    g_stSettings.m_logFolder = "special://masterprofile/";
  }

#ifdef HAS_XRANDR
  g_xrandr.LoadCustomModeLinesToAllOutputs();
#endif

  // Init our DllLoaders emu env
  init_emu_environ();

#ifdef HAS_EMBEDDED
  signal(SIGINT, signal_handler);
#endif

#ifdef HAS_SDL
  CLog::Log(LOGDEBUG, "Setup SDL");

  /* Clean up on exit, exit on window close and interrupt */
  atexit(SDL_Quit);

  uint32_t sdlFlags = 0;

#ifdef HAS_SDL_OPENGL
  sdlFlags |= SDL_INIT_VIDEO;
#endif

#ifdef HAS_SDL_AUDIO
  sdlFlags |= SDL_INIT_AUDIO;
#endif

#ifdef HAS_SDL_JOYSTICK
  sdlFlags |= SDL_INIT_JOYSTICK;
#endif

#endif // HAS_SDL

#ifdef _LINUX
  // for nvidia cards - vsync currently ALWAYS enabled.
  // the reason is that after screen has been setup changing this env var will make no difference.
  setenv("__GL_SYNC_TO_VBLANK", "1", 0);
  setenv("__GL_YIELD", "USLEEP", 0);
#endif

#ifdef HAS_SDL
  if (SDL_Init(sdlFlags) != 0)
  {
        CLog::Log(LOGFATAL, "XBAppEx: Unable to initialize SDL: %s", SDL_GetError());
        return E_FAIL;
  }
#endif

  // for python scripts that check the OS
#ifdef __APPLE__
  setenv("OS","OS X",true);
#elif defined(_LINUX)
  setenv("OS","Linux",true);
#else
  SetEnvironmentVariable("OS","Windows");
#endif

#ifdef __APPLE__
  CLog::Log(LOGINFO,"App Version: %s", Cocoa_GetAppVersion());
#endif
  
  // Reset FPS to the display FPS. 
  g_infoManager.ResetFPS(g_graphicsContext.GetFPS());


  // Initialize core peripheral port support. Note: If these parameters
  // are 0 and NULL, respectively, then the default number and types of
  // controllers will be initialized.
  if (!g_Windowing.InitWindowSystem())
  {
    CLog::Log(LOGFATAL, "CApplication::Create: Unable to init windowing system");
    return E_FAIL;
  }

  CLog::Log(LOGDEBUG, "Drives are mapped");

  BXOEMConfiguration::GetInstance().Load();
  g_guiSettings.SetString("server.hostname",BoxeeUtils::GetPlatformDefaultHostName());

  CLog::Log(LOGNOTICE, "load settings...");
  g_LoadErrorStr = "Unable to load settings";
  g_settings.m_iLastUsedProfileIndex = g_settings.m_iLastLoadedProfileIndex;
  if (g_settings.bUseLoginScreen && g_settings.m_iLastLoadedProfileIndex != 0)
    g_settings.m_iLastLoadedProfileIndex = 0;

	  m_bAllSettingsLoaded = g_settings.Load(m_bXboxMediacenterLoaded, m_bSettingsLoaded);
  if (!m_bAllSettingsLoaded)
  {
	  g_LoadErrorStr = "Error loading settings";
    FatalErrorHandler(true, true, true);
  }

#ifdef HAS_INTEL_SMD
  g_IntelSMDGlobals.InitAudio();
#endif

  update_emu_environ();//apply the GUI settings

#ifdef __APPLE__
  // Configure and possible manually start the helper.
  g_xbmcHelper.Configure();
#endif

  // update the window resolution
  g_Windowing.SetWindowResolution(g_guiSettings.GetInt("window.width"), g_guiSettings.GetInt("window.height"));

  if (g_advancedSettings.m_startFullScreen && g_guiSettings.m_LookAndFeelResolution == RES_WINDOW)
    g_guiSettings.m_LookAndFeelResolution = RES_DESKTOP;

  if (!g_graphicsContext.IsValidResolution(g_guiSettings.m_LookAndFeelResolution))
  {
    // Oh uh - doesn't look good for starting in their wanted screenmode
    CLog::Log(LOGERROR, "The screen resolution requested is not valid, resetting to a valid mode");
    g_guiSettings.m_LookAndFeelResolution = RES_DESKTOP;
  }

  int savedWidth = g_guiSettings.GetInt("videoscreen.width");
  int savedHeight = g_guiSettings.GetInt("videoscreen.height");
  float savedRefresh = g_guiSettings.GetFloat("videoscreen.refresh");
  bool interlace = g_guiSettings.GetBool("videoscreen.interlace");

  //printf("#@#@ Saved screen res: width %d height %d refresh %f\n", savedWidth, savedHeight, savedRefresh);

  RESOLUTION savedRes = RES_INVALID;
  savedRes = g_graphicsContext.MatchResolution(savedWidth, savedHeight, savedRefresh, interlace);
#ifdef HAS_EMBEDDED
  // test if saved resolution is different that what we're about to set
  // This can happen if for some reason we're returning resolutions in different order
  if(savedRes != g_guiSettings.m_LookAndFeelResolution)
  {
    //printf("saved %d lookandfeel %d\n", savedRes, g_guiSettings.m_LookAndFeelResolution);
    CLog::Log(LOGERROR, "Different res: saved %d %d %d %f %d set %d %d %d %f %d\n",
        savedRes, savedWidth, savedHeight, savedRefresh, interlace,
        g_guiSettings.m_LookAndFeelResolution,
        g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iWidth,
        g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iHeight,
        g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].fRefreshRate,
        g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].dwFlags & D3DPRESENTFLAG_INTERLACED);

    if(g_graphicsContext.IsValidResolution(savedRes))
    {
      g_guiSettings.SetInt("videoscreen.resolution", savedRes);
      g_guiSettings.SetInt("videoplayer.displayresolution", savedRes);
      g_guiSettings.SetInt("pictures.displayresolution", savedRes);
      g_guiSettings.m_LookAndFeelResolution = savedRes;
      g_settings.Save();
    }
  }
#endif

#ifdef __APPLE__
  // force initial window creation to be windowed, if fullscreen, it will switch to it below
  // fixes the white screen of death if starting fullscreen and switching to windowed.
  bool bFullScreen = false;
  if (!g_Windowing.CreateNewWindow("BOXEE", bFullScreen, g_settings.m_ResInfo[RES_WINDOW], OnEvent))
  {
    CLog::Log(LOGFATAL, "CApplication::Create: Unable to create window");
    return E_FAIL;
  }
#else
  bool bFullScreen = g_guiSettings.m_LookAndFeelResolution != RES_WINDOW;
#if defined(HAS_GLES) && defined(_WIN32)
  bFullScreen = false;
#endif
  if (!g_Windowing.CreateNewWindow("BOXEE", bFullScreen,
      g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution], OnEvent))
  {
    // give it another try with different res
    CLog::Log(LOGERROR, "Failed to create window with res %d. Trying more res",g_guiSettings.m_LookAndFeelResolution);
    bool bFoundRes = false;
    RESOLUTION testRes = RES_DESKTOP;
    while(!bFoundRes &&  (testRes < (int)g_settings.m_ResInfo.size()))
    {
      g_guiSettings.m_LookAndFeelResolution = (RESOLUTION)testRes;
      if (g_Windowing.CreateNewWindow("BOXEE", bFullScreen,
          g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution], OnEvent))
        bFoundRes = true;
      else
        testRes = (RESOLUTION) (((int) testRes) + 1);
    }
    if (!bFoundRes)
    {
      CLog::Log(LOGFATAL, "CApplication::Create: Unable to create window");
      return E_FAIL;
    }
    else
    {
      /*
      g_guiSettings.SetInt("videoscreen.resolution", g_guiSettings.m_LookAndFeelResolution);
      g_guiSettings.SetInt("videoplayer.displayresolution", g_guiSettings.m_LookAndFeelResolution);
      g_guiSettings.SetInt("pictures.displayresolution", g_guiSettings.m_LookAndFeelResolution);
      */
    }
  }
#endif

    // Create the Mouse and Keyboard devices
  g_Mouse.Initialize(&hWnd);
  g_Keyboard.Initialize();
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE) || defined(HAS_REMOTECONTROL)
  g_RemoteControl.Initialize();
#endif
#ifdef HAS_SDL_JOYSTICK
  g_Joystick.Initialize(hWnd);
#endif

  if (!g_Windowing.InitRenderSystem())
  {    
    CLog::Log(LOGFATAL, "CApplication::Create: Unable to init rendering system");
    g_LoadErrorStr = g_Windowing.GetRenderSystemErrorStatus();
#ifdef _WIN32
    MessageBox(NULL, g_LoadErrorStr.c_str(), "Error" , MB_OK|MB_ICONERROR);
#endif
    return E_FAIL;
  }

  g_Windowing.EnableStencil(false);
  g_Windowing.EnableDepthTest(false);

  // set GUI res and force the clear of the screen
  g_graphicsContext.SetVideoResolution(g_guiSettings.m_LookAndFeelResolution);

  // assume skin is 720p
  g_graphicsContext.SetSkinResolution(RES_HDTV_720p);

  bool lowRes = !g_guiSettings.GetBool("videoscreen.hiresrendering");
  g_graphicsContext.SetRenderLowresGraphics(lowRes);

  m_splash = new CSplash("special://xbmc/media/Splash.png");
  m_splash->Show();

  m_boxeeDeviceManager.Initialize();

  // assume skin is 720p
  g_graphicsContext.SetSkinResolution(RES_HDTV_720p);

  // initialize our charset converter
  g_charsetConverter.reset();

  // Load the langinfo to have user charset <-> utf-8 conversion
  CStdString strLanguage = g_guiSettings.GetString("locale.language");
  strLanguage[0] = toupper(strLanguage[0]);

  CStdString strLangInfoPath;
  strLangInfoPath.Format("special://xbmc/language/%s/langinfo.xml", strLanguage.c_str());

  CLog::Log(LOGINFO, "load language info file [%s]", _P(strLangInfoPath).c_str());
  g_langInfo.Load(strLangInfoPath);

  CStdString strLanguagePath;
  strLanguagePath.Format("special://xbmc/language/%s/strings.xml", strLanguage.c_str());

  CLog::Log(LOGINFO, "load language file [%s]", _P(strLanguagePath).c_str());
  if (!g_localizeStrings.Load(strLanguagePath))
  {
	  g_LoadErrorStr = "Error loading Strings file: " + strLanguagePath;
    FatalErrorHandler(false, false, true);
  }

  CLog::Log(LOGINFO, "load keymapping");
  if (!CButtonTranslator::GetInstance().Load())
    FatalErrorHandler(false, false, true);

  CLog::Log(LOGINFO, "load charsets");
  if (!g_charsetConverter.Initialize())
    FatalErrorHandler(false, false, true);

  // check the skin file for testing purposes
  CStdString strSkinBase = "special://home/skin/";
  CStdString strSkinPath = strSkinBase + g_guiSettings.GetString("lookandfeel.skin");
  if (!CFile::Exists(strSkinPath)) {
    strSkinBase = "special://xbmc/skin/";
    strSkinPath = strSkinBase + g_guiSettings.GetString("lookandfeel.skin");
  }
  CLog::Log(LOGDEBUG, "Checking skin version of: %s", g_guiSettings.GetString("lookandfeel.skin").c_str());
  if (!g_SkinInfo.Check(strSkinPath))
  {
    // reset to the default skin (DEFAULT_SKIN)
    CLog::Log(LOGWARNING, "The above skin isn't suitable - checking the version of the default: %s", DEFAULT_SKIN);
    strSkinPath = strSkinBase + DEFAULT_SKIN;
    if (!g_SkinInfo.Check(strSkinPath))
    {
      g_LoadErrorStr.Format("No suitable skin version found.\nWe require at least version %5.4f \n", g_SkinInfo.GetMinVersion());
      FatalErrorHandler(false, false, true);
    }
  }
  int iResolution = g_graphicsContext.GetVideoResolution();
  CLog::Log(LOGDEBUG, " GUI format %ix%i %s",
            g_settings.m_ResInfo[iResolution].iWidth,
            g_settings.m_ResInfo[iResolution].iHeight,
            g_settings.m_ResInfo[iResolution].strMode.c_str());
  g_windowManager.Initialize();

  g_Mouse.SetEnabled(g_guiSettings.GetBool("lookandfeel.enablemouse"));

  g_settings.LoadAdditionalSettings();

  CUtil::InitRandomSeed();

#ifdef _WIN32
  CWIN32Util::AddRemovableDrives();
#endif

  return Initialize();
}

CProfile* CApplication::InitDirectoriesLinux()
{
/*
   The following is the directory mapping for Platform Specific Mode:

   special://xbmc/          => [read-only] system directory (/usr/share/xbmc)
   special://home/          => [read-write] user's directory that will override special://xbmc/ system-wide
         installations like skins, screensavers, etc.
         ($HOME/.xbmc)
                               NOTE: XBMC will look in both special://xbmc/skin and special://xbmc/skin for skins.
                                     Same applies to screensavers, sounds, etc.
   special://masterprofile/ => [read-write] userdata of master profile. It will by default be
                               mapped to special://home/userdata ($HOME/.xbmc/userdata)
   special://profile/       => [read-write] current profile's userdata directory.
                               Generally special://masterprofile for the master profile or
                               special://masterprofile/profiles/<profile_name> for other profiles.

   NOTE: All these root directories are lowercase. Some of the sub-directories
         might be mixed case.
*/

#if defined(_LINUX) && !defined(__APPLE__)
  CProfile* profile = NULL;

  CStdString userName;
  if (getenv("USER"))
    userName = getenv("USER");
  else
    userName = "root";

  CStdString userHome;
  if (getenv("HOME"))
    userHome = getenv("HOME");
  else
    userHome = "/root";
  CSpecialProtocol::SetUserHomePath(userHome);

  CStdString boxeeHomeDir = userHome;
  boxeeHomeDir.append("/.boxee");
  CreateDirectory(boxeeHomeDir.c_str(), NULL);

  CStdString strHomePath;
  CUtil::GetHomePath(strHomePath);
  setenv("XBMC_HOME", strHomePath.c_str(), 0);

#ifdef CANMORE
  CDirectory::Create("/tmp/profile");
#endif

  // In BOXEE we always run in platformDirectories
  //if (m_bPlatformDirectories)
  if (1)
  {
    CStdString logDir = "/tmp/";
    if (getenv("USER"))
    {
      logDir += getenv("USER");
      logDir += "-";
    }
    g_stSettings.m_logFolder = logDir;
    printf("And the log goes to... %sboxee.log\n", logDir.c_str());

    // map our special drives
    CSpecialProtocol::SetXBMCPath(strHomePath);
    CSpecialProtocol::SetHomePath(userHome + "/.boxee/UserData");
    CSpecialProtocol::SetMasterProfilePath(userHome + "/.boxee/UserData");

#ifdef HAS_EMBEDDED
    CDirectory::Create("/tmp/boxee");
    CSpecialProtocol::SetTempPath("/tmp/boxee");
#else
    CStdString strTempPath = CUtil::AddFileToFolder(userHome, ".boxee/temp"); 
    CSpecialProtocol::SetTempPath(strTempPath);
    CUtil::AddSlashAtEnd(strTempPath);
#endif

    bool bCopySystemPlugins = false;
    if (!CDirectory::Exists("special://home/plugins") )
       bCopySystemPlugins = true;

    CDirectory::Create("special://home/");
    CDirectory::Create("special://temp/");
    CDirectory::Create("special://home/skin");
    CDirectory::Create("special://home/keymaps");
    CDirectory::Create("special://home/visualisations");
    CDirectory::Create("special://home/screensavers");
    CDirectory::Create("special://home/sounds");
    CDirectory::Create("special://home/system");
    CDirectory::Create("special://home/plugins");
    CDirectory::Create("special://home/plugins/video");
    CDirectory::Create("special://home/plugins/music");
    CDirectory::Create("special://home/plugins/pictures");
    //CDirectory::Create("special://home/plugins/programs");
    CDirectory::Create("special://home/plugins/weather");
    CDirectory::Create("special://home/scripts");
    CDirectory::Create("special://home/scripts/My Scripts");    // FIXME: both scripts should be in 1 directory

    CDirectory::Create("special://masterprofile");

    // copy required files
    //CopyUserDataIfNeeded("special://masterprofile/", "Keymap.xml");  // Eventual FIXME.
    CopyUserDataIfNeeded("special://masterprofile/", "RssFeeds.xml");
    CopyUserDataIfNeeded("special://masterprofile/", "Lircmap.xml");
    CopyUserDataIfNeeded("special://masterprofile/", "LCD.xml");

    // BOXEE
    CDirectory::Create("special://home/boxee");
    CDirectory::Create("special://home/boxee/packages");	
    CDirectory::Create("special://home/subtitles");

#ifdef CANMORE
    CUtil::CreateTempDirectory("special://home/temp");
    CUtil::CreateTempDirectory("special://home/cache");
    CUtil::CreateTempDirectory("special://home/browser");
#else
    CDirectory::Create("special://home/temp");
#endif

    CDirectory::Create("special://home/apps");
    CDirectory::Create("special://home/dvb");
    // End BOXEE
    
    // copy system-wide plugins into userprofile
    if ( bCopySystemPlugins )
       CUtil::CopyDirRecursive("special://xbmc/plugins", "special://home/plugins");
  }
  else
  {
    CUtil::AddSlashAtEnd(strHomePath);
    g_stSettings.m_logFolder = strHomePath;

    CSpecialProtocol::SetXBMCPath(strHomePath);
    CSpecialProtocol::SetHomePath(strHomePath);
    CSpecialProtocol::SetMasterProfilePath(CUtil::AddFileToFolder(strHomePath, "UserData"));

    CStdString strTempPath = CUtil::AddFileToFolder(strHomePath, "temp"); 
    CSpecialProtocol::SetTempPath(strTempPath);
    CDirectory::Create("special://temp/");

    CUtil::AddSlashAtEnd(strTempPath);
    g_stSettings.m_logFolder = strTempPath;
  }

  g_settings.m_vecProfiles.clear();
  g_settings.LoadProfiles( PROFILES_FILE );

  if (g_settings.m_vecProfiles.size()==0)
  {
    profile = new CProfile;
    profile->setDirectory("special://masterprofile/");
  }
  return profile;
#else
  return NULL;
#endif
}

CProfile* CApplication::InitDirectoriesOSX()
{
#ifdef __APPLE__
  // We're going to manually manage the screensaver.
  setenv("SDL_VIDEO_ALLOW_SCREENSAVER", "1", true);
//  setenv("SDL_ENABLEAPPEVENTS", "1", 1);

  // Set the working directory to be the resource directory. This
  // allows two-step dynamic library loading to work, as long as the load
  // paths are configured correctly.
  //
  CStdString strExecutablePath;
  CUtil::GetHomePath(strExecutablePath);

  CStdString strWorkingDir = strExecutablePath;
  chdir(strWorkingDir.c_str());
  
  CProfile* profile = NULL;
  CStdString temp_path;

  // special://temp/ common for both
  CSpecialProtocol::SetTempPath("/tmp/boxee");
  CDirectory::Create("special://temp/");
  
  CStdString userHome;
  userHome = getenv("HOME");
  if (userHome.IsEmpty() )
  {
    userHome = "/root";
  }
  CSpecialProtocol::SetUserHomePath(userHome);
  
  // BOXEE
  // Logs do not exist in tiger.
  CreateDirectory(userHome+"/Library/Logs", NULL);

  CStdString str = userHome;
  str.append("/Library/Application Support");
  CreateDirectory(str.c_str(), NULL);
  str.append("/BOXEE");
  CreateDirectory(str.c_str(), NULL);
  CStdString str2 = str;
  str2.append("/Mounts");
  CreateDirectory(str2.c_str(), NULL);
  str.append("/UserData");
  CreateDirectory(str.c_str(), NULL);
  // End BOXEE

  CStdString strHomePath;
  CUtil::GetHomePath(strHomePath);
  setenv("XBMC_HOME", strHomePath.c_str(), 0);

  // OSX always runs with m_bPlatformDirectories == true
  //if (m_bPlatformDirectories)
  if (1)
  {
    CStdString logDir = userHome + "/Library/Logs/";
    g_stSettings.m_logFolder = logDir;
    printf("And the log goes to... %s\n", logDir.c_str());

    // //Library/Application\ Support/XBMC/
    CStdString install_path;
    CUtil::GetHomePath(install_path);
    CSpecialProtocol::SetXBMCPath(install_path);
    CSpecialProtocol::SetHomePath(userHome + "/Library/Application Support/BOXEE/UserData");
    CSpecialProtocol::SetMasterProfilePath(userHome + "/Library/Application Support/BOXEE/UserData");

#ifdef __APPLE__
    CStdString strTempPath = userHome + "/Library/Logs";
#endif
    CUtil::AddSlashAtEnd(strTempPath);
    g_stSettings.m_logFolder = strTempPath;

    bool bCopySystemPlugins = false;
    if (!CDirectory::Exists("special://home/plugins") )
       bCopySystemPlugins = true;

    CDirectory::Create("special://home/");
    CDirectory::Create("special://temp/");
    CDirectory::Create("special://home/skin");
    CDirectory::Create("special://home/keymaps");
    CDirectory::Create("special://home/visualisations");
    CDirectory::Create("special://home/screensavers");
    CDirectory::Create("special://home/sounds");
    CDirectory::Create("special://home/system");
    CDirectory::Create("special://home/plugins");
    CDirectory::Create("special://home/profiles");
    CDirectory::Create("special://home/plugins/video");
    CDirectory::Create("special://home/plugins/music");
    CDirectory::Create("special://home/plugins/pictures");
    CDirectory::Create("special://home/plugins/programs");
    CDirectory::Create("special://home/plugins/weather");
    CDirectory::Create("special://home/scripts");
    CDirectory::Create("special://home/scripts/My Scripts"); // FIXME: both scripts should be in 1 directory

    strTempPath = install_path + "/scripts";
    symlink( strTempPath.c_str(),  _P("special://home/scripts/Common Scripts").c_str() );

    str = install_path + "/scripts";
    symlink( str.c_str(),  _P("special://home/scripts/Common Scripts").c_str() );

    CDirectory::Create("special://masterprofile/");

    // copy required files
    //CopyUserDataIfNeeded("special://masterprofile/", "Keymap.xml"); // Eventual FIXME.
    CopyUserDataIfNeeded("special://masterprofile/", "RssFeeds.xml");
    CopyUserDataIfNeeded("special://masterprofile/", "Lircmap.xml");
    CopyUserDataIfNeeded("special://masterprofile/", "LCD.xml");

    // BOXEE
    CDirectory::Create("special://home/boxee");
    CDirectory::Create("special://home/boxee/packages");
    CDirectory::Create("special://home/subtitles");
    CDirectory::Create("special://home/apps");
    CDirectory::Create("special://home/temp");
    // End BOXEE    

    // copy system-wide plugins into userprofile
    if ( bCopySystemPlugins )
       CUtil::CopyDirRecursive("special://xbmc/plugins", "special://home/plugins");
  }
  else
  {
    CUtil::AddSlashAtEnd(strHomePath);
    g_stSettings.m_logFolder = strHomePath;

    CSpecialProtocol::SetXBMCPath(strHomePath);
    CSpecialProtocol::SetHomePath(strHomePath);
    CSpecialProtocol::SetMasterProfilePath(CUtil::AddFileToFolder(strHomePath, "userdata"));

    CStdString strTempPath = CUtil::AddFileToFolder(strHomePath, "temp");
    CSpecialProtocol::SetTempPath(strTempPath);
    CDirectory::Create("special://temp/");

    CUtil::AddSlashAtEnd(strTempPath);
    g_stSettings.m_logFolder = strTempPath;
  }

  g_settings.m_vecProfiles.clear();
  g_settings.LoadProfiles( PROFILES_FILE );

  if (g_settings.m_vecProfiles.size()==0)
  {
    profile = new CProfile;
    profile->setDirectory("special://masterprofile/");
  }
  return profile;
#else
  return NULL;
#endif
}

CProfile* CApplication::InitDirectoriesWin32()
{
#ifdef _WIN32
  CProfile* profile = NULL;
  CStdString strExecutablePath;

  CUtil::GetHomePath(strExecutablePath);
  SetEnvironmentVariable("XBMC_HOME", strExecutablePath.c_str());
  CSpecialProtocol::SetXBMCPath(strExecutablePath);

  // In BOXEE we always run in platformDirectories
  //if (m_bPlatformDirectories)
  if (1)
  {
    CStdString strWin32UserFolder = CWIN32Util::GetProfilePath();

	  CStdString str = CUtil::AddFileToFolder(strWin32UserFolder, "BOXEE");
	  CDirectory::Create(str);
	
    // create user/app data/XBM
    CStdString homePath = CUtil::AddFileToFolder(strWin32UserFolder, "BOXEE/userdata");
	
    // move log to platform dirs
    g_stSettings.m_logFolder = homePath;
    CUtil::AddSlashAtEnd(g_stSettings.m_logFolder);
	
    // map our special drives
    CSpecialProtocol::SetXBMCPath(strExecutablePath);
    CSpecialProtocol::SetHomePath(homePath);
    CSpecialProtocol::SetMasterProfilePath(homePath/*CUtil::AddFileToFolder(homePath, "userdata")*/);
    SetEnvironmentVariable("XBMC_PROFILE_USERDATA",_P("special://masterprofile").c_str());

    CSpecialProtocol::SetMyVideosPath(CWIN32Util::GetMyVideosPath());
    CSpecialProtocol::SetMyMusicPath(CWIN32Util::GetMyMusicPath());
    CSpecialProtocol::SetMyPicturesPath(CWIN32Util::GetMyPicturesPath());


    bool bCopySystemPlugins = false;
    if (!CDirectory::Exists("special://home/plugins") )
       bCopySystemPlugins = true;

	  CDirectory::Create("special://home/");
    CDirectory::Create("special://home/skin");
    CDirectory::Create("special://home/keymaps");
    CDirectory::Create("special://home/visualisations");
    CDirectory::Create("special://home/screensavers");
    CDirectory::Create("special://home/sounds");
    CDirectory::Create("special://home/system");
    CDirectory::Create("special://home/plugins");
    CDirectory::Create("special://home/plugins/video");
    CDirectory::Create("special://home/plugins/music");
    CDirectory::Create("special://home/plugins/pictures");
    CDirectory::Create("special://home/plugins/programs");
    CDirectory::Create("special://home/plugins/weather");
    CDirectory::Create("special://home/scripts");
    CDirectory::Create("special://home/temp");

    CDirectory::Create("special://masterprofile");

    // copy required files
    //CopyUserDataIfNeeded("special://masterprofile/", "Keymap.xml");  // Eventual FIXME.
    CopyUserDataIfNeeded("special://masterprofile/", "RssFeeds.xml");
    CopyUserDataIfNeeded("special://masterprofile/", "favourites.xml");
    CopyUserDataIfNeeded("special://masterprofile/", "IRSSmap.xml");
    CopyUserDataIfNeeded("special://masterprofile/", "LCD.xml");

    // copy system-wide plugins into userprofile
    if ( bCopySystemPlugins )
       CUtil::CopyDirRecursive("special://xbmc/plugins", "special://home/plugins");

    // create user/app data/XBMC/cache
    CSpecialProtocol::SetTempPath(CUtil::AddFileToFolder(homePath,"cache"));
    CDirectory::Create("special://temp");

    // BOXEE
    CDirectory::Create("special://home/boxee");
    CDirectory::Create("special://home/boxee/packages");	
    CDirectory::Create("special://home/subtitles");
    CDirectory::Create("special://home/apps");
    // End BOXEE     
  }
  else
  {
    g_stSettings.m_logFolder = strExecutablePath;
    CUtil::AddSlashAtEnd(g_stSettings.m_logFolder);
    CStdString strTempPath = CUtil::AddFileToFolder(strExecutablePath, "cache");
    CSpecialProtocol::SetTempPath(strTempPath);
    CDirectory::Create("special://temp/");

    CSpecialProtocol::SetHomePath(strExecutablePath);
    CSpecialProtocol::SetMasterProfilePath(CUtil::AddFileToFolder(strExecutablePath,"userdata"));
    SetEnvironmentVariable("XBMC_PROFILE_USERDATA",_P("special://masterprofile/").c_str());
  }

  CStdString strBuffer;
  WCHAR buffer[MAX_PATH];
  SHGetSpecialFolderPathW(NULL, buffer, CSIDL_MYDOCUMENTS, 0);
  g_charsetConverter.wToUTF8(buffer, strBuffer);
  CSpecialProtocol::SetUserHomePath(strBuffer.c_str());
	
  g_settings.m_vecProfiles.clear();
  g_settings.LoadProfiles(PROFILES_FILE);

    if (g_settings.m_vecProfiles.size()==0)
    {
      profile = new CProfile;
    profile->setDirectory("special://masterprofile/");
    }

  // Expand the DLL search path with our directories
  CWIN32Util::ExtendDllPath();

    return profile;
#else
  return NULL;
#endif
}

XBPython &CApplication::GetPythonManager()
{
#ifdef HAS_PYTHON
  if (!m_pPythonManager)
    m_pPythonManager = new XBPython;
#endif
  return *m_pPythonManager;
}

#ifdef APP_JS
XBJavaScript &CApplication::GetJavaScriptManager()
{
  if (!m_pJavaScriptManager)
    m_pJavaScriptManager = new XBJavaScript;

  return *m_pJavaScriptManager;  
}
#endif

void CApplication::DeleteDatabaseAndThumbnails()
{
  // Show confirmation dialog
#ifndef HAS_EMBEDDED
  if (CGUIDialogYesNo2::ShowAndGetInput(51560,51561))
  {
#else
  CStdString header = g_localizeStrings.Get(51560);
  CStdString line = g_localizeStrings.Get(51561) + g_localizeStrings.Get(51582);
  CStdString noLabel = g_localizeStrings.Get(222);
  CStdString yesLabel = g_localizeStrings.Get(51583);

  if (CGUIDialogYesNo2::ShowAndGetInput(header,line,noLabel,yesLabel))
  {
#endif
    BOXEE::BXMediaDatabase db;
#ifndef HAS_EMBEDDED
    CLog::Log(LOGDEBUG,"CApplication::DeleteDatabaseAndThumbnails - going to clear DB tables (ddat)");
    db.CleanTables();
#else
    CGUIDialogProgress* progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      progress->StartModal();
    }

    CLog::Log(LOGDEBUG,"CApplication::DeleteDatabaseAndThumbnails - going to stop MetadataEngine (ddat)");

    BOXEE::Boxee::GetInstance().GetMetadataEngine().Stop();

    CLog::Log(LOGDEBUG,"CApplication::DeleteDatabaseAndThumbnails - going to stop FileScanner (ddat)");

    m_FileScanner.Stop();

    CLog::Log(LOGDEBUG,"CApplication::DeleteDatabaseAndThumbnails - going to delete DB file [%s] (ddat)",db.getDatabaseFilePath().c_str());

    // delete the DB file before reboot
    CFile::Delete(db.getDatabaseFilePath());
#endif

    CLog::Log(LOGDEBUG,"CApplication::DeleteDatabaseAndThumbnails - going to delete thumbnails directory [%s] (ddat)",g_settings.GetThumbnailsFolder().c_str());

    m_tumbnailsMgr.HardReset();
    g_directoryCache.ClearAll();

#ifndef HAS_EMBEDDED
    VECSOURCES* pVecShares = g_settings.GetAllMediaSources(true);
    CMediaSource* pShare = NULL;
    for (IVECSOURCES it = pVecShares->begin(); !m_bStop && it != pVecShares->end() && !m_bStop; it++)
    {
      pShare = &(*it);
      db.AddMediaShare(pShare->strName, pShare->strPath, pShare->m_type, pShare->m_iScanType);
    }
#else
    if (progress)
    {
      progress->Close();
    }

    CLog::Log(LOGDEBUG,"CApplication::DeleteDatabaseAndThumbnails - going to send TMSG_QUIT message (ddat)");

    ThreadMessage tMsg(TMSG_QUIT);
    m_applicationMessenger.SendMessage(tMsg,true);
#endif
  }
}

HRESULT CApplication::Initialize()
{
  DbgInit();

  if (!g_curlInterface.IsLoaded())
     g_curlInterface.Load();

#ifdef _WIN32
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif

#if defined(_DEBUG) && defined(_WIN32)
  /*
	AllocConsole();
	freopen ("CONOUT$", "w", stderr ); 
  */
#endif

  CDirectory::Create("special://home/cache");
  if (!m_httpCache.Initialize("special://home/cache"))
  {
    CLog::Log(LOGERROR,"CApplication::Initialize - FAILED to initialize HTTP cache. Deleting and re-creating. (hsrv)");

    m_httpCache.Deinitialize();
    m_httpCache.Delete();
    if (!m_httpCache.Initialize("special://home/cache"))
    {
      CLog::Log(LOGERROR,"CApplication::Initialize - FAILED to initialize HTTP cache again. (hsrv)");
    }
  }

  CZeroconfBrowser::GetInstance()->Start();

#if defined(_LINUX) && defined(HAS_FILESYSTEM_SMB)
  // init smb global context
  smb.Init();
#endif
  // since we are statically linking with ffmpeg - initialize it now
#if defined(__APPLE__) && !defined(_LINUX)
  av_register_all();
  sws_rgb2rgb_init(SWS_CPU_CAPS_MMX2);
#endif

#ifdef HAS_DVD_DRIVE
  // turn off cdio logging
  cdio_loglevel_default = CDIO_LOG_ERROR;
#endif

  // lock a few important libraries in memory (so they dont get unloaded)
  m_lockedLibraries.push_back(new DllImageLib);
  m_lockedLibraries.push_back(new DllLibID3Tag);
#ifdef __APPLE__
  m_lockedLibraries.push_back(new DllAvCodec);
  m_lockedLibraries.push_back(new DllAvFormat);
  m_lockedLibraries.push_back(new DllSwScale);
  m_lockedLibraries.push_back(new DllPostProc);
#endif
  
  for (size_t i=0; i<m_lockedLibraries.size(); i++)
    m_lockedLibraries[i]->Load();
  
  // init python
#ifdef HAS_PYTHON
  GetPythonManager();
#endif
  
  g_settings.bUseLoginScreen = true;
  CGUISoundPlayer::GetInstance().Initialize();

#ifdef HAS_EMBEDDED
  g_infoManager.SetIsShowOemLogo(CFile::Exists(_P("special://xbmc/skin/boxee/media/oem.png")));
#endif
  //end Boxee

  CLog::Log(LOGDEBUG, "creating subdirectories");

  CDirectory::Create("special://home/screenshots");

  CStdString strScreenShots = g_guiSettings.GetString("pictures.screenshotpath",false);
  if (strScreenShots.empty())
    strScreenShots = _P("special://home/screenshots");

  CLog::Log(LOGDEBUG, "userdata folder: %s", g_settings.GetProfileUserDataFolder().c_str());
  CLog::Log(LOGDEBUG, "  recording folder:%s", g_guiSettings.GetString("mymusic.recordingpath",false).c_str());
  CLog::Log(LOGDEBUG, "  screenshots folder:%s", strScreenShots.c_str());

  // UserData folder layout:
  // UserData/
  //   Database/
  //     CDDb/
  //   Thumbnails/
  //     Music/
  //       temp/
  //     0 .. F/

  CDirectory::Create(g_settings.GetUserDataFolder());
  CDirectory::Create(g_settings.GetProfileUserDataFolder());
  g_settings.CreateProfileFolders();

  CDirectory::Create(g_settings.GetProfilesThumbFolder());
  CDirectory::Create("special://temp/temp"); // temp directory for python and dllGetTempPathA
  m_tumbnailsMgr.Initialize("special://home/Thumbnails");
  SetupHttpProxy();
  // initialize network
  if (!m_bXboxMediacenterLoaded)
  {
    CLog::Log(LOGINFO, "using default network settings");
    g_guiSettings.SetString("network.ipaddress", "192.168.0.100");
    g_guiSettings.SetString("network.subnet", "255.255.255.0");
    g_guiSettings.SetString("network.gateway", "192.168.0.1");
    g_guiSettings.SetString("network.dns", "192.168.0.1");
    g_guiSettings.SetBool("servers.ftpserver", true);
    g_guiSettings.SetBool("servers.webserver", false);
    g_guiSettings.SetBool("locale.timeserver", false);
  }

  StartServices();

  //Boxee section
  // boxee initialization
  
  BOXEE::Logger::GetLogger().SetLoggerFunction(BoxeeLoggerFunction);
  
  // Set temporary directory that will be used by Boxee library
#ifdef HAS_EMBEDDED
  BOXEE::Boxee::GetInstance().SetTempFolderPath("/tmp/boxee/");
#else
  BOXEE::Boxee::GetInstance().SetTempFolderPath(PTH_IC("special://home/temp/"));
#endif
 
  XFILE::CFileCurl::SetCookieJar(BOXEE::BXCurl::GetCookieJar());
  
#ifdef _LINUX
  setenv("BOXEE_COOKIEJAR", BOXEE::BXCurl::GetCookieJar().c_str(), 1);
#else
  SetEnvironmentVariable("BOXEE_COOKIEJAR", BOXEE::BXCurl::GetCookieJar().c_str());
#endif
  
  BOXEE::Boxee::GetInstance().setDatabaseFolderPath(_P("special://home/Database/"));
  
  BOXEE::Boxee::GetInstance().SetHttpCacheManager(&m_httpCache);
  
  // Set WebServer port for discovery
  BOXEE::Boxee::GetInstance().SetWebServerPort(g_guiSettings.GetString("servers.webserverport"));
  
  // Set WebServer password for discovery
  BOXEE::Boxee::GetInstance().SetIsWebServerPasswordEmpty((g_guiSettings.GetString("servers.webserverpassword")).IsEmpty());
  
  // Remove old update packages
  RemoveOldUpdatePackages();
  
  CStdString strShortSysInfo;
  CStdString strUserAgent;
  strUserAgent.Format("curl/7.16.3 (%s; en-US; beta) boxee/%s", g_sysinfo.GetKernelVersion().c_str(),  g_infoManager.GetVersion().c_str());
  
  BOXEE::BXCurl::SetGlobalUserAgent(strUserAgent.c_str());
  
  BOXEE::Boxee::GetInstance().SetPlatform(BoxeeUtils::GetPlatformStr());

#if 0
  m_httpServer = new CBoxeeApiHttpServer();
  if (m_httpServer)
  {
    CLog::Log(LOGDEBUG,"CApplication::Initialize - going to START HTTP server (hsrv)");
    m_httpServer->Start(HTTP_SERVER_PORT);
  }
  else
  {
    CLog::Log(LOGERROR,"CApplication::Initialize - FAILED to allocate HTTP server (hsrv)");
  }
#endif

  // We open and create the database here to avoid concurrency problems
  CVideoDatabase db;
  if (!db.Open())
  {
    CLog::Log(LOGERROR,"Unable to initialize video database");
  }
  db.Close();
  
  // create the boxee media db  
  // During construction the media database creates tables if this is necessary
  
  {
  BOXEE::BXDatabase bdb;
  
  if (!bdb.FileExists())
  {
    //if the file does not exist, create it
    CLog::Log(LOGDEBUG,"Media database file does not exist, creating database");
    bdb.Open();
    bdb.CreateTables(); //here's the version table created
  }
  else
    bdb.Open();
  
  if (!bdb.TablesExist())
    bdb.CreateTables(); //here's the version table created (incase the file does exist)
  else 
    bdb.UpdateTables(); //returns true
  
  BOXEE::BXVideoDatabase video_db;
  BOXEE::BXAudioDatabase audio_db;
  BOXEE::BXMediaDatabase media_db;
  
  if (!media_db.TablesExist())
    media_db.CreateTables();
  else 
    media_db.UpdateTables();
  
  if (!video_db.TablesExist())
    video_db.CreateTables();
  else 
    video_db.UpdateTables();
  
  if (!audio_db.TablesExist())
    audio_db.CreateTables();
  else 
    audio_db.UpdateTables();
  }

  m_ItemLoader = new CItemLoader;
  
  BOXEE::Boxee::GetInstance().Start();

  // initialize ThumbCreatorProcess
  CDirectory::s_thumbCreatorProcess.SetName("Directory Thumb Creator Processor");
  if (!CDirectory::s_thumbCreatorProcess.Start(NUM_THUMB_CREATOR_THREADS))
  {
    CLog::Log(LOGERROR,"FAILED to start ThumbCreatorProcess (foldert)");
  }

  // Initialize item loder
  m_ItemLoader->Init();
  
  m_watchDog.AddListener(new BoxeeWatchdogListener());
  
  // start the watchdog (monitor net)
  m_watchDog.Start();
    
#ifdef INFO_PAGE
  m_infoPage.Create();
#endif
    
  // Init DPMS, before creating the corresponding setting control.
  m_dpms = new DPMSSupport();
  g_guiSettings.GetSetting("screensaver.sep_powersaving")->SetVisible(
      m_dpms->IsSupported());
  g_guiSettings.GetSetting("screensaver.powersavingtime")->SetVisible(
      m_dpms->IsSupported());

  //Boxee
  g_windowManager.Add(new CGUIWindowBoxeeMain);           // window id = 0
  //end Boxee

  CLog::Log(LOGNOTICE, "load default skin:[%s]", g_guiSettings.GetString("lookandfeel.skin").c_str());
  LoadSkin(g_guiSettings.GetString("lookandfeel.skin"));

#ifndef _BOXEE_
  g_windowManager.Add(new CGUIWindowPrograms);                 // window id = 1
  g_windowManager.Add(new CGUIWindowPictures);                 // window id = 2
#endif

  g_windowManager.Add(new CGUIWindowFileManager);      // window id = 3
#ifndef _BOXEE_
  g_windowManager.Add(new CGUIWindowVideoFiles);          // window id = 6
#endif
  g_windowManager.Add(new CGUIWindowSettings);                 // window id = 4
  g_windowManager.Add(new CGUIWindowSystemInfo);               // window id = 7
#ifdef HAS_GL  
  g_windowManager.Add(new CGUIWindowTestPatternGL);      // window id = 8
#endif
#ifdef HAS_GLES
  g_windowManager.Add(new CGUIWindowTestPatternGLES);      // window id = 8
#endif
#ifdef HAS_DX
  g_windowManager.Add(new CGUIWindowTestPatternDX);      // window id = 8
#endif

  //Boxee
  g_windowManager.Add(new CGUIDialogBoxeeLoggingIn);         // window id = 255
  g_windowManager.Add(new CGUIDialogBoxeeCredits);         // window id = 256
  g_windowManager.Add(new CGUIDialogBoxeeVideoCtx);         // window id = 354

#ifdef HAS_DVB
  g_windowManager.Add(new CGUIWindowBoxeeLiveTv);
  g_windowManager.Add(new CGUIDialogBoxeeLiveTvCtx);
  g_windowManager.Add(new CGUIDialogBoxeeLiveTvEditChannels);
  g_windowManager.Add(new CGUIDialogBoxeeLiveTvInfo);
  g_windowManager.Add(new CGUIDialogBoxeeLiveTvScan);
#endif

  g_windowManager.Add(new CGUIDialogBoxeeMusicCtx);         // window id = 355
  g_windowManager.Add(new CGUIDialogBoxeePictureCtx);         // window id = 356
  g_windowManager.Add(new CGUIDialogBoxeeAppCtx);         // window id = 357
  g_windowManager.Add(new CGUIDialogBoxeeBrowserCtx);         // window id = 362
  //end Boxee

  g_windowManager.Add(new CGUIDialogTeletext);               // window id =
  g_windowManager.Add(new CGUIWindowSettingsScreenCalibration); // window id = 11
  g_windowManager.Add(new CGUIWindowSettingsCategory);         // window id = 12 slideshow:window id 2007
#ifndef _BOXEE_
  g_windowManager.Add(new CGUIWindowScripts);                  // window id = 20
  g_windowManager.Add(new CGUIWindowVideoNav);                 // window id = 36
  g_windowManager.Add(new CGUIWindowVideoPlaylist);            // window id = 28
#endif

  g_windowManager.Add(new CGUIWindowLoginScreen);            // window id = 29
  g_windowManager.Add(new CGUIWindowSettingsProfile);          // window id = 34
  g_windowManager.Add(new CGUIDialogYesNo);              // window id = 100
  g_windowManager.Add(new CGUIDialogProgress);           // window id = 101
  g_windowManager.Add(new CGUIDialogKeyboard);           // window id = 103
  g_windowManager.Add(&m_guiDialogVolumeBar);          // window id = 104
  g_windowManager.Add(&m_guiDialogSeekBar);            // window id = 115
  g_windowManager.Add(new CGUIDialogSubMenu);            // window id = 105
  g_windowManager.Add(new CGUIDialogContextMenu);        // window id = 106
  g_windowManager.Add(&m_guiDialogKaiToast);           // window id = 107
  g_windowManager.Add(new CGUIDialogNumeric);            // window id = 109
  g_windowManager.Add(new CGUIDialogGamepad);            // window id = 110
  g_windowManager.Add(new CGUIDialogButtonMenu);         // window id = 111
  g_windowManager.Add(new CGUIDialogMusicScan);          // window id = 112
  g_windowManager.Add(new CGUIDialogPlayerControls);     // window id = 113
#ifdef HAS_KARAOKE
  g_windowManager.Add(new CGUIDialogKaraokeSongSelectorSmall); // window id 143
  g_windowManager.Add(new CGUIDialogKaraokeSongSelectorLarge); // window id 144
#endif
  g_windowManager.Add(new CGUIDialogSlider);             // window id = 145
  g_windowManager.Add(new CGUIDialogMusicOSD);           // window id = 120
  g_windowManager.Add(new CGUIDialogVisualisationSettings);     // window id = 121
  g_windowManager.Add(new CGUIDialogVisualisationPresetList);   // window id = 122
  g_windowManager.Add(new CGUIDialogVideoSettings);             // window id = 123
  g_windowManager.Add(new CGUIDialogAudioSubtitleSettings);     // window id = 124
  g_windowManager.Add(new CGUIDialogBoxeeBrowseSubtitleSettings);     // window id =
  g_windowManager.Add(new CGUIDialogBoxeeBrowseLocalSubtitleSettings);     // window id =
  g_windowManager.Add(new CGUIDialogSubtitleSettings);     // window id = 146
  g_windowManager.Add(new CGUIDialogVideoBookmarks);      // window id = 125
  // Don't add the filebrowser dialog - it's created and added when it's needed
  g_windowManager.Add(new CGUIDialogNetworkSetup);  // window id = 128
  g_windowManager.Add(new CGUIDialogMediaSource);   // window id = 129
  g_windowManager.Add(new CGUIDialogProfileSettings); // window id = 130
  g_windowManager.Add(new CGUIDialogVideoScan);      // window id = 133
  g_windowManager.Add(new CGUIDialogFavourites);     // window id = 134
  g_windowManager.Add(new CGUIDialogSongInfo);       // window id = 135
  g_windowManager.Add(new CGUIDialogSmartPlaylistEditor);       // window id = 136
  g_windowManager.Add(new CGUIDialogSmartPlaylistRule);       // window id = 137
  g_windowManager.Add(new CGUIDialogBusy);      // window id = 138
  g_windowManager.Add(new CGUIDialogPictureInfo);      // window id = 139
  g_windowManager.Add(new CGUIDialogPluginSettings);      // window id = 140
#ifdef HAS_BOXEE_HAL
  g_windowManager.Add(new CGUIDialogAccessPoints);      // window id = 141
  g_windowManager.Add(new CGUIDialogWirelessAuthentication); // window id = 420
#endif

  g_windowManager.Add(new CGUIDialogLockSettings); // window id = 131

  g_windowManager.Add(new CGUIDialogContentSettings);        // window id = 132

  //Boxee

  g_windowManager.Add(new CGUIDialogBoxeeLoginWizardChooseUserToAddType);
  g_windowManager.Add(new CGUIDialogBoxeeLoginWizardAddExistingUser);
  g_windowManager.Add(new CGUIDialogBoxeeLoginWizardAddNewUser);
  g_windowManager.Add(new CGUIDialogBoxeeLoginWizardNewUserDetails);
  g_windowManager.Add(new CGUIDialogBoxeeLoginWizardTOU);
  g_windowManager.Add(new CGUIDialogBoxeeLoginWizardConnectSocialServices);
  g_windowManager.Add(new CGUIDialogBoxeeLoginEditExistingUser);
  g_windowManager.Add(new CGUIDialogBoxeeLoginWizardMenuCust);
  g_windowManager.Add(new CGUIDialogBoxeeLoginWizardConfirmation);
  g_windowManager.Add(new CGUIDialogBoxeeLoginWizardQuickTip);
  g_windowManager.Add(new CGUIDialogBoxeeLoginWizardQuickTipAirPlay);
  g_windowManager.Add(new CGUIDialogBoxeeShare);       // window id = 350
  g_windowManager.Add(new CGUIDialogBoxeeRate);              // window id = 351
  //g_windowManager.Add(new CGUIDialogBoxeeUserInfo);          // window id = 352
  //g_windowManager.Add(new CGUIDialogBoxeeFriendsList);       // window id = 353
  g_windowManager.Add(new CGUIDialogBoxeeMediaAction);       // window id = 401
  g_windowManager.Add(new CGUIDialogBoxeeManualResolve);     // window id = 402
  g_windowManager.Add(new CGUIDialogBoxeeManualResolveMovie);     // window id = 412
  g_windowManager.Add(new CGUIDialogBoxeeManualResolveEpisode);     // window id = 413
  g_windowManager.Add(new CGUIDialogBoxeeManualResolveDetails);     // window id = 415
  g_windowManager.Add(new CGUIDialogBoxeeManualResolveAddFiles);     // window id = 416
  g_windowManager.Add(new CGUIDialogBoxeeManualResolvePartAction);     // window id = 417
  g_windowManager.Add(new CGUIDialogBoxeeManualResolveAlbum);         //window id= 420
  g_windowManager.Add(new CGUIDialogBoxeeManualResolveAudio);         //window id = 421 
  g_windowManager.Add(new CGUIDialogBoxeeBrowseMenu);     // window id = 10423
  g_windowManager.Add(new CGUIDialogBoxeeMessageScroll);       //
#ifdef HAS_WIDGETS
  g_windowManager.Add(new CGUIDialogWidgets);                // window id = 340
#endif
  g_windowManager.Add(new CGUIDialogBoxeeUserLogin);         // window id = 358
  g_windowManager.Add(new CGUIDialogBoxeeBuffering);         // window id = 359
  g_windowManager.Add(new CGUIDialogBoxeeChapters);         // window id = 360
  g_windowManager.Add(new CGUIDialogBoxeeExitVideo);         // window id = 361
  g_windowManager.Add(new CGUIDialogBoxeeQuickTip);         // window id = 363
  g_windowManager.Add(new CGUIDialogBoxeeSeekBar);         // window id = 366
  g_windowManager.Add(new CGUIDialogBoxeePair);            // window id = 367
  g_windowManager.Add(new CGUIWindowBoxeeSettingsDevices); // window id = 368
  g_windowManager.Add(new CGUIDialogBoxeeChannelFilter);   // window id = 369
  g_windowManager.Add(new CGUIDialogFirstTimeUseMenuCust);  // window id = 10147
#ifdef XBMC_STANDALONE
#ifdef HAS_XRANDR
  g_windowManager.Add(new CGUIWindowBoxeeWizardResolution);  // window id = 450
#endif
  g_windowManager.Add(new CGUIWindowBoxeeWizardAudio);       // window id = 451
  g_windowManager.Add(new CGUIWindowBoxeeWizardNetwork);       // window id = 452
  g_windowManager.Add(new CGUIWindowBoxeeWizardNetworkManual);       // window id = 453
  g_windowManager.Add(new CGUIWindowBoxeeWizardTimezone);       // window id = 454
  g_windowManager.Add(new CGUIWindowBoxeeWizardAddSource);       // window id = 455
  g_windowManager.Add(new CGUIWindowBoxeeWizardSourceManager);  // window id = 457
  g_windowManager.Add(new CGUIWindowBoxeeWizardSourceName);  // window id = 458
#endif
  g_windowManager.Add(new CGUIWindowBoxeeWizardAddSourceManual); // window id = 456
  g_windowManager.Add(new CGUIDialogBoxeeSortDropdown); // 10364
  g_windowManager.Add(new CGUIDialogBoxeeTechInfo);     // 10365
  g_windowManager.Add(new CGUIWindowBoxeeMediaSources);	// 10460
  g_windowManager.Add(new CGUIWindowBoxeeMediaSourceAddFolder); // 10461
  g_windowManager.Add(new CGUIWindowBoxeeMediaSourceAddShare);  // 10462
  g_windowManager.Add(new CGUIWindowBoxeeMediaSourceInfo);	// 10463
  g_windowManager.Add(new CGUIWindowBoxeeMediaSourceList);  	// 10464
  //g_windowManager.Add(new CGUIWindowBoxeeBrowse);       // window id = 10466
  g_windowManager.Add(new CGUIDialogBoxeeMainMenu);     // window id = 10467
  //g_windowManager.Add(new CGUIDialogBoxeeOptionsMenu);  // window id = 10468, deprecated
  g_windowManager.Add(new CGUIWindowBoxeeMediaInfo);    // window id = 10469
  g_windowManager.Add(new CGUIWindowBoxeeApplicationSettings);    // window id = 10470
  //g_windowManager.Add(new CGUIDialogBoxeeLibraryStatus);    // window id = 10471
  //g_windowManager.Add(new CGUIDialogBoxeeManualSeriesInput);    // window id = 10472
  g_windowManager.Add(new CGUIWindowBoxeeAlbumInfo);    // window id = 10473
  g_windowManager.Add(new CGUIDialogYesNo2);    // window id = 10474
  g_windowManager.Add(new CGUIDialogOK2);    // window id = 10475
  g_windowManager.Add(new CGUIWebDialog); // window id = 10515
  g_windowManager.Add(new CGUIDialogBoxeeUpdateMessage);    // window id = 10403
  g_windowManager.Add(new CGUIDialogBoxeeDropdown);    // window id = 10406
  g_windowManager.Add(new CGUIDialogBoxeePostPlay);       // window id = 10407
  g_windowManager.Add(new CGUIDialogBoxeeSearch);    // window id = 10408
  g_windowManager.Add(new CGUIDialogBoxeeGlobalSearch);    // window id = 10409
  g_windowManager.Add(new CGUIDialogBoxeeUserPassword);      // window id = 10410
  g_windowManager.Add(new CGUIDialogBoxeeUpdateProgress);    // window id = 10404
  g_windowManager.Add(new CGUIDialogBoxeeApplicationAction); // window id = 10476
  g_windowManager.Add(new CGUIDialogBoxeeShortcutAction);    // window id = 10477
  g_windowManager.Add(new CGUIWindowSettingsScreenSimpleCalibration);    // window id = 10478
  g_windowManager.Add(new CGUIDialogBoxeeRssFeedInfo);    // window id = 10405
  g_windowManager.Add(new CGUIWindowBoxeeBrowseLocal);    // window id = 10479
  g_windowManager.Add(new CGUIDialogBoxeeBrowserDropdown); //window id = 10246
  g_windowManager.Add(new CGUIWindowBoxeeBrowseTvShows);  // window id = 10480
  g_windowManager.Add(new CGUIWindowBoxeeBrowseMovies);   // window id = 10481
  g_windowManager.Add(new CGUIWindowBoxeeBrowseApps);     // window id = 10482
  g_windowManager.Add(new CGUIWindowBoxeeBrowseTvEpisodes); // window id = 10483
  g_windowManager.Add(new CGUIWindowBoxeeBrowseAlbums);     // window id = 10484
  g_windowManager.Add(new CGUIWindowBoxeeBrowseTracks);     // window id = 10485
  g_windowManager.Add(new CGUIWindowBoxeeBrowseRepositories);     // window id = 10486
  //g_windowManager.Add(new CGUIWindowBoxeeBrowseAppBox);     // window id = 10487
  g_windowManager.Add(new CGUIWindowBoxeeBrowseProduct);     // window id = 10488
  g_windowManager.Add(new CGUIWindowBoxeeBrowseQueue);     // window id = 10489
  g_windowManager.Add(new CGUIDialogBoxeeWatchLaterGetStarted);     // window id = 10523
  g_windowManager.Add(new CGUIDialogBoxeeMakeBoxeeSocial);     // window id = 10524
  g_windowManager.Add(new CGUIDialogBoxeeGetFacebookExtraCredential);  // window id = 10525
  g_windowManager.Add(new CGUIWindowBoxeeBrowseDiscover);     // window id = 10490
  g_windowManager.Add(new CGUIWindowBoxeeBrowsePhotos);     // window id = 10491
  //g_windowManager.Add(new CGUIWindowBoxeeBrowseShortcuts);     // window id = 10492
  g_windowManager.Add(new CGUIWindowBoxeeBrowseHistory);     // window id = 10493
  g_windowManager.Add(new CGUIWindowBoxeeBrowseSimpleApp);     // window id = 10494
  g_windowManager.Add(new CGUIWindowBoxeeLoginPrompt);     // window id = 10498
  g_windowManager.Add(new CGUIDialogBoxeeNetworkNotification);  // window id = 10499

#ifdef HAS_EMBEDDED
  g_windowManager.Add(new CGUIWindowFirstTimeUseCalibration); //window id = 10436
  g_windowManager.Add(new CGUIWindowFirstTimeUseBackground); //window id = 10438
  g_windowManager.Add(new CGUIDialogFirstTimeUseConfNetwork);     // window id = 10439
  g_windowManager.Add(new CGUIDialogFirstTimeUseLang);     // window id = 10440
  g_windowManager.Add(new CGUIDialogFirstTimeUseWelcome);  // window id = 10441
  g_windowManager.Add(new CGUIDialogFirstTimeUseWireless);  // window id = 10445
  g_windowManager.Add(new CGUIDialogFirstTimeUseConfWirelessPassword);  // window id = 10446
  g_windowManager.Add(new CGUIDialogFirstTimeUseConfWirelessSecurity);  // window id = 10447
  g_windowManager.Add(new CGUIDialogFirstTimeUseConfWirelessSSID);  // window id = 10448
  g_windowManager.Add(new CGUIDialogFirstTimeUseNetworkMessage);  // window id = 10442
  g_windowManager.Add(new CGUIDialogFirstTimeUseUpdateMessage);  // window id = 10443
  g_windowManager.Add(new CGUIDialogFirstTimeUseUpdateProgress);  // window id = 10444
  g_windowManager.Add(new CGUIDialogFirstTimeUseSimpleMessage);  // window id = 10442
#endif

#ifdef HAS_DVB
  g_windowManager.Add(new CGUIDialogBoxeeOTAConnectionConfiguration);
  g_windowManager.Add(new CGUIDialogBoxeeOTAWelcome); // window id = 10506
  g_windowManager.Add(new CGUIDialogBoxeeOTAConfirmLocation); // window id = 10507
  g_windowManager.Add(new CGUIDialogBoxeeOTACountriesLocationConfiguration); // window id = 10510
  g_windowManager.Add(new CGUIDialogBoxeeOTAZipcodeLocationConfiguration); // window id = 10513
  g_windowManager.Add(new CGUIDialogBoxeeOTAFacebookConnect); // window id = 10515
  g_windowManager.Add(new CGUIDialogBoxeeOTANoChannels); // window id = 10515
#endif

  g_windowManager.Add(new CGUIWindowTestBadPixelsManager);
#ifdef HAS_GL
  g_windowManager.Add(new CGUIWindowTestBadPixelsGL);
#endif
#ifdef HAS_GLES
  g_windowManager.Add(new CGUIWindowTestBadPixelsGLES);
#endif
#ifdef HAS_DX
  g_windowManager.Add(new CGUIWindowTestBadPixelsDX);
#endif
  g_windowManager.Add(new CGUIDialogBoxeePaymentProducts); // window id = 10430
  g_windowManager.Add(new CGUIDialogBoxeePaymentTou); // window id = 10431
  g_windowManager.Add(new CGUIDialogBoxeePaymentUserData); // window id = 10432
  g_windowManager.Add(new CGUIDialogBoxeePaymentOkPlay); // window id = 10434
  g_windowManager.Add(new CGUIDialogBoxeePaymentWaitForServerApproval); // window id = 10435

  //end Boxee
#ifndef _BOXEE_
  g_windowManager.Add(new CGUIWindowMusicPlayList);          // window id = 500
  g_windowManager.Add(new CGUIWindowMusicSongs);             // window id = 501
  g_windowManager.Add(new CGUIWindowMusicNav);               // window id = 502
  g_windowManager.Add(new CGUIWindowMusicPlaylistEditor);    // window id = 503
  g_windowManager.Add(new CGUIWindowVideoInfo);                // window id = 2003
#endif
  g_windowManager.Add(new CGUIDialogBoxeeEject);             // window id = 10522
  g_windowManager.Add(new CGUIDialogSelect);             // window id = 2000
  g_windowManager.Add(new CGUIWindowMusicInfo);                // window id = 2001
  g_windowManager.Add(new CGUIDialogOK);                 // window id = 2002
  g_windowManager.Add(new CGUIWindowScriptsInfo);              // window id = 2004
  g_windowManager.Add(new CGUIWindowFullScreen);         // window id = 2005
  g_windowManager.Add(new CGUIWindowVisualisation);      // window id = 2006
  g_windowManager.Add(new CGUIWindowSlideShow);          // window id = 2007
  g_windowManager.Add(new CGUIDialogFileStacking);       // window id = 2008
#ifdef HAS_KARAOKE
  g_windowManager.Add(new CGUIWindowKaraokeLyrics);      // window id = 2009
#endif
  g_windowManager.Add(new CGUIWindowOSD);                // window id = 2901
  g_windowManager.Add(new CGUIWindowMusicOverlay);       // window id = 2903
  g_windowManager.Add(new CGUIWindowVideoOverlay);       // window id = 2904
  g_windowManager.Add(new CGUIWindowScreensaver);        // window id = 2900 Screensaver
  g_windowManager.Add(new CGUIWindowStartup);            // startup window (id 2999)
  g_windowManager.Add(new CGUIDialogBoxeeVideoQuality);  // window id  = 10418
  g_windowManager.Add(new CGUIDialogBoxeeSelectionList);  // window id  = 10418
  g_windowManager.Add(new CGUIDialogBoxeeVideoResume);  // window id  = 10424
  /* window id's 3000 - 3100 are reserved for python */

  SAFE_DELETE(m_splash);

  if (g_guiSettings.GetBool("masterlock.startuplock") &&
      g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE &&
     !g_settings.m_vecProfiles[0].getLockCode().IsEmpty())
  {
     g_passwordManager.CheckStartUpLock();
  }

#if defined(_WIN32) && defined(HAS_DX)
  if(g_Windowing.GetTextureMemorySize() < 128)
  {
    CGUIDialogOK2::ShowAndGetInput(54093, 54110);
  }

#endif

  // check if we should use the login screen
  
  bool loginWasDone = false;

// Boxee
  m_keyboards.Load();

#ifdef _WIN32
  CStdString flashPath;
  CUtil::AddFileToFolder(CWIN32Util::GetSystemPath(), "Macromed\\Flash\\NPSWF32.dll", flashPath);
  if (!CFile::Exists(flashPath))
  {
	  CGUIDialogOK2::ShowAndGetInput(54108, 54109);
  }
#endif

#ifdef HAS_BOXEE_HAL
  IHalServices& hal = CHalServicesFactory::GetInstance();
  hal.AddListener(&g_halListener);
#endif

#ifdef HAS_EMBEDDED
  // check if need to run FTU

  if (m_bEnableFTU && !g_stSettings.m_doneFTU)
  {
    if (CInitializeBoxManager::GetInstance().Run())
    {
      g_stSettings.m_doneFTU = true;
      g_settings.Save();
    }
    else
    {
      CLog::Log(LOGERROR,"CApplication::Initialize - FAILED in running FTU");
    }
  }
#endif

  if (g_settings.bUseLoginScreen)
  {
    bool needToDoFTU2 = false;

#ifdef HAS_EMBEDDED

    //////////////////////////////////////////////////
    // check if need to run FTU2 before LoginScreen //
    //////////////////////////////////////////////////

    needToDoFTU2 = !g_stSettings.m_doneFTU2;
#endif

    if (needToDoFTU2)
    {
      g_windowManager.ActivateWindow(WINDOW_FTU_CALIBRATION);
    }
    else
    {
      loginWasDone = m_BoxeeLoginManager.Login();
    }
  }
  else
  {
    RESOLUTION res = RES_INVALID;
    CStdString startupPath = g_SkinInfo.GetSkinPath("Startup.xml", &res);
    int startWindow = g_guiSettings.GetInt("lookandfeel.startupwindow");
    // test for a startup window, and activate that instead of home
    if (CFile::Exists(startupPath) && (!g_SkinInfo.OnlyAnimateToHome() || startWindow == WINDOW_HOME))
    {
      g_windowManager.ActivateWindow(WINDOW_STARTUP);
    }
    else
    {
      // We need to Popup the WindowHome to initiate the GUIWindowManger for MasterCode popup dialog!
      // Then we can start the StartUpWindow! To prevent BlackScreen if the target Window is Protected with MasterCode!
      g_windowManager.ActivateWindow(WINDOW_HOME);
      if (startWindow != WINDOW_HOME)
        g_windowManager.ActivateWindow(startWindow);
    }

    PostLoginInitializations();
  }

  return S_OK;
}

void CApplication::PostLoginInitializations()
{
// Boxee

  CLog::Log(LOGDEBUG,"CApplication::PostLoginInitializations - Enter function (login)");

  XFILE::CFileCurl::SetCookieJar(BOXEE::BXCurl::GetCookieJar());
#ifdef _LINUX
  setenv("BOXEE_COOKIEJAR", BOXEE::BXCurl::GetCookieJar().c_str(), 1);
#else
  SetEnvironmentVariable("BOXEE_COOKIEJAR", BOXEE::BXCurl::GetCookieJar().c_str());
#endif

  // Tricky - create the browser service, but don't start the background scanner until
  // we've done the rest of our post login work. This allows us to flag user shares for
  // scanning prior to starting the scanner
  if (!m_pBrowserService)
  {
    m_pBrowserService = new CBrowserService();
  }
  
  BoxeePostLoginInitializations();

  m_pBrowserService->Init();

  BOXEE::Boxee::GetInstance().SetHttpCacheManager(&m_httpCache);

// end Boxee
  
#ifdef HAS_PYTHON
  g_pythonParser.m_bStartup = true;
#endif
  g_sysinfo.Refresh();

  CLog::Log(LOGINFO, "removing tempfiles");
  CUtil::RemoveTempFiles();

  if (!m_bAllSettingsLoaded)
  {
    CLog::Log(LOGWARNING, "settings not correct, show dialog");
    CStdString test;
    CUtil::GetHomePath(test);
    CGUIDialogOK *dialog = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    if (dialog)
    {
      dialog->SetHeading(279);
      dialog->SetLine(0, "Error while loading settings");
      dialog->SetLine(1, test);
      dialog->SetLine(2, "");;
      dialog->DoModal();
    }
  }

  //  Show mute symbol
  if (g_stSettings.m_nVolumeLevel == VOLUME_MINIMUM)
    Mute();

  // if the user shutoff the xbox during music scan
  // restore the settings
  if (g_stSettings.m_bMyMusicIsScanning)
  {
    CLog::Log(LOGWARNING,"System rebooted during music scan! ... restoring UseTags and FindRemoteThumbs");
    RestoreMusicScanSettings();
  }

  if (!g_settings.bUseLoginScreen)
    UpdateLibraries();

#ifdef HAS_HAL
  g_HalManager.Initialize();
#endif

  m_slowTimer.StartZero();

#ifdef __APPLE__
  g_xbmcHelper.CaptureAllInput();
#endif

  g_powerManager.Initialize();

  CLog::Log(LOGNOTICE, "initialize done");

  m_bInitializing = false;

  // reset our screensaver (starts timers etc.)
  ResetScreenSaver();

  BOXEE::BXMediaDatabase media_db;
  VECSOURCES* pVecShares = g_settings.GetAllMediaSources(true);
  CMediaSource* pShare = NULL;
  for (IVECSOURCES it = pVecShares->begin(); !m_bStop && it != pVecShares->end() && !m_bStop; it++)
  {
    pShare = &(*it);
    media_db.AddMediaShare(pShare->strName, pShare->strPath, pShare->m_type, pShare->m_iScanType);
  }

  CBoxeeBrowseMenuManager::GetInstance().Init();

#ifdef HAS_DVB
  DVBManager::GetInstance().Start();
#endif
}

void CApplication::StartWebServer()
{
#ifdef HAS_WEB_SERVER
  if (g_guiSettings.GetBool("servers.webserver") && m_network.IsAvailable())
  {
    int webPort = atoi(g_guiSettings.GetString("servers.webserverport"));
    CLog::Log(LOGNOTICE, "Webserver: Starting...");
#ifdef _LINUX
    if (webPort < 1024 && geteuid() != 0)
    {
        CLog::Log(LOGERROR, "Cannot start Web Server as port is smaller than 1024 and user is not root");
        return;
    }
#endif
    CSectionLoader::Load("LIBHTTP");
    if (m_network.GetFirstConnectedInterface())
    {
       m_pWebServer = new CWebServer();
       m_pWebServer->Start(m_network.GetFirstConnectedInterface()->GetCurrentIPAddress().c_str(), webPort, "special://xbmc/web", false);
       
       // Set WebServer port for discovery
       BOXEE::Boxee::GetInstance().SetWebServerPort(g_guiSettings.GetString("servers.webserverport"));
    }
    if (m_pWebServer)
    {
      m_pWebServer->SetUserName(g_guiSettings.GetString("servers.webserverusername").c_str());
      m_pWebServer->SetPassword(g_guiSettings.GetString("servers.webserverpassword").c_str());
     
#ifdef HAS_ZEROCONF 
      std::map<std::string, std::string> txt;
      // publish web frontend and API services
      CZeroconf::GetInstance()->PublishService("servers.webserver", "_http._tcp", "Boxee Web Server", webPort, txt);
      CZeroconf::GetInstance()->PublishService("servers.webapi", "_xbmc-web._tcp", "Boxee HTTP API", webPort, txt);
#endif
    }
    if (m_pWebServer && m_pXbmcHttp && g_stSettings.m_HttpApiBroadcastLevel>=1)
      getApplicationMessenger().HttpApi("broadcastlevel; StartUp;1");
  }
#endif
}

void CApplication::StopWebServer(bool bWait)
{
#ifdef HAS_WEB_SERVER
  if (m_pWebServer)
  {
    if (!bWait)
    {
      CLog::Log(LOGNOTICE, "Webserver: Stopping...");
      m_pWebServer->Stop(false);
    }
    else
    {
      m_pWebServer->Stop(true);
      delete m_pWebServer;
      m_pWebServer = NULL;
      CSectionLoader::Unload("LIBHTTP");
      CLog::Log(LOGNOTICE, "Webserver: Stopped...");
      CZeroconf::GetInstance()->RemoveService("servers.webserver");
      CZeroconf::GetInstance()->RemoveService("servers.webapi");
    }
  }
#endif
}

void CApplication::StartAirplayServer()
{
#ifdef HAS_AIRPLAY
  if (g_guiSettings.GetBool("airplay2.enable") && m_network.IsAvailable())
  {
    if (!CAirPlayServer::StartServer(9091, true))
    {
      CLog::Log(LOGERROR, "Failed to start AirPlay Server");
    }
  }
#endif

#ifdef HAS_AIRTUNES
  if (g_guiSettings.GetBool("airplay2.enable") && m_network.IsAvailable())
  {
    if (!CAirTunesServer::StartServer(5000, true))
    {
      CLog::Log(LOGERROR, "Failed to start AirTunes Server");
    }
  }
#endif
}

void CApplication::StopAirplayServer(bool bWait)
{
#ifdef HAS_AIRPLAY
  CAirPlayServer::StopServer(bWait);
#endif

#ifdef HAS_AIRTUNES
  CAirTunesServer::StopServer(bWait);
#endif
}

void CApplication::StartJSONRPCServer()
{
#ifdef HAS_JSONRPC
  CJSONRPC::Initialize();

  if (CTCPServer::StartServer(9090, true))
  {
    std::map<std::string, std::string> txt;
    CZeroconf::GetInstance()->PublishService("servers.jsonrpc", "_boxee-jsonrpc._tcp", "Boxee", 9090, txt);
  }
#endif
}

void CApplication::StopJSONRPCServer(bool bWait)
{
#ifdef HAS_JSONRPC
  CTCPServer::StopServer(bWait);
  CZeroconf::GetInstance()->RemoveService("servers.jsonrpc");
#endif
}

void CApplication::StartFtpServer()
{
#ifdef HAS_FTP_SERVER
  if ( g_guiSettings.GetBool("servers.ftpserver") && m_network.IsAvailable() )
  {
    CLog::Log(LOGNOTICE, "XBFileZilla: Starting...");
    if (!m_pFileZilla)
    {
      CStdString xmlpath = "special://xbmc/system/";
      // if user didn't upgrade properly,
      // check whether UserData/FileZilla Server.xml exists
      if (CFile::Exists(g_settings.GetUserDataItem("FileZilla Server.xml")))
        xmlpath = g_settings.GetUserDataFolder();

      // check file size and presence
      CFile xml;
      if (xml.Open(xmlpath+"FileZilla Server.xml") && xml.GetLength() > 0)
      {
        m_pFileZilla = new CXBFileZilla(_P(xmlpath));
        m_pFileZilla->Start(false);
      }
      else
      {
        // 'FileZilla Server.xml' does not exist or is corrupt,
        // falling back to ftp emergency recovery mode
        CLog::Log(LOGNOTICE, "XBFileZilla: 'FileZilla Server.xml' is missing or is corrupt!");
        CLog::Log(LOGNOTICE, "XBFileZilla: Starting ftp emergency recovery mode");
        StartFtpEmergencyRecoveryMode();
      }
      xml.Close();
    }
  }
#endif
}

void CApplication::StopFtpServer()
{
#ifdef HAS_FTP_SERVER
  if (m_pFileZilla)
  {
    CLog::Log(LOGINFO, "XBFileZilla: Stopping...");

    std::vector<SXFConnection> mConnections;
    std::vector<SXFConnection>::iterator it;

    m_pFileZilla->GetAllConnections(mConnections);

    for(it = mConnections.begin();it != mConnections.end();it++)
    {
      m_pFileZilla->CloseConnection(it->mId);
    }

    m_pFileZilla->Stop();
    delete m_pFileZilla;
    m_pFileZilla = NULL;

    CLog::Log(LOGINFO, "XBFileZilla: Stopped");
  }
#endif
}

void CApplication::StartTimeServer()
{
#ifdef HAS_TIME_SERVER
  if (g_guiSettings.GetBool("locale.timeserver") && m_network.IsAvailable() )
  {
    if( !m_psntpClient )
    {
      CSectionLoader::Load("SNTP");
      CLog::Log(LOGNOTICE, "start timeserver client");

      m_psntpClient = new CSNTPClient();
      m_psntpClient->Update();
    }
  }
#endif
}

void CApplication::StopTimeServer()
{
#ifdef HAS_TIME_SERVER
  if( m_psntpClient )
  {
    CLog::Log(LOGNOTICE, "stop time server client");
    SAFE_DELETE(m_psntpClient);
    CSectionLoader::Unload("SNTP");
  }
#endif
}

void CApplication::StartUPnP()
{
#ifdef HAS_UPNP
    //StartUPnPClient();
    StartUPnPServer();
    StartUPnPRenderer();
#endif
}

void CApplication::StopUPnP(bool bWait)
{
#ifdef HAS_UPNP
  if (CUPnP::IsInstantiated())
  {
    CLog::Log(LOGNOTICE, "stopping upnp");
    CUPnP::ReleaseInstance(bWait);
  }
#endif
}

void CApplication::StartEventServer()
{
#ifdef HAS_EVENT_SERVER
  CEventServer* server = CEventServer::GetInstance();
  if (!server)
  {
    CLog::Log(LOGERROR, "ES: Out of memory");
    return;
  }
  if (g_guiSettings.GetBool("remoteevents.enabled"))
  {
    CLog::Log(LOGNOTICE, "ES: Starting event server");
    server->StartServer();
  }
#endif
}

bool CApplication::StopEventServer(bool bWait, bool promptuser)
{
#ifdef HAS_EVENT_SERVER
  CEventServer* server = CEventServer::GetInstance();
  if (!server)
  {
    CLog::Log(LOGERROR, "ES: Out of memory");
    return false;
  }
  if (promptuser)
  {
    if (server->GetNumberOfClients() > 0)
    {
      bool cancelled = false;
      if (!CGUIDialogYesNo::ShowAndGetInput(13140, 13141, 13142, 20022,
                                            -1, -1, cancelled, 10000)
          || cancelled)
      {
        CLog::Log(LOGNOTICE, "ES: Not stopping event server");
        return false;
      }
    }
    CLog::Log(LOGNOTICE, "ES: Stopping event server with confirmation");
    
    CEventServer::GetInstance()->StopServer(true);
  }
  else
  {
    if (!bWait)
    CLog::Log(LOGNOTICE, "ES: Stopping event server");
    
    CEventServer::GetInstance()->StopServer(bWait);
  }
 #endif
  return true;
}

void CApplication::RefreshEventServer()
{
#ifdef HAS_EVENT_SERVER
  if (g_guiSettings.GetBool("remoteevents.enabled"))
  {
    CEventServer::GetInstance()->RefreshSettings();
  }
#endif
}

void CApplication::StartDbusServer()
{
#ifdef HAS_DBUS_SERVER
  CDbusServer* serverDbus = CDbusServer::GetInstance();
  if (!serverDbus)
  {
    CLog::Log(LOGERROR, "DS: Out of memory");
    return;
  }
  CLog::Log(LOGNOTICE, "DS: Starting dbus server");
  serverDbus->StartServer( this );
#endif
}

bool CApplication::StopDbusServer(bool bWait)
{
#ifdef HAS_DBUS_SERVER
  CDbusServer* serverDbus = CDbusServer::GetInstance();
  if (!serverDbus)
  {
    CLog::Log(LOGERROR, "DS: Out of memory");
    return false;
  }
  CDbusServer::GetInstance()->StopServer(bWait);
#endif
  return true;
}

void CApplication::StartUPnPRenderer()
{
#ifdef HAS_UPNP
  if (g_guiSettings.GetBool("upnp.renderer"))
  {
    CLog::Log(LOGNOTICE, "starting upnp renderer");
    CUPnP::GetInstance()->StartRenderer();
  }
#endif
}

void CApplication::StopUPnPRenderer()
{
#ifdef HAS_UPNP
  if (CUPnP::IsInstantiated())
  {
    CLog::Log(LOGNOTICE, "stopping upnp renderer");
    CUPnP::GetInstance()->StopRenderer();
  }
#endif
}

void CApplication::StartUPnPClient()
{
#ifdef HAS_UPNP
  if (g_guiSettings.GetBool("upnp.client"))
  {
    CLog::Log(LOGNOTICE, "starting upnp client");
    CUPnP::GetInstance()->StartClient();
  }
#endif
}

void CApplication::StopUPnPClient()
{
#ifdef HAS_UPNP
  if (CUPnP::IsInstantiated())
  {
    CLog::Log(LOGNOTICE, "stopping upnp client");
    CUPnP::GetInstance()->StopClient();
  }
#endif
}

void CApplication::StartUPnPServer()
{
#ifdef HAS_UPNP
  if (g_guiSettings.GetBool("upnp.server"))
  {
    CLog::Log(LOGNOTICE, "starting upnp server");
    CUPnP::GetInstance()->StartServer();
  }
#endif
}

void CApplication::StopUPnPServer()
{
#ifdef HAS_UPNP
  if (CUPnP::IsInstantiated())
  {
    CLog::Log(LOGNOTICE, "stopping upnp server");
    CUPnP::GetInstance()->StopServer();
  }
#endif
}

void CApplication::StartSmbServer()
{
#ifdef HAS_EMBEDDED
  bool isSmbServerEnable = g_guiSettings.GetBool("smbd.enable");
  CLog::Log(LOGNOTICE,"CApplication::StartSmbServer - Enter function. [isSmbServerEnable=%d] (smbd)",isSmbServerEnable);

  if (isSmbServerEnable)
  {
    CStdString username = "guest";
    CStdString password = g_guiSettings.GetString("smbd.password");
    CStdString workgroup = g_guiSettings.GetString("smbd.workgroup");
    CStdString hostname = g_guiSettings.GetString("server.hostname");
    if (workgroup.IsEmpty())
    {
      workgroup = "WORKGROUP";
    }

    CLog::Log(LOGNOTICE,"CApplication::StartSmbServer - starting SMB server with [username=%s][password=%s][workgroup=%s] (smbd)",username.c_str(),password.c_str(),workgroup.c_str());

    if (!CHalServicesFactory::GetInstance().EnableSambaShares(password,username,workgroup, hostname))
    {
      CLog::Log(LOGERROR,"CApplication::StartSmbServer - FAILED to start SMB server with [username=%s][password=%s][workgroup=%s] (smbd)",username.c_str(),password.c_str(),workgroup.c_str());
    }
  }
#endif
}

void CApplication::SetHostname()
{
#ifdef HAS_EMBEDDED
  CStdString hostname = g_guiSettings.GetString("server.hostname");
  CLog::Log(LOGNOTICE,"CApplication::SetHostname - Enter function. [hostname=%s] (smbd)",hostname.c_str());
  if (!CHalServicesFactory::GetInstance().SetHostName(hostname))
  {
    CLog::Log(LOGERROR,"CApplication::SetHostname - FAILED to set hostname with [hostname=%s] (smbd)", hostname.c_str());
  }
#endif
}

void CApplication::StopSmbServer()
{
#ifdef HAS_EMBEDDED

  CLog::Log(LOGNOTICE,"CApplication::StopSmbServer - stopping SMB server (smbd)");

  if (!CHalServicesFactory::GetInstance().DisableSambaShares())
  {
    CLog::Log(LOGERROR,"CApplication::StopSmbServer - FAILED to stop SMB server (smbd)");
  }
#endif
}

void CApplication::StartZeroconf()
{
#ifdef HAS_ZEROCONF
  //entry in guisetting only present if HAS_ZEROCONF is set
  if(g_guiSettings.GetBool("servers.zeroconf"))
  {
    CLog::Log(LOGNOTICE, "starting zeroconf publishing");
    CZeroconf::GetInstance()->Start();
  }
#endif
}

void CApplication::StopZeroconf()
{
#ifdef HAS_ZEROCONF
  if(CZeroconf::IsInstantiated())
  {
  CLog::Log(LOGNOTICE, "stopping zeroconf publishing");
  CZeroconf::GetInstance()->Stop();
  }
#endif
}

void CApplication::DimLCDOnPlayback(bool dim)
{
#ifdef HAS_LCD
  if(g_lcd && dim && (g_guiSettings.GetInt("lcd.disableonplayback") != LED_PLAYBACK_OFF) && (g_guiSettings.GetInt("lcd.type") != LCD_TYPE_NONE))
  {
    if ( (IsPlayingVideo()) && g_guiSettings.GetInt("lcd.disableonplayback") == LED_PLAYBACK_VIDEO)
      g_lcd->SetBackLight(0);
    if ( (IsPlayingAudio()) && g_guiSettings.GetInt("lcd.disableonplayback") == LED_PLAYBACK_MUSIC)
      g_lcd->SetBackLight(0);
    if ( ((IsPlayingVideo() || IsPlayingAudio())) && g_guiSettings.GetInt("lcd.disableonplayback") == LED_PLAYBACK_VIDEO_MUSIC)
      g_lcd->SetBackLight(0);
  }
  else if(!dim)
#ifdef _LINUX
    g_lcd->SetBackLight(1);
#else
    g_lcd->SetBackLight(g_guiSettings.GetInt("lcd.backlight"));
#endif
#endif
}

void CApplication::StartServices()
{
#if !defined(_WIN32) && defined(HAS_DVD_DRIVE)
  // Start Thread for DVD Mediatype detection
  CLog::Log(LOGNOTICE, "start dvd mediatype detection");
  m_DetectDVDType.Create(false, THREAD_MINSTACKSIZE);
#endif

  CLog::Log(LOGNOTICE, "initializing playlistplayer");
  g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC, g_stSettings.m_bMyMusicPlaylistRepeat ? PLAYLIST::REPEAT_ALL : PLAYLIST::REPEAT_NONE);
  g_playlistPlayer.SetShuffle(PLAYLIST_MUSIC, g_stSettings.m_bMyMusicPlaylistShuffle);
  g_playlistPlayer.SetRepeat(PLAYLIST_VIDEO, g_stSettings.m_bMyVideoPlaylistRepeat ? PLAYLIST::REPEAT_ALL : PLAYLIST::REPEAT_NONE);
  g_playlistPlayer.SetShuffle(PLAYLIST_VIDEO, g_stSettings.m_bMyVideoPlaylistShuffle);
  CLog::Log(LOGNOTICE, "DONE initializing playlistplayer");

#ifdef HAS_LCD
  CLCDFactory factory;
  g_lcd = factory.Create();
  if (g_lcd)
  {
    g_lcd->Initialize();
  }
#endif
  }

void CApplication::StopServices()
{
  m_network.NetworkMessage(CNetwork::SERVICES_DOWN, 0);

#if !defined(_WIN32) && defined(HAS_DVD_DRIVE)
  CLog::Log(LOGNOTICE, "stop dvd detect media");
  m_DetectDVDType.StopThread();
#endif
}

void CApplication::DelayLoadSkin()
{
  m_skinReloadTime = CTimeUtils::GetFrameTime() + 2000;
  return ;
}

void CApplication::CancelDelayLoadSkin()
{
  m_skinReloadTime = 0;
}

void CApplication::ReloadSkin()
{
  CGUIMessage msg(GUI_MSG_LOAD_SKIN, -1, g_windowManager.GetActiveWindow());
  g_windowManager.SendMessage(msg);
  // Reload the skin, restoring the previously focused control.  We need this as
  // the window unload will reset all control states.
  CGUIWindow* pWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
  unsigned int iCtrlID = pWindow?pWindow->GetFocusedControlID():0;
  g_application.LoadSkin(g_guiSettings.GetString("lookandfeel.skin"));
  pWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
  if (pWindow && pWindow->HasSaveLastControl())
  {
    CGUIMessage msg3(GUI_MSG_SETFOCUS, g_windowManager.GetActiveWindow(), iCtrlID, 0);
    pWindow->OnMessage(msg3);
  }
}

void CApplication::TogglePlayPause()
{
  m_pPlayer->Pause();
#ifdef HAS_KARAOKE
  m_pKaraokeMgr->SetPaused( m_pPlayer->IsPaused() );
#endif
  if (!m_pPlayer->IsPaused())
  {
    GetItemLoader().Pause(true);
    // Pause the media library
    BOXEE::Boxee::GetInstance().GetMetadataEngine().Pause();
    GetItemLoader().Pause();
    // unpaused - set the playspeed back to normal
    SetPlaySpeed(1);
#ifdef HAS_EMBEDDED
    //show the video again
    if(m_pPlayer->HasVideo())
      SwitchToFullScreen();
#endif
  }
  else
  {
    GetItemLoader().Resume();
    GetItemLoader().Resume(true);
    BOXEE::Boxee::GetInstance().GetMetadataEngine().Resume();
  }
#ifdef HAS_EMBEDDED
  g_audioManager.Enable(false);
#else
  g_audioManager.Enable(m_pPlayer->IsPaused());
#endif
}

void CApplication::LoadSkin(const CStdString& strSkin)
{
  bool bPreviousPlayingState=false;
  bool bPreviousRenderingState=false;
  if (g_application.m_pPlayer && g_application.IsPlayingVideo())
  {
    bPreviousPlayingState = !g_application.m_pPlayer->IsPaused();
    if (bPreviousPlayingState)
      g_application.m_pPlayer->Pause();
#ifdef HAS_VIDEO_PLAYBACK
    if (!g_renderManager.Paused())
    {
      if (g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
     {
        g_windowManager.ActivateWindow(WINDOW_HOME);
        bPreviousRenderingState = true;
      }
    }
#endif
  }
  //stop the busy renderer if it's running before we lock the graphic context or we could deadlock.
  g_ApplicationRenderer.Stop();
  // close the music and video overlays (they're re-opened automatically later)
  CSingleLock lock(g_graphicsContext);

  m_skinReloadTime = 0;

  CStdString strHomePath;
  CStdString strSkinPath = g_settings.GetSkinFolder(strSkin);

  CLog::Log(LOGDEBUG, "  load skin from:%s", strSkinPath.c_str());

  // save the current window details
  int currentWindow = g_windowManager.GetActiveWindow();
  vector<int> currentModelessWindows;
  g_windowManager.GetActiveModelessWindows(currentModelessWindows);

  CLog::Log(LOGDEBUG, "  delete old skin...");
  UnloadSkin();

  // Load in the skin.xml file if it exists
  g_SkinInfo.Load(strSkinPath);

  CLog::Log(LOGDEBUG, "  load fonts for skin...");
  g_graphicsContext.SetMediaDir(strSkinPath);
  g_directoryCache.ClearSubPaths(strSkinPath);
  if (g_langInfo.ForceUnicodeFont() && !g_fontManager.IsFontSetUnicode(g_guiSettings.GetString("lookandfeel.font")))
  {
    CLog::Log(LOGDEBUG, "    language needs a ttf font, loading first ttf font available");
    CStdString strFontSet;
    if (g_fontManager.GetFirstFontSetUnicode(strFontSet))
    {
      CLog::Log(LOGDEBUG, "    new font is '%s'", strFontSet.c_str());
      g_guiSettings.SetString("lookandfeel.font", strFontSet);
      g_settings.Save();
    }
    else
      CLog::Log(LOGERROR, "    no ttf font found, but needed for the language %s.", g_guiSettings.GetString("locale.language").c_str());
  }
  g_colorManager.Load(g_guiSettings.GetString("lookandfeel.skincolors"));

  g_fontManager.LoadFonts(g_guiSettings.GetString("lookandfeel.font"));

  // load in the skin strings
  CStdString skinPath, skinEnglishPath;
  CUtil::AddFileToFolder(strSkinPath, "language", skinPath);
  CUtil::AddFileToFolder(skinPath, g_guiSettings.GetString("locale.language"), skinPath);
  CUtil::AddFileToFolder(skinPath, "strings.xml", skinPath);

  CUtil::AddFileToFolder(strSkinPath, "language", skinEnglishPath);
  CUtil::AddFileToFolder(skinEnglishPath, "English", skinEnglishPath);
  CUtil::AddFileToFolder(skinEnglishPath, "strings.xml", skinEnglishPath);

  g_localizeStrings.LoadSkinStrings(skinPath, skinEnglishPath);

  int64_t start;
  start = CurrentHostCounter();

  CLog::Log(LOGDEBUG, "  load new skin...");
  CGUIWindowHome *pHome = (CGUIWindowHome *)g_windowManager.GetWindow(WINDOW_HOME);
  if (!g_SkinInfo.Check(strSkinPath) || !pHome || !pHome->Load("Home.xml"))
  {
    // failed to load home.xml
    // fallback to default skin
    if ( strcmpi(strSkin.c_str(), DEFAULT_SKIN) != 0)
    {
      CLog::Log(LOGERROR, "failed to load home.xml for skin:%s, fallback to \"%s\" skin", strSkin.c_str(), DEFAULT_SKIN);
      g_guiSettings.SetString("lookandfeel.skin", DEFAULT_SKIN);
      LoadSkin(g_guiSettings.GetString("lookandfeel.skin"));
      return ;
    }
  }

  // Load the user windows
  LoadUserWindows(strSkinPath);

  g_TextureManager.LoadPinnedTextures();

  int64_t end, freq;
  end = CurrentHostCounter();
  freq = CurrentHostFrequency();
  CLog::Log(LOGDEBUG,"Load Skin XML: %.2fms", 1000.f * (end - start) / freq);

  CLog::Log(LOGDEBUG, "  initialize new skin...");
  m_guiPointer.AllocResources(true);
  m_guiDialogVolumeBar.AllocResources(true);
  m_guiDialogSeekBar.AllocResources(true);
  m_guiDialogKaiToast.AllocResources(true);
  m_guiDialogMuteBug.AllocResources(true);

  // Add targets to listen to events.
  g_windowManager.AddMsgTarget(this);
  g_windowManager.AddMsgTarget(&g_fontManager);
  g_windowManager.AddMsgTarget(&g_playlistPlayer);
  g_windowManager.AddMsgTarget(&g_infoManager);
  g_windowManager.SetCallback(*this);
  g_windowManager.Initialize();
  g_audioManager.Initialize(CAudioContext::DEFAULT_DEVICE);
  g_audioManager.Load();

  CGUIDialogFullScreenInfo* pDialog = NULL;
  RESOLUTION res;
  CStdString strPath = g_SkinInfo.GetSkinPath("DialogFullScreenInfo.xml", &res);
  if (CFile::Exists(strPath))
    pDialog = new CGUIDialogFullScreenInfo;
   
  if (pDialog)
    g_windowManager.Add(pDialog); // window id = 142

  CLog::Log(LOGDEBUG, "  skin loaded...");

  // leave the graphics lock
  lock.Leave();
  g_ApplicationRenderer.Start();

  // restore windows
  if ((currentWindow != WINDOW_INVALID) && (currentWindow != WINDOW_LOGIN_SCREEN))
  {
    g_windowManager.ActivateWindow(currentWindow);
    for (unsigned int i = 0; i < currentModelessWindows.size(); i++)
    {
      CGUIDialog *dialog = (CGUIDialog *)g_windowManager.GetWindow(currentModelessWindows[i]);
      if (dialog) dialog->Show();
    }
  }

  if (g_application.m_pPlayer && g_application.IsPlayingVideo())
  {
    if (bPreviousPlayingState)
      g_application.m_pPlayer->Pause();
    if (bPreviousRenderingState)
      g_windowManager.ActivateWindow(WINDOW_FULLSCREEN_VIDEO);
  }
}

void CApplication::UnloadSkin()
{
  g_ApplicationRenderer.Stop();
  g_audioManager.DeInitialize(CAudioContext::DEFAULT_DEVICE);

  g_windowManager.DeInitialize();
  
  //These windows are not handled by the windowmanager (why not?) so we should unload them manually
  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0);
  m_guiPointer.OnMessage(msg);
  m_guiPointer.ResetControlStates();
  m_guiPointer.FreeResources(true);
  m_guiDialogMuteBug.OnMessage(msg);
  m_guiDialogMuteBug.ResetControlStates();
  m_guiDialogMuteBug.FreeResources(true);

  // remove the skin-dependent window
  g_windowManager.Delete(WINDOW_DIALOG_FULLSCREEN_INFO);

  g_TextureManager.Cleanup();

  g_fontManager.Clear();

  g_colorManager.Clear();

  g_charsetConverter.reset();

  g_infoManager.Clear();
}

bool CApplication::LoadUserWindows(const CStdString& strSkinPath)
{
  WIN32_FIND_DATA FindFileData;
  WIN32_FIND_DATA NextFindFileData;
  HANDLE hFind;
  TiXmlDocument xmlDoc;
  RESOLUTION resToUse = RES_INVALID;

  // Start from wherever home.xml is
  g_SkinInfo.GetSkinPath("Home.xml", &resToUse);
  std::vector<CStdString> vecSkinPath;
  if (resToUse == RES_HDTV_1080i)
    vecSkinPath.push_back(CUtil::AddFileToFolder(strSkinPath, g_SkinInfo.GetDirFromRes(RES_HDTV_1080i)));
  if (resToUse == RES_HDTV_720p)
    vecSkinPath.push_back(CUtil::AddFileToFolder(strSkinPath, g_SkinInfo.GetDirFromRes(RES_HDTV_720p)));
  if (resToUse == RES_PAL_16x9 || resToUse == RES_NTSC_16x9 || resToUse == RES_HDTV_480p_16x9 || resToUse == RES_HDTV_720p || resToUse == RES_HDTV_1080i)
    vecSkinPath.push_back(CUtil::AddFileToFolder(strSkinPath, g_SkinInfo.GetDirFromRes(g_SkinInfo.GetDefaultWideResolution())));
  vecSkinPath.push_back(CUtil::AddFileToFolder(strSkinPath, g_SkinInfo.GetDirFromRes(g_SkinInfo.GetDefaultResolution())));
  for (unsigned int i=0;i<vecSkinPath.size();++i)
  {
    CStdString strPath = CUtil::AddFileToFolder(vecSkinPath[i], "custom*.xml");
    CLog::Log(LOGDEBUG, "Loading user windows, path %s", vecSkinPath[i].c_str());
    hFind = FindFirstFile(_P(strPath).c_str(), &NextFindFileData);

    CStdString strFileName;
    while (hFind != INVALID_HANDLE_VALUE)
    {
      FindFileData = NextFindFileData;

      if (!FindNextFile(hFind, &NextFindFileData))
      {
        FindClose(hFind);
        hFind = INVALID_HANDLE_VALUE;
      }

      // skip "up" directories, which come in all queries
      if (!strcmp(FindFileData.cFileName, ".") || !strcmp(FindFileData.cFileName, ".."))
        continue;

      strFileName = CUtil::AddFileToFolder(vecSkinPath[i], FindFileData.cFileName);
      CLog::Log(LOGDEBUG, "Loading skin file: %s", strFileName.c_str());
      CStdString strLower(FindFileData.cFileName);
      strLower.MakeLower();
      strLower = CUtil::AddFileToFolder(vecSkinPath[i], strLower);
      if (!xmlDoc.LoadFile(strFileName) && !xmlDoc.LoadFile(strLower))
      {
        CLog::Log(LOGERROR, "unable to load:%s, Line %d\n%s", strFileName.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
        continue;
      }

      // Root element should be <window>
      TiXmlElement* pRootElement = xmlDoc.RootElement();
      CStdString strValue = pRootElement->Value();
      if (!strValue.Equals("window"))
      {
        CLog::Log(LOGERROR, "file :%s doesnt contain <window>", strFileName.c_str());
        continue;
      }

      // Read the <type> element to get the window type to create
      // If no type is specified, create a CGUIWindow as default
      CGUIWindow* pWindow = NULL;
      CStdString strType;
      if (pRootElement->Attribute("type"))
        strType = pRootElement->Attribute("type");
      else
      {
        const TiXmlNode *pType = pRootElement->FirstChild("type");
        if (pType && pType->FirstChild())
          strType = pType->FirstChild()->Value();
      }
      if (strType.Equals("dialog"))
        pWindow = new CGUIDialog(0, "");
      else if (strType.Equals("submenu"))
        pWindow = new CGUIDialogSubMenu();
      else if (strType.Equals("buttonmenu"))
        pWindow = new CGUIDialogButtonMenu();
      else
        pWindow = new CGUIStandardWindow();

      int id = WINDOW_INVALID;
      if (!pRootElement->Attribute("id", &id))
      {
        const TiXmlNode *pType = pRootElement->FirstChild("id");
        if (pType && pType->FirstChild())
          id = atol(pType->FirstChild()->Value());
      }
      // Check to make sure the pointer isn't still null
      if (pWindow == NULL || id == WINDOW_INVALID)
      {
        CLog::Log(LOGERROR, "Out of memory / Failed to create new object in LoadUserWindows");
        return false;
      }
      if (g_windowManager.GetWindow(WINDOW_HOME + id))
      {
        delete pWindow;
        continue;
      }
      // set the window's xml file, and add it to the window manager.
      pWindow->SetXMLFile(FindFileData.cFileName);
      pWindow->SetID(WINDOW_HOME + id);

      g_windowManager.AddCustomWindow(pWindow);
    }
    CloseHandle(hFind);
  }
  return true;
}

void CApplication::RenderNoPresent()
{
  MEASURE_FUNCTION;

  int vsync_mode = g_guiSettings.GetInt("videoscreen.vsync");
  
  bool bForceMovieFps     = IsPlayingVideo() && m_itemCurrentFile.get() && m_itemCurrentFile->GetPropertyBOOL("UseMovieFPS");
  bool bEmulateVideoFullScreen = IsPlayingVideo() && m_itemCurrentFile.get() && m_itemCurrentFile->GetPropertyBOOL("EmulateVideoFullScreen");

  if ( (bForceMovieFps || g_graphicsContext.IsFullScreenVideo()) && IsPlaying() && vsync_mode == VSYNC_VIDEO)
    g_Windowing.SetVSync(true);
  else if (vsync_mode == VSYNC_ALWAYS)
    g_Windowing.SetVSync(true);
  else if (vsync_mode != VSYNC_DRIVER)
    g_Windowing.SetVSync(false);

  // dont show GUI when playing full screen video
  if (g_graphicsContext.IsFullScreenVideo() && IsPlaying() && !IsPaused())
  {
    if (vsync_mode==VSYNC_VIDEO)
      g_Windowing.SetVSync(true);
    if (m_bPresentFrame)
      g_renderManager.Present();
    else
      g_renderManager.RenderUpdate(true, 0, 255);

    if (NeedRenderFullScreen())
      RenderFullScreen();

    RenderMemoryStatus();

    ResetScreenSaver();
    g_infoManager.ResetCache();

    if (m_pPlayer->MouseRenderingEnabled() && (m_pPlayer->ForceMouseRendering() || g_Mouse.IsActive()))
    {
      m_guiPointer.Render();
    }

    return;
  }


// DXMERGE: This may have been important?
//  g_graphicsContext.AcquireCurrentContext();

  if (bEmulateVideoFullScreen)
    ResetScreenSaver();
  
  g_ApplicationRenderer.Render();

  // Render the mouse pointer
  if (g_Mouse.IsActive())
  {
    m_guiPointer.Render();
  }
}

void CApplication::DoRender()
{  
  g_graphicsContext.Lock();

  g_windowManager.UpdateModelessVisibility();

#ifdef _DEBUG
  if(g_advancedSettings.m_bWireFrameMode)
  {
    // Boxee: do not clear the buffers as it just eat fills rate and the boxee ui always draws full screen
    // draw GUI
    g_graphicsContext.Clear();
  }
#endif

#ifndef HAS_EMBEDDED
  g_graphicsContext.Clear();
#endif

  if(IsPlayingLiveTV())
    g_graphicsContext.Clear();
  
  if (m_renderingEnabled || g_windowManager.HasDialogOnScreen())
  {
    // when using overscan, we need to clear the window,
    // since our rendering does not cover the entire screen
    if(g_graphicsContext.IsUsingOverscan() || g_graphicsContext.IsCalibrating())
      g_graphicsContext.Clear();

    g_windowManager.Render();
    g_windowManager.RenderDialogs();
  }

  CAppManager::GetInstance().PingNativeApps(); // make sure all render operations take place always - no matter if the app is showing or not

  RenderMemoryStatus();
  RenderScreenSaver();

  g_TextureManager.FreeUnusedTextures();

  g_graphicsContext.Unlock();

  // reset our info cache - we do this at the end of Render so that it is
  // fresh for the next process(), or after a windowclose animation (where process()
  // isn't called)
  g_infoManager.ResetCache();
}

static int screenSaverFadeAmount = 0;

void CApplication::RenderScreenSaver()
{
  bool draw = false;
    float amount = 0.0f;
    if (m_screenSaverMode == "Dim")
      amount = 1.0f - g_guiSettings.GetInt("screensaver.dimlevel")*0.01f;
    else if (m_screenSaverMode == "Black")
      amount = 1.0f; // fully fade
  // special case for dim screensaver
  if (amount > 0.f)
  {
    if (m_bScreenSave)
    {
      draw = true;
      if (screenSaverFadeAmount < 100)
      {
        screenSaverFadeAmount = std::min(100, screenSaverFadeAmount + 2);  // around a second to fade
      }
  }
  else
    {
      if (screenSaverFadeAmount > 0)
      {
        draw = true;
        screenSaverFadeAmount = std::max(0, screenSaverFadeAmount - 4);  // around a half second to unfade
      }
    }
  }
  if (draw)
  {
    color_t color = ((color_t)(screenSaverFadeAmount * amount * 2.55f) & 0xff) << 24;
    CGUITexture::DrawQuad(CRect(0, 0, (float)g_graphicsContext.GetWidth(), (float)g_graphicsContext.GetHeight()), color);
  }
}

bool CApplication::WaitFrame(unsigned int timeout)
{
  bool done = false;
#ifdef HAS_SDL
  // Wait for all other frames to be presented
  SDL_mutexP(m_frameMutex);
  while(m_frameCount > 0)
  {
    int result = SDL_CondWaitTimeout(m_frameCond, m_frameMutex, timeout);
    if(result == SDL_MUTEX_TIMEDOUT)
      break;
    if(result < 0)
      CLog::Log(LOGWARNING, "CApplication::WaitFrame - error from conditional wait");
  }  
  done = m_frameCount == 0;
  SDL_mutexV(m_frameMutex);
#endif
  return done;
}

void CApplication::NewFrame()
{
#ifdef HAS_SDL
  // We just posted another frame. Keep track and notify.
  SDL_mutexP(m_frameMutex);
  m_frameCount++;
  SDL_mutexV(m_frameMutex);
  SDL_CondBroadcast(m_frameCond);
#endif
}

void CApplication::Render()
{
  bool bForceMovieFps = false;

  if (!m_AppActive && !m_bStop && (!IsPlayingVideo() || IsPaused()))
  {
    Sleep(1);
    ResetScreenSaver();
    return;
  }
  
#ifdef __APPLE__
  if (m_AppFocused)
  {
    // Need to "tickle" the OS in order to prevent its screen saver from taking over
    if ((CTimeUtils::GetTimeMS() - m_dwOSXscreensaverTicks) > 30000)
    {
      Cocoa_UpdateSystemActivity();
      m_dwOSXscreensaverTicks = CTimeUtils::GetTimeMS();        
    }
  }
#endif  
  
  if (m_pPlayer)
    m_pPlayer->Ping();
  
  { // frame rate limiter (really bad, but it does the trick :p)
    static unsigned int lastFrameTime = 0;
    unsigned int currentTime = CTimeUtils::GetTimeMS();
    int nDelayTime = 0;
    // Less fps in DPMS or Black screensaver
    bool lowfps = (m_dpmsIsActive
                   || (m_bScreenSave && (m_screenSaverMode == "Black")
                       && (screenSaverFadeAmount >= 100)));
    // Whether externalplayer is playing and we're unfocused
    bool extPlayerActive = m_eCurrentPlayer >= EPC_EXTPLAYER && IsPlaying() && !m_AppFocused;
    unsigned int singleFrameTime = 16; // default limit 60 fps

    m_bPresentFrame = false;
    bool bIsDirectRendering = false;
    if(m_pPlayer)
      bIsDirectRendering = m_pPlayer->IsDirectRendering();
    if (!bIsDirectRendering && !extPlayerActive && g_graphicsContext.IsFullScreenVideo() && !IsPaused())
      bForceMovieFps = IsPlayingVideo() && m_itemCurrentFile.get() && m_itemCurrentFile->GetPropertyBOOL("UseMovieFPS");
    if (!extPlayerActive && (g_graphicsContext.IsFullScreenVideo() || bForceMovieFps) && !IsPaused())
    {
#ifdef HAS_SDL
      SDL_mutexP(m_frameMutex);

      // If we have frames or if we get notified of one, consume it.
      while(m_frameCount == 0 && !bIsDirectRendering)
      {
        int result = SDL_CondWaitTimeout(m_frameCond, m_frameMutex, 100);
        if(result == SDL_MUTEX_TIMEDOUT)
          break;
        if(result < 0)
          CLog::Log(LOGWARNING, "CApplication::Render - error from conditional wait");
      }

      m_bPresentFrame = m_frameCount > 0;
      SDL_mutexV(m_frameMutex);
#else
      m_bPresentFrame = true;
#endif
    }
    else
    {
      // engage the frame limiter as needed
      bool limitFrames = lowfps || extPlayerActive;
      // DXMERGE - we checked for g_videoConfig.GetVSyncMode() before this
      //           perhaps allowing it to be set differently than the UI option??
      if (g_guiSettings.GetInt("videoscreen.vsync") == VSYNC_DISABLED ||
          g_guiSettings.GetInt("videoscreen.vsync") == VSYNC_VIDEO)
        limitFrames = true; // not using vsync.
      else if ((g_infoManager.GetFPS() > g_graphicsContext.GetFPS() + 10) && g_infoManager.GetFPS() > 1000/singleFrameTime)
        limitFrames = true; // using vsync, but it isn't working.

      if (limitFrames)
      {
        if (extPlayerActive)
        {
          ResetScreenSaver();  // Prevent screensaver dimming the screen
          singleFrameTime = 1000;  // 1 fps, high wakeup latency but v.low CPU usage
        }
        else if (lowfps)
          singleFrameTime = 200;  // 5 fps, <=200 ms latency to wake up

        if (lastFrameTime + singleFrameTime > currentTime)
          nDelayTime = lastFrameTime + singleFrameTime - currentTime;
#if defined (HAS_FRAMELIMITER) || defined (HAS_EMBEDDED)
        Sleep(nDelayTime);
#endif
      }
    }

    lastFrameTime = CTimeUtils::GetTimeMS();
  }
  g_graphicsContext.Lock();

  if(!g_Windowing.BeginRender())
    return;

  g_graphicsContext.ResetAllStacks();

  RenderNoPresent();
  g_Windowing.EndRender();

  if (m_renderingEnabled || g_windowManager.HasDialogOnScreen())
  {
    g_graphicsContext.Flip();
  }
  else
  {
#ifdef CANMORE
    // turn plane D off completely. its a minor optimization to load on gdl memory.
    // the next time that eglSwapBuffers will be called - the plane will turn on again.
    gdl_flip(GDL_PLANE_ID_UPP_D, GDL_SURFACE_INVALID, GDL_FLIP_ASYNC);
#endif
#ifndef WIN32
    usleep(25000);
#else
    Sleep(25);
#endif
  }

  CTimeUtils::UpdateFrameTime();
  g_infoManager.UpdateFPS();
  g_renderManager.UpdateResolution();
  g_graphicsContext.Unlock();

#ifdef HAS_SDL
  SDL_mutexP(m_frameMutex);
  if(m_frameCount > 0)
    m_frameCount--;
  SDL_mutexV(m_frameMutex);
  SDL_CondBroadcast(m_frameCond);
#endif
#ifdef HAS_EMBEDDED
  SetLeds();
#endif
}

std::string CApplication::GetBoxeeServerIP()
{
  CSingleLock lock(m_ipLock);
  return m_serverIpAddress;
}

void CApplication::SetBoxeeServerIP(const std::string &ip)
{
  CSingleLock lock(m_ipLock);
  m_serverIpAddress = ip;
}


void CApplication::RenderMemoryStatus()
{
  MEASURE_FUNCTION;

  static CStdString info;
  static CStdString info2;
  static int renderMemory = 0;
  static float x;
  static float y;
  static float y2;

  renderMemory++;

  if (CLog::m_showLogLine && info.length() > 0)
  {
    static CGUITextLayout* infoLayout = NULL;
    static CGUITextLayout* infoLayout2 = NULL;

    if (!infoLayout)
    {
      infoLayout = new CGUITextLayout(g_fontManager.GetFont("font13"), false, 20);
      infoLayout2 = new CGUITextLayout(g_fontManager.GetFont("font13"), false, 20);
    }

    infoLayout->Update(info);
    infoLayout->Render(x, y, 0.0, 0xffffffff, 0xff000000, 0, 1280, true, false);

    if (info2.length())
    {
      infoLayout2->Update(info2);
      infoLayout2->Render(x, y2, 0.0, 0xffffffff, 0xff000000, 0, 1280, true, false);
    }
  }

  if (renderMemory % 120 != 0)
  {
    return;
  }

  g_cpuInfo.getUsedPercentage(); // must call it to recalculate pct values

    // reset the window scaling and fade status
    RESOLUTION res = g_graphicsContext.GetVideoResolution();

    MEMORYSTATUS stat;
    GlobalMemoryStatus(&stat);
    CStdString profiling = CGUIControlProfiler::IsRunning() ? " (profiling)" : "";
#ifdef __APPLE__
      double dCPU = m_resourceCounter.GetCPUUsage();
      
      info.Format("FreeMem %ju/%ju MB, FPS %2.1f, IP: [%s] Loader Q:[%d] Textures:[%u (%4.2f M)]", 
               stat.dwAvailPhys/(1024*1024), stat.dwTotalPhys/(1024*1024),
               g_infoManager.GetFPS(),
               m_network.GetFirstConnectedInterface()?m_network.GetFirstConnectedInterface()->GetCurrentIPAddress().c_str():"Disconnected", 
                  m_ItemLoader?GetItemLoader().GetQueueSize():0, g_TextureManager.GetNumOfTextures(), (double)g_TextureManager.GetMemoryUsage() / 1048576.0);

      info2.Format("CPU-Total %d%%. CPU-XBMC %4.2f%%%s", g_cpuInfo.getUsedPercentage(), dCPU, profiling.c_str());
#elif !defined(_LINUX) || defined(HAS_EMBEDDED)
    CStdString strCores = g_cpuInfo.GetCoresUsageString();
    info.Format("FreeMem %d/%d Kb, FPS %2.1f, %s%s Loader Q:[%d] Textures:[%u (%4.2f M)]", stat.dwAvailPhys/1024, stat.dwTotalPhys/1024, g_infoManager.GetFPS(), strCores.c_str(), profiling.c_str(), 
                m_ItemLoader?GetItemLoader().GetQueueSize():0, g_TextureManager.GetNumOfTextures(), (double)g_TextureManager.GetMemoryUsage() / 1048576.0);
#else
      double dCPU = m_resourceCounter.GetCPUUsage();
      CStdString strCores = g_cpuInfo.GetCoresUsageString();
    info.Format("FreeMem %d/%d Kb, FPS %2.1f, IP: [%s] Loader Q:[%d] Textures:[%u (%4.2f M)]", stat.dwAvailPhys/1024, stat.dwTotalPhys/1024,
               g_infoManager.GetFPS(),
               m_network.GetFirstConnectedInterface()?m_network.GetFirstConnectedInterface()->GetCurrentIPAddress().c_str():"Disconnected", 
                m_ItemLoader?GetItemLoader().GetQueueSize():0, g_TextureManager.GetNumOfTextures(), (double)g_TextureManager.GetMemoryUsage() / 1048576.0);
    info2.Format("%s. CPU-XBMC %4.2f%%%s ", strCores.c_str(), dCPU , profiling.c_str());
#endif
    
    // boxee
    info += " SERVER: [" + GetBoxeeServerIP() + "]";
    // end boxee
    
    info += " ver: " + g_infoManager.GetVersion();


    static int yShift = 20;
    static int xShift = 40;
    static unsigned int lastShift = time(NULL);
    time_t now = time(NULL);
    if (now - lastShift > 10)
    {
      yShift *= -1;
      if (now % 5 == 0)
        xShift *= -1;
      lastShift = now;
      CLog::Log(LOGDEBUG,"(dbg) %s", info.c_str());
      CLog::Log(LOGDEBUG,"(dbg) %s", info2.c_str());
    }

    x = xShift + 0.04f * g_graphicsContext.GetWidth() + g_settings.m_ResInfo[res].Overscan.left;
    y = yShift + 0.04f * g_graphicsContext.GetHeight() + g_settings.m_ResInfo[res].Overscan.top;
    y2 = yShift + 0.07f * g_graphicsContext.GetHeight() + g_settings.m_ResInfo[res].Overscan.top;

/*
#ifdef HAS_EMBEDDED
  if (info.length() > 0)
      CLog::Log(LOGINFO, "%s", info.c_str());
#endif
*/
}

bool CApplication::ShouldDeactivateMouse()
{
  if (g_graphicsContext.IsFullScreenVideo() &&
      IsPlaying() &&
      !IsPaused() &&
      m_pPlayer->MouseRenderingEnabled() &&
      (m_pPlayer->ForceMouseRendering() || g_Mouse.IsActive()))
    return false;
  else
    return true;
}

bool CApplication::CheckKonamiCode(CKey& key)
{
  uint32_t button = key.GetButtonCode();

  static CStdString KONAMI_CODE = "UUDDLRLRBA";
  static CStdString konamiCodeCandidate;

  if (button == (KEY_VKEY | 0x28)) konamiCodeCandidate += "D";
  else if (button == (KEY_VKEY | 0x26)) konamiCodeCandidate += "U";
  else if (button == (KEY_VKEY | 0x25)) konamiCodeCandidate += "L";
  else if (button == (KEY_VKEY | 0x27)) konamiCodeCandidate += "R";
  else if (button == (KEY_VKEY | 0x41)) konamiCodeCandidate += "A";
  else if (button == (KEY_VKEY | 0x42)) konamiCodeCandidate += "B";
  else if (button == (KEY_VKEY | 0xd)) konamiCodeCandidate += "E";

  if (KONAMI_CODE.find(konamiCodeCandidate) != 0)
  {
    konamiCodeCandidate = "";
    return false;
  }
  else if (KONAMI_CODE == konamiCodeCandidate)
  {
    konamiCodeCandidate = "";
    return true;
  }
  else
  {
    return false;
  }
}

#ifdef HAS_EMBEDDED
bool CApplication::CheckEnableEDIDCode(CKey& key)
{
  uint32_t button = key.GetButtonCode();

  static CStdString FORCE_EDID_CODE = "URDLURDLBA";
  static CStdString ForceEDIDCodeCandidate;

  if (button == (KEY_VKEY | 0x28)) ForceEDIDCodeCandidate += "D";
  else if (button == (KEY_VKEY | 0x26)) ForceEDIDCodeCandidate += "U";
  else if (button == (KEY_VKEY | 0x25)) ForceEDIDCodeCandidate += "L";
  else if (button == (KEY_VKEY | 0x27)) ForceEDIDCodeCandidate += "R";
  else if (button == (KEY_VKEY | 0x41)) ForceEDIDCodeCandidate += "A";
  else if (button == (KEY_VKEY | 0x42)) ForceEDIDCodeCandidate += "B";
  else if (button == (KEY_VKEY | 0xd)) ForceEDIDCodeCandidate += "E";

  if (FORCE_EDID_CODE.find(ForceEDIDCodeCandidate) != 0)
  {
    ForceEDIDCodeCandidate = "";
    return false;
  }
  else if (FORCE_EDID_CODE == ForceEDIDCodeCandidate)
  {
    ForceEDIDCodeCandidate = "";
    return true;
  }
  else
  {
    return false;
  }
}


bool CApplication::CheckGUISettingsCode(CKey& key)
{
  uint32_t button = key.GetButtonCode();

  static CStdString DELETE_GUI_SETTINGS_CODE = "URURULULDDBA";
  static CStdString DeleteGUISettingsCandidate;

  if (button == (KEY_VKEY | 0x28)) DeleteGUISettingsCandidate += "D";
  else if (button == (KEY_VKEY | 0x26)) DeleteGUISettingsCandidate += "U";
  else if (button == (KEY_VKEY | 0x25)) DeleteGUISettingsCandidate += "L";
  else if (button == (KEY_VKEY | 0x27)) DeleteGUISettingsCandidate += "R";
  else if (button == (KEY_VKEY | 0x41)) DeleteGUISettingsCandidate += "A";
  else if (button == (KEY_VKEY | 0x42)) DeleteGUISettingsCandidate += "B";
  else if (button == (KEY_VKEY | 0xd)) DeleteGUISettingsCandidate += "E";

  if (DELETE_GUI_SETTINGS_CODE.find(DeleteGUISettingsCandidate) != 0)
  {
    DeleteGUISettingsCandidate = "";
    return false;
  }
  else if (DELETE_GUI_SETTINGS_CODE == DeleteGUISettingsCandidate)
  {
    DeleteGUISettingsCandidate = "";
    return true;
  }
  else
  {
    return false;
  }
}

bool CApplication::CheckSwitchTopRemoteButtonFunctionality(CKey& key)
{
  bool switchWasMade = false;

  // Handle switching Netflix and Play/Pause button functionality
  if (key.GetButtonCode() == 0xF0B3 || key.GetButtonCode() == 0xF07A || key.GetButtonCode() == 0xF07E)
  {
    WORD myKey = (WORD)key.GetButtonCode();

    // if user has selected to always use netflix/pp button as netflix, return f15/F07E code
    if (key.GetButtonCode() == 0xF07A && g_guiSettings.GetInt("services.netflixplaypause") == NRDPP_NETFLIX)
    {
      myKey = 0xF07E;
    }
    // if user has selected to always use netflix/pp button as play_pause, return f11/F07A code
    if (key.GetButtonCode() == 0xF07E && g_guiSettings.GetInt("services.netflixplaypause") == NRDPP_PLAYPAUSE)
    {
      myKey = 0xF07A;
    }
    // if user has selected to always use play_pause button as netflix, return f15/F07E code
    if (key.GetButtonCode() == 0xF0B3 && g_guiSettings.GetInt("services.netflixplaypause") == NRDPP_NETFLIX)
    {
      myKey = 0xF07E;
    }

    CKey tempKey(myKey, key.GetLeftTrigger(), key.GetRightTrigger(), key.GetLeftThumbX(), key.GetLeftThumbY(), key.GetRightThumbX(), key.GetRightThumbY(), key.GetRepeat());
    key = tempKey;

    switchWasMade = true;
  }

  return switchWasMade;
}

#endif

// OnKey() translates the key into a CAction which is sent on to our Window Manager.
// The window manager will return true if the event is processed, false otherwise.
// If not already processed, this routine handles global keypresses.  It returns
// true if the key has been processed, false otherwise.
bool CApplication::OnKey(CKey& key)
{
  if (CheckKonamiCode(key))
  {
    std::vector<CStdString> params;
    g_windowManager.CloseDialogs(true);
    g_windowManager.ActivateWindow(WINDOW_SETTINGS_MYPICTURES /* not really mypictures */, params);
  }

#ifdef HAS_EMBEDDED
  if (CheckEnableEDIDCode(key))
  {
    bool enable = g_guiSettings.GetBool("videoscreen.forceedid");
    g_guiSettings.SetBool("videoscreen.forceedid", !enable);
    g_settings.Save();
    CLog::Log(LOGNONE, "CApplication::OnKey forceedid is now %d. Restarting...", !enable);
    ThreadMessage tMsg(TMSG_QUIT);
    g_application.getApplicationMessenger().SendMessage(tMsg,true);
  }

  if (CheckGUISettingsCode(key))
  {
    CLog::Log(LOGNONE,"CApplication::CheckGUISettingsCode detected - will delete GuiSettings.xml file and restart");

    CStdString header = g_localizeStrings.Get(51578);
    CStdString line = g_localizeStrings.Get(51579) + g_localizeStrings.Get(51582);
    CStdString noLabel = g_localizeStrings.Get(222);
    CStdString yesLabel = g_localizeStrings.Get(51583);

    //delete GuiSettings.xml file
    CFile settings;
    settings.Delete("special://masterprofile/guisettings.xml");

    CLog::Log(LOGNONE, "CApplication::CheckGUISettingsCode - going to send TMSG_QUIT message");

    //sent quit msg
    ThreadMessage tMsg(TMSG_QUIT);
    g_application.getApplicationMessenger().SendMessage(tMsg, true);
  }

  CheckSwitchTopRemoteButtonFunctionality(key);
#endif

  // Turn the mouse off, as we've just got a keypress from controller or remote
  if (ShouldDeactivateMouse())
  {
    g_Mouse.SetActive(false);
  }
  CAction action;  
  
  // get the current focused window
  int iWin = g_windowManager.GetFocusedWindow() & WINDOW_ID_MASK;
	
  // this will be checked for certain keycodes that need 
  // special handling if the screensaver is active   
  CButtonTranslator::GetInstance().GetAction(iWin, key, action);

  CLog::Log(LOGDEBUG,"CApplication::OnKey - [key=%u][window=%d][action=%u]",key.GetButtonCode(), iWin, action.id);

  action.unicode = g_Keyboard.GetUnicode();
  
  // a key has been pressed.
  // Reset the screensaver timer
  // but not for the analog thumbsticks/triggers
  if (!key.IsAnalogButton())
  {    
    // reset Idle Timer
    m_idleTimer.StartZero();
    bool processKey = AlwaysProcess(action);
		
    ResetScreenSaver();
		
    // allow some keys to be processed while the screensaver is active
    if (WakeUpScreenSaverAndDPMS() && !processKey)
    {      
      g_Keyboard.Reset();
      return true;
    }
  }
	
  // change this if we have a dialog up
  if (g_windowManager.HasModalDialog())
  {
    iWin = g_windowManager.GetTopMostModalDialogID() & WINDOW_ID_MASK;
  }

  if (iWin == WINDOW_DIALOG_FULLSCREEN_INFO)
  { // fullscreen info dialog - special case
    CButtonTranslator::GetInstance().GetAction(iWin, key, action);
		
    g_Keyboard.Reset();

    if (OnAction(action))
      return true;
		
    // fallthrough to the main window
    iWin = WINDOW_FULLSCREEN_VIDEO;
  }
  if (iWin == WINDOW_FULLSCREEN_VIDEO  &&
     (!g_application.m_pPlayer || !g_application.m_pPlayer->KeyboardPassthrough())) // for flash player
  {
    // current active window is full screen video.
    if (g_application.m_pPlayer && g_application.m_pPlayer->IsInMenu())
    {
      // if player is in some sort of menu, (ie DVDMENU) map buttons differently
      CButtonTranslator::GetInstance().GetAction(WINDOW_VIDEO_MENU, key, action);
    }
    else
    {
      // no then use the fullscreen window section of keymap.xml to map key->action
      CButtonTranslator::GetInstance().GetAction(iWin, key, action);
    }
  }
  else
  {
    // current active window isnt the fullscreen window
    // just use corresponding section from keymap.xml
    // to map key->action
		
    // first determine if we should use keyboard input directly
	  
		bool isNavigationKey = true;
	  
    // If the key is preseed within keyboard window and it is not one of the navigation keys it should be treated as keyboard
		if (key.FromKeyboard())
		{
			WORD code = (WORD) key.GetButtonCode();
			BYTE b = code & 0xFF;
			if ((b != 0x25) && (b != 0x27) && (b != 0x26) && (b != 0x28) && (b != 0x0D) && (b != 0x1B) && (b != 0xB3) && (b != 0xFB) && (b != 0x7D) && (b != 0xAF) && (b != 0xAD) && (b != 0x7C) && (b != 0x21) && (b != 0x22) && (b != 0x6c))
			{
			  isNavigationKey = false;
      }
		}
	  
		bool useKeyboard = false;
	  
    CGUIWindow *window = g_windowManager.GetWindow(iWin);
    if (window)
    {
      CGUIControl *control = window->GetFocusedControl();
      if (control)
      {
        if (((control->GetControlType() == CGUIControl::GUICONTROL_EDIT || control->GetControlType() == CGUIControl::GUICONTROL_WEB) && !isNavigationKey && action.id != ACTION_SELECT_ITEM) ||
            (control->IsContainer() && g_Keyboard.GetShift()) || g_Keyboard.GetVKey() == 9)
          useKeyboard = true;
      }
      
      if ((iWin == WINDOW_DIALOG_KEYBOARD || iWin == WINDOW_DIALOG_NUMERIC ||
          (iWin == WINDOW_FULLSCREEN_VIDEO  && g_application.m_pPlayer && g_application.m_pPlayer->KeyboardPassthrough()))
          && !isNavigationKey && action.id != ACTION_SELECT_ITEM)
      {
        useKeyboard = true;
      }
      
      if (window->GetPropertyBOOL("PassthroughKeys") && !isNavigationKey && action.id != ACTION_VOLUME_UP && action.id != ACTION_VOLUME_DOWN)
      {
        useKeyboard = true;
      }

      if ((iWin == WINDOW_DIALOG_BOXEE_BROWSE_MENU || iWin == WINDOW_HOME || iWin == WINDOW_DIALOG_WEB || (iWin >= WINDOW_BOXEE_BROWSE_LOCAL && iWin <= WINDOW_BOXEE_BROWSE_HISTORY)) && !isNavigationKey
          && action.id != ACTION_SELECT_ITEM && action.id == 0)
      {
        useKeyboard = true;
      }
    }
	  
    if (useKeyboard && action.id != ACTION_BUILT_IN_FUNCTION)
    {
      {
        // keyboard entry - pass the keys through directly
      if (key.GetFromHttpApi())
      {
        if (key.GetButtonCode() != KEY_INVALID)
          action.id = key.GetButtonCode();
        action.unicode = key.GetUnicode();
      }
      else
      { // see if we've got an ascii key
        if (g_Keyboard.GetUnicode())
        {
          if (action.unicode == 25)
            action.id = ACTION_PREV_CONTROL;
					
#ifdef __APPLE__
          // If not plain ASCII, use the button translator.
          else if (g_Keyboard.GetAscii() < 32 || g_Keyboard.GetAscii() > 126)
          {
            CButtonTranslator::GetInstance().GetAction(iWin, key, action);
          }
#elif defined(_LINUX)
          else if ( action.kKey.Ctrl && action.unicode == 22)
            action.id = ACTION_PASTE;          
#endif
	        else  
          {  
            action.id = (WORD)g_Keyboard.GetAscii() | KEY_ASCII; // Only for backwards compatibility
            action.unicode = g_Keyboard.GetUnicode();
          }
        }
        else
        {
          if (g_Keyboard.GetVKey() == 9)
          {
            action.id = g_Keyboard.GetShift()?ACTION_PREV_CONTROL:ACTION_NEXT_CONTROL;
          }
          else
          {
            action.id = g_Keyboard.GetVKey() | KEY_VKEY;
            action.unicode = 0;
          }
        }
      }

      g_Keyboard.Reset();

      if (OnAction(action))
        return true;
      // failed to handle the keyboard action, drop down through to standard action
    }
		if (key.GetFromHttpApi())
		{
			if (key.GetButtonCode() != KEY_INVALID)
			{
        action.id = key.GetButtonCode();
        CButtonTranslator::GetInstance().GetAction(iWin, key, action);
      }
      else
      {
        if (action.unicode == 25)
        {
          action.id = ACTION_PREV_CONTROL;
        }
        else 
        {
          CButtonTranslator::GetInstance().GetAction(iWin, key, action);
        }
      }
    }
   }
  }

  if (!key.IsAnalogButton())
    CLog::Log(LOGDEBUG, "%s: %i pressed, action is %i", __FUNCTION__, (int) key.GetButtonCode(), action.id);

  //  Play a sound based on the action
  g_audioManager.PlayActionSound(action);
	
  XBMCMod SavedModState = g_Keyboard.GetModState();
  g_Keyboard.Reset();
  g_Keyboard.SetModState(SavedModState);
	
  return OnAction(action);
}

bool CApplication::OnAction(CAction &action)
{
#ifdef HAS_WEB_SERVER
  // Let's tell the outside world about this action
  if (m_pXbmcHttp && g_stSettings.m_HttpApiBroadcastLevel>=2)
  {
    CStdString tmp;
    tmp.Format("%i",action.id);
    getApplicationMessenger().HttpApi("broadcastlevel; OnAction:"+tmp+";2");
  }
#endif

  if (action.id == ACTION_VOLUME_UP_COND)
  {
    action.amount2 = true; //true - originating from ACTION_VOLUME_UP_COND
    if (!g_guiSettings.GetBool("audiooutput.controlvolume"))
      action.id = ACTION_MOVE_UP;
    else
    {
      action.id = ACTION_VOLUME_UP;
      return OnAction(action);
    }
  }

  if (action.id == ACTION_VOLUME_DOWN_COND)
  {
    action.amount2 = true; //true - originating from ACTION_VOLUME_UP_COND
    if (!g_guiSettings.GetBool("audiooutput.controlvolume"))
      action.id = ACTION_MOVE_DOWN;
    else
    {
      action.id = ACTION_VOLUME_DOWN;
      return OnAction(action);
    }
  }

  // special case for switching between GUI & fullscreen mode.
  if (action.id == ACTION_SHOW_GUI)
  { // Switch to fullscreen mode if we can
    if (SwitchToFullScreen())
    {
      m_navigationTimer.StartZero();
      return true;
    }
  }

  if (action.id == ACTION_TOGGLE_FULLSCREEN)
  {
    g_graphicsContext.ToggleFullScreenRoot();
    return true;
  }

  // in normal case
  // just pass the action to the current window and let it handle it
  if (g_windowManager.OnAction(action))
  {
    m_navigationTimer.StartZero();
    return true;
  }

  // handle extra global presses

  // screenshot : take a screenshot :)
  if (action.id == ACTION_TAKE_SCREENSHOT)
  {
    CUtil::TakeScreenshot();
    return true;
  }
  // built in functions : execute the built-in
  if (action.id == ACTION_BUILT_IN_FUNCTION)
  {
    CBuiltins::Execute(action.strAction);
    m_navigationTimer.StartZero();
    return true;
  }

  // reload keymaps
  if (action.id == ACTION_RELOAD_KEYMAPS)
  {
    CButtonTranslator::GetInstance().Clear();
    CButtonTranslator::GetInstance().Load();
    }

  // show info : Shows the current video or song information
  if (action.id == ACTION_SHOW_INFO)
  {
    g_infoManager.ToggleShowInfo();
    return true;
  }

  // codec info : Shows the current song, video or picture codec information
  if (action.id == ACTION_SHOW_CODEC)
  {
    g_infoManager.ToggleShowCodec();
    return true;
  }

  if ((action.id == ACTION_INCREASE_RATING || action.id == ACTION_DECREASE_RATING) && IsPlayingAudio())
  {
    const CMusicInfoTag *tag = g_infoManager.GetCurrentSongTag();
    if (tag)
    {
      *m_itemCurrentFile->GetMusicInfoTag() = *tag;
      char rating = tag->GetRating();
      bool needsUpdate(false);
      if (rating > '0' && action.id == ACTION_DECREASE_RATING)
      {
        m_itemCurrentFile->GetMusicInfoTag()->SetRating(rating - 1);
        needsUpdate = true;
      }
      else if (rating < '5' && action.id == ACTION_INCREASE_RATING)
      {
        m_itemCurrentFile->GetMusicInfoTag()->SetRating(rating + 1);
        needsUpdate = true;
      }
      if (needsUpdate)
      {
        CMusicDatabase db;
        if (db.Open())      // OpenForWrite() ?
        {
          db.SetSongRating(m_itemCurrentFile->m_strPath, m_itemCurrentFile->GetMusicInfoTag()->GetRating());
          db.Close();
        }
        // send a message to all windows to tell them to update the fileitem (eg playlistplayer, media windows)
        CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, m_itemCurrentFile);
        g_windowManager.SendMessage(msg);
      }
    }
    return true;
  }

  // stop : stops playing current audio song
  if (action.id == ACTION_STOP)
  {
    StopPlaying();
    return true;
  }

  // previous : play previous song from playlist
  if (action.id == ACTION_PREV_ITEM)
  {
    if (m_itemCurrentFile->HasProperty("isradio") && !m_itemCurrentFile->HasProperty("hasprevitem"))
    {
      return true;
    }
    
    if (!g_infoManager.IsPlayerActionAllowed(PLAYER_ACTION_PREV))
    {
      CLog::Log(LOGDEBUG,"Application::OnAction, PLAYER_ACTION_PREV is locked (playerlock)");
      return true;
    }
  
    CAppManager::GetInstance().OnActionPrev();

    // first check whether we're within 3 seconds of the start of the track
    // if not, we just revert to the start of the track
    if (m_pPlayer && m_pPlayer->CanSeek() && GetTime() > 3)
    {
      SeekTime(0);
      SetPlaySpeed(1);
    }
    else
    {
      g_playlistPlayer.PlayPrevious();
    }
    return true;
  }

  // next : play next song from playlist
  if (action.id == ACTION_NEXT_ITEM)
  {
    if (!g_infoManager.IsPlayerActionAllowed(PLAYER_ACTION_NEXT)) 
    {
      CLog::Log(LOGDEBUG,"Application::OnAction, ACTION_NEXT_ITEM is locked (playerlock)");
      return true;
    }
    
    CAppManager::GetInstance().OnActionNext();
  
    if (IsPlaying() && m_pPlayer->SkipNext())
      return true;

    g_playlistPlayer.PlayNext();

    return true;
  }

  if ( IsPlaying())
  {
    // OSD toggling
    if (action.id == ACTION_SHOW_OSD)
    {
      if (IsPlayingVideo() && IsPlayingFullScreenVideo() && !m_pPlayer->OSDDisabled())
      {
        CGUIWindowOSD *pOSD = (CGUIWindowOSD *)g_windowManager.GetWindow(WINDOW_OSD);
        if (pOSD)
        {
          if (pOSD->IsDialogRunning())
            pOSD->Close();
          else
            pOSD->DoModal();
          return true;
        }
      }
    }

    // pause : pauses current audio song
    if (action.id == ACTION_PAUSE && m_iPlaySpeed == 1)
    {
      TogglePlayPause();
      return true;
    }
    if (!m_pPlayer->IsPaused())
    {
      // if we do a FF/RW in my music then map PLAY action togo back to normal speed
      // if we are playing at normal speed, then allow play to pause
      if (action.id == ACTION_PLAYER_PLAY || action.id == ACTION_PAUSE)
      {
        if (m_iPlaySpeed != 1)
        {
          SetPlaySpeed(1);
        }
        else
        {
          m_pPlayer->Pause();
        }
        return true;
      }
      if (action.id == ACTION_PLAYER_FORWARD || action.id == ACTION_PLAYER_REWIND)
      {
        int iPlaySpeed = m_iPlaySpeed;
        if (action.id == ACTION_PLAYER_REWIND && iPlaySpeed == 1) // Enables Rewinding
          iPlaySpeed *= -2;
        else if (action.id == ACTION_PLAYER_REWIND && iPlaySpeed > 1) //goes down a notch if you're FFing
          iPlaySpeed /= 2;
        else if (action.id == ACTION_PLAYER_FORWARD && iPlaySpeed < 1) //goes up a notch if you're RWing
          iPlaySpeed /= 2;
        else
          iPlaySpeed *= 2;

        if (action.id == ACTION_PLAYER_FORWARD && iPlaySpeed == -1) //sets iSpeed back to 1 if -1 (didn't plan for a -1)
          iPlaySpeed = 1;
        if (iPlaySpeed > 32 || iPlaySpeed < -32)
          iPlaySpeed = 1;

        SetPlaySpeed(iPlaySpeed);
        return true;
      }
      else if ((action.amount1 || GetPlaySpeed() != 1) && (action.id == ACTION_ANALOG_REWIND || action.id == ACTION_ANALOG_FORWARD))
      {
        // calculate the speed based on the amount the button is held down
        int iPower = (int)(action.amount1 * MAX_FFWD_SPEED + 0.5f);
        // returns 0 -> MAX_FFWD_SPEED
        int iSpeed = 1 << iPower;
        if (iSpeed != 1 && action.id == ACTION_ANALOG_REWIND)
          iSpeed = -iSpeed;
        g_application.SetPlaySpeed(iSpeed);
        if (iSpeed == 1)
          CLog::Log(LOGDEBUG,"Resetting playspeed");
        return true;
      }
    }
    // allow play to unpause
    else
    {
      if (action.id == ACTION_PLAYER_PLAY)
      {
        // unpause, and set the playspeed back to normal
        m_pPlayer->Pause();
        g_audioManager.Enable(m_pPlayer->IsPaused());

        g_application.SetPlaySpeed(1);
        return true;
      }
    }
  }
  if (action.id == ACTION_MUTE)
  {
    Mute();
    return true;
  }

  if (action.id == ACTION_TOGGLE_DIGITAL_ANALOG)
  {
    if(g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL_SPDIF)
#ifdef HAS_AUDIO_HDMI
      g_guiSettings.SetInt("audiooutput.mode", AUDIO_DIGITAL_HDMI);
    else if (g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL_HDMI)
#endif
      g_guiSettings.SetInt("audiooutput.mode", AUDIO_ANALOG);
    else
      g_guiSettings.SetInt("audiooutput.mode", AUDIO_DIGITAL_SPDIF);
    g_application.Restart();
    if (g_windowManager.GetActiveWindow() == WINDOW_SETTINGS_SYSTEM)
    {
      CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0,0,WINDOW_INVALID,g_windowManager.GetActiveWindow());
      g_windowManager.SendMessage(msg);
    }
    return true;
  }

  // Check for global volume control
  if (action.amount1 && (action.id == ACTION_VOLUME_UP || action.id == ACTION_VOLUME_DOWN))
  {
    CLog::Log(LOGDEBUG,"Analog audio volume control enabled");
    action.amount2 = false; //false - originating from ACTION_VOLUME_UP_COND/ACTION_VOLUME_DOWN_COND

#ifdef __APPLE__
    // on apple - no control over system volume when digital connector is in
    if (g_guiSettings.GetInt("audiooutput.mode") == AUDIO_ANALOG)
#else
    if (1)
#endif
    {
      if (!m_pPlayer || !m_pPlayer->IsPassthrough())
      {
        // increase or decrease the volume
        UpdateVolume();

        int volume = g_stSettings.m_nVolumeLevel + g_stSettings.m_dynamicRangeCompressionLevel;

        // calculate speed so that a full press will equal 1 second from min to max
        float speed = float(VOLUME_MAXIMUM - VOLUME_MINIMUM);
        if( action.repeat )
          speed *= action.repeat;
        else
          speed /= 50; //50 fps
        if (g_stSettings.m_bMute)
        {
          // only unmute if volume is to be increased, otherwise leave muted
          if (action.id == ACTION_VOLUME_DOWN)
            return true;
          Mute();
          return true;
        }

        if (action.id == ACTION_VOLUME_UP)
        {
            volume += (int)((float)fabs(action.amount1) * action.amount1 * speed);
        }
        else
        {
            volume -= (int)((float)fabs(action.amount1) * action.amount1 * speed);
        }

        SetHardwareVolume(volume);
    
        if (!IsPlaying()) {
          g_audioManager.SetVolume(g_stSettings.m_nVolumeLevel);
        }
      }
      // show visual feedback of volume change...
      m_guiDialogVolumeBar.Show();
      m_guiDialogVolumeBar.OnAction(action);
    }
    else
    {
      m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::ICON_EXCLAMATION,"",g_localizeStrings.Get(51610),3000 , KAI_ORANGE_COLOR , KAI_ORANGE_COLOR);
    }
    
    return true;
  }
  // Check for global seek control
  if (IsPlaying() && action.amount1 && (action.id == ACTION_ANALOG_SEEK_FORWARD || action.id == ACTION_ANALOG_SEEK_BACK))
  { 
    if (!m_pPlayer->CanSeek() || m_pPlayer->OSDDisabled()) return false;
    m_guiDialogSeekBar.OnAction(action);
    return true;
  }
  if (action.id == ACTION_GUIPROFILE_BEGIN)
  {
    CGUIControlProfiler::Instance().SetOutputFile(_P("special://home/guiprofiler.xml"));
    CGUIControlProfiler::Instance().Start();
    return true;
  }

  if (action.id == ACTION_OSD_EXT_CLICK)
  {
    CAppManager::GetInstance().OnOsdExt((int)action.amount1);
    return true;
  }

  return false;
}

void CApplication::UpdateLCD()
{
#ifdef HAS_LCD
  static long lTickCount = 0;

  if (!g_lcd || g_guiSettings.GetInt("lcd.type") == LCD_TYPE_NONE)
    return ;
  long lTimeOut = 1000;
  if ( m_iPlaySpeed != 1)
    lTimeOut = 0;
  if ( ((long)CTimeUtils::GetTimeMS() - lTickCount) >= lTimeOut)
  {
    if (g_application.NavigationIdleTime() < 5)
      g_lcd->Render(ILCD::LCD_MODE_NAVIGATION);
    else if (IsPlayingVideo())
      g_lcd->Render(ILCD::LCD_MODE_VIDEO);
    else if (IsPlayingAudio())
      g_lcd->Render(ILCD::LCD_MODE_MUSIC);
    else if (IsInScreenSaver())
      g_lcd->Render(ILCD::LCD_MODE_SCREENSAVER);
    else
      g_lcd->Render(ILCD::LCD_MODE_GENERAL);

    // reset tick count
    lTickCount = CTimeUtils::GetTimeMS();
  }
#endif
}

void CApplication::FrameMove()
{
  MEASURE_FUNCTION;

  // currently we calculate the repeat time (ie time from last similar keypress) just global as fps
  float frameTime = m_frameTime.GetElapsedSeconds();
  m_frameTime.StartZero();
  // never set a frametime less than 2 fps to avoid problems when debuggin and on breaks
  if( frameTime > 0.5 ) frameTime = 0.5;

  // check if there are notifications to display
  if (m_guiDialogKaiToast.DoWork())
  {
    if (!m_guiDialogKaiToast.IsDialogRunning())
    {
      m_guiDialogKaiToast.Show();
    }
  }

  UpdateLCD();

#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE) || defined(HAS_REMOTECONTROL)
  // Read the input from a remote
  g_RemoteControl.Update();
#endif
  
  // process input actions
  CWinEvents::MessagePump();
  ProcessDeferredActions();
  ProcessMouse();
  ProcessHTTPApiButtons();
  ProcessRemote(frameTime);
  ProcessGamepad(frameTime);
  ProcessEventServer(frameTime);
}

bool CApplication::ProcessGamepad(float frameTime)
{
#ifdef HAS_SDL_JOYSTICK
  int iWin = g_windowManager.GetActiveWindow() & WINDOW_ID_MASK;
  if (g_windowManager.HasModalDialog())
  {
    iWin = g_windowManager.GetTopMostModalDialogID() & WINDOW_ID_MASK;
  }
  int bid;
  g_Joystick.Update();
  if (g_Joystick.GetButton(bid))
  {
    // reset Idle Timer
    m_idleTimer.StartZero();

    ResetScreenSaver();
    if (WakeUpScreenSaverAndDPMS())
    {
      g_Joystick.Reset(true);
      return true;
    }

    CAction action;
    bool fullrange;
    string jname = g_Joystick.GetJoystick();
    if (CButtonTranslator::GetInstance().TranslateJoystickString(iWin, jname.c_str(), bid, JACTIVE_BUTTON, action.id, action.strAction, fullrange))
    {
      action.amount1 = 1.0f;
      action.repeat = 0.0f;
      g_audioManager.PlayActionSound(action);
      g_Joystick.Reset();

      if (action.id == ACTION_PARENT_DIR || action.id == ACTION_PREVIOUS_MENU)
        action.buttonCode = KEY_BUTTON_BACK;

      g_Mouse.SetActive(false);
      return OnAction(action);
    }
    else
    {
      g_Joystick.Reset();
    }
  }

  if (g_Joystick.GetAxis(bid))
  {
    CAction action;
    bool fullrange;

    string jname = g_Joystick.GetJoystick();
    action.amount1 = g_Joystick.GetAmount();

    if (action.amount1<0)
    {
      bid = -bid;
    }
    if (CButtonTranslator::GetInstance().TranslateJoystickString(iWin, jname.c_str(), bid, JACTIVE_AXIS, action.id, action.strAction, fullrange))
    {
      ResetScreenSaver();
      if (WakeUpScreenSaverAndDPMS())
      {
        return true;
      }

      if (fullrange)
      {
        action.amount1 = (action.amount1+1.0f)/2.0f;
      }
      else
      {
        action.amount1 = fabs(action.amount1);
      }
      action.amount2 = 0.0;
      action.repeat = 0.0;
      g_audioManager.PlayActionSound(action);
      g_Joystick.Reset();
      g_Mouse.SetActive(false);
      return OnAction(action);
    }
    else
    {
      g_Joystick.ResetAxis(abs(bid));
    }
  }
  int position;
  if (g_Joystick.GetHat(bid, position))
  {
    CAction action;
    bool fullrange;

    string jname = g_Joystick.GetJoystick();
    bid = position<<16|bid;

    if (CButtonTranslator::GetInstance().TranslateJoystickString(iWin, jname.c_str(), bid, JACTIVE_HAT, action.id, action.strAction, fullrange))
    {
      action.amount1 = 1.0f;
      action.repeat = 0.0f;
      g_audioManager.PlayActionSound(action);
      g_Joystick.Reset();
      g_Mouse.SetActive(false);
      return OnAction(action);
    }
  }
#endif
  return false;
}

bool CApplication::ProcessRemote(float frameTime)
{
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE) || defined(HAS_REMOTECONTROL)
  WORD button = g_RemoteControl.GetButton();

  if (button)
{
	// time depends on whether the movement is repeated (held down) or not.
	// If it is, we use the FPS timer to get a repeatable speed.
	// If it isn't, we use 20 to get repeatable jumps.
	float time = (g_RemoteControl.IsHolding()) ? frameTime : 0.020f;
        CKey key(button, 0, 0, 0, 0, 0, 0, time);
	g_RemoteControl.Reset();
	return OnKey(key);
}
#endif
  return false;
}

void CApplication::DeferAction(CAction action)
{
  m_deferredActions.push(action);
}

bool CApplication::ProcessDeferredActions()
{
  MEASURE_FUNCTION;
  
  if (m_deferredActions.size() > 0)
  {
    CAction action = m_deferredActions.front();
    m_deferredActions.pop();
    return g_windowManager.OnAction(action);    
  }
  
  return false;
}

bool CApplication::ProcessMouse()
{
  MEASURE_FUNCTION;

  if (!g_Mouse.IsActive() || !m_AppFocused)
    return false;

  // Reset the screensaver and idle timers
  m_idleTimer.StartZero();
  ResetScreenSaver();
  if (WakeUpScreenSaverAndDPMS())
    return true;

  // call OnAction with ACTION_MOUSE
  CAction action;
  action.id = ACTION_MOUSE;
  action.amount1 = (float) m_guiPointer.GetXPosition();
  action.amount2 = (float) m_guiPointer.GetYPosition();

  return g_windowManager.OnAction(action);
}

void  CApplication::CheckForTitleChange()
{
#ifdef HAS_WEB_SERVER
  if (g_stSettings.m_HttpApiBroadcastLevel>=1)
  {
    if (IsPlayingVideo())
    {
      const CVideoInfoTag* tagVal = g_infoManager.GetCurrentMovieTag();
      if (m_pXbmcHttp && tagVal && !(tagVal->m_strTitle.IsEmpty()))
      {
        CStdString msg=m_pXbmcHttp->GetOpenTag()+"MovieTitle:"+tagVal->m_strTitle+m_pXbmcHttp->GetCloseTag();
        if (m_prevMedia!=msg && g_stSettings.m_HttpApiBroadcastLevel>=1)
        {
          getApplicationMessenger().HttpApi("broadcastlevel; MediaChanged:"+msg+";1");
          m_prevMedia=msg;
        }
      }
    }
    else if (IsPlayingAudio())
    {
      const CMusicInfoTag* tagVal=g_infoManager.GetCurrentSongTag();
      if (m_pXbmcHttp && tagVal)
	  {
	    CStdString msg="";
	    if (!tagVal->GetTitle().IsEmpty())
        msg=m_pXbmcHttp->GetOpenTag()+"AudioTitle:"+tagVal->GetTitle()+m_pXbmcHttp->GetCloseTag();
	    if (!tagVal->GetArtist().IsEmpty())
        msg+=m_pXbmcHttp->GetOpenTag()+"AudioArtist:"+tagVal->GetArtist()+m_pXbmcHttp->GetCloseTag();
	    if (m_prevMedia!=msg)
	    {
        getApplicationMessenger().HttpApi("broadcastlevel; MediaChanged:"+msg+";1");
	      m_prevMedia=msg;
	    }
      }
    }
}
#endif
}

bool CApplication::ProcessHTTPApiButtons()
{
#ifdef HAS_WEB_SERVER
  if (m_pXbmcHttp)
  {    
    // copy key from webserver, and reset it in case we're called again before
    // whatever happens in OnKey()
    CKey keyHttp(m_pXbmcHttp->GetKey());
    m_pXbmcHttp->ResetKey();
    if (keyHttp.GetButtonCode() != KEY_INVALID)
    {
#ifdef __APPLE__
      // Need to "tickle" the OS in order to prevent it from dimming the screen
      if ((CTimeUtils::GetTimeMS() - m_dwOSXscreensaverTicks) > 30000)
      {
        Cocoa_UpdateSystemActivity();
        m_dwOSXscreensaverTicks = CTimeUtils::GetTimeMS();        
      }
#endif
      
      if (keyHttp.GetButtonCode() == KEY_BROWSER_MOUSE) //virtual mouse
      {
        CPoint p = g_Mouse.GetLocation();
        XBMC_Event newEvent;
        newEvent.type = XBMC_MOUSEMOTION;

        RESOLUTION iRes = g_graphicsContext.GetVideoResolution();
        int m_screenX1 = g_settings.m_ResInfo[iRes].Overscan.left;
        int m_screenY1 = g_settings.m_ResInfo[iRes].Overscan.top;
        int m_screenX2 = g_settings.m_ResInfo[iRes].Overscan.right;
        int m_screenY2 = g_settings.m_ResInfo[iRes].Overscan.bottom;

        newEvent.motion.y = std::max(m_screenY1, std::min(m_screenY2 - 10, (int)(p.y + (int)keyHttp.GetLeftThumbY())));
        newEvent.motion.x = std::max(m_screenX1, std::min(m_screenX2 - 10, (int)(p.x + (int)keyHttp.GetLeftThumbX())));
        CLog::Log(LOGDEBUG, "KEY_BROWSER_MOUSE current location %f:%f new location %d:%d remote request %f:%f", p.x, p.y, newEvent.motion.x, newEvent.motion.y, keyHttp.GetLeftThumbX(), keyHttp.GetLeftThumbY());
        CAction action;
        if (keyHttp.GetLeftThumbY() != 0.0 && p.y == (float)newEvent.motion.y)
        {
          if (keyHttp.GetLeftThumbY() < 0.0)
            action.id = ACTION_MOVE_UP;
          else
            action.id = ACTION_MOVE_DOWN;
          g_windowManager.OnAction(action);
        }
        else if (keyHttp.GetLeftThumbX() != 0.0 && p.x == (float)newEvent.motion.x)
        {
          if (keyHttp.GetLeftThumbX() < 0.0)
            action.id = ACTION_STEP_BACK;
          else
            action.id = ACTION_STEP_FORWARD;
          g_windowManager.OnAction(action);
        }
        else
          g_Mouse.HandleEvent(newEvent);
      }
      else if (keyHttp.GetButtonCode() == KEY_VMOUSE) //virtual mouse
      {
        CAction action;
        action.id = ACTION_MOUSE;
        g_Mouse.SetLocation(CPoint(keyHttp.GetLeftThumbX(), keyHttp.GetLeftThumbY()));
        if (keyHttp.GetLeftTrigger()!=0)
          g_Mouse.bClick[keyHttp.GetLeftTrigger()-1]=true;
        if (keyHttp.GetRightTrigger()!=0)
          g_Mouse.bDoubleClick[keyHttp.GetRightTrigger()-1]=true;
        action.amount1 = keyHttp.GetLeftThumbX();
        action.amount2 = keyHttp.GetLeftThumbY();
        g_windowManager.OnAction(action);
      }
      else
      {
        OnKey(keyHttp);
      }
      return true;
    }
  }
#endif

  return false;
}

bool CApplication::ProcessEventServer(float frameTime)
{
#ifdef HAS_EVENT_SERVER
  CEventServer* es = CEventServer::GetInstance();
  if (!es || !es->Running() || es->GetNumberOfClients()==0)
    return false;

  // process any queued up actions
  if (es->ExecuteNextAction())
  {
    // reset idle timers
    m_idleTimer.StartZero();
    ResetScreenSaver();
    WakeUpScreenSaverAndDPMS();
  }

  // now handle any buttons or axis
  std::string joystickName;
  bool isAxis = false;
  float fAmount = 0.0;

  WORD wKeyID = es->GetButtonCode(joystickName, isAxis, fAmount);

  if (wKeyID)
  {
    if (joystickName.length() > 0)
    {
      if (fAmount == 0.0)
        fAmount = 1;

      if (isAxis == true)
      {
        if (fabs(fAmount) >= 0.08)
          m_lastAxisMap[joystickName][wKeyID] = fAmount;
        else
          m_lastAxisMap[joystickName].erase(wKeyID);
      }

      return ProcessJoystickEvent(joystickName, wKeyID, isAxis, fAmount);
    }
    else
    {
      CKey key;
      if(wKeyID == KEY_BUTTON_LEFT_ANALOG_TRIGGER)
        key = CKey(wKeyID, (BYTE)(255*fAmount), 0, 0.0, 0.0, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_RIGHT_ANALOG_TRIGGER)
        key = CKey(wKeyID, 0, (BYTE)(255*fAmount), 0.0, 0.0, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_LEFT)
        key = CKey(wKeyID, 0, 0, -fAmount, 0.0, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_RIGHT)
        key = CKey(wKeyID, 0, 0,  fAmount, 0.0, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_UP)
        key = CKey(wKeyID, 0, 0, 0.0,  fAmount, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_DOWN)
        key = CKey(wKeyID, 0, 0, 0.0, -fAmount, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_LEFT)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0, -fAmount, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0,  fAmount, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_UP)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0, 0.0,  fAmount, frameTime);
      else if(wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_DOWN)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0, 0.0, -fAmount, frameTime);
      else
        key = CKey(wKeyID);
      return OnKey(key);
    }
  }

  if (m_lastAxisMap.size() > 0)
  {
    // Process all the stored axis.
    for (map<std::string, map<int, float> >::iterator iter = m_lastAxisMap.begin(); iter != m_lastAxisMap.end(); ++iter)
    {
      for (map<int, float>::iterator iterAxis = (*iter).second.begin(); iterAxis != (*iter).second.end(); ++iterAxis)
        ProcessJoystickEvent((*iter).first, (*iterAxis).first, true, (*iterAxis).second);
    }
  }

  {
    CAction action;
    action.id = ACTION_MOUSE;
    if (es->GetMousePos(action.amount1, action.amount2) && g_Mouse.IsEnabled())
    {
      CPoint point;
      point.x = action.amount1;
      point.y = action.amount2;
      g_Mouse.SetLocation(point, true);

      return g_windowManager.OnAction(action);
    }
  }
#endif
  return false;
}

bool CApplication::ProcessJoystickEvent(const std::string& joystickName, int wKeyID, bool isAxis, float fAmount)
{
#if defined(HAS_EVENT_SERVER)
  m_idleTimer.StartZero();

   // Make sure to reset screen saver, mouse.
   ResetScreenSaver();
   if (WakeUpScreenSaverAndDPMS())
     return true;

#ifdef HAS_SDL_JOYSTICK
   g_Joystick.Reset();
#endif
   g_Mouse.SetActive(false);

   // Figure out what window we're taking the event for.
   int iWin = g_windowManager.GetActiveWindow() & WINDOW_ID_MASK;
   if (g_windowManager.HasModalDialog())
       iWin = g_windowManager.GetTopMostModalDialogID() & WINDOW_ID_MASK;

   // This code is copied from the OnKey handler, it should be factored out.
   if (iWin == WINDOW_FULLSCREEN_VIDEO &&
       g_application.m_pPlayer &&
       g_application.m_pPlayer->IsInMenu())
   {
     // If player is in some sort of menu, (ie DVDMENU) map buttons differently.
     iWin = WINDOW_VIDEO_MENU;
   }

   bool fullRange = false;
   CAction action;
   action.amount1 = fAmount;

   //if (action.amount1 < 0.0)
   // wKeyID = -wKeyID;

   // Translate using regular joystick translator.
   if (CButtonTranslator::GetInstance().TranslateJoystickString(iWin, joystickName.c_str(), wKeyID, isAxis ? JACTIVE_AXIS : JACTIVE_BUTTON, action.id, action.strAction, fullRange))
   {
     action.repeat = 0.0f;
     g_audioManager.PlayActionSound(action);
     return OnAction(action);
   }
   else
   {
     CLog::Log(LOGDEBUG, "ERROR mapping joystick action. Joystick: %s %i",joystickName.c_str(), wKeyID);
   }
#endif

   return false;
}

bool CApplication::ProcessKeyboard()
{
  MEASURE_FUNCTION;

  // process the keyboard buttons etc.
  uint8_t vkey = g_Keyboard.GetVKey();
  wchar_t unicode = g_Keyboard.GetUnicode();
  if (vkey || unicode)
  {
    // got a valid keypress - convert to a key code
    uint32_t keyID;
    if (vkey) // FIXME, every ascii has a vkey so vkey would always and ascii would never be processed, but fortunately OnKey uses wkeyID only to detect keyboard use and the real key is recalculated correctly.
      keyID = vkey | KEY_VKEY;
    else
      keyID = KEY_UNICODE;
    //  CLog::Log(LOGDEBUG,"Keyboard: time=%i key=%i", CTimeUtils::GetFrameTime(), vkey);
    CKey key(keyID);
    key.SetHeld(g_Keyboard.KeyHeld());
    key.SetModifiers(g_Keyboard.GetModState());
    return OnKey(key);
  }
  return false;
}

HRESULT CApplication::Cleanup()
{
  try
  {
#ifndef _BOXEE_
    g_windowManager.Delete(WINDOW_MUSIC_PLAYLIST);
    g_windowManager.Delete(WINDOW_MUSIC_PLAYLIST_EDITOR);
    g_windowManager.Delete(WINDOW_MUSIC_FILES);
    g_windowManager.Delete(WINDOW_MUSIC_NAV);
#endif
    g_windowManager.Delete(WINDOW_MUSIC_INFO);
#ifndef _BOXEE_
    g_windowManager.Delete(WINDOW_VIDEO_INFO);
    g_windowManager.Delete(WINDOW_VIDEO_FILES);
    g_windowManager.Delete(WINDOW_VIDEO_PLAYLIST);
    g_windowManager.Delete(WINDOW_VIDEO_NAV);
#endif
    g_windowManager.Delete(WINDOW_FILES);
    g_windowManager.Delete(WINDOW_DIALOG_YES_NO);
    g_windowManager.Delete(WINDOW_DIALOG_PROGRESS);
    g_windowManager.Delete(WINDOW_DIALOG_NUMERIC);
    g_windowManager.Delete(WINDOW_DIALOG_GAMEPAD);
    g_windowManager.Delete(WINDOW_DIALOG_SUB_MENU);
    g_windowManager.Delete(WINDOW_DIALOG_BUTTON_MENU);
    g_windowManager.Delete(WINDOW_DIALOG_CONTEXT_MENU);
    g_windowManager.Delete(WINDOW_DIALOG_MUSIC_SCAN);
    g_windowManager.Delete(WINDOW_DIALOG_PLAYER_CONTROLS);
    g_windowManager.Delete(WINDOW_DIALOG_KARAOKE_SONGSELECT);
    g_windowManager.Delete(WINDOW_DIALOG_KARAOKE_SELECTOR);
    g_windowManager.Delete(WINDOW_DIALOG_MUSIC_OSD);
    g_windowManager.Delete(WINDOW_DIALOG_VIS_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_VIS_PRESET_LIST);
    g_windowManager.Delete(WINDOW_DIALOG_SELECT);
    g_windowManager.Delete(WINDOW_DIALOG_OK);
    g_windowManager.Delete(WINDOW_DIALOG_WEB);
    g_windowManager.Delete(WINDOW_DIALOG_FILESTACKING);
    g_windowManager.Delete(WINDOW_DIALOG_KEYBOARD);
    g_windowManager.Delete(WINDOW_FULLSCREEN_VIDEO);
    g_windowManager.Delete(WINDOW_DIALOG_PROFILE_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_LOCK_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_NETWORK_SETUP);
    g_windowManager.Delete(WINDOW_DIALOG_MEDIA_SOURCE);
    g_windowManager.Delete(WINDOW_DIALOG_VIDEO_OSD_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_AUDIO_OSD_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_SUBTITLE_OSD_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_VIDEO_BOOKMARKS);
    g_windowManager.Delete(WINDOW_DIALOG_VIDEO_SCAN);
    g_windowManager.Delete(WINDOW_DIALOG_CONTENT_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_FAVOURITES);
    g_windowManager.Delete(WINDOW_DIALOG_SONG_INFO);
    g_windowManager.Delete(WINDOW_DIALOG_SMART_PLAYLIST_EDITOR);
    g_windowManager.Delete(WINDOW_DIALOG_SMART_PLAYLIST_RULE);
    g_windowManager.Delete(WINDOW_DIALOG_BUSY);
    g_windowManager.Delete(WINDOW_DIALOG_PICTURE_INFO);
    g_windowManager.Delete(WINDOW_DIALOG_PLUGIN_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_ACCESS_POINTS);
    g_windowManager.Delete(WINDOW_DIALOG_SLIDER);

    g_windowManager.Delete(WINDOW_DIALOG_OSD_TELETEXT);

    g_windowManager.Delete(WINDOW_STARTUP);
    g_windowManager.Delete(WINDOW_LOGIN_SCREEN);
    g_windowManager.Delete(WINDOW_VISUALISATION);
    g_windowManager.Delete(WINDOW_KARAOKELYRICS);
    g_windowManager.Delete(WINDOW_SETTINGS_MENU);
    g_windowManager.Delete(WINDOW_SETTINGS_PROFILES);
    g_windowManager.Delete(WINDOW_SETTINGS_MYPICTURES);  // all the settings categories
    g_windowManager.Delete(WINDOW_TEST_PATTERN);
    g_windowManager.Delete(WINDOW_SCREEN_CALIBRATION);
    g_windowManager.Delete(WINDOW_SYSTEM_INFORMATION);
    g_windowManager.Delete(WINDOW_SCREENSAVER);
    g_windowManager.Delete(WINDOW_OSD);
    g_windowManager.Delete(WINDOW_MUSIC_OVERLAY);
    g_windowManager.Delete(WINDOW_VIDEO_OVERLAY);
    g_windowManager.Delete(WINDOW_SCRIPTS_INFO);
    g_windowManager.Delete(WINDOW_SLIDESHOW);

    g_windowManager.Delete(WINDOW_HOME);
#ifndef _BOXEE_
    g_windowManager.Delete(WINDOW_PROGRAMS);
    g_windowManager.Delete(WINDOW_PICTURES);
    g_windowManager.Delete(WINDOW_SCRIPTS);
#endif
    g_windowManager.Delete(WINDOW_WEATHER);

    g_windowManager.Delete(WINDOW_SETTINGS_MYPICTURES);
    g_windowManager.Remove(WINDOW_SETTINGS_PARRENTAL);
    g_windowManager.Remove(WINDOW_SETTINGS_MYPERSONAL);
    g_windowManager.Remove(WINDOW_SETTINGS_SYSTEM);
    g_windowManager.Remove(WINDOW_SETTINGS_MYMEDIA);
    g_windowManager.Remove(WINDOW_SETTINGS_NETWORK);
    g_windowManager.Remove(WINDOW_SETTINGS_LIVE_TV);
    g_windowManager.Remove(WINDOW_SETTINGS_APPEARANCE);
    g_windowManager.Remove(WINDOW_DIALOG_KAI_TOAST);
    g_windowManager.Remove(WINDOW_SETTINGS_MYMEDIA);

    g_windowManager.Remove(WINDOW_DIALOG_SEEK_BAR);
    g_windowManager.Remove(WINDOW_DIALOG_VOLUME_BAR);

// Boxee
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_LOGGING_IN);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_CREDITS);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_SHARE);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_POST_PLAY);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_RATE);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_USER_INFO);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_USER_PASSWORD);
   
#ifdef HAS_WIDGETS
    g_windowManager.Delete(WINDOW_DIALOG_WIDGETS_TEST);
#endif
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_USER_LOGIN);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_VIDEO_CTX);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_MUSIC_CTX);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_PICTURE_CTX);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_APP_CTX);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_USER_LOGIN);
#ifdef HAS_XRANDR
    g_windowManager.Delete(WINDOW_BOXEE_WIZARD_RESOLUTION);
#endif
    g_windowManager.Delete(WINDOW_BOXEE_WIZARD_AUDIO);
    g_windowManager.Delete(WINDOW_BOXEE_WIZARD_NETWORK);
    g_windowManager.Delete(WINDOW_BOXEE_WIZARD_TIMEZONE);
    g_windowManager.Delete(WINDOW_BOXEE_WIZARD_ADD_SOURCE);
    g_windowManager.Delete(WINDOW_BOXEE_WIZARD_ADD_SOURCE_MANUAL);
    g_windowManager.Delete(WINDOW_BOXEE_WIZARD_SOURCE_MANAGER);
    g_windowManager.Delete(WINDOW_BOXEE_WIZARD_SOURCE_NAME);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_BUFFERING);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_CHAPTERS);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_EXIT_VIDEO);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_QUICK_TIP);
    g_windowManager.Delete(WINDOW_DIALOG_MENU_CUSTOMIZATION);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_DROPDOWN);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_SEARCH);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_GLOBAL_SEARCH);
    //g_windowManager.Remove(WINDOW_BOXEE_BROWSE); 
    g_windowManager.Remove(WINDOW_BOXEE_BROWSE_LOCAL);
    g_windowManager.Remove(WINDOW_BOXEE_BROWSE_TVSHOWS);
    g_windowManager.Remove(WINDOW_BOXEE_BROWSE_MOVIES);
    g_windowManager.Remove(WINDOW_BOXEE_BROWSE_APPS);
    g_windowManager.Remove(WINDOW_BOXEE_BROWSE_TVEPISODES);
    g_windowManager.Remove(WINDOW_BOXEE_BROWSE_ALBUMS);
    g_windowManager.Remove(WINDOW_BOXEE_BROWSE_TRACKS);
    g_windowManager.Remove(WINDOW_BOXEE_BROWSE_REPOSITORIES);
    //g_windowManager.Remove(WINDOW_BOXEE_BROWSE_APPBOX);
    g_windowManager.Remove(WINDOW_BOXEE_BROWSE_SIMPLE_APP);
    g_windowManager.Remove(WINDOW_BOXEE_BROWSE_PRODUCT);
    g_windowManager.Remove(WINDOW_BOXEE_BROWSE_QUEUE);
    g_windowManager.Remove(WINDOW_BOXEE_BROWSE_DISCOVER);
    g_windowManager.Remove(WINDOW_BOXEE_BROWSE_HISTORY);
    g_windowManager.Remove(WINDOW_BOXEE_BROWSE_PHOTOS);
    g_windowManager.Delete(WINDOW_BOXEE_MAIN);
    g_windowManager.Delete(WINDOW_BOXEE_DIALOG_MAIN_MENU);
    g_windowManager.Delete(WINDOW_BOXEE_LIVETV);
    //g_windowManager.Delete(WINDOW_BOXEE_DIALOG_OPTIONS_MENU); // deprecated
    g_windowManager.Delete(WINDOW_BOXEE_MEDIA_INFO);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_MANUAL_RESOLVE);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_MANUAL_MOVIE);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_MANUAL_RESOLVE_ALBUM);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_MANUAL_RESOLVE_AUDIO);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_MANUAL_EPISODE);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_MANUAL_DETAILS);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_MANUAL_ADD_FILES);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_MANUAL_PART_ACTION);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_BROWSE_MENU);
//    g_windowManager.Delete(WINDOW_BOXEE_DIALOG_SERIES_INPUT);
    g_windowManager.Delete(WINDOW_BOXEE_ALBUM_INFO);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_RSS_FEED_INFO);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_VIDEO_QUALITY);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_VIDEO_RESUME);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_PAYMENT_PRODUCTS);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_PAYMENT_TOU);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_PAYMENT_USERDATA);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_PAYMENT_OK_PLAY);
    g_windowManager.Delete(WINDOW_DIALOG_BOXEE_PAYMENT_WAIT_FOR_SERVER_APPROVAL);
// end Boxee section

    CLog::Log(LOGNOTICE, "unload sections");
    CSectionLoader::UnloadAll();

#ifdef HAS_PERFORMANCE_SAMPLE
    CLog::Log(LOGNOTICE, "performance statistics");
    m_perfStats.DumpStats();
#endif

    //  Shutdown as much as possible of the
    //  application, to reduce the leaks dumped
    //  to the vc output window before calling
    //  _CrtDumpMemoryLeaks(). Most of the leaks
    //  shown are no real leaks, as parts of the app
    //  are still allocated.

    g_localizeStrings.Clear();
    g_LangCodeExpander.Clear();
    g_charsetConverter.clear();
    g_directoryCache.Clear();
    CButtonTranslator::GetInstance().Clear();
#ifdef HAS_LASTFM
    CLastfmScrobbler::RemoveInstance();
    CLibrefmScrobbler::RemoveInstance();
    CLastFmManager::RemoveInstance();
#endif
#ifdef HAS_EVENT_SERVER
    CEventServer::RemoveInstance();
#endif
#ifdef HAS_DBUS_SERVER
    CDbusServer::RemoveInstance();
#endif
    DllLoaderContainer::Clear();
    g_playlistPlayer.Clear();
    g_settings.Clear();
    g_guiSettings.Clear();
    g_advancedSettings.Clear();
    g_Mouse.Cleanup();

#ifdef _LINUX
    CXHandle::DumpObjectTracker();
#endif

#ifdef _CRTDBG_MAP_ALLOC
    _CrtDumpMemoryLeaks();
    while(1); // execution ends
#endif
    return S_OK;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CApplication::Cleanup()");
    return E_FAIL;
  }
}

void CApplication::Stop()
{  
  try
  {
    ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AF_System, "xbmc", "ApplicationStop");

    BOXEE::Boxee::GetInstance().SignalStop();

#ifdef HAS_WEB_SERVER
    if (m_pXbmcHttp)
    {
      if(g_stSettings.m_HttpApiBroadcastLevel>=1)
        getApplicationMessenger().HttpApi("broadcastlevel; ShutDown;1");

      m_pXbmcHttp->shuttingDown=true;
    }
#endif

    if( m_bSystemScreenSaverEnable )
      g_Windowing.EnableSystemScreenSaver(true);

    CLog::Log(LOGNOTICE, "Storing total System Uptime");
    g_stSettings.m_iSystemTimeTotalUp = g_stSettings.m_iSystemTimeTotalUp + (int)(CTimeUtils::GetFrameTime() / 60000);

    // Update the settings information (volume, uptime etc. need saving)
    if (CFile::Exists(g_settings.GetSettingsFile()))
    {
      CLog::Log(LOGNOTICE, "Saving settings");
      g_settings.Save();
    }
    else
      CLog::Log(LOGNOTICE, "Not saving settings (settings.xml is not present)");

    if (m_pPlayer)
    {
      CLog::Log(LOGNOTICE, "stopping player");
      CGUIWindowVisualisation *vis = (CGUIWindowVisualisation*)g_windowManager.GetWindow(WINDOW_VISUALISATION);
      if (vis)
        vis->FreeResources(true);
      // this will take care of deleting the player
      CGUIMessage msg( GUI_MSG_PLAYBACK_STOPPED, 0, 0 );
      g_windowManager.SendMessage(msg);
    }

    m_bStop = true;
    BOXEE::Boxee::GetInstance().GetMetadataEngine().Stop();

    class CTriggerStopDirectories : public CallbackTrigger
    {
      virtual void HandleListener(BoxeeListener *pListener)
      {
        if (pListener)
        {
          pListener->OnForcedStop();
        }
      }
    };

    CTriggerStopDirectories trigger;

    BOXEE::Boxee::GetInstance().TriggerAllListeners(&trigger);

    CLog::Log(LOGNOTICE, "stop all");

    // stop scanning before we kill the network and so on
    CGUIDialogMusicScan *musicScan = (CGUIDialogMusicScan *)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
    if (musicScan)
      musicScan->StopScanning();

    CGUIDialogVideoScan *videoScan = (CGUIDialogVideoScan *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
    if (videoScan)
      videoScan->StopScanning();

    StopServices();

    m_playScheduler.Stop();

#ifdef __APPLE__
    g_xbmcHelper.ReleaseAllInput();
#endif

#if HAS_FILESYTEM_DAAP
    CLog::Log(LOGNOTICE, "stop daap clients");
    g_DaapClient.Release();
#endif


#ifdef HAS_FILESYSTEM_SAP
    CLog::Log(LOGNOTICE, "stop sap announcement listener");
    g_sapsessions.StopThread();
#endif
#ifdef HAS_ZEROCONF
    if(CZeroconfBrowser::IsInstantiated())
    {
      CLog::Log(LOGNOTICE, "stop zeroconf browser");
      CZeroconfBrowser::GetInstance()->Stop();
      CZeroconfBrowser::ReleaseInstance();
    }
#endif
    getApplicationMessenger().Cleanup();

    CLog::Log(LOGNOTICE, "clean cached files!");
    g_RarManager.ClearCache(true);

    CLog::Log(LOGNOTICE, "unload skin");
    UnloadSkin();
    
#ifdef __APPLE__
    if (g_xbmcHelper.IsAlwaysOn() == false)
      g_xbmcHelper.Stop();
#endif
    
    /* Python resource freeing must be done after skin has been unloaded, not before
     some windows still need it when deinitializing during skin unloading. */
#ifdef HAS_PYTHON
    CLog::Log(LOGNOTICE, "stop python");
    g_pythonParser.FreeResources();
#endif
#ifdef HAS_LCD
    if (g_lcd)
    {
      g_lcd->Stop();
      delete g_lcd;
      g_lcd=NULL;
    }
#endif

#ifdef HAS_HAL
    g_HalManager.Stop();
#endif

    CGUISoundPlayer::GetInstance().Deinitialize();

//Boxee
    CLog::Log(LOGNOTICE, "Application: delete item loader...");
    if (m_ItemLoader)
      delete m_ItemLoader;
    m_ItemLoader = NULL;
    
    BoxeeUserLogoutAction();
    
#ifdef HAS_INTEL_SMD
    g_IntelSMDGlobals.DeInitAudio();
#endif

    // unload boxee
    CLog::Log(LOGNOTICE, "stopping boxee...");
    BOXEE::Boxee::GetInstance().Stop();

    if (m_pPythonManager)
      delete m_pPythonManager;
    m_pPythonManager = NULL;

    for (size_t i=0; i<m_lockedLibraries.size(); i++)
    {
      m_lockedLibraries[i]->Unload();
      delete m_lockedLibraries[i];      
    }
    
    m_lockedLibraries.clear();

    CDirectory::s_thumbCreatorProcess.SignalStop();
    if (CDirectory::s_thumbCreatorProcess.IsRunning())
    {    
      CLog::Log(LOGNOTICE, "Stopping DirectoryThumbCreator Processor");
      CDirectory::s_thumbCreatorProcess.Stop();
    }

    CMetadataResolverVideo::DeinitializeVideoResolver();

#if 0
    if (m_httpServer)
    {
      CLog::Log(LOGDEBUG,"CApplication::Stop - going to STOP HTTP server (hsrv)");
      m_httpServer->Stop();
    }
#endif

//end Boxee

    CZeroconfBrowser::GetInstance()->Stop();

#if defined(_LINUX) && defined(HAS_FILESYSTEM_SMB)
    smb.Deinit();
#endif

    g_directoryCache.Clear();
    
    m_httpCache.Deinitialize();
    
    m_tumbnailsMgr.Deinitialize();

    m_dbConnectionPool.Cleanup();

	// Premature clearing of settings causes problems
	// in case some of them are required during cleanup stage
    //g_settings.Clear();
    //g_guiSettings.Clear();


    CLog::Log(LOGNOTICE, "stopped");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CApplication::Stop()");
  }

  // we may not get to finish the run cycle but exit immediately after a call to g_application.Stop()
  // so we may never get to Destroy() in CXBApplicationEx::Run(), we call it here.

  g_Windowing.DestroyRenderSystem();
  Destroy();
}

bool CApplication::PlayMedia(const CFileItem& item, int iPlaylist)
{
  CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  pDlgProgress->SetHeading("");
  pDlgProgress->SetLine(0, "Starting media playback...");
  pDlgProgress->SetLine(1, "");
  pDlgProgress->SetLine(2, "");
  pDlgProgress->StartModal("CApplication::PlayMedia");
  pDlgProgress->Progress();

  // if the GUI thread is creating the player sthen we do it in background in order not to block the gui
  if (IsCurrentThread())
  {
    CBackgroundPlayer *pBGPlayer = new CBackgroundPlayer(item, iPlaylist);
    
    m_playScheduler.Schedule(pBGPlayer);
    //pBGPlayer->Create(true); // will delete itself when done
  }
  else 
    return PlayMediaSync(item, iPlaylist);

  return true;
}

bool CApplication::PlayMediaSync(const CFileItem& item, int iPlaylist)
{
#ifdef HAS_LASTFM
  if (item.IsLastFM())
  {
#ifndef _BOXEE_
    g_partyModeManager.Disable();
#endif
    return CLastFmManager::GetInstance()->ChangeStation(item.GetAsUrl());
  }
#endif
  if (item.IsSmartPlayList())
  {
    CDirectory dir;
    CFileItemList items;
    if (dir.GetDirectory(item.m_strPath, items) && items.Size())
    {
      CSmartPlaylist smartpl;
      //get name and type of smartplaylist, this will always succeed as GetDirectory also did this.
      smartpl.OpenAndReadName(item.m_strPath);
      CPlayList playlist;
      playlist.Add(items);
      return ProcessAndStartPlaylist(smartpl.GetName(), playlist, (smartpl.GetType() == "songs" || smartpl.GetType() == "albums") ? PLAYLIST_MUSIC:PLAYLIST_VIDEO);
    }
  }
//  else if (item.IsInternetStream())
//  {
//
//  }
  else if (item.IsPlayList() /*|| item.IsInternetStream()*/)
  {
    CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (item.IsInternetStream() && dlgProgress)
    {
       dlgProgress->ShowProgressBar(false);
       dlgProgress->SetHeading(260);
       dlgProgress->SetLine(0, 14003);
       dlgProgress->SetLine(1, "");
       dlgProgress->SetLine(2, "");
       //dlgProgress->StartModal("CApplication::PlayMediaSync");
    }

    //is or could be a playlist
    auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(item));
    bool gotPlayList = (pPlayList.get() && pPlayList->Load(item.m_strPath));
    if (item.IsInternetStream() && dlgProgress)
    {
       dlgProgress->Close();
       if (dlgProgress->IsCanceled())
          return true;
    }

    if (gotPlayList)
    {
      if (iPlaylist != PLAYLIST_NONE)
      {
        bool bRc = ProcessAndStartPlaylist(item.m_strPath, *pPlayList, iPlaylist);
        dlgProgress->Close();
        return bRc;
      }
      else
      {
        dlgProgress->Close();
        CLog::Log(LOGWARNING, "CApplication::PlayMedia called to play a playlist %s but no idea which playlist to use, playing first item", item.m_strPath.c_str());
        if(pPlayList->size())
        {
          return PlayFile(*(*pPlayList)[0], false);
      }
    }
    }
    else
      dlgProgress->Close();
  }

  //nothing special just play
  return PlayFile(item, false);
}

// PlayStack()
// For playing a multi-file video.  Particularly inefficient
// on startup, as we are required to calculate the length
// of each video, so we open + close each one in turn.
// A faster calculation of video time would improve this
// substantially.
bool CApplication::PlayStack(const CFileItem& item, bool bRestart)
{
  if (!item.IsStack())
    return false;
  
  // see if we have the info in the database
  // TODO: If user changes the time speed (FPS via framerate conversion stuff)
  //       then these times will be wrong.
  //       Also, this is really just a hack for the slow load up times we have
  //       A much better solution is a fast reader of FPS and fileLength
  //       that we can use on a file to get it's time.
  vector<int> times;
  bool haveTimes(false);
  CVideoDatabase dbs;
  if (dbs.Open())
  {
    dbs.GetVideoSettings(item.m_strPath, g_stSettings.m_currentVideoSettings);
    haveTimes = dbs.GetStackTimes(item.m_strPath, times);
    dbs.Close();
  }


  // calculate the total time of the stack
  CStackDirectory dir;
  dir.GetDirectory(item.m_strPath, *m_currentStack);

  long totalTime = 0;
  for (int i = 0; i < m_currentStack->Size(); i++)
  {
    //Boxee

    ////////////////////////////////////////////////////////////////////////////////////////////
    // set relevant properties from original item in the stack item for correct server report //
    ////////////////////////////////////////////////////////////////////////////////////////////

    BoxeeUtils::UpdateStackFileForServerReport(item, *(m_currentStack->Get(i)));

    //end Boxee

    if (haveTimes)
    {
      CFileItemPtr file = m_currentStack->Get(i);
      file->m_lEndOffset = times[i];
      //(*m_currentStack)[i]->m_lEndOffset = times[i];
    }
    else
    {
      int duration = 0;
      CStdString strPath = (*m_currentStack)[i]->m_strPath;
      if (CDVDFileInfo::GetFileDuration(strPath, duration))
      {
      totalTime += duration / 1000;
      (*m_currentStack)[i]->m_lEndOffset = totalTime;
      times.push_back(totalTime);
    }
      else
      {
        times.push_back(0);
        //m_currentStack->Clear();
        //return false;
  }
    }
  }

  double seconds = item.m_lStartOffset / 75.0;

  if (!haveTimes || item.m_lStartOffset == STARTOFFSET_RESUME )
  {  // have our times now, so update the dB
    if (dbs.Open())
    {
      if( !haveTimes )
        dbs.SetStackTimes(item.m_strPath, times);

      if( item.m_lStartOffset == STARTOFFSET_RESUME )
      {
        // can only resume seek here, not dvdstate
        CBookmark bookmark;
        if( dbs.GetResumeBookMark(item.m_strPath, bookmark) )
          seconds = bookmark.timeInSeconds;
        else
          seconds = 0.0f;
      }
      dbs.Close();
    }
  }

  *m_itemCurrentFile = item;
  m_currentStackPosition = 0;
  m_eCurrentPlayer = EPC_NONE; // must be reset on initial play otherwise last player will be used 

  if (seconds > 0)
  {
    // work out where to seek to
    for (int i = 0; i < m_currentStack->Size(); i++)
    {
      if (seconds < (*m_currentStack)[i]->m_lEndOffset)
      {
        CFileItem item(*(*m_currentStack)[i]);
        long start = (i > 0) ? (*m_currentStack)[i-1]->m_lEndOffset : 0;
        item.m_lStartOffset = (long)(seconds - start) * 75;
        m_currentStackPosition = i;
        return PlayFile(item, true);
      }
    }
  }

  return PlayFile(*(*m_currentStack)[0], true);
}

bool CApplication::PlayFile(const CFileItem& item, bool bRestart)
{
  if (m_pPlayer && m_itemCurrentFile && m_itemCurrentFile->GetPropertyBOOL("WaitPlaybackPath"))
  {
    CPlayerOptions options;
    m_itemCurrentFile->SetProperty("ActualPlaybackPath", item.m_strPath);
    bool bOk = m_pPlayer->OpenFile(*m_itemCurrentFile, options);
    m_itemCurrentFile->ClearProperty("ActualPlaybackPath");
#ifdef HAS_EMBEDDED
    HandlePlayerErrors();
#endif
    return bOk;
  }
  
  if (item.IsApp()) 
  {
    return CAppManager::GetInstance().Launch(item);
  }
  
  if (!bRestart)
  {
    SaveCurrentFileSettings();

    OutputDebugString("new file set audiostream:0\n");
    // Switch to default options
    g_stSettings.m_currentVideoSettings = g_stSettings.m_defaultVideoSettings;
    // see if we have saved options in the database

    m_iPlaySpeed = 1;
    *m_itemCurrentFile = item;
//Boxee
    BoxeeUtils::FillVideoItemDetails(m_itemCurrentFile);
//end Boxee
    m_nextPlaylistItem = -1;
    m_currentStackPosition = 0;
    m_currentStack->Clear();
  }

  if (item.IsPlayList())
    return false;

  if (item.IsPlugin())
  { // we modify the item so that it becomes a real URL
    CFileItem item_new;
    if (DIRECTORY::CPluginDirectory::GetPluginResult(item.m_strPath, item_new))
      return PlayFile(item_new, false);
    return false;
  }

#ifndef HAS_UPNP_AV
  if (CUtil::IsUPnP(item.m_strPath))
  {
    CFileItem item_new(item);
    if (DIRECTORY::CUPnPDirectory::GetResource(item.m_strPath, item_new))
    {
      item_new.SetProperty("IsUPnP", true);
      return PlayFile(item_new, false);
    }
    return false;
  }
#endif

  // if we have a stacked set of files, we need to setup our stack routines for
  // "seamless" seeking and total time of the movie etc.
  // will recall with restart set to true
  if (item.IsStack())
    return PlayStack(item, bRestart);

#ifdef HAS_FILESYSTEM_TUXBOX
  //Is TuxBox, this should probably be moved to CFileTuxBox
  if(item.IsTuxBox())
  {
    CLog::Log(LOGDEBUG, "%s - TuxBox URL Detected %s",__FUNCTION__, item.m_strPath.c_str());

    if(g_tuxboxService.IsRunning())
      g_tuxboxService.Stop();

    CFileItem item_new;
    if(g_tuxbox.CreateNewItem(item, item_new))
    {

      // Make sure it doesn't have a player
      // so we actually select one normally
      m_eCurrentPlayer = EPC_NONE;

      // keep the tuxbox:// url as playing url
      // and give the new url to the player
      if(PlayFile(item_new, true))
      {
        if(!g_tuxboxService.IsRunning())
          g_tuxboxService.Start();
        return true;
      }
    }
    return false;
  }
#endif

  CPlayerOptions options;

  if (item.HasProperty("StartPercent"))
  {
    options.startpercent = item.GetPropertyDouble("StartPercent");
  }

  PLAYERCOREID eNewCore = EPC_NONE;
  if( bRestart )
  {
    // have to be set here due to playstack using this for starting the file
    options.starttime = item.m_lStartOffset / 75.0;
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0 && m_itemCurrentFile->m_lStartOffset != 0)
      m_itemCurrentFile->m_lStartOffset = STARTOFFSET_RESUME; // to force fullscreen switching

    if( m_eForcedNextPlayer != EPC_NONE )
      eNewCore = m_eForcedNextPlayer;
    else if( m_eCurrentPlayer == EPC_NONE )
      eNewCore = CPlayerCoreFactory::GetDefaultPlayer(item);
    else
      eNewCore = m_eCurrentPlayer;
  }
  else
  {
    options.starttime = item.m_lStartOffset / 75.0;

    if (item.IsVideo())
    {
      // open the d/b and retrieve the bookmarks for the current movie
      CVideoDatabase dbs;
      dbs.Open();
      dbs.GetVideoSettings(item.m_strPath, g_stSettings.m_currentVideoSettings);

      if( item.m_lStartOffset == STARTOFFSET_RESUME )
      {
        options.starttime = 0.0f;
        CBookmark bookmark;
        if(dbs.GetResumeBookMark(item.m_strPath, bookmark))
        {
          options.starttime = bookmark.timeInSeconds;
          options.state = bookmark.playerState;
        }
      }
      else if (item.HasVideoInfoTag())
      {
        const CVideoInfoTag *tag = item.GetVideoInfoTag();

        if (tag->m_iBookmarkId != -1 && tag->m_iBookmarkId != 0)
        {
          CBookmark bookmark;
          dbs.GetBookMarkForEpisode(*tag, bookmark);
          options.starttime = bookmark.timeInSeconds;
          options.state = bookmark.playerState;
        }
      }

      dbs.Close();
    }

    if (m_eForcedNextPlayer != EPC_NONE)
      eNewCore = m_eForcedNextPlayer;
    else
      eNewCore = CPlayerCoreFactory::GetDefaultPlayer(item);
  }

  // this really aught to be inside !bRestart, but since PlayStack
  // uses that to init playback, we have to keep it outside
  int playlist = g_playlistPlayer.GetCurrentPlaylist();
  if (playlist == PLAYLIST_VIDEO && g_playlistPlayer.GetPlaylist(playlist).size() > 1)
  { // playing from a playlist by the looks
    // don't switch to fullscreen if we are not playing the first item...
    options.fullscreen = !g_playlistPlayer.HasPlayedFirstFile() && g_advancedSettings.m_fullScreenOnMovieStart && !g_stSettings.m_bStartVideoWindowed && !item.GetPropertyBOOL("DisableFullScreen");
  }
  else if(m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
  {
    // TODO - this will fail if user seeks back to first file in stack
    if(m_currentStackPosition == 0 || m_itemCurrentFile->m_lStartOffset == STARTOFFSET_RESUME)
      options.fullscreen = g_advancedSettings.m_fullScreenOnMovieStart && !g_stSettings.m_bStartVideoWindowed && !item.GetPropertyBOOL("DisableFullScreen");
    else
      options.fullscreen = false;
    // reset this so we don't think we are resuming on seek
    m_itemCurrentFile->m_lStartOffset = 0;
  }
  else
    options.fullscreen = g_advancedSettings.m_fullScreenOnMovieStart && !g_stSettings.m_bStartVideoWindowed && !item.GetPropertyBOOL("DisableFullScreen");

  // reset m_bStartVideoWindowed as it's a temp setting
  g_stSettings.m_bStartVideoWindowed = false;
  // reset any forced player
  m_eForcedNextPlayer = EPC_NONE;

#ifdef HAS_KARAOKE
  //We have to stop parsing a cdg before mplayer is deallocated
  // WHY do we have to do this????
  if (m_pKaraokeMgr)
    m_pKaraokeMgr->Stop();
#endif

  // tell system we are starting a file
  m_bPlaybackStarting = true;

  IPlayer *pKeepPlayer = NULL;
  // We should restart the player, unless the previous and next tracks are using
  // one of the players that allows gapless playback (paplayer, dvdplayer)
  if (m_pPlayer)
  {
    if ( !(m_eCurrentPlayer == eNewCore && (m_eCurrentPlayer == EPC_DVDPLAYER || m_eCurrentPlayer  == EPC_PAPLAYER)) )
    {
      pKeepPlayer = m_pPlayer;
      m_pPlayer = NULL;
    }
  }

  if (!m_pPlayer)
  {
    m_eCurrentPlayer = eNewCore;
    m_pPlayer = CPlayerCoreFactory::CreatePlayer(eNewCore, *this);
  }

  if (pKeepPlayer)
  {
    ThreadMessage msg;
    msg.lpVoid = pKeepPlayer;
    msg.dwMessage = TMSG_DELETE_PLAYER;
    m_applicationMessenger.SendMessage(msg,false);
  }
  
  // Workaround for bug/quirk in SDL_Mixer on OSX.
  // TODO: Remove after GUI Sounds redux
#if defined(__APPLE__)
    g_audioManager.Enable(false);
#endif

  bool bResult;
  if (m_pPlayer)
  {
    // don't hold graphicscontext here since player
    // may wait on another thread, that requires gfx
    CSingleExit ex(g_graphicsContext);
    bResult = m_pPlayer->OpenFile(item, options);

    if(!bResult && CUtil::IsSmb(item.m_strPath))
    {
      if(!g_guiSettings.GetString("smb.username").IsEmpty())
      {
        CFileItem tmpItem = item;
        CURI url(tmpItem.m_strPath);
        url.SetUserName(g_guiSettings.GetString("smb.username"));
        url.SetPassword(g_guiSettings.GetString("smb.password"));
        tmpItem.m_strPath = url.Get();

        m_eCurrentPlayer = eNewCore;
        m_pPlayer = CPlayerCoreFactory::CreatePlayer(eNewCore, *this);
        bResult = m_pPlayer->OpenFile(tmpItem, options);
      }
    }
  }
  else
  {
    CLog::Log(LOGERROR, "Error creating player for item %s (File doesn't exist?)", item.m_strPath.c_str());
    bResult = false;
  }
  
  if(bResult)
  {
    // boxee section
    // report to boxee server about the item being played
    
    bool dontReport = item.GetPropertyBOOL("dont-report");
    
    if(dontReport)
    {
      CLog::Log(LOGDEBUG,"CApplication::PlayFile - Item [label=%s] ISN'T going to be reported to the server because its property value is [dont-report=%d=%s] (rts)",(item.GetLabel()).c_str(),dontReport,(item.GetProperty("dont-report")).c_str());
    }
    else
    {
      BOXEE::BXObject playObj;
      if (item.HasExternlFileItem())
      {
        playObj = BoxeeUtils::FileItemToObject(item.GetExternalFileItem().get());
      }
      else
      {
        playObj = BoxeeUtils::FileItemToObject(&item);
      }
       
      if (playObj.IsValid())
      {
        // For debug //
        BoxeeUtils::LogReportToServerAction(item,playObj,CReportToServerActionType::PLAY);
        ///////////////          
        
        CLog::Log(LOGDEBUG, "Report PLAYBACK %s", item.m_strPath.c_str());
        BOXEE::BXGeneralMessage action;
        if (item.IsFlash())
        {
          action.SetMessageType(MSG_ACTION_TYPE_WATCH);
        }
        else if (item.IsAudio())
        {
          action.SetMessageType(MSG_ACTION_TYPE_LISTEN);
        }
        else
        {
          action.SetMessageType(MSG_ACTION_TYPE_WATCH);
        }
        
        // In case there is "referral" property -> Report it to the server
        if(item.HasProperty("referral"))
        {
          action.SetReferral(item.GetProperty("referral"));
          
          CLog::Log(LOGDEBUG,"CApplication::PlayFile - For item [label=%s][referral=%s] Message was set with [referral=%s] (rts)(referral)",(item.GetLabel()).c_str(),(item.GetProperty("referral")).c_str(),(action.GetReferral()).c_str());
        }

        action.SetTimestamp(time(NULL));
        action.AddObject(playObj);
        BOXEE::Boxee::GetInstance().SetUserAction_bg(action);
      }
    }
    
    // end boxee section

    if (m_iPlaySpeed != 1)
    {
      int iSpeed = m_iPlaySpeed;
      m_iPlaySpeed = 1;
      SetPlaySpeed(iSpeed);
    }

#ifdef HAS_VIDEO_PLAYBACK
    if( IsPlayingVideo() )
    {
      // if player didn't manange to switch to fullscreen by itself do it here
      if( options.fullscreen && g_renderManager.IsStarted()
       && g_windowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO )
       SwitchToFullScreen();

      // Save information about the stream if we currently have no data
      if (item.HasVideoInfoTag())
      {
        CVideoInfoTag *details = m_itemCurrentFile->GetVideoInfoTag();
        if (!details->HasStreamDetails())
        {
          if (m_pPlayer->GetStreamDetails(details->m_streamDetails) && details->HasStreamDetails())
          {
            CVideoDatabase dbs;
            dbs.Open();
            dbs.SetStreamDetailsForFileId(details->m_streamDetails, details->m_iFileId);
            dbs.Close();
            CUtil::DeleteVideoDatabaseDirectoryCache();
          }
        }
      }
    }
#endif

#if !defined(__APPLE__)
      g_audioManager.Enable(false);
#endif
  }
  m_bPlaybackStarting = false;
  if(bResult)
  {
    // we must have started, otherwise player might send this later
    if(IsPlaying())
      OnPlayBackStarted();
  }
  else
  {
    // we send this if it isn't playlistplayer that is doing this
    const CPlayList& playlist = g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist());
    int next = g_playlistPlayer.GetNextSong();
    int size = playlist.size();
    if(next < 0 
    || next >= size)
      OnPlayBackStopped();
  }

#ifdef HAS_EMBEDDED
  HandlePlayerErrors();
#endif

  return bResult;
}

void CApplication::OnPlayBackEnded(bool bError, const CStdString& error)
{
  CGUIDialogProgress::EndProgressOperation("CApplication::PlayMedia");
  
  GetItemLoader().Resume();
  BOXEE::Boxee::GetInstance().GetMetadataEngine().Resume();
  m_FileScanner.Resume();
#ifdef HAS_DVB
  CAppManager::GetInstance().OnPlayBackEnded();
#endif

  if(m_bPlaybackStarting)
    return;

  // informs python script currently running playback has ended
  // (does nothing if python is not loaded)
#ifdef HAS_PYTHON
  g_pythonParser.OnPlayBackEnded();
#endif

#ifdef HAS_WEB_SERVER
  // Let's tell the outside world as well
  if (m_pXbmcHttp && g_stSettings.m_HttpApiBroadcastLevel>=1)
    getApplicationMessenger().HttpApi("broadcastlevel; OnPlayBackEnded;1");
#endif

  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AF_Playback, "xbmc", "PlaybackEnded");

  CLog::Log(LOGDEBUG,"CApplication::OnPlayBackEnded - playback has finished. [berr=%d][err=%s]",bError,error.c_str());

  CGUIMessage msg(GUI_MSG_PLAYBACK_ENDED, 0, 0);
  msg.SetParam1(bError);
  msg.SetLabel(error);
  g_windowManager.SendThreadMessage(msg);

  g_infoManager.SetShowAudioCodecLogo(false);
}

void CApplication::OnPlayBackStarted()
{
  if(!IsPlayingAudio())
  {
    GetItemLoader().Pause(true);
  }

  BOXEE::Boxee::GetInstance().GetMetadataEngine().Pause();
  m_FileScanner.Pause();

  CGUIDialogProgress::EndProgressOperation("CApplication::PlayMedia");

  // Reset to the claimed video FPS.
  g_infoManager.ResetFPS(m_pPlayer->GetActualFPS());
  
  CAppManager::GetInstance().OnPlayBackStarted();
  
  if(m_bPlaybackStarting)
    return;

#ifdef HAS_PYTHON
  // informs python script currently running playback has started
  // (does nothing if python is not loaded)
  g_pythonParser.OnPlayBackStarted();
#endif

#ifdef HAS_WEB_SERVER
  // Let's tell the outside world as well
  if (m_pXbmcHttp && g_stSettings.m_HttpApiBroadcastLevel>=1)
    getApplicationMessenger().HttpApi("broadcastlevel; OnPlayBackStarted;1");
#endif

  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AF_Playback, "xbmc", "PlaybackStarted");

  CLog::Log(LOGDEBUG, "%s - Playback has started", __FUNCTION__);

  CGUIMessage msg(GUI_MSG_PLAYBACK_STARTED, 0, 0);
  g_windowManager.SendThreadMessage(msg);

  // 260911 - disable show AudioCodec logo
  g_infoManager.SetShowAudioCodecLogo(false);
}

void CApplication::StackedMovieBackSeekHandler(__int64 seek)
{
  if (PlayingStackedMovie() && m_currentStackPosition > 0)
  { 
    CFileItem& item = *(*m_currentStack)[--m_currentStackPosition];
    
    PlayFile(item, true);
    SeekTime(item.m_lEndOffset + seek/1000);
  }
}

bool CApplication::PlayingStackedMovie()
{
  return m_itemCurrentFile && m_itemCurrentFile->IsStack() && m_currentStack && m_currentStack->Size() > 0;
}

void CApplication::OnQueueNextItem()
{
  
  CAppManager::GetInstance().OnQueueNextItem();

  // informs python script currently running that we are requesting the next track
  // (does nothing if python is not loaded)
#ifdef HAS_PYTHON
  g_pythonParser.OnQueueNextItem(); // currently unimplemented
#endif

#ifdef HAS_WEB_SERVER
  // Let's tell the outside world as well
  if (m_pXbmcHttp && g_stSettings.m_HttpApiBroadcastLevel>=1)
    getApplicationMessenger().HttpApi("broadcastlevel; OnQueueNextItem;1");
#endif

  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AF_Playback, "xbmc", "QueueNextItem");

  CLog::Log(LOGDEBUG, "Player has asked for the next item");

  if(IsPlayingAudio())
  {
#ifdef HAS_LASTFM
    CLastfmScrobbler::GetInstance()->SubmitQueue();
    CLibrefmScrobbler::GetInstance()->SubmitQueue();
#endif
  }

  CGUIMessage msg(GUI_MSG_QUEUE_NEXT_ITEM, 0, 0);
  g_windowManager.SendThreadMessage(msg);
}

void CApplication::OnPlayBackStopped()
{
  CGUIDialogProgress::EndProgressOperation("CApplication::PlayMedia");

  GetItemLoader().Resume();
  BOXEE::Boxee::GetInstance().GetMetadataEngine().Resume();

  m_FileScanner.Resume();

  // Reset FPS to the display FPS. 
  g_infoManager.ResetFPS(g_graphicsContext.GetFPS());
  
  CAppManager::GetInstance().OnPlayBackStopped();
  
  if(m_bPlaybackStarting)
    return;

  // informs python script currently running playback has ended
  // (does nothing if python is not loaded)
#ifdef HAS_PYTHON
  g_pythonParser.OnPlayBackStopped();
#endif

#ifdef HAS_WEB_SERVER
  // Let's tell the outside world as well
  if (m_pXbmcHttp && g_stSettings.m_HttpApiBroadcastLevel>=1)
    getApplicationMessenger().HttpApi("broadcastlevel; OnPlayBackStopped;1");
#endif

  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AF_Playback, "xbmc", "PlaybackStopped", m_itemCurrentFile);

#ifdef HAS_LASTFM
  CLastfmScrobbler::GetInstance()->SubmitQueue();
  CLibrefmScrobbler::GetInstance()->SubmitQueue();
#endif

  CLog::Log(LOGDEBUG, "%s - Playback was stopped", __FUNCTION__);

  CGUIMessage msg( GUI_MSG_PLAYBACK_STOPPED, 0, 0 );
  g_windowManager.SendThreadMessage(msg);
}

void CApplication::OnPlayBackPaused()
{
#ifdef HAS_PYTHON
  g_pythonParser.OnPlayBackPaused();
#endif

#ifdef HAS_HTTPAPI
  // Let's tell the outside world as well
  if (g_settings.m_HttpApiBroadcastLevel>=1)
    getApplicationMessenger().HttpApi("broadcastlevel; OnPlayBackPaused;1");
#endif

  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AF_Playback, "xbmc", "PlaybackPaused", m_itemCurrentFile);
}

void CApplication::OnPlayBackResumed()
{
#ifdef HAS_PYTHON
  g_pythonParser.OnPlayBackResumed();
#endif

#ifdef HAS_HTTPAPI
  // Let's tell the outside world as well
  if (g_settings.m_HttpApiBroadcastLevel>=1)
    getApplicationMessenger().HttpApi("broadcastlevel; OnPlayBackResumed;1");
#endif

  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AF_Playback, "xbmc", "PlaybackResumed", m_itemCurrentFile);
}

void CApplication::OnPlayBackSpeedChanged(int iSpeed)
{
#ifdef HAS_PYTHON
  g_pythonParser.OnPlayBackSpeedChanged(iSpeed);
#endif

#ifdef HAS_HTTPAPI
  // Let's tell the outside world as well
  if (g_settings.m_HttpApiBroadcastLevel>=1)
  {
    CStdString tmp;
    tmp.Format("broadcastlevel; OnPlayBackSpeedChanged:%i;1",iSpeed);
    getApplicationMessenger().HttpApi(tmp);
  }
#endif

  CVariant param;
  param["speed"] = iSpeed;
  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AF_Playback, "xbmc", "PlaybackSpeedChanged", m_itemCurrentFile, param);
}

void CApplication::OnPlayBackSeek(int iTime, int seekOffset)
{
#ifdef HAS_PYTHON
  g_pythonParser.OnPlayBackSeek(iTime, seekOffset);
#endif

  CAppManager::GetInstance().OnPlaybackSeek(iTime);

#ifdef HAS_HTTPAPI
  // Let's tell the outside world as well
  if (g_settings.m_HttpApiBroadcastLevel>=1)
  {
    CStdString tmp;
    tmp.Format("broadcastlevel; OnPlayBackSeek:%i;1",iTime);
    getApplicationMessenger().HttpApi(tmp);
  }
#endif

  CVariant param;
  param["time"] = iTime;
  param["seekoffset"] = seekOffset;
  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AF_Playback, "xbmc", "PlaybackSeek", param);
  g_infoManager.SetDisplayAfterSeek(2500, seekOffset/1000);
}

void CApplication::OnPlayBackSeekChapter(int iChapter)
{
#ifdef HAS_PYTHON
  g_pythonParser.OnPlayBackSeekChapter(iChapter);
#endif

#ifdef HAS_HTTPAPI
  // Let's tell the outside world as well
  if (g_settings.m_HttpApiBroadcastLevel>=1)
  {
    CStdString tmp;
    tmp.Format("broadcastlevel; OnPlayBackSkeekChapter:%i;1",iChapter);
    getApplicationMessenger().HttpApi(tmp);
  }
#endif

  CVariant param;
  param["chapter"] = iChapter;
  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AF_Playback, "xbmc", "PlaybackSeekChapter", param);
}

bool CApplication::IsPlaying() const
{
  if (!m_pPlayer)
    return false;
  if (!m_pPlayer->IsPlaying())
    return false;
  return true;
}

bool CApplication::IsPaused() const
{
  if (!m_pPlayer)
    return false;
  if (!m_pPlayer->IsPlaying())
    return false;
  return m_pPlayer->IsPaused();
}

bool CApplication::IsPlayingAudio() const
{
  if (!m_pPlayer)
    return false;
  if (!m_pPlayer->IsPlaying())
    return false;
  if (m_pPlayer->HasVideo())
    return false;
  if (m_pPlayer->HasAudio())
    return true;
  return false;
}

bool CApplication::IsPlayingVideo() const
{
  if (!m_pPlayer)
    return false;
  if (!m_pPlayer->IsPlaying())
    return false;
  if (m_pPlayer->HasVideo())
    return true;

  return false;
}

bool CApplication::IsPlayingLiveTV() const
{
  if (!m_pPlayer)
    return false;
  if (!m_pPlayer->IsPlaying())
    return false;
  if (m_pPlayer->HasLiveTV())
    return true;

  return false;
}

bool CApplication::IsCanPause() const
{
  if (!m_pPlayer)
  {
    return false;
  }
  
  if (m_pPlayer->CanPause())
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool CApplication::IsCanSkip() const
{
  if (!m_pPlayer)
  {
    return false;
  }
  
  if (m_pPlayer->CanSkip())
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool CApplication::IsCanGetTime() const
{
  if (!m_pPlayer)
  {
    return false;
  }

  if (m_pPlayer->CanReportGetTime())
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool CApplication::IsCanSetVolume() const
{
  if (!m_pPlayer)
  {
    return false;
  }
  
  if (m_pPlayer->CanSetVolume())
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool CApplication::IsPlayingFullScreenVideo() const
{
  return IsPlayingVideo() && g_graphicsContext.IsFullScreenVideo();
}

void CApplication::MarkVideoAsWatched(const CStdString& path, const CStdString& boxeeId, double videoTime)const
{
  CStdString linkType = m_progressTrackingItem->GetProperty("link-boxeetype");
  if (m_progressTrackingItem->GetPropertyBOOL("IsTrailer") || CFileItemList::GetLinkBoxeeTypeAsEnum(linkType) == CLinkBoxeeType::TRAILER)
  {
    CLog::Log(LOGDEBUG,"CApplication::MarkVideoAsWatched - playing trailer -> NOT going to MarkVideoAsWatched. [label=%s][path=%s][linkType=%s] (wchd)",m_progressTrackingItem->GetLabel().c_str(),m_progressTrackingItem->m_strPath.c_str(),linkType.c_str());
    return;
  }

  BOXEE::Boxee::GetInstance().GetMetadataEngine().MarkAsWatched(path,boxeeId,videoTime);
  
  // attempt to update cache with current item
  m_progressTrackingItem->SetProperty("watched", true);
  g_directoryCache.UpdateItem(*m_progressTrackingItem);

  std::vector<CStdString> paramsVec;
  paramsVec.push_back("MarkAsWatched");
  paramsVec.push_back(m_progressTrackingItem->GetProperty("itemid"));
  paramsVec.push_back("1");

  CGUIMessage markAsWatchedEpisodeMsg(GUI_MSG_UPDATE_ITEM, WINDOW_BOXEE_BROWSE_TVEPISODES,0);
  markAsWatchedEpisodeMsg.SetStringParams(paramsVec);
  g_windowManager.SendThreadMessage(markAsWatchedEpisodeMsg, WINDOW_BOXEE_BROWSE_TVEPISODES);

  CGUIMessage markAsWatchedMovieMsg(GUI_MSG_UPDATE_ITEM, WINDOW_BOXEE_BROWSE_MOVIES,0);
  markAsWatchedMovieMsg.SetStringParams(paramsVec);
  g_windowManager.SendThreadMessage(markAsWatchedMovieMsg, WINDOW_BOXEE_BROWSE_MOVIES);
}

void CApplication::SaveFileState()
{
  CStdString progressTrackingFile = m_progressTrackingItem->m_strPath;

  CLog::Log(LOGDEBUG,"CApplication::SaveFileState - enter function. [path=%s] (wchd)",progressTrackingFile.c_str());
  
  if (progressTrackingFile.IsEmpty())
  {
    CLog::Log(LOGWARNING,"CApplication::SaveFileState - FAILED to save file state since [path=%s] is EMPTY (wchd)",progressTrackingFile.c_str());
    return;
  }

  bool isVideo = m_progressTrackingItem->IsVideo();
  if (isVideo || progressTrackingFile.Left(11) == "playlist://")
  {
    CLog::Log(LOGDEBUG,"CApplication::SaveFileState - saving file state for video [IsVideo=%d] or playlist file. [path=%s][progressTrackingPlayCountUpdate=%d] (wchd)",isVideo,progressTrackingFile.c_str(),m_progressTrackingPlayCountUpdate);

    if (m_progressTrackingPlayCountUpdate)
    {
      bool isTrailer = m_progressTrackingItem->GetPropertyBOOL("istrailer");
      if (!isTrailer)
      {
        CLog::Log(LOGDEBUG,"CApplication::SaveFileState - since [path=%s] is not trailer [isTrailer=%d] going to call MarkVideoAsWatched (wchd)",progressTrackingFile.c_str(),isTrailer);
        MarkVideoAsWatched(progressTrackingFile, m_progressTrackingItem->GetProperty("boxeeid"),m_progressTrackingVideoResumeBookmark.timeInSeconds);
      }
    }

    CVideoDatabase videodatabase;
    if (videodatabase.Open())
    {
      if (m_progressTrackingPlayCountUpdate)
      {
        CLog::Log(LOGDEBUG,"CApplication::SaveFileState - going to mark video file [path=%s] as watched (wchd)",progressTrackingFile.c_str());

        // consider this item as played
        videodatabase.MarkAsWatched(*m_progressTrackingItem);

        CUtil::DeleteVideoDatabaseDirectoryCache();
        CGUIMessage message(GUI_MSG_NOTIFY_ALL, g_windowManager.GetActiveWindow(), 0, GUI_MSG_UPDATE, 0);
        g_windowManager.SendMessage(message);
      }

      if (g_stSettings.m_currentVideoSettings != g_stSettings.m_defaultVideoSettings)
      {
        videodatabase.SetVideoSettings(progressTrackingFile, g_stSettings.m_currentVideoSettings);
      }

      if (m_progressTrackingVideoResumeBookmark.timeInSeconds < 0.0f && !m_progressTrackingItem->IsInternetStream())
      {
        videodatabase.ClearBookMarksOfFile(progressTrackingFile, CBookmark::RESUME);
      }
      else
      if (m_progressTrackingVideoResumeBookmark.timeInSeconds > 0.0f || m_progressTrackingItem->IsInternetStream())
      {
        CLog::Log(LOGDEBUG,"CApplication::SaveFileState - since [timeInSeconds=%f] is bigger that 0, going to AddBookMarkToFile and MarkVideoAsWatched (wchd)",m_progressTrackingVideoResumeBookmark.timeInSeconds);
        videodatabase.AddBookMarkToFile(progressTrackingFile, m_progressTrackingVideoResumeBookmark, CBookmark::RESUME);
        MarkVideoAsWatched(progressTrackingFile, m_progressTrackingItem->GetProperty("boxeeid"),m_progressTrackingVideoResumeBookmark.timeInSeconds);
      }

      videodatabase.Close();
    }
    else
    {
      CLog::Log(LOGDEBUG,"CApplication::SaveFileState - open of VideoDatabase returned FALSE (wchd)");
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "CApplication::SaveFileState - saving file state for audio file [path=%s] (wchd)",progressTrackingFile.c_str());

    if (m_progressTrackingPlayCountUpdate)
    {
      // Can't write to the musicdatabase while scanning for music info
      CGUIDialogMusicScan *dialog = (CGUIDialogMusicScan *)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
      if (dialog && !dialog->IsDialogRunning())
      {
        // consider this item as played
        CLog::Log(LOGDEBUG, "CApplication::SaveFileState - going to mark audio file [path=%s] as listened (wchd)",progressTrackingFile.c_str());

        CMusicDatabase musicdatabase;
        if (musicdatabase.Open())
        {
          musicdatabase.IncrTop100CounterByFileName(progressTrackingFile);
          musicdatabase.Close();
        }
      }
    }
  }
}

void CApplication::UpdateFileState()
{
  // No resume for livetv
  if (m_progressTrackingItem->IsLiveTV())
    return;

  // Did the file change?
  if (m_progressTrackingItem->m_strPath != "" && m_progressTrackingItem->m_strPath != CurrentFile())
  {
    //CLog::Log(LOGDEBUG,"CApplication::UpdateFileState - [trackingPath=%s][currentPath=%s] (wchd)", m_progressTrackingItem->m_strPath.c_str(),CurrentFile().c_str());

    SaveFileState();

    // Reset tracking item
    m_progressTrackingItem->Reset();
  }
  else
  {
    if (m_progressTrackingItem->m_strPath == "")
    {
      // Init some stuff
      //CLog::Log(LOGDEBUG,"CApplication::UpdateFileState, init (wchd)");
      *m_progressTrackingItem = CurrentFileItem();
      m_progressTrackingPlayCountUpdate = false;
      //CLog::Log(LOGDEBUG,"CApplication::UpdateFileState - after set m_progressTrackingPlayCountUpdate to [%d=FALSE]. [path=%s] (wchd)", m_progressTrackingPlayCountUpdate,m_progressTrackingItem->m_strPath.c_str());
    }

    if ((m_progressTrackingItem->IsAudio() && g_advancedSettings.m_audioPlayCountMinimumPercent > 0 &&
        GetPercentage() >= g_advancedSettings.m_audioPlayCountMinimumPercent) ||
        (m_progressTrackingItem->IsVideo() && GetPercentage() >= g_advancedSettings.m_videoPlayCountMinimumPercent))
    {
      m_progressTrackingPlayCountUpdate = true;
      //CLog::Log(LOGDEBUG,"CApplication::UpdateFileState - after set m_progressTrackingPlayCountUpdate to [%d=TRUE]. [path=%s] (wchd)", m_progressTrackingPlayCountUpdate,m_progressTrackingItem->m_strPath.c_str());
    }

    if (m_progressTrackingItem->IsVideo() || m_progressTrackingItem->m_strPath.Left(11) == "playlist://")
    {
      // Update bookmark for save
      m_progressTrackingVideoResumeBookmark.player = CPlayerCoreFactory::GetPlayerName(m_eCurrentPlayer);
      m_progressTrackingVideoResumeBookmark.playerState = m_pPlayer->GetPlayerState();
      m_progressTrackingVideoResumeBookmark.thumbNailImage.Empty();

      if ((g_advancedSettings.m_videoIgnoreAtEnd > 0) && (GetTime() > 0) &&
          ((GetTotalTime() - GetTime()) < g_advancedSettings.m_videoIgnoreAtEnd))
      {
        // Delete the bookmark
        m_progressTrackingVideoResumeBookmark.timeInSeconds = -1.0f;
      }
      else
      if (GetTime() > g_advancedSettings.m_videoIgnoreAtStart)
      {
        // Update the bookmark
        m_progressTrackingVideoResumeBookmark.timeInSeconds = GetTime();
        m_progressTrackingVideoResumeBookmark.totalTimeInSeconds = GetTotalTime();
      }
      else if(GetTime() > 0)
      {
        // Do nothing
        m_progressTrackingVideoResumeBookmark.timeInSeconds = 0.0f;
      }
      SaveFileState();

      //CLog::Log(LOGDEBUG,"CApplication::UpdateFileState, set m_progressTrackingVideoResumeBookmark to %d (wchd)", m_progressTrackingVideoResumeBookmark.timeInSeconds);
    }
  }
}

void CApplication::StopPlaying()
{
  int iWin = g_windowManager.GetActiveWindow();
  if ( IsPlaying() )
  {
#ifdef HAS_KARAOKE
    if( m_pKaraokeMgr )
      m_pKaraokeMgr->Stop();
#endif

    // turn off visualisation window when stopping
    if (iWin == WINDOW_VISUALISATION)
      g_windowManager.PreviousWindow();

    CLog::Log(LOGDEBUG,"CApplication::StopPlaying - call UpdateFileState (wchd)");

    // We will update the file state through the OnPlaybackStop callback
    //UpdateFileState();

    if (m_pPlayer)
      m_pPlayer->CloseFile();

    if (m_itemCurrentFile)
    {
      m_itemCurrentFile->Reset();
      CLog::Log(LOGDEBUG,"CApplication::StopPlaying - m_itemCurrentFile was reset (wchd)");
    }
#ifndef _BOXEE_
    g_partyModeManager.Disable();
#endif
  }

  CAppManager::GetInstance().OnActionStop();
  GetItemLoader().Resume();
}

bool CApplication::NeedRenderFullScreen()
{
  if(IsPlayingLiveTV())
    return true;

  if (g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
  {
    g_windowManager.UpdateModelessVisibility();

    if (g_windowManager.HasDialogOnScreen()) return true;
    if (g_Mouse.IsActive()) return true;

    CGUIWindowFullScreen *pFSWin = (CGUIWindowFullScreen *)g_windowManager.GetWindow(WINDOW_FULLSCREEN_VIDEO);
    if (!pFSWin)
      return false;
    return pFSWin->NeedRenderFullScreen();
  }
  return false;
}

void CApplication::RenderFullScreen()
{
  MEASURE_FUNCTION;

  g_ApplicationRenderer.Render(true);
}

void CApplication::DoRenderFullScreen()
{
  MEASURE_FUNCTION;

  if (g_graphicsContext.IsFullScreenVideo())
  {
    // make sure our overlays are closed
    CGUIDialog *overlay = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_VIDEO_OVERLAY);
    if (overlay) overlay->Close(true);
    overlay = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_MUSIC_OVERLAY);
    if (overlay) overlay->Close(true);

#ifdef HAS_DVB
    if(g_application.IsPlayingLiveTV())
    {
      CGUIWindowBoxeeLiveTv *pFSWin = (CGUIWindowBoxeeLiveTv *)g_windowManager.GetWindow(WINDOW_BOXEE_LIVETV);
      if (!pFSWin)
      {
        return ;
      }
      pFSWin->RenderFullScreen();
    }
    else
#endif
    {
      CGUIWindowFullScreen *pFSWin = (CGUIWindowFullScreen *)g_windowManager.GetWindow(WINDOW_FULLSCREEN_VIDEO);
      if (!pFSWin)
        return ;
      pFSWin->RenderFullScreen();
    }

    if (g_windowManager.HasDialogOnScreen())
      g_windowManager.RenderDialogs();
    
    g_TextureManager.FreeUnusedTextures();
  }
}

void CApplication::ResetScreenSaver()
{
  // reset our timers
  m_shutdownTimer.StartZero();

  // screen saver timer is reset only if we're not already in screensaver or
  // DPMS mode
  if ((!m_bScreenSave && m_iScreenSaveLock == 0) && !m_dpmsIsActive)
    ResetScreenSaverTimer();
}

void CApplication::ResetScreenSaverTimer()
{
  m_screenSaverTimer.StartZero();
}

bool CApplication::WakeUpScreenSaverAndDPMS()
{
  // First reset DPMS, if active
  if (m_dpmsIsActive)
  {
    // TODO: if screensaver lock is specified but screensaver is not active
    // (DPMS came first), activate screensaver now.
    m_dpms->DisablePowerSaving();
    m_dpmsIsActive = false;
    ResetScreenSaverTimer();
    return !m_bScreenSave || WakeUpScreenSaver();
  }
  else
    return WakeUpScreenSaver();
}

bool CApplication::WakeUpScreenSaver()
{
  if (m_iScreenSaveLock == 2)
    return false;

  m_bInactive = false;  // reset the inactive flag as a key has been pressed

  // if Screen saver is active
  if (m_bScreenSave)
  {
    int iProfile = g_settings.m_iLastLoadedProfileIndex;
    if (m_iScreenSaveLock == 0)
      if (g_guiSettings.GetBool("screensaver.uselock")                           &&
          g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE        &&
          g_settings.m_vecProfiles[iProfile].getLockMode() != LOCK_MODE_EVERYONE &&
          !g_guiSettings.GetString("screensaver.mode").Equals("Black")           &&
          !(g_guiSettings.GetBool("screensaver.usemusicvisinstead")              &&
          !g_guiSettings.GetString("screensaver.mode").Equals("Black")           &&
          g_application.IsPlayingAudio())                                          )
      {
        m_iScreenSaveLock = 2;
        CGUIMessage msg(GUI_MSG_CHECK_LOCK,0,0);
        g_windowManager.GetWindow(WINDOW_SCREENSAVER)->OnMessage(msg);
      }
    if (m_iScreenSaveLock == -1)
    {
      m_iScreenSaveLock = 0;
      return true;
    }

    // disable screensaver
    m_bScreenSave = false;
    m_iScreenSaveLock = 0;
    ResetScreenSaverTimer();

#ifdef HAS_EMBEDDED
   CBoxeeVersionUpdateJob versionUpdateJob = g_boxeeVersionUpdateManager.GetBoxeeVerUpdateJob();
   bool isPlayingOrPaused = g_application.IsPlaying() || g_application.IsPaused();

   if(!isPlayingOrPaused && versionUpdateJob.acquireVersionUpdateJobForPerformUpdate())
   {
     VERSION_UPDATE_FORCE versionUpdateForce = (versionUpdateJob.GetVersionUpdateInfo()).GetVersionUpdateForce();

     if(versionUpdateForce == VUF_YES)
     {
       CLog::Log(LOGDEBUG,"CApplication::WakeUpScreenSaver - UpdateForce is VUF_YES, going to open Update dialog WITHOUT CANCEL button (update)");
       bool retVal = CBoxeeVersionUpdateManager::HandleUpdateVersionButton(true);
       CLog::Log(LOGDEBUG,"CApplication::WakeUpScreenSaver - Call to CBoxeeVersionUpdateManager::HandleUpdateVersionButton() returned [%d] (update)",retVal); 
     }
  }
#endif

    if (m_screenSaverMode.CompareNoCase("Visualisation") == 0 || 
        m_screenSaverMode.CompareNoCase("Slideshow") == 0 || 
        m_screenSaverMode.CompareNoCase("Fanart Slideshow") == 0)
    {
      // we can just continue as usual from vis mode
      return false;
    }
    else if (m_screenSaverMode == "Dim" || m_screenSaverMode == "Black")
      return true;
    else if (m_screenSaverMode != "None")
    { // we're in screensaver window
      if (g_windowManager.GetActiveWindow() == WINDOW_SCREENSAVER)
        g_windowManager.PreviousWindow();  // show the previous window
      }
      return true;
    }
  else
    return false;
}

bool CApplication::IsUserLoggedIn()
{
  return m_userLoggedIn;
}

void CApplication::SetUserLoggedIn(bool userLoggedIn)
{
  m_userLoggedIn = userLoggedIn;
}

void CApplication::CheckRefreshHomeScreen()
{
  if (g_windowManager.GetActiveWindow() == WINDOW_HOME)
  {
    // Case of WINDOW_HOME is active -> Check if HomeScreen timer is on
    
    if(IsHomeScreenTimerOn())
    {
      // Case of timer on -> Check if we should refresh the HomeScreen
      
      if ((long)(CTimeUtils::GetTimeMS() - m_dwRefreshHomeScreenTick) >= (long)HOME_SCREEN_REFRESH_INTERVAL_IN_MIN*60*1000L)
      {
        // Case of timer expire -> Need to refresh the HomeScreen

        SetHomeScreenTimerOnStatus(false);

        CGUIMessage message(GUI_MSG_UPDATE, WINDOW_HOME, 0);
        g_windowManager.SendMessage(message);
      }
      else
      {
        // Case of timer not expire -> Continue
      }
    }
    else
    {
      // Case of time off -> activate the timer
      
      m_dwRefreshHomeScreenTick = CTimeUtils::GetTimeMS();
      SetHomeScreenTimerOnStatus(true);
    }
  }
  else
  {
    // Case of WINDOW_HOME is not active -> reset the HomeScreen timer
    
    SetHomeScreenTimerOnStatus(false);    
  }
}

void CApplication::SetHomeScreenTimerOnStatus(bool status)
{
  m_homeScreenTimerOn = status;
}

bool CApplication::IsHomeScreenTimerOn()
{  
  return m_homeScreenTimerOn;
}

void CApplication::RemoveOldThumbnails(bool forceCheck, bool showConfirmationDialog)
{  
  if (showConfirmationDialog && !CGUIDialogYesNo2::ShowAndGetInput(51562,51563))
  {
    return;
  }

  bool isInScreensaver = GetInSlideshowScreensaver();
  
  // Going to check for old thumbnails if we are in screensaver or if forceCheck==TRUE
  if (isInScreensaver || forceCheck)
  {
    // Get the current time
    time_t currentTime = time(NULL);
  
    int checkDiffInDays = (-1);
    time_t lastCheck = 0;
    
    if (forceCheck == false)
    {
      // Need to calculate the diff between check dates only if [forceCheck==FALSE]
      
      // Get the last time we checked for thumbnails
      lastCheck = g_stSettings.m_lastTimeCheckForThumbRemoval;
  
      // Check if we need to perform the check for thumbnail removal
      checkDiffInDays = BoxeeUtils::DayDiff(currentTime,lastCheck);
    }

    if ((checkDiffInDays >= CHECK_REMOVE_OLD_THUMBNAILS_INTERVAL_IN_DAYS) || forceCheck)
    {
      m_tumbnailsMgr.ClearThumnails(currentTime - (CHECK_REMOVE_OLD_THUMBNAILS_INTERVAL_IN_DAYS * 60*60*24) , g_advancedSettings.m_thumbsMaxSize);

      // Save the time of the check for thumbnails removal
      g_stSettings.m_lastTimeCheckForThumbRemoval = currentTime;
    }
  }
}

void CApplication::CheckScreenSaverAndDPMS()
{
  bool maybeScreensaver = 
      !m_dpmsIsActive && !m_bScreenSave
      && g_guiSettings.GetString("screensaver.mode") != "None";
  bool maybeDPMS =
      m_AppFocused && // app focused is bogus
      !m_dpmsIsActive && m_dpms->IsSupported()
      && g_guiSettings.GetInt("screensaver.powersavingtime") > 0;


  // Has the screen saver window become active?
  if (maybeScreensaver && g_windowManager.IsWindowActive(WINDOW_SCREENSAVER))
  {
    m_bScreenSave = true;
    maybeScreensaver = false;
  }

  if (!maybeScreensaver && !maybeDPMS) return;  // Nothing to do.

  // See if we need to reset timer.
  // * Are we playing a video and it is not paused?
  if ((IsPlayingVideo() && (!m_pPlayer->IsPaused() || m_pPlayer->IsCaching()))
      // * Are we playing some music in fullscreen vis?
      || (IsPlayingAudio() && g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION))
  {
    ResetScreenSaverTimer();
    return;
  }


  float elapsed = m_screenSaverTimer.GetElapsedSeconds();

  // DPMS has priority (it makes the screensaver not needed)
  if (maybeDPMS
      && (!IsPlayingAudio() || m_pPlayer->IsPaused())
      && elapsed > g_guiSettings.GetInt("screensaver.powersavingtime") * 60)
  {
    m_dpms->EnablePowerSaving(m_dpms->GetSupportedModes()[0]);
    m_dpmsIsActive = true;
    WakeUpScreenSaver();
  }
  else if (maybeScreensaver
           && elapsed > g_guiSettings.GetInt("screensaver.time") * 60)
  {
    ActivateScreenSaver();
  }
}

// activate the screensaver.
// if forceType is true, we ignore the various conditions that can alter
// the type of screensaver displayed
void CApplication::ActivateScreenSaver(bool forceType /*= false */)
{
  m_bScreenSave = true;
  
  m_bInactive = true;
  //m_dwSaverTick = CTimeUtils::GetTimeMS();  // Save the current time for the shutdown timeout

  // Get Screensaver Mode
  m_screenSaverMode = g_guiSettings.GetString("screensaver.mode");

  if (!forceType)
  {
    // Check if we are Playing Audio and Vis instead Screensaver!
    if (IsPlayingAudio() && !IsPaused() && g_guiSettings.GetBool("screensaver.usemusicvisinstead") && g_guiSettings.GetString("mymusic.visualisation") != "None")
    { // activate the visualisation
      m_screenSaverMode = "Visualisation";
      g_windowManager.ActivateWindow(WINDOW_VISUALISATION);
      return;
    }
  }
  // Picture slideshow
  if (m_screenSaverMode == "SlideShow" || m_screenSaverMode == "Fanart Slideshow")
  {
    // reset our codec info - don't want that on screen
    g_infoManager.SetShowCodec(false);
    m_applicationMessenger.PictureSlideShow(g_guiSettings.GetString("screensaver.slideshowpath"), true);
    
    // Set that we are in Slideshow ScreenSaver.
    // TODO - Make it generic when Boxee start using a real screensaver
    SetInSlideshowScreensaver(true);

    return;
  }
  else if (m_screenSaverMode == "Dim")
    return;
  else if (m_screenSaverMode == "Black")
    return;
  else if (m_screenSaverMode != "None")
  {
    g_windowManager.ActivateWindow(WINDOW_SCREENSAVER);
  }
}
  
void CApplication::CheckShutdown()
{
  CGUIDialogMusicScan *pMusicScan = (CGUIDialogMusicScan *)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
  CGUIDialogVideoScan *pVideoScan = (CGUIDialogVideoScan *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);

  // first check if we should reset the timer
  bool resetTimer = false;
  if (IsPlaying()) // is something playing?
    resetTimer = true;

#ifdef HAS_FTP_SERVER
  if (m_pFileZilla && m_pFileZilla->GetNoConnections() != 0) // is FTP active ?
    resetTimer = true;
#endif

  if (pMusicScan && pMusicScan->IsScanning()) // music scanning?
    resetTimer = true;

  if (pVideoScan && pVideoScan->IsScanning()) // video scanning?
    resetTimer = true;

  if (g_windowManager.IsWindowActive(WINDOW_DIALOG_PROGRESS)) // progress dialog is onscreen
    resetTimer = true;

  if (resetTimer)
{
    m_shutdownTimer.StartZero();
    return ;
    }

  if ( m_shutdownTimer.GetElapsedSeconds() > g_guiSettings.GetInt("system.shutdowntime") * 60 )
{
    // Since it is a sleep instead of a shutdown, let's set everything to reset when we wake up.
    m_shutdownTimer.Stop();

    // Sleep the box
    getApplicationMessenger().Shutdown();
  }
    }

bool CApplication::OnMessage(CGUIMessage& message)
{  
  switch ( message.GetMessage() )
  {
  case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1()==GUI_MSG_REMOVED_MEDIA)
      {
        // Update general playlist: Remove DVD playlist items
        int nRemoved = g_playlistPlayer.RemoveDVDItems();
        if ( nRemoved > 0 )
        {
          CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0 );
          g_windowManager.SendMessage( msg );
        }
        // stop the file if it's on dvd (will set the resume point etc)
        if (m_itemCurrentFile->IsOnDVD())
          StopPlaying();
      }
    }
    break;

  case GUI_MSG_PLAYBACK_STARTED:
    {
      // Update our infoManager with the new details etc.
      if (m_nextPlaylistItem >= 0)
      { // we've started a previously queued item

        // guard against playlist being flushed while a message is in the queue
        CPlayList& playlist = g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist());
        if( m_nextPlaylistItem < playlist.size())
        {
          CFileItemPtr item = playlist[m_nextPlaylistItem];
          
          // update the playlist manager
          int currentSong = g_playlistPlayer.GetCurrentSong();
          int param = ((currentSong & 0xffff) << 16) | (m_nextPlaylistItem & 0xffff);
          CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_CHANGED, 0, 0, g_playlistPlayer.GetCurrentPlaylist(), param, item);
          g_windowManager.SendThreadMessage(msg);
          g_playlistPlayer.SetCurrentSong(m_nextPlaylistItem);
          *m_itemCurrentFile = *item;
        }
      }
      
      if (m_itemCurrentFile->GetThumbnailImage().Left(7) == "http://")
      {
        CPictureThumbLoader loader;
        loader.LoadItem(m_itemCurrentFile.get(), true);
      }
      
      g_infoManager.SetCurrentItem(*m_itemCurrentFile);
#ifdef HAS_LASTFM
      CLastFmManager::GetInstance()->OnSongChange(*m_itemCurrentFile);
#endif
#ifndef _BOXEE_
      g_partyModeManager.OnSongChange(true);
#endif
      DimLCDOnPlayback(true);

      if (IsPlayingAudio())
      {
        // Start our cdg parser as appropriate
#ifdef HAS_KARAOKE
        if (m_pKaraokeMgr && g_guiSettings.GetBool("karaoke.enabled") && !m_itemCurrentFile->IsInternetStream())
        {
          m_pKaraokeMgr->Stop();
          if (m_itemCurrentFile->IsMusicDb())
          {
            if (!m_itemCurrentFile->HasMusicInfoTag() || !m_itemCurrentFile->GetMusicInfoTag()->Loaded())
            {
              IMusicInfoTagLoader* tagloader = CMusicInfoTagLoaderFactory::CreateLoader(m_itemCurrentFile->m_strPath);
              tagloader->Load(m_itemCurrentFile->m_strPath,*m_itemCurrentFile->GetMusicInfoTag());
              delete tagloader;
            }
            m_pKaraokeMgr->Start(m_itemCurrentFile->GetMusicInfoTag()->GetURL());
          }
          else
            m_pKaraokeMgr->Start(m_itemCurrentFile->m_strPath);
        }
#endif
        // Let scrobbler know about the track
        const CMusicInfoTag* tag=g_infoManager.GetCurrentSongTag();
        if (tag)
        {
#ifdef HAS_LASTFM
          CLastfmScrobbler::GetInstance()->AddSong(*tag, CLastFmManager::GetInstance()->IsRadioEnabled());
          CLibrefmScrobbler::GetInstance()->AddSong(*tag, CLastFmManager::GetInstance()->IsRadioEnabled());
#endif
        }
      }
      UpdateFileState();
      return true;
    }
    UpdateFileState();
    break;

  case GUI_MSG_QUEUE_NEXT_ITEM:
    {
      // Check to see if our playlist player has a new item for us,
      // and if so, we check whether our current player wants the file
      int iNext = g_playlistPlayer.GetNextSong();
      CPlayList& playlist = g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist());
      if (iNext < 0 || iNext >= playlist.size())
      {
        if (m_pPlayer) m_pPlayer->OnNothingToQueueNotify();
        return true; // nothing to do
      }
      // ok, grab the next song
      CFileItemPtr item = playlist[iNext];
      // ok - send the file to the player if it wants it
      if (m_pPlayer && m_pPlayer->QueueNextFile(*item))
      { // player wants the next file
        m_nextPlaylistItem = iNext;
      }
      return true;
    }
    break;

  case GUI_MSG_PLAYBACK_STOPPED:
  case GUI_MSG_PLAYBACK_ENDED:
  case GUI_MSG_PLAYLISTPLAYER_STOPPED:
    {
#ifdef HAS_KARAOKE
      if (m_pKaraokeMgr)
        m_pKaraokeMgr->Stop();
#endif
      UpdateFileState();

      // first check if we still have items in the stack to play
      if (message.GetMessage() == GUI_MSG_PLAYBACK_ENDED)
      {
        if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0 && m_currentStackPosition < m_currentStack->Size() - 1)
        { // just play the next item in the stack
          PlayFile(*(*m_currentStack)[++m_currentStackPosition], true);
          return true;
        }
        //BOXEE
        else
        {
          // can mark video as watched
          if (CurrentFileItem().IsVideo())
          {
            MarkVideoAsWatched(CurrentFileItem().m_strPath,CurrentFileItem().GetProperty("boxeeid"),m_progressTrackingVideoResumeBookmark.timeInSeconds);
          }

          if (message.GetParam1() == 1)
          {
            // printf("  ------------ message.GetParam1()=%d:%s\n", message.GetParam1(), message.GetLabel().c_str());
            ThreadMessage tMsg(TMSG_SHOW_PLAY_ERROR);
            tMsg.strParam = message.GetLabel();
            g_application.getApplicationMessenger().SendMessage(tMsg, false);
          }
          else
          {
            bool bShowPostPlay = g_guiSettings.GetBool("boxee.showpostplay") && CurrentFileItem().IsVideo() && (BoxeeUtils::CanShare(CurrentFileItem()) || CurrentFileItem().GetNextItem().get() );
            if ( bShowPostPlay )
            {
              ThreadMessage tMsg(TMSG_SHOW_POST_PLAY_DIALOG);
              CFileItem *pItem = new CFileItem;
              *pItem = CurrentFileItem();
              tMsg.lpVoid = pItem;
              g_application.getApplicationMessenger().SendMessage(tMsg, false);
            }
          }
        }
        //end BOXEE
      } // end GUI_MSG_PLAYBACK_ENDED.      

      // reset the current playing file
      m_itemCurrentFile->Reset();
      CLog::Log(LOGDEBUG,"CApplication::StopPlaying - m_itemCurrentFile was reset (wchd)");
      g_infoManager.ResetCurrentItem();
      m_currentStack->Clear();

      if (message.GetMessage() == GUI_MSG_PLAYBACK_ENDED)
      {
        // sending true to PlayNext() effectively passes bRestart to PlayFile()
        // which is not generally what we want (except for stacks, which are
        // handled above)
        g_playlistPlayer.PlayNext();        
      }
      else
      {
#ifdef HAS_LASTFM
        // stop lastfm
        if (CLastFmManager::GetInstance()->IsRadioEnabled())
          CLastFmManager::GetInstance()->StopRadio();
#endif

        if (m_pPlayer && !m_bPlaybackStarting)
        {
          IPlayer *pTempPlayer = m_pPlayer;
          m_pPlayer = NULL;  // sometimes deleting the player will cause this code to run again. so to prevent double free of the
                             // player- we rest it and then delete
          delete pTempPlayer;
        }
      }

      if (!IsPlaying())
      {
        g_audioManager.Enable(true);
        DimLCDOnPlayback(false);
      }

      if (!IsPlayingVideo() && (g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO || g_windowManager.GetActiveWindow() == WINDOW_BOXEE_LIVETV) && message.GetMessage() != GUI_MSG_PLAYBACK_ENDED)
      {
        g_windowManager.PreviousWindow();
      }

      if (!IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_NONE && g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION)
      {
        g_settings.Save();  // save vis settings
        WakeUpScreenSaverAndDPMS();
        g_windowManager.PreviousWindow();
      }

      // reset the audio playlist on finish
      if (!IsPlayingAudio() && (g_guiSettings.GetBool("mymusic.clearplaylistsonend")) && (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC))
      {
        g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
        g_playlistPlayer.Reset();
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);
      }

      // DVD ejected while playing in vis ?
      if (!IsPlayingAudio() && (m_itemCurrentFile->IsCDDA() || m_itemCurrentFile->IsOnDVD()) && !g_mediaManager.IsDiscInDrive() && g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION)
      {
        // yes, disable vis
        g_settings.Save();    // save vis settings
        WakeUpScreenSaverAndDPMS();
        g_windowManager.PreviousWindow();
      }
      return true;
    }
    break;

  case GUI_MSG_PLAYLISTPLAYER_STARTED:
  case GUI_MSG_PLAYLISTPLAYER_CHANGED:
    {
      return true;
    }
    break;
  case GUI_MSG_FULLSCREEN:
    {
      // Switch to fullscreen, if we can
      SwitchToFullScreen();
      return true;
    }
    break;
  case GUI_MSG_EXECUTE:
    if (message.GetNumStringParams())
      return ExecuteXBMCAction(message.GetStringParam());
    else
    {
      CGUIActionDescriptor action = message.GetAction();
      action.m_sourceWindowId = message.GetControlId(); // set source window id, 
      return ExecuteAction(action);
    }

    break;
  }
  return false;
}

bool CApplication::ExecuteXBMCAction(std::string actionStr)
{
  // see if it is a user set string
  CLog::Log(LOGDEBUG,"%s : Translating %s", __FUNCTION__, actionStr.c_str());
  CGUIInfoLabel info(actionStr, "");
  actionStr = info.GetLabel(0);
  CLog::Log(LOGDEBUG,"%s : To %s", __FUNCTION__, actionStr.c_str());

  // user has asked for something to be executed
  if (CBuiltins::HasCommand(actionStr))
    CBuiltins::Execute(actionStr);
  else
  {
    // try translating the action from our ButtonTranslator
        int actionID;
        if (CButtonTranslator::TranslateActionString(actionStr.c_str(), actionID))
    {
      CAction action;
          action.id = actionID;
          action.amount1 = 1.0f;
      OnAction(action);
      return true;
    }
    CFileItem item(actionStr, false);
#ifdef HAS_PYTHON
    if (item.IsPythonScript())
    { // a python script
      g_pythonParser.evalFile(item.m_strPath.c_str());
    }
    else
#endif
    if (item.IsAudio() || item.IsVideo())
    { // an audio or video file
      PlayFile(item);
    }
    else
      return false;
  }
  return true;
}

bool CApplication::ExecuteAction(CGUIActionDescriptor action)
{
  if (action.m_lang == CGUIActionDescriptor::LANG_XBMC)
  {
    return ExecuteXBMCAction(action.m_action);
  }
  else if (action.m_lang == CGUIActionDescriptor::LANG_PYTHON)
  {
#ifdef HAS_PYTHON
    // Determine the context of the action, if possible
    if (action.m_sourceWindowId >= WINDOW_APPS_START && action.m_sourceWindowId <= WINDOW_APPS_END) 
    {
      // this is an application window, get the id of currently running application from the AppManager
      CLog::Log(LOGDEBUG, "CApplication::ExecuteAction, execute python action for window %u in app context: %s (python)",action.m_sourceWindowId, CAppManager::GetInstance().GetLastLaunchedId().c_str());
      CStdString targetPath = _P("special://home/apps/");
      CStdString partnerId = CAppManager::GetInstance().GetLastLaunchedDescriptor().GetPartnerId();

      targetPath += CAppManager::GetInstance().GetLastLaunchedId();

      std::vector<CStdString> params;
      g_pythonParser.evalStringInContext(action.m_action, targetPath, CAppManager::GetInstance().GetLastLaunchedId(), partnerId, params);
    }
    else 
    {
      g_pythonParser.evalString(action.m_action);
    }
    return true;
#else
    return false;
#endif
  }
#ifdef APP_JS
  else if (action.m_lang == CGUIActionDescriptor::LANG_JAVASCRIPT)
  {
    return GetJavaScriptManager().evalString(action.m_action);
  }  
#endif
  return false;
}

void CApplication::Process()
{  
  // check if we need to load a new skin
  if (m_skinReloadTime && CTimeUtils::GetFrameTime() >= m_skinReloadTime)
  {
    ReloadSkin();
  }

  // dispatch the messages generated by python or other threads to the current window
  g_windowManager.DispatchThreadMessages();

  // process messages which have to be send to the gui
  // (this can only be done after g_windowManager.Render())
  m_applicationMessenger.ProcessWindowMessages();

#ifdef HAS_PYTHON
  // process any Python scripts
  g_pythonParser.Process();
#endif

  // process messages, even if a movie is playing
  m_applicationMessenger.ProcessMessages();
  if (g_application.m_bStop) return; //we're done, everything has been unloaded

  // check if we can free unused memory
#ifndef _LINUX
  g_audioManager.FreeUnused();
#endif

  // check how far we are through playing the current item
  // and do anything that needs doing (lastfm submission, playcount updates etc)
  CheckPlayingProgress();

  // update sound
  if (m_pPlayer)
    m_pPlayer->DoAudioWork();

  // do any processing that isn't needed on each run
  if(m_slowTimer.GetElapsedMilliseconds() > 2000 )
  {
    m_slowTimer.Reset();
    ProcessSlow();
  }
}

// We get called every 500ms
void CApplication::ProcessSlow()
{
  if (IsPlayingVideo())
  {
    // Store our file state for use on close()
    // Cache the current time.
    m_progressTrackingVideoResumeBookmark.timeInSeconds = GetTime();
    m_progressTrackingVideoResumeBookmark.totalTimeInSeconds = GetTotalTime();

    if ((g_advancedSettings.m_videoIgnoreAtEnd > 0) && (GetTime() > 0) &&
        ((GetTotalTime() - GetTime()) < g_advancedSettings.m_videoIgnoreAtEnd))
    {
      // Delete the bookmark
      m_progressTrackingVideoResumeBookmark.timeInSeconds = -1.0f;
    }

    // Check if we need to activate the screensaver / DPMS.
    CheckScreenSaverAndDPMS();
  }
  else
  {
    // Check if the HomeScreen need to be refresh
    CheckRefreshHomeScreen();

    // Check if need to remove old thumbnails
    RemoveOldThumbnails(false, false);

    // run resume jobs if we are coming from suspend/hibernate

    if (IsPlayingAudio())
    {
#ifdef HAS_LASTFM
      CLastfmScrobbler::GetInstance()->UpdateStatus();
      CLibrefmScrobbler::GetInstance()->UpdateStatus();
#endif
    }

    // Check if we need to activate the screensaver / DPMS.
    CheckScreenSaverAndDPMS();

    // Check if we need to shutdown (if enabled).
#ifdef __APPLE__
    if (g_guiSettings.GetInt("system.shutdowntime") && g_advancedSettings.m_fullScreen)
#else
    if (g_guiSettings.GetInt("system.shutdowntime"))
#endif
    {
      CheckShutdown();
    }

    // check if we should restart the player
    CheckDelayedPlayerRestart();
    //
    //  check if we can unload any unreferenced dlls or sections
    CSectionLoader::UnloadDelayed();

#ifdef HAS_KARAOKE
    if ( m_pKaraokeMgr )
      m_pKaraokeMgr->ProcessSlow();
#endif

    // LED - LCD SwitchOn On Paused! m_bIsPaused=TRUE -> LED/LCD is ON!
    if(IsPaused() != m_bIsPaused)
    {
#ifdef HAS_LCD
      if(g_guiSettings.GetBool("lcd.enableonpaused"))
        DimLCDOnPlayback(m_bIsPaused);
#endif
      m_bIsPaused = IsPaused();
    }

    if (!IsPlayingVideo())
      g_largeTextureManager.CleanupUnusedImages();

#ifdef HAS_DVD_DRIVE  
    // checks whats in the DVD drive and tries to autostart the content (xbox games, dvd, cdda, avi files...)
    m_Autorun.HandleAutorun();
#endif

    //Check to see if current playing Title has changed and whether we should broadcast the fact
    CheckForTitleChange();

    g_mediaManager.ProcessEvents();

#if defined(HAS_LIRC)
    if (g_RemoteControl.IsInUse() && !g_RemoteControl.IsInitialized())
      g_RemoteControl.Initialize();
#endif

#ifdef HAS_LCD
    // attempt to reinitialize the LCD (e.g. after resuming from sleep)
    if (g_lcd && !g_lcd->IsConnected())
    {
      g_lcd->Stop();
      g_lcd->Initialize();
    }
#endif
  }
}

// Global Idle Time in Seconds
// idle time will be resetet if on any OnKey()
// int return: system Idle time in seconds! 0 is no idle!
int CApplication::GlobalIdleTime()
{
  if(!m_idleTimer.IsRunning())
  {
    m_idleTimer.Stop();
    m_idleTimer.StartZero();
  }
  return (int)m_idleTimer.GetElapsedSeconds();
}

float CApplication::NavigationIdleTime()
{
  if (!m_navigationTimer.IsRunning())
  {
    m_navigationTimer.Stop();
    m_navigationTimer.StartZero();
  }
  return m_navigationTimer.GetElapsedSeconds();
}

void CApplication::DelayedPlayerRestart()
{
  m_restartPlayerTimer.StartZero();
}

void CApplication::CheckDelayedPlayerRestart()
{
  if (m_restartPlayerTimer.GetElapsedSeconds() > 3)
  {
    m_restartPlayerTimer.Stop();
    m_restartPlayerTimer.Reset();
    Restart(true);
  }
}

void CApplication::Restart(bool bSamePosition)
{
  // this function gets called when the user changes a setting (like noninterleaved)
  // and which means we gotta close & reopen the current playing file

  // first check if we're playing a file
  if ( !IsPlayingVideo() && !IsPlayingAudio())
    return ;

  if( !m_pPlayer )
    return ;

  // do we want to return to the current position in the file
  if (false == bSamePosition)
  {
    // no, then just reopen the file and start at the beginning
    PlayFile(*m_itemCurrentFile, true);
    return ;
  }

  // else get current position
  double time = GetTime();

  // get player state, needed for dvd's
  CStdString state = m_pPlayer->GetPlayerState();

  // set the requested starttime
  m_itemCurrentFile->m_lStartOffset = (long)(time * 75.0);

  // reopen the file
  if ( PlayFile(*m_itemCurrentFile, true) && m_pPlayer )
    m_pPlayer->SetPlayerState(state);
}

const CStdString& CApplication::CurrentFile()
{
  return m_itemCurrentFile->m_strPath;
}

CFileItem& CApplication::CurrentFileItem()
{
  return *m_itemCurrentFile;
}

void CApplication::Mute(void)
{
  if (g_stSettings.m_bMute)
  { // muted - unmute.
    // check so we don't get stuck in some muted state
    if( g_stSettings.m_iPreMuteVolumeLevel == 0 )
      g_stSettings.m_iPreMuteVolumeLevel = 1;
    SetVolume(g_stSettings.m_iPreMuteVolumeLevel);
  }
  else
  { // mute
    g_stSettings.m_iPreMuteVolumeLevel = GetVolume();
    SetVolume(0);
  }
}

void CApplication::UpdateVolume()
{
  if (g_guiSettings.GetBool("audiooutput.controlmastervolume"))
  {
    CAudioUtils::GetInstance().UpdateAppVolume();
    return;
  }
}

void CApplication::SetVolume(int iPercent)
{
  // convert the percentage to a mB (milliBell) value (*100 for dB)
  long hardwareVolume = (long)((float)iPercent * 0.01f * (VOLUME_MAXIMUM - VOLUME_MINIMUM) + VOLUME_MINIMUM);
  SetHardwareVolume(hardwareVolume);
  g_audioManager.SetVolume(hardwareVolume);
}

void CApplication::SetHardwareVolume(long hardwareVolume)
{
  //normalize the values
  if (hardwareVolume >= VOLUME_MAXIMUM) // + VOLUME_DRC_MAXIMUM
    hardwareVolume = VOLUME_MAXIMUM;// + VOLUME_DRC_MAXIMUM;
  if (hardwareVolume <= VOLUME_MINIMUM)
  {
    hardwareVolume = VOLUME_MINIMUM;
  }

  //if control master volume is checked
  if (g_guiSettings.GetBool("audiooutput.controlmastervolume"))
  {
    CAudioUtils::GetInstance().SetMasterVolume(hardwareVolume);
  }
#ifndef HAS_EMBEDDED
  // If this code is in embedded, then volume control on the browser is not working. I was afraid
  // to completely delete this code, but I think it is not really useful
  else if(m_pPlayer && ( !m_pPlayer->CanSetVolume() || m_pPlayer->OSDDisabled() ))
  {
    m_pPlayer->SetVolume(g_stSettings.m_nVolumeLevel);
    return;
  }
#endif

  // update our settings
  if (hardwareVolume > VOLUME_MAXIMUM)
  {
    g_stSettings.m_dynamicRangeCompressionLevel = hardwareVolume - VOLUME_MAXIMUM;
    g_stSettings.m_nVolumeLevel = VOLUME_MAXIMUM;
  }
  else
  {
    g_stSettings.m_dynamicRangeCompressionLevel = 0;
    g_stSettings.m_nVolumeLevel = hardwareVolume;
  }

  // update mute state
  if(!g_stSettings.m_bMute && hardwareVolume <= VOLUME_MINIMUM)
  {
    g_stSettings.m_bMute = true;
    if (!m_guiDialogMuteBug.IsDialogRunning())
      m_guiDialogMuteBug.Show();
  }
  else if(g_stSettings.m_bMute && hardwareVolume > VOLUME_MINIMUM)
  {
    g_stSettings.m_bMute = false;
    if (m_guiDialogMuteBug.IsDialogRunning())
      m_guiDialogMuteBug.Close();
  }

  // and tell our player to update the volume
  if (m_pPlayer && !g_guiSettings.GetBool("audiooutput.controlmastervolume"))
  {
    m_pPlayer->SetVolume(g_stSettings.m_nVolumeLevel);
    // TODO DRC
    //    m_pPlayer->SetDynamicRangeCompression(g_stSettings.m_dynamicRangeCompressionLevel);
  }

}

int CApplication::GetVolume() const
{
  // converts the hardware volume (in mB) to a percentage
  return int(((float)(g_stSettings.m_nVolumeLevel + g_stSettings.m_dynamicRangeCompressionLevel - VOLUME_MINIMUM)) / (VOLUME_MAXIMUM - VOLUME_MINIMUM)*100.0f + 0.5f);
}

int CApplication::GetSubtitleDelay() const
{
  // converts subtitle delay to a percentage
  return int(((float)(g_stSettings.m_currentVideoSettings.m_SubtitleDelay + g_advancedSettings.m_videoSubsDelayRange)) / (2 * g_advancedSettings.m_videoSubsDelayRange)*100.0f + 0.5f);
}

int CApplication::GetAudioDelay() const
{
  // converts subtitle delay to a percentage
  return int(((float)(g_stSettings.m_currentVideoSettings.m_AudioDelay + g_advancedSettings.m_videoAudioDelayRange)) / (2 * g_advancedSettings.m_videoAudioDelayRange)*100.0f + 0.5f);
}

void CApplication::SetPlaySpeed(int iSpeed)
{
  if (!IsPlayingAudio() && !IsPlayingVideo())
    return ;
  if (m_iPlaySpeed == iSpeed)
    return ;
  if (!m_pPlayer->CanSeek())
    return;
  if (m_pPlayer->IsPaused())
  {
    if (
      ((m_iPlaySpeed > 1) && (iSpeed > m_iPlaySpeed)) ||
      ((m_iPlaySpeed < -1) && (iSpeed < m_iPlaySpeed))
    )
    {
      iSpeed = m_iPlaySpeed; // from pause to ff/rw, do previous ff/rw speed
    }
    m_pPlayer->Pause();
  }
  m_iPlaySpeed = iSpeed;

  m_pPlayer->ToFFRW(m_iPlaySpeed);
  if (m_iPlaySpeed == 1)
  { // restore volume
    m_pPlayer->SetVolume(g_stSettings.m_nVolumeLevel);
  }
  else
  { // mute volume
    m_pPlayer->SetVolume(VOLUME_MINIMUM);
  }
}

int CApplication::GetPlaySpeed() const
{
  return m_iPlaySpeed;
}

void CApplication::SetOfflineMode(bool bOffLineMode)
{
  m_bOffLineMode = bOffLineMode;
  BOXEE::Boxee::GetInstance().SetInOfflineMode(m_bOffLineMode);
}

bool CApplication::IsOfflineMode() const
{
  return m_bOffLineMode;
}

// called when the ip address, network settings, or available networks change
void CApplication::NetworkConfigurationChanged()
{
  if( m_pBrowserService )
    m_pBrowserService->Refresh();
}

// Returns the total time in seconds of the current media.  Fractional
// portions of a second are possible - but not necessarily supported by the
// player class.  This returns a double to be consistent with GetTime() and
// SeekTime().
double CApplication::GetTotalTime() const
{
  double rc = 0.0;

  if (IsPlaying() && m_pPlayer)
  {
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
      rc = (*m_currentStack)[m_currentStack->Size() - 1]->m_lEndOffset;
    else
      rc = m_pPlayer->GetTotalTime();
  }

  return rc;
}

void CApplication::ResetPlayTime()
{
  if (IsPlaying() && m_pPlayer)
    m_pPlayer->ResetTime();
}

// Returns the current time in seconds of the currently playing media.
// Fractional portions of a second are possible.  This returns a double to
// be consistent with GetTotalTime() and SeekTime().
double CApplication::GetTime() const
{
  double rc = 0.0;

  if (IsPlaying() && m_pPlayer)
  {
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
    {
      long startOfCurrentFile = (m_currentStackPosition > 0) ? (*m_currentStack)[m_currentStackPosition-1]->m_lEndOffset : 0;
      rc = (double)startOfCurrentFile + m_pPlayer->GetTime() * 0.001;
    }
    else
      rc = static_cast<double>(m_pPlayer->GetTime() * 0.001f);
  }

  return rc;
}

// Sets the current position of the currently playing media to the specified
// time in seconds.  Fractional portions of a second are valid.  The passed
// time is the time offset from the beginning of the file as opposed to a
// delta from the current position.  This method accepts a double to be
// consistent with GetTime() and GetTotalTime().
void CApplication::SeekTime( double dTime )
{
  if (IsPlaying() && m_pPlayer && (dTime >= 0.0))
  {
    if (!m_pPlayer->CanSeek()) return;
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
    {
      // find the item in the stack we are seeking to, and load the new
      // file if necessary, and calculate the correct seek within the new
      // file.  Otherwise, just fall through to the usual routine if the
      // time is higher than our total time.
      for (int i = 0; i < m_currentStack->Size(); i++)
      {
        if ((*m_currentStack)[i]->m_lEndOffset > dTime)
        {
          long startOfNewFile = (i > 0) ? (*m_currentStack)[i-1]->m_lEndOffset : 0;
          if (m_currentStackPosition == i)
            m_pPlayer->SeekTime((__int64)((dTime - startOfNewFile) * 1000.0));
          else
          { // seeking to a new file
            m_currentStackPosition = i;
            CFileItem item(*(*m_currentStack)[i]);
            item.m_lStartOffset = (long)((dTime - startOfNewFile) * 75.0);
            // don't just call "PlayFile" here, as we are quite likely called from the
            // player thread, so we won't be able to delete ourselves.
            m_applicationMessenger.PlayFile(item, true);
          }
          return;
        }
      }
    }
    // convert to milliseconds and perform seek
    m_pPlayer->SeekTime( static_cast<__int64>( dTime * 1000.0 ) );
  }
}

float CApplication::GetPercentage() const
{
  if (IsPlaying() && m_pPlayer)
  {
    if (IsPlayingAudio() && m_itemCurrentFile->HasMusicInfoTag())
    {
      const CMusicInfoTag& tag = *m_itemCurrentFile->GetMusicInfoTag();
      if (tag.GetDuration() > 0)
        return (float)(GetTime() / tag.GetDuration() * 100);
    }

    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
      return (float)(GetTime() / GetTotalTime() * 100);
    else
      return m_pPlayer->GetPercentage();
  }
  return 0.0f;
}
  
float CApplication::GetPercentageWithCache() const
{
  if (IsPlaying() && m_pPlayer)
  {
    if (IsPlayingVideo())
      return m_pPlayer->GetPercentageWithCache();
    else 
      return GetPercentage();
  }
  return 0.0f;
}
  
void CApplication::SeekPercentage(float percent)
{
  if (IsPlaying() && m_pPlayer && (percent >= 0.0))
  {
    if (!m_pPlayer->CanSeek()) return;
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
      SeekTime(percent * 0.01 * GetTotalTime());
    else
      m_pPlayer->SeekPercentage(percent);
  }
}

// SwitchToFullScreen() returns true if a switch is made, else returns false
bool CApplication::SwitchToFullScreen()
{
  // if playing from the video info window, close it first!
#ifndef _BOXEE_
  if (g_windowManager.HasModalDialog() && g_windowManager.GetTopMostModalDialogID() == WINDOW_VIDEO_INFO)
  {
    CGUIWindowVideoInfo* pDialog = (CGUIWindowVideoInfo*)g_windowManager.GetWindow(WINDOW_VIDEO_INFO);
    if (pDialog) pDialog->Close(true);
  }
#endif

  // don't switch if there is a dialog on screen or the slideshow is active
  if (/*g_windowManager.HasModalDialog() ||*/ g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW)
    return false;

  if( IsPlayingLiveTV())
  {
    if (g_windowManager.GetActiveWindow() != WINDOW_BOXEE_LIVETV)
        g_windowManager.ActivateWindow(WINDOW_BOXEE_LIVETV);
    return true;
  }

  // See if we're playing a video, and are in GUI mode
  if ( IsPlayingVideo() && g_windowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO)
  {
#ifdef HAS_SDL
    // Reset frame count so that timing is FPS will be correct.
    SDL_mutexP(m_frameMutex);
    m_frameCount = 0;
    SDL_mutexV(m_frameMutex);
#endif

    // then switch to fullscreen mode
    g_windowManager.ActivateWindow(WINDOW_FULLSCREEN_VIDEO);
    return true;
  }
  // special case for switching between GUI & visualisation mode. (only if we're playing an audio song)
  if (IsPlayingAudio() && g_windowManager.GetActiveWindow() != WINDOW_VISUALISATION)
  { // then switch to visualisation
    g_windowManager.ActivateWindow(WINDOW_VISUALISATION);
    return true;
  }
  return false;
}

void CApplication::Minimize()
{
  g_Windowing.Minimize();
}

PLAYERCOREID CApplication::GetCurrentPlayer()
{
  return m_eCurrentPlayer;
}

// when a scan is initiated, save current settings
// and enable tag reading and remote thums
void CApplication::SaveMusicScanSettings()
{
  CLog::Log(LOGINFO,"Music scan has started... Enabling tag reading, and remote thumbs");
  g_stSettings.m_bMyMusicIsScanning = true;
  g_settings.Save();
}

void CApplication::RestoreMusicScanSettings()
{
  g_stSettings.m_bMyMusicIsScanning = false;
  g_settings.Save();
}

void CApplication::UpdateLibraries()
{
  if (g_guiSettings.GetBool("videolibrary.updateonstartup"))
  {
    CLog::Log(LOGNOTICE, "%s - Starting video library startup scan", __FUNCTION__);
    CGUIDialogVideoScan *scanner = (CGUIDialogVideoScan *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
    SScraperInfo info;
    VIDEO::SScanSettings settings;
    if (scanner && !scanner->IsScanning())
      scanner->StartScanning("",info,settings,false);
  }
 
  if (g_guiSettings.GetBool("musiclibrary.updateonstartup"))
  {
    CLog::Log(LOGNOTICE, "%s - Starting music library startup scan", __FUNCTION__);
    CGUIDialogMusicScan *scanner = (CGUIDialogMusicScan *)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
    if (scanner && !scanner->IsScanning())
      scanner->StartScanning("");
  }
}

void CApplication::CheckPlayingProgress()
{
  // check if we haven't rewound past the start of the file
  if (IsPlaying())
  {
    int iSpeed = g_application.GetPlaySpeed();
    if (iSpeed < 1)
    {
      iSpeed *= -1;
      int iPower = 0;
      while (iSpeed != 1)
      {
        iSpeed >>= 1;
        iPower++;
      }
      if (g_infoManager.GetPlayTime() / 1000 < iPower)
      {
        g_application.SetPlaySpeed(1);
        g_application.SeekTime(0);
      }
    }
  }
}

bool CApplication::ProcessAndStartPlaylist(const CStdString& strPlayList, CPlayList& playlist, int iPlaylist)
{
  CLog::Log(LOGDEBUG,"CApplication::ProcessAndStartPlaylist(%s, %i)",strPlayList.c_str(), iPlaylist);

  // initial exit conditions
  // no songs in playlist just return
  if (playlist.size() == 0)
    return false;

  // illegal playlist
  if (iPlaylist < PLAYLIST_MUSIC || iPlaylist > PLAYLIST_VIDEO)
    return false;

  // setup correct playlist
  g_playlistPlayer.ClearPlaylist(iPlaylist);

  // if the playlist contains an internet stream, this file will be used
  // to generate a thumbnail for musicplayer.cover
  g_application.m_strPlayListFile = strPlayList;

  // add the items to the playlist player
  g_playlistPlayer.Add(iPlaylist, playlist);

  // if we have a playlist
  if (g_playlistPlayer.GetPlaylist(iPlaylist).size())
  {
    // start playing it
    g_playlistPlayer.SetCurrentPlaylist(iPlaylist);
    g_playlistPlayer.Reset();
    g_playlistPlayer.Play();

    //show visualization
    g_application.getApplicationMessenger().SwitchToFullscreen();
    return true;
  }
  return false;
}

void CApplication::StartFtpEmergencyRecoveryMode()
{
#ifdef HAS_FTP_SERVER
  m_pFileZilla = new CXBFileZilla(NULL);
  m_pFileZilla->Start();

  // Default settings
  m_pFileZilla->mSettings.SetMaxUsers(0);
  m_pFileZilla->mSettings.SetWelcomeMessage("XBMC emergency recovery console FTP.");

  // default user
  CXFUser* pUser;
  m_pFileZilla->AddUser("xbox", pUser);
  pUser->SetPassword("xbox");
  pUser->SetShortcutsEnabled(false);
  pUser->SetUseRelativePaths(false);
  pUser->SetBypassUserLimit(false);
  pUser->SetUserLimit(0);
  pUser->SetIPLimit(0);
  pUser->AddDirectory("/", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS | XBDIR_HOME);
  pUser->AddDirectory("C:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
  pUser->AddDirectory("D:\\", XBFILE_READ | XBDIR_LIST | XBDIR_SUBDIRS);
  pUser->AddDirectory("E:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
  pUser->AddDirectory("Q:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
  //Add. also Drive F/G
  if (CIoSupport::DriveExists('F')){
    pUser->AddDirectory("F:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
  }
  if (CIoSupport::DriveExists('G')){
    pUser->AddDirectory("G:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
  }
  pUser->CommitChanges();
#endif
}

void CApplication::SaveCurrentFileSettings()
{
  if (m_itemCurrentFile->IsVideo())
  {
    // save video settings
    if (g_stSettings.m_currentVideoSettings != g_stSettings.m_defaultVideoSettings)
    {
      CVideoDatabase dbs;
      dbs.Open();
      dbs.SetVideoSettings(m_itemCurrentFile->m_strPath, g_stSettings.m_currentVideoSettings);
      dbs.Close();
    }
  }
}

CProfile* CApplication::InitDirectories()
{
  CProfile* profile = NULL;

  // only the InitDirectories* for the current platform should return
  // non-null (if at all i.e. to set a profile)
  // putting this before the first log entries saves another ifdef for g_stSettings.m_logFolder
  profile = InitDirectoriesLinux();
  if (!profile)
    profile = InitDirectoriesOSX();
  if (!profile)
    profile = InitDirectoriesWin32();
 
  return profile;
}

// TODO - Remove this function when Boxee start using a real screensaver (and not slideshow)
void CApplication::SetInSlideshowScreensaver(bool inSlideshowScreensaver)
{
  m_inSlideshowScreensaver = inSlideshowScreensaver;
  
  CLog::Log(LOGDEBUG,"CApplication::SetmInSlideshowScreensaver - m_inSlideshowScreensaver was set to [%d] (ssm)",m_inSlideshowScreensaver);

  // Notify libBoxee that we are in/out of Slideshow Screensaver.
  BOXEE::Boxee::GetInstance().SetInScreenSaver(m_inSlideshowScreensaver);
}

void CApplication::RemoveOldUpdatePackages()
{
  CLog::Log(LOGDEBUG,"CApplication::RemoveOldUpdatePackages - Enter function (update)");
  
  CFileItemList items;
  CStdString packagesDirPath;
  
  DIRECTORY::CDirectory::GetDirectory(_P("special://home/boxee/packages"), items);
  
  CLog::Log(LOGDEBUG,"CApplication::RemoveOldUpdatePackages - Call to GetDirectory for [path=%s] returned [%d] items (update)",packagesDirPath.c_str(),items.Size());
  
  for (int i = 0; i < items.Size(); i++)
  {
    if(items[i]->m_bIsFolder)
    {
      // We found a folder under packagesDirPath
      
      CStdString dirPath = items[i]->m_strPath;
      
      CUtil::RemoveSlashAtEnd(dirPath);
      
      CURI url(dirPath);
      char cSep = url.GetDirectorySeparator();
      
      int iPos = dirPath.ReverseFind(cSep);
      
      if (iPos >= 0)
      {
        // We found the folder name
        
        CStdString dirName = dirPath.Right(dirPath.size()-iPos-1);
        
        CLog::Log(LOGDEBUG,"CApplication::RemoveOldUpdatePackages - Going to check if directory [%s] is the current version [BoxeeVersion=%s] (update)",dirName.c_str(),(g_infoManager.GetVersion()).c_str());
        
        // Check if the folder name is the current Boxee version
        
        if(dirName == g_infoManager.GetVersion())
        {
          // The folder name is the current Boxee version -> Boxee was updated to this version -> Remove it
          
          CUtil::WipeDir(dirPath);
          
          CLog::Log(LOGDEBUG,"CApplication::RemoveOldUpdatePackages - Directory [%s] was deleted because Boxee was already updated to this version [BoxeeVersion=%s] (update)",dirPath.c_str(),(g_infoManager.GetVersion()).c_str());
        }
        else if(dirName.Find("_tmp") != (-1))
        {
          // The folder name contain "_tmp" -> Should not be (left from update download) -> Remove it
          
          CUtil::WipeDir(dirPath);
          
          CLog::Log(LOGDEBUG,"CApplication::RemoveOldUpdatePackages - Directory with _tmp was deleted [%s]. [BoxeeVersion=%s] (update)",dirPath.c_str(),(g_infoManager.GetVersion()).c_str());
        }
      }
    }
  }
}

// TODO - Remove this function when Boxee start using a real screensaver (and not slideshow)
bool CApplication::GetInSlideshowScreensaver()
{
  return m_inSlideshowScreensaver;  
}

bool CApplication::AlwaysProcess(const CAction& action)
{
  // check if this button is mapped to a built-in function
  if (action.strAction)
  {
    CStdString builtInFunction;
    vector<CStdString> params;
    CUtil::SplitExecFunction(action.strAction, builtInFunction, params);
    builtInFunction.ToLower();

    // should this button be handled normally or just cancel the screensaver?
    if (   builtInFunction.Equals("powerdown")
        || builtInFunction.Equals("reboot")
        || builtInFunction.Equals("restart")
        || builtInFunction.Equals("restartapp")
        || builtInFunction.Equals("suspend")
        || builtInFunction.Equals("hibernate")
        || builtInFunction.Equals("quit")
        || builtInFunction.Equals("shutdown"))
    {
      return true;
    }
  }

  return false;
}

CApplicationMessenger& CApplication::getApplicationMessenger()
{
   return m_applicationMessenger;
}

BoxeeAuthenticator& CApplication::GetBoxeeAuthenticator()
{
  return m_boxeeAuthenticator;
}

CFileScanner& CApplication::GetBoxeeFileScanner()
{
  return m_FileScanner;
}

CBoxeeLoginManager& CApplication::GetBoxeeLoginManager()
{
  return m_BoxeeLoginManager;
}

CBoxeeSocialUtilsManager& CApplication::GetBoxeeSocialUtilsManager()
{
  return m_BoxeeSocialUtilsManager;
}

CBoxeeSocialUtilsUIManager& CApplication::GetBoxeeSocialUtilsUIManager()
{
  return m_BoxeeSocialUtilsUIManager;
}


CBoxeeItemsHistory& CApplication::GetBoxeeItemsHistoryList()
{
  return m_BoxeeItemsHistory;
}

CBoxeeBrowserHistory& CApplication::GetBoxeeBrowseHistoryList()
{
  return m_BoxeeBrowserHistory;
}

CBoxeeDeviceManager& CApplication::GetBoxeeDeviceManager()
{
  return m_boxeeDeviceManager;
}

CHttpServer* CApplication::GetHttpServer()
{
  return m_httpServer;
}

void CApplication::BoxeePostLoginInitializations()
{
  CLog::Log(LOGDEBUG,"CApplication::BoxeePostLoginInitializations - Enter function (login)");

  m_httpCache.Deinitialize();

  m_tumbnailsMgr.Initialize("special://home");

#ifdef CANMORE

 /*
   * these links are used to a soft links so we need to check if so, delete it and
   * create the dir
   */
  CStdString softToDir[] =
  {
    "special://profile/browser",
    "special://profile/Thumbnails",
    "special://masterprofile/Thumbnails",
    "special://home/apps",
  };

  for(size_t i=0; i<sizeof(softToDir)/sizeof(softToDir[0]); i++)
  {
    struct stat fileStat;
    int rc = lstat(_P(softToDir[i]).c_str(), &fileStat);

    CLog::Log(LOGINFO, "%s - check if it's a soft link and needs to be deleted (lstat %d) (file mode %d)",  _P(softToDir[i]).c_str(), rc, fileStat.st_mode);

    if (rc == 0 && S_ISLNK(fileStat.st_mode))
    {
       CFile::Delete(softToDir[i]);
       CLog::Log(LOGINFO, "%s is a soft link and needs to be deleted", _P(softToDir[i]).c_str());
    } 

    CDirectory::Create(softToDir[i]); 
  }
#endif

  // Load the BoxeeItemsHistory list.
  // Note: In case of error, log will be written in m_BoxeeItemsHistory.LoadFilesHistory() function
  m_BoxeeItemsHistory.LoadItemsHistory();
  m_BoxeeBrowserHistory.LoadItemsHistory();

  // Initialize RSS Manager
  if (!m_RssSourceManager.Init())
  {
    CLog::Log(LOGWARNING,"FAILED to initialize RSS Source Manager (login)");
  }
  else
  {
    CLog::Log(LOGDEBUG,"RSS Source Manager Initialized successfully (login)");
  }

  if (BOXEE::BXConfiguration::GetInstance().GetIntParam("Boxee.MetadataEngine.Enabled",1) == 1)
  {
    CLog::Log(LOGDEBUG,"CApplication::BoxeePostLoginInitializations - MetadataEngine.Enabled is enable (login)");

    // various initializations of the resolver mechanism
    CMetadataResolverVideo::InitializeVideoResolver();
    CMetadataResolverMusic::InitializeAudioResolver();

    //bool bResult = CMetadataResolverVideo::RunUnitTest();
    //CLog::Log(bResult ? LOGDEBUG : LOGERROR,"CApplication::BoxeePostLoginInitializations - VideoResolver %s UnitTest", bResult ? "PASSED" : "FAILED");

    Boxee::GetInstance().GetMetadataEngine().InitializeArtistDataMap();
    
    BoxeeUtils::InitializeLanguageToCodeMap();

    if (!m_FileScanner.Init())
    {
      CLog::Log(LOGWARNING,"FAILED to initialize Media File Scanner");
    }
    else
    {
      BOXEE::Boxee::GetInstance().StartMetadataEngine();
      m_FileScanner.Start();
    }
  }

  if (m_watchDog.IsStoped())
  {
    m_watchDog.Start();
  }

  if(!m_BoxeeSocialUtilsManager.Initialize())
  {
    CLog::Log(LOGWARNING,"CApplication::BoxeePostLoginInitializations - BoxeeSocialUtilsManager initialization FAILED (login)");
  }

  size_t n=0;
  for (n = 0; n<g_settings.m_videoSources.size(); n++)
    m_watchDog.AddPathToWatch(g_settings.m_videoSources[n].strPath);
  for (n = 0; n<g_settings.m_musicSources.size(); n++)
    m_watchDog.AddPathToWatch(g_settings.m_musicSources[n].strPath);
  for (n = 0; n<g_settings.m_pictureSources.size(); n++)
    m_watchDog.AddPathToWatch(g_settings.m_pictureSources[n].strPath);  
  
  // need to reload keyboards - this time - from the loaded profile
  m_keyboards.Load();

  BXUserProfileDatabase bu;
  bu.Init();

  CGUIWindowStateDatabase wsd;
  wsd.Init();
}

void CApplication::BoxeeUserLogoutAction()
{
  //CGUIImage::DeInitialize();
  m_watchDog.CleanWatchedPaths();
  m_BoxeeSocialUtilsManager.Reset();

  CLog::Log(LOGNOTICE, "stop rss source manager");
  m_RssSourceManager.Stop();

  CLog::Log(LOGNOTICE, "stop file scanner");
  m_FileScanner.Stop();

  CLog::Log(LOGNOTICE, "stopping watchog...");
  m_watchDog.Stop();
#ifdef INFO_PAGE  
  m_infoPage.StopThread(false);
#endif  

#ifdef HAS_DVB
  CLog::Log(LOGNOTICE, "stopping DVB manager...");
  DVBManager::GetInstance().Stop();
#endif

  CSpecialProtocol::ClearCacheMap();
}


CDBConnectionPool* CApplication::GetDBConnectionPool()
{
  return &m_dbConnectionPool;
}


CBrowserService* CApplication::GetBrowserService()
{
  return m_pBrowserService;
}

void CApplication::SetCountryCode(const CStdString countryCode)
{
  CSingleLock lock(m_countryCodeLock);

  m_currentCountry = countryCode;
  m_currentCountry.ToLower();

  CLog::Log(LOGDEBUG,"CApplication::SetCountryCode - After setting [LoginCountryCode=%s] (cc)",m_currentCountry.c_str());
}

const CStdString& CApplication::GetCountryCode()
{
  return m_currentCountry;
}

bool CApplication::IsCurrentThread() const
{
  return CThread::IsCurrentThread(m_threadID);
}

bool CApplication::IsPresentFrame()
{
#ifdef HAS_SDL // TODO:DIRECTX
  SDL_mutexP(m_frameMutex);
  bool ret = m_bPresentFrame;
  SDL_mutexV(m_frameMutex);

  return ret;
#else
  return false;
#endif
}

#if defined(HAS_LINUX_NETWORK)
CNetworkLinux& CApplication::getNetwork()
{
  return m_network;
}
#elif defined(HAS_WIN32_NETWORK)
CNetworkWin32& CApplication::getNetwork()
{
  return m_network;
}
#else
CNetwork& CApplication::getNetwork()
{
  return m_network;
}

#endif
#ifdef HAS_PERFORMANCE_SAMPLE
CPerformanceStats &CApplication::GetPerformanceStats()
{
  return m_perfStats;
}
#endif

#ifdef _WIN32
#define SET_ENV(a,b,c) SetEnvironmentVariable((a),(b))
#define UNSET_ENV(a,b,c) SetEnvironmentVariable((a),(b))
#else
#define SET_ENV setenv
#define UNSET_ENV(a,b,c) unsetenv(a)
#endif

void CApplication::SetupHttpProxy()
{
  std::string strProxyString;
  std::string strCredString;
  if (g_guiSettings.GetBool("network.usehttpproxy") && !g_guiSettings.GetString("network.httpproxyserver").IsEmpty())
  {
    if (g_guiSettings.GetString("network.httpproxyusername").length() > 0)
    {
      strCredString = g_guiSettings.GetString("network.httpproxyusername");
      strCredString += ":" + g_guiSettings.GetString("network.httpproxypassword");
    }

    strProxyString = "http://";
    if (!strCredString.empty())
      strProxyString += strCredString + "@";
      
    strProxyString += g_guiSettings.GetString("network.httpproxyserver");
    strProxyString += ":" + g_guiSettings.GetString("network.httpproxyport");
  }  
  
  if (!strProxyString.empty())
  {
    CLog::Log(LOGINFO,"setting proxy to <%s>", strProxyString.c_str());
    SET_ENV("http_proxy", strProxyString.c_str(), 1);
    SET_ENV("HTTPS_PROXY", strProxyString.c_str(), 1);
    
    SET_ENV("BOXEE_PROXY", g_guiSettings.GetString("network.httpproxyserver").c_str(), 1);
    SET_ENV("BOXEE_PROXY_PORT", g_guiSettings.GetString("network.httpproxyport").c_str(), 1);
    SET_ENV("BOXEE_PROXY_USER", g_guiSettings.GetString("network.httpproxyusername").c_str(), 1);
    SET_ENV("BOXEE_PROXY_PASS", g_guiSettings.GetString("network.httpproxypassword").c_str(), 1);
  }
  else 
  {
    UNSET_ENV("http_proxy", "", 1);
    UNSET_ENV("HTTPS_PROXY", "", 1);    
    UNSET_ENV("BOXEE_PROXY", "", 1);
    UNSET_ENV("BOXEE_PROXY_PORT", "", 1);
    UNSET_ENV("BOXEE_PROXY_USER", "", 1);
    UNSET_ENV("BOXEE_PROXY_PASS", "", 1);
  }
}


void CApplication::GetInfoPage(CStdString *str, CStdString params) {
  if (params == "d") {
    if (CLog::m_logLevel == LOGDEBUG) {
      CLog::m_logLevel = LOGWARNING;
    } else {
      CLog::m_logLevel = LOGDEBUG;
    }
  }
  *str += "Pass d to toggle debug level\n";    
  *str += "m_currentCountry= " + m_currentCountry + "\n";
  *str += "m_serverIpAddress= " + m_serverIpAddress + "\n";  
};

void CApplication::HandlePlayerErrors()
{
  if(!m_pPlayer)
    return;

  switch(m_pPlayer->GetError())
  {
  case IPLAYER_FILE_OPEN_ERROR:
    CGUIDialogOK2::ShowAndGetInput(51676, 51675);
    break;
  case IPLAYER_FORMAT_NOT_SUPPORTED:
    CGUIDialogOK2::ShowAndGetInput(51676, 51674);
    break;
  default:
    break;
  }
}

void CApplication::SetRenderingEnabled(bool enabled) 
{ 
  m_renderingEnabled = enabled; 
 
  if (enabled)
  {
    Render();
  }
  else
  {
    g_windowManager.CloseDialogs(true);
    g_graphicsContext.Clear();
    g_graphicsContext.Flip();
  }    
}

bool CApplication::OnAppMessage(const CStdString& strHandler, const CStdString& strParam)
{
  bool bHandled = false;

  if(strHandler == "mlb:chapters")
  {
    if(g_application.m_pPlayer)
    {
      bHandled = g_application.m_pPlayer->OnAppMessage(strHandler, strParam);
    }
  }

  return bHandled;
}

bool CApplication::IsPlayingStreamPlaylist()
{
  return m_pPlayer && m_pPlayer->IsPlayingStreamPlaylist();
}

double CApplication::GetStreamPlaylistTimecode()
{
  double timecode = -1;

  if(IsPlayingStreamPlaylist())
  {
    timecode = m_pPlayer->GetStreamPlaylistTimecode();
  }

  return timecode;
}

bool CApplication::ShouldConnectToInternet(bool checkNow)
{
  return (IsConnectedToInternet(checkNow) && !IsInScreenSaver() && !((IsPlayingVideo() && (GetCurrentPlayer() != EPC_FLASHPLAYER))|| IsPlayingAudio()));
}
