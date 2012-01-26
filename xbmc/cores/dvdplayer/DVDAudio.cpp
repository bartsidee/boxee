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
 
#include "utils/SingleLock.h"
#include "utils/log.h"
#include "DVDAudio.h"
#include "DVDCodecs/Audio/DVDAudioCodec.h"  // for frame flags
#include "Util.h"
#include "DVDClock.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDPlayerAudio.h"
#include "Application.h"
#include "../AudioRenderers/AudioRendererFactory.h"

using namespace std;

CDVDAudio::CDVDAudio(volatile bool &bStop)
  : m_bStop(bStop)
{
  m_pAudioDecoder = NULL;
  m_pCallback = NULL;
  m_iBufferSize = 0;
  m_dwPacketSize = 0;
  m_pBuffer = NULL;
  m_iSpeed = 0;
  m_bPassthrough = false;
  m_Codec = CODEC_ID_NONE;
  m_iBitsPerSample = 0;
  m_iBitrate = 0;
  m_iChannels = 0;
  m_bDisablePtsCorrection = false;
}

CDVDAudio::~CDVDAudio()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioDecoder)
  {
    m_pAudioDecoder->Deinitialize();
    delete m_pAudioDecoder;
  }
  if (m_pBuffer) delete[] m_pBuffer;
}

void CDVDAudio::RegisterAudioCallback(IAudioCallback* pCallback)
{
  CSingleLock lock (m_critSection);
  m_pCallback = pCallback;
  if (m_pCallback && m_pAudioDecoder) m_pAudioDecoder->RegisterAudioCallback(pCallback);
}

void CDVDAudio::UnRegisterAudioCallback()
{
  CSingleLock lock (m_critSection);
  m_pCallback = NULL;
  if (m_pAudioDecoder) m_pAudioDecoder->UnRegisterAudioCallback();
}

bool CDVDAudio::Create(const DVDAudioFrame &audioframe, CodecID codec)
{
  bool passthrough = 0 != (audioframe.frame_flags & FFLAG_PASSTHROUGH);
  bool hwdecode = 0 != (audioframe.frame_flags & FFLAG_HWDECODE);

  CLog::Log(LOGNOTICE, "Creating audio device with codec id: %i, channels: %i, sample rate: %i, %s", codec, audioframe.channels, audioframe.sample_rate, passthrough ? "pass-through" : "no pass-through");

  // if passthrough isset do something else
  CSingleLock lock (m_critSection);

  const char* codecstring="";

  AudioMediaFormat mediaFormat = AUDIO_MEDIA_FMT_PCM;


  if(codec == CODEC_ID_VORBIS)
  {
    codecstring = "Vorbis";
  }
  else if (codec == CODEC_ID_AC3)
  {
    codecstring = "AC3";
    mediaFormat = AUDIO_MEDIA_FMT_DD;
  }
  else if (codec == CODEC_ID_DTS && 0 != (audioframe.frame_flags & FFLAG_HDFORMAT))
  {
    codecstring = "DTSHD";
    mediaFormat = AUDIO_MEDIA_FMT_DTS_HD_MA;
  }
  else if (codec == CODEC_ID_DTS)
  {
    codecstring = "DTS";
    mediaFormat = AUDIO_MEDIA_FMT_DTS;
  }
  else if (codec == CODEC_ID_EAC3)
  {
    codecstring = "EAC3";
    mediaFormat = AUDIO_MEDIA_FMT_DD_PLUS;
  }
  else if (codec == CODEC_ID_TRUEHD)
  {
    codecstring = "TRUEHD";
    mediaFormat = AUDIO_MEDIA_FMT_TRUE_HD;
  }
  else if(codec == CODEC_ID_MP3)
  {
    codecstring = "MP3";
    mediaFormat = AUDIO_MEDIA_FMT_MPEG;
  }
  else if(codec == CODEC_ID_AAC)
  {
    codecstring = "AAC";
    mediaFormat = AUDIO_MEDIA_FMT_AAC;
  }
  else
    codecstring = "PCM";

  // Rather than check in each switch, just reset this here.
  if( !passthrough && !hwdecode )
    mediaFormat = AUDIO_MEDIA_FMT_PCM;

  bool timed = AUDIO_NOT_TIMED;
#ifdef HAS_INTEL_SMD
  timed = AUDIO_TIMED;
#endif

  // if we're using dvdplayer to play only PCM audio then we open non-timed device
  if(!g_application.m_pPlayer->HasVideo() && audioframe.channels <= 2 && mediaFormat == AUDIO_MEDIA_FMT_PCM)
    timed = AUDIO_NOT_TIMED;

  if(!g_application.m_pPlayer->HasVideo() && mediaFormat == AUDIO_MEDIA_FMT_DTS)
    mediaFormat = AUDIO_MEDIA_FMT_PCM;

  m_pAudioDecoder = CAudioRendererFactory::Create(m_pCallback, audioframe.channels, audioframe.channel_map, audioframe.sample_rate,
    audioframe.bits_per_sample, false, codecstring, false, passthrough, timed, mediaFormat);

  if (!m_pAudioDecoder) return false;

  m_iChannels = audioframe.channels;
  m_iBitrate = audioframe.sample_rate;
  m_iBitsPerSample = audioframe.bits_per_sample;
  m_bPassthrough = passthrough;
  m_Codec = codec;

  m_dwPacketSize = m_pAudioDecoder->GetChunkLen();
  if (m_pBuffer)
  {
    delete[] m_pBuffer;
    m_pBuffer = NULL;
  }
  if( m_dwPacketSize )
    m_pBuffer = new BYTE[m_dwPacketSize];

  m_pAudioDecoder->DisablePtsCorrection(m_bDisablePtsCorrection);
  
  return true;
}

