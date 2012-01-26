
#include "PlatformDefs.h"
#include "BoxeeDatabaseDirectory.h"
#include "BoxeeUtils.h"
#include "lib/libBoxee/bxmetadataengine.h"
#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxutils.h"
#include "FileSystem/File.h"
#include "FileSystem/StackDirectory.h"
#include "VideoInfoTag.h"
#include "MusicInfoTag.h"
#include "PictureInfoTag.h"
#include "VideoInfoTag.h"
#include "URL.h"
#include "Util.h"
#include <stdio.h>
#include <math.h>
#include "utils/log.h"
#include "Album.h"
#include "DllLibExif.h"
#include "Application.h"
#include "FileSystem/DirectoryCache.h"
#include "SpecialProtocol.h"
#include "AdvancedSettings.h"
#include "../BrowseWindowFilter.h"

#define UNWATCHED_FILTER_ID   601
#define UNWATCHED_FILTER_NAME "unwatched"

#define SOURCE_FILTER_ID  701
#define SOURCE_FILTER_NAME  "local-source"

#define ITEMS_FILTERED_OUT_PROPERTY  "FilteredOut"

using namespace BOXEE;
using namespace XFILE;

namespace DIRECTORY
{

CBoxeeDatabaseDirectory::CBoxeeDatabaseDirectory()
{
  m_cacheDirectory = DIR_CACHE_ALWAYS;  // by default, caching is done.
}

CBoxeeDatabaseDirectory::~CBoxeeDatabaseDirectory()
{
}

bool CBoxeeDatabaseDirectory::GetMovies(std::map<std::string, std::string>& mapParams, CFileItemList &items, int iLimit)
{
  BXMetadataEngine& MDE = BOXEE::Boxee::GetInstance().GetMetadataEngine();

  std::vector<BXMetadata*> vecVideoFiles;
  std::vector<std::string> vecPathFilter;
  std::vector<CBrowseWindowLocalFilter*> vecFilters;

  if (CreateShareFilter("video", vecPathFilter))
  {
    CStdString genre = mapParams["genre"];
    CStdString prefix = mapParams["prefix"];
    CStdString searchTerm = mapParams["term"];

    if (searchTerm.IsEmpty())
    {
      MDE.GetMovies(vecVideoFiles, genre, prefix, vecPathFilter, iLimit);
    }
    else
    {
      CUtil::UrlDecode(searchTerm);
      MDE.SearchMoviesByTitle(searchTerm, vecVideoFiles, vecPathFilter, iLimit);
    }

    if (mapParams.find("unwatched") != mapParams.end())
    {
      CStdString strShowUnwatched = mapParams["unwatched"];

      if (strShowUnwatched == "true")
      {
        vecFilters.push_back(new CBrowseWindowTvShowUnwatchedFilter(UNWATCHED_FILTER_ID,UNWATCHED_FILTER_NAME, true));
      }
    }

    if (mapParams.find("source") != mapParams.end())
    {
      CStdString strSourceFilter = mapParams["source"];

      if (!strSourceFilter.IsEmpty())
      {
        vecFilters.push_back(new CBrowseWindowLocalSourceFilter(SOURCE_FILTER_ID,SOURCE_FILTER_NAME, strSourceFilter));
      }
    }

    CreateFileItemList(vecVideoFiles, items);

    for (int i = 0; i < items.Size(); i++)
    {
      bool isFiltered = false;

      for (std::vector<CBrowseWindowLocalFilter*>::iterator it = vecFilters.begin(); it != vecFilters.end() ; it++)
      {
        if ((*it)->Apply(&*items[i]) != true)
        {
          items.Remove(i);
          i--;
          isFiltered = true;
          break; //we don't continue filtering if we removed that item
        }
      }

      if (!isFiltered)
      {
        items.Get(i)->SetProperty("value", items.Get(i)->GetLabel());
      }
    }

    if (!vecFilters.empty() && items.IsEmpty())
    {
      items.SetProperty(ITEMS_FILTERED_OUT_PROPERTY,true);
    }

    for (std::vector<CBrowseWindowLocalFilter*>::iterator it = vecFilters.begin(); it != vecFilters.end() ; it = vecFilters.erase(it))
    {
      delete (*it);
    }

    ClearMetadataVector(vecVideoFiles);
  }

  return true;
}

bool CBoxeeDatabaseDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  BXMetadataEngine& MDE = BOXEE::Boxee::GetInstance().GetMetadataEngine();

  CStdString strParams;
  CStdString strDir;
  CStdString strFile;
  std::map<std::string, std::string> mapParams;

  // Parse boxeedb url
  if (!BoxeeUtils::ParseBoxeeDbUrl(strPath, strDir, strFile, mapParams))
    return false;

  CLog::Log(LOGDEBUG,"CBoxeeDatabaseDirectory::GetDirectory - [path=%s][strDir=%s][strFile=%s][strParams=%s] (browse)(bdbd)", strPath.c_str(), strDir.c_str(), strFile.c_str(), strParams.c_str());

  // Set the limit
  int iLimit = BXUtils::StringToInt(mapParams["limit"]);
  if (iLimit <= 0)
  {
    iLimit = 100000; // TODO: Check that this is valid !!!
  }

  std::vector<BXMetadata*> vecMediaItems;

