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
#include "DVDPlayerAudio.h"
#include "DVDCodecs/Audio/DVDAudioCodec.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDPerformanceCounter.h"
#include "GUISettings.h"
#include "VideoReferenceClock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

#include <sstream>
#include <iomanip>
#include <cmath>

#ifdef HAS_INTEL_SMD
#include "IntelSMDGlobals.h"
#endif

using namespace std;

CPTSOutputQueue::CPTSOutputQueue()
{
  Flush();
}

void CPTSOutputQueue::Add(double pts, double delay, double duration)
{
  CSingleLock lock(m_sync);

  TPTSItem item;
  item.pts = pts;
  item.timestamp = CDVDClock::GetAbsoluteClock() + delay;
  item.duration = duration;

  // first one is applied directly
  if(m_queue.empty() && m_current.pts == DVD_NOPTS_VALUE)
    m_current = item;
  else
    m_queue.push(item);

  // call function to make sure the queue 
  // doesn't grow should nobody call it
  Current();
}
void CPTSOutputQueue::Flush()
{
  CSingleLock lock(m_sync);

  while( !m_queue.empty() ) m_queue.pop();
  m_current.pts = DVD_NOPTS_VALUE;
  m_current.timestamp = 0.0;
  m_current.duration = 0.0;
}

double CPTSOutputQueue::Current()
{
  CSingleLock lock(m_sync);

  if(!m_queue.empty() && m_current.pts == DVD_NOPTS_VALUE)
  {
    m_current = m_queue.front();
    m_queue.pop();
  }

  while( !m_queue.empty() && CDVDClock::GetAbsoluteClock() >= m_queue.front().timestamp )
  {
    m_current = m_queue.front();
    m_queue.pop();
  }

  if( m_current.timestamp == 0 ) return m_current.pts;

  return m_current.pts + min(m_current.duration, (CDVDClock::GetAbsoluteClock() - m_current.timestamp));
}  

void CPTSInputQueue::Add(__int64 bytes, double pts)
{
  CSingleLock lock(m_sync);

  m_list.insert(m_list.begin(), make_pair(bytes, pts));
}

void CPTSInputQueue::Flush()
{
  CSingleLock lock(m_sync);

  m_list.clear();
}
double CPTSInputQueue::Get(__int64 bytes, bool consume)
{
  CSingleLock lock(m_sync);

  IT it = m_list.begin();
  for(; it != m_list.end(); it++)
  {
    if(bytes <= it->first)
    {
      double pts = it->second;
      if(consume)
      {
        it->second = DVD_NOPTS_VALUE;
        m_list.erase(++it, m_list.end());        
      }
      return pts; 
    }
    bytes -= it->first;
  }
  return DVD_NOPTS_VALUE;
}

class CDVDMsgAudioCodecChange : public CDVDMsg
{
public:
  CDVDMsgAudioCodecChange(const CDVDStreamInfo &hints, CDVDAudioCodec* codec)
    : CDVDMsg(GENERAL_STREAMCHANGE)
    , m_codec(codec)
    , m_hints(hints)
  {}
  ~CDVDMsgAudioCodecChange()
  {
    delete m_codec;
  }
  CDVDAudioCodec* m_codec;
  CDVDStreamInfo  m_hints;
};

extern bool g_bFirstFrameDecoded;
bool g_bRunning;

CDVDPlayerAudio::CDVDPlayerAudio(CDVDClock* pClock) 
: CThread()
, m_messageQueue("audio")
, m_dvdAudio((bool&)m_bStop)
{
  m_pClock = pClock;
  m_pAudioCodec = NULL;  
  m_audioClock = 0;
  m_droptime = 0;
  m_speed = DVD_PLAYSPEED_NORMAL;
  m_stalled = false;
  m_started = false;
  g_bRunning = false;
  m_bTimed = false;
  m_bDisablePtsCorrection = false;

#ifdef HAS_INTEL_SMD
  m_bTimed = true;
#endif

  m_freq = CurrentHostFrequency();

  InitializeCriticalSection(&m_critCodecSection);
  m_messageQueue.SetMaxDataSize(4 * 1024 * 1024);
  m_messageQueue.SetMaxTimeSize(16.0);
  g_dvdPerformanceCounter.EnableAudioQueue(&m_messageQueue);
  memset( (void*)&m_ContinuityData, 0, sizeof(m_ContinuityData));
}

