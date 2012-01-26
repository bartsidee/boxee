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

#include "PAPlayer.h"
#include "CodecFactory.h"
#include "../../utils/GUIInfoManager.h"
#include "AudioContext.h"
#include "../../FileSystem/FileShoutcast.h"
#include "../../Application.h"
#include "FileItem.h"
#include "AdvancedSettings.h"
#include "GUISettings.h"
#include "Settings.h"
#include "MusicInfoTag.h"
#include "../AudioRenderers/AudioRendererFactory.h"
#include "../../utils/TimeUtils.h"

#ifdef _LINUX
#define XBMC_SAMPLE_RATE 44100
#else
#define XBMC_SAMPLE_RATE 48000
#endif

#define VOLUME_FFWD_MUTE 900 // 9dB

#define FADE_TIME 2 * 2048.0f / XBMC_SAMPLE_RATE.0f      // 2 packets

#define TIME_TO_CACHE_NEXT_FILE 5000L         // 5 seconds
#define TIME_TO_CROSS_FADE      10000L        // 10 seconds

extern XFILE::CFileShoutcast* m_pShoutCastRipper;

// PAP: Psycho-acoustic Audio Player
// Supporting all open  audio codec standards.
// First one being nullsoft's nsv audio decoder format

PAPlayer::PAPlayer(IPlayerCallback& callback) : IPlayer(callback)
{
  m_bIsPlaying = false;
  m_bPaused = false;
  m_cachingNextFile = false;
  m_currentlyCrossFading = false;
  m_bQueueFailed = false;

  m_currentDecoder = 0;

  m_iSpeed = 1;
  m_SeekTime=-1;
  m_IsFFwdRewding = false;
  m_timeOffset = 0;

  for (int i=0; i<2; i++)
  {
    m_channelCount[i]   = 0;
    m_channelMap[i]     = NULL;
    m_sampleRate[i]     = 0;
    m_bitsPerSample[i]  = 0;

    m_pAudioDecoder[i] = NULL;
    m_pcmBuffer[i] = NULL;
    m_bufferPos[i] = 0;
    m_Chunklen[i]  = PACKET_SIZE;
    m_format[i] = AUDIO_MEDIA_FMT_PCM;
    m_passthrough[i] = false;

    m_packet[i][0].packet = NULL;
  }

  m_currentStream = 0;

  m_bytesSentOut = 0;
  m_BytesPerSecond = 0;

  m_resampleAudio = false;

  m_visBufferLength = 0;
  m_pCallback = NULL;

  m_forceFadeToNext = false;
  m_CacheLevel = 0;
  m_LastCacheLevelCheck = 0;

  m_currentFile = new CFileItem;
  m_nextFile = new CFileItem;
}

PAPlayer::~PAPlayer()
{
  CloseFileInternal(true);
  delete m_currentFile;
  m_currentFile = NULL;
  delete m_nextFile;
  m_nextFile = NULL;
}


void PAPlayer::OnExit()
{

}

bool PAPlayer::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  if (m_currentlyCrossFading) CloseFileInternal(false); //user seems to be in a hurry

  m_crossFading = g_guiSettings.GetInt("musicplayer.crossfade");
  //no crossfading for cdda, cd-reading goes mad and no crossfading for last.fm doesn't like two connections
  if (file.IsCDDA() || file.IsLastFM() || file.IsShoutCast() ) m_crossFading = 0;
  if (m_crossFading && IsPlaying())
  {
    //do a short crossfade on trackskip
    //set to max 2 seconds for these prev/next transitions
    if (m_crossFading > 2) m_crossFading = 2;
    //queue for crossfading
    bool result = QueueNextFile(file, false);
    if (result)
    {
      //crossfading value may be update by QueueNextFile when nr of channels changed
      if (!m_crossFading) // swap to next track
        m_decoder[m_currentDecoder].SetStatus(STATUS_ENDED);
      else //force to fade to next track immediately
        m_forceFadeToNext = true;
    }
    return result;
  }

  // normal opening of file, nothing playing or crossfading not enabled
  // however no need to return to gui audio device
  CloseFileInternal(false);

  // always open the file using the current decoder
  m_currentDecoder = 0;

  if (!m_decoder[m_currentDecoder].Create(file, (__int64)(options.starttime * 1000), m_crossFading))
    return false;

  m_iSpeed = 1;
  m_bPaused = false;
  m_bStopPlaying = false;
  m_bytesSentOut = 0;

  CLog::Log(LOGINFO, "PAPlayer: Playing %s", file.m_strPath.c_str());

  m_timeOffset = (__int64)(options.starttime * 1000);

  unsigned int channel, sampleRate, bitsPerSample;
  AudioMediaFormat format;
  bool passthrough;

  m_decoder[m_currentDecoder].GetDataFormat(&channel, &sampleRate, &bitsPerSample, &format, &passthrough);

  if (!CreateStream(m_currentStream, channel, sampleRate, bitsPerSample, format, passthrough))
  {
    m_decoder[m_currentDecoder].Destroy();
    CLog::Log(LOGERROR, "PAPlayer::Unable to create audio stream");
  }

  *m_currentFile = file;

  if (ThreadHandle() == NULL)
    Create();

  m_startEvent.Set();

  m_bIsPlaying = true;
  m_cachingNextFile = false;
  m_currentlyCrossFading = false;
  m_forceFadeToNext = false;
  m_bQueueFailed = false;

  m_decoder[m_currentDecoder].Start();  // start playback

  if (file.HasProperty("pause-on-seek"))
    m_bPauseOnSeek = true;
  else
    m_bPauseOnSeek = false;

  return true;
}

