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
#include "../dlgcache.h"

class CFlashVideoPlayer : public IPlayer, public CThread, public IFlashPlayerListener
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
  virtual __int64 GetTime();
  virtual int GetTotalTime();
  virtual void ToFFRW(int iSpeed);
  virtual void ShowOSD(bool bOnoff);
  virtual void DoAudioWork() {}
  
  virtual CStdString GetPlayerState();
  virtual bool SetPlayerState(CStdString state);
  
  virtual void FlashPlaybackEnded();
  virtual void FlashPlaybackStarted();
  virtual void FlashNewFrame() ;
  virtual void FlashPaused();
  virtual void FlashResumed() ;
  virtual void FlashProgress(int nPct) ;
  virtual void FlashTime(int nTime) ;
  virtual void FlashDuration(int nTime) ;
  virtual void FlashConfigChange(int nWidth, int nHeight);
  virtual void FlashSetMode(FlashLibMode nMode);

  virtual void FlashNotification(const char *text, const char *thumb, int nTimeout);
  virtual void FlashEnableExt(int nID, const char *extText, const char *extThumb);
  virtual void FlashDisableExt(int nID);
  
  virtual void FlashGetTextInput(const char *title, const char *callback);
  
  virtual void FlashSetCanPause(bool canPause);
  virtual void FlashSetCanSkip(bool canSkip);
  virtual void FlashSetCanSetVolume(bool canSetVolume);

  virtual bool CanPause() const;
  virtual bool CanSkip() const;
  virtual bool CanSetVolume() const;

  virtual bool OSDDisabled()      const { return m_bBrowserMode; }
  virtual bool FullScreenLocked() const { return m_bBrowserMode; }

  virtual void Ping();

  virtual bool PreRender();
  virtual void PostRender();

  virtual bool OnAction(const CAction &action);

  virtual void OSDExtensionClicked(int nId);

private:
  virtual void Process();
  void CheckConfig();
  void ParseFlashURL(const CStdString &strUrl, std::vector<CStdString> &vars, std::vector<CStdString> &values);

  bool m_paused;
  bool m_playing;
  bool m_bConfigChanged;
  bool m_bImageLocked;
  bool m_bDirty;
  
  bool m_canPause;
  bool m_canSkip;
  bool m_canSetVolume;

  bool m_bBrowserMode;
  
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
  bool         m_bShowKeyboard;
  bool         m_bCloseKeyboard;
  CStdString   m_strKeyboardCaption;
  CStdString   m_strKeyboardCallback;
 
  CStdString   m_path;
  CCriticalSection m_lock;
};
