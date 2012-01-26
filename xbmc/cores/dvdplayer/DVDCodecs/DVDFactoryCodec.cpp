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
 
#include "utils/log.h"
#ifndef WIN32
#include <config.h>
#endif
#include "DVDFactoryCodec.h"
#include "Video/DVDVideoCodec.h"
#include "Audio/DVDAudioCodec.h"
#include "Overlay/DVDOverlayCodec.h"

#include "Video/DVDVideoCodecFFmpeg.h"
#ifdef HAS_DVD_LIBMPEG2_CODEC
#include "Video/DVDVideoCodecLibMpeg2.h"
#endif
#ifdef HAS_OPENMAX
#include "Video/DVDVideoCodecOmx.h"
#endif
#ifdef HAS_DX
#include "Video/DVDVideoCodecDXVA.h"
#endif
#ifdef CANMORE
#include "Video/DVDVideoCodecSMD.h"
#endif
#ifdef __APPLE__
#include "Video/DVDVideoCodecVDA.h"
#endif
#include "Audio/DVDAudioCodecFFmpeg.h"
#ifdef HAS_DVD_LIBA52_CODEC
#include "Audio/DVDAudioCodecLiba52.h"
#endif
#ifdef HAS_DVD_LIBDTS_CODEC
#include "Audio/DVDAudioCodecLibDts.h"
#endif
#include "Audio/DVDAudioCodecLibMad.h"
#include "Audio/DVDAudioCodecLibFaad.h"
#include "Audio/DVDAudioCodecPcm.h"
#include "Audio/DVDAudioCodecLPcm.h"
#if defined HAS_DVD_LIBA52_CODEC && defined HAS_DVD_LIBDTS_CODEC
#include "Audio/DVDAudioCodecPassthrough.h"
#endif
#if defined HAS_INTEL_SMD
#include "Audio/DVDAudioCodecSMD.h"
#endif
#include "Overlay/DVDOverlayCodecSSA.h"
#include "Overlay/DVDOverlayCodecText.h"
#include "Overlay/DVDOverlayCodecTX3G.h"
#include "Overlay/DVDOverlayCodecFFmpeg.h"

#include "DVDStreamInfo.h"
#include "GUISettings.h"

#include "LicenseConfig.h"

CDVDVideoCodec* CDVDFactoryCodec::OpenCodec(CDVDVideoCodec* pCodec, CDVDStreamInfo &hints, CDVDCodecOptions &options )
{  
  try
  {
    CLog::Log(LOGDEBUG, "FactoryCodec - Video: %s - Opening", pCodec->GetName());
    if( pCodec->Open( hints, options ) )
    {
      CLog::Log(LOGDEBUG, "FactoryCodec - Video: %s - Opened", pCodec->GetName());
      return pCodec;
    }

    CLog::Log(LOGDEBUG, "FactoryCodec - Video: %s - Failed", pCodec->GetName());
    pCodec->Dispose();
    delete pCodec;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "FactoryCodec - Video: Failed with exception");
  }
  return NULL;
}

CDVDAudioCodec* CDVDFactoryCodec::OpenCodec(CDVDAudioCodec* pCodec, CDVDStreamInfo &hints, CDVDCodecOptions &options )
{    
  try
  {
    CLog::Log(LOGDEBUG, "FactoryCodec - Audio: %s - Opening", pCodec->GetName());
    if( pCodec->Open( hints, options ) )
    {
      CLog::Log(LOGDEBUG, "FactoryCodec - Audio: %s - Opened", pCodec->GetName());
      return pCodec;
    }

    CLog::Log(LOGDEBUG, "FactoryCodec - Audio: %s - Failed", pCodec->GetName());
    pCodec->Dispose();
    delete pCodec;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "FactoryCodec - Audio: Failed with exception");
  }
  return NULL;
}

CDVDOverlayCodec* CDVDFactoryCodec::OpenCodec(CDVDOverlayCodec* pCodec, CDVDStreamInfo &hints, CDVDCodecOptions &options )
{  
  try
  {
    CLog::Log(LOGDEBUG, "FactoryCodec - Overlay: %s - Opening", pCodec->GetName());
    if( pCodec->Open( hints, options ) )
    {
      CLog::Log(LOGDEBUG, "FactoryCodec - Overlay: %s - Opened", pCodec->GetName());
      return pCodec;
    }

    CLog::Log(LOGDEBUG, "FactoryCodec - Overlay: %s - Failed", pCodec->GetName());
    pCodec->Dispose();
    delete pCodec;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "FactoryCodec - Audio: Failed with exception");
  }
  return NULL;
}


