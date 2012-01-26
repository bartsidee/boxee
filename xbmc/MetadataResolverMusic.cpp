

#include "MetadataResolverMusic.h"

#include "FileSystem/Directory.h"
#include "FileSystem/BoxeeDatabaseDirectory.h"
#include "BoxeeUtils.h"
#include "Util.h"
#include "utils/MusicInfoScraper.h"
#include "MusicInfoLoader.h"
#include "ImusicInfoTagLoader.h"
#include "PictureInfoLoader.h"
#include "MusicInfoTag.h"
#include "MusicAlbumInfo.h"
#include "MusicInfoTagLoaderMP3.h"
#include "SpecialProtocol.h"
#include "MusicInfoTagLoaderFactory.h"
#include "cores/dvdplayer/DVDFileInfo.h"
#include "Picture.h"

#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "lib/libBoxee/bxutils.h"
#include "lib/libBoxee/bxmetadataengine.h"
#include "AdvancedSettings.h"

#include "Application.h"

using namespace MUSIC_INFO;

// Defines number of songs that are sufficient to be defines as album
#define ALBUM_TRESHOLD 1

// cache is not enable at this point
std::vector<CResolvedAlbum> CMetadataResolverMusic::albumCache;

bool CMetadataResolverMusic::m_bInitialized = false;
bool CMetadataResolverMusic::m_bStopped = false;

void CResolvingTrack::LoadTags()
{
  //CLog::Log(LOGERROR, "CMetadataResolverMusic::ResolveMusicFolder, loading tags for %s (musicresolver)", strPath.c_str());
  CStdString strResolvedPath = _P(strPath);

  CFileItemPtr pTempItem(new CFileItem(strResolvedPath, false));

  bool bResult = false;
  std::auto_ptr<IMusicInfoTagLoader> pLoader (CMusicInfoTagLoaderFactory::CreateLoader(pTempItem->m_strPath));
  if (NULL != pLoader.get())
  {
    // load tags
    bResult = pLoader->Load(pTempItem->m_strPath, *pTempItem->GetMusicInfoTag());
  }

  if (bResult)
  {
    strTitle = pTempItem->GetMusicInfoTag()->GetTitle();
    if (strTitle == "")
    {
      // use filename instead
      strTitle = CUtil::GetFileName(strPath);
    }

    int _iTrackNumber = pTempItem->GetMusicInfoTag()->GetTrackNumber();
    iTrackNumber = _iTrackNumber > 0 ? _iTrackNumber : 0;

    strAlbum = pTempItem->GetMusicInfoTag()->GetAlbum();
    strAlbumArtist = pTempItem->GetMusicInfoTag()->GetAlbumArtist();
    strArtist = pTempItem->GetMusicInfoTag()->GetArtist();
    strGenre = pTempItem->GetMusicInfoTag()->GetGenre();
    iYear = pTempItem->GetMusicInfoTag()->GetYear();
    iDuration = pTempItem->GetMusicInfoTag()->GetDuration();

    //CDVDFileInfo::GetFileMetaData(strResolvedPath, pTempItem.get());
    //iDuration = pTempItem->GetPropertyInt("duration-msec") / 1000;

    //pTempItem->Dump();
  }
  else
  {
     CLog::Log(LOGDEBUG, "CResolvingTrack::LoadTags, could not load tag for path = %s (musicresolver)", strPath.c_str());
  }
}

CResolvedAlbum::CResolvedAlbum(const CStdString& _strName, const CStdString& _strArtist, const CStdString& _strFolderPath, const CStdString& _strGenre, int _iYear)
{
  strName = _strName;
  strArtist = _strArtist;
  strFolderPath = _strFolderPath;
  strGenre = _strGenre;
  iYear = _iYear;

  bResolved = false;

  // TODO: calculate effective folder name
}

CMetadataResolverMusic::CMetadataResolverMusic()
{
  if (!m_bInitialized)
  {
    CMetadataResolverMusic::InitializeAudioResolver();
  }

  m_bStopped = false;
}