CDVDPlayerAudio::~CDVDPlayerAudio()
{
  StopThread();
  g_dvdPerformanceCounter.DisableAudioQueue();

  // close the stream, and don't wait for the audio to be finished
  // CloseStream(true);
  DeleteCriticalSection(&m_critCodecSection);
}

bool CDVDPlayerAudio::OpenStream( CDVDStreamInfo &hints )                                 
{
  CLog::Log(LOGNOTICE, "Finding audio codec for: %i", hints.codec);
  CDVDAudioCodec* codec = CDVDFactoryCodec::CreateAudioCodec(hints);
  if( !codec )
  {
    CLog::Log(LOGERROR, "CDVDPlayerAudio::OpenStream Unsupported audio codec for decoding or bitstreaming %d", hints.codec);
    return false;
  }

  if( m_messageQueue.IsInited() )
  {
    m_messageQueue.Put(new CDVDMsgAudioCodecChange(hints, codec), 0);
  }
  else
  {
    OpenStream(hints, codec);
    m_messageQueue.Init();
    CLog::Log(LOGNOTICE, "Creating audio thread");
    Create();
  }
  return true;
}

void CDVDPlayerAudio::OpenStream( CDVDStreamInfo &hints, CDVDAudioCodec* codec)
{
  EnterCriticalSection(&m_critCodecSection);

  SAFE_DELETE(m_pAudioCodec);
  m_pAudioCodec = codec;

  LeaveCriticalSection(&m_critCodecSection);
  
  /* store our stream hints */
  m_streaminfo = hints;
  
  /* update codec information from what codec gave us */
  m_streaminfo.channels = m_pAudioCodec->GetChannels();
  m_streaminfo.samplerate = m_pAudioCodec->GetSampleRate();

  m_droptime = 0;
  m_audioClock = 0;
  m_stalled = m_messageQueue.GetPacketCount(CDVDMsg::DEMUXER_PACKET) == 0;
  m_started = false;

  m_synctype = SYNC_DISCON;
  m_setsynctype = g_guiSettings.GetInt("videoplayer.synctype");
  m_prevsynctype = -1;
  m_resampler.SetQuality(g_guiSettings.GetInt("videoplayer.resamplequality"));

  m_error = 0;
  m_errorbuff = 0;
  m_errorcount = 0;
  m_integral = 0;
  m_skipdupcount = 0;
  m_prevskipped = false;
  m_syncclock = true;
  m_errortime = CurrentHostCounter();

  m_maxspeedadjust = g_guiSettings.GetFloat("videoplayer.maxspeedadjust");
}

void CDVDPlayerAudio::CloseStream(bool bWaitForBuffers)
{
  // FIXME: when we do this, we lose track of the external play state provided in any pending messages
  // we can't just cache the current play speed as there may be 3-4 queued ones in the message queue
  SetSpeed(DVD_PLAYSPEED_NORMAL);

  // wait until buffers are empty
  if (bWaitForBuffers && m_speed > 0)
  {
    m_messageQueue.WaitUntilEmpty();
  }

  if(bWaitForBuffers && m_bTimed)
  {
    Sleep(1000);
  }


  // send abort message to the audio queue
  m_messageQueue.Abort();

  CLog::Log(LOGNOTICE, "Waiting for audio thread to exit");

  // shut down the audio_decode thread and wait for it
  StopThread(); // will set this->m_bStop to true

  // destroy audio device
  CLog::Log(LOGNOTICE, "Closing audio device");
  if (bWaitForBuffers && m_speed > 0)
  {
    m_bStop = false;
    m_dvdAudio.Drain();
    m_bStop = true;
  }
  m_dvdAudio.Destroy();

  // uninit queue
  m_messageQueue.End();

  CLog::Log(LOGNOTICE, "Deleting audio codec");
  if (m_pAudioCodec)
  {
    m_pAudioCodec->Dispose();
    delete m_pAudioCodec;
    m_pAudioCodec = NULL;
  }

  // flush any remaining pts values
  m_ptsOutput.Flush();
  m_resampler.Flush();
  memset( (void*)&m_ContinuityData, 0, sizeof(m_ContinuityData));
}
#if 0

