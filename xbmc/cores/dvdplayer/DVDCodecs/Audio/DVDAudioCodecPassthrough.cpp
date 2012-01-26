/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *  The following functions are based on ffdshow-tryout and are also (C) ffdshow-tryout
 *   CDVDAudioCodecPassthrough::PaddTrueHDData
 *   CDVDAudioCodecPassthrough::PaddDTSHDData
 *   CDVDAudioCodecPassthrough::ParseTrueHDFrame
 *   CDVDAudioCodecPassthrough::ParseDTSHDFrame
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
#include "system.h"
#if defined HAS_DVD_LIBA52_CODEC && defined HAS_DVD_LIBDTS_CODEC
 
#include "DVDAudioCodecPassthrough.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDStreamInfo.h"
#include "DVDPlayerAudio.h"
#include "GUISettings.h"
#include "AdvancedSettings.h"
#include "Settings.h"
#include "utils/log.h"

#undef  MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#define OUT_SAMPLESIZE 16
#define OUT_CHANNELS 2
#define OUT_SAMPLERATE 48000

// per above this is only meaningful for 2 channel content...
#define OUT_SAMPLESTOBYTES(a) ((a) * OUT_CHANNELS * (OUT_SAMPLESIZE>>3))

// Size of frame headers
#define HEADER_SIZE 20

/* swab even and uneven data sizes. make sure dst can hold an size aligned to 2 */
static inline int swabdata(char* dst, char* src, int size)
{
  if( size & 0x1 )
  {
    swab(src, dst, size-1);
    dst+=size-1;
    src+=size-1;

    dst[0] = 0x0;
    dst[1] = src[0];
    return size+1;
  }
  else
  {
    swab(src, dst, size);
    return size;
  }

}

CDVDAudioCodecPassthrough::CDVDAudioCodecPassthrough(void)
{
  m_pStateA52 = NULL;
  m_pStateDTS = NULL;

  m_iFrameSize = 0;
  m_iLastFrameSize = 0;
  m_OutputSize = 0;
  m_InputSize  = 0;

  m_iByteCounter = 0;
  m_uiLastFrameTime = 0;
  m_iSamplesPerFrame = 0;
  m_iLastSampleRateBit = 0;

  m_bIsDTSHD = false;
  m_bFirstDTSFrame = true;
  m_DTSPrevFrameLen = 0;

  m_bHardwareBitStream = g_advancedSettings.m_bForceAudioHardwarePassthrough;

  m_bHardwareDecode = false;
}

CDVDAudioCodecPassthrough::~CDVDAudioCodecPassthrough(void)
{
  Dispose();
}


int CDVDAudioCodecPassthrough::PaddDTSData( BYTE* pData, int iDataSize, BYTE* pOut)
{
  /* we always output aligned sizes to allow for byteswapping*/
  int iDataSize2 = (iDataSize+1) & ~1;

  pOut[0] = 0x72; pOut[1] = 0xf8; /* iec 61937     */
  pOut[2] = 0x1f; pOut[3] = 0x4e; /*  syncword     */

  switch( m_iSamplesPerFrame )
  {
  case 512:
    pOut[4] = 0x0b;      /* DTS-1 (512-sample bursts) */
    break;
  case 1024:
    pOut[4] = 0x0c;      /* DTS-2 (1024-sample bursts) */
    break;
  case 2048:
    pOut[4] = 0x0d;      /* DTS-3 (2048-sample bursts) */
    break;
  default:
    CLog::Log(LOGERROR, "CDVDAudioCodecPassthrough::PaddDTSData - DTS: %d-sample bursts not supported\n", m_iSamplesPerFrame);
    pOut[4] = 0x00;
    break;
  }

  pOut[5] = 0;                      /* ?? */    
  pOut[6] = (iDataSize2 << 3) & 0xFF;
  pOut[7] = (iDataSize2 >> 5) & 0xFF;

  int iOutputSize = OUT_SAMPLESTOBYTES(m_iSamplesPerFrame);

  if ( iDataSize2 > iOutputSize - 8 ) 
  {          
    //Crap frame with more data than we can handle, can be worked around i think
    CLog::Log(LOGERROR, "CDVDAudioCodecPassthrough::PaddDTSData - larger frame than will fit, skipping");
    return 0;
  }

  //Swap byteorder if syncword indicates bigendian
  if( pData[0] == 0x7f || pData[0] == 0x1f )
    swabdata((char*)pOut+8, (char*)pData, iDataSize);
  else
    memcpy((char*)pOut+8, (char*)pData, iDataSize);

  memset(pOut + iDataSize2 + 8, 0, iOutputSize - iDataSize2 - 8);  

  return iOutputSize;
}

