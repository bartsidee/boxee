#pragma once

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

#include <queue>

#include "system.h" // for HAS_DVD_DRIVE et. al.
#include "XBApplicationEx.h"

#include "IMsgTargetCallback.h"

class CFileItem;
class CFileItemList;

#include "GUIDialogSeekBar.h"
#include "GUIDialogKaiToast.h"
#include "GUIDialogVolumeBar.h"
#include "GUIDialogMuteBug.h"
#include "GUIWindowPointer.h"   // Mouse pointer

#include "cores/IPlayer.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "PlayListPlayer.h"
#if !defined(_WIN32) && defined(HAS_DVD_DRIVE)
#include "DetectDVDType.h"
#endif
#include "Autorun.h"
#include "Bookmark.h"
#include "utils/Stopwatch.h"
#include "ApplicationMessenger.h"
#include "utils/Network.h"
#include "utils/CharsetConverter.h"
#ifdef HAS_PERFORMANCE_SAMPLE
#include "utils/PerformanceStats.h"
#endif
#ifdef _LINUX
#include "linux/LinuxResourceCounter.h"
#endif
#include "XBMC_events.h"
#include "utils/Thread.h"

#include "RssSourceManager.h"
#include "FileScanner.h"

#include "utils/PerformanceStats.h"

//Boxee
#include "WatchDog.h"
#include "InfoPage.h"
#include "BoxeeItemsHistory.h"
#include "BoxeeLoginManager.h"
#include "DynamicDll.h"
#ifdef APP_JS
#include "XBJavaScript.h"
#endif
#include "BoxeeAuthenticator.h"

#include "utils/DBConnectionPool.h"
#include "KeyboardManager.h"

//end Boxee

#include "HttpCacheManager.h"

#ifdef HAS_DVD_DRIVE
using namespace MEDIA_DETECT;
#endif
using namespace MUSIC_INFO;

#ifdef HAS_SDL
#include <SDL/SDL_mutex.h>
#endif

class CWebServer;
class CXBFileZilla;
class CSNTPClient;
class CKaraokeLyricsManager;
class CApplicationMessenger;
class XBPython;
class DPMSSupport;
class CBrowserService;
class CProfile;
class CSplash;
class CItemLoader;

#ifdef _WIN32
class CRemoteWrapper;
#endif

class CBackgroundPlayer : public CThread
{
public:
  CBackgroundPlayer(const CFileItem &item, int iPlayList);
  virtual ~CBackgroundPlayer();
  virtual void Process();
protected:
  CFileItem *m_item;
  int       m_iPlayList;
};

class CApplication : public CXBApplicationEx, public IPlayerCallback, public IMsgTargetCallback, public InfoPageble
{
public:
  CApplication(void);
  virtual ~CApplication(void);
  virtual HRESULT Initialize();
  virtual void FrameMove();
  virtual void Render();
  virtual void DoRender();
  virtual void RenderNoPresent();
  virtual void Preflight();
  virtual HRESULT Create(HWND hWnd);
  virtual HRESULT Cleanup();
  void PostLoginInitializations();
  void StartServices();
  void StopServices();
  void StartWebServer();
  void StopWebServer(bool bWait);
  void StartFtpServer();
  void StopFtpServer();
  void StartTimeServer();
  void StopTimeServer();
  void StartUPnP();
  void StopUPnP(bool bWait);
  void StartUPnPRenderer();
  void StopUPnPRenderer();
  void StartUPnPClient();
  void StopUPnPClient();
  void StartUPnPServer();
  void StopUPnPServer();
  void StartEventServer();
  bool StopEventServer(bool bWait, bool promptuser);
  void RefreshEventServer();
  void StartDbusServer();
  bool StopDbusServer(bool bWait);
  void StartZeroconf();
  void StopZeroconf();
  void DimLCDOnPlayback(bool dim);
  bool IsCurrentThread() const;
  void Stop();
  void RestartApp();
  void LoadSkin(const CStdString& strSkin);
  void UnloadSkin();
  bool LoadUserWindows(const CStdString& strSkinPath);
  void DelayLoadSkin();
  void CancelDelayLoadSkin();
  void ReloadSkin();
  const CStdString& CurrentFile();
  CFileItem& CurrentFileItem();
  virtual bool OnMessage(CGUIMessage& message);
  PLAYERCOREID GetCurrentPlayer();
  virtual void OnPlayBackEnded(bool bError = false, const CStdString& error = "");
  virtual void OnPlayBackStarted();
  virtual void OnPlayBackStopped();
  virtual void OnQueueNextItem();
  bool PlayMedia(const CFileItem& item, int iPlaylist = PLAYLIST_MUSIC);
  bool PlayMediaSync(const CFileItem& item, int iPlaylist = PLAYLIST_MUSIC);
  bool ProcessAndStartPlaylist(const CStdString& strPlayList, PLAYLIST::CPlayList& playlist, int iPlaylist);
  bool PlayFile(const CFileItem& item, bool bRestart = false);
  void SaveFileState();
  void MarkVideoAsWatched(const CStdString& path, const CStdString& boxeeId,const double videoTime)const;
  void UpdateFileState();
  void StopPlaying();
  void Restart(bool bSamePosition = true);
  void DelayedPlayerRestart();
  void CheckDelayedPlayerRestart();
  void RenderFullScreen();
  void DoRenderFullScreen();
  bool NeedRenderFullScreen();
  bool IsPlaying() const ;
  bool IsPaused() const;
  bool IsPlayingAudio() const ;
  bool IsPlayingVideo() const ;
  bool IsCanPause() const;
  bool IsCanSkip() const;
  bool IsCanSetVolume() const;
  