bool CDVDPlayerAudio::OpenDecoder(CDVDStreamInfo &hints, BYTE* buffer /* = NULL*/, unsigned int size /* = 0*/)
{
  EnterCriticalSection(&m_critCodecSection);

  /* close current audio codec */
  if( m_pAudioCodec )
  {
    CLog::Log(LOGNOTICE, "Deleting audio codec");
    m_pAudioCodec->Dispose();
    SAFE_DELETE(m_pAudioCodec);
  }

  /* store our stream hints */
  m_streaminfo = hints;

  CLog::Log(LOGNOTICE, "Finding audio codec for: %i", m_streaminfo.codec);
  m_pAudioCodec = CDVDFactoryCodec::CreateAudioCodec( m_streaminfo );
  if( !m_pAudioCodec )
  {
    if( !hints.bitstream )
      CLog::Log(LOGERROR, "Unsupported audio codec");

    m_streaminfo.Clear();
    LeaveCriticalSection(&m_critCodecSection);
    return false;
  }

  /* update codec information from what codec gave ut */  
  m_streaminfo.channels = m_pAudioCodec->GetChannels();
  m_streaminfo.samplerate = m_pAudioCodec->GetSampleRate();

  LeaveCriticalSection(&m_critCodecSection);

  return true;
}
#endif

void CDVDPlayerAudio::FixContinuity(DVDAudioFrame& audioFrame)
{
  // try to patch the pts on incoming frames based on past data. some dts-hd clips we have
  // seen were encoded with out of order timestamps, but once rewritten play just fine
  // the below code will build up an average of 5 frames to determine what the expected pts gap is
  // when it hits 5, it will apply a check on new packets to ensure they are within +/- 10% of the
  // expected range. if not, it will rewrite the pts
  // if it rewrites too many pts values it will start over
  // any place where this logic needs to be reset can be done by memset(m_ContinuityData, 0, ...)

  if( m_ContinuityData.lastpts )
  {
    double gap = audioFrame.pts - m_ContinuityData.lastpts;
    if( !gap || abs(gap) > 200000.0f)
    {
      // nothing we can do with this - the frame pts data is not (packets are next to each other, or have significant jumps)
      memset( (void*)&m_ContinuityData, 0, sizeof(m_ContinuityData));
    }
    else if( m_ContinuityData.weight < 5 )
    {
      double full = m_ContinuityData.avgptsgap * m_ContinuityData.weight;
      full += gap;
      m_ContinuityData.weight++;
      m_ContinuityData.avgptsgap = (full / m_ContinuityData.weight);
    }
    else
    {
      // Allow a 10% margin on either side for the pts..
      if( gap > (m_ContinuityData.avgptsgap * 1.1) ||
          gap < (m_ContinuityData.avgptsgap * .9) )
      {
        // rewrite pts
        audioFrame.pts = m_ContinuityData.lastpts + m_ContinuityData.avgptsgap;
#ifdef HAS_INTEL_SMD
// dump this to console, we need to know if a clip is getting altered...
printf("rewriting audio packet pts from %.2f to %.2f gap %.2f avg %.2f\n", m_audioClock, audioFrame.pts, gap, m_ContinuityData.avgptsgap);
#endif
        m_audioClock = audioFrame.pts;
        CLog::Log(LOGDEBUG, "Rewrote packet pts due to excessive timestamp gap %.2f (vs expected %.2f)\n",
            gap, m_ContinuityData.avgptsgap);
        m_ContinuityData.rewrites++;
        if( m_ContinuityData.rewrites == 2 )
        {
          // Something is wrong, start over
          memset((void*)&m_ContinuityData, 0, sizeof(m_ContinuityData));
        }
      }
      else
      {
        m_ContinuityData.rewrites = 0;
      }
    }
  }
  m_ContinuityData.lastpts = audioFrame.pts;
}

