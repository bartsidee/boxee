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
#include "GUISound.h"
#include "AudioContext.h"
#include "Settings.h"
#include "FileSystem/File.h"
#include "utils/log.h"
#include "FileSystem/SpecialProtocol.h"
#include "../xbmc/cores/AudioRenderers/AudioRendererFactory.h"
#include "utils/SingleLock.h"

#pragma pack(1)
typedef struct
{
  char chunk_id[4];
  int32_t chunksize;
} WAVE_CHUNK;

#pragma pack(1)
typedef struct
{
  char riff[4];
  int32_t filesize;
  char rifftype[4];
} WAVE_RIFFHEADER;

CGUISound::CGUISound()
{
  m_soundBuffer = NULL;
  m_soundBufferLen = 0;
}

CGUISound::~CGUISound()
{
  if (m_soundBuffer)
  {
    delete [] m_soundBuffer;
    m_soundBufferLen = 0;
  }
}
// \brief Loads a wav file by filename
bool CGUISound::Load(const CStdString& strFile)
{
  WAVEFORMATEX wfx;
  DBG(DAUDIO,5,"Load %d wave data", m_soundBufferLen);

#if defined(_LINUX) && !defined(__APPLE__) && !defined(HAS_INTEL_SMD)
  Stop();  // This makes us more responsive.
#endif 

  if (m_soundBuffer)
  {
    delete [] m_soundBuffer;
    m_soundBufferLen = 0;
  }

  if (!LoadWav(strFile, &wfx, &m_soundBuffer, &m_soundBufferLen))
  {
    return false;
  }
  return true;
}

// \brief Starts playback of the sound
void CGUISound::Play()
{
  CAudioData audioData;
  audioData.data = m_soundBuffer;
  audioData.len = m_soundBufferLen;

  CGUISoundPlayer::GetInstance().Queue(audioData);
}

// \brief returns true if the sound is playing
bool CGUISound::IsPlaying()
{
  return CGUISoundPlayer::GetInstance().IsPlaying();
}

// \brief Stops playback if the sound
void CGUISound::Stop()
{
  CGUISoundPlayer::GetInstance().Stop();
}

// \brief Sets the volume of the sound
void CGUISound::SetVolume(int level)
{
  CGUISoundPlayer::GetInstance().SetVolume(level);
}

bool CGUISound::LoadWav(const CStdString& strFile, WAVEFORMATEX* wfx, LPBYTE* ppWavData, int* pDataSize)
{
  XFILE::CFile file;
  if (!file.Open(strFile))
    return false;

  // read header
  WAVE_RIFFHEADER riffh;
  file.Read(&riffh, sizeof(WAVE_RIFFHEADER));

  // file valid?
  if (strncmp(riffh.riff, "RIFF", 4)!=0 && strncmp(riffh.rifftype, "WAVE", 4)!=0)
  {
    file.Close();
    return false;
  }

  long offset=0;
  offset += sizeof(WAVE_RIFFHEADER);
  offset -= sizeof(WAVE_CHUNK);

  // parse chunks
  do
  {
    WAVE_CHUNK chunk;

    // always seeking to the start of a chunk
    file.Seek(offset + sizeof(WAVE_CHUNK), SEEK_SET);
    file.Read(&chunk, sizeof(WAVE_CHUNK));

    if (!strncmp(chunk.chunk_id, "fmt ", 4))
    { // format chunk
      memset(wfx, 0, sizeof(WAVEFORMATEX));
      file.Read(wfx, 16);
      // we only need 16 bytes of the fmt chunk
      if (chunk.chunksize-16>0)
        file.Seek(chunk.chunksize-16, SEEK_CUR);
    }
    else if (!strncmp(chunk.chunk_id, "data", 4))
    { // data chunk
      *ppWavData=new BYTE[chunk.chunksize+1];
      file.Read(*ppWavData, chunk.chunksize);
      *pDataSize=chunk.chunksize;

      if (chunk.chunksize & 1)
        offset++;
    }
    else
    { // other chunk - unused, just skip
      if( chunk.chunksize != 0 )
        file.Seek(chunk.chunksize, SEEK_CUR);
    }

    offset+=(chunk.chunksize+sizeof(WAVE_CHUNK));

    if (offset & 1)
      offset++;

  } while (offset+(int)sizeof(WAVE_CHUNK) < riffh.filesize);

  file.Close();
  return (*ppWavData!=NULL);
}


CGUISoundPlayer& CGUISoundPlayer::GetInstance()
{
  static CGUISoundPlayer g_player;
  return g_player;
}

CGUISoundPlayer::CGUISoundPlayer() : CThread()
{
  m_hWorkerEvent = CreateEvent(NULL, true, false, NULL);
  m_volume = VOLUME_MAXIMUM;
  CThread::SetName("GUISound");
  m_audioRenderer = NULL;
}