int CDVDAudioCodecPassthrough::PaddDTSHDData( BYTE* pData, int iDataSize, bool bIsDTSHDFrame, BYTE* pOut)
{
  int idx = 0;

  // First and foremost. If we don't support DTSHD bitstream, then drop DTSHD frames and pass on the 
  // standard DTS ones
  if( !m_bBitstreamDTSHD )
  {
    if( bIsDTSHDFrame )
      return 0;
    else
      return PaddDTSData( pData, iDataSize, pOut );
  }

  // With DTS-HD we will get intermingled DTS and DTS-HD frames. So we need to keep the first
  // DTS frame for any DTS or DTS-HD stream, just in case the next frame is DTS-HD. If the second
  // frame isn't DTS-HD, we push out the first frame with the second as regular DTS, and after that
  // we pass all frames directly to the DTS handler
  // If the second frame is DTS-HD then we have to bundle DTS + DTS-HD frames in a block from here on out
  if( !m_bIsDTSHD )
  {
    if( m_bFirstDTSFrame )
    {
      // Frame 1
      memcpy( (void*)m_DTSPrevFrame, (void*)pData, iDataSize );
      m_DTSPrevFrameLen = iDataSize;
      m_bFirstDTSFrame = false;
      return 0;
    }
    else if( m_DTSPrevFrameLen )
    {
      // Frame 2
      int sz = PaddDTSData( m_DTSPrevFrame, m_DTSPrevFrameLen, pOut );
      sz += PaddDTSData( pData, iDataSize, pOut + sz );
      m_DTSPrevFrameLen = 0;
      return sz;
    }
    else
    {
      // Frame 3...N
      return PaddDTSData( pData, iDataSize, pOut );
    }
  }

  // At this point we know we have a DTS-HD stream and we are bitstreaming
  if( !bIsDTSHDFrame )
  {
    if( m_DTSPrevFrameLen )
    {
      CLog::Log(LOGDEBUG, "Have a previous DTS frame and received another - looks like we're going to drop one, oops!");
    }
    else
    {
      memcpy( (void*)m_DTSPrevFrame, (void*)pData, iDataSize );
      m_DTSPrevFrameLen = iDataSize;
      return 0;
    }
  }
  else if( m_bHardwareBitStream )
  {
    // We only hit this after the first frame - we detect DTS-HD but have a buffered DTS frame.
    // be nice to the audio sink and package things up per hardware bitstream rules
    memcpy( pOut, m_DTSPrevFrame, m_DTSPrevFrameLen );
    memcpy( pOut+m_DTSPrevFrameLen, pData, iDataSize );

    idx = m_DTSPrevFrameLen + iDataSize;
    m_DTSPrevFrameLen = 0;

    return idx;
  }
  else
  {
    // Otherwise, prep the syncword and insert the frame size..
    const BYTE dtshdPreamble[] = {0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0xfe,0xfe};

    unsigned short frame_size = (unsigned short)iDataSize + m_DTSPrevFrameLen;
    unsigned short data_size = frame_size + 2 + sizeof(dtshdPreamble);

    pOut[0] = 0x72; pOut[1] = 0xf8; /* iec 61937     */
    pOut[2] = 0x1f; pOut[3] = 0x4e; /*  syncword     */

    pOut[4] = 0x11;  // type
    pOut[5] = 0x04;  // datatype info
    pOut[6] = data_size & 0xFF;  // size in bytes; includes the frame size, frame, preamble
    pOut[7] = (data_size >> 8) & 0xFF;

    idx = 8;

    // stuff in the dtsma preamble
    memcpy((char*)pOut+idx,(char*)dtshdPreamble, sizeof(dtshdPreamble));
    idx += sizeof(dtshdPreamble);

    // insert the frame length
    memcpy( (char*)pOut+idx, (char*)&frame_size, 2 );
    idx += 2;

    // copy the DTS frame
    swabdata( (char*)pOut+idx, (char*)m_DTSPrevFrame, m_DTSPrevFrameLen );
    idx += m_DTSPrevFrameLen;
    m_DTSPrevFrameLen = 0;

    // copy the DTS-HD frame
    swabdata((char*)pOut+idx, (char*)pData, iDataSize);
    idx += iDataSize;

    // zero the rest of the block
    memset( (char*)pOut+idx, 0, 32768 - idx );
    idx = 32768;
  }
  return idx;
}

