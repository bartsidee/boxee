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

#ifdef HAS_DVD_DRIVE

#include "CDDADirectory.h"
#include "FileItem.h"
#include "FileSystem/File.h"
#include "MediaManager.h"
#include "MusicInfoLoader.h"
#include "ImusicInfoTagLoader.h"
#include "MusicInfoTagLoaderFactory.h"
#include "MusicInfoTag.h"
#include "Util.h"

using namespace XFILE;
using namespace DIRECTORY;
using namespace MEDIA_DETECT;


CCDDADirectory::CCDDADirectory(void)
{
}

CCDDADirectory::~CCDDADirectory(void)
{
}


bool CCDDADirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  // Reads the tracks from an audio cd

  if (!g_mediaManager.IsDiscInDrive(strPath))
    return false;

  // Get information for the inserted disc
  CCdInfo* pCdInfo = g_mediaManager.GetCdInfo(strPath);
  if (pCdInfo == NULL)
    return false;

  // If the disc has no tracks, we are finished here.
  int nTracks = pCdInfo->GetTrackCount();
  if (nTracks <= 0)
    return false;

  // Generate fileitems
  for (int i = 1;i <= nTracks;++i)
  {
    // Skip Datatracks for display,
    // but needed to query cddb
    if (!pCdInfo->IsAudio(i))
      continue;

    CFileItemPtr pItem( new CFileItem() );

    pItem->m_bIsFolder = false;
    pItem->m_strPath.Format("cdda://local/%02.2i.cdda", i);

    // load tag from file
    std::auto_ptr< MUSIC_INFO::IMusicInfoTagLoader > pLoader( 
      MUSIC_INFO::CMusicInfoTagLoaderFactory::CreateLoader( pItem->m_strPath ) );

    if( pLoader.get() )
      if( pLoader->Load( pItem->m_strPath, *( pItem->GetMusicInfoTag() ) ) )
      {
        const MUSIC_INFO::CMusicInfoTag* pTag = pItem->GetMusicInfoTag();
        CStdString strLabel( pTag->GetTitle() );

        pItem->SetLabel( strLabel );
      }

    struct __stat64 s64;

    if (CFile::Stat(pItem->m_strPath, &s64) == 0)
      pItem->m_dwSize = s64.st_size;

    items.Add(pItem);
  }

  // update global data to list
  CFileItemPtr pItem = items[ 0 ];
  const MUSIC_INFO::CMusicInfoTag* pTag = pItem->GetMusicInfoTag();

  items.SetLabel( pTag->GetAlbum() );
  items.GetMusicInfoTag()->SetYear( pTag->GetYear() );
  items.GetMusicInfoTag()->SetArtist( pTag->GetArtist() );
  items.GetMusicInfoTag()->SetGenre( pTag->GetGenre() );

  return true;
}

#endif
