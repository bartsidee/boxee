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
#ifndef WIN32
#include <config.h>
#endif
#ifdef HAS_DVD_LIBDTS_CODEC
#include "AdvancedSettings.h"
#include "GUISettings.h"
#include "DVDAudioCodecLibDts.h"
#include "DVDStreamInfo.h"
#include "utils/log.h"

#define HEADER_SIZE 14

#define MAKE_DWORD(x) ((unsigned int)((x)[0] << 24) | ((x)[1] << 16) | ((x)[2] << 8) | ((x)[3]))

static inline int16_t convert(int32_t i)
{
#ifdef LIBDTS_FIXED
    i >>= 15;
#else
    i -= 0x43c00000;
#endif
    return (i > 32767) ? 32767 : ((i < -32768) ? -32768 : i);
}

/**
 * \brief Function to convert the "planar" float format used by libdts
 * into the interleaved int16 format used by us.
 * \param in the input buffer containing the planar samples.
 * \param out the output buffer where the interleaved result is stored.
 * \param channels the total number of channels in the decoded data
 */
static int resample_int16(sample_t * in, int16_t *out, unsigned int channels)
{
  unsigned int i, ch;
  int16_t *p = out;
  for(i = 0; i < 256; ++i)
  {
    for(ch = 0; ch < channels; ++ch)
    {
      *p = convert( ((int32_t*)in)[i + (ch << 8)] );
      ++p;
    }
  }
  return p - out; 
}

CDVDAudioCodecLibDts::CDVDAudioCodecLibDts() : CDVDAudioCodec()
{
  m_pState = NULL;
  SetDefault();
}

CDVDAudioCodecLibDts::~CDVDAudioCodecLibDts()
{
  Dispose();
}

bool CDVDAudioCodecLibDts::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (!m_dll.Load())
    return false;

  SetDefault();
  
  m_pState = m_dll.dts_init(0);
  if (!m_pState)
  {
    Dispose();
    return false;
  }

  m_fSamples = m_dll.dts_samples(m_pState);

  // Output will be decided once we query the stream.
  m_iOutputChannels = 0;
  
  CLog::Log(LOGINFO, "CDVDAudioCodecLibDts::Open successful");

  return true;
}

void CDVDAudioCodecLibDts::Dispose()
{
  if (m_pState) m_dll.dts_free(m_pState);
  m_pState = NULL;
}