// decode one audio frame and returns its uncompressed size
int CDVDPlayerAudio::DecodeFrame(DVDAudioFrame &audioframe, bool bDropPacket)
{
  int result = 0;
  int datatimeout = 1000;

  // make sure the sent frame is clean
  memset(&audioframe, 0, sizeof(DVDAudioFrame));

  while (!m_bStop)
  {
    /* NOTE: the audio packet can contain several frames */
    while( !m_bStop && m_decode.size > 0 )
    {
      if( !m_pAudioCodec ) 
        return DECODE_FLAG_ERROR;

      /* the packet dts refers to the first audioframe that starts in the packet */      
      double dts = m_ptsInput.Get(m_decode.size + m_pAudioCodec->GetBufferSize(), true);
      if (dts != DVD_NOPTS_VALUE)
      {
        m_audioClock = dts;
      }
      int len = m_pAudioCodec->Decode(m_decode.data, m_decode.size);
      m_audioStats.AddSampleBytes(m_decode.size);
      if (len < 0)
      {
        /* if error, we skip the packet */
        CLog::Log(LOGERROR, "CDVDPlayerAudio::DecodeFrame - Decode Error. Skipping audio packet");
        m_decode.Release();
        m_pAudioCodec->Reset();
        return DECODE_FLAG_ERROR;
      }

      // fix for fucked up decoders
      if( len > m_decode.size )
      {        
        CLog::Log(LOGERROR, "CDVDPlayerAudio:DecodeFrame - Codec tried to consume more data than available. Potential memory corruption");        
        m_decode.Release();
        m_pAudioCodec->Reset();
        assert(0);
      }

      m_decode.data += len;
      m_decode.size -= len;

      // get decoded data and the size of it
      audioframe.size = m_pAudioCodec->GetData(&audioframe.data);
      audioframe.pts = m_audioClock;
      audioframe.channels = m_pAudioCodec->GetChannels();
      audioframe.channel_map = m_pAudioCodec->GetChannelMap(); // can be expensive
      audioframe.bits_per_sample = m_pAudioCodec->GetBitsPerSample();
      audioframe.sample_rate = m_pAudioCodec->GetSampleRate();
      audioframe.frame_flags = m_pAudioCodec->GetFlags();

      if (audioframe.size <= 0) 
        continue;

      //printf("audio frame size %d pts %.2f\n", audioframe.size, audioframe.pts != DVD_NOPTS_VALUE ? audioframe.pts : 0.0f);

      if( 0 != (audioframe.frame_flags & FFLAG_PASSTHROUGH) &&
          0 != (audioframe.frame_flags & FFLAG_HDFORMAT) &&
          CODEC_ID_DTS == m_streaminfo.codec)
      {
#ifndef HAS_INTEL_SMD
        FixContinuity(audioframe);
#endif
      }

      // at this point this is a real audio frame

      // compute duration. if we are doing encoded passthrough, try to use the source packet from the demux
      // TODO; if we are hitting this codepath, move the audio clock forward only after the full packet is processed
      // this is to better handle multiple frames per packet.
      if( audioframe.frame_flags & FFLAG_PASSTHROUGH
          || audioframe.frame_flags & FFLAG_HWDECODE )
      {
        DemuxPacket* pkt = m_decode.msg->GetPacket();
        if( pkt && pkt->duration )
        {
          audioframe.duration = pkt->duration;
        }
        else
        {
          audioframe.duration = 0.0f;
        }

        m_audioClock += audioframe.duration;
      }
      else
      {
        int n = (audioframe.channels * audioframe.bits_per_sample * audioframe.sample_rate)>>3;
        if (n > 0)
        {
          // safety check, if channels == 0, n will result in 0, and that will result in a nice devide exception
          audioframe.duration = ((double)audioframe.size * DVD_TIME_BASE) / n;

          // increase audioclock to after the packet
          m_audioClock += audioframe.duration;
          datatimeout = (unsigned int)(DVD_TIME_TO_MSEC(audioframe.duration * 2.0));
        }
      }

      // if demux source want's us to not display this, continue
      if(m_decode.msg->GetPacketDrop())
        continue;

      //If we are asked to drop this packet, return a size of zero. then it won't be played
      //we currently still decode the audio.. this is needed since we still need to know it's 
      //duration to make sure clock is updated correctly.
      if( bDropPacket )
        result |= DECODE_FLAG_DROP;

      return result;
    }
    // free the current packet
    m_decode.Release();

    if (m_messageQueue.ReceivedAbortRequest()) return DECODE_FLAG_ABORT;

    CDVDMsg* pMsg;
    int iPriority = (m_speed == DVD_PLAYSPEED_PAUSE) ? 1 : 0;
    // read next packet and return -1 on error
    LeaveCriticalSection(&m_critCodecSection); //Leave here as this might stall a while
    MsgQueueReturnCode ret = m_messageQueue.Get(&pMsg, 1 /*datatimeout*/, iPriority);
    EnterCriticalSection(&m_critCodecSection);

    if (ret == MSGQ_TIMEOUT) 
      return DECODE_FLAG_TIMEOUT;

    if (MSGQ_IS_ERROR(ret) || ret == MSGQ_ABORT) 
      return DECODE_FLAG_ABORT;

    if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      m_decode.Attach((CDVDMsgDemuxerPacket*)pMsg);
      m_ptsInput.Add( m_decode.size, m_decode.dts );
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_SYNCHRONIZE))
    {
      ((CDVDMsgGeneralSynchronize*)pMsg)->Wait( &m_bStop, SYNCSOURCE_AUDIO );
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_SYNCHRONIZE");
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_STREAMCHANGE))
    {
      CDVDMsgAudioCodecChange* msg(static_cast<CDVDMsgAudioCodecChange*>(pMsg));
#ifdef HAS_INTEL_SMD
      m_dvdAudio.Drain();
#endif
      OpenStream(msg->m_hints, msg->m_codec);
      msg->m_codec = NULL;      
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESYNC))
    { //player asked us to set internal clock
      CDVDMsgGeneralResync* pMsgGeneralResync = (CDVDMsgGeneralResync*)pMsg;
      //printf("DVDPlayerAudio GENERAL_RESYNC %.2f\n",  pMsgGeneralResync->m_timestamp / 1000000);

      if (pMsgGeneralResync->m_timestamp != DVD_NOPTS_VALUE)
      {
        m_dvdAudio.Resync(pMsgGeneralResync->m_timestamp);
        m_audioClock = pMsgGeneralResync->m_timestamp;
      }

      m_ptsOutput.Add(m_audioClock, m_dvdAudio.GetDelay(), 0);
      if (pMsgGeneralResync->m_clock)
      {
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_RESYNC(%f, 1)", m_audioClock);
        m_pClock->Discontinuity(m_ptsOutput.Current());
      }
      else
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_RESYNC(%f, 0)", m_audioClock);
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH))
    {
      m_dvdAudio.Flush();
      m_ptsOutput.Flush();
      m_ptsInput.Flush();
      m_resampler.Flush();
      m_syncclock = true;

      m_decode.Release();
      memset( (void*)&m_ContinuityData, 0, sizeof(m_ContinuityData));
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_EOF))
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_EOF");
      m_dvdAudio.Finish();
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_DELAY))
    {
      if (m_speed != DVD_PLAYSPEED_PAUSE)
      {
        double timeout = static_cast<CDVDMsgDouble*>(pMsg)->m_value;

        CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_DELAY(%f)", timeout);

        timeout *= (double)DVD_PLAYSPEED_NORMAL / abs(m_speed);
        timeout += CDVDClock::GetAbsoluteClock();

        while(!m_bStop && CDVDClock::GetAbsoluteClock() < timeout)
          Sleep(1);
      }
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
    {
      m_speed = static_cast<CDVDMsgInt*>(pMsg)->m_value;
      if (m_speed == DVD_PLAYSPEED_PAUSE)
      {
        m_ptsOutput.Flush();
        m_resampler.Flush();
        m_syncclock = true;
        m_dvdAudio.Pause();
      }
      else
        m_dvdAudio.Resume();
    }
    pMsg->Release();
  }
  return 0;
}