CMetadataResolverMusic::~CMetadataResolverMusic()
{

}

void CMetadataResolverMusic::InitializeAudioResolver()
{
  if (m_bInitialized)
    return;

  m_bInitialized = true;

}

int CMetadataResolverMusic::ResolveMusicFolder(const CStdString& strPath)
{

  CResolvingFolder folder;

  // Read folder contents
  bool status = ReadMusicFolder(strPath, folder);

  if (!status)
  {
    CLog::Log(LOGERROR, "CMetadataResolverMusic::ResolveMusicFolder, could not read music folder  %s (musicresolver)", strPath.c_str()); 
    return RESOLVER_ABORTED;
  }

  SynchronizeFolderAndDatabase(folder);

  std::vector<CResolvedAlbum> albums;
  CreateAlbumsFromFolder(folder, albums);

  // Go over resolved albums
  for (size_t i = 0; i < albums.size() && !m_bStopped; i++)
  {
    CResolvedAlbum& album = albums[i];
    CLog::Log(LOGDEBUG, "CMetadataResolver::ResolveMusicFolder, resolved album, path: %s, name: %s, artist %s, num tracks: %d (musicresolver)",
        album.strFolderPath.c_str(), album.strName.c_str(), album.strArtist.c_str(), (int)album.vecTracks.size());

    if (album.vecTracks.size() >= ALBUM_TRESHOLD)
    {
      AddAlbumToDatabase(album);
    }
  }

  // We always mark folder resolved at this point
  BOXEE::Boxee::GetInstance().GetMetadataEngine().MarkFolderResolved(folder.strFolderPath, -1);

  return RESOLVER_SUCCESS;
}

bool CMetadataResolverMusic::ReadMusicFolder(const CStdString& strPath, CResolvingFolder& folder)
{
  //  if (!g_application.IsPathAvailable(strPath, false))
  //  {
  //    return false;
  //  }

  // We wont try to resolve RAR or ZIP folder - in this case the files will stay unresolved
  if (CUtil::IsRAR(strPath) || CUtil::IsZIP(strPath))
  {
	CLog::Log(LOGDEBUG,"CMetadataResolverMusic::ReadMusicFolder  file %s is compressed file - keep it unresolved", strPath.c_str());
	return true;
  }

  CFileItemList items;
  if (!DIRECTORY::CDirectory::GetDirectory(strPath, items)) 
  {
    return false;
  }

  for (int i=0; i < items.Size(); i++)
  {
    CFileItemPtr pItem = items[i];

    if (pItem->IsAudio() && !pItem->IsHidden() && !pItem->IsRAR() && !pItem->IsZIP() && !pItem->IsPlayList(false)) 
    {
      CResolvingTrack track(pItem->m_strPath);
      track.strFolderPath = strPath;
      folder.vecTracks.push_back(track);
    }
    else if (pItem->IsPlayList(false))
    {
      // TODO: Handle playlists

    }
    else if (pItem->IsPicture())
    {
      CStdString extension = CUtil::GetExtension(pItem->m_strPath);

      CStdString supportedExtensions = ".png|.jpg|.jpeg|.bmp|.gif";

      CStdStringArray thumbs;
      StringUtils::SplitString(supportedExtensions, "|", thumbs);
      for (unsigned int j = 0; j < thumbs.size(); ++j)
      {
        if (extension.CompareNoCase(thumbs[j]) == 0)
        {
          folder.vecThumbs.push_back(pItem->m_strPath);
          break;
        }
      }
    }
  }

  folder.strFolderPath = strPath;
  folder.strEffectiveFolderName = BOXEE::BXUtils::GetEffectiveFolderName(strPath);

  return true;
}


