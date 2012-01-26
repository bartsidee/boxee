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

#include "IAudioCallback.h"
#include "Key.h"

struct TextCacheStruct_t;
class TiXmlElement;
class CStreamDetails;

class IPlayerCallback
{
public:
  virtual ~IPlayerCallback() {}
  virtual void OnPlayBackEnded(bool bError = false, const CStdString& error = "") = 0;
  virtual void OnPlayBackStarted() = 0;
  virtual void OnPlayBackStopped() = 0;
  virtual void OnQueueNextItem() = 0;
  virtual void OnPlayBackPaused() {};
  virtual void OnPlayBackResumed() {};
  virtual void OnPlayBackSeek(int iTime, int seekOffset) {};
  virtual void OnPlayBackSeekChapter(int iChapter) {};
  virtual void OnPlayBackSpeedChanged(int iSpeed) {};
};

class CPlayerOptions
{
public:
  CPlayerOptions()
  {
    starttime = 0LL;
    startpercent = 0LL;
    identify = false;
    fullscreen = false;
    video_only = false;
  }
  double  starttime; /* start time in seconds */
  double  startpercent; /* start time in percent between 0-1 */
  bool    identify;  /* identify mode, used for checking format and length of a file */
  CStdString state;  /* potential playerstate to restore to */
  bool    fullscreen; /* player is allowed to switch to fullscreen */
  bool    video_only; /* player is not allowed to play audio streams, video streams only */
};

class CFileItem;
class CRect;

// define some possible errors for IPlayer
typedef enum {
   IPLAYER_SUCCESS               =  0,
   IPLAYER_FILE_OPEN_ERROR       =  1,
   IPLAYER_FORMAT_NOT_SUPPORTED  =  2,
   IPLAYER_ERROR_UNSPECIFIED     =  99
} iplayer_error;


class IPlayer
{
public:
  IPlayer(IPlayerCallback& callback): m_callback(callback){};
  virtual ~IPlayer(){};
  virtual bool Initialize(TiXmlElement* pConfig) { return true; };
  virtual void RegisterAudioCallback(IAudioCallback* pCallback) = 0;
  virtual void UnRegisterAudioCallback() = 0;
  virtual bool OpenFile(const CFileItem& file, const CPlayerOptions& options){ return false;}
  virtual bool QueueNextFile(const CFileItem &file) { return false; }
  virtual void OnNothingToQueueNotify() {}
  virtual bool CloseFile(){ return true;}
  virtual bool IsPlaying() const { return false;} 
  virtual void Pause() = 0;
  virtual bool IsPaused() const = 0;
  virtual bool HasVideo() const = 0;
  virtual bool HasAudio() const = 0;
  virtual bool HasLiveTV() const { return false; }
  virtual bool IsPassthrough() const { return false;}
  virtual bool CanSeek() {return true;}
  virtual void Seek(bool bPlus = true, bool bLargeStep = false) = 0;
  virtual bool SeekScene(bool bPlus = true) {return false;}
  virtual void SeekPercentage(float fPercent = 0){}
  virtual float GetPercentage(){ return 0;}
  virtual float GetPercentageWithCache(){ return GetPercentage();}
  virtual int GetPositionWithCache() { return 0;}
  virtual void SetVolume(long nVolume){}
  virtual void SetDynamicRangeCompression(long drc){}
  virtual void GetAudioInfo( CStdString& strAudioInfo) = 0;
  virtual void GetVideoInfo( CStdString& strVideoInfo) = 0;
  virtual void GetGeneralInfo( CStdString& strVideoInfo) = 0;
  virtual void Update(bool bPauseDrawing = false) = 0;
  virtual void GetVideoRect(CRect& SrcRect, CRect& DestRect) {}
  virtual void GetVideoAspectRatio(float& fAR) { fAR = 1.0f; }
  virtual bool CanRecord() { return false;};
  virtual bool IsRecording() { return false;};
  virtual bool Record(bool bOnOff) { return false;};
  virtual bool IsDirectRendering() { return false;};

  virtual bool CanSeekToTime() {return true;}

  virtual void  SetAVDelay(float fValue = 0.0f) { return; }
  virtual float GetAVDelay()                    { return 0.0f;};

  virtual void SetSubTitleDelay(float fValue = 0.0f){};
  virtual float GetSubTitleDelay()    { return 0.0f; }
  virtual int  GetSubtitleCount()     { return 0; }
  virtual int  GetSubtitle()          { return -1; }
  virtual void GetSubtitleName(int iStream, CStdString &strStreamName){};
  virtual void GetSubtitleLang(int iStream, CStdString &strStreamLang){};
  virtual void SetSubtitle(int iStream){};
  virtual bool GetSubtitleVisible(){ return false;};
  virtual void SetSubtitleVisible(bool bVisible){};
  virtual bool GetSubtitleForced(){ return false;};
  virtual void SetSubtitleForced(bool bVisible){};
  virtual bool GetSubtitleExtension(CStdString &strSubtitleExtension){ return false;};
  virtual bool AddSubtitle(const CStdString& strSubPath) {return false;};