void CDVDAudioCodecLibDts::SetupChannels(int flags)
{
  /* These are channel mappings that libdts outputs */
  static enum PCMChannels channelMaps[14][6] =
  {
    /* Without LFE */
    {/* DTS_MONO    */ PCM_FRONT_CENTER                                                                                             },
    {/* DTS_STEREO  */ PCM_FRONT_LEFT  , PCM_FRONT_RIGHT                                                                            },
    {/* DTS_3F      */ PCM_FRONT_CENTER, PCM_FRONT_LEFT   , PCM_FRONT_RIGHT                                                         },
    {/* DTS_2F1R    */ PCM_FRONT_LEFT  , PCM_FRONT_RIGHT  , PCM_BACK_CENTER                                                         },
    {/* DTS_3F1R    */ PCM_FRONT_CENTER, PCM_FRONT_LEFT   , PCM_FRONT_RIGHT  , PCM_BACK_CENTER                                      },
    {/* DTS_2F2R    */ PCM_FRONT_LEFT  , PCM_FRONT_RIGHT  , PCM_SIDE_LEFT    , PCM_SIDE_RIGHT                                       },
    {/* DTS_3F2R    */ PCM_FRONT_CENTER, PCM_FRONT_LEFT   , PCM_FRONT_RIGHT  , PCM_SIDE_LEFT  , PCM_SIDE_RIGHT                      },
    /* With LFE */
    {/* DTS_MONO    */ PCM_FRONT_CENTER, PCM_LOW_FREQUENCY                                                                          },
    {/* DTS_STEREO  */ PCM_FRONT_LEFT  , PCM_FRONT_RIGHT  , PCM_LOW_FREQUENCY                                                       },
    {/* DTS_3F      */ PCM_FRONT_CENTER, PCM_FRONT_LEFT   , PCM_FRONT_RIGHT  , PCM_LOW_FREQUENCY                                    },
    {/* DTS_2F1R    */ PCM_FRONT_LEFT  , PCM_FRONT_RIGHT  , PCM_BACK_CENTER                                                         },
    {/* DTS_3F1R    */ PCM_FRONT_CENTER, PCM_FRONT_LEFT   , PCM_FRONT_RIGHT  , PCM_BACK_CENTER, PCM_LOW_FREQUENCY                   },
    {/* DTS_2F2R    */ PCM_FRONT_LEFT  , PCM_FRONT_RIGHT  , PCM_SIDE_LEFT    , PCM_SIDE_RIGHT , PCM_LOW_FREQUENCY                   },
    {/* DTS_3F2R    */ PCM_FRONT_CENTER, PCM_FRONT_LEFT   , PCM_FRONT_RIGHT  , PCM_SIDE_LEFT  , PCM_SIDE_RIGHT   , PCM_LOW_FREQUENCY},
  };

  m_iSourceFlags = flags;

  // setup channel map
  int channels = 0;
  int chOffset = (flags & DTS_LFE) ? 7 : 0;
  switch (m_iSourceFlags & DTS_CHANNEL_MASK)
  {
    case DTS_MONO   : m_pChannelMap = channelMaps[chOffset + 0]; channels = 1; break;
    case DTS_CHANNEL:
    case DTS_DOLBY  :
    case DTS_STEREO : m_pChannelMap = channelMaps[chOffset + 1]; channels = 2; break;
    case DTS_3F     : m_pChannelMap = channelMaps[chOffset + 2]; channels = 3; break;
    case DTS_2F1R   : m_pChannelMap = channelMaps[chOffset + 3]; channels = 3; break;
    case DTS_3F1R   : m_pChannelMap = channelMaps[chOffset + 4]; channels = 4; break;
    case DTS_2F2R   : m_pChannelMap = channelMaps[chOffset + 5]; channels = 4; break;
    case DTS_3F2R   : m_pChannelMap = channelMaps[chOffset + 6]; channels = 5; break;
    default         : m_pChannelMap = NULL;                                    break;
  }
  CLog::Log(LOGINFO, "LIBDTS channels %d LFE %c\n", channels, flags & DTS_LFE ? 't' : 'f');
  
  if(m_pChannelMap == NULL)
    CLog::Log(LOGERROR, "CDVDAudioCodecLibDts::SetupChannels - Invalid channel mapping");

  if( flags & DTS_LFE )
    channels++;

  if(m_iSourceChannels > 0 && m_iSourceChannels != channels)
    CLog::Log(LOGWARNING, "%s - Number of channels changed in stream from %d to %d, data might be truncated", __FUNCTION__, m_iOutputChannels, channels);


  m_iSourceChannels = channels;
  m_iOutputChannels = m_iSourceChannels;
  m_iOutputFlags    = m_iSourceFlags;

  // If we can't support multichannel output downmix
  if (g_guiSettings.GetBool("audiooutput.downmixmultichannel") || m_pChannelMap == NULL)
  {
    m_iOutputChannels = 2;
    m_pChannelMap     = channelMaps[1];
    m_iOutputFlags    = DTS_STEREO;
  }

  // dconti- below only seems to come into play if we actually downmix..

  /* adjust level should always be set, to keep samples in proper range */
  /* after any downmixing has been done */
  m_iOutputFlags |= DTS_ADJUST_LEVEL;
}

int CDVDAudioCodecLibDts::ParseFrame(BYTE* data, int size, BYTE** frame, int* framesize)
{
  int flags, len, framelen;
  BYTE* orig = data;

  *frame     = NULL;
  *framesize = 0;

  if(m_inputSize == 0 && size > HEADER_SIZE)
  {
    // try to sync directly in packet
    m_iFrameSize = m_dll.dts_syncinfo(m_pState, data, &flags, &m_iSourceSampleRate, &m_iSourceBitrate, &framelen);

    if(m_iFrameSize > 0)
    {

      if(m_iSourceFlags != flags)
        SetupChannels(flags);

      if(size >= m_iFrameSize)
      {
        *frame     = data;
        *framesize = m_iFrameSize;
        return m_iFrameSize;
      }
      else
      {
        m_inputSize = size;
        memcpy(m_inputBuffer, data, m_inputSize);
        return m_inputSize;
      }
    }
    else if( size >= 10 )
    {
      // Check for a DTS-HD frame. If found, we drop it, to save ourselves the scan
      int frame_size = ParseDTSHDFrame(data, size);
      if( frame_size > 0 )
      {
        *frame = NULL;
        *framesize = 0;
        return frame_size;
      }
    }
  }

  // attempt to fill up to 7 bytes
  if(m_inputSize < HEADER_SIZE) 
  {
    len = HEADER_SIZE-m_inputSize;
    if(len > size)
      len = size;
    memcpy(m_inputBuffer+m_inputSize, data, len);
    m_inputSize += len;
    data        += len;
    size        -= len;
  }

  if(m_inputSize < HEADER_SIZE) 
    return data - orig;

  // attempt to sync by shifting bytes
  while(true)
  {
    m_iFrameSize = m_dll.dts_syncinfo(m_pState, m_inputBuffer, &flags, &m_iSourceSampleRate, &m_iSourceBitrate, &framelen);
    if(m_iFrameSize > 0)
      break;

    // Scan for DTS-HD
    if( size >= 10 )
    {
      int fs = ParseDTSHDFrame(data, size);
      if( fs > 0 )
      {
        *frame = NULL;
        *framesize = 0;
        return (data+fs) - orig;
      }
    }
    
    if(size == 0)
      return data - orig;

    memmove(m_inputBuffer, m_inputBuffer+1, HEADER_SIZE-1);
    m_inputBuffer[HEADER_SIZE-1] = data[0];
    data++;
    size--;
  }

  if(m_iSourceFlags != flags)
    SetupChannels(flags);

  len = m_iFrameSize-m_inputSize;
  if(len > size)
    len = size;

  memcpy(m_inputBuffer+m_inputSize, data, len);
  m_inputSize += len;
  data        += len;
  size        -= len;

  if(m_inputSize >= m_iFrameSize)
  {
    *frame     = m_inputBuffer;
    *framesize = m_iFrameSize;
    m_inputSize = 0;
  }

  return data - orig;
}