bool CMetadataResolverMusic::CreateAlbumsFromFolder(const CResolvingFolder& folder, std::vector<CResolvedAlbum>& albums)
{
  for (size_t i = 0; i < folder.vecTracks.size(); i++)
  {
    if (g_application.m_bStop)
      return false;
    
    CResolvingTrack track = folder.vecTracks[i];

    // Read metadata tags of the track
    track.LoadTags();

    if (track.strAlbum == "")
    {
      track.strAlbum = "Unknown Album";
    }

    if (track.strArtist == "")
    {
      track.strArtist = "Unknown Artist";
    }

    if (track.strTitle == "")
    {
      track.strTitle = CUtil::GetFileName(track.strPath);
    }

    // Check if this album already exists in the vector
    int index = FindAlbumInVector(albums, track.strAlbum, track.strArtist);
    if (index == -1)
    {
      // Create new resolving album
      CResolvedAlbum album(track.strAlbum, track.strArtist, track.strFolderPath, track.strGenre, track.iYear);

      // Check if local thumb exists for this album
      FindLocalThumbnail(album, folder);

      album.vecTracks.push_back(track);
      albums.push_back(album);
    }
    else
    {
      // we found existing album
      CResolvedAlbum& album = albums[index];
      album.vecTracks.push_back(track);
    }
  }

  return true;
}

bool CMetadataResolverMusic::AddAlbumToDatabase(CResolvedAlbum& album)
{
  // TODO: Check the cache for the album to avoid unnecessary database access

  CLog::Log(LOGDEBUG,"CMetadataResolverMusic::AddAlbumToDatabase, Adding album: %s (musicresolver)", album.strName.c_str());

  // Add artist
  BOXEE::BXArtist bxArtist;
  bxArtist.m_strName = album.strArtist;

  if (!BOXEE::BXUtils::GetArtistData(album.strArtist, &bxArtist))
  {
    CLog::Log(LOGERROR,"CMetadataResolverMusic::AddAlbumToDatabase, Could not load artist data from network. %s (musicresolver)", album.strArtist.c_str());
    //return false;
  }

  int iArtistId = BOXEE::Boxee::GetInstance().GetMetadataEngine().AddArtist(&bxArtist);
  if (iArtistId == MEDIA_DATABASE_ERROR) 
  {
    CLog::Log(LOGERROR,"CMetadataResolverMusic::AddAlbumToDatabase, Could not add artist to database. %s (musicresolver)", album.strArtist.c_str());
    return false;
  }

  album.iArtistDbIndex = iArtistId;

  BOXEE::BXAlbum bxAlbum;
  bxAlbum.m_strTitle = album.strName;
  bxAlbum.m_strArtist = album.strArtist;
  bxAlbum.m_strArtwork = album.strCover;
  bxAlbum.m_iArtistId = iArtistId;
  bxAlbum.SetPath(_P(album.strFolderPath)); //  set the path of the album folder
  bxAlbum.m_iVirtual = 0; // set to 0 to indicate that this is a local (not virtual) album
  bxAlbum.m_strGenre = album.strGenre;
  bxAlbum.m_iYear = album.iYear;

  int iAlbumId = BOXEE::Boxee::GetInstance().GetMetadataEngine().AddAlbum(&bxAlbum);
  if (iAlbumId == MEDIA_DATABASE_ERROR) 
  {
    CLog::Log(LOGERROR,"Could not add album to database. %s", bxAlbum.m_strTitle.c_str());
    return false;
  }

  album.iAlbumDbIndex = iAlbumId;

  // Cache album thumbnail using album database path, same path is used during album retrieval 
  if (album.strCover != "")
  {
    CPicture::CreateThumbnail(album.strCover, CUtil::GetCachedAlbumThumb(album.strName, album.strArtist), true);
  }

  // Add all tracks
  for (size_t i = 0; i < album.vecTracks.size(); i++) 
  {
    const CResolvingTrack& track = album.vecTracks[i];
    BOXEE::BXAudio bxAudio;
    bxAudio.m_strPath = track.strPath;
    bxAudio.m_strTitle = track.strTitle;
    bxAudio.m_strAlbum = track.strAlbum;
    bxAudio.m_iTrackNumber = track.iTrackNumber;
    bxAudio.m_iDuration = track.iDuration;

    bxAudio.m_iArtistId = iArtistId;
    bxAudio.m_iAlbumId = iAlbumId;

    BOXEE::Boxee::GetInstance().GetMetadataEngine().AddAudio(&bxAudio);
    BOXEE::Boxee::GetInstance().GetMetadataEngine().UpdateAudioFileStatus(track.strPath, STATUS_RESOLVED);

  }

  return true;

}