  if( strDir == "album" && strFile == "" )
  {
    // path of type "boxeedb://album/?id=xxx"

    std::vector<BXMetadata*> vecMusicFiles;

    CStdString strTitle = BXUtils::URLDecode(mapParams["title"]);
    CStdString strAristId = mapParams["artistId"];
    CStdString strAristName = BXUtils::URLDecode(mapParams["artist"]);
    CStdString strId = mapParams["id"];

    if (!strId.IsEmpty())
    {
      int iAlbumId = BXUtils::StringToInt(strId);
      MDE.GetSongsFromAlbum(iAlbumId, vecMusicFiles);
    }
    else if (!strAristName.IsEmpty())
    {
      MDE.GetAlbumsByTitleAndArtist(strTitle, strAristName, vecMusicFiles);
    }
    else
    {
      MDE.GetAlbumsByTitleAndArtist(strTitle, BXUtils::StringToInt(strAristId), vecMusicFiles);
    }

    CreateFileItemList(vecMusicFiles, items);

    for(size_t i=0; i < vecMusicFiles.size(); i++) {
      delete vecMusicFiles[i];
    }
    vecMusicFiles.clear();

  }
  //  else if( strDir == "album" )
  //  {
  //    // path of type boxeedb://album/101
  //    BXMetadata* pAlbumMetadata = new BXMetadata(MEDIA_ITEM_TYPE_ALBUM);
  //    BXAlbum* pAlbum = (BXAlbum*)pAlbumMetadata->GetDetail(MEDIA_DETAIL_ALBUM);
  //    BXArtist* pArtist = (BXArtist*)pAlbumMetadata->GetDetail(MEDIA_DETAIL_ARTIST);
  //
  //    std::vector<BXMetadata*> vecAlbums;
  //
  //    int iAlbumId = BXUtils::StringToInt(strFile);
  //    if (MDE.GetAlbumById(iAlbumId, pAlbum))
  //    {
  //      // Get artist id
  //      int iArtistId = pAlbum->m_iArtistId;
  //      MDE.GetArtistById(iArtistId, pArtist);
  //      vecAlbums.push_back(pAlbumMetadata);
  //    }
  //
  //    CreateFileItemList(vecAlbums, items);
  //    delete pAlbumMetadata;
  //    vecAlbums.clear();
  //  }
  else if (strDir == "artist")
  {
    int iArtistId = BXUtils::StringToInt(strFile);

    CLog::Log(LOGDEBUG, "CBoxeeDatabaseDirectory::GetDirectory - Retrieving albums for artist %d (browse)(bdbd)", iArtistId);

    std::vector<BXMetadata*> vecMusicFiles;
    std::vector<std::string> vecPathFilter;
    if (CreateShareFilter("music", vecPathFilter))
    {
      MDE.GetAlbumsByArtist(iArtistId, vecMusicFiles, vecPathFilter);
      CreateFileItemList(vecMusicFiles, items);

      for(size_t i=0; i < vecMusicFiles.size(); i++) {
        delete vecMusicFiles[i];
      }
      vecMusicFiles.clear();
    }
  }
  else if (strDir == "tracks")
  {
    int iAlbumId = BXUtils::StringToInt(BXUtils::URLDecode(mapParams["albumId"]));

    CLog::Log(LOGDEBUG, "CBoxeeDatabaseDirectory::GetDirectory - Retrieving tracks for album [id=%d] (browse)(bdbd)", iAlbumId);

    // Get album details
    BXMetadata* pAlbumMetadata = new BXMetadata(MEDIA_ITEM_TYPE_ALBUM);
    BXAlbum* pAlbum = (BXAlbum*)pAlbumMetadata->GetDetail(MEDIA_DETAIL_ALBUM);
    BXArtist* pArtist = (BXArtist*)pAlbumMetadata->GetDetail(MEDIA_DETAIL_ARTIST);

    std::vector<BXMetadata*> vecAlbums;

    if (MDE.GetAlbumById(iAlbumId, pAlbum))
    {
      // Get artist id
      int iArtistId = pAlbum->m_iArtistId;
      MDE.GetArtistById(iArtistId, pArtist);
    }

    //CFileItem albumItem;
    CreateAlbumItem(pAlbumMetadata, &items);
    delete pAlbumMetadata;

    std::vector<BXMetadata*> vecMusicFiles;
    MDE.GetSongsFromAlbum(iAlbumId, vecMusicFiles);
    CreateFileItemList(vecMusicFiles, items);

    for(size_t i=0; i < vecMusicFiles.size(); i++)
    {
      delete vecMusicFiles[i];
    }
    vecMusicFiles.clear();

  }
  //  else if (strDir == "series") // NOT SUPPORTED IN CARLA
  //  {
  //    int iSeriesId = BXUtils::StringToInt(strFile);
  //
  //    std::vector<BXMetadata*> vecSeasons;
  //    std::vector<std::string> vecPathFilter;
  //    if (CreateShareFilter("video", vecPathFilter))
  //    {
  //      MDE.GetSeasonsFromSeries(iSeriesId, vecSeasons, vecPathFilter);
  //      CreateFileItemList(vecSeasons, items);
  //
  //      for(size_t i=0; i < vecSeasons.size(); i++) {
  //        delete vecSeasons[i];
  //      }
  //      vecSeasons.clear();
  //    }
  //  }
  //  else if (strDir == "season")
  //  {
  //    CStdStringArray series;
  //    StringUtils::SplitString(strFile, "_", series);
  //
  //    CStdString strSeason = series[series.size() -1];
  //    CStdString strSeries  = series[series.size() -2];
  //
  //    int iSeriesId = BXUtils::StringToInt(strSeries);
  //    int iSeasonId = BXUtils::StringToInt(strSeason);
  //
  //    std::vector<BXMetadata*> vecEpisodes;
  //    std::vector<std::string> vecPathFilter;
  //    if (CreateShareFilter("video", vecPathFilter))
  //    {
  //      MDE.GetEpisodesFromSeason(iSeriesId, iSeasonId, vecEpisodes, vecPathFilter);
  //      CreateFileItemList(vecEpisodes, items);
  //
  //      for(size_t i=0; i < vecEpisodes.size(); i++) {
  //        delete vecEpisodes[i];
  //      }
  //      vecEpisodes.clear();
  //    }
  //  }
  else if (strDir == "episodes")
  {
    CStdString strTvShowBoxeeId = mapParams["seriesId"];

    CUtil::UrlDecode(strTvShowBoxeeId);

    CLog::Log(LOGDEBUG,"CBoxeeDatabaseDirectory::GetDirectory - get episodes for [%s] (browse)(bdbd)", strTvShowBoxeeId.c_str());

    std::vector<BXMetadata*> vecEpisodes;
    std::vector<std::string> vecPathFilter;
    if (CreateShareFilter("video", vecPathFilter))
    {
      MDE.GetEpisodes(strTvShowBoxeeId, -1 /*all seasons */, vecEpisodes, vecPathFilter);
      CreateFileItemList(vecEpisodes, items);

      CLog::Log(LOGDEBUG, "CBoxeeDatabaseDirectory::GetDirectory - got [%zu] created [%d] episodes for [%s] (browse)(bdbd)", vecEpisodes.size(), items.Size(), strTvShowBoxeeId.c_str());

      for(size_t i=0; i < vecEpisodes.size(); i++)
      {
        delete vecEpisodes[i];
      }

      vecEpisodes.clear();
    }
  }
  else if (strDir == "artists")
  {
    GetArtists(items, iLimit);
  }
  else if (strDir == "music")
  {
    std::vector<BXMetadata*> vecMusic;
    std::vector<std::string> vecPathFilter;
    if (CreateShareFilter("music", vecPathFilter))
    {

      CStdString searchTerm = mapParams["term"];
      if (!searchTerm.IsEmpty())
      {
        CUtil::UrlDecode(searchTerm);
        BOXEE::Boxee::GetInstance().GetMetadataEngine().SearchMusic(searchTerm, vecMusic, vecPathFilter, iLimit);
      }

      CreateFileItemList(vecMusic, items);

      for (int i = 0; i < items.Size(); i++)
      {
        items.Get(i)->SetProperty("value", items.Get(i)->GetLabel());
      }

      ClearMetadataVector(vecMusic);
    }
  }
  else if (strDir == "albums")
  {
    std::vector<BXMetadata*> vecAlbums;
    std::vector<std::string> vecPathFilter;
    if (CreateShareFilter("music", vecPathFilter))
    {
      BOXEE::Boxee::GetInstance().GetMetadataEngine().GetAlbums(vecAlbums, vecPathFilter, iLimit);

      CreateFileItemList(vecAlbums, items);
      ClearMetadataVector(vecAlbums);
    }
  }
  else if (strDir == "video")
  {
    std::vector<BXMetadata*> vecVideoFiles;
    std::vector<std::string> vecPathFilter;
    if (CreateShareFilter("video", vecPathFilter))
    {
      CStdString strTitle = BXUtils::URLDecode(mapParams["title"]);

      if (strTitle != "")
      {
        MDE.GetVideosByTitle(vecVideoFiles, strTitle,vecPathFilter, iLimit);
      }
      else
      {
        CLog::Log(LOGERROR, "CBoxeeDatabaseDirectory::GetDirectory - Can not get videos without titles (browse)(bdbd)");
      }

      CreateFileItemList(vecVideoFiles, items);
      ClearMetadataVector(vecVideoFiles);
    }
  }
  else if (strDir == "tvshows")
  {
    std::vector<BXMetadata*> vecVideoFiles;
    std::vector<std::string> vecPathFilter;
    if (CreateShareFilter("video", vecPathFilter))
    {
      CStdString genre = mapParams["genre"];
      CStdString prefix = mapParams["prefix"];
      CStdString searchTerm = mapParams["term"];

      if (searchTerm.IsEmpty())
      {
        CLog::Log(LOGDEBUG, "CBoxeeDatabaseDirectory::HandleTvShows - get tv shows [genre=%s][prefix=%s] (browse)(bdbd)", genre.c_str(), prefix.c_str());
        MDE.GetTvShows(vecVideoFiles, genre, prefix, vecPathFilter, iLimit);
      }
      else
      {
        CLog::Log(LOGDEBUG, "CBoxeeDatabaseDirectory::HandleTvShows, search by title = %s (search)", searchTerm.c_str());
        CUtil::UrlDecode(searchTerm);
        MDE.SearchTvShowsByTitle(searchTerm, vecVideoFiles, vecPathFilter, iLimit);
      }

      CLog::Log(LOGDEBUG, "CBoxeeDatabaseDirectory::HandleTvShows - get [%d] tv shows [genre=%s][prefix=%s] (browse)(bdbd)", (int)vecVideoFiles.size(), genre.c_str(), prefix.c_str());
      CreateFileItemList(vecVideoFiles, items);

      for (int i = 0; i < items.Size(); i++)
      {
        items.Get(i)->SetProperty("value", items.Get(i)->GetLabel());
      }

      ClearMetadataVector(vecVideoFiles);
    }
  }
  else if (strDir == "movies")
  {
    GetMovies(mapParams,items,iLimit);
  }
  else if (strDir == "unresolvedVideoFolders")
  {
    std::vector<BXMetadata*> vecVideoFolders;
    std::vector<std::string> vecPathFilter;
    if (CreateShareFilter("video", vecPathFilter))
    {
      MDE.GetUnresolvedVideoFolders(vecVideoFolders, vecPathFilter, iLimit);
      CreateFileItemList(vecVideoFolders, items);
      ClearMetadataVector(vecVideoFolders);
    }
  }
  else if (strDir == "unresolvedVideoFiles")
  {
    std::vector<BXMetadata*> vecUnresolvedVideos;
    std::vector<std::string> vecPathFilter;
    if (CreateShareFilter("video", vecPathFilter))
    {
      MDE.GetUnresolvedVideoFiles(vecUnresolvedVideos, vecPathFilter, iLimit);
      CreateFileItemList(vecUnresolvedVideos, items);
      for (int i = 0; i < items.Size(); i++)
      {
        items.Get(i)->SetProperty("isUnresolvedVideo", true);
        CStdString label = items.Get(i)->GetLabel();
        CUtil::UrlDecode(label);
        items.Get(i)->SetLabel(label);
      }
      ClearMetadataVector(vecUnresolvedVideos);
    }
  }
  else if (strDir == "pictures")
  {
    GetPictures(items, iLimit);
  }
  else if (strDir == "recent")
  {
    // We retreive all three types first
    CFileItemList videos;
    CFileItemList albums;
    CFileItemList tvshows;
    CFileItemList recent;

    GetDirectory("boxeedb://tvshows/?limit=0", tvshows);
    GetDirectory("boxeedb://movies/?limit=0", videos);
    GetDirectory("boxeedb://albums/?limit=0", albums);

    recent.Append(videos);
    recent.Append(albums);
    recent.Append(tvshows);

    recent.Sort(SORT_METHOD_DATE_ADDED, SORT_ORDER_DESC);

    int size = iLimit < recent.Size() ? iLimit : recent.Size();
    for (int i = 0; i < size ; i++)
    {
      CFileItemPtr pNewItem ( new CFileItem(*recent.Get(i)) );
      items.Add(pNewItem);
    }

    videos.Clear();
    albums.Clear();
    tvshows.Clear();
    recent.Clear();

  }
  else if (strDir == "unresolved")
  {
    CStdString strPath = mapParams["path"];
    CLog::Log(LOGDEBUG, "STATUS, Get all unresolved folders under: %s", strPath.c_str());

    std::vector<BXMetadata*> vecUnresolvedFolders;
    MDE.GetUnresolvedFoldersByPath(strPath, vecUnresolvedFolders);

    CLog::Log(LOGDEBUG, "STATUS, Got %zu unresolved folders for path %s", vecUnresolvedFolders.size(), strPath.c_str());

    CreateFileItemList(vecUnresolvedFolders, items);

    CLog::Log(LOGDEBUG, "STATUS, Got %d unresolved folder items for path %s", items.Size(), strPath.c_str());
    ClearMetadataVector(vecUnresolvedFolders);

  }
  else
  {
    CLog::Log(LOGERROR, "CBoxeeDatabaseDirectory::GetDirectory, unrecognized path %s", strPath.c_str());
    return false;
  }