  bool IsPlayingFullScreenVideo() const ;
  bool IsStartingPlayback() const { return m_bPlaybackStarting; }
  bool OnKey(CKey& key);
  bool OnAction(CAction &action);
  void RenderMemoryStatus();
  void CheckShutdown();
  void CheckDisplaySleep();
  // Checks whether the screensaver and / or DPMS should become active.
  void CheckScreenSaverAndDPMS();
  void CheckPlayingProgress();
  void CheckAudioScrobblerStatus();
  void ActivateScreenSaver(bool forceType = false);

  virtual void Process();
  void ProcessSlow();
  void ResetScreenSaver();
  int GetVolume() const;
  void SetVolume(int iPercent);

  bool IsOfflineMode() const;
  void SetOfflineMode(bool bOffLineMode);

  void Mute(void);
  int GetPlaySpeed() const;
  int GetSubtitleDelay() const;
  int GetAudioDelay() const;
  void SetPlaySpeed(int iSpeed);
  void ResetScreenSaverTimer();
  // Wakes up from the screensaver and / or DPMS. Returns true if woken up.
  bool WakeUpScreenSaverAndDPMS();
  bool WakeUpScreenSaver();
  double GetTotalTime() const;
  double GetTime() const;
  float GetPercentage() const;
  float GetPercentageWithCache() const;
  void SeekPercentage(float percent);
  void SeekTime( double dTime = 0.0 );
  void ResetPlayTime();

  void SaveMusicScanSettings();
  void RestoreMusicScanSettings();
  void UpdateLibraries();
  void CheckMusicPlaylist();
  
  bool ExecuteXBMCAction(std::string action);
  bool ExecuteAction(CGUIActionDescriptor action);

  static bool OnEvent(XBMC_Event& newEvent);
  

  CApplicationMessenger& getApplicationMessenger();
#if defined(HAS_LINUX_NETWORK)
  CNetworkLinux& getNetwork();
#elif defined(HAS_WIN32_NETWORK)
  CNetworkWin32& getNetwork();
#else
  CNetwork& getNetwork();
#endif
#ifdef HAS_PERFORMANCE_SAMPLE
  CPerformanceStats &GetPerformanceStats();
#endif

  InfoPage m_infoPage;
  CGUIDialogVolumeBar m_guiDialogVolumeBar;
  CGUIDialogSeekBar m_guiDialogSeekBar;
  CGUIDialogKaiToast m_guiDialogKaiToast;
  CGUIDialogMuteBug m_guiDialogMuteBug;
  CGUIWindowPointer m_guiPointer;

#ifdef HAS_DVD_DRIVE  
  MEDIA_DETECT::CAutorun m_Autorun;
#endif
  
#if !defined(_WIN32) && defined(HAS_DVD_DRIVE)
  MEDIA_DETECT::CDetectDVDMedia m_DetectDVDType;
#endif
  CSNTPClient *m_psntpClient;
  CWebServer* m_pWebServer;
  CXBFileZilla* m_pFileZilla;
  IPlayer* m_pPlayer;

  inline bool IsInScreenSaver() { return m_bScreenSave; };
  int m_iScreenSaveLock; // spiff: are we checking for a lock? if so, ignore the screensaver state, if -1 we have failed to input locks

  unsigned int m_skinReloadTime;
  bool m_bIsPaused;
  bool m_bPlaybackStarting;

  std::queue<CGUIMessage> m_vPlaybackStarting;

  CKaraokeLyricsManager* m_pKaraokeMgr;

  PLAYERCOREID m_eForcedNextPlayer;
  CStdString m_strPlayListFile;