CDVDVideoCodec* CDVDFactoryCodec::CreateVideoCodec( CDVDStreamInfo &hint )
{
  CDVDVideoCodec* pCodec = NULL;
  CDVDCodecOptions options;
  CDVDCodecOptions dvdOptions;

  // try to decide if we want to try halfres decoding
#if !defined(_LINUX) && !defined(_WIN32)
  float pixelrate = (float)hint.width*hint.height*hint.fpsrate/hint.fpsscale;
  if( pixelrate > 1400.0f*720.0f*30.0f )
  {
    CLog::Log(LOGINFO, "CDVDFactoryCodec - High video resolution detected %dx%d, trying half resolution decoding ", hint.width, hint.height);    
    options.push_back(CDVDCodecOption("lowres","1"));    
  }
  else 
#endif

#ifdef HAS_INTEL_SMD
    // for DVD use libmpeg2
//    if(hint.width == 720 && (hint.height == 480 || hint.height == 576))
//      if( hint.codec == CODEC_ID_MPEG2VIDEO && (pCodec = OpenCodec(new CDVDVideoCodecLibMpeg2(), hint, dvdOptions)) )
//        return pCodec;

    if( !hint.software && (pCodec = OpenCodec(new CDVDVideoCodecSMD(), hint, dvdOptions)) )
      return pCodec;
#endif

  { // non halfres mode, we can use other decoders
    if (hint.codec == CODEC_ID_MPEG2VIDEO || hint.codec == CODEC_ID_MPEG1VIDEO)
    {
      CDVDCodecOptions dvdOptions;

#ifdef HAVE_LIBVDPAU
      if (hint.height >= 720)
      {
        CLog::Log(LOGNOTICE,"Trying VDPAU-MPEG from FFMPEG");
        if( (pCodec = OpenCodec(new CDVDVideoCodecFFmpeg(), hint, dvdOptions)) ) return pCodec;
      }
#endif

#ifdef HAS_DVD_LIBMPEG2_CODEC
      if( hint.codec == CODEC_ID_MPEG1VIDEO && (pCodec = OpenCodec(new CDVDVideoCodecLibMpeg2(), hint, dvdOptions)) ) return pCodec;

      if( hint.codec == CODEC_ID_MPEG2VIDEO && (pCodec = OpenCodec(new CDVDVideoCodecLibMpeg2(), hint, dvdOptions)) ) return pCodec;
#endif
    }
  }

#ifdef HAS_OPENMAX
  if( !hint.software && (pCodec = OpenCodec(new CDVDVideoCodecOmx(), hint, dvdOptions)) ) return pCodec;
#endif


   // on win32 try using DXVA first
#ifdef HAS_DX_
  if(!hint.software && g_guiSettings.GetBool("videoplayer.hwaccel"))
    if( (pCodec = OpenCodec(new CDVDVideoCodecDXVA(), hint, dvdOptions)) )
      return pCodec;
#endif

#if defined(__APPLE__)
  if (!hint.software && g_guiSettings.GetBool("videoplayer.hwaccel") && hint.codec == CODEC_ID_H264)
  {
    CLog::Log(LOGINFO, "Trying Apple VDA Decoder...");
    if ( (pCodec = OpenCodec(new CDVDVideoCodecVDA(), hint, options)) ) return pCodec;
  }
#endif

  if( (pCodec = OpenCodec(new CDVDVideoCodecFFmpeg(), hint, dvdOptions)) ) return pCodec;

  return NULL;
}

bool isDolbyAudioCodec(CodecID codec)
{
  switch(codec) {
          case CODEC_ID_AC3:
          case CODEC_ID_EAC3:
          case CODEC_ID_TRUEHD:
            return true;
          default:
            return false;
  }
}

bool isDTSAudioCodec(CodecID codec)
{
  if(CODEC_ID_DTS == codec)
    return true;
  else
     return false;
}