int CDVDAudioCodecPassthrough::PaddAC3Data( BYTE* pData, int iDataSize, BYTE* pOut)
{
  /* we always output aligned sizes to allow for byteswapping*/
  int iDataSize2 = (iDataSize+1) & ~1;

  //Setup syncword and header
  pOut[0] = 0x72; pOut[1] = 0xF8;
  pOut[2] = 0x1F; pOut[3] = 0x4E;

  pOut[4] = 0x01; //(length) ? data_type : 0; /* & 0x1F; */
  pOut[5] = 0x00;
  pOut[6] = (iDataSize2 << 3) & 0xFF;
  pOut[7] = (iDataSize2 >> 5) & 0xFF;

  int iOutputSize = OUT_SAMPLESTOBYTES(m_iSamplesPerFrame);

  if ( iDataSize2 > iOutputSize - 8 ) 
  {          
    //Crap frame with more data than we can handle, can be worked around i think
    CLog::Log(LOGERROR, "CDVDAudioCodecPassthrough::PaddAC3Data - larger frame than will fit, skipping");
    return 0;
  }

  //Swap byteorder
  swabdata((char*)pOut+8, (char*)pData, iDataSize);
  memset(pOut + iDataSize2 + 8, 0, iOutputSize - iDataSize2 - 8);
  return iOutputSize;
}
int CDVDAudioCodecPassthrough::PaddTrueHDData( BYTE* pData, int iDataSize, BYTE* pOut)
{
  // we have to finagle the frames here for bitstreaming truehd.
  // basically each frame should be zero padded to take an appropriate amount of wire time
  // but to calculate the zero padding you need to know the elapsed time between frames
  // so this loop retains state from the previous frame to determine the elapsed time and how much
  // zero padding is required.

  // the resulting data stream is split into 61440 byte chunks
  // we lead with an 8 byte IEC syncword and frame size
  // after this we have the 20 byte truehd preamble
  // we then add audio frames, padding them as needed to fill out the individual frame size
  // at 30708+8 we have a truehd sync frame
  // we continue adding audio frames and padding
  // then we finish with the truehd footer
  // note that everything is byteswapped in except the IEC header

  // since the decode loop gives us a frame at a time we have some state between frames. this means the
  // code probably breaks with seeking (but that's ok, so does all of the syncword handling...)
  //  m_iSamplesPerFrame - the calculated wire samples the frame should take
  //  m_iLastFrameSize   - the actual byte count used by the previous frame - with the above calculates the padding
  //  m_iByteCounter     - where we are at in the larger iec frame so we can insert sync points properly
  // 
  const BYTE truehdIECheader[] = {0x72,0xf8,0x1f,0x4e,0x16,0x00,0xf0,0xef};  // 8 bytes
  const WORD truehdPreamble[] = {0x9E07,0x0300,0x0184,0x0101,0x0080,0xa556,0xf43b,0x8381,0x8049,0xe077}; // 20 bytes
  const WORD truehdMiddle[] = {0xc1c3,0x4942,0xfa3b,0x8382,0x8049,0xe077};  // 12 bytes
  const WORD truehdFooter[] = {0xc2c3,0xc4c0,0x0000,0x0000,0x0000,0x0000,0x0000,0x1197,0x0000,0x0000,0x0000,0x0000}; // 24 bytes

  int iFullFrameSize = 0;  // size of the frame including truehd data and IEC header, excluding zeros
  int iBytesToCopy = iDataSize;   // amount of data to copy
  int iZerosToFill = m_iSamplesPerFrame ? ((m_iSamplesPerFrame - m_iLastFrameSize) & 0xfff) : 0;
  int idx = 0;  // location in the dest buffer (iFullFrameSize + number of zeros copied)
  int sz = 0;   
  int dataidx = 0;  // offset in the source buffer

  // copy the frame. our output buffer is sufficiently large that we'll never run out
  while( iBytesToCopy )
  {
    // see if this is the start of an IEC frame
    if( 0 == m_iByteCounter )
    {
      // Insert iec
      sz = sizeof(truehdIECheader);
      memcpy( (char*)pOut+idx, (char*)truehdIECheader, sz );
      idx += sz;
      m_iByteCounter += sz;
      iFullFrameSize += sz;

      // Insert preamble
      sz = sizeof(truehdPreamble);
      swabdata( (char*)pOut+idx, (char*)truehdPreamble, sz );
      idx += sz;
      m_iByteCounter += sz;
      iFullFrameSize += sz;
    }
    else if( 30716 == m_iByteCounter )
    {
      // Insert mid
      sz = sizeof(truehdMiddle);
      swabdata( (char*)pOut+idx, (char*)truehdMiddle, sz );
      idx += sz;
      m_iByteCounter += sz;
      iFullFrameSize += sz;

      // Tricky. Cut the zero pad for the current frame down
      iZerosToFill = (iZerosToFill - sz > 0 ? iZerosToFill - sz : 0);
      //iFullFrameSize += sz;
    }
    else if( (61440-24) == m_iByteCounter )
    {
      // Insert footer
      sz = sizeof(truehdFooter);
      swabdata( (char*)pOut+idx, (char*)truehdFooter, sz );
      idx += sz;
      iFullFrameSize += sz;

      // Now, if we have any data left to copy we cycle immediately
      // This ensures the next header/syncword make it in
      m_iByteCounter = 0;
      if( iBytesToCopy || iZerosToFill )
      {
        continue;
      }
    }

    // Pad out the previous frame, careful around the middle and end
    if( iZerosToFill )
    {
      int this_fill = iZerosToFill;
      if( m_iByteCounter < 30716 && (m_iByteCounter + this_fill) > 30716 )
      {
        // the zero space is crossing the middle of the frame
        // this is tricky because we clip the middle bytes from the zero fill total
        this_fill = 30716 - m_iByteCounter;
      }
      else if( m_iByteCounter + this_fill > (61440-24) )
      {
        this_fill = (61440-24) - m_iByteCounter;
      }
      memset( (char*)pOut+idx, 0, this_fill );
      m_iByteCounter += this_fill;
      idx += this_fill;
      iZerosToFill -= this_fill;
    }

    // Copy the current frame data, careful around the middle and end
    // If we just zero-filled to an edge, the below must be a no-op (this_copy == 0)
    int this_copy = iBytesToCopy;
    if( m_iByteCounter <= 30716 && (m_iByteCounter + this_copy > 30716) )
    {
      this_copy = 30716 - m_iByteCounter;
    }
    else if( m_iByteCounter + this_copy > (61440-24) )
    {
      this_copy = (61440-24) - m_iByteCounter;
    }
    swabdata( (char*)pOut+idx, (char*)pData + dataidx, this_copy );
    m_iByteCounter += this_copy;
    idx += this_copy;
    dataidx += this_copy;
    iFullFrameSize += this_copy;
    iBytesToCopy -= this_copy;
  }

  m_iLastFrameSize = iFullFrameSize;
  return idx; // bytes copied
}

