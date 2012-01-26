#pragma once

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

#include "IPlayer.h"
#include "utils/Thread.h"
#include "DllFlashLib.h"
#include "FlashPlayerListener.h"
#include "dvdplayer/DVDPlayer.h"
#include "paplayer/PAPlayer.h"
#include "../dlgcache.h"
#include "../../lib/libjson/include/json/json.h"
#include "MouseStat.h"
#ifdef _LINUX
#include "linux/LinuxResourceCounter.h"
#endif

#define DIRECTION_NONE 0
#define DIRECTION_UP 1
#define DIRECTION_DOWN 2
#define DIRECTION_LEFT 3
#define DIRECTION_RIGHT 4


class CFlashVideoPlayer :
    public IPlayer
  , public CThread
  , public IFlashPlayerListener
  , public IPlayerCallback  // for dvdplayer callbacks into the flash player
{
public:
  CFlashVideoPlayer(IPlayerCallback& callback);
  virtual ~CFlashVideoPlayer();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback) {}
  virtual void UnRegisterAudioCallback()                        {}
  virtual bool OpenFile(const CFileItem& file, const CPlayerOptions &options);
  virtual bool CloseFile();
  virtual bool IsPlaying() const;
  virtual void Pause();
  virtual bool IsPaused() const;
  virtual bool HasVideo() const;
  virtual bool HasAudio() const;
  virtual void ToggleOSD() { }; // empty
  virtual void SwitchToNextLanguage();
  virtual void ToggleSubtitles();
  virtual void ToggleFrameDrop();
  virtual bool CanSeek();
  virtual bool CanSeekToTime();
  virtual void Seek(bool bPlus, bool bLargeStep);
  virtual void SeekPercentage(float iPercent);
  virtual float GetPercentage();
  virtual void SetVolume(long nVolume); 
  virtual void SetDynamicRangeCompression(long drc) {}
  virtual void SetContrast(bool bPlus) {}
  virtual void SetBrightness(bool bPlus) {}
  virtual void SetHue(bool bPlus) {}
  virtual void SetSaturation(bool bPlus) {}
  virtual void GetAudioInfo(CStdString& strAudioInfo);
  virtual void GetVideoInfo(CStdString& strVideoInfo);
  virtual void GetGeneralInfo( CStdString& strVideoInfo);
  virtual void Update(bool bPauseDrawing)                       {}
  virtual void GetVideoRect(RECT& SrcRect, RECT& DestRect)      {}
  virtual void GetVideoAspectRatio(float& fAR)                  {}
  virtual void SwitchToNextAudioLanguage();
  virtual bool CanRecord() { return false; }
  virtual bool IsRecording() { return false; }
  virtual bool Record(bool bOnOff) { return false; }
  virtual void SetAVDelay(float fValue = 0.0f);
  virtual float GetAVDelay();

  void Render();

  virtual void SetSubTitleDelay(float fValue = 0.0f);
  virtual float GetSubTitleDelay();

  virtual void SeekTime(__int64 iTime);
  virtual bool CanReportGetTime();
  virtual __int64 GetTime();
  virtual int GetTotalTime();
  virtual void ToFFRW(int iSpeed);
  virtual void ShowOSD(bool bOnoff);
  virtual void DoAudioWork() {}
  virtual CStdString GetPlayerState();
  virtual bool SetPlayerState(CStdString state);
  
  virtual void FlashProcessCommand(const char *command);
  virtual void FlashNewFrame() ;

  virtual void FlashPlaybackEnded();
  void InternalFlashPlaybackEnded(bool bIssueMessage);


  virtual bool CanPause() const;
  virtual bool CanSkip() const;
  virtual bool CanSetVolume() const;

  virtual bool OSDDisabled()      const { return m_bDrawMouse; }

  virtual void Ping();

  virtual bool PreRender();
  virtual void PostRender();

  virtual bool OnAction(const CAction &action);
  virtual void OSDExtensionClicked(int nId);

  virtual void ProcessExternalMessage(/*ThreadMessage */  void *pMsg);

  //Returns true if not playback (paused or stopped beeing filled)
  virtual bool IsCaching() const;

  //Cache filled in Percent
  virtual int GetCacheLevel() const;

  virtual bool ForceMouseRendering();
  virtual bool MouseRenderingEnabled();
  virtual bool KeyboardPassthrough() {return true;}
  
  // IPlayerCallback calls from dvdplayer for html5 video
  virtual void OnPlayBackEnded(bool bError = false, const CStdString& error = "");
  virtual void OnPlayBackStarted();// { printf("ONPLAYBACKSTARTED\n"); }
  virtual void OnPlayBackStopped() { printf("ONPLAYBACKSTOPPED\n"); }
  virtual void OnQueueNextItem()   { printf("ONQUEUENEXTITEM\n");   }
  virtual void OnPlayBackSeek(int iTime, int seekOffset);

  virtual void OnPlayBackResumed();
  virtual void OnPlayBackPaused();

  bool LoadURL(const CStdString& url, const CRect &rect);