CDVDAudioCodec* CDVDFactoryCodec::CreateAudioCodec( CDVDStreamInfo &hint )
{
  CDVDAudioCodec* pCodec = NULL;
  CDVDCodecOptions options;

#if defined(HAS_INTEL_SMD)
  if( !hint.software )
    pCodec = OpenCodec( new CDVDAudioCodecSMD(), hint, options );
#elif defined HAS_DVD_LIBA52_CODEC && defined HAS_DVD_LIBDTS_CODEC
  if( !hint.software )
    pCodec = OpenCodec( new CDVDAudioCodecPassthrough(), hint, options );
#endif
  if( pCodec )
    return pCodec;

  // if this failed, we fail here
  if( hint.bitstream )
  {
    CLog::Log(LOGERROR, "CDVDFactoryCodec::CreateAudioCodec failed since hint is bitstream");
    return NULL;
  }
  
#ifdef HAS_EMBEDDED
  if (isDolbyAudioCodec(hint.codec)
      && !(g_lic_settings.is_dolby_sw_decode_allowed()))
  {
    return NULL;
  }
  if (isDTSAudioCodec(hint.codec)
      && !(g_lic_settings.is_dts_sw_decode_allowed()))
  {
    return NULL;
  }
#endif

  switch (hint.codec)
  {
#ifdef HAS_DVD_LIBA52_CODEC
  case CODEC_ID_AC3:
    {
      pCodec = OpenCodec( new CDVDAudioCodecLiba52(), hint, options );
      if( pCodec ) return pCodec;
      break;
    }
#endif
#ifdef HAS_DVD_LIBDTS_CODEC
  case CODEC_ID_DTS:
    {
      pCodec = OpenCodec( new CDVDAudioCodecLibDts(), hint, options );
      if( pCodec ) return pCodec;
      break;
    }
#endif
  case CODEC_ID_MP2:
  case CODEC_ID_MP3:
    {
      pCodec = OpenCodec( new CDVDAudioCodecLibMad(), hint, options );
      if( pCodec ) return pCodec;
      break;
    }
  case CODEC_ID_AAC:
  //case CODEC_ID_MPEG4AAC:
    {
      pCodec = OpenCodec( new CDVDAudioCodecLibFaad(), hint, options );
      if( pCodec ) return pCodec;
      break;
    }
  case CODEC_ID_PCM_S32LE:
  case CODEC_ID_PCM_S32BE:
  case CODEC_ID_PCM_U32LE:
  case CODEC_ID_PCM_U32BE:
  case CODEC_ID_PCM_S24LE:
  case CODEC_ID_PCM_S24BE:
  case CODEC_ID_PCM_U24LE:
  case CODEC_ID_PCM_U24BE:
  case CODEC_ID_PCM_S24DAUD:
  case CODEC_ID_PCM_S16LE:
  case CODEC_ID_PCM_S16BE:
  case CODEC_ID_PCM_U16LE:
  case CODEC_ID_PCM_U16BE:
  case CODEC_ID_PCM_S8:
  case CODEC_ID_PCM_U8:
  case CODEC_ID_PCM_ALAW:
  case CODEC_ID_PCM_MULAW:
    {
      pCodec = OpenCodec( new CDVDAudioCodecPcm(), hint, options );
      if( pCodec ) return pCodec;
      break;
    }
#if 0
  //case CODEC_ID_LPCM_S16BE:
  //case CODEC_ID_LPCM_S20BE:
  case CODEC_ID_LPCM_S24BE:
    {
      pCodec = OpenCodec( new CDVDAudioCodecLPcm(), hint, options );
      if( pCodec ) return pCodec;
      break;
    }
#endif
  default:
    {
      pCodec = NULL;
      break;
    }
  }

  pCodec = OpenCodec( new CDVDAudioCodecFFmpeg(), hint, options );
  if( pCodec ) return pCodec;

  return NULL;
}

CDVDOverlayCodec* CDVDFactoryCodec::CreateOverlayCodec( CDVDStreamInfo &hint )
{
  CDVDOverlayCodec* pCodec = NULL;
  CDVDCodecOptions options;

  switch (hint.codec)
  {
    case CODEC_ID_TEXT:
      pCodec = OpenCodec(new CDVDOverlayCodecText(), hint, options);
      if( pCodec ) return pCodec;

    case CODEC_ID_SSA:
      pCodec = OpenCodec(new CDVDOverlayCodecSSA(), hint, options);
      if( pCodec ) return pCodec;

      pCodec = OpenCodec(new CDVDOverlayCodecText(), hint, options);
      if( pCodec ) return pCodec;

    case CODEC_ID_MOV_TEXT:
      pCodec = OpenCodec(new CDVDOverlayCodecTX3G(), hint, options);
      if( pCodec ) return pCodec;

    default:
      pCodec = OpenCodec(new CDVDOverlayCodecFFmpeg(), hint, options);
      if( pCodec ) return pCodec;
  }

  return NULL;
}
