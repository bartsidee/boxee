// Copyright © 2008 BOXEE. All rights reserved.
#ifndef BXMETADATAENGINE_H_
#define BXMETADATAENGINE_H_

#include <string>
#include <vector>
#include <queue>
#include <set>

#include "bximetadataresolver.h"
#include "bxmediadatabase.h"
#include "bxbgprocess.h"
#include "StdString.h"

namespace BOXEE {

class BXIMetadataResolver;

class BXIAlbumResolver
{
public:
  virtual ~BXIAlbumResolver() {}
  virtual void ResolveAlbumMetadata(std::vector<BOXEE::BXAlbum*>& albums) = 0;
  virtual bool Stop()=0;
};

class BXMetadataScannerJob : public BXBGJob
{
public:
	BXMetadataScannerJob(BXFolder* pFolder);
	virtual ~BXMetadataScannerJob();
	virtual void DoWork();
	const BXFolder* GetFolder() const {return m_pFolder; }
	std::string GetFolderPath() const { return m_pFolder->GetPath(); }

private:
	BXFolder* m_pFolder;
};

class BXAlbumRescanJob : public BXMetadataScannerJob
{
public:
	BXAlbumRescanJob(BXFolder* pFolder);
	virtual void DoWork();
};

/**
 * Job for retreiving remote metadata for audio albums
 * 
 */
class BXAlbumResolverJob : public BXBGJob
{
public:
  BXAlbumResolverJob(const std::vector<BXAlbum*>& vecAlbums);
  virtual ~BXAlbumResolverJob();
  virtual void DoWork();
  
private:
  std::vector<BXAlbum*> m_vecAlbums;
};

/**
 * Metadata Engine class has two purposes:
 * 1. It acts as a facade for Boxee database related functions
 * 2. It implements the media file resolution mechanisms
 * 
 */


class BXMetadataEngine
{
public:
	BXMetadataEngine();
	virtual ~BXMetadataEngine();

	bool Start();
	bool Stop();
	void Pause();
	void Resume();
	bool Resolve(BXMetadataScannerJob* pJob);
	bool Reset();
	
 	bool Rescan(const std::string& pathToScan) { return m_AudioFolderProcessor.QueueFront(new BXAlbumRescanJob(new BXFolder(pathToScan))); }
	
	void ResolveAlbums();
	void ResolveAlbumMetadata(std::vector<BOXEE::BXAlbum*>& albums);

	void AddMetadataResolver(BXIMetadataResolver* pResolver);
	void AddAlbumResolver(BXIAlbumResolver* pResolver);
	
	bool UpdateIfModified(const std::string& strPath, int iModDate, bool &bModified); 
	
  int AddMediaFolder(const std::string& strPath, const std::string& strType, int iModTime);
  int GetMediaFolderId(const std::string& strPath, const std::string& strType);
  bool IsMediaFolderBeingResolved(const std::string& strPath);

  void MarkAsWatched(const std::string& strPath, const std::string& strBoxeeId, double iLastPosition);
  void MarkAsUnWatched(const std::string& strPath, const std::string& strBoxeeId);
  bool IsWatchedByPath(const std::string& strPath);
  bool IsWatchedById(const std::string& strBoxeeId);

  bool AddQueueItem(const std::string& strLabel, const std::string& strPath, const std::string& strThumbPath, const std::string& strClientId);
  bool GetQueueItem(const std::string& strClientId, std::string& strLabel, std::string& strPath, std::string& strThumbPath);
  bool RemoveQueueItem(const std::string& strClientId);

	int AddArtist(const BXArtist* pArtist);
	int AddAudio(const BXAudio* pAudio);
	int AddAlbum(const BXAlbum* pAlbum);

	int AddVideoFile(const std::string& strPath, const std::string& strSharePath, int FolderId, int64_t size);
	int UpdateVideoFileStatus(const std::string& strPath, int iStatus);
  int MarkNewFolderFilesAsProcessing(int folderId);
	bool GetUnresolvedVideoFiles(std::vector<BXMetadata*> &vecUnresolvedVideos, const std::vector<std::string>& vecPathFilter, int iItemLimit);
	bool GetShareUnresolvedVideoFilesCountByFolder(int folderId, int iStatus);
	int RemoveUnresolvedVideoByPath(const std::string& strPath);
	int RemoveUnresolvedVideoByFolder(const std::string& strPath);
	int GetUnresolvedVideoFilesByFolder(std::map<std::string, BXMetadata*> &mapMediaFiles, int folderId);