bool CDVDAudioCodecPassthrough::IsPassthroughAudioCodec( int Codec )
{
  bool bSupportsAC3Out = false;
  bool bSupportsEAC3Out = false;
  bool bSupportsDTSOut = false;
  bool bSupportsDTSHDOut = false;
  bool bSupportsTrueHDOut = false;

  if( g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL_HDMI ||
      g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL_SPDIF)
  {
    bSupportsAC3Out = g_guiSettings.GetBool("audiooutput.ac3passthrough");
    bSupportsDTSOut = g_guiSettings.GetBool("audiooutput.dtspassthrough");
  }
  if( g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL_HDMI )
  {
    bSupportsEAC3Out = g_guiSettings.GetBool("audiooutput.eac3passthrough");
    bSupportsTrueHDOut = g_guiSettings.GetBool("audiooutput.truehdpassthrough");
    bSupportsDTSHDOut = g_guiSettings.GetBool("audiooutput.dtshdpassthrough");
  }

  // NOTE - DTS-HD is not a codec setting and is always detected after the first two frames
  if ((Codec == CODEC_ID_AC3 && bSupportsAC3Out)
    || (Codec == CODEC_ID_EAC3 && bSupportsEAC3Out)
    || (Codec == CODEC_ID_DTS && bSupportsDTSOut)
    || (Codec == CODEC_ID_TRUEHD && bSupportsTrueHDOut))
  {
    return true;
  }

  return false;
}