  virtual int  GetAudioStreamCount()  { return 0; }
  virtual int  GetAudioStream()       { return -1; }
  virtual void GetAudioStreamName(int iStream, CStdString &strStreamName){};
  virtual void GetAudioStreamLang(int iStream, CStdString &strStreamLang){};
  virtual void SetAudioStream(int iStream){};

  virtual TextCacheStruct_t* GetTeletextCache() { return NULL; };
  virtual void LoadPage(int p, int sp, unsigned char* buffer) {};

  virtual int  GetChapterCount()                               { return 0; }
  virtual int  GetChapter()                                    { return -1; }
  virtual void GetChapterName(CStdString& strChapterName)      { return; }
  virtual int  SeekChapter(int iChapter)                       { return -1; }
//  virtual bool GetChapterInfo(int chapter, SChapterInfo &info) { return false; }

  virtual float GetActualFPS() { return 0.0f; };
  virtual void SeekTime(__int64 iTime = 0){};
  virtual bool CanReportGetTime() { return true; }
  virtual __int64 GetTime(){ return 0;};
  virtual void ResetTime() {};
  virtual int GetTotalTime(){ return 0;};
  virtual int GetStartTime(){ return 0;};
  virtual int GetAudioBitrate(){ return 0;}
  virtual int GetVideoBitrate(){ return 0;}
  virtual int GetSourceBitrate(){ return 0;}
  virtual int GetChannels(){ return 0;};
  virtual int GetBitsPerSample(){ return 0;};
  virtual int GetSampleRate(){ return 0;};
  virtual CStdString GetAudioCodecName(){ return "";}
  virtual CStdString GetVideoCodecName(){ return "";}
  virtual int GetPictureWidth(){ return 0;}
  virtual int GetPictureHeight(){ return 0;}
  virtual bool GetStreamDetails(CStreamDetails &details){ return false;}
  virtual void ToFFRW(int iSpeed = 0){};
  // Skip to next track/item inside the current media (if supported).
  virtual bool SkipNext(){return false;}

  //Returns true if not playback (paused or stopped beeing filled)
  virtual bool IsCaching() const {return false;};
  //Cache filled in Percent
  virtual int GetCacheLevel() const {return -1;}
  virtual void SetCaching(bool enabled) { }

  virtual bool IsInMenu() const {return false;}
  virtual bool HasMenu() { return false; }

  virtual void DoAudioWork(){};
  virtual bool OnAction(const CAction &action) { return false; };

  virtual bool GetCurrentSubtitle(CStdString& strSubtitle) { strSubtitle = ""; return false; }
  //returns a state that is needed for resuming from a specific time
  virtual CStdString GetPlayerState() { return ""; };
  virtual bool SetPlayerState(CStdString state) { return false;};

  virtual bool PreRender() { return true; } // called from renderer (ui thread)
  virtual void PostRender(){} // called from renderer (ui thread)
  
  virtual void Ping() {} // this is called from the main thread and lets the player do actions in the context of this thread (gui)
  
  virtual bool CanPause() const {return true;}
  virtual bool CanSkip() const {return true;}
  virtual bool CanSetVolume() const {return true;}
  
  virtual bool OSDDisabled()      const { return false; }
  virtual bool FullScreenLocked() const { return false; }

  virtual CStdString GetPlayingTitle() { return ""; }

  virtual void OSDExtensionClicked(int nId) {}
  virtual bool RestartSubtitleStream(){return true;}
  virtual void ProcessExternalMessage(/*ThreadMessage */  void *pMsg) {};
  
  virtual iplayer_error GetError() { return IPLAYER_SUCCESS; }
  virtual CStdString GetErrorString() { return ""; }
  virtual bool ForceMouseRendering() { return false;}
  virtual bool MouseRenderingEnabled() {return false;}
  virtual bool KeyboardPassthrough() {return false;}
  virtual bool OnAppMessage(const CStdString& strHandler, const CStdString& strParam) {return false;}
  virtual bool IsPlayingStreamPlaylist() {return false;}
  virtual int  GetStreamPlaylistTimecode() {return -1;}

  virtual void GetVideoCacheLevel(unsigned int& cur, unsigned int& max) { cur = 0; max = 0; }
  virtual void GetAudioCacheLevel(unsigned int& cur, unsigned int& max) { cur = 0; max = 0; }
protected:
  IPlayerCallback& m_callback;
};
