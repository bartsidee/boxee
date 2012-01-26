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

#pragma once

#include <deque>
#include "../xbmc/cores/AudioRenderers/IAudioRenderer.h"
#include "utils/Thread.h"
#include "utils/CriticalSection.h"
#include "StdString.h"

struct CAudioData
{
  BYTE*  data;
  int    len;
};

class CGUISoundPlayer : public CThread, public CCriticalSection
{
public:
  static CGUISoundPlayer& GetInstance();
  void Initialize();
  void Deinitialize();
  void Queue(CAudioData& strFile);
  void Stop();
  void SetVolume(int level);
  bool IsPlaying();
  
protected:
  CGUISoundPlayer();
  virtual ~CGUISoundPlayer();
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();  

  std::deque<CAudioData> m_queue;
  HANDLE m_hWorkerEvent;
  IAudioRenderer* m_audioRenderer;
  int              m_volume;
  bool            m_stopped;
};

class CGUISound
{

public:
  CGUISound();
  virtual ~CGUISound();

  bool        Load(const CStdString& strFile);
  void        Play();
  void        Stop();
  bool        IsPlaying();
  void        SetVolume(int level);
  

private:
  bool        LoadWav(const CStdString& strFile, WAVEFORMATEX* wfx, LPBYTE* ppWavData, int* pDataSize);

  BYTE* m_soundBuffer;
  int   m_soundBufferLen;
};