bool CDVDAudioCodecPassthrough::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  bool bSupportsPassthrough = IsPassthroughAudioCodec( hints.codec );
  bool bSupportedSampleRate = (hints.samplerate == 0 || hints.samplerate == 48000);

  // See if we will support bitstreaming DTS-HD
  if( hints.codec == CODEC_ID_DTS )
  {
    m_bBitstreamDTSHD = g_guiSettings.GetBool("audiooutput.dtshdpassthrough") &&
                        g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL_HDMI;
  }

  // Caller has requested bitstream only; if we don't support bitstream for this format
  // exit now, otherwise we'll end up hardware decoding
  if( hints.bitstream && !bSupportsPassthrough )
  {
    return false;
  }
  

  if(m_bHardwareBitStream)
  {
    m_iChannels = hints.channels;
    m_Codec = hints.codec;
    m_iSourceSampleRate = hints.samplerate;
    return true;
  }


  if( bSupportsPassthrough )
  {
    // TODO - this is only valid for video files, and should be moved somewhere else
    if( hints.channels == 2 && g_stSettings.m_currentVideoSettings.m_OutputToAllSpeakers )
    {
      CLog::Log(LOGINFO, "CDVDAudioCodecPassthrough::Open - disabled passthrough due to video OTAS");
      return false;
    }

    // Not clear if this is really valid - we should check this against the EDID for HDMI TODO
    if( !bSupportedSampleRate )
    {
      CLog::Log(LOGINFO, "CDVDAudioCodecPassthrough::Open - disabled passthrough due to sample rate not being 48000");
      return false;
    }

    // trust the hints for now
    m_iChannels = hints.channels;
    m_iSourceSampleRate = hints.samplerate;

    m_Codec = hints.codec;
    m_iByteCounter = 0;

    // truehd content can have some ac3 frames mixed in
    if(m_Codec == CODEC_ID_AC3 || m_Codec == CODEC_ID_TRUEHD)
    {
      if (!m_dllA52.Load())
        return false;
      m_pStateA52 = m_dllA52.a52_init(0);
      if(m_pStateA52 == NULL)
        return false;

      if( m_Codec == CODEC_ID_AC3 )
        m_iSamplesPerFrame = 6*256;
    }
    else if(m_Codec == CODEC_ID_DTS)
    {
      if (!m_dllDTS.Load())
        return false;
      m_pStateDTS = m_dllDTS.dts_init(0);
      if(m_pStateDTS == NULL)
        return false;
    } 

    return true;    
  }
  else
    return false;
}

void CDVDAudioCodecPassthrough::Dispose()
{
  if( m_pStateA52 )
  {
    m_dllA52.a52_free(m_pStateA52);
    m_pStateA52 = NULL;
  }
  if( m_pStateDTS )
  {
    m_dllDTS.dts_free(m_pStateDTS);
    m_pStateDTS = NULL;
  }
}