void PAPlayer::UpdateCrossFadingTime(const CFileItem& file)
{
  if ((m_crossFading = g_guiSettings.GetInt("musicplayer.crossfade")))
  {
    if (
      m_crossFading &&
      (
        file.IsCDDA() ||
        file.IsLastFM() ||
        (
          file.HasMusicInfoTag() && !g_guiSettings.GetBool("musicplayer.crossfadealbumtracks") &&
          (m_currentFile->GetMusicInfoTag()->GetAlbum() != "") &&
          (m_currentFile->GetMusicInfoTag()->GetAlbum() == file.GetMusicInfoTag()->GetAlbum()) &&
          (m_currentFile->GetMusicInfoTag()->GetDiscNumber() == file.GetMusicInfoTag()->GetDiscNumber()) &&
          (m_currentFile->GetMusicInfoTag()->GetTrackNumber() == file.GetMusicInfoTag()->GetTrackNumber() - 1)
        )
      )
    )
    {
      m_crossFading = 0;
    }
  }
}

void PAPlayer::OnNothingToQueueNotify()
{
  //nothing to queue, stop playing
  m_bQueueFailed = true;
}

bool PAPlayer::QueueNextFile(const CFileItem &file)
{
  return QueueNextFile(file, true);
}

bool PAPlayer::QueueNextFile(const CFileItem &file, bool checkCrossFading)
{
  if (IsPaused())
    Pause();

  if (file.m_strPath == m_currentFile->m_strPath &&
      file.m_lStartOffset > 0 &&
      file.m_lStartOffset == m_currentFile->m_lEndOffset)
  { // continuing on a .cue sheet item - return true to say we'll handle the transistion
    *m_nextFile = file;
    return true;
  }

  // check if we can handle this file at all
  int decoder = 1 - m_currentDecoder;
  int64_t seekOffset = (file.m_lStartOffset * 1000) / 75;
  if (!m_decoder[decoder].Create(file, seekOffset, m_crossFading))
  {
    m_bQueueFailed = true;
    return false;
  }

  // ok, we're good to go on queuing this one up
  CLog::Log(LOGINFO, "PAPlayer: Queuing next file %s", file.m_strPath.c_str());

  m_bQueueFailed = false;
  if (checkCrossFading)
  {
    UpdateCrossFadingTime(file);
  }

  unsigned int channels, samplerate, bitspersample;
  AudioMediaFormat format;
  bool passthrough;
  m_decoder[decoder].GetDataFormat(&channels, &samplerate, &bitspersample, &format, &passthrough);

  // check the number of channels isn't changing (else we can't do crossfading)
  if (m_crossFading &&
      m_decoder[m_currentDecoder].GetChannels() == channels &&
      m_format[m_currentDecoder] == AUDIO_MEDIA_FMT_PCM     && 
      format == AUDIO_MEDIA_FMT_PCM)
  { // crossfading - need to create a new stream
    if (!CreateStream(1 - m_currentStream, channels, samplerate, bitspersample, format, passthrough))
    {
      m_decoder[decoder].Destroy();
      CLog::Log(LOGERROR, "PAPlayer::Unable to create audio stream");
    }
  }
  else
  { // no crossfading if nr of channels is not the same or if this isn't 100% PCM
    m_crossFading = 0;
  }

  *m_nextFile = file;

  return true;
}



bool PAPlayer::CloseFileInternal(bool bAudioDevice /*= true*/)
{
  if (IsPaused())
    Pause();

  m_bStopPlaying = true;
  m_bStop = true;

  m_visBufferLength = 0;

  // kill both our streams if we need to
  for (int i = 0; i < 2; i++)
  {
    m_decoder[i].Destroy();
    if (bAudioDevice)
      FreeStream(i);
  }

  StopThread();

  if(m_currentFile)
    m_currentFile->Reset();
 
  if(m_nextFile)
    m_nextFile->Reset();

  if(bAudioDevice)
    g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);
  else
    FlushStreams();

  return true;
}

void PAPlayer::FreeStream(int stream)
{
  if (m_pAudioDecoder[stream])
  {
    DrainStream(stream);

    delete m_pAudioDecoder[stream];
    free(m_pcmBuffer[stream]);
  }
  m_pAudioDecoder[stream] = NULL;
  m_pcmBuffer[stream] = NULL;

  if (m_packet[stream][0].packet)
    free(m_packet[stream][0].packet);

  for (int i = 0; i < PACKET_COUNT; i++)
  {
    m_packet[stream][i].packet = NULL;
  }
  if( m_format[stream] == AUDIO_MEDIA_FMT_PCM )
    m_resampler[stream].DeInitialize();
}

void PAPlayer::DrainStream(int stream)
{
  if(m_bStopPlaying || m_pAudioDecoder[1 - stream] || m_format[stream] != AUDIO_MEDIA_FMT_PCM)
  {
    m_pAudioDecoder[stream]->Stop();
    return;
  }

  DWORD silence = m_Chunklen[stream] - m_bufferPos[stream] % m_Chunklen[stream];

  if(silence > 0 && m_bufferPos[stream] > 0)
  {
    CLog::Log(LOGDEBUG, "PAPlayer: Drain - adding %d bytes of silence, real pcmdata size: %d, chunk size: %d", silence, m_bufferPos[stream], m_pAudioDecoder[stream]->GetChunkLen());
    memset(m_pcmBuffer[stream] + m_bufferPos[stream], 0, silence);
    m_bufferPos[stream] += silence;
  }

  DWORD added = 0;
  while(m_bufferPos[stream] - added >= m_Chunklen[stream])
  {
    added += m_pAudioDecoder[stream]->AddPackets(m_pcmBuffer[stream] + added, m_bufferPos[stream] - added);
    Sleep(1);
  }
  m_bufferPos[stream] = 0;

  m_pAudioDecoder[stream]->WaitCompletion();
}

