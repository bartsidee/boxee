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
#include "config.h"
#endif
#include "system.h"
#include "FileItem.h"
#include "XBAudioConfig.h"
#include "CodecFactory.h"
#include "MP3codec.h"
#ifdef HAS_APE_CODEC
#include "APEcodec.h"
#endif
#include "CDDAcodec.h"
#include "OGGcodec.h"
#include "SHNcodec.h"
#include "FLACcodec.h"
#include "WAVcodec.h"
#ifdef HAS_WAVPACK_CODEC
#include "WAVPackcodec.h"
#endif
#include "ModuleCodec.h"
#ifdef HAS_NSF_CODEC
#include "NSFCodec.h"
#endif
#include "AC3Codec.h"
#include "AC3CDDACodec.h"
#ifdef HAS_SPC_CODEC
#include "SPCCodec.h"
#endif
#ifdef HAS_GYM_CODEC
#include "GYMCodec.h"
#endif
#ifdef HAS_SID_CODEC
#include "SIDCodec.h"
#endif
#include "AdplugCodec.h"
#ifdef HAS_VGMSTREAM_CODEC
#include "VGMCodec.h"
#endif
#ifdef HAS_YM_CODEC
#include "YMCodec.h"
#endif
#include "AIFFcodec.h"
#ifdef HAS_ADPCM_CODEC
#include "ADPCMCodec.h"
#endif
#ifdef HAS_TIMIDITY_CODEC
#include "TimidityCodec.h"
#endif
#include "ASAPCodec.h"
#include "URL.h"
#include "DVDPlayerCodec.h"
#ifdef HAS_DTS_CODEC
#include "DTSCodec.h"
#include "DTSCDDACodec.h"
#endif
#include "BXAcodec.h"

#include "Util.h"

ICodec* CodecFactory::CreateCodec(const CStdString& strFileType)
{
  if (strFileType.Equals("mp3") || strFileType.Equals("mp2"))
    return new MP3Codec();
#ifdef HAS_APE_CODEC
  else if (strFileType.Equals("ape") || strFileType.Equals("mac"))
    return new APECodec();
#endif
  else if (strFileType.Equals("cdda"))
    return new CDDACodec();
  else if (strFileType.Equals("mpc") || strFileType.Equals("mp+") || strFileType.Equals("mpp"))
    return new DVDPlayerCodec();
  else if (strFileType.Equals("shn"))
#ifdef _XBOX
    return new SHNCodec();
#else
    return new DVDPlayerCodec();
#endif
  else if (strFileType.Equals("flac"))
    return new FLACCodec();
  else if (strFileType.Equals("wav"))
    return new DVDPlayerCodec();
  else if (strFileType.Equals("m4b"))
    return new DVDPlayerCodec();
  else if (strFileType.Equals("dts"))
#if defined(HAS_DTS_CODEC) && !defined(HAS_INTEL_SMD)
    return new DTSCodec();
#else
    return new DVDPlayerCodec();
#endif
  else if (strFileType.Equals("ac3"))
#if defined(HAS_AC3_CODEC) && !defined(HAS_INTEL_SMD_DD_DECODER)
    return new AC3Codec();
#else
    return new DVDPlayerCodec();
#endif
  else if (strFileType.Equals("ec3") || strFileType.Equals("eac3"))
    return new DVDPlayerCodec();
  else if (strFileType.Equals("hbra"))
    return new DVDPlayerCodec();
  else if (strFileType.Equals("m4a") || strFileType.Equals("aac"))
    return new DVDPlayerCodec();
#ifdef HAS_WAVPACK_CODEC
  else if (strFileType.Equals("wv"))
    return new WAVPackCodec();
#endif
  else if (ModuleCodec::IsSupportedFormat(strFileType))
    return new ModuleCodec();
#ifdef HAS_NSF_CODEC
  else if (strFileType.Equals("nsf") || strFileType.Equals("nsfstream"))
    return new NSFCodec();
#endif
#ifdef HAS_SPC_CODEC
  else if (strFileType.Equals("spc"))
    return new SPCCodec();
#endif
#ifdef HAS_GYM_CODEC
  else if (strFileType.Equals("gym"))
    return new GYMCodec();
#endif
#ifdef HAS_SID_CODEC
  else if (strFileType.Equals("sid") || strFileType.Equals("sidstream"))
    return new SIDCodec();
#endif
  else if (AdplugCodec::IsSupportedFormat(strFileType))
    return new AdplugCodec();
#ifdef HAS_VGMSTREAM_CODEC
  else if (VGMCodec::IsSupportedFormat(strFileType))
    return new VGMCodec();
#endif
#ifdef HAS_YM_CODEC
  else if (strFileType.Equals("ym"))
    return new YMCodec();
#endif
  else if (strFileType.Equals("wma"))
    return new DVDPlayerCodec();
  else if (strFileType.Equals("aiff") || strFileType.Equals("aif"))
    return new DVDPlayerCodec();
#ifdef HAS_ADPCM_CODEC
  else if (strFileType.Equals("xwav"))
    return new ADPCMCodec();
#endif
#ifdef HAS_TIMIDITY_CODEC
  else if (TimidityCodec::IsSupportedFormat(strFileType))
    return new TimidityCodec();
#endif
  else if (ASAPCodec::IsSupportedFormat(strFileType) || strFileType.Equals("asapstream"))
    return new ASAPCodec();

  return NULL;
}

