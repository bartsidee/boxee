

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

#include "lib/libBoxee/bxconfiguration.h"
#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "lib/libBoxee/bxutils.h"
#include "lib/libBoxee/bxmetadataengine.h"
#include "lib/libBoxee/bxmetadata.h"
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
  CLog::Log(LOGDEBUG, "CResolvingTrack::LoadTags, trying to load tags for %s (musicresolver)", strPath.c_str());
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
    CLog::Log(LOGDEBUG, "CResolvingTrack::LoadTags, resolved audio file path: [%s], album:[%s], album-artist:[%s], artist:[%s], genre:[%s], iYear:[%d], iDuration:[%d], iTrackNumber:[%d] (musicresolver)",
              strPath.c_str(), strAlbum.c_str(), strAlbumArtist.c_str(), strArtist.c_str(), strGenre.c_str(), iYear, iDuration, iTrackNumber);
  }
  else
  {
    CLog::Log(LOGWARNING, "CResolvingTrack::LoadTags, could not load tag for path = %s (musicresolver)", strPath.c_str());
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

int CMetadataResolverMusic::ResolveMusicFolder(BXMetadataScannerJob* pJob, bool rescan /*=false*/)
{
  CStdString strPath = pJob->GetFolder()->GetPath();
  if (strPath == "")
  {
    CLog::Log(LOGERROR, "CMetadataResolver::ResolveMusicFolder, DESIGN ERROR, should not attempt empty path (musicresolver)");
    return RESOLVER_FAILED;
  }

  CResolvingFolder folder;

  // Read folder contents
  int result = ReadMusicFolder(strPath, folder, pJob);

  if (result != RESOLVER_SUCCESS)
  {
    CLog::Log(LOGERROR, "CMetadataResolverMusic::ResolveMusicFolder, could not read music folder  %s (musicresolver)", strPath.c_str()); 
    return result;
  }

  if (rescan)
  {
    // When rescanning we should first remove previous instance of an album in order to avoid duplicates
    if (BOXEE::Boxee::GetInstance().GetMetadataEngine().RemoveAudioByFolder(strPath)!= MEDIA_DATABASE_OK)
    {
      CLog::Log(LOGERROR, "CMetadataResolverMusic::ResolveMusicFolder, failed to remove a previous instance of an album from path %s (musicresolver)", strPath.c_str());
      return RESOLVER_FAILED;
    }
  }
  else
  {
    SynchronizeFolderAndDatabase(folder);
  }

  std::vector<CResolvedAlbum> albums;
  result = CreateAlbumsFromFolder(folder, albums, pJob);

  if (result != RESOLVER_SUCCESS)
  {
    return result;
  }

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

int CMetadataResolverMusic::ReadMusicFolder(const CStdString& strPath, CResolvingFolder& folder, BXMetadataScannerJob* pJob)
{
  CLog::Log(LOGDEBUG,"CMetadataResolverMusic::ReadMusicFolder  reading music folder %s (musicresolver)", strPath.c_str());

  // Update folder path
  folder.strFolderPath = strPath;

  // We wont try to resolve RAR or ZIP folder - in this case the files will stay unresolved
  if (CUtil::IsRAR(strPath) || CUtil::IsZIP(strPath))
  {
	  CLog::Log(LOGDEBUG,"CMetadataResolverMusic::ReadMusicFolder  file %s is compressed file - keep it unresolved (musicresolver)", strPath.c_str());
	  return RESOLVER_SUCCESS;
  }

  CFileItemList items;
  if (!DIRECTORY::CDirectory::GetDirectory(strPath, items)) 
  {
    CLog::Log(LOGDEBUG,"CMetadataResolverMusic::ReadMusicFolder  unable to read directory %s (musicresolver)", strPath.c_str());
    return RESOLVER_FAILED;
  }

  for (int i=0; i < items.Size() && pJob->IsActive(); i++)
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

  if (!pJob->IsActive())
  {
    CLog::Log(LOGDEBUG,"CMetadataResolverMusic::ReadMusicFolder  aborted, resolver was paused when reading folder %s (pause) (musicresolver) ", strPath.c_str());
    return RESOLVER_ABORTED;
  }

  folder.strFolderPath = strPath;
  folder.strEffectiveFolderName = BOXEE::BXUtils::GetEffectiveFolderName(strPath);

  CLog::Log(LOGDEBUG,"CMetadataResolverMusic::ReadMusicFolder  successfully finished reading folder %s (musicresolver) ", strPath.c_str());
  return RESOLVER_SUCCESS;
}


int CMetadataResolverMusic::CreateAlbumsFromFolder(const CResolvingFolder& folder, std::vector<CResolvedAlbum>& albums, BXMetadataScannerJob* pJob)
{
  CLog::Log(LOGDEBUG,"CMetadataResolverMusic::CreateAlbumsFromFolder  creating albums from folder %s (musicresolver) ", folder.strFolderPath.c_str());
  for (size_t i = 0; i < folder.vecTracks.size() && pJob->IsActive(); i++)
  {
    if (g_application.m_bStop)
    {
      return RESOLVER_ABORTED;
    }
    
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
    int index = FindAlbumInVector(albums, track.strAlbum, track.strAlbumArtist.IsEmpty()?track.strArtist:track.strAlbumArtist);
    if (index == -1)
    {
      // Create new resolving album
      CResolvedAlbum album(track.strAlbum, track.strAlbumArtist.IsEmpty()?track.strArtist:track.strAlbumArtist, track.strFolderPath, track.strGenre, track.iYear);

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

  if (!pJob->IsActive())
  {
    CLog::Log(LOGDEBUG,"CMetadataResolverMusic::CreateAlbumsFromFolder  aborted, resolver was paused when resolving %s (pause) (musicresolver)", folder.strFolderPath.c_str());
    return RESOLVER_ABORTED;
}

  CLog::Log(LOGDEBUG,"CMetadataResolverMusic::CreateAlbumsFromFolder  successfully created albums from folder %s (musicresolver) ", folder.strFolderPath.c_str());
  return RESOLVER_SUCCESS;
}

bool CMetadataResolverMusic::AddAlbumToDatabase(CResolvedAlbum& album)
{
  // TODO: Check the cache for the album to avoid unnecessary database access

  CLog::Log(LOGDEBUG,"CMetadataResolverMusic::AddAlbumToDatabase, Adding album: %s (musicresolver)", album.strName.c_str());

  // Add artist or album artist
  BOXEE::BXArtist bxArtist;
  bxArtist.m_strName = album.strArtist;

  if (!BOXEE::BXUtils::GetArtistData(album.strArtist, &bxArtist))
  {
    CLog::Log(LOGWARNING,"CMetadataResolverMusic::AddAlbumToDatabase, Could not load artist data from network. %s (musicresolver)", album.strArtist.c_str());
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


  BOXEE::BXArtist bxTrackArtist;
  int iTrackArtistId = -1;
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

    //avoid accessing the db if the album has sequentially the same artist
    if (iTrackArtistId == -1 || bxTrackArtist.m_strName != track.strArtist)
    {
      bxTrackArtist.m_strName = track.strArtist;
      iTrackArtistId = BOXEE::Boxee::GetInstance().GetMetadataEngine().AddArtist(&bxTrackArtist);
    }

    bxAudio.m_iArtistId = iTrackArtistId;
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

    if (strThumbFileName.ToLower() == "folder")
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
    BOXEE::BXAlbum AlbumInDB;

    CStdString strAlbum = album->m_strTitle;
    CStdString strArtist = album->m_strArtist;

    BOXEE::BXMetadata metadata(MEDIA_ITEM_TYPE_AUDIO);
    if (ResolveAlbumMetadata(strAlbum, strArtist, &metadata))
    {

      BOXEE::BXAlbum* pUpdatedAlbum = (BOXEE::BXAlbum*)metadata.GetDetail(MEDIA_DETAIL_ALBUM);
      pUpdatedAlbum->m_iId = album->m_iId;
      pUpdatedAlbum->m_iStatus = STATUS_RESOLVED;
      //before writing, there might be already an artwork which was chosen by the user locally
      //if this is a valid local artwork, override the http:// value
      BOXEE::Boxee::GetInstance().GetMetadataEngine().GetAlbumById(albums[i]->m_iId,&AlbumInDB);

      if (BoxeeUtils::IsPathLocal( AlbumInDB.m_strArtwork ) && BOXEE::BXUtils::FileExists(AlbumInDB.m_strArtwork))
      {
        pUpdatedAlbum->m_strArtwork = AlbumInDB.m_strArtwork;
      }

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


  bool bResolved = GetMetadataFromServer(strAlbum, strArtist, pMetadata);
  if (bResolved)
  {
    pMetadata->m_bResolved = true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "CMetadataResolverMusic::ResolveAlbumMetadata, Could not resolve album = %s, artist = %s (musicresolver)", strAlbum.c_str(), strArtist.c_str());
  }

  return bResolved;
}

bool CMetadataResolverMusic::GetResultsFromServer(const CStdString& _strAlbum,const CStdString& _strArtist, const int _resultCount, BXXMLDocument& _resultDoc)
{
  CStdString strBoxeeServerUrl = BOXEE::BXConfiguration::GetInstance().GetStringParam("Boxee.Resolver.Server","http://res.boxee.tv");
  CStdString strLink;
  ListHttpHeaders headers;
  bool bRetVal = false;

  //_resultDoc.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  
  std::map<CStdString, CStdString> mapOptions;

  if (!_strArtist.IsEmpty())
  {
    mapOptions["artist"] = BOXEE::BXUtils::URLEncode(_strArtist);
  }
  if (!_strAlbum.IsEmpty())
  {
    mapOptions["title"] = BOXEE::BXUtils::URLEncode(_strAlbum);
  }
  if (_resultCount > 0)
  {
    std::stringstream strInt;
    strInt << _resultCount;
    mapOptions["count"] =  BOXEE::BXUtils::URLEncode(strInt.str());
  }

  strLink = strBoxeeServerUrl;
  strLink += "/api/album";
  strLink += BoxeeUtils::BuildParameterString(mapOptions);

  CLog::Log(LOGDEBUG, "CMetadataResolverMusic::GetMetadataFromServer, resolving album %s, by %s [link = %s] (musicresolver) (resolver)",_strAlbum.c_str(),_strArtist.c_str(), strLink.c_str());

  bRetVal = _resultDoc.LoadFromURL(strLink.c_str(), headers);

  return bRetVal;
}

bool CMetadataResolverMusic::GetMetadataFromServer(const CStdString& _strAlbum, const CStdString& _strArtist, BOXEE::BXMetadata * pMetadata)
{
  CLog::Log(LOGDEBUG, "CMetadataResolverMusic::GetMetadataFromServer, resolving album %s, by %s  (musicresolver) (resolver)",_strAlbum.c_str(),_strArtist.c_str());

  BXXMLDocument doc;
  bool bRetVal = GetResultsFromServer(_strAlbum,_strArtist,1,doc); //we need only one result

  if (!bRetVal)
  {
    int nLastErr = doc.GetLastNetworkError();
    CLog::Log(LOGDEBUG, "CMetadataResolverMusic::GetMetadataFromServer, unable to get results for album %s, by %s, network error = %d (musicresolver) (resolver)", _strAlbum.c_str(), _strArtist.c_str(), nLastErr);
    return bRetVal;
	}

  bRetVal = LoadAlbumInfo(doc,*pMetadata); //parse the first result in the doc

  return bRetVal;
}

bool CMetadataResolverMusic::LoadAlbumInfo(BOXEE::BXXMLDocument& doc,BXMetadata& albumRead)
{
  TiXmlElement* pElement = doc.GetDocument().RootElement();

  if (pElement && pElement->ValueStr() == "results" && pElement->FirstChildElement() && pElement->FirstChildElement()->ValueStr() == "album")
  {
    //there should be only 1 result from the server in this case
    return LoadAlbumInfo(pElement->FirstChildElement(),albumRead);
  }
  else
  {
    return false;
  }
}

bool CMetadataResolverMusic::LoadAlbumInfo(TiXmlElement* albumElement, BXMetadata& albumRead)
{
  if (albumRead.GetType() != MEDIA_ITEM_TYPE_AUDIO) 
    return false;

  BXAlbum* pAlbum = (BXAlbum*)albumRead.GetDetail(MEDIA_DETAIL_ALBUM);
  BXArtist* pArtist = (BXArtist*)albumRead.GetDetail(MEDIA_DETAIL_ARTIST);
  BXAudio* pAudio = (BXAudio*)albumRead.GetDetail(MEDIA_DETAIL_AUDIO);

  if (!pAlbum || !pArtist || !pAudio)
  {
    CLog::Log(LOGERROR, "%s - one of the details is NULL!! album: %p, artist: %p, audio: %p. ", __FUNCTION__, pAlbum, pArtist, pAudio);
    return false;
  }

  if (albumElement->ValueStr() == "album")
    albumElement = albumElement->FirstChildElement();

  if (albumElement->ValueStr() == "title")
  {
    if (albumElement->FirstChild())
      pAlbum->m_strTitle = albumElement->FirstChild()->ValueStr();
  }
  else
    return false;

  albumElement = albumElement->NextSiblingElement();

  if (albumElement->ValueStr() == "notes")
  {
    if (albumElement->FirstChild())
      pAlbum->m_strDescription = albumElement->FirstChild()->ValueStr();
  }
  else
    return false;

  albumElement = albumElement->NextSiblingElement();

  if (albumElement->ValueStr() == "genres")
  {
    if (albumElement->FirstChild())
      pAlbum->m_strGenre = albumElement->FirstChild()->ValueStr();
  }
  else
    return false;

  albumElement = albumElement->NextSiblingElement();

  if (albumElement->ValueStr() == "styles")
  {
    if (albumElement->FirstChild())
      pAudio->m_strGenre = albumElement->FirstChild()->ValueStr();
    
  }
  else
    return false;
  
  albumElement = albumElement->NextSiblingElement();

  if (albumElement->ValueStr() == "date")
  {
    if (albumElement->FirstChild())
      pAlbum->m_iYear = BOXEE::BXUtils::StringToInt(albumElement->FirstChild()->ValueStr());
  }
  else
    return false;

  albumElement = albumElement->NextSiblingElement();

  if (albumElement->ValueStr() == "artist")
  {
    if (albumElement->FirstChild())
      pAlbum->m_strArtist = albumElement->FirstChild()->ValueStr();
  }
  else
    return false;

  albumElement = albumElement->NextSiblingElement();

  if (albumElement->ValueStr() == "image")
  {
    if (albumElement->FirstChild())
      pAlbum->m_strArtwork = albumElement->FirstChild()->ValueStr();
  }
  else
    return false;
  
  pAudio->m_strAlbum = pAlbum->m_strTitle;
  pArtist->m_strName = pAlbum->m_strArtist;

  return true;
}

bool CMetadataResolverMusic::LoadAlbumsInfo(BOXEE::BXXMLDocument& doc, vectorMetadata& list)
{
  TiXmlElement* pRootElement = doc.GetDocument().RootElement();
  bool bRetVal = true;
  if (pRootElement->ValueStr() == "results")
  {
    TiXmlNode* pTag = 0;
    BXMetadata album(MEDIA_ITEM_TYPE_AUDIO);

    while ((pTag = pRootElement->IterateChildren(pTag)))
    {
      if (pTag && pTag->ValueStr() == "album")
      {
        TiXmlElement* pValue = pTag->FirstChildElement();

        if (pValue && (LoadAlbumInfo(pValue,album)))
          list.push_back(album);
        else
           bRetVal = false;
      }
      else
        bRetVal = false;
    }
  }
  else
    bRetVal = false;

  return bRetVal;
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