  for(size_t i=0; i < vecMediaItems.size(); i++) {
    delete vecMediaItems[i];
  }
  vecMediaItems.clear();

  //remove not allowed content
  int i = 0;
  while(i < items.Size())
  {
    if(!items[i]->IsAllowed())
    {
      items.Remove(i);
      continue;
    }
    i++;
  }

  return true;
}

bool CBoxeeDatabaseDirectory::GetArtists(CFileItemList &items, int iLimit)
{
  // Retreive all music files from the database
  // TODO: Add filters so that artists that have no albums on available drives are not shown
  std::vector<BXMetadata*> vecMusicFiles;
  std::vector<std::string> vecPathFilter;

  if (CreateShareFilter("music", vecPathFilter))
  {
    BOXEE::Boxee::GetInstance().GetMetadataEngine().GetArtists(vecMusicFiles, vecPathFilter, iLimit);

    CreateFileItemList(vecMusicFiles, items);
    ClearMetadataVector(vecMusicFiles);
    return true;
  }
  return false;
}

bool CBoxeeDatabaseDirectory::GetPictures(CFileItemList &items, int iLimit)
{
  std::vector<BXMetadata*> vecPictureFiles;
  VECSOURCES * pVecShares = g_settings.GetSourcesFromType("pictures");

  if (pVecShares) 
  {
    for (IVECSOURCES it = pVecShares->begin(); it != pVecShares->end(); it++)
    {
      CFileItemPtr pItem ( new CFileItem(it->strName) );
      pItem->m_strPath = it->strPath;
      pItem->m_bIsFolder = true;
      // We have to reset the because we do not was them to be sorted as ones
      pItem->SetProperty("IsShare",false);
      pItem->SetProperty("IsPictureFolder","1");
      pItem->SetProperty("IsAvailable", g_application.IsPathAvailable(pItem->m_strPath, !pItem->IsSmb()));

      // Filter out plugins and applications
      if (!pItem->IsPlugin() && !pItem->IsApp())
      {
        items.Add(pItem);
      }
    }
  }
  return true;
}