bool PAPlayer::CreateStream(int num, unsigned int channels, unsigned int samplerate, unsigned int bitspersample, AudioMediaFormat format, bool passthrough, CStdString codec)
{
  unsigned int outputSampleRate = samplerate;

  if( channels <= 2 && AUDIO_MEDIA_FMT_PCM == format && g_advancedSettings.m_musicResample )
    outputSampleRate = g_advancedSettings.m_musicResample;

  if (m_pAudioDecoder[num] != NULL &&
      m_channelCount[num] == channels &&
      m_sampleRate[num] == outputSampleRate /* && */
    /*m_bitsPerSample[num] == bitspersample */ &&
      m_format[num] == format &&
      m_passthrough[num] == passthrough)
  {
    CLog::Log(LOGDEBUG, "PAPlayer: Using existing audio renderer");
  }
  else
  {
    FreeStream(num);
    CLog::Log(LOGDEBUG, "PAPlayer: Creating new audio renderer");
    m_bitsPerSample[num]  = (AUDIO_MEDIA_FMT_PCM == format ? 16 : bitspersample);
    m_sampleRate[num]     = outputSampleRate;
    m_channelCount[num]   = channels;
    m_channelMap[num]     = m_decoder[num].GetChannelMap();
    m_format[num]         = format;
    m_passthrough[num]    = passthrough;

    m_BytesPerSecond      = (m_bitsPerSample[num] / 8)* outputSampleRate * channels;

    /* Open the device */
    m_pAudioDecoder[num] = CAudioRendererFactory::Create(m_pCallback, m_channelCount[num], m_channelMap[num], m_sampleRate[num], m_bitsPerSample[num], false, codec.c_str(), true, m_passthrough[num], false, m_format[num]);

    if (!m_pAudioDecoder[num]) return false;

    int nChunkLen = (int)(m_pAudioDecoder[num]->GetChunkLen());
    if(nChunkLen == 0)
      nChunkLen = PACKET_SIZE;

    m_Chunklen[num]  = nChunkLen;

    if( m_format[num] == AUDIO_MEDIA_FMT_PCM )
    {
      m_pcmBuffer[num] = (unsigned char*)malloc((m_Chunklen[num] + PACKET_SIZE));
    }
    else
    {
      m_pcmBuffer[num] = (unsigned char*)malloc(4 * (m_Chunklen[num]));
    }

    m_bufferPos[num] = 0;
    m_latency[num]   = m_pAudioDecoder[num]->GetDelay();

    m_packet[num][0].packet = (BYTE*)malloc(PACKET_SIZE * PACKET_COUNT);
    for (int i = 1; i < PACKET_COUNT ; i++)
      m_packet[num][i].packet = m_packet[num][i - 1].packet + PACKET_SIZE;
  }
  
  // set initial volume
  SetStreamVolume(num, g_stSettings.m_nVolumeLevel);

  if( AUDIO_MEDIA_FMT_PCM == m_format[num] )
    m_resampler[num].InitConverter(samplerate, bitspersample, channels, outputSampleRate, m_bitsPerSample[num], PACKET_SIZE);

  // TODO: How do we best handle the callback, given that our samplerate etc. may be
  // changing at this point?

  // fire off our init to our callback  
  if (m_pCallback)
    m_pCallback->OnInitialize(channels, outputSampleRate, m_bitsPerSample[num]);
  return true;
}

void PAPlayer::Pause()
{
  CLog::Log(LOGDEBUG,"PAPlayer: pause m_bplaying: %d", m_bIsPlaying);
  if (!m_bIsPlaying || !m_pAudioDecoder)
  return ;

  m_bPaused = !m_bPaused;

  if (m_bPaused)
  {
    if (m_pAudioDecoder[m_currentStream])
      m_pAudioDecoder[m_currentStream]->Pause();

    if (m_currentlyCrossFading && m_pAudioDecoder[1 - m_currentStream])
      m_pAudioDecoder[1 - m_currentStream]->Pause();

    m_callback.OnPlayBackPaused();
    CLog::Log(LOGDEBUG, "PAPlayer: Playback paused");
  }
  else
  {
    if (m_pAudioDecoder[m_currentStream])
      m_pAudioDecoder[m_currentStream]->Resume();

    if (m_currentlyCrossFading && m_pAudioDecoder[1 - m_currentStream])
      m_pAudioDecoder[1 - m_currentStream]->Resume();

    m_callback.OnPlayBackResumed();
    CLog::Log(LOGDEBUG, "PAPlayer: Playback resumed");
  }
}

void PAPlayer::SetVolume(long nVolume)
{
  if (m_pAudioDecoder[m_currentStream])
    m_pAudioDecoder[m_currentStream]->SetCurrentVolume(nVolume);
}

void PAPlayer::SetDynamicRangeCompression(long drc)
{
  // TODO: Add volume amplification
  CLog::Log(LOGDEBUG,"PAPlayer::SetDynamicRangeCompression - drc: %lu", drc);
}