bool CMetadataResolverMusic::FindLocalThumbnail(CResolvedAlbum& album, const CResolvingFolder& folder)
{
  // Go over all files in the folder that could be an album thumb (basically, picture files)
  for (int i = 0; i < (int)folder.vecThumbs.size(); i++)
  {
    CStdString strThumbFileName = CUtil::GetFileName(folder.vecThumbs[i]);

    // Get all possible thumbnail file names from the settings
    CStdStringArray thumbs;
    StringUtils::SplitString(g_advancedSettings.m_musicThumbs, "|", thumbs);
    for (unsigned int j = 0; j < thumbs.size(); ++j)
    {

      if (strThumbFileName.CompareNoCase(thumbs[j]) == 0)
      {
        album.strCover = folder.vecThumbs[i];
        return true;
      }
    }

    CUtil::RemoveExtension(strThumbFileName);

    // Handle the case that folder name is the same as the jpeg name
    if (strThumbFileName == folder.strEffectiveFolderName) 
    {
      album.strCover = folder.vecThumbs[i];
      return true;
    }

    if (strThumbFileName == album.strName)
    {
      album.strCover = folder.vecThumbs[i];
      return true;
    }

    // TODO: Add more cases
  }

  return false;
}

bool CMetadataResolverMusic::Stop()
{
  CLog::Log(LOGINFO, "Boxee Metadata Resolver Music, stopping...");
  CMetadataResolverMusic::m_bStopped = true;
  return m_bStopped;
}

void CMetadataResolverMusic::ResolveAlbumMetadata(std::vector<BOXEE::BXAlbum*>& albums)
{
  for (size_t i = 0; i < albums.size() && !m_bStopped; i++)
  {
    BOXEE::BXAlbum* album = albums[i];

    CStdString strAlbum = album->m_strTitle;
    CStdString strArtist = album->m_strArtist;

    BOXEE::BXMetadata metadata(MEDIA_ITEM_TYPE_AUDIO);
    if (ResolveAlbumMetadata(strAlbum, strArtist, &metadata))
    {

      BOXEE::BXAlbum* pUpdatedAlbum = (BOXEE::BXAlbum*)metadata.GetDetail(MEDIA_DETAIL_ALBUM);
      pUpdatedAlbum->m_iId = album->m_iId;
      pUpdatedAlbum->m_iStatus = STATUS_RESOLVED;
      BOXEE::Boxee::GetInstance().GetMetadataEngine().UpdateAlbum(pUpdatedAlbum);
    }
    else
    {
      BOXEE::Boxee::GetInstance().GetMetadataEngine().UpdateAlbumStatus(album->m_iId, STATUS_UNRESOLVED);
    }
  }
}