private:
  void CloseOSD();
  virtual void Process();
  void CheckConfig();
  void ParseFlashURL(const CStdString &strUrl, std::vector<CStdString> &vars, std::vector<CStdString> &values);
  void UpdateMouseSpeed(int nDirection);
  void MoveMouse(int iX, int iY);
  bool TranslateMouseCoordinates(float &x, float &y);
  bool PrintPlayerInfo();
  bool Launch(const CFileItem& file, const CPlayerOptions &options);

  bool m_paused;
  bool m_playing;
  bool m_bConfigChanged;
  bool m_bImageLocked;
  bool m_bDirty;

  bool m_canPause;
  bool m_canSkip;
  bool m_canSkipTo;
  bool m_canSetVolume;

  bool m_bDrawMouse;
  bool m_bMouseActiveTimeout;
  bool m_bFullScreenPlayback;

  int  m_loadingPct;

  bool    m_clockSet;
  __int64 m_clock;
  int     m_pct;
  DWORD m_lastTime;
  int m_speed;

  int          m_totalTime;
  DllFlashLib  m_dll;
  FW_HANDLE    m_handle;
  int          m_width;
  int          m_height;
  int          m_cropTop;
  int          m_cropBottom;
  int          m_cropLeft;
  int          m_cropRight;
  bool         m_isFullscreen;

  CDlgCache   *m_pDlgCache;
  //IpcEventWaiter m_ipc_waiter;
  bool         m_userStopped;
  bool         m_bUserRequestedToStop;
  bool         m_bFlashPlaybackEnded;
  bool         m_bCloseKeyboard;

  bool         m_bLockedPlayer;
  bool         m_bWebControl;
  CRect        m_rect;

  CStdString        m_path;
  CStdString        m_mode;
  CStdStringArray   m_modesStack;
  
  CCriticalSection m_lock;

  unsigned int m_lastMoveTime;
  float m_fSpeed;
  int m_nDirection;
  float m_fMaxSpeed;
  float m_fAcceleration;
  bool  m_bMouseMoved;
  unsigned int  m_lastMoveUpdateTime;
  
  unsigned int m_menuCommandSentTime;
  bool         m_bUnAckedMenuCommand;

  int m_screenX1, m_screenX2, m_screenY1, m_screenY2, m_screenWidth, m_screenHeight;
  MOUSE_STATE m_mouseState;

#ifdef _LINUX
  CLinuxResourceCounter m_resourceCounter;
#endif

  double m_scaleX, m_scaleY;
  // html5 video playback
  struct
  {
    IPlayer* pPlayer;
    CFileItem item;
    __int64 currentTime;
    bool paused;
    bool muted;
    int preMuteVolume;
    bool opening;
    int width, height;
    int buffered;
    bool isBuffering;
    bool loaded;
    int playerID;
    void clear()
    {
      if( pPlayer )
      {
        pPlayer->CloseFile();
        delete pPlayer;
        pPlayer = NULL;
      }
      item.Reset();
      currentTime = -1;
      paused = true;
      muted = false;
      preMuteVolume = 0;
      opening = false;
      width = height = 0;
      buffered = 0;
      isBuffering = true;
      loaded = false;
      playerID = 0;
    }
  } m_mediaPlayerInfo;

  CStdString m_urlSource;
  CStdString m_sQtPluginPath;
  __int64 m_seekTimeRequest;
  unsigned int m_lastSeekTimeUpdateTime;
};