CGUISoundPlayer::~CGUISoundPlayer()
{
  m_bStop = true;
  SetEvent(m_hWorkerEvent);
  StopThread();
  CloseHandle(m_hWorkerEvent);
}

void CGUISoundPlayer::Initialize()
{
  CSingleLock locker(*this);
  if (!m_audioRenderer)
    m_audioRenderer = CAudioRendererFactory::Create(NULL, 2, NULL, 44100, 16, false, "gui", true, false);
  else
    m_audioRenderer->Initialize(NULL, 2, NULL, 44100, 16, false, "gui", true, false);

  if (m_audioRenderer)
  {
    m_audioRenderer->SetCurrentVolume(m_volume);
    Create(false, THREAD_MINSTACKSIZE);
  }
}

void CGUISoundPlayer::Deinitialize()
{
  CSingleLock locker(*this);
  
  if (m_audioRenderer)
    m_audioRenderer->Deinitialize();
  
  Stop();
  m_bStop = true;
  ::SetEvent(m_hWorkerEvent);
  
  locker.Leave();
  StopThread();
}

void CGUISoundPlayer::Queue(CAudioData& audioData)
{
  DBG(DAUDIO,5,"Data Len: %d", audioData.len);
  CAudioData newData;
  newData.data = new unsigned char[audioData.len];
  newData.len = audioData.len;
  memcpy(newData.data, audioData.data, audioData.len);
  {
    CSingleLock locker(*this);
    for (size_t i=0; i<m_queue.size(); i++)
      delete [] m_queue[i].data;
    m_queue.clear();
    m_queue.push_back(newData);
    ::SetEvent(m_hWorkerEvent);
  }
}

void CGUISoundPlayer::Stop()
{
  m_stopped = true;
  CSingleLock locker(*this);
  m_queue.clear();
  if (m_audioRenderer)
  m_audioRenderer->Stop();
}

void CGUISoundPlayer::SetVolume(int level)
{
  CSingleLock locker(*this);
  m_volume = level;
  if (m_audioRenderer)
    m_audioRenderer->SetCurrentVolume(level);
}

bool CGUISoundPlayer::IsPlaying()
{
  CSingleLock locker(*this);
  bool result = m_queue.size() > 0;
  return result;
}

void CGUISoundPlayer::OnStartup()
{
}

void CGUISoundPlayer::OnExit()
{
}

void CGUISoundPlayer::Process()
{
  while (!m_bStop)
  {
#ifndef _WIN32
    if (WaitForSingleObject(m_hWorkerEvent, 2000) == WAIT_TIMEOUT)
    {
      if (m_bStop)
        break;
      
#ifndef HAS_INTEL_SMD
      CLog::Log(LOGDEBUG,"no gui sound for 2 seconds. pausing device");
      m_audioRenderer->Pause();
#endif
      WaitForSingleObject(m_hWorkerEvent, INFINITE);
    }
    
#else
    WaitForSingleObject(m_hWorkerEvent, INFINITE);
#endif
    
    m_stopped = false;
    if (m_bStop)
      break;

    CAudioData audioData;

    // EnterCriticalSection protects m_queue.
    ::EnterCriticalSection( this );

    ::ResetEvent(m_hWorkerEvent);
#ifndef HAS_INTEL_SMD
    m_audioRenderer->Resume();
#endif

    while (!m_bStop && m_queue.size())
    {
      audioData = m_queue.front();
      m_queue.pop_front();
      int remaining = (int)audioData.len;
      unsigned char* current = audioData.data;
      int nChunkLen = m_audioRenderer->GetChunkLen();
      ::LeaveCriticalSection(this);

      if(nChunkLen == 0)
        nChunkLen = audioData.len;

      while (!m_bStop && remaining >= nChunkLen && !m_stopped)
      {
        ::EnterCriticalSection(this);
        int processed = m_audioRenderer->AddPackets(current, nChunkLen);
        ::LeaveCriticalSection(this);
        if (processed == 0)
          Sleep(20);
        remaining -= processed;
        current += processed;
      }

      if (!m_bStop && remaining > 0 && !m_stopped)
      {
        unsigned char* data = new unsigned char[nChunkLen];
        ::EnterCriticalSection(this);
        memset(data, 0, nChunkLen);
        memcpy(data, current, remaining);
        m_audioRenderer->AddPackets(data, nChunkLen);
        delete[] data;
        ::LeaveCriticalSection(this);
      }
      
      if (!m_bStop)
      {
        // Not locked so we can stop durring run.
        m_audioRenderer->WaitCompletion();
        m_stopped = false;
      }
      delete [] audioData.data;
      ::EnterCriticalSection(this);
    }
    ::LeaveCriticalSection(this);
  }
}