#define MAKE_DWORD(x) ((unsigned int)((x)[0] << 24) | ((x)[1] << 16) | ((x)[2] << 8) | ((x)[3]))
int CDVDAudioCodecPassthrough::ParseTrueHDFrame(BYTE* buffer, int* sampleRate, int* bitRate)
{
  // manually process truehd sync frames
  // bytes 0-3  frame time/size
  //       4-7  syncword (0xba is truehd, 0xbb is mlp)
  //  for mlp
  //       8    bits per sample (upper nibble), ignored (lower nibble)
  //       9    sample rate (upper nibble), sample rate (lower nibble)
  //      10-11 first 11 bits ignored; lowest 5 are channels
  //  for truehd
  //       8    sample rate (upper nibble), ignored (lower nibble)
  //       9-11 ignored (first nibble), channel key 1 (5 bits), ignored (2 bits), channel key 2 (13 bits)
  //  for both
  //      12-17 ignored
  //      18-19 vbr indicator (top bit), peak bitrate (next 15; multiplied by samplerate)
  //      20    number of substreams in the stream (upper nibble), ignored (lower nibble)

  int frame_size = 0;
  unsigned int syncword = MAKE_DWORD( &buffer[4] );

  // First check if we have a syncword
  if( 0xf8726fba == syncword || 0xf8726fbb == syncword )
  {
    // TrueHD frame - calculate the frame time, frame size, bits/sample, etc
    unsigned int frame_time = MAKE_DWORD( &buffer[0] );
    unsigned int fs = ((frame_time >> 16 ) & 0xfff) * 2;  // this is how ffdshow does it - don't ask
    frame_time &= 0xffff;

    char sample_rate;
    if( (syncword & 0xff) == 0xbb )
    {
      char bits_per_sample = buffer[8] >> 4;
      if( bits_per_sample == 0 )
        bits_per_sample = 16;

      sample_rate = buffer[9] >> 4;
    }
    else
    {
      sample_rate = buffer[8] >> 4;
    }
    m_iLastSampleRateBit = sample_rate;

    *sampleRate = (sample_rate == 0x0f ? 0 : (sample_rate & 0x08 ? 44100 : 48000) << (sample_rate & 0x07));
    unsigned short bitrate = (buffer[18] << 8 | buffer[19]) & 0x7fff;
    *bitRate = (*sampleRate * bitrate + 8) >> 4;
    
    if( m_uiLastFrameTime )
    {
      m_iSamplesPerFrame = ((frame_time - m_uiLastFrameTime)&0xff) * (64 >> (sample_rate & 0x07));
    }
    m_uiLastFrameTime = frame_time;

    frame_size = (int) fs;
  }
  else
  {
    // see if this is standard ac3 which can be mixed in; if so liba52 should process
    unsigned short syncshort = (unsigned short) buffer[0] << 8 | buffer[1];
    if( 0x0b77 == syncshort )
    {
      int flags=0;
      frame_size = m_dllA52.a52_syncinfo( buffer, &flags, sampleRate, bitRate );
      if(m_iSourceFlags != flags)
      {
        m_iSourceFlags = flags;
        //CLog::Log(LOGDEBUG, "%s - source flags changed flags:%x sr:%d br:%d", __FUNCTION__, m_iSourceFlags, m_iSourceSampleRate, m_iSourceBitrate);
      }
    }
    else
    {
      // this is a non major frame. so we calculate the frame size and time for the pipeline,
      // and just pass these back.
      unsigned int frame_time = MAKE_DWORD( &buffer[0] );
      unsigned int fs = ((frame_time >> 16 ) & 0xfff) * 2;
      frame_time &= 0xffff;

      if( m_uiLastFrameTime )
      {
        m_iSamplesPerFrame = ((frame_time - m_uiLastFrameTime)&0xff) * (64 >> (m_iLastSampleRateBit & 0x07));
      }
      m_uiLastFrameTime = frame_time;

      frame_size = fs;
    }
  }
  return frame_size;
}

int CDVDAudioCodecPassthrough::ParseDTSHDFrame(BYTE* buffer, int* sampleRate, int* bitRate, int* samplesPerFrame )
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

    // we don't parse this from the dts-hd blocks, so just make sure we don't zero out existing vals
    *sampleRate = m_iSourceSampleRate;
    *bitRate = m_iSourceBitrate;
    *samplesPerFrame = m_iSamplesPerFrame;
  }

  return frame_size;
}