void CBoxeeDatabaseDirectory::ClearMetadataVector(std::vector<BXMetadata*>& vecMediaItems)
{
  for(size_t i=0; i < vecMediaItems.size(); i++)
  {
    delete vecMediaItems[i];
  }
  vecMediaItems.clear();
}

void CBoxeeDatabaseDirectory::FillItemDetails(CFileItem *pItem)
{
  if ( pItem->IsRSS() )
    pItem->SetProperty ( "IsRss","1" );

  if ( pItem->IsLastFM() )
    pItem->SetProperty ( "IsLastFM","1" );

  if ( pItem->IsShoutCast() )
    pItem->SetProperty ( "IsShoutCast","1" );

  if ( pItem->IsBoxeeDb() )
    pItem->SetProperty ( "IsBoxeeDB","1" );

  if ( pItem->IsPlugin() )
    pItem->SetProperty ( "IsPlugin","1" );

  if ( pItem->IsScript() )
    pItem->SetProperty ( "IsScript","1" );

  if ( pItem->m_bIsFolder )
    pItem->SetProperty ( "IsFolder","1" );

  if ( pItem->IsInternetStream() )
    pItem->SetProperty ( "IsInternetStream","1" );

  if ( pItem->m_strPath == g_application.CurrentFile() )
    pItem->SetProperty ( "IsNowPlaying","1" );

  if ( pItem->HasMusicInfoTag() )
  {
    pItem->SetProperty ( "HasMusicInfo","1" );
    if ( !pItem->GetMusicInfoTag()->GetComment().IsEmpty() )
    {
      pItem->SetProperty ( "HasMoreInfo","1" );
      pItem->SetProperty ( "HasDescription","1" );
    }
  }

  if ( pItem->HasVideoInfoTag() )
  {
    pItem->SetProperty("isvideo","1");

    pItem->SetProperty ( "HasVideoInfo","1" );
    if ( pItem->GetVideoInfoTag()->m_strPlotOutline.IsEmpty() )
      pItem->GetVideoInfoTag()->m_strPlotOutline = pItem->GetVideoInfoTag()->m_strPlot;

    if ( !pItem->GetVideoInfoTag()->m_strPlot.IsEmpty() )
    {
      pItem->SetProperty ( "HasMoreInfo","1" );

      if (pItem->GetProperty("description").IsEmpty())
      {
        pItem->SetProperty("description",pItem->GetVideoInfoTag()->m_strPlot);
      }
    }

    if ( !pItem->GetVideoInfoTag()->m_strPlotOutline.IsEmpty() )
    {
      pItem->SetProperty ( "HasDescription","1" );

      if (pItem->GetProperty("description").IsEmpty())
      {
        pItem->SetProperty("description",pItem->GetVideoInfoTag()->m_strPlotOutline);
      }
    }

    if (pItem->GetProperty("rts-mediatype").IsEmpty())
    {
      if (pItem->GetVideoInfoTag()->m_iEpisode > 0) // hack
      {
        pItem->SetProperty("istvshow","1");
        pItem->SetProperty("rts-mediatype","tv_show");
      }
      else if (!pItem->GetProperty("boxeeid").IsEmpty())
      {
        pItem->SetProperty("rts-mediatype","movie");
      }
      else
      {
        pItem->SetProperty("rts-mediatype","video");
      }
    }
  }

  if ( pItem->HasProperty ( "description" ) && !pItem->GetProperty ( "description" ).IsEmpty() )
    pItem->SetProperty ( "HasDescription","1" );

  //if ( !pItem->m_strPath.IsEmpty() )
  //  pItem->SetProperty ( "IsAvailable", g_application.IsPathAvailable ( pItem->m_strPath, !pItem->IsSmb() ));

  if ( !pItem->GetThumbnailImage().IsEmpty() || !pItem->GetIconImage().IsEmpty() )
    pItem->SetProperty ( "HasThumb", "1" );

  CURI url ( pItem->m_strPath );
  if ( url.GetProtocol() == "boxeedb" )
  {
    pItem->SetProperty ( "DisplayPath", "" );
  }
  else
  {
    CStdString strUrl = url.GetWithoutUserDetails();
    pItem->SetProperty ( "DisplayPath", strUrl );
  }

  BoxeeUtils::MarkWatched(pItem);
}