void CDVDPlayerAudio::OnStartup()
{
  CThread::SetName("CDVDPlayerAudio");

  m_decode.msg = NULL;
  m_decode.Release();

  g_dvdPerformanceCounter.EnableAudioDecodePerformance(ThreadHandle());
}

void CDVDPlayerAudio::Process()
{
  CLog::Log(LOGNOTICE, "running thread: CDVDPlayerAudio::Process()");

  int result;
  bool packetadded;

  DVDAudioFrame audioframe;
  m_audioStats.Start();

  while (!m_bStop)
  {
    //Don't let anybody mess with our global variables
    EnterCriticalSection(&m_critCodecSection);
    result = DecodeFrame(audioframe, m_speed != DVD_PLAYSPEED_NORMAL); // blocks if no audio is available, but leaves critical section before doing so
    LeaveCriticalSection(&m_critCodecSection);
    if( result & DECODE_FLAG_ERROR ) 
    { 
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio::Process - Decode Error");
      continue;
    }

    if( result & DECODE_FLAG_TIMEOUT ) 
    {
      if(m_started)
        m_stalled = true;
      continue;
    }

    if( result & DECODE_FLAG_ABORT )
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio::Process - Abort received, exiting thread");
      break;
    }

#ifdef PROFILE /* during profiling we just drop all packets, after having decoded */
    m_pClock->Discontinuity(audioframe.pts);
    continue;
#endif

    if( audioframe.size == 0 )
      continue;

    m_stalled = false;
    m_started = true;

    packetadded = true;

    m_dvdAudio.DisablePtsCorrection(m_bDisablePtsCorrection);

    // we have succesfully decoded an audio frame, setup renderer to match
    if (!m_dvdAudio.IsValidFormat(audioframe, m_streaminfo.codec))
    {
      m_dvdAudio.Destroy();
      if(!m_dvdAudio.Create(audioframe, m_streaminfo.codec))
        CLog::Log(LOGERROR, "%s - failed to create audio renderer", __FUNCTION__);
    }

    if( result & DECODE_FLAG_DROP )
    {
      //frame should be dropped. Don't let audio move ahead of the current time thou
      //we need to be able to start playing at any time
      //when playing backwords, we try to keep as small buffers as possible

      if(m_droptime == 0.0)
        m_droptime = m_pClock->GetAbsoluteClock();
      if(m_speed > 0)
        m_droptime += audioframe.duration * DVD_PLAYSPEED_NORMAL / m_speed;
      while( !m_bStop && m_droptime > m_pClock->GetAbsoluteClock() ) Sleep(1);
    } 
    else
    {
      m_droptime = 0.0;

      SetSyncType((audioframe.frame_flags & FFLAG_PASSTHROUGH) != 0);

      // add any packets play
      packetadded = OutputPacket(audioframe);
    }

    // store the delay for this pts value so we can calculate the current playing time
    if(m_speed != DVD_PLAYSPEED_PAUSE && packetadded)
      m_ptsOutput.Add(audioframe.pts, m_dvdAudio.GetDelay() - audioframe.duration, audioframe.duration);

    if( m_ptsOutput.Current() == DVD_NOPTS_VALUE )
      continue;

    if( m_speed != DVD_PLAYSPEED_NORMAL )
      continue;

    if (packetadded)
      HandleSyncError(audioframe.duration);
  }

  CLog::Log(LOGNOTICE, "exit thread: CDVDPlayerAudio::Process()");
}