int CDVDAudioCodecPassthrough::ParseFrame(BYTE* data, int size, BYTE** frame, int* framesize, frametype* ft)
{
  int flags, len;
  BYTE* orig = data;

  *frame     = NULL;
  *framesize = 0;

  switch( m_Codec )
  {
  case CODEC_ID_AC3:
    *ft = AC3;
    break;
  case CODEC_ID_TRUEHD:
    *ft = TrueHD;
    break;
  case CODEC_ID_DTS:
    *ft = DTS;
    break;
  }

  if(m_InputSize == 0 && size > HEADER_SIZE)
  {
    // try to sync directly in packet
    if(m_Codec == CODEC_ID_AC3)
      m_iFrameSize = m_dllA52.a52_syncinfo(data, &flags, &m_iSourceSampleRate, &m_iSourceBitrate);
    else if(m_Codec == CODEC_ID_TRUEHD)
      m_iFrameSize = ParseTrueHDFrame( data, &m_iSourceSampleRate, &m_iSourceBitrate );
    else if(m_Codec == CODEC_ID_DTS)
    {
      // Try to sync on regular DTS
      m_iFrameSize = m_dllDTS.dts_syncinfo(m_pStateDTS, data, &flags, &m_iSourceSampleRate, &m_iSourceBitrate, &m_iSamplesPerFrame);

      // If that failed then look for the DTS-HD syncword
      if( !m_iFrameSize )
      {
        m_iFrameSize = ParseDTSHDFrame( data, &m_iSourceSampleRate, &m_iSourceBitrate, &m_iSamplesPerFrame );
        if( m_iFrameSize )
        {
          flags = m_iSourceFlags;
          *ft = DTS_HD;
          if( !m_bIsDTSHD )
          {
            m_bIsDTSHD = true;
            CLog::Log(LOGDEBUG, "Detected DTS-HD audio; bitstreaming %s", m_bBitstreamDTSHD ? "enabled" : "disabled");
          }
        }
      }
    }

    if(m_iFrameSize > 0)
    {
      if(m_iSourceFlags != flags)
      {
        m_iSourceFlags = flags;
        //CLog::Log(LOGDEBUG, "%s - source flags changed flags:%x sr:%d br:%d", __FUNCTION__, m_iSourceFlags, m_iSourceSampleRate, m_iSourceBitrate);
      }

      if(size >= m_iFrameSize)
      {
        *frame     = data;
        *framesize = m_iFrameSize;
        return m_iFrameSize;
      }
      else
      {
        if( size > (int) sizeof(m_InputBuffer) )
        {
          CLog::Log(LOGWARNING, "Passthrough codec breaking an audio frame of %d bytes\n", size);
          size = sizeof(m_InputBuffer);
        }
        m_InputSize = size;
        memcpy(m_InputBuffer, data, m_InputSize);
        return m_InputSize;
      }
    }
  }

  // attempt to fill up to 20 bytes (needed for truehd)
  if(m_InputSize < HEADER_SIZE) 
  {
    len = HEADER_SIZE-m_InputSize;
    if(len > size)
      len = size;
    memcpy(m_InputBuffer+m_InputSize, data, len);
    m_InputSize += len;
    data        += len;
    size        -= len;
  }

  if(m_InputSize < HEADER_SIZE) 
    return data - orig;

  // attempt to sync by shifting bytes
  while(true)
  {
    if(m_Codec == CODEC_ID_AC3)
      m_iFrameSize = m_dllA52.a52_syncinfo(m_InputBuffer, &flags, &m_iSourceSampleRate, &m_iSourceBitrate);
    else if(m_Codec == CODEC_ID_TRUEHD)
      m_iFrameSize = ParseTrueHDFrame( m_InputBuffer, &m_iSourceSampleRate, &m_iSourceBitrate );
    else if(m_Codec == CODEC_ID_DTS)
    {
      // Try to sync on regular DTS
      m_iFrameSize = m_dllDTS.dts_syncinfo(m_pStateDTS, m_InputBuffer, &flags, &m_iSourceSampleRate, &m_iSourceBitrate, &m_iSamplesPerFrame);

      // If that failed or if it's DTS-HD then look for the DTS-HD syncword
      if( !m_iFrameSize )
      {
        m_iFrameSize = ParseDTSHDFrame( m_InputBuffer, &m_iSourceSampleRate, &m_iSourceBitrate, &m_iSamplesPerFrame );
        if( m_iFrameSize )
        {
          flags = m_iSourceFlags;
          *ft = DTS_HD;
          if( !m_bIsDTSHD )
          {
            m_bIsDTSHD = true;
            CLog::Log(LOGDEBUG, "Detected DTS-HD audio; bitstreaming %s", m_bBitstreamDTSHD ? "enabled" : "disabled");
          }
        }
      }
    }
    if(m_iFrameSize > 0)
      break;

    if(size == 0)
      return data - orig;

    memmove(m_InputBuffer, m_InputBuffer+1, HEADER_SIZE-1);
    m_InputBuffer[HEADER_SIZE-1] = data[0];
    data++;
    size--;
  }

  if(m_iSourceFlags != flags)
  {
    m_iSourceFlags = flags;
    //CLog::Log(LOGDEBUG, "%s - source flags changed flags:%x sr:%d br:%d", __FUNCTION__, m_iSourceFlags, m_iSourceSampleRate, m_iSourceBitrate);
  }

  len = m_iFrameSize-m_InputSize;
  if(len > size)
    len = size;
  if( len + m_InputSize > (int) sizeof(m_InputBuffer) )
  {
    // We don't have room to buffer this frame, so drop, mark the packet consumed, and move on
    CLog::Log(LOGWARNING, "Passsthrough codec dropping a frame of size %d\n", len + m_InputSize);
    len += m_InputSize;
    m_InputSize = 0;
    *framesize = 0;
    *frame = NULL;
    return len;
  }
  else
  {
    memcpy(m_InputBuffer+m_InputSize, data, len);
    m_InputSize += len;
    data        += len;
    size        -= len;

    if(m_InputSize >= m_iFrameSize)
    {
      *frame     = m_InputBuffer;
      *framesize = m_iFrameSize;
      m_InputSize = 0;
    }
  }

  return data - orig;
}