ICodec* CodecFactory::CreateCodecDemux(const CStdString& strFile, const CStdString& strContent, unsigned int filecache, bool &initialized)
{
  CStdString strExt;
  CUtil::GetExtension(strFile,strExt);
  initialized = false;
  strExt.ToLower();
  strExt.TrimLeft('.');

  CURI urlFile(strFile);
  if (strContent.Equals("audio/x-boxee-pcm"))
    return (ICodec*)new BXACodec();
  if( strContent.Equals("audio/mpeg")
    ||  strContent.Equals("audio/mp3") )
  {
    return new MP3Codec();
  }
  else if( strContent.Equals("audio/aac") 
    || strContent.Equals("audio/aacp") )
  {
      DVDPlayerCodec *pCodec = new DVDPlayerCodec;
    if (urlFile.GetProtocol() == "shout" )
      pCodec->SetContentType(strContent);
      return pCodec; 
    }
  else if( strContent.Equals("audio/x-ms-wma") || urlFile.GetProtocol() == "mms" )
    return new DVDPlayerCodec();

  if (urlFile.GetProtocol() == "lastfm" || urlFile.GetProtocol() == "shout")
  {
    return new DVDPlayerCodec(); // if we got this far with internet radio - content-type was wrong. let dvdplayer decide.
  }

  if( urlFile.GetProtocol() == "upnp" )
  {
    return new DVDPlayerCodec();  // probe for upnp content, since we rarely have extensions and may not have content type
  }

  if (urlFile.GetFileType().Equals("wav"))
  {
    ICodec* codec;
#ifdef HAS_DTS_CODEC
    //lets see what it contains...
    //this kinda sucks 'cause if it's a plain wav file the file
    //will be opened, sniffed and closed 2 times before it is opened *again* for wav
    //would be better if the papcodecs could work with bitstreams instead of filenames.
    codec = new DTSCodec();
    if (codec->Init(strFile, filecache))
    {
      return codec;
    }
    delete codec;
#endif
#ifdef HAS_AC3_CODEC
    codec = new AC3Codec();
    if (codec->Init(strFile, filecache))
    {
      return codec;
    }
    delete codec;
#endif
#ifdef HAS_ADPCM_CODEC
    codec = new ADPCMCodec();
    if (codec->Init(strFile, filecache))
    {
      return codec;
    }
    delete codec;
#endif
    
    codec = new DVDPlayerCodec();
    if (codec->Init(strFile, filecache))
    {
      return codec;
    }
    delete codec;
  }
  if (urlFile.GetFileType().Equals("cdda"))
  {
#ifdef HAS_DTS_CDDA_CODEC
    //lets see what it contains...
    //this kinda sucks 'cause if it's plain cdda the file
    //will be opened, sniffed and closed 2 times before it is opened *again* for cdda
    //would be better if the papcodecs could work with bitstreams instead of filenames.
    ICodec* codec = new DTSCDDACodec();
    if (codec->Init(strFile, filecache))
    {
      return codec;
    }
    delete codec;
#endif
#ifdef HAS_AC3_CDDA_CODEC
    codec = new AC3CDDACodec();
    if (codec->Init(strFile, filecache))
    {
      return codec;
    }
    delete codec;
#endif
  }
  else if (urlFile.GetFileType().Equals("ogg") || urlFile.GetFileType().Equals("oggstream") || urlFile.GetFileType().Equals("oga"))
  {
    // oldnemesis: we want to use OGGCodec() for OGG music since unlike DVDCodec it provides better
    //  timings for Karaoke. However OGGCodec() cannot handle ogg-flac and ogg videos, that's why this block.
    ICodec* codec = new OGGCodec();
    try
    {
      if (codec->Init(strFile, filecache))
      {
        delete codec; // class can't be inited twice - deinit doesn't properly deinit some members.
        return new OGGCodec;
    }
    }
    catch( ... )
    {
    }
    delete codec;
    return new DVDPlayerCodec();
  }
  //default
  return CreateCodec(urlFile.GetFileType());
}