  int GlobalIdleTime();
  void NewFrame();
  bool WaitFrame(unsigned int timeout);

//Boxee
  CRssSourceManager& GetRssSourceManager()
  {
    return m_RssSourceManager;
  }

  CItemLoader& GetItemLoader()
  {
	  return *m_ItemLoader;
  }

  inline bool IsConnectedToNet() { return m_network.IsConnected(); }
  inline bool IsConnectedToServer() { return m_watchDog.IsConnectedToServer(); }
  inline bool IsConnectedToInternet() { return m_watchDog.IsConnectedToInternet(); }
  inline bool IsPathAvailable(const CStdString &stdPath, bool bDefault=true) { return m_watchDog.IsPathAvailable(stdPath, bDefault); }
  inline void AddPathToWatch(const CStdString &stdPath) { m_watchDog.AddPathToWatch(stdPath); }
  inline void AddListenerToWatchDog(IWatchDogListener *listener) { m_watchDog.AddListener(listener); }
  inline void RemoveListenerFromWatchDog(IWatchDogListener *listener) { m_watchDog.RemoveListener(listener); }

  inline void SetVerbose(BOOL bVerbose) { m_bVerbose = bVerbose; }
  inline BOOL IsVerbose() { return m_bVerbose; }
  CBoxeeItemsHistory& GetBoxeeItemsHistoryList();
  
  XBPython &GetPythonManager();
  XBMC::CHttpCacheManager &GetHttpCacheManager() { return m_httpCache; }
  
#ifdef APP_JS
  XBJavaScript &GetJavaScriptManager();
#endif  
  
  // Deletes the boxee database file and clears all thumbnails
  void DeleteDatabaseAndThumbnails();
 
  void SetHomeScreenTimerOnStatus(bool status);

  void SetUserLoggedIn(bool userLoggedIn);
  bool IsUserLoggedIn();
  
  CProfile* InitDirectories();

  // TODO - Remove this function when Boxee start using a real screensaver (and not slideshow)
  void SetInSlideshowScreensaver(bool inSlideshowScreensaver);
  // TODO - Remove this function when Boxee start using a real screensaver (and not slideshow)
  bool GetInSlideshowScreensaver();

  void RemoveOldThumbnails(bool forceCheck, bool showConfirmationDialog);

  void SetCountryCode(const CStdString countryCode);

  const CStdString& GetCountryCode();

  CBoxeeLoginManager& GetBoxeeLoginManager();
  void BoxeePostLoginInitializations();
  void BoxeeUserLogoutAction();
  CDBConnectionPool* GetDBConnectionPool();
  CBrowserService*  GetBrowserService();
//end Boxee

  void EnablePlatformDirectories(bool enable=true)
  {
    m_bPlatformDirectories = enable;
  }

  bool PlatformDirectoriesEnabled()
  {
    return m_bPlatformDirectories;
  }

  void SetStandAlone(bool value)
  {
    m_bStandalone = value;
  }

  bool IsStandAlone()
  {
    return m_bStandalone;
  }

  void SetEnableLegacyRes(bool value)
  {
    m_bEnableLegacyRes = value;
  }

  bool IsEnableLegacyRes()
  {
    return m_bEnableLegacyRes;
  }

  BoxeeAuthenticator& GetBoxeeAuthenticator();
  CFileScanner& GetBoxeeFileScanner();
  
  bool IsPresentFrame();

  void Minimize();

  bool m_bRunResumeJobs;

  void DeferAction(CAction action);
  
  std::string GetBoxeeServerIP();
  void SetBoxeeServerIP(const std::string &ip);

  void SetupHttpProxy();
  
  XBMC::KeyboardManager &GetKeyboards() { return m_keyboards; }
  
  void StackedMovieBackSeekHandler(__int64 seek);
  bool PlayingStackedMovie();
protected:
  void RenderScreenSaver();

  friend class CApplicationMessenger;
  // screensaver
  bool m_bInactive;
  bool m_bDisplaySleeping;
  bool m_bScreenSave;
  bool m_bOffLineMode; //Off-Line mode flag
  CStdString m_screenSaverMode;

#ifdef __APPLE__
  DWORD m_dwOSXscreensaverTicks;
#endif

  // timer information
  CStopWatch m_idleTimer;
  CStopWatch m_restartPlayerTimer;
  CStopWatch m_frameTime;
  CStopWatch m_navigationTimer;
  CStopWatch m_slowTimer;
  CStopWatch m_screenSaverTimer;
  CStopWatch m_shutdownTimer;

  DPMSSupport* m_dpms;
  bool m_dpmsIsActive;