  int AddVideo(const BXMetadata* pVideo);

  int AddAudioFile(const std::string& strPath, const std::string& strSharePath, int FolderId, int64_t size);
  int UpdateAudioFileStatus(const std::string& strPath, int iStatus);
  int RemoveUnresolvedAudioByPath(const std::string& strPath);
  int RemoveUnresolvedAudioByFolder(const std::string& strFolder);
  int GetUnresolvedAudioFilesByFolder(std::map<std::string, BXMetadata*> &mapMediaFiles, int folderId);

	int AddPart(int iVideoId, int iPartNumber, const std::string& strPath);
	
	int UpdateAlbum(const BXAlbum* pAlbum);
	int UpdateAlbumStatus(int iId, int iStatus);
	
	int GetAudios(std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit);

	bool GetAlbums(std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit);
	bool SearchMusic(const std::string& strTitle, std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit);

	int GetArtists(std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit);
	
	bool GetLocalShowGenres (std::set<std::string>& output , const std::vector<std::string>& vecPathFilter);
	bool GetLocalMoviesGenres (std::set<std::string>& output , const std::vector<std::string>& vecPathFilter);

	int GetSongsFromAlbum(int iAlbumId, std::vector<BXMetadata*> &vecMediaFiles);
	int GetAlbumsByArtist(int iArtistId, std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter);
	int GetAlbumsByTitleAndArtist(const std::string& strTitle, int iArtistId, std::vector<BXMetadata*> &vecMediaFiles);
	int GetAlbumsByTitleAndArtist(const std::string& strTitle, const std::string& strArtist, std::vector<BXMetadata*> &vecMediaFiles);
	int GetAudioByPath(const std::string& strPath, BXMetadata* pMetadata);
	int GetVideoByPath(const std::string& strPath, BXMetadata* pMetadata);
	int GetAudiosByFolder(std::map<std::string, BXMetadata*> &mapMediaFiles, const std::string& strFolderPath);
	int GetAlbum(const std::string& strAlbum, const std::string& strArtist, BXMetadata* pMetadata);

	int GetAlbumByPath(const std::string& strPath, BXAlbum* pAlbum);
	int GetMusicGenres(std::vector<std::string>& vecGenres);
	
	bool GetAlbumById(int iAlbumId, BXAlbum* pAlbum);
	bool GetArtistById(int iArtistId, BXArtist* pArtist);

	bool GetTvShows(std::vector<BXMetadata*> &vecMediaFiles, const std::string& strGenre, const std::string& strPrefix, const std::vector<std::string>& vecPathFilter, int iItemLimit);
	bool GetMovies(std::vector<BXMetadata*> &vecMediaFiles, const std::string& strGenre, const std::string& strPrefix, const std::vector<std::string>& vecPathFilter, int iItemLimit);
	int GetVideosByTitle(std::vector<BXMetadata*> &vecMediaFiles, const std::string& strTitle, const std::vector<std::string>& vecPathFilter, int iItemLimit);
	int GetVideosByFolder(std::map<std::string, BXMetadata*> &mapMediaFiles, const std::string& strFolderPath, bool bGetFiles = false);
  int GetUnresolvedVideoFolders(std::vector<BXMetadata*> &vecVideoFolders, const std::vector<std::string>& vecPathFilter, int iItemLimit);
  int GetUnresolvedVideos(std::vector<BXMetadata*> &vecUnresolvedVideos, const std::vector<std::string>& vecPathFilter, int iItemLimit);

  bool GetLinks(const std::string& strBoxeeId, std::vector<BXPath>& vecLinks, const std::vector<std::string>& vecPathFilter);
	
	//int GetVideoByIMDBId(const std::string& strId, BXMetadata* pMetadata);
	//int GetVideoByBoxeeId(const std::string& strId, BXMetadata* pMetadata);
	