void CDVDPlayerAudio::SetSyncType(bool passthrough)
{
  //set the synctype from the gui
  //use skip/duplicate when resample is selected and passthrough is on
  m_synctype = m_setsynctype;
  if (passthrough && m_synctype == SYNC_RESAMPLE)
    m_synctype = SYNC_SKIPDUP;

  //tell dvdplayervideo how much it can change the speed
  //if SetMaxSpeedAdjust returns false, it means no video is played and we need to use clock feedback
  double maxspeedadjust = 0.0;
  if (m_synctype == SYNC_RESAMPLE)
    maxspeedadjust = m_maxspeedadjust;

  if (!m_pClock->SetMaxSpeedAdjust(maxspeedadjust))
    m_synctype = SYNC_DISCON;

  if (m_synctype != m_prevsynctype)
  {
    const char *synctypes[] = {"clock feedback", "skip/duplicate", "resample", "invalid"};
    int synctype = (m_synctype >= 0 && m_synctype <= 2) ? m_synctype : 3;
    CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: synctype set to %i: %s", m_synctype, synctypes[synctype]);
    m_prevsynctype = m_synctype;
  }
}

double g_fCurrentAudioClock = 0.0;

void CDVDPlayerAudio::HandleSyncError(double duration)
{

  double clock = m_pClock->GetClock();
  double error = m_ptsOutput.Current() - clock;
  int64_t now;

  g_fCurrentAudioClock = clock;
  if( fabs(error) > DVD_MSEC_TO_TIME(100) || m_syncclock )
  {
    m_pClock->Discontinuity(clock+error);

    // When using SMD this error is not relevant
#ifndef HAS_INTEL_SMD
    if(m_speed == DVD_PLAYSPEED_NORMAL)
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Discontinuity - was:%f, should be:%f, error:%f", clock, clock+error, error);
#endif

    m_errorbuff = 0;
    m_errorcount = 0;
    m_skipdupcount = 0;
    m_error = 0;
    m_syncclock = false;
    m_errortime = CurrentHostCounter();

    return;
  }

  if (m_speed != DVD_PLAYSPEED_NORMAL)
  {
    m_errorbuff = 0;
    m_errorcount = 0;
    m_integral = 0;
    m_skipdupcount = 0;
    m_error = 0;
    m_resampler.Flush();
    m_errortime = CurrentHostCounter();
    return;
  }

  if(m_bTimed)
    return;

  m_errorbuff += error;
  m_errorcount++;

  //check if measured error for 1 second
  now = CurrentHostCounter();
  if ((now - m_errortime) >= m_freq)
  {
    m_errortime = now;
    m_error = m_errorbuff / m_errorcount;

    m_errorbuff = 0;
    m_errorcount = 0;

    if (m_synctype == SYNC_DISCON && fabs(m_error) > DVD_MSEC_TO_TIME(10))
    {
      m_pClock->Discontinuity(clock+m_error);
      if(m_speed == DVD_PLAYSPEED_NORMAL)
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Discontinuty - was:%f, should be:%f, error:%f", clock, clock+m_error, m_error);
    }
    else if (m_synctype == SYNC_SKIPDUP && m_skipdupcount == 0 && fabs(m_error) > DVD_MSEC_TO_TIME(10))
    {
      //check how many packets to skip/duplicate
      m_skipdupcount = (int)(m_error / duration);
      //if less than one frame off, see if it's more than two thirds of a frame, so we can get better in sync
      if (m_skipdupcount == 0 && fabs(m_error) > duration / 3 * 2)
        m_skipdupcount = (int)(m_error / (duration / 3 * 2));

      if (m_skipdupcount > 0)
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Duplicating %i packet(s) of %.2f ms duration",
            m_skipdupcount, duration / DVD_TIME_BASE * 1000.0);
      else if (m_skipdupcount < 0)
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Skipping %i packet(s) of %.2f ms duration ",
            m_skipdupcount * -1,  duration / DVD_TIME_BASE * 1000.0);
    }
    else if (m_synctype == SYNC_RESAMPLE)
    {
      //reset the integral on big errors, failsafe
      if (fabs(m_error) > DVD_TIME_BASE)
        m_integral = 0;
      else if (fabs(m_error) > DVD_MSEC_TO_TIME(5))
        m_integral += m_error / DVD_TIME_BASE / INTEGRAL;
    }
  }
}