  CFileItemPtr m_itemCurrentFile;
  CFileItemList* m_currentStack;
  CStdString m_prevMedia;
  CSplash* m_splash;
  ThreadIdentifier m_threadID;       // application thread ID.  Used in applicationMessanger to know where we are firing a thread with delay from.
  PLAYERCOREID m_eCurrentPlayer;
  bool m_bXboxMediacenterLoaded;
  bool m_bSettingsLoaded;
  bool m_bAllSettingsLoaded;
  bool m_bInitializing;
  bool m_bPlatformDirectories;

  CBookmark m_progressTrackingVideoResumeBookmark;
  CFileItemPtr m_progressTrackingItem;
  bool m_progressTrackingPlayCountUpdate;

  int m_iPlaySpeed;
  int m_currentStackPosition;
  int m_nextPlaylistItem;

  bool m_bPresentFrame;

  bool m_bStandalone;
  bool m_bEnableLegacyRes;
  bool m_bSystemScreenSaverEnable;
#ifdef HAS_SDL
  int        m_frameCount;
  SDL_mutex* m_frameMutex;
  SDL_cond*  m_frameCond;
#endif

  static LONG WINAPI UnhandledExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo);

  void SetHardwareVolume(long hardwareVolume);
  void UpdateLCD();
  void FatalErrorHandler(bool WindowSystemInitialized, bool MapDrives, bool InitNetwork);

  bool PlayStack(const CFileItem& item, bool bRestart);
  bool SwitchToFullScreen();
  bool ProcessMouse();
  bool ProcessHTTPApiButtons();
  bool ProcessKeyboard();
  bool ProcessRemote(float frameTime);
  bool ProcessGamepad(float frameTime);
  bool ProcessEventServer(float frameTime);
  bool ProcessDeferredActions();
  bool ProcessJoystickEvent(const std::string& joystickName, int button, bool isAxis, float fAmount);

  void StartFtpEmergencyRecoveryMode();
  float NavigationIdleTime();
  void CheckForTitleChange();
  static bool AlwaysProcess(const CAction& action);

  void SaveCurrentFileSettings();

  CProfile* InitDirectoriesLinux();
  CProfile* InitDirectoriesOSX();
  CProfile* InitDirectoriesWin32();

//Boxee
  virtual void GetInfoPage(CStdString *str, CStdString params);
  void CheckRefreshHomeScreen();
  bool IsHomeScreenTimerOn();
  
  void RemoveOldUpdatePackages();
  
//end Boxee

  CApplicationMessenger m_applicationMessenger;

  CRssSourceManager m_RssSourceManager;
  CItemLoader  *m_ItemLoader;
  CFileScanner m_FileScanner;

#if defined(HAS_LINUX_NETWORK)
  CNetworkLinux m_network;
#elif defined(HAS_WIN32_NETWORK)
  CNetworkWin32 m_network;
#else
  CNetwork    m_network;
#endif
#ifdef HAS_PERFORMANCE_SAMPLE
  CPerformanceStats m_perfStats;
#endif
#ifdef _LINUX
  CLinuxResourceCounter m_resourceCounter;
#endif

#ifdef _WIN32
  CRemoteWrapper* m_RemoteControl;
#endif

#ifdef HAS_EVENT_SERVER
  std::map<std::string, std::map<int, float> > m_lastAxisMap;
#endif
//Boxee
  WatchDog m_watchDog;
  BOOL     m_bVerbose;
  CBoxeeItemsHistory m_BoxeeItemsHistory;
  XBPython *m_pPythonManager;
  CBoxeeLoginManager m_BoxeeLoginManager;
#ifdef APP_JS	
  XBJavaScript *m_pJavaScriptManager;
#endif	
  CCriticalSection m_playerLock;
  
  std::vector<DllDynamic *> m_lockedLibraries; // locked to memory - never unloaded till shutdown
  
  bool m_homeScreenTimerOn;
  DWORD m_dwRefreshHomeScreenTick;
  
  bool m_userLoggedIn;
  
  bool m_inSlideshowScreensaver;
  
  BoxeeAuthenticator m_boxeeAuthenticator;
  
  CStdString m_currentCountry;
  
  std::queue<CAction> m_deferredActions;
  
  CCriticalSection m_ipLock;
  CStdString m_serverIpAddress;

  CDBConnectionPool m_dbConnectionPool;
  CBrowserService* m_pBrowserService;

  XBMC::CHttpCacheManager m_httpCache;
  XBMC::KeyboardManager m_keyboards;
  //end boxee
};

extern CApplication g_application;
extern CStdString g_LoadErrorStr;