void CDVDAudio::Destroy()
{
  CSingleLock lock (m_critSection);

  if (m_pAudioDecoder)
  {
    m_pAudioDecoder->Stop();
    m_pAudioDecoder->Deinitialize();
    delete m_pAudioDecoder;
  }

  if (m_pBuffer) delete[] m_pBuffer;
  m_pBuffer = NULL;
  m_dwPacketSize = 0;
  m_pAudioDecoder = NULL;
  m_iBufferSize = 0;
  m_iChannels = 0;
  m_iBitrate = 0;
  m_iBitsPerSample = 0;
  m_bPassthrough = false;
  m_Codec = CODEC_ID_NONE;
  m_iSpeed = 1;
}

void CDVDAudio::SetSpeed(int iSpeed)
{
  m_iSpeed = abs(iSpeed);

}

DWORD CDVDAudio::AddPacketsRenderer(unsigned char* data, DWORD len, double pts, double duration, CSingleLock &lock)
{ 
  if(!m_pAudioDecoder)
    return 0;

  DWORD bps = m_iChannels * m_iBitrate * (m_iBitsPerSample>>3);

  if(!bps && !m_pAudioDecoder->IsTimed())
    return 0;

  //Calculate a timeout when this definitely should be done
  double timeout;
  timeout  = DVD_SEC_TO_TIME(m_pAudioDecoder->GetDelay() + (double)len / bps);
  timeout += DVD_SEC_TO_TIME(1.0);
  timeout += CDVDClock::GetAbsoluteClock();

  DWORD total = len;
  DWORD copied;
  double remaining_dur = duration;
  do
  {
    copied = m_pAudioDecoder->AddPackets(data, len, pts, remaining_dur);
    data += copied;
    len -= copied;

    if (!len || (m_dwPacketSize && len < m_dwPacketSize))
      break;

    if (copied == 0 && timeout < CDVDClock::GetAbsoluteClock())
    {
      CLog::Log(LOGERROR, "CDVDAudio::AddPacketsRenderer - timeout adding data to renderer");
      break;
    }

    // adjust pts to cover fragmented packet
    if( duration && 0 != len )
    {
      double elapsed = (double)copied / (double)total;
      elapsed *= duration;
      pts += elapsed;
      remaining_dur -= elapsed;
      if( remaining_dur < 0.0f )
      {
        remaining_dur = 0.0f;
      }
    }

    lock.Leave();
    Sleep(1);
    lock.Enter();
  } while (!m_bStop);

  return total - len;
}

DWORD CDVDAudio::AddPackets(const DVDAudioFrame &audioframe)
{
  CSingleLock lock (m_critSection);

  unsigned char* data = audioframe.data;
  DWORD len = audioframe.size;

  DWORD total = len;
  DWORD copied;

//  printf("Packet size %d pts %.2f\n", audioframe.size, audioframe.pts / 1000000);
  if (m_iBufferSize > 0) // See if there are carryover bytes from the last call. need to add them 1st.
  {
    copied = std::min(m_dwPacketSize - m_iBufferSize, len); // Smaller of either the data provided or the leftover data

    memcpy(m_pBuffer + m_iBufferSize, data, copied); // Tack the caller's data onto the end of the buffer
    data += copied; // Move forward in caller's data
    len -= copied; // Decrease amount of data available from caller 
    m_iBufferSize += copied; // Increase amount of data available in buffer

    if(m_lastPts == DVD_NOPTS_VALUE)
      m_lastPts = audioframe.pts;

    if(m_iBufferSize < m_dwPacketSize) // If we don't have enough data to give to the renderer, wait until next time
      return copied;

    if(AddPacketsRenderer(m_pBuffer, m_iBufferSize, m_lastPts, 0, lock) != m_iBufferSize)
    {
      m_iBufferSize = 0;
      m_lastPts = DVD_NOPTS_VALUE;
      CLog::Log(LOGERROR, "%s - failed to add leftover bytes to render", __FUNCTION__);
      return copied;
    }

    m_iBufferSize = 0;
    m_lastPts = DVD_NOPTS_VALUE;
    if (!len)
      return copied; // We used up all the caller's data
  }

  copied = AddPacketsRenderer(data, len, audioframe.pts, audioframe.duration, lock);
  data += copied;
  len -= copied;

  // if we have more data left, save it for the next call to this function
  if (len > 0 && !m_bStop && m_dwPacketSize)
  {
    if((len+m_iBufferSize) > m_dwPacketSize)
      CLog::Log(LOGERROR, "%s - More bytes left than can be stored in buffer", __FUNCTION__);

    unsigned int bytes_to_copy = std::min(len, m_dwPacketSize - m_iBufferSize);

    memcpy(m_pBuffer + m_iBufferSize, data, bytes_to_copy);
    len  -= bytes_to_copy;
    m_iBufferSize += bytes_to_copy;

    // if we copied the entire frame (i.e. we are buffering a whole packet)
    // then save the pts for this frame, so we are honest with the renderer later
    if( audioframe.size == bytes_to_copy )
      m_lastPts = audioframe.pts;
  }
  return total - len;
}

