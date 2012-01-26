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
#include "MusicInfoTagLoaderFactory.h"
#include "MusicInfoTagLoaderMP3.h"
#include "MusicInfoTagLoaderOgg.h"
#include "MusicInfoTagLoaderWMA.h"
#include "MusicInfoTagLoaderFlac.h"
#include "MusicInfoTagLoaderMP4.h"
#include "MusicInfoTagLoaderCDDA.h"
#ifdef HAS_APE_CODEC
#include "MusicInfoTagLoaderApe.h"
#endif
#include "MusicInfoTagLoaderMPC.h"
#include "MusicInfoTagLoaderShn.h"
#ifdef HAS_SID_CODEC
#include "MusicInfoTagLoaderSid.h"
#endif
#include "MusicInfoTagLoaderMod.h"
#include "MusicInfoTagLoaderWav.h"
#include "MusicInfoTagLoaderAAC.h"
#ifdef HAS_WAVPACK_CODEC
#include "MusicInfoTagLoaderWavPack.h"
#endif
#ifdef HAS_MOD_PLAYER
#include "cores/ModPlayer.h"
#endif
#ifdef HAS_NSF_CODEC
#include "MusicInfoTagLoaderNSF.h"
#endif
#ifdef HAS_SPC_CODEC
#include "MusicInfoTagLoaderSPC.h"
#endif
#ifdef HAS_GYM_CODEC
#include "MusicInfoTagLoaderGYM.h"
#endif
#include "MusicInfoTagLoaderAdplug.h"
#ifdef HAS_YM_CODEC
#include "MusicInfoTagLoaderYM.h"
#endif
#include "MusicInfoTagLoaderDatabase.h"
#include "MusicInfoTagLoaderASAP.h"
#ifdef HAS_TIMIDITY_CODEC
#include "MusicInfoTagLoaderMidi.h"
#endif
#include "MusicInfoTagLoaderUPnP.h"

#include "Util.h"
#include "FileItem.h"
#include "log.h"
#include "UPnPDirectory.h"

using namespace MUSIC_INFO;

CMusicInfoTagLoaderFactory::CMusicInfoTagLoaderFactory()
{}

CMusicInfoTagLoaderFactory::~CMusicInfoTagLoaderFactory()
{}

IMusicInfoTagLoader* CMusicInfoTagLoaderFactory::CreateLoader(const CStdString& strFileName)
{
  // dont try to read the tags for streams & shoutcast
  CFileItem item(strFileName, false);
  if (item.IsInternetStream())
    return NULL;

  if (item.IsMusicDb())
    return new CMusicInfoTagLoaderDatabase();

  CStdString strExtension;
#ifndef HAS_UPNP_AV
  if (strFileName.Left(7) == "upnp://")
  {
    DIRECTORY::CUPnPDirectory dir;
    CFileItem item;
    CURI urlfile(strFileName);
    if (dir.GetResource(urlfile,item))
    {
      CUtil::GetExtension( item.m_strPath, strExtension);
    }
  }
  else
#endif
  {
    CUtil::GetExtension( strFileName, strExtension);
  }


  strExtension.ToLower();
  strExtension.TrimLeft('.');

  CLog::Log(LOGDEBUG,"CMusicInfoTagLoaderFactory::CreateLoader, loading id3tag for path: [%s] , extension: [%s] (tlf)", strFileName.c_str(), strExtension.c_str());

  if (strExtension.IsEmpty())
    return NULL;

  if (strExtension == "mp3")
  {
    CMusicInfoTagLoaderMP3 *pTagLoader = new CMusicInfoTagLoaderMP3();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "ogg" || strExtension == "oggstream")
  {
    CMusicInfoTagLoaderOgg *pTagLoader = new CMusicInfoTagLoaderOgg();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "wma")
  {
    CMusicInfoTagLoaderWMA *pTagLoader = new CMusicInfoTagLoaderWMA();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "flac")
  {
    CMusicInfoTagLoaderFlac *pTagLoader = new CMusicInfoTagLoaderFlac();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "m4a" || strExtension == "mp4")
  {
    CMusicInfoTagLoaderMP4 *pTagLoader = new CMusicInfoTagLoaderMP4();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#ifdef HAS_DVD_DRIVE  
  else if (strExtension == "cdda")
  {
    CMusicInfoTagLoaderCDDA *pTagLoader = new CMusicInfoTagLoaderCDDA();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#endif
#ifdef HAS_APE_CODEC
  else if (strExtension == "ape" || strExtension == "mac")
  {
    CMusicInfoTagLoaderApe *pTagLoader = new CMusicInfoTagLoaderApe();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#endif
  else if (strExtension == "mpc" || strExtension == "mpp" || strExtension == "mp+")
  {
    CMusicInfoTagLoaderMPC *pTagLoader = new CMusicInfoTagLoaderMPC();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "shn")
  {
    CMusicInfoTagLoaderSHN *pTagLoader = new CMusicInfoTagLoaderSHN();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#ifdef HAS_SID_CODEC
  else if (strExtension == "sid" || strExtension == "sidstream")
  {
    CMusicInfoTagLoaderSid *pTagLoader = new CMusicInfoTagLoaderSid();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#endif
#ifdef HAS_MOD_PLAYER
  else if (ModPlayer::IsSupportedFormat(strExtension) || strExtension == "mod" || strExtension == "it" || strExtension == "s3m")
  {
    CMusicInfoTagLoaderMod *pTagLoader = new CMusicInfoTagLoaderMod();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#endif
  else if (strExtension == "wav")
  {
    CMusicInfoTagLoaderWAV *pTagLoader = new CMusicInfoTagLoaderWAV();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "aac")
  {
    CMusicInfoTagLoaderAAC *pTagLoader = new CMusicInfoTagLoaderAAC();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#ifdef HAS_WAVPACK_CODEC
  else if (strExtension == "wv")
  {
    CMusicInfoTagLoaderWAVPack *pTagLoader = new CMusicInfoTagLoaderWAVPack();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#endif
#ifdef HAS_NSF_CODEC
  else if (strExtension == "nsf" || strExtension == "nsfstream")
  {
    CMusicInfoTagLoaderNSF *pTagLoader = new CMusicInfoTagLoaderNSF();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#endif
#ifdef HAS_SPC_CODEC
  else if (strExtension == "spc")
  {
    CMusicInfoTagLoaderSPC *pTagLoader = new CMusicInfoTagLoaderSPC();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#endif
#ifdef HAS_GYM_CODEC
  else if (strExtension == "gym")
  {
    CMusicInfoTagLoaderGYM *pTagLoader = new CMusicInfoTagLoaderGYM();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#endif
#ifdef HAS_YM_CODEC
  else if (strExtension == "ym")
  {
    CMusicInfoTagLoaderYM *pTagLoader = new CMusicInfoTagLoaderYM();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#endif
  else if (AdplugCodec::IsSupportedFormat(strExtension))
  {
    CMusicInfoTagLoaderAdplug *pTagLoader = new CMusicInfoTagLoaderAdplug();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (ASAPCodec::IsSupportedFormat(strExtension) || strExtension == "asapstream")
  {
    CMusicInfoTagLoaderASAP *pTagLoader = new CMusicInfoTagLoaderASAP();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#ifdef HAS_TIMIDITY_CODEC
  else if ( TimidityCodec::IsSupportedFormat( strExtension ) )
  {
    CMusicInfoTagLoaderMidi * pTagLoader = new CMusicInfoTagLoaderMidi();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#endif

  return NULL;
}
