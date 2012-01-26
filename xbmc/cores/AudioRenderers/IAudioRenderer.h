/*
* XBMC Media Center
* Copyright (c) 2002 d7o3g4q and RUNTiME
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// AsyncAudioRenderer.h: interface for the CAsyncDirectSound class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IAUDIORENDERER_H__B590A94D_D15E_43A6_A41D_527BD441B5F5__INCLUDED_)
#define AFX_IAUDIORENDERER_H__B590A94D_D15E_43A6_A41D_527BD441B5F5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IAudioCallback.h"
#include "PCMRemap.h"

extern void RegisterAudioCallback(IAudioCallback* pCallback);
extern void UnRegisterAudioCallback();

#ifndef DVD_NOPTS_VALUE
#define DVD_NOPTS_VALUE    (-1LL<<52) // should be possible to represent in both double and __int64
#endif

#define AUDIO_NOT_TIMED false
#define AUDIO_TIMED true

typedef enum
{
   AUDIO_MEDIA_FMT_PCM , /**< - stream is raw linear PCM data*/
   AUDIO_MEDIA_FMT_DVD_PCM , /**< - stream is linear PCM data coming from a DVD */
   AUDIO_MEDIA_FMT_BLURAY_PCM , /**< - stream is linear PCM data coming from a BD (HDMV Audio) */
   AUDIO_MEDIA_FMT_MPEG , /**< - stream uses MPEG-1 or MPEG-2 BC algorithm*/
   AUDIO_MEDIA_FMT_AAC , /**< - stream uses MPEG-2 or MPEG-4 AAC algorithm*/
   AUDIO_MEDIA_FMT_AAC_LOAS , /**< - stream uses MPEG-2 or MPEG-4 AAC algorithm with LOAS header format*/
   AUDIO_MEDIA_FMT_AAC_LATM, /** DVBT */
   AUDIO_MEDIA_FMT_DD , /**< - stream uses Dolby Digital (AC3) algorithm*/
   AUDIO_MEDIA_FMT_DD_PLUS , /**< - stream uses Dolby Digital Plus (E-AC3) algorithm*/
   AUDIO_MEDIA_FMT_TRUE_HD , /**< - stream uses Dolby TrueHD algorithm*/
   AUDIO_MEDIA_FMT_DTS_HD , /**< - stream uses DTS High Definition audio algorithm */
   AUDIO_MEDIA_FMT_DTS_HD_HRA , /**< - DTS-HD High Resolution Audio */
   AUDIO_MEDIA_FMT_DTS_HD_MA , /**< - DTS-HD Master Audio */
   AUDIO_MEDIA_FMT_DTS , /**< - stream uses DTS  algorithm*/
   AUDIO_MEDIA_FMT_DTS_LBR , /**< - stream uses DTS Low BitRate decode algorithm*/
   AUDIO_MEDIA_FMT_WM9 , /**< - stream uses Windows Media 9 algorithm*/
} AudioMediaFormat;

class IAudioRenderer
{
public:
  IAudioRenderer() {};
  virtual ~IAudioRenderer() {};
  virtual bool Initialize(IAudioCallback* pCallback, int iChannels, enum PCMChannels* channelMap, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, const char* strAudioCodec, bool bIsMusic, bool bPassthrough, bool bTimed = AUDIO_NOT_TIMED, AudioMediaFormat audioMediaFormat = AUDIO_MEDIA_FMT_PCM) = 0;
  virtual void UnRegisterAudioCallback() = 0;
  virtual void RegisterAudioCallback(IAudioCallback* pCallback) = 0;
  virtual float GetDelay() = 0;
  virtual float GetCacheTime() = 0;

  virtual unsigned int AddPackets(const void* data, unsigned int len, double pts = DVD_NOPTS_VALUE, double duration = 0) = 0;
  virtual bool IsResampling() { return false;};
  virtual unsigned int GetSpace() = 0;
  virtual bool Deinitialize() = 0;
  virtual bool Pause() = 0;
  virtual bool Stop() = 0;
  virtual bool Resume() = 0;
  virtual void Flush() {}
  virtual void Resync(double pts) {}
  virtual unsigned int GetChunkLen() = 0;

  virtual long GetCurrentVolume() const = 0;
  virtual void Mute(bool bMute) = 0;
  virtual bool SetCurrentVolume(long nVolume) = 0;
  virtual void SetDynamicRangeCompression(long drc) {};
  virtual int SetPlaySpeed(int iSpeed) = 0;
  virtual void WaitCompletion() = 0;
  virtual void SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers) = 0;
  virtual bool IsTimed() { return false; }
  virtual void DisablePtsCorrection(bool bDisable) { }

protected:
  CPCMRemap m_remap;
private:
};

#endif // !defined(AFX_IAUDIORENDERER_H__B590A94D_D15E_43A6_A41D_527BD441B5F5__INCLUDED_)
