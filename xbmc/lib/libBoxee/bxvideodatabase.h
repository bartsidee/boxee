// Copyright © 2008 BOXEE. All rights reserved.
#ifndef BXVIDEODATABASE_H_
#define BXVIDEODATABASE_H_

#include "bxdatabase.h"
#include "../../utils/CriticalSection.h"
#include <set>

namespace dbiplus {
class Dataset;
}

namespace BOXEE
{

class BXMetadata;
class BXVideo;
class BXFolder;
class BXSeries;
class BXSeason;
class BXPath;
class BXVideoLink;

#define BXVIDEO_ALL_VIDEOS   1
#define BXVIDEO_MOVIES_ONLY  2
#define BXVIDEO_TVSHOWS_ONLY 3

#define BXVIDEO_REAL_VIDEOS 1
#define BXVIDEO_FEED_VIDEOS 2
#define BXVIDEO_REAL_AND_FEED_VIDEOS 3


class BXVideoDatabase : public BOXEE::BXDatabase
{
public:
	BXVideoDatabase();
	virtual ~BXVideoDatabase();
	
	
	bool GetMovies(std::vector<BXMetadata*> &vecMediaFiles, const std::string& strGenre, const std::string& strPrefix, const std::vector<std::string>& vecPathFilter, int iItemLimit, bool bLoadOnlyFirstTrailerLink = false);
	bool SearchMoviesByTitle(const std::string& strTitle, std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit);

	bool GetVideoParts(int iVideoId, std::vector<std::string> &vecVideoParts);
	
	bool GetLinks(const std::string& strBoxeeId, std::vector<BXPath>& vecLinks, const std::vector<std::string>& vecPathFilter);

	bool GetTvShows(std::vector<BXMetadata*> &vecMediaFiles, const std::string& strGenre, const std::string& strPrefix, const std::vector<std::string>& vecPathFilter, int iItemLimit);

	bool GetTvShowsGenres (std::set<std::string> &setGenres , const std::vector<std::string>& vecPathFilter);
	bool GetMoviesGenres (std::set<std::string> &setGenres , const std::vector<std::string>& vecPathFilter);
	bool SearchTvShowsByTitle(const std::string& strTitle, std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit);

	unsigned int GetLatestEpisodeReleaseDate(const std::string& strTvShowId, int iSeason = -1);
	bool GetEpisodes(const std::string& strTvShowBoxeeId, int iSeason, std::vector<BXMetadata*> &vecEpisodes, const std::vector<std::string>& vecPathFilter);

	int GetVideosByFolder(std::map<std::string, BXMetadata*> &mapMediaFiles, const std::string& _strFolderPath, bool bGetFiles = false);
	bool GetVideosByTitle(std::vector<BXMetadata*> &vecMediaFiles, const std::string& strTitle, const std::vector<std::string>& vecPathFilter, int iItemLimit);

	// Video files
	int AddVideoFile(const std::string& strPath, const std::string& strSharePath, int FolderId, int64_t size);
	int UpdateVideoFileStatus(const std::string& strPath, int iStatus, int idVideo=-1);
	int MarkNewFolderFilesAsProcessing(int FolderId);
	bool GetUnresolvedVideoFiles(std::vector<BXMetadata*> &vecUnresolvedVideos, const std::vector<std::string>& vecPathFilter, int iItemLimit);
	int GetShareUnresolvedVideoFilesCount(const std::string& iStrSharePath, int iStatus);
	int GetShareUnresolvedVideoFilesCountByFolder(int folderId, int iStatus);
	int GetUserUnresolvedVideoFilesCount(const std::string& iSharesList, int iStatus);
	bool AreVideoFilesBeingScanned(const std::string& iSharesList);
	int RemoveUnresolvedVideoByPath(const std::string& strPath);
	int RemoveUnresolvedVideoByFolder(const std::string& strFolder);
	int GetUnresolvedVideoFilesByFolder(std::map<std::string, BXMetadata*> &mapMediaFiles, int folderId);