int CDVDAudioCodecPassthrough::Decode(BYTE* pData, int iSize)
{  
  frametype ft;
  int len, framesize;
  BYTE* frame;

  if(m_bHardwareBitStream)
  {
    memcpy(m_OutputBuffer, pData, iSize);
    m_OutputSize = iSize;
    len = iSize;

    return len;
  }

  m_OutputSize = 0;
  len = ParseFrame(pData, iSize, &frame, &framesize, &ft);
  if(!frame)
    return len;

  if(m_Codec == CODEC_ID_AC3)
    m_OutputSize = PaddAC3Data(frame, framesize, m_OutputBuffer);
  else if(m_Codec == CODEC_ID_TRUEHD)
    m_OutputSize = PaddTrueHDData(frame, framesize, m_OutputBuffer);
  else if(m_Codec == CODEC_ID_DTS /*&& m_bIsDTSHD */)
    m_OutputSize = PaddDTSHDData(frame, framesize, ft == DTS_HD, m_OutputBuffer);

  // DTS bitstream has to be handled by the DTSHD layer

  return len;
}

int CDVDAudioCodecPassthrough::GetData(BYTE** dst)
{
  int size;
  if(m_OutputSize)
  {
    *dst = m_OutputBuffer;
    size = m_OutputSize;

    m_OutputSize = 0;
    return size;
  }
  else
    return 0;
}

void CDVDAudioCodecPassthrough::Reset()
{
  m_InputSize = 0;
  m_OutputSize = 0;
  m_Synced = false;
}

int CDVDAudioCodecPassthrough::GetChannels()
{
  // here's the dilemma. on some passthrough formats we need to lie and indicate 2 channels
  // for truehd though we have to lie and indicate 8
  // this could use some work

  return CODEC_ID_TRUEHD == m_Codec ? m_iChannels : OUT_CHANNELS;
}

enum PCMChannels* CDVDAudioCodecPassthrough::GetChannelMap()
{
  // We always present stereo ch mapping
  static enum PCMChannels map[2] = {PCM_FRONT_LEFT, PCM_FRONT_RIGHT };
  return map;
}

int CDVDAudioCodecPassthrough::GetSampleRate()
{
  return m_iSourceSampleRate;
}

int CDVDAudioCodecPassthrough::GetBitsPerSample()
{
  return OUT_SAMPLESIZE;
}

// See DVDPlayerAudio.h
unsigned char CDVDAudioCodecPassthrough::GetFlags()
{
  unsigned char flags = 0;
  if( m_bHardwareDecode ) flags |= FFLAG_HWDECODE;
  else                    flags |= FFLAG_PASSTHROUGH;

  if( CODEC_ID_TRUEHD == m_Codec || (m_bIsDTSHD && m_bBitstreamDTSHD) )
    flags |= FFLAG_HDFORMAT;

  return flags; 
}

#endif