bool CMetadataResolverMusic::ResolveAlbumMetadata(const CStdString& strAlbum, const CStdString& strArtist, BOXEE::BXMetadata * pMetadata)
{
  if (!pMetadata || pMetadata->GetType() != MEDIA_ITEM_TYPE_AUDIO)
  {
    CLog::Log(LOGERROR, "CMetadataResolverMusic::ResolveAlbumMetadata, Will not resolve album metadata NULL or invalid type (musicresolver)");
    return false;
  }

  if (strAlbum.IsEmpty() || strAlbum == "unknown album" || strArtist.IsEmpty() || strArtist == "unknown artist")
  {
    CLog::Log(LOGERROR, "CMetadataResolverMusic::ResolveAlbumMetadata, Will not resolve album = %s, artist = %s (musicresolver)", strAlbum.c_str(), strArtist.c_str());
    return false;
  }

  CLog::Log(LOGDEBUG, "CMetadataResolverMusic::ResolveAlbumMetadata, Resolving album = %s, artist = %s (musicresolver)", strAlbum.c_str(), strArtist.c_str());

  MUSIC_GRABBER::CMusicAlbumInfo albumInfo;
  bool bResolved = GetMetadataFromAMG(strAlbum, strArtist, albumInfo);

  if (bResolved)
  {
    CLog::Log(LOGDEBUG, "CMetadataResolverMusic::ResolveAlbumMetadata, Resolved album = %s, artist = %s, description = %s (musicresolver)",
        strAlbum.c_str(), strArtist.c_str(), albumInfo.GetAlbum().strReview.c_str());
    BoxeeUtils::ConvertAlbumInfoToBXMetadata(albumInfo, pMetadata);
    pMetadata->m_bResolved = true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "CMetadataResolverMusic::ResolveAlbumMetadata, Could not resolve album = %s, artist = %s (musicresolver)", strAlbum.c_str(), strArtist.c_str());
  }

  return bResolved;
}

bool CMetadataResolverMusic::GetMetadataFromAMG(const CStdString& _strAlbum, const CStdString& _strArtist,MUSIC_GRABBER::CMusicAlbumInfo& album)
{
  CStdString strAlbum = _strAlbum;
  strAlbum = strAlbum.ToLower();

  CStdString strArtist = _strArtist;
  strArtist = strArtist.ToLower();

  // This function can resolve by album only, so we do not care about the artist at this point
  if (strAlbum.IsEmpty() || strArtist.IsEmpty()) 
    return false;

  SScraperInfo info;
  info.strPath = "allmusic.xml";
  info.strContent = "albums";

  int iRetries = 2;

  while (iRetries-- > 0) 
  {
    MUSIC_GRABBER::CMusicInfoScraper scraper(info);

    scraper.SetAlbum(strAlbum);
    scraper.SetArtist(strArtist);

    scraper.FindAlbuminfo();
    scraper.LoadAlbuminfo();

    int iSelectedAlbum = 0;

        while (!scraper.Completed() && !scraper.IsCanceled())
        {
          if (m_bStopped)
          {
            if (!scraper.IsCanceled())
            {
              //CLog::Log(LOGDEBUG, "LIBRARY: Boxee Metadata Resolver, Canceling scraper");
              scraper.Cancel();
              break;
            }
          }
          Sleep(200);
        }

    if (scraper.Successfull() && scraper.GetAlbumCount() > 0)
    {
      // did we found at least 1 album?
      int iAlbumCount = scraper.GetAlbumCount();
      if (iAlbumCount >= 1)
      {
        if (iAlbumCount > 1)
        {
          int bestMatch = -1;

          double minRelevance;

          minRelevance = 0.7;

          double bestRelevance = 0;
          double secondBestRelevance = 0;

          for (int i = 0; i < iAlbumCount; ++i)
          {
            MUSIC_GRABBER::CMusicAlbumInfo& info = scraper.GetAlbum(i);

            CStdString  strFoundAlbum = info.GetAlbum().strAlbum; 
            strFoundAlbum.MakeLower();
            CStdString  strFoundArtist = info.GetAlbum().strArtist; 
            strFoundArtist.MakeLower();

            bool bFoundSoundtrack = false;


            if ((strFoundAlbum.Find("soundtrack") != -1) || (strFoundArtist.Find("soundtrack") != -1))
            {
              bFoundSoundtrack = true;
              // reset found artist in order to avoid artist comparison
              strFoundArtist = "";

              //strFoundAlbum = CleanAlbumName(strFoundAlbum);

              // check if we can remove the "[original soundtrack]" token
              int pos = strFoundAlbum.Find("[original soundtrack]");
              if (pos != -1) 
              {
                strFoundAlbum.erase(pos, 21);
              }
              else
              {
                // try to remove words separately
                int pos1 = strFoundAlbum.Find("soundtrack");
                if (pos1 != -1)
                {
                  strFoundAlbum.erase(pos1, 10);
                }

                int pos2 = strFoundAlbum.Find("original");
                if (pos2 != -1) 
                {
                  strFoundAlbum.erase(pos2, 8);
                }
              }
            }


            // TODO: Check if originally we were looking for a soundtrack
            // Check if we need a sound track
            //            if (pContext->bIsSoundtrack && !bFoundSoundtrack)
            //            {
            //              continue;
            //            }

            double relevance = CUtil::AlbumRelevance(strFoundAlbum, strAlbum, strFoundArtist, strArtist);

            // if we're doing auto-selection (ie querying all albums at once, then allow 95->100% for perfect matches)
            // otherwise, perfect matches only
            if (relevance >= std::max(minRelevance, bestRelevance))
            { // we auto-select the best of these
              secondBestRelevance = bestRelevance;
              bestRelevance = relevance;
              bestMatch = i;
            }
          }


          if (bestMatch > -1 && bestRelevance != secondBestRelevance)
          { // autochoose the single best matching item
            iSelectedAlbum = bestMatch;
          }
          else
          { //  nothing found, or two equal matches to choose from
            // perform additional checks

            return false;
          }

        }
      }
      else {
        return false;
      }

      MUSIC_GRABBER::CMusicAlbumInfo info = scraper.GetAlbum(iSelectedAlbum);

      // Save album and artist names as they get erased in the process
      CStdString strTempAlbumName = info.GetAlbum().strAlbum;
      CStdString strTempArtistName = info.GetAlbum().strArtist;

      scraper.LoadAlbuminfo(iSelectedAlbum);
      while (!scraper.Completed() && !scraper.IsCanceled())
      {
        //        if (m_bStopped)
        //        {
        //          if (!scraper.IsCanceled()) {
        //            scraper.Cancel();
        //          }
        //        }
        Sleep(200);
      }

      if (!scraper.IsCanceled() && scraper.Successfull()) 
      {
        album = scraper.GetAlbum(iSelectedAlbum);

        // Restore erased fields
        album.GetAlbum().strAlbum = strTempAlbumName;
        album.GetAlbum().strArtist = strTempArtistName;  

        return true;
      }
      else {
        if (scraper.HadNetworkProblems()) {
          Sleep(500);
          continue;
        }
        return false;
      }
    }
    else {
      if (scraper.HadNetworkProblems()) {
        Sleep(500);
        continue;

      }
      return false;
    }
  } // while retries

  return false;
}