bool CDVDPlayerAudio::OutputPacket(DVDAudioFrame &audioframe)
{
  if (m_synctype == SYNC_DISCON)
  {
    m_dvdAudio.AddPackets(audioframe);
  }
  else if (m_synctype == SYNC_SKIPDUP)
  {
    if (m_skipdupcount < 0)
    {
      m_prevskipped = !m_prevskipped;
      if (!m_prevskipped)
      {
        m_dvdAudio.AddPackets(audioframe);
        m_skipdupcount++;
      }
    }
    else if (m_skipdupcount > 0)
    {
      m_dvdAudio.AddPackets(audioframe);
      m_dvdAudio.AddPackets(audioframe);
      m_skipdupcount--;
    }
    else if (m_skipdupcount == 0)
    {
      m_dvdAudio.AddPackets(audioframe);
    }
  }
  else if (m_synctype == SYNC_RESAMPLE)
  {
    double proportional = 0.0, proportionaldiv;

    //on big errors use more proportional
    if (fabs(m_error / DVD_TIME_BASE) > 0.0)
    {
      proportionaldiv = PROPORTIONAL * (PROPREF / fabs(m_error / DVD_TIME_BASE));
      if (proportionaldiv < PROPDIVMIN) proportionaldiv = PROPDIVMIN;
      else if (proportionaldiv > PROPDIVMAX) proportionaldiv = PROPDIVMAX;

      proportional = m_error / DVD_TIME_BASE / proportionaldiv;
    }
    m_resampler.SetRatio(1.0 / g_VideoReferenceClock.GetSpeed() + proportional + m_integral);

    //add to the resampler
    m_resampler.Add(audioframe, audioframe.pts);
    //give any packets from the resampler to the audiorenderer
    bool packetadded = false;
    while(m_resampler.Retreive(audioframe, audioframe.pts))
    {
      m_dvdAudio.AddPackets(audioframe);
      packetadded = true;
    }
    return packetadded;
  }

  return true;
}