void PAPlayer::Process()
{
  CLog::Log(LOGDEBUG, "PAPlayer: Thread started");
  if (m_startEvent.WaitMSec(100))
  {
    m_startEvent.Reset();

    do
    {
      if (!m_bPaused)
      {
        if (!ProcessPAP())
          break;
      }
      else
      {
        Sleep(100);
      }
    }
    while (!m_bStopPlaying && m_bIsPlaying && !m_bStop);

    CLog::Log(LOGINFO, "PAPlayer: End of playback reached");
    m_bIsPlaying = false;
    if (!m_bStopPlaying && !m_bStop)
      m_callback.OnPlayBackEnded();
    else
      m_callback.OnPlayBackStopped();
  }
  CLog::Log(LOGDEBUG, "PAPlayer: Thread end");
}

void PAPlayer::ToFFRW(int iSpeed)
{
  m_iSpeed = iSpeed;
  m_callback.OnPlayBackSpeedChanged(iSpeed);
}

void PAPlayer::UpdateCacheLevel()
{
  //check cachelevel every .5 seconds
  if (m_LastCacheLevelCheck + 500 < CTimeUtils::GetTimeMS())
  {
    ICodec* codec = m_decoder[m_currentDecoder].GetCodec();
    if (codec)
    {
      m_CacheLevel = codec->GetCacheLevel();
      m_LastCacheLevelCheck = CTimeUtils::GetTimeMS();
      //CLog::Log(LOGDEBUG,"Cachelevel: %i%%", m_CacheLevel);
    }
  }
}

