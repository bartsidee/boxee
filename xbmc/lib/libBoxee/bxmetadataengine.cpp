
#include "bxmetadataengine.h"
#include "boxee.h"
#include "bximetadataresolver.h"
#include "bxmediadatabase.h"
#include "bxuserprofiledatabase.h"
#include "bxpicturedatabase.h"
#include "bxvideodatabase.h"
#include "bxaudiodatabase.h"
#include "bxconfiguration.h"
#include "bxutils.h"
#include "bxdatabase.h"
#include "logger.h"
#include "time.h"
#include "../../MetadataResolverMusic.h"
#include "../../guilib/GUIMessage.h"
#include "../../guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"

using namespace std;

using namespace dbiplus;

// Specifies number of threads that will be used
// by the metadata scanner
#define NUM_VIDEO_SCANNER_THREADS 2
#define MAX_VIDEO_WORK_CAPACITY 100000

#define NUM_AUDIO_SCANNER_THREADS 1
#define MAX_AUDIO_WORK_CAPACITY 100000

#define NUM_ALBUM_RESOLVER_THREADS 1

namespace BOXEE {

BXMetadataEngine::BXMetadataEngine() : m_VideoFolderProcessor(MAX_VIDEO_WORK_CAPACITY), 
m_AudioFolderProcessor(MAX_AUDIO_WORK_CAPACITY),
m_AlbumMetadataProcessor(MAX_AUDIO_WORK_CAPACITY)

{
  m_bStarted = false;
  m_VideoFolderProcessor.SetName("Video Folder Processor");
  m_AudioFolderProcessor.SetName("Audio Folder Processor");
  m_AlbumMetadataProcessor.SetName("Album Metadata Processor");

  m_ArtistCacheGuard = SDL_CreateMutex();

}

BXMetadataEngine::~BXMetadataEngine()
{
  if (m_ArtistCacheGuard)
    SDL_DestroyMutex(m_ArtistCacheGuard);

  if (m_pAlbumResolver)
  {
    delete m_pAlbumResolver;
  }
}

void BXMetadataEngine::AddMetadataResolver(BXIMetadataResolver* pResolver) 
{
  m_vecMetadataResolvers.push_back(pResolver);
}

void BXMetadataEngine::AddAlbumResolver(BXIAlbumResolver* pResolver)
{
  m_pAlbumResolver = pResolver;
}

bool BXMetadataEngine::Start()
{
  if (m_bStarted)
  {
    return true;
  }

  if (BXConfiguration::GetInstance().GetIntParam("Boxee.MetadataEngine.Enabled",1) == 0)
  {
    LOG(LOG_LEVEL_DEBUG,"Boxee Metadata Engine, is disabled.... (resolver)");

    return false;
  }

  LOG(LOG_LEVEL_INFO,"Boxee Metadata Engine, is starting... (resolver)");

  bool result = false;

  m_bStarted = true;

  { // we need to put this in a block to avoid depleting the database pool
  BXMediaDatabase pDB;
  // Reset metadata if required
  if (BXConfiguration::GetInstance().GetIntParam("Boxee.ResetMetadata",0) == 1)
  {
    pDB.ResetMetadata();
  }

  // Reset all items in intermediate status, such as resolving or unresolved to new
  if (pDB.ResetUnresolvedFiles() != MEDIA_DATABASE_OK)
  {
    LOG(LOG_LEVEL_ERROR, "Boxee Metadata Engine, Could not reset unresolved media files");
  }
  }

  {
    BXAudioDatabase pAudioDB;
    pAudioDB.ResetAlbumStatus();
  }

  result &= m_VideoFolderProcessor.Start(NUM_VIDEO_SCANNER_THREADS);
  result &= m_AudioFolderProcessor.Start(NUM_AUDIO_SCANNER_THREADS);
  result &= m_AlbumMetadataProcessor.Start(NUM_ALBUM_RESOLVER_THREADS);

  // Create main engine thread
  m_thread = SDL_CreateThread(EngineThread, NULL);

  return result;
}

bool BXMetadataEngine::Stop()
{
  if (!m_bStarted)
    return true;

  CLog::Log(LOGNOTICE, "Boxee Metadata Engine, stopping....");

  // Go over all resolvers and stop them
  std::vector<BXIMetadataResolver*>::iterator it = m_vecMetadataResolvers.begin();
  while (it != m_vecMetadataResolvers.end())
  {
    (*it)->Stop();
    it++;
  }

  // Stop album resolver
  m_pAlbumResolver->Stop();

  m_bStarted = false;

  m_AudioFolderProcessor.SignalStop();
  m_VideoFolderProcessor.SignalStop();
  m_AlbumMetadataProcessor.SignalStop();

  CLog::Log(LOGNOTICE, "Stopping audio processor");
  m_AudioFolderProcessor.Stop();  

  CLog::Log(LOGNOTICE, "Stopping video processor");
  m_VideoFolderProcessor.Stop();

  m_AlbumMetadataProcessor.Stop();

  if (m_thread) {
    SDL_WaitThread(m_thread,NULL);
  }

  m_thread = NULL;
  LOG(LOG_LEVEL_DEBUG,"Boxee Metadata Engine, stoppped....");
  return true;
}

void BXMetadataEngine::Pause()
{
  LOG(LOG_LEVEL_DEBUG, "BXMetadataEngine::Pause, metadata engine paused (pause)");
  if (m_bPaused) return;
  m_bPaused = true;
  m_VideoFolderProcessor.Pause();
  m_AudioFolderProcessor.Pause();
  m_AlbumMetadataProcessor.Pause();
}

void BXMetadataEngine::Resume()
{
  LOG(LOG_LEVEL_DEBUG, "BXMetadataEngine::Resume, metadata engine resumed (pause)");
  if (!m_bPaused) return;
  m_bPaused = false;
  m_VideoFolderProcessor.Resume();
  m_AudioFolderProcessor.Resume();
  m_AlbumMetadataProcessor.Resume();
}

bool BXMetadataEngine::Reset()
{
  m_vecMetadataResolvers.clear();
  return true;
}

bool BXMetadataEngine::Resolve(BXMetadataScannerJob* pJob)
{
  // Get the folder that should be resolved from the job
  const BXFolder* pMediaFolder = pJob->GetFolder();

  if (!m_bStarted) {
    LOG(LOG_LEVEL_ERROR, "BXMetadataEngine::Resolve, Engine, not started (resolver)");
    BXMediaDatabase db;
    db.UpdateFolderAborted(pMediaFolder->GetPath());
    return false;
  }

  if (m_vecMetadataResolvers.size() == 0) {
    LOG(LOG_LEVEL_ERROR, "BXMetadataEngine::Resolve, Engine, has no registered resolvers (resolver)");
    BXMediaDatabase db;
    db.UpdateFolderAborted(pMediaFolder->GetPath());
    return false;
  }

  LOG(LOG_LEVEL_INFO, "BXMetadataEngine::Resolve, Resolving folder: %s (resolver)", pMediaFolder->GetPath().c_str());

  // the DataBase handle shouldn't be active the whole function since it hold a db connection in the pool, and
  // we might try to catch another handle in its sub functions.
  {
  BXMediaDatabase pDB;
  pDB.UpdateFolderResolving(pMediaFolder);
  }

  std::vector<std::vector<BXFolder*> > resolutions;
  int iResult;

  // Go over all resolvers and receive their resolutions
  // Each resolver may return a vector of results

  std::vector<BXIMetadataResolver*>::iterator it = m_vecMetadataResolvers.begin();

  bool bResult = true;

  while (it != m_vecMetadataResolvers.end() && m_bStarted)
  {
    std::vector<BXFolder*> vecResult;
    iResult = ((*it)->Resolve(pJob, vecResult));

    if (iResult == RESOLVER_SUCCESS)
    {
      resolutions.push_back(vecResult);
      bResult &= true;
    }
		else if (iResult == RESOLVER_ABORTED) 
		{
			BXMediaDatabase db;
			db.UpdateFolderAborted(pMediaFolder->GetPath());
			BXUtils::FreeFolderVec(vecResult);
			bResult &= true;
		}
    /*
		else if (iResult == RESOLVER_CANT_ACCESS)
		{
		  BXMediaDatabase db;
		  db.UpdateFolderAvailability(pMediaFolder->GetPath(), false);
		  BXUtils::FreeFolderVec(vecResult);
		  bResult &= false;
		}
     */
    else
    {
      BXUtils::FreeFolderVec(vecResult);
      bResult &= false;
    }

    it++;

  } // while on all resolvers

  if (!bResult)
  {
    BXMediaDatabase db;
    db.UpdateFolderUnresolved(pMediaFolder->GetPath());
  }

  //LOG(LOG_LEVEL_INFO, "BXMetadataEngine::Resolve, Finished resolving folder: %s (resolver)", pMediaFolder->GetPath().c_str());

  // Delete the results
  size_t nResolutions = resolutions.size();
  for (size_t i=0; i<nResolutions; i++)
  {
    BXUtils::FreeFolderVec(resolutions[i]);
  }
  resolutions.clear();

  return bResult;
}

int BXMetadataEngine::ProcessFolders(const std::string& strType, BXBGProcess& processor)
{
  std::vector<BXFolder*> vecFolders;

  BXMetadataEngine &engine  = Boxee::GetInstance().GetMetadataEngine();

  BXMediaDatabase pDB;
  int result = pDB.GetUnresolvedFolders(vecFolders, strType);

  if (result == MEDIA_DATABASE_OK)
  {
    //LOG(LOG_LEVEL_INFO,"BXMetadataEngine::ProcessFolders,  running over %d unresolved folders of type %s (resolver)", vecFolders.size(), strType.c_str());

    while (vecFolders.size() > 0 && engine.m_bStarted)
    {
      BXFolder* pFolder = vecFolders.front();
      vecFolders.erase(vecFolders.begin());

      std::string strPath = pFolder->GetPath();

      // Use the resolver to determine whether this folder can be accessed before queueing
      if (!engine.m_vecMetadataResolvers[0]->CheckPath(strPath))
      {
        //LOG(LOG_LEVEL_DEBUG,"BXMetadataEngine::ProcessFolders,  folder %s is not available, moving on (resolver)", strPath.c_str());
        delete pFolder;
        continue;
      }

      BXMetadataScannerJob* job = new BXMetadataScannerJob(pFolder);

      // This call will block if the processor is at maximum capacity
      // This ensures that the thread will not overload

      LOG(LOG_LEVEL_DEBUG,"BXMetadataEngine::ProcessFolders,  queueing job, processor = %s, path = %s (resolver)", processor.GetName().c_str(), pFolder->GetPath().c_str());
      if (!processor.QueueJob(job))
      {
        LOG(LOG_LEVEL_ERROR, "BXMetadataEngine::ProcessFolders, unable to queue job for folder, path = %s (resolver)", pFolder->GetPath().c_str());
        delete job;
      }
    }

    //LOG(LOG_LEVEL_INFO,"BXMetadataEngine::ProcessFolders,  finished resolving batch of %d unresolved folders of type %s", vecFolders.size(), strType.c_str());
  }

  // Erase all folders that are left in the vector
  for(size_t i = 0; i < vecFolders.size(); i++)
    delete vecFolders[i];
  vecFolders.clear();

  return result;
}

void BXMetadataEngine::ResolveAlbums()
{
  //Get all unresolved albums from the database (unresolved album is marked by value 0 in the iNumTracks field
  std::vector<BXAlbum*> vecUnresolvedAlbums;
  BXAudioDatabase pDB;

  int iResult = pDB.GetUnresolvedAlbums(vecUnresolvedAlbums);

  if (iResult == MEDIA_DATABASE_ERROR)
    return;

  for(size_t i = 0; i < vecUnresolvedAlbums.size(); i++)
  {
    std::vector<BXAlbum*> vecAlbumBatch;
    vecAlbumBatch.push_back(vecUnresolvedAlbums[i]);
    BXAlbumResolverJob* job = new BXAlbumResolverJob(vecAlbumBatch);
    if (!m_AlbumMetadataProcessor.QueueJob(job))
    {
      // If we were not able to queue the job, delete all albums in the batch
      for (size_t j = 0; j < vecAlbumBatch.size(); j++)
      {
        delete vecAlbumBatch[j];
      }
      vecAlbumBatch.clear();
      delete job;
    }
  }
}

void BXMetadataEngine::ResolveAlbumMetadata(std::vector<BOXEE::BXAlbum*>& albums)
{
  m_pAlbumResolver->ResolveAlbumMetadata(albums);
}

int BXMetadataEngine::EngineThread(void *pParam) 
{
#ifdef _WIN32
  ::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#endif

  LOG(LOG_LEVEL_DEBUG,"BXMetadataEngine::EngineThread,  Boxee Metadata Engine thread starting....(resolver)");

  int iMillisecondsToSleep = BXConfiguration::GetInstance().GetIntParam("Boxee.MetadataEngine.Scan.Interval", 60000);

  while (Boxee::GetInstance().GetMetadataEngine().m_bStarted)
  {
    LOG(LOG_LEVEL_DEBUG,"BXMetadataEngine::EngineThread,  processing folders....(resolver)");
    ProcessFolders(MEDIA_ITEM_TYPE_VIDEO_FOLDER, Boxee::GetInstance().GetMetadataEngine().m_VideoFolderProcessor);
    ProcessFolders(MEDIA_ITEM_TYPE_MUSIC_FOLDER, Boxee::GetInstance().GetMetadataEngine().m_AudioFolderProcessor);

    Boxee::GetInstance().GetMetadataEngine().ResolveAlbums();

    // picture folders are not scanned at this point

    int nWait = iMillisecondsToSleep ;
    // busy wait. to make sure we stop when stopped.
    while (nWait > 0 && Boxee::GetInstance().GetMetadataEngine().m_bStarted)
    {
      SDL_Delay(1000);
      nWait -= 1000;
    }
  } // while

  LOG(LOG_LEVEL_DEBUG,"BXMetadataEngine::EngineThread,  Engine, exiting...(resolver)");
  return 0;
}

BXMetadataScannerJob::BXMetadataScannerJob(BXFolder* pFolder) : BXBGJob("BXMetadataScannerJob")
{
  m_pFolder = pFolder;
}

BXMetadataScannerJob::~BXMetadataScannerJob()
{
  // The job is responsible for deleting the metadata after
  // storing all the data in the database
  delete m_pFolder;
}

void BXMetadataScannerJob::DoWork()
{
  LOG(LOG_LEVEL_DEBUG,"BXMetadataScannerJob::DoWork, started resolving: %s (resolver)", m_pFolder->GetPath().c_str());
  BXMetadataEngine &engine  = Boxee::GetInstance().GetMetadataEngine();

  engine.Resolve(this);
}

// ///////////////////////////////////////
// Album Rescan Job

BXAlbumRescanJob::BXAlbumRescanJob(BXFolder* pFolder) : BXMetadataScannerJob(pFolder)
{
}

void BXAlbumRescanJob::DoWork()
{
  std::string folderPath = GetFolderPath();
  CMetadataResolverMusic::ResolveMusicFolder(this, true);

  BXAlbum album;
  BOXEE::Boxee::GetInstance().GetMetadataEngine().GetAlbumByPath(folderPath,&album);

  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_FILE_RESCAN_COMPLETE,album.m_iId);
  msg.SetStringParam(folderPath);
  g_windowManager.SendThreadMessage(msg);