int CDVDAudioCodecLibDts::ParseDTSHDFrame(BYTE* buffer, int len)
{
  // manually process DTSHD frames
  // bytes 0-3  syncword (0x64582025
  //       4    ignored
  //       5-9  bitfield...
  //
  //         2 bits    ignored
  //         1 bit     large header indicator; if set the remaining header is 32 bytes, otherwise 24
  //         8 bits    ignored
  //      if large header:
  //             4 bits  ignored
  //            20 bits  frame size
  //      else
  //            16 bits  frame size

  // 
  if( len < 10 ) return 0;
  
  unsigned int syncword = MAKE_DWORD( &buffer[0] );
  int frame_size = 0;

  if( 0x64582025 == syncword )
  {
    if( buffer[5] & 0x20 )
    {
      unsigned int fs = MAKE_DWORD( &buffer[6] );
      fs = (fs >> 5) & 0x000fffff;
      frame_size = (int) fs + 1;
    }
    else
    {
      unsigned int fs = MAKE_DWORD( &buffer[6] );
      fs = (fs >> 13) & 0x0000ffff;
      frame_size = (int) fs + 1;
    }
  }

  return frame_size;
}

int CDVDAudioCodecLibDts::Decode(BYTE* pData, int iSize)
{
  int len, framesize;
  BYTE* frame;

  m_decodedSize = 0;

  len = ParseFrame(pData, iSize, &frame, &framesize);
  if(!frame)
    return len;
  
  level_t  level = 1.0f;
  sample_t bias  = 384;
  int      flags = m_iOutputFlags;
  
  m_dll.dts_frame(m_pState, frame, &flags, &level, bias);

  if (!g_advancedSettings.m_audioApplyDrc)
    m_dll.dts_dynrng(m_pState, NULL, NULL);
        
  int blocks = m_dll.dts_blocks_num(m_pState);
  for (int i = 0; i < blocks; i++)
  {
    if (m_dll.dts_block(m_pState) != 0)
    {
      CLog::Log(LOGERROR, "CDVDAudioCodecLibDts::Decode : dts_block != 0");
      break;
    }
    m_decodedSize += 2*resample_int16(m_fSamples, (int16_t*)(m_decodedData + m_decodedSize), m_iOutputChannels);
  }
        
  return len;
}

int CDVDAudioCodecLibDts::GetData(BYTE** dst)
{
  *dst = m_decodedData;
  return m_decodedSize;
}

void CDVDAudioCodecLibDts::SetDefault()
{
  m_iFrameSize = 0;
  m_iSourceFlags = 0;
  m_iSourceChannels = 0;
  m_iSourceSampleRate = 0;
  m_iSourceBitrate = 0;
  m_iOutputChannels = 0;
  m_iOutputFlags = 0;
  m_decodedSize = 0;
  m_inputSize = 0;
}

void CDVDAudioCodecLibDts::Reset()
{
  if (m_pState) m_dll.dts_free(m_pState);

  SetDefault();

  m_pState = m_dll.dts_init(0);
  m_fSamples = m_dll.dts_samples(m_pState);
}

#endif