	// Get specific episode , in case bReal is true, return real (non feed) episode only
	//int GetEpisode(const std::string& strSeriesName, int iSeason, int iEpisode, BXMetadata* pMetadata, bool bReal);
	int GetSeriesByName(const std::string& strSeriesName, BXSeries* pSeries);
	//int GetSeasonsFromSeries(int iSeriesId, std::vector<BXMetadata*> &vecSeasons, const std::vector<std::string>& vecPathFilter);
	//int GetEpisodesFromSeason(int iSeriesId, int iSeasonId, std::vector<BXMetadata*> &vecSeasons, const std::vector<std::string>& vecPathFilter);
	bool GetEpisodes(const std::string& strTvShowBoxeeId, int iSeason, std::vector<BXMetadata*> &vecEpisodes, const std::vector<std::string>& vecPathFilter);
	//std::string GetTrailerUrl(const std::string& strTitle);

	
	int GetChildFolders(std::set<std::string> &setFolders, const std::string &strPath , bool getImmediate = true);
	int GetMediaFolders(std::vector<BXFolder*>& vecFolders, const std::string& strType = "");
	int GetUnresolvedFoldersByPath(const std::string& strPath, std::vector<BXMetadata*>& vecFolders);
	
	// Status changes
	int MarkFolderNew(const std::string& strFolderPath);
	int MarkFolderTreeNew(const std::string& strFolderPath);
	int MarkFolderUnresolved(const std::string& strFolderPath, bool bDecreaseRescan = true);
	int MarkFolderResolved(const std::string& strFolderPath, int iMetadataId);
	int MarkFoldersAvailable(const std::string& strFolderPath, bool bAvailable);
	
	int RemoveVideoByPath(const std::string& strPath);
	int RemoveAudioByPath(const std::string& strPath);
	int RemoveVideoByFolder(const std::string& strFolderPath);
	int RemoveAudioByFolder(const std::string& strFolderPath);
	int RemoveFolderByPath(const std::string& strPath);
	int RemoveSeries(int iSeriesId, int iSeason = -1);
	
	bool SearchTvShowsByTitle(const std::string& strTitle, std::vector<BXMetadata*> &vecTvShows, const std::vector<std::string>& vecPathFilter, int iItemLimit);
	bool SearchMoviesByTitle(const std::string& strTitle, std::vector<BXMetadata*> &vecMovies, const std::vector<std::string>& vecPathFilter, int iItemLimit);
	
	int DropVideoByPath(const std::string& strPath);
	int DropVideoById(int iId);
	
	// Marks album as dropped, which means that it was not recognized correctly 
	// and should not be displayed. The album is just marked and not completely removed
	// in order to prevent the system from trying to recognize it again
	int DropAlbumById(int iId);
	int RemoveAlbumById(int iId);

	unsigned int GetFolderDate(const std::string& strPath);
  	
	//int ResolveVideosByHash(std::map<std::string, std::pair<std::string, std::string> >& mapHashToName);
	
	bool IsStarted()
	{
	  return m_bStarted;
	}
	
  void InitializeArtistDataMap();
  void AddArtistToCache(const std::string& strArtist, BXArtist artist);
  bool FindArtistInCache(const std::string& strArtist, BXArtist& artist);
		
  bool AddMediaShare(const std::string& strLabel, const std::string& strPath, const std::string& strType, int iScanType);
  bool UpdateMediaShare(const std::string& strOrgLabel, const std::string& strOrgType, const std::string& strLabel, const std::string& strPath, const std::string& strType, int iScanType);
  bool DeleteMediaShare(const std::string& strLabel, const std::string& strPath, const std::string& strType);

  bool GetScanTime(const std::string& strLabel, const std::string& strPath, const std::string& strType, time_t& iLastScanned);
  bool UpdateScanTime(const std::string& strLabel, const std::string& strPath, const std::string& strType, time_t iLastScanned);
		
  int AddProviderPerf(const std::string& strProvider, int quality);
  int UpdateProviderPerf(const std::string& strProvider, int quality);
  int GetProviderPerfQuality(const std::string& strProvider);

  int AddPlayableFolder(const std::string& strPath);
  bool IsPlayableFolder(const std::string& strPath);

protected:

	static int EngineThread(void *pParam);
	static int ProcessFolders(const std::string& strType, BXBGProcess& processor);

	bool m_bStarted;
	bool m_bPaused;

private:

	// Vector of resolvers that are used by the engine
	std::vector<BXIMetadataResolver*> m_vecMetadataResolvers;
	
	BXIAlbumResolver* m_pAlbumResolver;

	BXBGProcess m_VideoFolderProcessor;
	BXBGProcess m_AudioFolderProcessor;
	BXBGProcess m_AlbumMetadataProcessor;

	SDL_Thread *m_thread;
	
  SDL_mutex  * m_ArtistCacheGuard;
  std::map<std::string, BXArtist> m_mapArtistDataCache;
};

}

#endif /*BXMETADATAENGINE_H_*/