void CDVDPlayerAudio::OnExit()
{
  g_dvdPerformanceCounter.DisableAudioDecodePerformance();

  CLog::Log(LOGNOTICE, "thread end: CDVDPlayerAudio::OnExit()");
}

void CDVDPlayerAudio::SetSpeed(int speed)
{ 
#ifdef HAS_INTEL_SMD
  if(speed == DVD_PLAYSPEED_PAUSE)
    m_dvdAudio.Pause();
#endif

  if(m_messageQueue.IsInited())
    m_messageQueue.Put( new CDVDMsgInt(CDVDMsg::PLAYER_SETSPEED, speed), 1 );
  else
    m_speed = speed;
}

void CDVDPlayerAudio::Flush()
{
  m_messageQueue.Flush();
  m_messageQueue.Put( new CDVDMsg(CDVDMsg::GENERAL_FLUSH), 1);
}

void CDVDPlayerAudio::WaitForBuffers()
{
  // make sure there are no more packets available
  m_messageQueue.WaitUntilEmpty();

  // make sure almost all has been rendered
  // leave 500ms to avound buffer underruns

  while( m_dvdAudio.GetDelay() > DVD_TIME_BASE/2 )
  {
    Sleep(5);
  }
}

string CDVDPlayerAudio::GetPlayerInfo()
{
  std::ostringstream s;
  s << "aq:"     << setw(2) << min(99,m_messageQueue.GetLevel()) << "%" << "(" <<
      fixed << setprecision(2) << (m_messageQueue.GetLastPTS() - m_messageQueue.GetFirstPTS()) / DVD_TIME_BASE << "s)";
  s << ", kb/s:" << fixed << setprecision(2) << (double)GetAudioBitrate() / 1024.0;
#ifdef HAS_INTEL_SMD
  CStdString timeStr = "N/A";
  double pts = m_messageQueue.GetFirstPTS();
  if (pts != DVD_NOPTS_VALUE)
  {
    pts /= 1000;
    StringUtils::MilisecondsToTimeString((int) pts, timeStr);
  }
  s << ", pts: " << timeStr;
#endif
  return s.str();
}

int CDVDPlayerAudio::GetAudioBitrate()
{
  return (int)m_audioStats.GetBitrate();
}

bool CDVDPlayerAudio::IsStalled()
{
  bool bStalled = m_stalled && m_messageQueue.GetDataSize() == 0;
#ifdef HAS_INTEL_SMD
  unsigned int curDepth = 0, totalDepth = 0;
  if(bStalled)
  {
    g_IntelSMDGlobals.GetPortStatus(g_IntelSMDGlobals.GetAudioDevicePort(g_IntelSMDGlobals.GetPrimaryAudioDevice()),
        curDepth, totalDepth);
    bStalled = (bStalled && curDepth == 0);
  }
#endif
  return bStalled;
}

bool CDVDPlayerAudio::IsPassthrough() const
{
  return m_pAudioCodec && 0 != (FFLAG_PASSTHROUGH & m_pAudioCodec->GetFlags());
}

bool CDVDPlayerAudio::IsHWDecode() const
{
  return m_pAudioCodec && 0 != (FFLAG_HWDECODE & m_pAudioCodec->GetFlags());
}