bool CBoxeeDatabaseDirectory::CreateAudioItem(const BXMetadata* pMetadata, CFileItem* pItem)
{
  if (pMetadata->GetType() != MEDIA_ITEM_TYPE_AUDIO) return false;

  CMusicInfoTag* pInfoTag = pItem->GetMusicInfoTag();
  BXAudio* pAudio = (BXAudio*)pMetadata->GetDetail(MEDIA_DETAIL_AUDIO);
  BXArtist* pArtist = (BXArtist*)pMetadata->GetDetail(MEDIA_DETAIL_ARTIST);
  BXAlbum* pAlbum = (BXAlbum*)pMetadata->GetDetail(MEDIA_DETAIL_ALBUM);

  //CLog::Log(LOGDEBUG, "CBoxeeDatabaseDirectory::CreateAudioItem, album = %s, artist = %s", pAlbum->m_strTitle.c_str(), pArtist->m_strName.c_str());

  pInfoTag->SetTitle(pAudio->m_strTitle);
  pInfoTag->SetYear(pAudio->m_iYear);
  pInfoTag->SetDuration(pAudio->m_iDuration);
  pInfoTag->SetGenre(pAudio->m_strGenre);
  pInfoTag->SetTrackNumber(pAudio->m_iTrackNumber);
  pItem->SetLabel(pAudio->m_strTitle);

  // Set the path item to be the real path of the audio file
  pItem->m_strPath = pAudio->m_strPath;

  pInfoTag->SetArtist(pArtist->m_strName);
  pInfoTag->SetAlbum(pAlbum->m_strTitle);

  CAlbum album;
  CScraperUrl url( pAlbum->m_strArtwork);
  album.thumbURL = url;
  album.iYear = pAlbum->m_iYear;
  album.strArtist = pArtist->m_strName;
  album.strAlbum = pAlbum->m_strTitle;
  album.strGenre = pAlbum->m_strGenre;
  pInfoTag->SetAlbum(album);

  pItem->SetProperty("BoxeeDBAlbumId", pAlbum->m_iId);

  pItem->m_dateTime = CDateTime(pAlbum->m_iDateModified);
  pItem->SetProperty("dateadded", pAlbum->m_iDateAdded);

  pItem->SetThumbnailImage(pAlbum->m_strArtwork);

  if ((pItem->m_strPath == "") || (pItem->GetLabel() == ""))
  {
    CLog::Log(LOGWARNING,"CBoxeeDatabaseDirectory::CreateAudioItem - return FALSE since item [path=%s] or [label=%s] is EMPTY",pItem->m_strPath.c_str(),pItem->GetLabel().c_str());
    return false;
  }

  return true;
}

/*
 * Handles the case of multipart videos by constructing a stack path
 */
CStdString CBoxeeDatabaseDirectory::ConstructVideoPath(const BXPath& path)
{
  CStdString strPath;

  if (path.m_vecParts.size() > 0)
  {
    // Construct the list of file items to use with stack directory
    CFileItemList partItems;

    std::vector<int> stack;
    for (size_t i = 0; i < path.m_vecParts.size(); i++)
    {
      CFileItemPtr item ( new CFileItem(path.m_vecParts[i], false) );
      partItems.Add(item);

      stack.push_back(i);
    }

    CStackDirectory stackDirectory;
    strPath = stackDirectory.ConstructStackPath(partItems, stack);
  }
  else
  {
    strPath = path.m_strPath;
  }
  return strPath;
}

