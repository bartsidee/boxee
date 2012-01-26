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
#include "DVDAudioCodec.h"
#ifndef _WIN32
#include <config.h>
#endif
#ifdef HAS_DVD_LIBA52_CODEC
#include "DllLiba52.h"
#endif
#ifdef HAS_DVD_LIBDTS_CODEC
#include "DllLibDts.h"
#endif

class CDVDAudioCodecPassthrough : public CDVDAudioCodec
{
public:
  CDVDAudioCodecPassthrough();
  virtual ~CDVDAudioCodecPassthrough();

  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual int GetData(BYTE** dst);
  virtual void Reset();
  virtual int GetChannels();
  virtual enum PCMChannels* GetChannelMap();
  virtual int GetSampleRate();
  virtual int GetBitsPerSample();
  virtual unsigned char GetFlags();
  virtual const char* GetName()  { return "passthrough"; }
  
private:
  enum frametype
  {
    AC3 = 0,
    DTS,
    TrueHD,
    DTS_HD
  };
  int ParseFrame(BYTE* data, int size, BYTE** frame, int* framesize, frametype* ft);
  int ParseTrueHDFrame(BYTE* buffer, int* sampleRate, int* bitRate);
  int ParseDTSHDFrame(BYTE* buffer, int* sampleRate, int* bitRate, int* samplesPerFrame );
  int PaddAC3Data( BYTE* pData, int iDataSize, BYTE* pOut);
  int PaddTrueHDData( BYTE* pData, int iDataSize, BYTE* pOut);
  int PaddDTSData( BYTE* pData, int iDataSize, BYTE* pOut);
  int PaddDTSHDData( BYTE* pData, int iDataSize, bool bIsDTSHDFrame, BYTE* pOut);
  
  static bool IsPassthroughAudioCodec( int Codec );

  BYTE m_OutputBuffer[131072];
  int  m_OutputSize;

  // keep enough space to buffer a full dts-hd frame, since dts parsing in ffmpeg is marginal at best
  BYTE m_InputBuffer[8192];
  int  m_InputSize;

  int m_iFrameSize;

  int m_iSamplesPerFrame;
  int m_iChannels;

  int     m_Codec;
  bool    m_Synced;

  int m_iSourceFlags;
  int m_iSourceSampleRate;
  int m_iSourceBitrate;

#ifdef HAS_DVD_LIBDTS_CODEC
  DllLibDts m_dllDTS;
  dts_state_t* m_pStateDTS;
#endif
#ifdef HAS_DVD_LIBA52_CODEC
  DllLiba52 m_dllA52;
  a52_state_t* m_pStateA52;
#endif

  // State tracking for trueHD streams
  int m_iByteCounter;
  unsigned int m_uiLastFrameTime;
  int m_iLastSampleRateBit;
  int m_iLastFrameSize;

  bool m_bIsDTSHD;
  bool m_bBitstreamDTSHD;

  bool m_bHardwareBitStream;
  bool m_bHardwareDecode;

  bool m_bFirstDTSFrame;
  BYTE m_DTSPrevFrame[2048];
  int  m_DTSPrevFrameLen;
};