bool PAPlayer::ProcessPAP()
{
  /*
   * Here's what we should be doing in each player loop:
   *
   * 1.  Run DoWork() on our audio device to actually output audio.
   *
   * 2.  Pass our current buffer to the audio device to see if it wants anything,
   *     and if so, reduce our buffer size accordingly.
   *
   * 3.  Check whether we have space in our buffer for more data, and if so,
   *     read some more in.
   *
   * 4.  Check for end of file and return false if we reach it.
   *
   * 5.  Perform any seeking and ffwd/rewding as necessary.
   *
   * 6.  If we don't do anything in 2...5, we can take a breather and break out for sleeping.
   */
  while (true)
  {
    if (m_bStop) return false;

    // Check for .cue sheet item end
    if (m_currentFile->m_lEndOffset && GetTime() >= GetTotalTime64())
    {
      CLog::Log(LOGINFO, "PAPlayer: Passed end of track in a .cue sheet item");
      m_decoder[m_currentDecoder].SetStatus(STATUS_ENDED);
    }

    // check whether we need to send off our callbacks etc.
    int status = m_decoder[m_currentDecoder].GetStatus();
    if (status == STATUS_NO_FILE)
      return false;

    UpdateCacheLevel();

    // check whether we should queue the next file up
    if ((GetTotalTime64() > 0) && GetTotalTime64() - GetTime() < TIME_TO_CACHE_NEXT_FILE + m_crossFading * 1000L && !m_cachingNextFile)
    { // request the next file from our application
      m_callback.OnQueueNextItem();
      m_cachingNextFile = true;
    }

    if (m_crossFading &&
        m_decoder[0].GetChannels() == m_decoder[1].GetChannels() &&
        m_format[0] == AUDIO_MEDIA_FMT_PCM &&
        m_format[1] == AUDIO_MEDIA_FMT_PCM)
    {
      if (((GetTotalTime64() - GetTime() < m_crossFading * 1000L) || (m_forceFadeToNext)) && !m_currentlyCrossFading)
      { // request the next file from our application
        if (m_decoder[1 - m_currentDecoder].GetStatus() == STATUS_QUEUED && m_pAudioDecoder[1 - m_currentStream])
        {
          m_currentlyCrossFading = true;
          if (m_forceFadeToNext)
          {
            m_forceFadeToNext = false;
            m_crossFadeLength = m_crossFading * 1000L;
          }
          else
          {
            m_crossFadeLength = GetTotalTime64() - GetTime();
          }
          m_currentDecoder = 1 - m_currentDecoder;
          m_decoder[m_currentDecoder].Start();
          m_currentStream = 1 - m_currentStream;
          CLog::Log(LOGDEBUG, "Starting Crossfade - resuming stream %i", m_currentStream);

          m_pAudioDecoder[m_currentStream]->Resume();

          m_callback.OnPlayBackStarted();
          m_timeOffset = m_nextFile->m_lStartOffset * 1000 / 75;
          m_bytesSentOut = 0;
          *m_currentFile = *m_nextFile;
          m_nextFile->Reset();
          m_cachingNextFile = false;
        }
      }
    }

    // Check for EOF and queue the next track if applicable
    if (m_decoder[m_currentDecoder].GetStatus() == STATUS_ENDED)
    { // time to swap tracks
      if (m_nextFile->m_strPath != m_currentFile->m_strPath ||
          !m_nextFile->m_lStartOffset ||
          m_nextFile->m_lStartOffset != m_currentFile->m_lEndOffset)
      { // don't have a .cue sheet item
        int nextstatus = m_decoder[1 - m_currentDecoder].GetStatus();
        if (nextstatus == STATUS_QUEUED || nextstatus == STATUS_QUEUING || nextstatus == STATUS_PLAYING)
        { // swap streams
          CLog::Log(LOGDEBUG, "PAPlayer: Swapping tracks %i to %i", m_currentDecoder, 1-m_currentDecoder);
          if (!m_crossFading || m_decoder[0].GetChannels() != m_decoder[1].GetChannels())
          {
            // check if we need to change the resampler (due to format change)
            unsigned int channels, samplerate, bitspersample;
            unsigned int channels2, samplerate2, bitspersample2;
            AudioMediaFormat format, format2;
            bool passthrough, passthrough2;

            m_decoder[m_currentDecoder].GetDataFormat(&channels, &samplerate, &bitspersample, &format, &passthrough);
            m_decoder[1 - m_currentDecoder].GetDataFormat(&channels2, &samplerate2, &bitspersample2, &format2, &passthrough2);

            // playing gapless (we use only the 1 output stream in this case)
            if( AUDIO_MEDIA_FMT_PCM == format && AUDIO_MEDIA_FMT_PCM == format2 )
            {
              int prefixAmount = m_decoder[m_currentDecoder].GetDataSize();
              CLog::Log(LOGDEBUG, "PAPlayer::Prefixing %i samples of old data to new track for gapless playback", prefixAmount);
              m_decoder[1 - m_currentDecoder].PrefixData(m_decoder[m_currentDecoder].GetData(prefixAmount), prefixAmount);
            }

            // change of channels - reinitialize our speaker configuration
            if (channels != channels2       ||
                format != format2           ||
                passthrough != passthrough2 ||
                (g_advancedSettings.m_musicResample == 0 &&
                   (samplerate != samplerate2 || bitspersample != bitspersample2)))
            {
              CLog::Log(LOGINFO, "PAPlayer: Stream properties have changed, restarting stream");
              FreeStream(m_currentStream);
              if (!CreateStream(m_currentStream, channels2, samplerate2, bitspersample2, format2, passthrough2))
              {
                CLog::Log(LOGERROR, "PAPlayer: Error creating stream!");
                return false;
              }
              m_pAudioDecoder[m_currentStream]->Resume();
            }
            else if (samplerate != samplerate2 || bitspersample != bitspersample2)
            {
              CLog::Log(LOGINFO, "PAPlayer: Restarting resampler due to a change in data format");
              if( format == AUDIO_MEDIA_FMT_PCM )
                m_resampler[m_currentStream].DeInitialize();
              if( format2 == AUDIO_MEDIA_FMT_PCM &&
                  !m_resampler[m_currentStream].InitConverter(samplerate2, bitspersample2, channels2, g_advancedSettings.m_musicResample, 16, PACKET_SIZE))
              {
                CLog::Log(LOGERROR, "PAPlayer: Error initializing resampler!");
                return false;
              }
            }
            CLog::Log(LOGINFO, "PAPlayer: Starting new track");

            m_decoder[m_currentDecoder].Destroy();
            m_decoder[1 - m_currentDecoder].Start();
            m_callback.OnPlayBackStarted();
            m_timeOffset = m_nextFile->m_lStartOffset * 1000 / 75;
            m_bytesSentOut = 0;
            *m_currentFile = *m_nextFile;
            m_nextFile->Reset();
            m_cachingNextFile = false;
            m_currentDecoder = 1 - m_currentDecoder;
          }
          else
          { // cross fading - shouldn't ever get here - if we do, return false
            if (!m_currentlyCrossFading)
            {
              CLog::Log(LOGERROR, "End of file Reached before crossfading kicked in!");
              return false;
            }
            else
            {
              CLog::Log(LOGINFO, "End of file reached before crossfading finished!");
              return false;
            }
          }
        }
        else
        {
          if (GetTotalTime64() <= 0 && !m_bQueueFailed)
          { //we did not know the duration so didn't queue the next song, try queueing it now
            if (!m_cachingNextFile)
            {// request the next file from our application
              m_callback.OnQueueNextItem();
              m_cachingNextFile = true;
            }
          }
          else
          {
            // no track queued - return and get another one once we are finished
            // with the current stream
            WaitForStream();
            return false;
          }
        }
      }
      else
      {
        // set the next track playing (.cue sheet)
        m_decoder[m_currentDecoder].SetStatus(STATUS_PLAYING);
        m_callback.OnPlayBackStarted();
        m_timeOffset = m_nextFile->m_lStartOffset * 1000 / 75;
        m_bytesSentOut = 0;
        *m_currentFile = *m_nextFile;
        m_nextFile->Reset();
        m_cachingNextFile = false;
      }
    }

    // handle seeking and ffwd/rewding.
    HandleSeeking();
    if (!HandleFFwdRewd())
    {
      // need to skip to the next track - let's see if we already have another one
      m_decoder[m_currentDecoder].SetStatus(STATUS_ENDED);
      continue; // loop around to start the next track
    }

    if (!m_bPaused)
    {

      // Let our decoding stream(s) do their thing
      int retVal = m_decoder[m_currentDecoder].ReadSamples(PACKET_SIZE);
      if (retVal == RET_ERROR)
      {
        m_decoder[m_currentDecoder].Destroy();
        return false;
      }

      int retVal2 = m_decoder[1 - m_currentDecoder].ReadSamples(PACKET_SIZE);
      if (retVal2 == RET_ERROR)
      {
        m_decoder[1 - m_currentDecoder].Destroy();
      }

      // if we're cross-fading, then we do this for both streams, otherwise
      // we do it just for the one stream.
      if (m_currentlyCrossFading)
      {
        if (GetTime() >= m_crossFadeLength)  // finished
        {
          CLog::Log(LOGDEBUG, "Finished Crossfading");
          m_currentlyCrossFading = false;
          SetStreamVolume(m_currentStream, g_stSettings.m_nVolumeLevel);
          FreeStream(1 - m_currentStream);
          m_decoder[1 - m_currentDecoder].Destroy();
        }
        else
        {
          float fraction = (float)(m_crossFadeLength - GetTime()) / (float)m_crossFadeLength - 0.5f;
          // make sure we can take valid logs.
          if (fraction > 0.499f) fraction = 0.499f;
          if (fraction < -0.499f) fraction = -0.499f;
          float volumeCurrent = 2000.0f * log10(0.5f - fraction);
          float volumeNext = 2000.0f * log10(0.5f + fraction);
          SetStreamVolume(m_currentStream, g_stSettings.m_nVolumeLevel + (int)volumeCurrent);
          SetStreamVolume(1 - m_currentStream, g_stSettings.m_nVolumeLevel + (int)volumeNext);
          if (AddPacketsToStream(1 - m_currentStream, m_decoder[1 - m_currentDecoder]))
            retVal2 = RET_SUCCESS;
        }
      }

      // add packets as necessary
      if (AddPacketsToStream(m_currentStream, m_decoder[m_currentDecoder]))
        retVal = RET_SUCCESS;

      if (retVal == RET_SLEEP && retVal2 == RET_SLEEP)
      {
        float maximumSleepTime = m_pAudioDecoder[m_currentStream]->GetCacheTime();
        
        if (m_pAudioDecoder[1 - m_currentStream])
          maximumSleepTime = std::min(maximumSleepTime, m_pAudioDecoder[1 - m_currentStream]->GetCacheTime());

        int sleep = std::max((int)((maximumSleepTime / 2.0f) * 1000.0f), 1);

        Sleep(std::min(sleep, 15));
    }
    }
    else
      Sleep(100);
  }
  return true;
}