	int AddVideo(const BXMetadata* pMetadata);
	int AddPart(int iVideoId, int iPartNumber, const std::string& strPath);
	int AddActor(std::string strName);
	int AddDirector(std::string strDirector);
	int AddActorToVideo(int iActorId, int iVideoId);
	int AddDirectorToVideo(int iDirectorId, int iVideoId);

	int RemoveFeedVideo(const BXMetadata* pMetadata);
	int GetDirectorIdByName(const std::string& strName);
  int GetActorIdByName(const std::string& strName);
	std::string GetDirectorById(int iDirectorId);
	std::string GetDirectorByVideoId(int iVideoId);
	std::string GetActorById(int iActorId);
	int GetSeriesById(int iSeriesId, BXSeries* pSeries);
	bool GetSeriesByBoxeeId(const std::string& strBoxeeId, BXSeries* pSeries);

	int GetSeriesByName(const std::string& strSeriesName, BXSeries* pSeries);
	//int GetVideoByIMDBId(const std::string& strId, BXMetadata* pMetadata);
	int GetVideoByBoxeeIdAndPath(const std::string& strId, const std::string& strPath, BXMetadata* pMetadata);
	int GetVideoByPath(const std::string& strPath, BXMetadata* pMetadata);
	int GetVideoById(int iId, BXMetadata* pMetadata);
	int RemoveVideoByPath(const std::string& strPath);
	int RemoveVideoByFolder(const std::string& strFolderPath);
	int RemoveSeries(int iSeriesId, int iSeason = -1);

	int AddVideoLink(const std::string& strBoxeeId , const BXVideoLink& videoLink);
	int RemoveVideoLinkByURL(const std::string& strURL);
	int RemoveVideoLinksByBoxeeId(const std::string& strBoxeeId);
	int GetVideoLinks(std::vector<BXVideoLink>& videoLinks, const std::string& strBoxeeId = "", const std::string& strBoxeeType = "" , int iCount = -1);

	int DropVideoByPath(const std::string& strPath);
	int DropVideoById(int iId);

  virtual bool UpdateTables() { return true; }

private:

  bool ConvertDatasetToUniqueSet(std::set<std::string>& output, dbiplus::Dataset* ds , const std::vector<std::string>& vecPathFilter);

  int AddSeries(const BXSeries* pSeries);
  int AddSeason(const BXSeason* pSeries);
  bool HasEpisodes(const std::string& strBoxeeId, int iSeason, const std::vector<std::string>& vecPathFilter);
  unsigned long GetLatestEpisodeDate(const std::string& strBoxeeId);

  bool CreateVideosFromDataset(dbiplus::Dataset* pDS, std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit , bool bLoadOnlyFirstTrailerLink = false);
  bool CreateVideoFromDataset(dbiplus::Dataset* pDS, BXMetadata* pMetadata, std::map<int, std::string>& mapDirectors, std::map<int, std::vector<std::string> >& mapActors);

	bool LoadVideoFromDataset(dbiplus::Dataset* pDSa, BXVideo* pVideo);
	bool LoadFullVideoFromDataset(dbiplus::Dataset* pDSa, BXVideo* pVideo);
	bool LoadSeriesFromDataset(dbiplus::Dataset* pDS, BXSeries* pSeries);

	int GetVideoByStringField(const std::string& strFieldName, const std::string& strFieldValue, BXMetadata* pMetadata);
	bool GetActorsByVideoId(int iVideoId, std::vector<std::string>& vecActors);
	
	void MapDirectorsByVideoId(std::map<int, std::string>& mapDirectors);
	void MapActorsByVideoId(std::map<int, std::vector<std::string> >& mapActors);

	static CCriticalSection m_lock;
};

}

#endif /*BXVIDEODATABASE_H_*/