int CMetadataResolverMusic::FindAlbumInVector(const std::vector<CResolvedAlbum> &albums, const CStdString& _strAlbum, const CStdString& _strArtist)
{
  int index = -1;

  CStdString strAlbum1 = _strAlbum; strAlbum1.Trim(); strAlbum1 = strAlbum1.ToLower();
  //CStdString strArtist1 = _strArtist; strArtist1.Trim(); strArtist1 = strArtist1.ToLower();

  for (int i = 0; i < (int)albums.size(); i++)
  {
    CStdString strAlbum2 = albums[i].strName; strAlbum2.Trim(); strAlbum2 = strAlbum2.ToLower();
    //CStdString strArtist2 = albums[i].strArtist; strArtist2.Trim(); strArtist2 = strArtist2.ToLower();

    if ((strAlbum1 == strAlbum2))
      index = i;
  }
  return index;
}

void CMetadataResolverMusic::SynchronizeFolderAndDatabase(CResolvingFolder& folder)
{
  // Get the list of resolved tracks under this folder from the database
  BOXEE::BXMetadataEngine& MDE = BOXEE::Boxee::GetInstance().GetMetadataEngine();
  std::map<std::string, BOXEE::BXMetadata*> mapResolvedTracks;
  MDE.GetAudiosByFolder(mapResolvedTracks, folder.strFolderPath.c_str());

  // Go over all tracks and check which of them are already resolved
  std::vector<CResolvingTrack>::iterator it = folder.vecTracks.begin();
  while (it != folder.vecTracks.end())
  {
    std::map<std::string, BOXEE::BXMetadata*>::iterator it2 = mapResolvedTracks.find(it->strPath);
    if (it2 != mapResolvedTracks.end()) 
    {
      delete it2->second;
      mapResolvedTracks.erase(it2);
      it = folder.vecTracks.erase(it);
    }
    else
    {
      ++it;
    }
  }

  // The files that still remain in the resolved tracks map are no longer on the disk 
  // and should be removed from the database
  CleanDeletedAudiosFromDatabase(mapResolvedTracks);

  // Clear remaining objects in the map
  std::map<std::string, BOXEE::BXMetadata*>::const_iterator it3 = mapResolvedTracks.begin();
  for (;it3 != mapResolvedTracks.end(); it3++) {
    delete it3->second;
  }
  mapResolvedTracks.clear();
}