void CDVDAudio::Finish()
{
  CSingleLock lock (m_critSection);
  if (!m_pAudioDecoder) 
    return;

  DWORD silence = m_dwPacketSize ? (m_dwPacketSize - m_iBufferSize % m_dwPacketSize) : 0;

  if(silence > 0 && m_iBufferSize > 0)
  {
    CLog::Log(LOGDEBUG, "CDVDAudio::Drain - adding %d bytes of silence, buffer size: %d, chunk size: %d", silence, m_iBufferSize, m_dwPacketSize);
    memset(m_pBuffer+m_iBufferSize, 0, silence);
    m_iBufferSize += silence;
}

  if(!m_pAudioDecoder->IsTimed() && m_iBufferSize)
    if(AddPacketsRenderer(m_pBuffer, m_iBufferSize, 0, 0, lock) != m_iBufferSize)
      CLog::Log(LOGERROR, "CDVDAudio::Drain - failed to play the final %d bytes", m_iBufferSize);

  m_iBufferSize = 0;
}

void CDVDAudio::Drain()
{
  Finish();
  CSingleLock lock (m_critSection);
  if (m_pAudioDecoder) 
    m_pAudioDecoder->WaitCompletion();
}

void CDVDAudio::SetVolume(int iVolume)
{
#ifndef HAS_INTEL_SMD
  CSingleLock lock (m_critSection);
#endif
  if (m_pAudioDecoder) m_pAudioDecoder->SetCurrentVolume(iVolume);
}

void CDVDAudio::SetDynamicRangeCompression(long drc)
{
  CSingleLock lock (m_critSection);
  if (m_pAudioDecoder) m_pAudioDecoder->SetDynamicRangeCompression(drc);
}

void CDVDAudio::Pause()
{
#ifndef HAS_INTEL_SMD
  CSingleLock lock (m_critSection);
#endif
  if (m_pAudioDecoder) m_pAudioDecoder->Pause();
}

void CDVDAudio::Resume()
{
#ifndef HAS_INTEL_SMD
  CSingleLock lock (m_critSection);
#endif
  if (m_pAudioDecoder) m_pAudioDecoder->Resume();
}

double CDVDAudio::GetDelay()
{
  CSingleLock lock (m_critSection);

  double delay = 0.0;
  if(m_pAudioDecoder)
    delay = m_pAudioDecoder->GetDelay();

  DWORD bps = m_iChannels * m_iBitrate * (m_iBitsPerSample>>3);
  if(m_iBufferSize && bps)
    delay += (double)m_iBufferSize / bps;

  return delay * DVD_TIME_BASE;
}

void CDVDAudio::Flush()
{

  CSingleLock lock (m_critSection);

  if (m_pAudioDecoder)
  {
#ifdef HAS_INTEL_SMD
    m_pAudioDecoder->Flush();
#else
    m_pAudioDecoder->Stop();
    m_pAudioDecoder->Resume();
#endif
  }
  m_iBufferSize = 0;
  m_lastPts = DVD_NOPTS_VALUE;
}

void CDVDAudio::Resync(double pts)
{
  if(m_pAudioDecoder)
  {
    m_pAudioDecoder->Resync(pts);
  } 
}

bool CDVDAudio::IsValidFormat(const DVDAudioFrame &audioframe, CodecID codec)
{
  if(!m_pAudioDecoder)
    return false;

  if((0 != (audioframe.frame_flags & FFLAG_PASSTHROUGH)) != m_bPassthrough)
  {
    CLog::Log(LOGERROR, "CDVDAudio::IsValidFormat - passthrough flag does not match");
    return false;
  }

#ifndef HAS_INTEL_SMD
  if(audioframe.channels != m_iChannels 
  || audioframe.sample_rate != m_iBitrate 
  || audioframe.bits_per_sample != m_iBitsPerSample)
  {
    CLog::Log(LOGERROR, "CDVDAudio::IsValidFormat - audio frame properties does not match: channels %d %d sample rate %d %d bits per sample %d %d",
        audioframe.channels, m_iChannels, audioframe.sample_rate, m_iBitrate, audioframe.bits_per_sample, m_iBitsPerSample);
    return false;
  }
#endif
  if(codec != m_Codec)
  {
    CLog::Log(LOGERROR, "CDVDAudio::IsValidFormat - codecs not matching %d %d", codec, m_Codec);
    return false;
  }
  return true;
}