void PAPlayer::ResetTime()
{
  m_bytesSentOut = 0;
}

__int64 PAPlayer::GetTime()
{
  if( m_format[m_currentDecoder] != AUDIO_MEDIA_FMT_PCM )
    return m_decoder[m_currentDecoder].GetTime();
  else
  {
    __int64  timeplus = m_BytesPerSecond ? (__int64)(((float) m_bytesSentOut / (float) m_BytesPerSecond ) * 1000.0) : 0;
    if (m_pAudioDecoder[m_currentStream])
      timeplus -= (__int64)(m_pAudioDecoder[m_currentStream]->GetDelay() * 1000.0f);
    return m_timeOffset + timeplus - m_currentFile->m_lStartOffset * 1000 / 75;
  }
}

__int64 PAPlayer::GetTotalTime64()
{
  __int64 total = m_decoder[m_currentDecoder].TotalTime();
  if (m_currentFile->m_lEndOffset)
    total = m_currentFile->m_lEndOffset * 1000 / 75;
  if (m_currentFile->m_lStartOffset)
    total -= m_currentFile->m_lStartOffset * 1000 / 75;
  return total;
}

int PAPlayer::GetTotalTime()
{
  return (int)(GetTotalTime64()/1000);
}

int PAPlayer::GetCacheLevel() const
{
  const ICodec* codec = m_decoder[m_currentDecoder].GetCodec();
  if (codec)
    return codec->GetCacheLevel();

  return -1;
}

int PAPlayer::GetChannels()
{
  ICodec* codec = m_decoder[m_currentDecoder].GetCodec();
  if (codec)
    return codec->m_Channels;
  return 0;
}

int PAPlayer::GetBitsPerSample()
{
  ICodec* codec = m_decoder[m_currentDecoder].GetCodec();
  if (codec)
    return codec->m_BitsPerSample;
  return 0;
}

int PAPlayer::GetSampleRate()
{
  ICodec* codec = m_decoder[m_currentDecoder].GetCodec();
  if (codec)
    return (int)((codec->m_SampleRate / 1000) + 0.5);
  return 0;
}

CStdString PAPlayer::GetAudioCodecName()
{
  ICodec* codec = m_decoder[m_currentDecoder].GetCodec();
  if (codec)
    return codec->m_CodecName;
  return "";
}

int PAPlayer::GetAudioBitrate()
{
  ICodec* codec = m_decoder[m_currentDecoder].GetCodec();
  if (codec)
    return (int)((codec->m_Bitrate / 1000) + 0.5); // in kbits/s, rounded to the nearest int
  return 0;
}

bool PAPlayer::IsPassthrough() const
{
  if( m_format[m_currentStream] != AUDIO_MEDIA_FMT_PCM && m_passthrough[m_currentStream] )
    return true;
  return false;
}

bool PAPlayer::CanSeek()
{
  return ((m_decoder[m_currentDecoder].TotalTime() > 0) && m_decoder[m_currentDecoder].CanSeek());
}

bool PAPlayer::CanSeekToTime()
{
  return CanSeek();
}

void PAPlayer::Seek(bool bPlus, bool bLargeStep)
{
  __int64 seek;
  if (g_advancedSettings.m_musicUseTimeSeeking && GetTotalTime() > 2*g_advancedSettings.m_musicTimeSeekForwardBig)
  {
    if (bLargeStep)
      seek = bPlus ? g_advancedSettings.m_musicTimeSeekForwardBig : g_advancedSettings.m_musicTimeSeekBackwardBig;
    else
      seek = bPlus ? g_advancedSettings.m_musicTimeSeekForward : g_advancedSettings.m_musicTimeSeekBackward;
    seek *= 1000;
    seek += GetTime();
  }
  else
  {
    float percent;
    if (bLargeStep)
      percent = bPlus ? (float)g_advancedSettings.m_musicPercentSeekForwardBig : (float)g_advancedSettings.m_musicPercentSeekBackwardBig;
    else
      percent = bPlus ? (float)g_advancedSettings.m_musicPercentSeekForward : (float)g_advancedSettings.m_musicPercentSeekBackward;
    seek = (__int64)(GetTotalTime64()*(GetPercentage()+percent)/100);
  }

  SeekTime(seek);
}