bool CBoxeeDatabaseDirectory::CreateVideoItem(const BXMetadata* pMetadata, CFileItem* pItem)
{
  if (pMetadata->GetType() != MEDIA_ITEM_TYPE_VIDEO) return false;

  BXVideo* pVideo = (BXVideo*)pMetadata->GetDetail(MEDIA_DETAIL_VIDEO);
  BXSeries* pSeries = (BXSeries*)pMetadata->GetDetail(MEDIA_DETAIL_SERIES);

  CVideoInfoTag* pInfoTag = pItem->GetVideoInfoTag();
  pInfoTag->Reset();
  BoxeeUtils::ConvertBXVideoToVideoInfoTag(pVideo, *pInfoTag);

  // IsAdult flag on video
  bool isAdult = BoxeeUtils::IsAdult(pVideo->m_strMPAARating,CRating::MPAA);
  pItem->SetAdult(isAdult);

  // Title of the video
  pItem->SetLabel(pVideo->m_strTitle);
  pItem->SetProperty("title", pVideo->m_strTitle);
  pItem->SetLabelPreformated(true);

  pItem->m_dateTime = CDateTime((time_t)pVideo->m_iReleaseDate);

  // Copy certain video info fields to properties
  pItem->SetProperty("showid", pSeries->m_strBoxeeId);
  pItem->SetProperty("duration", pInfoTag->m_strRuntime);
  pItem->SetProperty("videorating", pInfoTag->m_iRating);

  if (pInfoTag->m_rottenTomatoDetails.bValidAudienceDetails)
  {
    pItem->SetProperty("rt-audience-score",pInfoTag->m_rottenTomatoDetails.iAudienceScore);
    pItem->SetProperty("rt-audience-rating",pInfoTag->m_rottenTomatoDetails.strAudienceRating);
    pItem->SetProperty("rt-has-audience-info",true);
  }

  if (pInfoTag->m_rottenTomatoDetails.bValidCriticsDetails)
  {
    pItem->SetProperty("rt-critics-score",pInfoTag->m_rottenTomatoDetails.iCriticsScore);
    pItem->SetProperty("rt-critics-rating",pInfoTag->m_rottenTomatoDetails.strCriticsRating);
    pItem->SetProperty("rt-has-critics-info",true);
  }

  if (!pSeries->m_strTitle.empty())
  {
    pInfoTag->m_strShowTitle = pSeries->m_strTitle;
    pItem->SetProperty("tvshowtitle", pSeries->m_strTitle);
    pItem->SetProperty("tvshowdescription", pSeries->m_strDescription);
  }

  if (pItem->GetProperty("rts-mediatype").IsEmpty())
  {
    ///////////////////////////////////////////////////////////
    // 041010 - datebase contain only tv-show or movie items //
    ///////////////////////////////////////////////////////////

    // TODO: Consider changing this check to pVideo->m_iSeriesId;
    if (pInfoTag->m_iEpisode > 0) // hack
    {
      pItem->SetProperty("istvshow","1");
      pItem->SetProperty("rts-mediatype","tv_show");
    }
    else
    {
      pItem->SetProperty("rts-mediatype","movie");
    }
  }

  // Set the "isvideo" property to indicate that this is a video item
  pItem->SetProperty("isvideo","1");

  pItem->SetProperty("boxeeDBvideoId", pVideo->m_iId);
  pItem->SetProperty("boxeeId", pVideo->m_strBoxeeId);

  if (!pVideo->m_strNfoPath.empty())
  {
    pItem->SetProperty("hasNFO",true);
  }

  if (pInfoTag->m_strTrailer != "") 
  {
    pItem->SetProperty("hastrailer","1");
  }

  // An item might have several instances, we need to construct the matching paths and set them to the list
  std::vector<BXPath> vecLinks = pVideo->m_vecLinks;
  CStdString firstPath;
  for (size_t i = 0; i < vecLinks.size(); i++)
  {
    CStdString strPath = ConstructVideoPath(vecLinks[i]);
    pItem->AddLink(pItem->GetLabel(), strPath, pItem->GetContentType(true), CLinkBoxeeType::LOCAL, "", "", "", "all", true, "", 0, CLinkBoxeeOffer::FREE, "");

    pItem->SetProperty("haslink-free-local", true);

    if (firstPath.empty())
    {
      firstPath = strPath;
    }
  }

  for (std::vector<BXVideoLink>::iterator it = pVideo->m_vecVideoLinks.begin(); it != pVideo->m_vecVideoLinks.end(); it++)
  {
    pItem->AddLink(it->m_strTitle.c_str(), it->m_strURL.c_str(), it->m_strType.c_str(), CFileItemList::GetLinkBoxeeTypeAsEnum(it->m_strBoxeeType.c_str()),
        it->m_strProvider.c_str(), it->m_strProviderName.c_str(), it->m_strProviderThumb.c_str(), it->m_strCountryCodes.c_str(), BXUtils::StringToInt(it->m_strCountryRel),
        it->m_strQualityLabel.c_str(), BXUtils::StringToInt(it->m_strQuality.c_str()), CFileItemList::GetLinkBoxeeOfferAsEnum(it->m_strOffer.c_str()), "");
  }

  // We need to set this field in order to play the file correctly in Boxee Video Main screen
  pInfoTag->m_strFileNameAndPath = firstPath;
  pItem->m_strPath = firstPath;

  // Date when the file was last modified on the disk
  pItem->m_dateTime = CDateTime(pVideo->m_iDateModified);

  // Date when the item was added to the database
  pItem->SetProperty("dateadded", pVideo->m_iDateAdded);

  if (pItem->GetThumbnailImage().IsEmpty() && !pVideo->m_strCover.empty())
  {
    pItem->SetThumbnailImage(pVideo->m_strCover);
  }

  if (!pVideo->m_strOriginalCover.empty())
  {
    pItem->SetProperty("OriginalThumb",pVideo->m_strOriginalCover);
  }

  CStdString strRatingChar;
  strRatingChar.Format("%d",(int)(pItem->GetVideoInfoTag()->m_fRating));
  pItem->SetProperty("videorating",strRatingChar);
  CDateTime relDate(pVideo->m_iReleaseDate);

  pItem->SetProperty("releasedate", relDate.GetAsddMMMYYYYDate());

  if (pVideo->m_iReleaseDate != 0)
  {
    pItem->SetProperty("releasedateVal" , pVideo->m_iReleaseDate);
  }
  else
  {
    CDateTime relYear;
    relYear.SetDate(pVideo->m_iYear , 12 , 31);
    time_t epochTime;
    relYear.GetAsTime(epochTime);
    unsigned long iEpochTime = (unsigned long) epochTime;
    pItem->SetProperty("releasedateVal" , iEpochTime);
  }

  // fill more info (properties) on the item based on its values
  FillItemDetails(pItem);
  pItem->FillInDefaultIcon();

  std::string strActors = BXUtils::VectorTokened(pVideo->m_vecActors);
  if (!strActors.empty())
    pItem->SetProperty("cast", strActors);

  if ((pItem->m_strPath == "") || (pItem->GetLabel() == ""))
  {
    CLog::Log(LOGWARNING,"CBoxeeDatabaseDirectory::CreateVideoItem - return FALSE since item [path=%s] or [label=%s] is EMPTY",pItem->m_strPath.c_str(),pItem->GetLabel().c_str());
    return false;
  }

  return true;

}

bool CBoxeeDatabaseDirectory::CreatePictureItem(const BXMetadata* pMetadata, CFileItem* pItem)
{
  //CLog::Log(LOGDEBUG, "LIBRARY: Creating picture item for %s", pMetadata->GetPath().c_str());
  //BXPicture* pPicture = (BXPicture*)pMetadata->GetDetail(MEDIA_DETAIL_PICTURE);
  CPictureInfoTag* pInfoTag = pItem->GetPictureInfoTag();
  ExifInfo_t exifInfo = pInfoTag->GetEXIFfInfo();
  // TODO: Add correct formatting and information copying
  //exifInfo.DateTime = BXUtils::IntToString(pPicture->m_iDate).c_str();
  //pInfoTag->SetInfo() //? There is no such function

  pItem->m_dateTime = CDateTime(pMetadata->m_iDateModified);
  pItem->SetProperty("dateadded", pMetadata->m_iDateAdded);

  // fill more info (properties) on the item based on its values
  FillItemDetails(pItem);
  return true;
}

bool CBoxeeDatabaseDirectory::CreateArtistItem(const BXMetadata* pMetadata, CFileItem* pItem)
{
  CMusicInfoTag* pInfoTag = pItem->GetMusicInfoTag();

  BXArtist* pArtist = (BXArtist*)pMetadata->GetDetail(MEDIA_DETAIL_ARTIST);
  if (pArtist) {
    pInfoTag->SetArtist(pArtist->m_strName);
    pItem->SetLabel(pArtist->m_strName);
    pItem->SetLabel2(pArtist->m_strName);
    pItem->m_bIsFolder = true;
    pItem->m_strPath = pMetadata->GetPath();
    pItem->SetThumbnailImage(pArtist->m_strPortrait);

    FillItemDetails(pItem);
    pItem->SetProperty("IsArtist", true);
  }

  if ((pItem->m_strPath == "") || (pItem->GetLabel() == ""))
  {
    CLog::Log(LOGWARNING,"CBoxeeDatabaseDirectory::CreateArtistItem - return FALSE since item [path=%s] or [label=%s] is EMPTY",pItem->m_strPath.c_str(),pItem->GetLabel().c_str());
    return false;
  }

  return true;
}