void CMetadataResolverMusic::CleanDeletedAudiosFromDatabase(const std::map<std::string, BOXEE::BXMetadata*> &mapDeletedAudios)
{
  std::map<int, int> mapAlbums;

  std::map<std::string, BOXEE::BXMetadata*>::const_iterator it = mapDeletedAudios.begin();
  for (;it != mapDeletedAudios.end(); it++) {
    BOXEE::Boxee::GetInstance().GetMetadataEngine().RemoveAudioByPath(it->first);
    BOXEE::Boxee::GetInstance().GetMetadataEngine().RemoveUnresolvedAudioByPath(it->first);
  }
}

/*
CStdString CMetadataResolverMusic::CleanAlbumName(const CStdString& strName)
{
  static const char *SEPARATORS = ",.-_/\\{}() ";

  // first thing we do is find where the [] words start.
  // we ignore the rest of the line from the first [] word. (huristic)
  CStdString strCopy = strName;

  // Remove everything before the rectangular bracket
  int idx = strName.Find('[');
  if (idx >= 3) // sanity
    strCopy = strName.Mid(0,idx);
  else if (idx >= 0) // found [] in the begining
  {
    // find the first word which is not in [] and remove everything before it
    CRegExp reg;
    reg.RegComp(CStdString("][") + SEPARATORS + CStdString("]*[^[]"));
    int nPos = reg.RegFind(strName);
    if ( nPos > 0 )
      strCopy = strName.Mid(nPos);
  }


  // Do the same for the bracket
  idx = strName.Find('(');
  if (idx >= 3 ) // sanity
    strCopy = strName.Mid(0,idx);
  else if (idx >= 0) // found [] in the begining
  {
    // find the first word which is not in [] and remove everything before it
    CRegExp reg;
    reg.RegComp(CStdString(")(") + SEPARATORS + CStdString(")*(^()"));
    int nPos = reg.RegFind(strName);
    if ( nPos > 0 )
      strCopy = strName.Mid(nPos);
  }

  vector<std::string> vecWords = BOXEE::BXUtils::StringTokenize(strCopy, SEPARATORS);

  vector<std::string> vecGoodWords = RemoveBadWords(vecWords);

  CStdString strNewName = "";
  // Rebuild the name again
  for (unsigned int i = 0; i < vecGoodWords.size(); i++) {
    strNewName += vecGoodWords[i];
    strNewName.Trim();
    strNewName += " ";
  }

  return strNewName;
}
 */
unsigned int CMetadataResolverMusic::GetLevenshteinDistance(const CStdString& s1, const CStdString& s2)
{
  const size_t len1 = s1.size(), len2 = s2.size();
  std::vector<std::vector<unsigned int> > d(len1 + 1, std::vector<unsigned int>(len2 + 1));

  for(size_t i = 1; i <= len1; ++i) d[i][0] = i;
  for(size_t i = 1; i <= len2; ++i) d[0][i] = i;

  for(size_t i = 1; i <= len1; ++i)
    for(size_t j = 1; j <= len2; ++j)

      d[i][j] = std::min( std::min(d[i - 1][j] + 1,d[i][j - 1] + 1),
          d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1) );
  return d[len1][len2];
}