void PAPlayer::SeekTime(__int64 iTime /*=0*/)
{
  if (!CanSeek()) return;
  int seekOffset = (int)(iTime - GetTime());
  if (m_currentFile->m_lStartOffset)
    iTime += m_currentFile->m_lStartOffset * 1000 / 75;
  m_SeekTime = iTime;
  m_callback.OnPlayBackSeek((int)m_SeekTime, seekOffset);
  CLog::Log(LOGDEBUG, "PAPlayer::Seeking to time %f", 0.001f * m_SeekTime);
}

void PAPlayer::SeekPercentage(float fPercent /*=0*/)
{
  if (fPercent < 0.0f) fPercent = 0.0f;
  if (fPercent > 100.0f) fPercent = 100.0f;
  SeekTime((__int64)(fPercent * 0.01f * (float)GetTotalTime64()));
}

float PAPlayer::GetPercentage()
{
  float percent = (float)GetTime() * 100.0f / GetTotalTime64();
  return percent;
}

void PAPlayer::HandleSeeking()
{
  if (m_SeekTime != -1)
  {
    if (m_bPauseOnSeek)
      m_bPaused = true;

    DWORD time = CTimeUtils::GetTimeMS();
    m_timeOffset = m_decoder[m_currentDecoder].Seek(m_SeekTime);
    CLog::Log(LOGDEBUG, "Seek to time %f took %i ms", 0.001f * m_SeekTime, CTimeUtils::GetTimeMS() - time);
    FlushStreams();
    m_SeekTime = -1;
  }
  g_infoManager.m_performingSeek = false;
}

void PAPlayer::FlushStreams()
{
  m_bytesSentOut = 0;
  for (int stream = 0; stream < 2; stream++)
  {
    if (m_pAudioDecoder[stream] && m_packet[stream])
    {
#ifdef HAS_INTEL_SMD
      m_pAudioDecoder[stream]->Pause();
#else
      m_pAudioDecoder[stream]->Stop();
#endif
      m_pAudioDecoder[stream]->Resume();
      m_bufferPos[stream] = 0;
      if (m_format[stream] == AUDIO_MEDIA_FMT_PCM)
      {
        m_resampler[stream].DeInitialize();
        m_resampler[stream].InitConverter(m_sampleRate[stream] ,m_bitsPerSample[stream] ,m_channelCount[stream] ,m_sampleRate[stream] ,m_bitsPerSample[stream] ,PACKET_SIZE);
      }
    }
  }
}

bool PAPlayer::HandleFFwdRewd()
{
  if (!m_IsFFwdRewding && m_iSpeed == 1)
    return true;  // nothing to do
  if (m_IsFFwdRewding && m_iSpeed == 1)
  { // stop ffwd/rewd
    m_IsFFwdRewding = false;
    SetVolume(g_stSettings.m_nVolumeLevel);
    FlushStreams();
    return true;
  }
  // we're definitely fastforwarding or rewinding
  int snippet = m_BytesPerSecond / 2;
  if ( m_bytesSentOut >= snippet )
  {
    // Calculate offset to seek if we do FF/RW
    __int64 time = GetTime();
    if (m_IsFFwdRewding) snippet = (int)m_bytesSentOut;
    time += (__int64)((double)snippet * (m_iSpeed - 1.0) / m_BytesPerSecond * 1000.0);

    // Is our offset inside the track range?
    if (time >= 0 && time <= m_decoder[m_currentDecoder].TotalTime())
    { // just set next position to read
      m_IsFFwdRewding = true;
      time += m_currentFile->m_lStartOffset * 1000 / 75;
      m_timeOffset = m_decoder[m_currentDecoder].Seek(time);
      FlushStreams();
      SetVolume(g_stSettings.m_nVolumeLevel - VOLUME_FFWD_MUTE); // override xbmc mute
    }
    else if (time < 0)
    { // ...disable seeking and start the track again
      time = m_currentFile->m_lStartOffset * 1000 / 75;
      m_timeOffset = m_decoder[m_currentDecoder].Seek(time);
      FlushStreams();
      m_iSpeed = 1;
      SetVolume(g_stSettings.m_nVolumeLevel); // override xbmc mute
    } // is our next position greater then the end sector...
    else //if (time > m_codec->m_TotalTime)
    {
      // restore volume level so the next track isn't muted
      SetVolume(g_stSettings.m_nVolumeLevel);
      CLog::Log(LOGDEBUG, "PAPlayer: End of track reached while seeking");
      return false;
    }
  }
  return true;
}

void PAPlayer::SetStreamVolume(int stream, long nVolume)
{
  m_pAudioDecoder[stream]->SetCurrentVolume(nVolume);
}