  CGUIMessage msg2(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
  g_windowManager.SendThreadMessage(msg2);
}

// ///////////////////////////////////////
// Album Resolver Job

BXAlbumResolverJob::BXAlbumResolverJob(const std::vector<BXAlbum*>& vecAlbums) : BXBGJob("BXAlbumResolverJob")
{
  m_vecAlbums = vecAlbums;
}

BXAlbumResolverJob::~BXAlbumResolverJob()
{
  for (size_t i=0; i<m_vecAlbums.size(); i++)
    delete m_vecAlbums[i];
  m_vecAlbums.clear();
}

void BXAlbumResolverJob::DoWork()
{
  LOG(LOG_LEVEL_DEBUG,"BXAlbumResolverJob::DoWork, resolving %d albums (musicresolver)", m_vecAlbums.size()); 
  Boxee::GetInstance().GetMetadataEngine().ResolveAlbumMetadata(m_vecAlbums);
}

// The following set of methods delegates to the media datbase

int BXMetadataEngine::GetAudios(std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit)
{
  BXAudioDatabase pDB;
  return pDB.GetAudios(vecMediaFiles, vecPathFilter, iItemLimit);
}

bool BXMetadataEngine::GetAlbums(std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit)
{
  BXAudioDatabase pDB;
  return pDB.GetAlbums(vecMediaFiles, vecPathFilter, iItemLimit);
}

bool BXMetadataEngine::SearchMusic(const std::string& strTitle, std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit)
{
  BXAudioDatabase pDB;
  return pDB.SearchMusic(strTitle, vecMediaFiles, vecPathFilter, iItemLimit);
}

int BXMetadataEngine::GetArtists(std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit)
{
  BXAudioDatabase pDB;
  return pDB.GetArtists(vecMediaFiles, vecPathFilter, iItemLimit);
}

int BXMetadataEngine::GetVideosByTitle(std::vector<BXMetadata*> &vecMediaFiles, const std::string& strTitle, const std::vector<std::string>& vecPathFilter, int iItemLimit)
{
  BXVideoDatabase pDB;
  return pDB.GetVideosByTitle(vecMediaFiles, strTitle, vecPathFilter, iItemLimit);
}

bool BXMetadataEngine::GetTvShows(std::vector<BXMetadata*> &vecMediaFiles, const std::string& strGenre, const std::string& strPrefix, const std::vector<std::string>& vecPathFilter, int iItemLimit)
{
  BXVideoDatabase pDB;
  return pDB.GetTvShows(vecMediaFiles, strGenre, strPrefix, vecPathFilter, iItemLimit);
}

bool BXMetadataEngine::GetLinks(const std::string& strBoxeeId, std::vector<BXPath> & vecLinks, const std::vector<std::string>& vecPathFilter)
{
  BXVideoDatabase pDB;
  return pDB.GetLinks(strBoxeeId, vecLinks, vecPathFilter);
}

bool BXMetadataEngine::SearchTvShowsByTitle(const std::string& strTitle, std::vector<BXMetadata*> &vecTvShows, const std::vector<std::string>& vecPathFilter, int iItemLimit)
{
  BXVideoDatabase pDB;
  return pDB.SearchTvShowsByTitle(strTitle, vecTvShows, vecPathFilter, iItemLimit);
}

bool BXMetadataEngine::GetMovies(std::vector<BXMetadata*> &vecMediaFiles, const std::string& strGenre, const std::string& strPrefix, const std::vector<std::string>& vecPathFilter, int iItemLimit)
{
  BXVideoDatabase pDB;
  return pDB.GetMovies(vecMediaFiles, strGenre, strPrefix, vecPathFilter, iItemLimit);
}

bool BXMetadataEngine::SearchMoviesByTitle(const std::string& strTitle, std::vector<BXMetadata*> &vecMovies, const std::vector<std::string>& vecPathFilter, int iItemLimit)
{
  BXVideoDatabase pDB;
  return pDB.SearchMoviesByTitle(strTitle, vecMovies, vecPathFilter, iItemLimit);
}

int BXMetadataEngine::GetUnresolvedVideoFolders(std::vector<BXMetadata*> &vecVideoFolders, const std::vector<std::string>& vecPathFilter, int iItemLimit)
{
  BXMediaDatabase pDB;
  return pDB.GetUnresolvedVideoFolders(vecVideoFolders, vecPathFilter, iItemLimit);
}

int BXMetadataEngine::GetSongsFromAlbum(int iAlbumId, std::vector<BXMetadata*> &vecMediaFiles)
{
  BXAudioDatabase pDB;
  return pDB.GetSongsFromAlbum(iAlbumId, vecMediaFiles);
}

int BXMetadataEngine::GetAlbumsByArtist(int iArtistId, std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter)
{
  BXAudioDatabase pDB;
  return pDB.GetAlbumsByArtist(iArtistId, vecMediaFiles, vecPathFilter);
}

int BXMetadataEngine::GetAlbumsByTitleAndArtist(const std::string& strTitle, int iArtistId, std::vector<BXMetadata*> &vecMediaFiles)
{
  BXAudioDatabase pDB;
  return pDB.GetAlbumsByTitleAndArtist(strTitle, iArtistId, vecMediaFiles);
}

int BXMetadataEngine::GetAlbumsByTitleAndArtist(const std::string& strTitle, const std::string& strArtist, std::vector<BXMetadata*> &vecMediaFiles)
{
  BXAudioDatabase pDB;
  return pDB.GetAlbumsByTitleAndArtist(strTitle, strArtist, vecMediaFiles);
}

//int BXMetadataEngine::GetEpisodesFromSeason(int iSeriesId, int iSeasonId, std::vector<BXMetadata*> &vecSeasons, const std::vector<std::string>& vecPathFilter)
//{
//  BXVideoDatabase pDB;
//  return pDB.GetEpisodesFromSeason(iSeriesId, iSeasonId, vecSeasons, vecPathFilter);
//}

bool BXMetadataEngine::GetEpisodes(const std::string& strTvShowBoxeeId, int iSeason, std::vector<BXMetadata*> &vecEpisodes, const std::vector<std::string>& vecPathFilter)
{
  BXVideoDatabase pDB;
  return pDB.GetEpisodes(strTvShowBoxeeId, iSeason, vecEpisodes, vecPathFilter);
}

//int BXMetadataEngine::GetSeasonsFromSeries(int iSeriesId, std::vector<BXMetadata*> &vecSeasons, const std::vector<std::string>& vecPathFilter)
//{
//  BXVideoDatabase pDB;
//  return pDB.GetSeasonsFromSeries(iSeriesId, vecSeasons, vecPathFilter);
//}

int BXMetadataEngine::GetAlbum(const std::string& strAlbum, const std::string& strArtist, BXMetadata* pMetadata)
{
  BXAudioDatabase pDB;
  return pDB.GetAlbum(strAlbum, strArtist, pMetadata);
}

bool BXMetadataEngine::GetAlbumById(int iAlbumId, BXAlbum* pAlbum)
{
  BXAudioDatabase pDB;
  return pDB.GetAlbumById(iAlbumId, pAlbum);
}

bool BXMetadataEngine::GetArtistById(int iArtistId, BXArtist* pArtist)
{
  BXAudioDatabase pDB;
  return pDB.GetArtistById(iArtistId, pArtist);
}

int BXMetadataEngine::GetAlbumByPath(const std::string& strPath, BXAlbum* pAlbum)
{
  BXAudioDatabase pDB;
  return pDB.GetAlbumByPath(strPath, pAlbum);
}

int BXMetadataEngine::AddAlbum(const BXAlbum* pAlbum)
{
  BXAudioDatabase pDB;
  return pDB.AddAlbum(pAlbum);
}

int BXMetadataEngine::UpdateAlbum(const BXAlbum* pAlbum)
{
  BXAudioDatabase pDB;
  return pDB.UpdateAlbum(pAlbum);
}

int BXMetadataEngine::UpdateAlbumStatus(int iId, int iStatus)
{
  BXAudioDatabase pDB;
  return pDB.UpdateAlbumStatus(iId, iStatus);
}

int BXMetadataEngine::AddArtist(const BXArtist* pArtist)
{
  BXAudioDatabase pDB;
  return pDB.AddArtist(pArtist);
}

int BXMetadataEngine::AddAudio(const BXAudio* pAudio)
{
  BXAudioDatabase pDB;
  return pDB.AddAudio(pAudio);
}

/*
int BXMetadataEngine::GetVideoByIMDBId(const std::string& strId, BXMetadata* pMetadata) 
{
  BXVideoDatabase pDB;
  return pDB.GetVideoByIMDBId(strId, pMetadata);
}

int BXMetadataEngine::GetVideoByBoxeeId(const std::string& strId, BXMetadata* pMetadata)
{
  BXVideoDatabase pDB;
  return pDB.GetVideoByBoxeeId(strId, pMetadata);
}
*/

int BXMetadataEngine::GetSeriesByName(const std::string& strSeriesName, BXSeries* pSeries) 
{
  BXVideoDatabase pDB;
  return pDB.GetSeriesByName(strSeriesName, pSeries);
}

//int BXMetadataEngine::GetEpisode(const std::string& strSeriesName, int iSeason, int iEpisode, BXMetadata* pMetadata, bool bReal)
//{
//  BXVideoDatabase pDB;
//  return pDB.GetEpisode(strSeriesName, iSeason, iEpisode, pMetadata, bReal);
//}

int BXMetadataEngine::AddVideoFile(const std::string& strPath, const std::string& strSharePath, int FolderId, int64_t size)
{
  BXVideoDatabase pDB;
  return pDB.AddVideoFile(strPath, strSharePath, FolderId, size);
}

int BXMetadataEngine::UpdateVideoFileStatus(const std::string& strPath, int iStatus)
{
  BXVideoDatabase pDB;
  return pDB.UpdateVideoFileStatus(strPath, iStatus);
}

int BXMetadataEngine::MarkNewFolderFilesAsProcessing(int folderId)
{
  BXVideoDatabase pDB;
  return pDB.MarkNewFolderFilesAsProcessing(folderId);
}

bool BXMetadataEngine::GetUnresolvedVideoFiles(std::vector<BXMetadata*> &vecUnresolvedVideos, const std::vector<std::string>& vecPathFilter, int iItemLimit)
{
  BXVideoDatabase pDB;
  return pDB.GetUnresolvedVideoFiles(vecUnresolvedVideos, vecPathFilter, iItemLimit);
}

bool BXMetadataEngine::GetShareUnresolvedVideoFilesCountByFolder(int folderId, int iStatus)
{
  BXVideoDatabase pDB;
  return pDB.GetShareUnresolvedVideoFilesCountByFolder(folderId, iStatus);
}

int BXMetadataEngine::RemoveUnresolvedVideoByPath(const std::string& strPath)
{
  BXVideoDatabase pDB;
  return pDB.RemoveUnresolvedVideoByPath(strPath);
}
int BXMetadataEngine::RemoveUnresolvedVideoByFolder(const std::string& strFolder)
{
  BXVideoDatabase pDB;
  return pDB.RemoveUnresolvedVideoByFolder(strFolder);
}

int BXMetadataEngine::AddVideo(const BXMetadata* pMetadata)
{
  BXVideoDatabase pDB;
  return pDB.AddVideo(pMetadata);
}

int BXMetadataEngine::AddAudioFile(const std::string& strPath, const std::string& strSharePath, int FolderId, int64_t size)
{
  BXAudioDatabase pDB;
  return pDB.AddAudioFile(strPath, strSharePath, FolderId, size);
}

int BXMetadataEngine::UpdateAudioFileStatus(const std::string& strPath, int iStatus)
{
  BXAudioDatabase pDB;
  return pDB.UpdateAudioFileStatus(strPath, iStatus);
}
int BXMetadataEngine::RemoveUnresolvedAudioByPath(const std::string& strPath)
{
  BXAudioDatabase pDB;
  return pDB.RemoveUnresolvedAudioByPath(strPath);
}
int BXMetadataEngine::RemoveUnresolvedAudioByFolder(const std::string& strFolder)
{
  BXAudioDatabase pDB;
  return pDB.RemoveUnresolvedAudioByFolder(strFolder);
}

int BXMetadataEngine::AddPart(int iVideoId, int iPartNumber, const std::string& strPath)
{
  BXVideoDatabase pDB;
  return pDB.AddPart(iVideoId, iPartNumber, strPath);
}

//std::string BXMetadataEngine::GetTrailerUrl(const std::string& strTitle)
//{
//  BXVideoDatabase pDB;
//  return pDB.GetTrailerUrl(strTitle);
//}

int BXMetadataEngine::GetAudioByPath(const std::string& strPath, BXMetadata* pMetadata)
{
  BXAudioDatabase pDB;
  return pDB.GetAudioByPath(strPath, pMetadata);
}

bool BXMetadataEngine::GetLocalShowGenres (std::set<std::string>& output , const std::vector<std::string>& vecPathFilter)
{
  BXVideoDatabase pDB;
  return pDB.GetTvShowsGenres(output, vecPathFilter);
}

bool BXMetadataEngine::GetLocalMoviesGenres (std::set<std::string>& output , const std::vector<std::string>& vecPathFilter)
{
  BXVideoDatabase pDB;
  return pDB.GetMoviesGenres(output, vecPathFilter);
}

int BXMetadataEngine::GetVideoByPath(const std::string& strPath, BXMetadata* pMetadata)
{
  BXVideoDatabase pDB;
  return pDB.GetVideoByPath(strPath, pMetadata);
}

int BXMetadataEngine::GetUnresolvedVideoFilesByFolder(std::map<std::string, BXMetadata*> &mapMediaFiles, int folderId)
{
  BXVideoDatabase pDB;
  return pDB.GetUnresolvedVideoFilesByFolder(mapMediaFiles, folderId);
}

int BXMetadataEngine::AddMediaFolder(const std::string& strPath, const std::string& strType, int iModTime)
{
  BXMediaDatabase pDB;
  return pDB.AddMediaFolder(strPath, strType, iModTime);
}

bool BXMetadataEngine::IsMediaFolderBeingResolved(const std::string& strPath)
{
  BXMediaDatabase pDB;
  return pDB.IsFolderBeingResolved(strPath);
}

int BXMetadataEngine::GetMediaFolderId(const std::string& strPath, const std::string& strType)
{
  BXMediaDatabase pDB;
  return pDB.GetMediaFolderId(strPath, strType);
}

bool BXMetadataEngine::UpdateIfModified(const std::string& strPath, int iModDate, bool &bModified) 
{
  BXMediaDatabase pDB;
  return pDB.UpdateIfModified(strPath, iModDate, bModified);
}

int BXMetadataEngine::GetVideosByFolder(std::map<std::string, BXMetadata*> &mapMediaFiles, const std::string& strFolderPath, bool bGetFiles)
{
  BXVideoDatabase pDB;
  return pDB.GetVideosByFolder(mapMediaFiles, strFolderPath, bGetFiles);
}

int BXMetadataEngine::RemoveVideoByPath(const std::string& strPath) 
{
  BXVideoDatabase pDB;
  return pDB.RemoveVideoByPath(strPath);
}

int BXMetadataEngine::GetAudiosByFolder(std::map<std::string, BXMetadata*> &mapMediaFiles, const std::string& strFolderPath)
{
  BXAudioDatabase pDB;
  return pDB.GetAudiosByFolder(mapMediaFiles, strFolderPath);
}
int BXMetadataEngine::GetUnresolvedAudioFilesByFolder(std::map<std::string, BXMetadata*> &mapMediaFiles, int folderId)
{
  BXAudioDatabase pDB;
  return pDB.GetUnresolvedAudioFilesByFolder(mapMediaFiles, folderId);
}

int BXMetadataEngine::RemoveAudioByPath(const std::string& strPath) 
{
  BXAudioDatabase pDB;
  return pDB.RemoveAudioByPath(strPath);
}

int BXMetadataEngine::RemoveFolderByPath(const std::string& strPath)
{
  BXMediaDatabase pDB;
  return pDB.RemoveFolderByPath(strPath);
}

int BXMetadataEngine::RemoveSeries(int iSeriesId, int iSeason) 
{
  BXVideoDatabase pDB;
  return pDB.RemoveSeries(iSeriesId, iSeason);
}

int BXMetadataEngine::GetChildFolders(std::set<std::string> &setFolders, const std::string &strPath, bool getImmediate)
{
  BXMediaDatabase pDB;
  return pDB.GetChildFolders(setFolders, strPath, getImmediate);
}

int BXMetadataEngine::RemoveVideoByFolder(const std::string& strFolderPath)
{
  BXVideoDatabase pDB;
  return pDB.RemoveVideoByFolder(strFolderPath);
}

int BXMetadataEngine::RemoveAudioByFolder(const std::string& strFolderPath)
{
  BXAudioDatabase pDB;
  return pDB.RemoveAudioByFolder(strFolderPath);
}


int BXMetadataEngine::GetMediaFolders(std::vector<BXFolder*>& vecFolders, const std::string& strType) 
{
  BXMediaDatabase pDB;
  return pDB.GetMediaFolders(vecFolders, strType);
}

int BXMetadataEngine::GetUnresolvedFoldersByPath(const std::string& strPath, std::vector<BXMetadata*>& vecFolders)
{
  BXMediaDatabase pDB;
  return pDB.GetUnresolvedFoldersByPath(strPath, vecFolders);
}

int BXMetadataEngine::MarkFolderNew(const std::string& strFolderPath) 
{
  BXMediaDatabase pDB;
  return pDB.UpdateFolderNew(strFolderPath);
}

int BXMetadataEngine::MarkFolderTreeNew(const std::string& strFolderPath)
{
  BXMediaDatabase pDB;
  return pDB.UpdateFolderTreeNew(strFolderPath);
}

int BXMetadataEngine::MarkFolderUnresolved(const std::string& strFolderPath, bool bDecreaseRescan/* = true*/)
{
  BXMediaDatabase pDB;
  return pDB.UpdateFolderUnresolved(strFolderPath,bDecreaseRescan);
}

int BXMetadataEngine::MarkFoldersAvailable(const std::string& strFolderPath, bool bAvailable)
{
  BXMediaDatabase pDB;
  return pDB.UpdateFolderAvailability(strFolderPath, bAvailable);
}

int BXMetadataEngine::MarkFolderResolved(const std::string& strFolderPath, int iMetadataId) 
{
  BXMediaDatabase pDB;
  return pDB.UpdateFolderResolved(strFolderPath, iMetadataId);
}

int BXMetadataEngine::DropVideoByPath(const std::string& strPath)
{
  BXVideoDatabase pDB;
  return pDB.DropVideoByPath(strPath);
}

int BXMetadataEngine::DropVideoById(int iId)
{
  BXVideoDatabase pDB;
  return pDB.DropVideoById(iId);
}

int BXMetadataEngine::DropAlbumById(int iId)
{
  BXAudioDatabase pDB;
  return pDB.DropAlbumById(iId);
}

int BXMetadataEngine::RemoveAlbumById(int iId)
{
  BXAudioDatabase pDB;
  return pDB.RemoveAlbumById(iId);
}

unsigned int BXMetadataEngine::GetFolderDate(const std::string& strPath)
{
  BXMediaDatabase pDB;
  return pDB.GetFolderDate(strPath);
}

void BXMetadataEngine::InitializeArtistDataMap()
{
  SDL_LockMutex(m_ArtistCacheGuard);
  BXAudioDatabase pDB;
  pDB.GetAllArtists(m_mapArtistDataCache);
  SDL_UnlockMutex(m_ArtistCacheGuard);
}

void BXMetadataEngine::AddArtistToCache(const std::string& strArtist, BXArtist artist)
{
  SDL_LockMutex(m_ArtistCacheGuard);
  m_mapArtistDataCache[strArtist] = artist;
  SDL_UnlockMutex(m_ArtistCacheGuard);
}

bool BXMetadataEngine::FindArtistInCache(const std::string& strArtist, BXArtist& artist)
{
  SDL_LockMutex(m_ArtistCacheGuard);
  std::map<std::string, BXArtist>::iterator it = m_mapArtistDataCache.find(strArtist);

  if (it != m_mapArtistDataCache.end())
  {
    artist.m_strPortrait = it->second.m_strPortrait;
    SDL_UnlockMutex(m_ArtistCacheGuard);
    return true;
  }

  SDL_UnlockMutex(m_ArtistCacheGuard);
  return false;
}

int BXMetadataEngine::GetMusicGenres(std::vector<std::string>& vecGenres)
{
  BXAudioDatabase pDB;
  pDB.GetMusicGenres(vecGenres);
  return (int)vecGenres.size();
}

void BXMetadataEngine::MarkAsWatched(const std::string& strPath, const std::string& strBoxeeId, double iLastPosition)
{
  BXUserProfileDatabase pDB;
  pDB.MarkAsWatched(strPath, strBoxeeId, iLastPosition);
}

void BXMetadataEngine::MarkAsUnWatched(const std::string& strPath, const std::string& strBoxeeId)
{
  BXUserProfileDatabase pDB;
  pDB.MarkAsUnWatched(strPath, strBoxeeId);
}

bool BXMetadataEngine::IsWatchedByPath(const std::string& strPath)
{
  BXUserProfileDatabase pDB;
  return pDB.IsWatchedByPath(strPath);
}

bool BXMetadataEngine::IsWatchedById(const std::string& strPath)
{
  BXUserProfileDatabase pDB;
  return pDB.IsWatchedById(strPath);
}

bool BXMetadataEngine::AddQueueItem(const std::string& strLabel, const std::string& strPath, const std::string& strThumbPath, const std::string& strClientId)
{
  BXMediaDatabase pDB;
  return pDB.AddQueueItem(strLabel, strPath, strThumbPath, strClientId);
}

bool BXMetadataEngine::GetQueueItem(const std::string& strClientId, std::string& strLabel, std::string& strPath, std::string& strThumbPath)
{
  BXMediaDatabase pDB;
  return pDB.GetQueueItem(strClientId, strLabel, strPath, strThumbPath);
}

bool BXMetadataEngine::RemoveQueueItem(const std::string& strClientId)
{
  BXMediaDatabase pDB;
  return pDB.RemoveQueueItem(strClientId);
}

bool BXMetadataEngine::AddMediaShare(const std::string& strLabel, const std::string& strPath, const std::string& strType, int iScanType)
{
  BXMediaDatabase pDB;
  return pDB.AddMediaShare(strLabel, strPath, strType, iScanType);
}

bool BXMetadataEngine::UpdateMediaShare(const std::string& strOrgLabel, const std::string& strOrgType, const std::string& strLabel, const std::string& strPath, const std::string& strType, int iScanType)
{
  BXMediaDatabase pDB;
  return pDB.UpdateMediaShare(strOrgLabel, strOrgType, strLabel, strPath, strType, iScanType);
}

bool BXMetadataEngine::DeleteMediaShare(const std::string& strLabel, const std::string& strPath, const std::string& strType)
{
  BXMediaDatabase pDB;
  return pDB.DeleteMediaShare(strLabel, strPath, strType);
}

bool BXMetadataEngine::GetScanTime(const std::string& strLabel, const std::string& strPath, const std::string& strType, time_t& iLastScanned)
{
  BXMediaDatabase pDB;
  return pDB.GetScanTime(strLabel, strPath, strType, iLastScanned);
}

bool BXMetadataEngine::UpdateScanTime(const std::string& strLabel, const std::string& strPath, const std::string& strType, time_t iLastScanned)
{
  BXMediaDatabase pDB;
  return pDB.UpdateScanTime(strLabel, strPath, strType, iLastScanned);
}
int BXMetadataEngine::AddProviderPerf(const std::string& strProvider, int quality)
{
  BXMediaDatabase pDB;
  return pDB.AddProviderPerf(strProvider, quality);
}
int BXMetadataEngine::UpdateProviderPerf(const std::string& strProvider, int quality)
{
  BXMediaDatabase pDB;
  return pDB.UpdateProviderPerf(strProvider, quality);
}
int BXMetadataEngine::GetProviderPerfQuality(const std::string& strProvider)
{
  BXMediaDatabase pDB;
  return pDB.GetProviderPerfQuality(strProvider);

}

int BXMetadataEngine::AddPlayableFolder(const std::string& strPath)
{
  BXMediaDatabase pDB;
  return pDB.AddPlayableFolder(strPath);
}

bool BXMetadataEngine::IsPlayableFolder(const std::string& strPath)
{
  BXMediaDatabase pDB;
  return pDB.IsPlayableFolder(strPath);
}

}