bool CBoxeeDatabaseDirectory::CreateAlbumItem(const BXMetadata* pMetadata, CFileItem* pItem)
{
  CMusicInfoTag* pInfoTag = pItem->GetMusicInfoTag();

  BXArtist* pArtist = (BXArtist*)pMetadata->GetDetail(MEDIA_DETAIL_ARTIST);
  if (pArtist)
  {
    pInfoTag->SetArtist(pArtist->m_strName);
    pInfoTag->SetAlbumArtist(pArtist->m_strName);
  }

  BXAlbum* pAlbum = (BXAlbum*)pMetadata->GetDetail(MEDIA_DETAIL_ALBUM);
  if (pAlbum)
  {
    //CLog::Log(LOGDEBUG, "LIBRARY, DIRECTORY, LIB1, Creating album item for %s, path = %s", pAlbum->m_strTitle.c_str(), pMetadata->GetPath().c_str());

    pInfoTag->SetAlbum(pAlbum->m_strTitle);
    pInfoTag->SetGenre(pAlbum->m_strGenre);
    pInfoTag->SetYear(pAlbum->m_iYear);
    pInfoTag->SetComment(pAlbum->m_strDescription);
    pItem->SetProperty("description", pAlbum->m_strDescription.c_str());
    pItem->SetProperty("AlbumFolderPath", pAlbum->GetPath());

    if (pAlbum->m_strArtwork != "") {
      pItem->SetThumbnailImage(pAlbum->m_strArtwork);
    }

    pItem->SetLabel(pAlbum->m_strTitle);
    pItem->m_bIsFolder = true;

    pItem->m_strPath = "boxeedb://album/?id=";
    pItem->m_strPath += BXUtils::IntToString(pAlbum->m_iId);
    pItem->m_strPath += "/";

    pItem->m_dateTime = CDateTime(pAlbum->m_iDateModified);

    pItem->SetProperty("dateadded", pAlbum->m_iDateAdded);

    // fill more info (properties) on the item based on its values
    FillItemDetails(pItem);
    pItem->SetProperty("IsAlbum", true);
    pItem->SetProperty("BoxeeDBAlbumId", pAlbum->m_iId);

    if ((pItem->m_strPath == "") || (pItem->GetLabel() == ""))
    {
      CLog::Log(LOGWARNING,"CBoxeeDatabaseDirectory::CreateAlbumItem - return FALSE since item [path=%s] or [label=%s] is EMPTY",pItem->m_strPath.c_str(),pItem->GetLabel().c_str());
      return false;
    }

    if (pAlbum->m_iVirtual == 1)
    {
      pItem->SetProperty("isplayable", false);
    }
    else
    {
      pItem->SetProperty("isplayable", true);
    }

    return true;
  }
  else
  {
    CLog::Log(LOGERROR, "CBoxeeDatabaseDirectory::CreateAlbumItem - Album detail is NULL in metadata");
    return false;
  }
}

bool CBoxeeDatabaseDirectory::CreateSeriesItem(const BXMetadata* pMetadata, CFileItem* pItem)
{

  BXSeries* pSeries = (BXSeries*)pMetadata->GetDetail(MEDIA_DETAIL_SERIES);
  if (pSeries)
  {

    CVideoInfoTag* pInfoTag = pItem->GetVideoInfoTag();
    pInfoTag->m_strTitle = pSeries->m_strTitle;
    pInfoTag->m_iYear = pSeries->m_iYear;

    pInfoTag->m_strGenre = pSeries->m_strGenre;
    pInfoTag->m_strPlot = pSeries->m_strDescription;

    CScraperUrl scraperUrl(pSeries->m_strCover);
    pInfoTag->m_strPictureURL = scraperUrl;
    // TODO: Add actors and other stuff
    pItem->SetLabel(pSeries->m_strTitle);
    pItem->SetThumbnailImage(pSeries->m_strCover);
    pItem->m_bIsFolder = true;
    pItem->m_strPath = pMetadata->GetPath();

    pItem->SetProperty("isvideo",true);

    pItem->SetProperty("istvshow",true);

    pItem->m_dateTime = CDateTime(pSeries->m_iRecentlyAired);
    pItem->SetProperty("datemodified", pMetadata->m_iDateModified);
    pItem->SetProperty("dateadded", pMetadata->m_iDateAdded);

    pItem->SetProperty("boxeeId", pSeries->m_strBoxeeId);

    // fill more info (properties) on the item based on its values
    FillItemDetails(pItem);
  }
  else
  {
    return false;
  }

  if ((pItem->m_strPath == "") || (pItem->GetLabel() == ""))
  {
    CLog::Log(LOGWARNING,"CBoxeeDatabaseDirectory::CreateSeriesItem - return FALSE since item [path=%s] or [label=%s] is EMPTY",pItem->m_strPath.c_str(),pItem->GetLabel().c_str());
    return false;
  }

  return true;
}

bool CBoxeeDatabaseDirectory::CreateFileItem(const BXMetadata* pMetadata, CFileItem* pItem)
{
  CStdString title = CUtil::GetTitleFromPath(pMetadata->GetPath(),true);
  pItem->m_strPath = pMetadata->GetPath();
  pItem->SetProperty("share", pMetadata->GetFolder());
  pItem->SetLabel(title);
  pItem->m_strTitle = title;
  pItem->m_dwSize = pMetadata->GetFileSize();
  pItem->m_dateTime = CDateTime(pMetadata->m_iDateAdded);

  return true;
}

bool CBoxeeDatabaseDirectory::CreateDirectoryItem(const BXMetadata* pMetadata, CFileItem* pItem)
{
  //CLog::Log(LOGDEBUG, "LIBRARY: Creating directory item for %s", pMetadata->GetPath().c_str());

  if (pMetadata->m_strTitle != "") 
  {
    pItem->SetLabel(pMetadata->m_strTitle);
  }
  else {
    pItem->SetLabel(pMetadata->GetFolder() == ""?CUtil::GetTitleFromPath(pMetadata->GetPath(),true):pMetadata->GetFolder());
  }

  pItem->m_bIsFolder = true;
  pItem->m_strPath = pMetadata->GetPath();

  pItem->m_dateTime = CDateTime(pMetadata->m_iDateModified);
  pItem->SetProperty("dateadded", pMetadata->m_iDateAdded);

  // If this is a share, set the isshare property
  if (pMetadata->m_bShare) 
  {
    pItem->SetProperty("isshare", 1);
  }

  // fill more info (properties) on the item based on its values
  FillItemDetails(pItem);

  if ((pItem->m_strPath == "") || (pItem->GetLabel() == ""))
  {
    CLog::Log(LOGWARNING,"CBoxeeDatabaseDirectory::CreateDirectoryItem - return FALSE since item [path=%s] or [label=%s] is EMPTY",pItem->m_strPath.c_str(),pItem->GetLabel().c_str());
    return false;
  }

  return true;

}