bool PAPlayer::AddPacketsToStream(int stream, CAudioDecoder &dec)
{
  bool ret = false;

  if (!m_pAudioDecoder[stream] || dec.GetStatus() == STATUS_NO_FILE)
    return false;

  if( m_format[stream] == AUDIO_MEDIA_FMT_PCM )
  {
    int amount = m_resampler[stream].GetInputSamples();
    if (amount > 0 && amount <= (int)dec.GetDataSize())
    { // resampler wants more data - let's feed it
      m_resampler[stream].PutFloatData((float *)dec.GetData(amount), amount);
      ret = true;
    }
    else if (m_Chunklen[stream] > m_pAudioDecoder[stream]->GetSpace())
    { // resampler probably have data but wait until we can send atleast a packet
      ret = false;
    }
    else if (m_resampler[stream].GetData(m_packet[stream][0].packet))
    {
      // got some data from our resampler - construct audio packet
      m_packet[stream][0].length = PACKET_SIZE;
      m_packet[stream][0].stream = stream;

      unsigned char *pcmPtr = m_packet[stream][0].packet;
      int len = m_packet[stream][0].length;
      StreamCallback(&m_packet[stream][0]);

      memcpy(m_pcmBuffer[stream]+m_bufferPos[stream], pcmPtr, len);
      m_bufferPos[stream] += len;

      while (m_bufferPos[stream] >= (int)m_Chunklen[stream])
      {
        int rtn = m_pAudioDecoder[stream]->AddPackets(m_pcmBuffer[stream], m_bufferPos[stream]);
        m_bufferPos[stream] -= rtn;
        memmove(m_pcmBuffer[stream], m_pcmBuffer[stream] + rtn, m_bufferPos[stream]);
      }

      // something done
      ret = true;
    }
  }
  else
  {
    // either passthrough or hw decode mode
    // exit out if there's no space
    if( m_Chunklen[stream] > m_pAudioDecoder[stream]->GetSpace())
    {
      ret = false;
    } else
    {
      // outer loop pulls data from the decoder
      // inner loop feeds the renderer
      unsigned int dataSize = 0;
      for( dataSize = dec.GetDataSize(); dataSize >= m_Chunklen[stream]; dataSize = dec.GetDataSize() )
      {
        // can copy up to 4 chunks
        if( m_bufferPos[stream] <= (int) (3*m_Chunklen[stream]) )
        {
          void* data = dec.GetData(m_Chunklen[stream]);
          memcpy( m_pcmBuffer[stream] + m_bufferPos[stream], data, m_Chunklen[stream] );
          m_bufferPos[stream] += m_Chunklen[stream];
        }

        while( (unsigned int) m_bufferPos[stream] >= m_Chunklen[stream] && m_Chunklen[stream] <= m_pAudioDecoder[stream]->GetSpace() )
        {
          int copied = m_pAudioDecoder[stream]->AddPackets(m_pcmBuffer[stream], m_bufferPos[stream]);
          m_bufferPos[stream] -= copied;
          memmove( m_pcmBuffer[stream], m_pcmBuffer[stream] + copied, m_bufferPos[stream]);
        }

        ret = true;

        // exit here if we are full
        if( m_Chunklen[stream] > m_pAudioDecoder[stream]->GetSpace() ||
            (m_bufferPos[stream] == (int) (4*m_Chunklen[stream])) )
          break;
      }
    }
  }

  return ret;
}

bool PAPlayer::FindFreePacket( int stream, DWORD* pdwPacket )
{
  return true;
}

void PAPlayer::RegisterAudioCallback(IAudioCallback *pCallback)
{
  m_pCallback = pCallback;
  if (m_pCallback)
    m_pCallback->OnInitialize(m_channelCount[m_currentStream], m_sampleRate[m_currentStream], m_bitsPerSample[m_currentStream]);
}

void PAPlayer::UnRegisterAudioCallback()
{
  m_pCallback = NULL;
}

void PAPlayer::DoAudioWork()
{
  if (m_pCallback && m_visBufferLength)
  {
    m_pCallback->OnAudioData((BYTE*)m_visBuffer, m_visBufferLength);
    m_visBufferLength = 0;
  }
}

void PAPlayer::StreamCallback( LPVOID pPacketContext )
{
  AudioPacket *pkt = (AudioPacket *)pPacketContext;


  // only process from the current stream (if we're crossfading for instance)
  if (pkt->stream != m_currentStream)
    return;

  m_bytesSentOut += pkt->length;

  if (m_pCallback)
  { // copy into our visualisation buffer.
    // can't use a memcpy() here due to the context (will crash otherwise)
    memcpy((short*)m_visBuffer, pkt->packet, pkt->length);
    m_visBufferLength = pkt->length;
  }
}

void CALLBACK StaticStreamCallback( VOID* pStreamContext, VOID* pPacketContext, DWORD dwStatus )
{
  PAPlayer* pPlayer = (PAPlayer*)pStreamContext;
  pPlayer->StreamCallback(pPacketContext);
}

bool PAPlayer::HandlesType(const CStdString &type)
{
  ICodec* codec=CodecFactory::CreateCodec(type);

  if (codec && codec->CanInit())
  {
    delete codec;
    return true;
  }
  if (codec)
    delete codec;

  return false;
}

// Skip to next track/item inside the current media (if supported).
bool PAPlayer::SkipNext()
{
  if (m_decoder[m_currentDecoder].GetCodec() && m_decoder[m_currentDecoder].GetCodec()->SkipNext())
  {
    return true;
  }
  return false;
}

bool PAPlayer::CanRecord()
{
  if (!m_pShoutCastRipper) return false;
  return m_pShoutCastRipper->CanRecord();
}

bool PAPlayer::IsRecording()
{
  if (!m_pShoutCastRipper) return false;
  return m_pShoutCastRipper->IsRecording();
}

bool PAPlayer::Record(bool bOnOff)
{
  if (!m_pShoutCastRipper) return false;
  if (bOnOff && IsRecording()) return true;
  if (bOnOff == false && IsRecording() == false) return true;
  if (bOnOff)
    return m_pShoutCastRipper->Record();

  m_pShoutCastRipper->StopRecording();
  return true;
}

void PAPlayer::WaitForStream()
{
  // should we wait for our other stream as well?
  // currently we don't.
  if (m_pAudioDecoder[m_currentStream])
  {
    m_pAudioDecoder[m_currentStream]->WaitCompletion();
  }
}

bool PAPlayer::CanSkip() const
{
  if (m_currentFile->HasProperty("no-skip"))
    return false;

  return true;
}

bool PAPlayer::CanPause() const
{
  if (m_currentFile->HasProperty("no-pause"))
    return false;

  return true;
}