bool CBoxeeDatabaseDirectory::CreateSeasonItem(const BXMetadata* pMetadata, CFileItem* pItem)
{
  BXSeason* pSeason = (BXSeason*)pMetadata->GetDetail(MEDIA_DETAIL_SEASON);

  pItem->SetLabel(pMetadata->m_strTitle);
  pItem->m_bIsFolder = true;
  pItem->m_strPath = pMetadata->GetPath();

  //CLog::Log(LOGDEBUG, "LIBRARY: Video cover %s", pSeason->m_strCover.c_str());
  CScraperUrl scraperUrl(pSeason->m_strCover);
  pItem->SetThumbnailImage(scraperUrl.GetFirstThumb().m_url);

  pItem->SetProperty("isvideo",true);

  pItem->SetProperty("istvshow",true);
  pItem->SetProperty("isseason",true);

  // fill more info (properties) on the item based on its values
  FillItemDetails(pItem);

  if ((pItem->m_strPath == "") || (pItem->GetLabel() == ""))
  {
    CLog::Log(LOGWARNING,"CBoxeeDatabaseDirectory::CreateSeasonItem - return FALSE since item [path=%s] or [label=%s] is EMPTY",pItem->m_strPath.c_str(),pItem->GetLabel().c_str());
    return false;
  }

  return true;
}

void CBoxeeDatabaseDirectory::CreateFileItemList(std::vector<BXMetadata*> vecMetadataItems, CFileItemList &items)
{
  BXMetadata* pMetadata;
  for (unsigned int i = 0; i < vecMetadataItems.size(); i++)
  {
    pMetadata = vecMetadataItems[i];
    if (!pMetadata)
    {
      CLog::Log(LOGERROR,"CBoxeeDatabaseDirectory::CreateFileItemList - cannot create FileItem from BXMetadata. the metadata is NULL (bdbd)");
      continue;
    }

    std::string strType = pMetadata->GetType();

    CFileItemPtr pItem ( new CFileItem(pMetadata->GetPath(), false) );

    bool bCreated = false;

    if (strType == MEDIA_ITEM_TYPE_AUDIO)
    {
      bCreated = CreateAudioItem(pMetadata, pItem.get());
      pItem->SetProperty("ismusic", true);
    }
    else if (strType == MEDIA_ITEM_TYPE_VIDEO)
    {
      bCreated = CreateVideoItem(pMetadata, pItem.get());
      if (pItem->GetVideoInfoTag()->m_iEpisode > 0) // hack
        pItem->SetProperty("istvshow", true);
      else
        pItem->SetProperty("ismovie", true);
    }
    else if (strType == MEDIA_ITEM_TYPE_SERIES)
    {
      bCreated = CreateSeriesItem(pMetadata, pItem.get());
      pItem->SetProperty("istvshow", true);
      if (pItem->m_bIsFolder)
        pItem->SetProperty("istvshowfolder", true);
    }
    else if (strType == MEDIA_ITEM_TYPE_SEASON)
    {
      bCreated = CreateSeasonItem(pMetadata, pItem.get());
      pItem->SetProperty("istvshow",true);
      if (pItem->m_bIsFolder)
        pItem->SetProperty("istvshowfolder", true);
    }
    else if (strType == MEDIA_ITEM_TYPE_PICTURE)
    {
      bCreated = CreatePictureItem(pMetadata, pItem.get());
      pItem->SetProperty("ispicture", true);
    }
    else if (strType == MEDIA_ITEM_TYPE_ALBUM)
    {
      bCreated = CreateAlbumItem(pMetadata, pItem.get());
      pItem->SetProperty("isalbum", true);
      pItem->SetProperty("ismusic", true);
    }
    else if (strType == MEDIA_ITEM_TYPE_ARTIST)
    {
      bCreated = CreateArtistItem(pMetadata, pItem.get());
      pItem->SetProperty("isartist", true);
      pItem->SetProperty("ismusic", true);
    }
    else if (strType == MEDIA_ITEM_TYPE_DIR)
    {
      bCreated = CreateDirectoryItem(pMetadata, pItem.get());
    }
    else if (strType == MEDIA_ITEM_TYPE_FILE)
    {
      bCreated = CreateFileItem(pMetadata, pItem.get());
    }
    else
    {
      // TODO: Add other types here
      return;
    }

    //pItem->Dump();
    if (pItem && bCreated && pItem->m_strPath != "")
    {
      items.Add(pItem);
    }
  }
}

bool CBoxeeDatabaseDirectory::CreateShareFilter(const CStdString& strType, std::vector<std::string>& vecPathFilter, bool addSlash)
{
  VECSOURCES * pVecShares = g_settings.GetSourcesFromType(strType);

  if (pVecShares) 
  {
    vecPathFilter.clear();
    for (IVECSOURCES it = pVecShares->begin(); it != pVecShares->end(); it++)
    {
      if (!it->IsPrivate() && !CUtil::IsApp(it->strPath) && !CUtil::IsLastFM(it->strPath) && !CUtil::IsShoutCast(it->strPath) && g_application.IsPathAvailable(it->strPath))
      {
        CStdString strPath = _P(it->strPath);
        if (addSlash)
        {
          CUtil::AddSlashAtEnd(strPath);
        }
        vecPathFilter.push_back(strPath);
      }
    }
    return true;
  }
  return false;
}

void CBoxeeDatabaseDirectory::GetMusicGenres(std::vector<CStdString>& _vecGenres)
{
  BXMetadataEngine& MDE = BOXEE::Boxee::GetInstance().GetMetadataEngine();
  std::vector<std::string> vecGenres;
  MDE.GetMusicGenres(vecGenres);

  for (size_t i = 0; i < vecGenres.size(); i++)
  {
    _vecGenres.push_back(vecGenres[i]);
  }
}

bool CBoxeeDatabaseDirectory::Exists(const char* strPath)
{
  // NOT IMPLEMENTED
  return true;

}

}

