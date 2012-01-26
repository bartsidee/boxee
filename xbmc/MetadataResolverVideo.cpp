
#include "ScraperSettings.h"
#include "MetadataResolverVideo.h"
#include "MetadataResolver.h"
#include "VideoInfoTag.h"
#include "VideoInfoScanner.h"
#include "utils/IMDB.h"
#include "VideoDatabase.h"
#include "VideoInfoTag.h"
#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "Util.h"
#include "utils/log.h"
#include "bxutils.h"
#include "utils/RegExp.h"
#include "utils/fstrcmp.h"
#include "BoxeeUtils.h"
#include "Application.h"
#include "FileSystem/Directory.h"
#include "FileSystem/StackDirectory.h"
#include "FileSystem/File.h"
#include "RssSourceManager.h"
#include "FileSystem/BoxeeDatabaseDirectory.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "BoxeeBrowseMenuManager.h"

using namespace VIDEO;
using namespace BOXEE;
using namespace DIRECTORY;
using namespace XFILE;
using namespace MUSIC_INFO;
using namespace MUSIC_GRABBER;
using namespace std;

std::vector<std::string> CMetadataResolverVideo::m_badWords;
std::vector<CStdString> CMetadataResolverVideo::m_commonWords;
std::set<std::string> CMetadataResolverVideo::m_localMoviesGenres;
std::set<std::string> CMetadataResolverVideo::m_localShowsGenres;

SDL_mutex* CMetadataResolverVideo::m_localMoviesGenresGuard = NULL;
SDL_mutex* CMetadataResolverVideo::m_localShowsGenresGuard = NULL;

bool CMetadataResolverVideo::m_bInitialized = false;

void CVideoFolderContext::Load()
{
  CUtil::AddSlashAtEnd(strPath);
  strEffectiveParentPath = BXUtils::GetEffectiveFolderPath(strPath);
  strName = BXUtils::GetFolderName(strEffectiveParentPath);

  strName = CMetadataResolverVideo::RemoveYear(strName, strYear);

  if (!CMetadataResolverVideo::ExtractSeriesTag(strName, iSeason, iEpisode, strName, strAdditionalInfo))
  {
    // in case that we couldnt resolve the Series by tag we will try to extract  it as a daily show
    // without remove the year
    strName = BXUtils::GetFolderName(strEffectiveParentPath);
    if (!CMetadataResolverVideo::ExtractDailyShows(strName, iSeason, strEpisodeDate,strName))
    {
      // couldnt resolve by year - assume it move and remove the year again
      strName = CMetadataResolverVideo::RemoveYear(strName, strYear);
    }
  }

  strName = CMetadataResolverVideo::CleanName(strName);
  strAdditionalInfo = CMetadataResolverVideo::CleanName(strAdditionalInfo);
}

void CVideoFileContext::Load(const CStdString& _strPath)
{
  if (!_strPath.IsEmpty())
  {
    strPath = _strPath;
  }

  LoadNameFromPath();
  LoadFileData();
  LoadFileHash();
  LoadFileNfo();
}

void CVideoFileContext::LoadWithTag( const CVideoInfoTag* vTag )
{
  if( vTag )
  {
    LoadNameFromTag( vTag );
  }
  else
  {
    LoadNameFromPath();
  }
  LoadFileData();
  LoadFileHash();
  LoadFileNfo();
}

void CVideoFileContext::LoadNameFromTag( const CVideoInfoTag* vTag )
{
  // This is the UPnP resolve path. In our hopes and dreams we get a vtag which contains metadata
  // from the server. In reality most UPnP servers are sending just the filename without a path,
  // and the vtag is useless.
  strPath = vTag->m_strTitle;
  LoadNameFromPath();

  // If we didn't parse out useful info from the title, then try the vtag
  if( vTag->m_iYear && strYear == "" )
    strYear = vTag->m_iYear;
  
  if( vTag->m_iSeason != -1 && iSeason == -1)
  {
    bMovie = false;
    iSeason = vTag->m_iSeason;
  }

  if( vTag->m_iEpisode != -1 && iEpisode == -1 )
  {
    bMovie = false;
    iEpisode = vTag->m_iEpisode;
  }
}

void CVideoFileContext::LoadNameFromPath()
{
  strName = CUtil::GetFileName(strPath);
  CUtil::RemoveExtension(strName);

  strName = CMetadataResolverVideo::RemoveYear(strName, strYear);

  CStdString strPrefix ;
  bMovie = !CMetadataResolverVideo::ExtractPathSeriesTag(strPath, strName, iSeason, iEpisode, strPrefix, strAdditionalInfo);

  if (bMovie)
  {
    strName = CUtil::GetFileName(strPath);
    // try to extract series by date
    bMovie = !CMetadataResolverVideo::ExtractDailyShows(strName, iSeason, strEpisodeDate, strPrefix);

    if (bMovie)
    {
      // couldnt resolve by year - assume it move and remove the year again
      strName = CMetadataResolverVideo::RemoveYear(strName, strYear);
    }
  }

  strName = CMetadataResolverVideo::CleanName(strPrefix);
  strAdditionalInfo = CMetadataResolverVideo::CleanName(strAdditionalInfo);
}

void CVideoFileContext::LoadFileData()
{
  if (!CMetadataResolverVideo::GetFileData(strPath.c_str(), iSize, iModificationDate))
  {
    iSize = 0;
    iModificationDate = 0;
  }

  char size[17];
  memset(size,0,17);
  snprintf(size, 17, "%"PRId64, iSize);
  strSize = size;
}

void CVideoFileContext::LoadFileHash()
{
  // Calculate video hash
  uint64_t uiHash = CMetadataResolverVideo::CalculateVideoHash(strPath);

  // Convert hash to string
  char hash[17];
  memset(hash,0,17);
  snprintf(hash, 17, "%016"PRIx64, uiHash);
  strHash = hash;
}

bool CVideoFileContext::LoadVideoInfoTag(const std::vector<CStdString>& strFilePaths, CVideoInfoTag& nfoDetails)
{
  bool foundNFO = false;

  for (int i=0; i<(int)strFilePaths.size(); i++)
  {
    CStdString nfoFile = strFilePaths[i];

    CLog::Log(LOGDEBUG,"CVideoFileContext::LoadVideoInfoTag - handle [%s] file details (nfo)",nfoFile.c_str());

    if (CFile::Exists(nfoFile))
    {
      TiXmlDocument doc;

      if (!doc.LoadFile(nfoFile.c_str()))
      {
        CLog::Log(LOGERROR,"CVideoFileContext::LoadVideoInfoTag - NFO ERROR - Failed to parse [%s] file (nfo)", nfoFile.c_str());
        m_bNFOParseFailed = true;
        return false;
      }

      //nfo file loaded successfully, load file content
      if (nfoDetails.Load(doc.RootElement()))
      {
        CLog::Log(LOGDEBUG,"CVideoFileContext::LoadVideoInfoTag - load [%s] file details. (nfo)",nfoFile.c_str());

        m_strNFOPath = nfoFile;
        m_bNFOParseFailed = false;

        if (doc.FirstChildElement("movie") != NULL)
        {
          if (!bMovie)
          {
            //if it was a tvshow and the .nfo says its a movie, reset the season and episode and treat it as a movie
            iSeason = -1;
            iEpisode = -1;
            CLog::Log(LOGDEBUG,"CVideoFileContext::LoadVideoInfoTag, discovered .nfo defined for a movie, treating %s as a movie. (nfo)",nfoFile.c_str());
            bMovie = true;
            bMovieChanged = true;
          }
        }
        else
        {
          //meaning it is can be either a tvshow or episode
          if (bMovie)
          {
            //if it was discovered as a movie, switch it to be episode
            CLog::Log(LOGDEBUG,"CVideoFileContext::LoadVideoInfoTag, discovered .nfo defined for a tvshow/episode, treating %s as tvshow/episode. (nfo)",nfoFile.c_str());
            bMovie = false;
            bMovieChanged = true;
          }
        }

        struct __stat64 stat;
        CFile::Stat(nfoFile, &stat);

        nfoDetails.m_strNFOPath = nfoFile;
        nfoDetails.m_iNfoAccessTime = stat.st_atime;
        nfoDetails.m_iNfoModifiedTime = stat.st_mtime;

        foundNFO = true;
        break;
      }
    }
    else
    {
      CLog::Log(LOGDEBUG,"CVideoFileContext::LoadVideoInfoTag - file [%s] doesn't exist (nfo)",nfoFile.c_str());
    }
  }

  return foundNFO;
}

void CVideoFileContext::LoadFileNfo()
{
  std::vector<CStdString> nfoFilePathsVec;
  GetNfoFilePaths(nfoFilePathsVec); //get the possible video.nfo paths

  if (LoadVideoInfoTag(nfoFilePathsVec, m_nfoVideoDetails)) //load the movie or episode info
  {
    //mark that we have succeeded
    m_bVideoDetailsFromNFO = true;
  }

  if (!bMovie)
  {
    CStdString strTVShowPath = strPath;
    //load the tvshow.nfo as well, searching for up to 3 levels up
    std::vector<CStdString> tvShowNFOFilePath;

    CUtil::GetRecursiveParentPath(strTVShowPath,tvShowNFOFilePath,3);

    //add tvshow.nfo at the end of each path as the filename
    for (unsigned int i = 0 ; i < tvShowNFOFilePath.size() ; i++)
    {
      if (!CUtil::HasSlashAtEnd(tvShowNFOFilePath[i]))
        CUtil::AddSlashAtEnd(tvShowNFOFilePath[i]);

      tvShowNFOFilePath[i] += "tvshow.nfo";
    }

    if (LoadVideoInfoTag(tvShowNFOFilePath,m_nfoShowDetails))
    {
      //mark that we have succeeded
      m_bShowDetailsFromNFO = true;

      //load the adjacent folder.jpg or movie.tbn if exists
      CStdString strSeriesRootFolder;
      CUtil::GetParentPath(m_nfoShowDetails.m_strNFOPath, strSeriesRootFolder);

      CFileItemList pathItems;
      CDirectory::GetDirectory(strSeriesRootFolder, pathItems);

      for (int i=0; i<pathItems.Size(); i++)
      {
        CFileItemPtr pItem = pathItems.Get(i);

        if (!pItem->m_bIsFolder && !pItem->IsHidden() && pItem->IsPicture())
        {
          CStdString picFileStr = CUtil::GetFileName(pItem->m_strPath);

          if ((stricmp(picFileStr.c_str(),"folder.jpg") == 0) || (stricmp(picFileStr.c_str(),"movie.tbn") == 0))
          {
            CScraperUrl::SUrlEntry url;
            url.m_url = pItem->m_strPath;
            url.m_type = CScraperUrl::URL_TYPE_GENERAL;
            m_nfoShowDetails.m_strPictureURL.m_url.insert(m_nfoShowDetails.m_strPictureURL.m_url.begin(),url);
            break;
          }
        }
      }

      //if we found tvshow.nfo, the user wants us to resolve it as a TV Show.
      return;
    }
  }

  //try finding the id in the old way
  if (m_bNFOParseFailed)
  {
    for (int i=0; i<(int)nfoFilePathsVec.size(); i++)
    {
      CStdString nfoFile = nfoFilePathsVec[i];

      // if we didn't manage to open the file or the IMDB id was not found there, search the string inside the file (old way) - only for movies
      XFILE::CFile file;
      if (file.Exists(nfoFile) && file.Open(nfoFile))
      {
        char contents[8192];
        int bytes = file.Read(contents, sizeof(contents) - 1);
        contents[bytes] = '\0';
        file.Close();

        static CRegExp* reg = NULL;
        if (reg == NULL)
        {
          CRegExp* myReg = new CRegExp();
          if (myReg->RegComp("(tt[0-9]+)"))
          {
            reg = myReg;
          }
          else
          {
            delete myReg;
          }
        }

        if (reg != NULL)
        {
          if (reg->RegFind(contents) >= 0)
          {
            m_nfoVideoDetails.m_strIMDBNumber = reg->GetMatch(1);
            m_bVideoDetailsFromNFO = true;
            bMovie = true;
            bMovieChanged = true;
            iEpisode = -1;
            iSeason = -1;
            m_bNFOParseFailed = false;
            CLog::Log(LOGDEBUG,"CVideoFileContext::LoadFileNfo - found [imdbId=%s] in [nfoPath=%s] for [path=%s] (nfo)",strIMDBId.c_str(), nfoFile.c_str(), strPath.c_str());
            return;
          }
        }
      }
    }
  }
}

bool CVideoFileContext::GetNfoFilePaths(std::vector<CStdString>& nfoFilePathsVec)
{
  CStdString nfoFile;
  CStdString parentFolderPath;

  CLog::Log(LOGDEBUG, "CVideoFileContext::GetNfoFilePaths - handle [%s] (nfo)",strPath.c_str());

  if (bMovie && m_playableFolderType != CPlayableFolderType::PF_NO)
  {
    //if its a movie playable folder
    CStdString fileName = CUtil::GetFileName(strPath);

    nfoFile = strPath; //folder path without '/'
    CUtil::AddSlashAtEnd(nfoFile);

    CStdString playableFolderNfo = nfoFile + "movie.nfo"; // folderpath/movie.nfo

    //add folderpath/filename.nfo at the same folder
    nfoFile += fileName;
    nfoFile += ".nfo";

    CLog::Log(LOGDEBUG, "CVideoFileContext::GetNfoFilePaths - [PLAYABLE-FOLDER] - add possible nfo file [path=%s] (nfo)",nfoFile.c_str());
    nfoFilePathsVec.push_back(nfoFile);

    CLog::Log(LOGDEBUG, "CVideoFileContext::GetNfoFilePaths - [PLAYABLE-FOLDER] - add possible nfo file [path=%s] (nfo)",playableFolderNfo.c_str());
    nfoFilePathsVec.push_back(playableFolderNfo);
  }
  else
  {
    // handle movie or episode
    CStdString filePath;

    if (!CUtil::IsInArchive(strPath))
    {
      filePath = strPath;
      CUtil::GetParentPath(filePath,parentFolderPath);
    }
    else
    {
      CStdString tmpFilePath;
      CUtil::GetDirectory(strPath,tmpFilePath);
      CUtil::GetParentPath(tmpFilePath,parentFolderPath);
      filePath = parentFolderPath;
      filePath += CUtil::GetFileName(strPath);
    }

    //same file path just change the extension.
    CUtil::ReplaceExtension(filePath, ".nfo", nfoFile);

    CLog::Log(LOGDEBUG, "CVideoFileContext::GetNfoFilePaths - [IsMovie=%d] - add possible nfo file [path=%s] (nfo)",bMovie,nfoFile.c_str());
    nfoFilePathsVec.push_back(nfoFile);

    //if (bMovie)
    {
      //take the file path and look for movie.nfo
      nfoFile = parentFolderPath;
      nfoFile += "movie.nfo";

      CLog::Log(LOGDEBUG, "CVideoFileContext::GetNfoFilePaths - [IsMovie=%d] - add possible nfo file [path=%s] (nfo)",bMovie,nfoFile.c_str());
      nfoFilePathsVec.push_back(nfoFile);
    }
  }

  return true;
}

void CVideoFileContext::GetFileOptionsMap(std::map<CStdString, CStdString>& mapOptions)
{
  // Set common options
  if (m_bIsManualResolve && !strExternalIdName.IsEmpty() && !strExternalIdValue.IsEmpty())
  {
    mapOptions[strExternalIdName] = strExternalIdValue;
  }
  else
  {
    if (!strBoxeeId.IsEmpty())
    {
      mapOptions["id"] = strBoxeeId;
    }

    if (!strPath.IsEmpty())
    {
      CStdString filename;
      CURI url(strPath);

      filename = url.GetWithoutUserDetails();

      if (!filename.IsEmpty())
      {
        if (filename.Find("upnp://",0) == 0)
        {
          CUtil::UrlDecode(filename);
        }
        CStdString strFilenameParam = BXUtils::URLEncode(filename);
        mapOptions["filename"] = strFilenameParam;
      }
    }

    if (!strName.IsEmpty())
    {
      CStdString strNameParam = BXUtils::URLEncode(strName);

      mapOptions["name"] = strNameParam;
    }

    //override if there is a value read from the nfo
    if (bMovie)
    {
      if (m_bVideoDetailsFromNFO && m_nfoVideoDetails.m_iYear > 0)
      {
        strYear = BXUtils::IntToString(m_nfoVideoDetails.m_iYear);
      }
    }
    else
    {
      if (m_bShowDetailsFromNFO && m_nfoShowDetails.m_iYear > 0)
      {
        strYear = BXUtils::IntToString(m_nfoShowDetails.m_iYear);
      }
    }
  /*
    if (m_bVideoDetailsFromNFO && !m_nfoVideoDetails.m_strFirstAired.IsEmpty())
    {
      strEpisodeDate = m_nfoVideoDetails.m_strFirstAired;
    }
  */
    if (!strYear.IsEmpty() && strEpisodeDate.IsEmpty())
    {
      mapOptions["year"] = strYear;
    }

    if (bUseHash && !strHash.IsEmpty() && (BXConfiguration::GetInstance().GetIntParam("Boxee.Resolver.UseHash", 1) == 1))
    {
      CStdString hashParamValue = strHash;
      hashParamValue += ":";
      hashParamValue += strSize;
      CUtil::URLEncode(hashParamValue);
      mapOptions["h"] = hashParamValue;
    }

    if (!m_nfoVideoDetails.m_strIMDBNumber.IsEmpty())
    {
      if (bMovie)
      {
        mapOptions["imdb_id"] = m_nfoVideoDetails.m_strIMDBNumber;
      }
      else if (m_bVideoDetailsFromNFO)
      {
        mapOptions["tvdb_episode_id"] = m_nfoVideoDetails.m_strIMDBNumber;
      }
    }

    if (m_bShowDetailsFromNFO)
    {
      if (!m_nfoShowDetails.m_strTitle.IsEmpty())
      {
        CStdString strNameParam = BXUtils::URLEncode(m_nfoShowDetails.m_strTitle);
        mapOptions["nfo_title"] = strNameParam;
      }

      if (!m_nfoShowDetails.m_strIMDBNumber.IsEmpty())
      {
        mapOptions["tvdb_series_id"] = m_nfoShowDetails.m_strIMDBNumber;
      }
    }
  }

  if (bMovie)
  {

  }
  else
  {
    // the tvshow have date - so we assume that it doesn't have episode and season numbers

    if (!strEpisodeDate.IsEmpty())
    {
      mapOptions["episode_date"] = strEpisodeDate;
    }
    else
    {
      if (m_bVideoDetailsFromNFO && m_nfoVideoDetails.m_iSeason >= 0 && m_nfoVideoDetails.m_iEpisode >= 0)
      {
        iSeason = m_nfoVideoDetails.m_iSeason;
        iEpisode =  m_nfoVideoDetails.m_iEpisode;
      }

      if (iSeason >= 0)
      {
        CStdString strSeasonParam;
        strSeasonParam.Format("%d", iSeason);
        mapOptions["season"] = strSeasonParam;
      }

      if (iEpisode > 0)
      {
        CStdString strEpisodeParam;
        strEpisodeParam.Format("%d", iEpisode);
        mapOptions["episode"] = strEpisodeParam;
      }
    }
    // Set show_info parameter to get the info of the tv show
    mapOptions["show_info"] = "true";
  }
}

void CVideoFileContext::GetFolderOptionsMap(std::map<CStdString, CStdString>& mapOptions)
{
  GetFileOptionsMap(mapOptions);

  if (!folderContext.strName.IsEmpty())
  {
    CStdString strNameParam = folderContext.strName;
    CUtil::URLEncode(strNameParam);
    mapOptions["name"] = strNameParam;
  }

  if (!folderContext.strYear.IsEmpty())
  {
    mapOptions["year"] = folderContext.strYear;
  }

  if (folderContext.iSeason > 0 && folderContext.iEpisode > 0)
  {
    CStdString strSeasonParam;
    strSeasonParam.Format("%d", folderContext.iSeason);
    mapOptions["season"] = strSeasonParam;

    CStdString strEpisodeParam;
    strEpisodeParam.Format("%d", folderContext.iEpisode);
    mapOptions["episode"] = strEpisodeParam;
  }
}

bool CVideoFileContext::CanResolveByFile()
{
  if (!strName.IsEmpty() || !strHash.IsEmpty() || !strIMDBId.IsEmpty())
    return true;

  return false;
}

bool CVideoFileContext::CanResolveByFolder()
{
  if (folderContext.strName.IsEmpty() || CUtil::IsUPnP(folderContext.strPath))
    return false;

  return true;
}

bool CMetadataResolverVideo::ResolveVideoSendServerReq(CVideoFileContext& context, bool file , CFileItemList& list)
{
  CLog::Log(LOGDEBUG, "CMetadataResolverVideo::ResolveVideoSendServerReq - resolving video by [%s][path=%s]  (mrv)(resolver)",file ? "file" : "folder", context.strPath.c_str());

  CStdString strBoxeeServerUrl = BXConfiguration::GetInstance().GetStringParam("Boxee.Resolver.Server","http://res.boxee.tv");
  CStdString strLink;

  std::map<CStdString, CStdString> mapOptions;

  if (file)
    context.GetFileOptionsMap(mapOptions);
  else
    context.GetFolderOptionsMap(mapOptions);

  strLink = strBoxeeServerUrl;
  if (context.bMovie)
  {
    strLink += "/title/movie/";
  }
  else
  {
    strLink += "/title/tv/";
  }

  strLink += BoxeeUtils::BuildParameterString(mapOptions);

  CLog::Log(LOGDEBUG, "CMetadataResolverVideo::ResolveVideoSendServerReq - resolving [%s][path=%s][name=%s][link=%s] (mrv)(resolver)",context.bMovie ? "movie" : "tvshow", context.strPath.c_str(), context.strName.c_str(), strLink.c_str());

  CRssFeed feed;
  feed.Init(strLink,strLink);
  feed.ReadFeed();
  feed.GetItemList(list);

  if (list.Size() == 0)
  {
    CLog::Log(LOGDEBUG,"CMetadataResolverVideo::ResolveVideoSendServerReq, resolving against server failed. (mrv)(resolver)");
    return false;
  }

  // check the items list - we might request for a movie, and get a tv result vice versa
  CFileItemPtr item = list.Get(0);

  if (item->GetPropertyBOOL("isepisode") || item->GetPropertyBOOL("istvshow") )
  {
    if (context.bMovie)
    {
      CLog::Log(LOGDEBUG,"CMetadataResolverVideo::ResolveVideoSendServerReq - according to Server replies update type to tvshow. [path=%s][name=%s] (mrv)",context.strPath.c_str(), context.strName.c_str());
    }
    context.bMovie = false;
  }
  else if (item->GetPropertyBOOL("ismovie"))
  {
    if (context.bMovie)
    {
      CLog::Log(LOGDEBUG,"CMetadataResolverVideo::ResolveVideoSendServerReq - according to Server replies update type to movie. [path=%s][name=%s] (mrv)",context.strPath.c_str(), context.strName.c_str());
    }
    context.bMovie = true;
  }

  return true;
}

/*
 * The function receives a fully loaded video context, performs resolving against boxee server
 * and returns the BXMetadata object holding the resolved metadata
 */
bool CMetadataResolverVideo::ResolveVideo(CVideoFileContext& context, BXMetadata* pMetadata)
{
  CLog::Log(LOGDEBUG, "CMetadataResolverVideo::ResolveVideo - resolving video. [path=%s] (mrv)(resolver)", context.strPath.c_str());

  bool resolved = false;
  CFileItemList resultList;

  bool bNeedToResolve = true;

  if ((context.bMovie && context.m_bVideoDetailsFromNFO && context.m_nfoVideoDetails.m_strIMDBNumber.IsEmpty()) ||
      (!context.bMovie && context.m_bShowDetailsFromNFO && context.m_nfoShowDetails.m_strIMDBNumber.IsEmpty()))
  {
    //if we have an nfo and there's no imdb/tvdb id, then do not resolve using the server, use the .nfo data
    bNeedToResolve = false;
  }

  if (bNeedToResolve && context.CanResolveByFile())
  {
    resolved = ResolveVideoSendServerReq(context, true, resultList);
  }

  if (!resolved)
  {
    CLog::Log(LOGDEBUG, "CMetadataResolverVideo::ResolveVideo - going to try to resolve with NFO, if exists.. [bMovie=%d][VideoNfo=%d][ShowNfo=%d][path=%s]. (mrv)(resolver)", context.bMovie, context.m_bVideoDetailsFromNFO, context.m_bShowDetailsFromNFO , context.strPath.c_str());
  }

  bool result;
  if (context.bMovie)
  {
    result = LoadMovieInfo(resultList, context, pMetadata);
  }
  else
  {
    result = LoadEpisodeInfo(resultList, context, pMetadata);
  }

  CLog::Log(LOGDEBUG, "CMetadataResolverVideo::ResolveVideo - %s to resolve %s. [path=%s] (mrv)(resolver)", result?"SUCCEEDED":"FAILED", context.bMovie?"Movie":"TVShow", context.strPath.c_str());

  return result;
}

bool CMetadataResolverVideo::InitializeGenres()
{
  //lock both mutexes
  std::vector<std::string> vecPathFilter;
  if (DIRECTORY::CBoxeeDatabaseDirectory::CreateShareFilter("video", vecPathFilter))
  {
    BXMetadataEngine& MDE = BOXEE::Boxee::GetInstance().GetMetadataEngine();

    MDE.GetLocalShowGenres(m_localShowsGenres, vecPathFilter);  //get the genres from the database
    MDE.GetLocalMoviesGenres(m_localMoviesGenres, vecPathFilter);  //get the genres from the database
  }

  return true;
}

void CMetadataResolverVideo::LockLocalShowsGenres()
{
  SDL_LockMutex(m_localShowsGenresGuard);
}

void CMetadataResolverVideo::UnlockLocalShowsGenres()
{
  SDL_UnlockMutex(m_localShowsGenresGuard);
}

void CMetadataResolverVideo::LockLocalMoviesGenres()
{
  SDL_LockMutex(m_localMoviesGenresGuard);
}

void CMetadataResolverVideo::UnlockLocalMoviesGenres()
{
  SDL_UnlockMutex(m_localMoviesGenresGuard);
}

bool CMetadataResolverVideo::AddLocalMoviesGenres(const std::string& strGenre)
{
  bool newItemInserted = false;

  if (!strGenre.empty())
  {
    std::vector<std::string> vecGenres = BXUtils::StringTokenize(strGenre,",");

    LockLocalMoviesGenres();

    for (std::vector<std::string>::iterator it= vecGenres.begin() ; it != vecGenres.end() ; it++)
    {
      std::string lowercasedGenre = BXUtils::StringToLower(*it);
      BXUtils::StringTrim(lowercasedGenre);

      bool bIsNewItem = m_localMoviesGenres.insert(lowercasedGenre).second;

      if (bIsNewItem)
      {
        CLog::Log(LOGDEBUG, "CMetadataResolverVideo::AddLocalShowGenre, new genre was added to m_localMoviesGenres, [strGenre=%s] (local genres)",lowercasedGenre.c_str());
      }

      newItemInserted |= bIsNewItem;
    }

    UnlockLocalMoviesGenres();

    if (newItemInserted)
    {
      //if its something new, update the menu
      CBoxeeBrowseMenuManager::GetInstance().ClearDynamicMenuButtons("mn_local_movies_genres");
    }
  }
  return newItemInserted;
}

bool CMetadataResolverVideo::AddLocalShowsGenres(const std::string& strGenre)
{
  bool newItemInserted = false;

  if (!strGenre.empty())
  {
    std::vector<std::string> vecGenres = BXUtils::StringTokenize(strGenre,",");

    LockLocalShowsGenres();

    for (std::vector<std::string>::iterator it= vecGenres.begin() ; it != vecGenres.end() ; it++)
    {
      std::string lowercasedGenre = BXUtils::StringToLower(*it);
      BXUtils::StringTrim(lowercasedGenre);

      bool bIsNewItem = m_localShowsGenres.insert(lowercasedGenre).second;

      if (bIsNewItem)
      {
        CLog::Log(LOGDEBUG, "CMetadataResolverVideo::AddLocalShowGenre, new genre was added to m_localShowsGenres, [strGenre=%s] (local genres)",lowercasedGenre.c_str());
      }

      newItemInserted |= bIsNewItem;
    }

    UnlockLocalShowsGenres();

    if (newItemInserted)
    {
      //if its something new, update the menu
      CBoxeeBrowseMenuManager::GetInstance().ClearDynamicMenuButtons("mn_local_shows_genres");
    }
  }

  return newItemInserted;
}

void CMetadataResolverVideo::GetLocalShowsGenres(std::set<std::string>& outShowGenres)
{
  LockLocalShowsGenres();

  outShowGenres = m_localShowsGenres;

  UnlockLocalShowsGenres();
}

void CMetadataResolverVideo::GetLocalMoviesGenres(std::set<std::string>& outMovieGenres)
{
  LockLocalMoviesGenres();

  outMovieGenres = m_localMoviesGenres;

  UnlockLocalMoviesGenres();
}

void CMetadataResolverVideo::DeinitializeVideoResolver()
{
  SDL_DestroyMutex(m_localMoviesGenresGuard);
  SDL_DestroyMutex(m_localShowsGenresGuard);
}

void CMetadataResolverVideo::InitializeVideoResolver()
{
  if (m_bInitialized)
    return;

  m_bInitialized = true;

  m_localMoviesGenresGuard = SDL_CreateMutex();
  m_localShowsGenresGuard = SDL_CreateMutex();

  InitializeGenres();

  BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetBadWords(m_badWords);

  // NOTE: All words should be in lower case
  if (m_badWords.size() == 0)
  {
    m_badWords.push_back("s[0-9][0-9]e[0-9][0-9]");
    m_badWords.push_back("h[0-9]+[pw]?");
    m_badWords.push_back("[0-9]+x[0-9]+");
    m_badWords.push_back("xvid");
    m_badWords.push_back("720p");
    m_badWords.push_back("480p");
    m_badWords.push_back("1080p");
    m_badWords.push_back("1080i");
    m_badWords.push_back("sharereactor");
    m_badWords.push_back("webrip");
    m_badWords.push_back("dvd[0-9]*");
    m_badWords.push_back("hdtv");
    m_badWords.push_back("dsr");
    m_badWords.push_back("pdtv");
    m_badWords.push_back("dvdscr");
    m_badWords.push_back("^ts$");
    m_badWords.push_back("5.1");
    m_badWords.push_back("director'?s *cut");
    m_badWords.push_back("aerox");
    m_badWords.push_back("loki");
    m_badWords.push_back("ac3");
    m_badWords.push_back("dd5");
    m_badWords.push_back("dts");
    m_badWords.push_back("esir");
    m_badWords.push_back("x264");
    m_badWords.push_back("hddvd");
    m_badWords.push_back("bluray");
    m_badWords.push_back("sample");
    m_badWords.push_back("cd1");
    m_badWords.push_back("cd2");
    m_badWords.push_back("divx5pro");
    m_badWords.push_back("internal");
    m_badWords.push_back("dvdr");
    m_badWords.push_back("divx");
    m_badWords.push_back("retro");
    m_badWords.push_back("darkside_rg");
    m_badWords.push_back("dvdrip");
    m_badWords.push_back("^cam$");
    m_badWords.push_back("axxo");
    m_badWords.push_back("fxg");
    m_badWords.push_back("fxm");
    m_badWords.push_back("klaxxon");
    m_badWords.push_back("leetay");
    m_badWords.push_back("pukka");
    m_badWords.push_back("edition");
    m_badWords.push_back("unrated");
    m_badWords.push_back("readnfo");
    m_badWords.push_back("fs");
    m_badWords.push_back("tpz");
    m_badWords.push_back("part1");
    m_badWords.push_back("part2");
    m_badWords.push_back("eyetv");
    m_badWords.push_back("tnan");
    m_badWords.push_back("ctrlhd");
    m_badWords.push_back("arigold");
    m_badWords.push_back("haideaf");
    m_badWords.push_back("^immerse$");
    m_badWords.push_back("cbgb");
    m_badWords.push_back("dtheather");
    m_badWords.push_back("^metis$");
    m_badWords.push_back("h264");
    m_badWords.push_back("aac");
    m_badWords.push_back("^aaf$");
    m_badWords.push_back("whiizz");
    m_badWords.push_back("bdrip");
    m_badWords.push_back("^amc$");
    m_badWords.push_back("bestdivx");
    m_badWords.push_back("brip");
    m_badWords.push_back("brrip");
    m_badWords.push_back("cinefile");
    m_badWords.push_back("duqa");
    m_badWords.push_back("^dvb$");
    m_badWords.push_back("^eng$");
    m_badWords.push_back("eztv");
    m_badWords.push_back("fqm");
    m_badWords.push_back("^fs$");
    m_badWords.push_back("264");
    m_badWords.push_back("hardengsub");
    m_badWords.push_back("ltt");
    m_badWords.push_back("^md$");
    m_badWords.push_back("movieextreme");
    m_badWords.push_back("multisub");
    m_badWords.push_back("mp3");
    m_badWords.push_back("nedivx");
    m_badWords.push_back("newartriot");
    m_badWords.push_back("orenji");
    m_badWords.push_back("part[0-9]+");
    m_badWords.push_back("psp");
    m_badWords.push_back("blueladyrg");
    m_badWords.push_back("secretmyth");
    m_badWords.push_back("sharereactor");
    m_badWords.push_back("^tc$");
    m_badWords.push_back("^ts$");
    m_badWords.push_back("telesync");
    m_badWords.push_back("zektorm");
    m_badWords.push_back("vaughnage");
    m_badWords.push_back("wunseedee");
    m_badWords.push_back("xor");
    m_badWords.push_back("zune");
    m_badWords.push_back("flawl3ss");
    m_badWords.push_back("devise");
    m_badWords.push_back("fasm");
    m_badWords.push_back("avs720");
  }

  m_commonWords.push_back("^the$");

}

void CMetadataResolverVideo::SetBadWords(const std::vector<std::string>& vecBadWords)
{
  m_badWords = vecBadWords;
}

bool CMetadataResolverVideo::IsBadWord(const CStdString& strWord)
{
  // first try a few case sensitive special cases (huristic)
  if (strWord == "LiMiTED" || strWord == "LiMITED" || strWord == "LIMITED" || strWord == "REPACK" || strWord == "PROPER" ||
      strWord == "EXCLUSIVE" || strWord == "UNRATED" || strWord == "PAL" || strWord == "NTSC" || strWord == "R5" || strWord == "HD" ||
      strWord == "INTERNAL" || strWord == "iNTERNAL")
    return true;

  CStdString word = strWord;
  word.ToLower();

  //CLog::Log(LOGDEBUG, "LIBRARY: Check if bad word: %s", word.c_str());

  if (m_badWords.size() == 0)
    InitializeVideoResolver();

  std::vector<std::string>::const_iterator iter = m_badWords.begin();
  while (iter != m_badWords.end())
  {
    CRegExp reg;
    reg.RegComp((*iter).c_str());
    if (reg.RegFind(word) != -1)
    {
      //CLog::Log(LOGDEBUG, "LIBRARY, RESOLVER, LIB3: Detected bad word: %s on clause %s", word.c_str(), (*iter).c_str());
      return true;
    }
    iter++;
  }

  return false;

}

//bool CMetadataResolverVideo::IsCommonWord(const CStdString& strWord)
//{
//  CStdString word = strWord;
//  word.ToLower();
//
//  //CLog::Log(LOGDEBUG, "LIBRARY: Check if common word: %s", word.c_str());
//
//  if (m_commonWords.size() == 0)
//    InitializeVideoResolver();
//
//  std::vector<CStdString>::const_iterator iter = m_commonWords.begin();
//  while (iter != m_commonWords.end())
//  {
//    CRegExp reg;
//    reg.RegComp(*iter);
//    if (reg.RegFind(word) != -1) {
//      //CLog::Log(LOGDEBUG, "LIBRARY: Detected common word: %s on clause %s", word.c_str(), (*iter).c_str());
//      return true;
//    }
//    iter++;
//  }
//
//  return false;
//
//}

bool CMetadataResolverVideo::ParseSeriesTag(const CStdString& strTerm, const CStdString& strRegEx, int& iSeason, int& iEpisode, int& iTagPosition, int& iTagLength)
  {
  CStdString strCopyWord = "*"+strTerm+"*"; // to make sure we find the pattern if it appears at the begin or end

  strCopyWord.ToLower();

  CRegExp reg;
  char* season = NULL;
  char* episode = NULL;

  CStdString strRegExpr = strRegEx;
  reg.RegComp(strRegExpr);
  int iPos = reg.RegFind(strCopyWord);
  if (iPos != -1)
  {
    season = reg.GetReplaceString("\\1");
    episode = reg.GetReplaceString("\\2");

    iTagPosition = iPos;
    iTagLength = reg.GetFindLen();
  }

  if (!season || !episode)
  {
    free(season);
    free(episode);

    //CLog::Log(LOGDEBUG, "CMetadataResolverVideo::ParseSeriesTag, unable to parse term = %s, expr = %s (seriestag)",strTerm.c_str(), strRegEx.c_str());
    return false;
  }

  CLog::Log(LOGDEBUG, "CMetadataResolverVideo::ParseSeriesTag, term = %s, expr = %s, season = %s, episode = %s (seriestag)", strTerm.c_str(), strRegEx.c_str(), season, episode);
  iSeason = atoi(season);
  iEpisode = atoi(episode);

  free(season);
  free(episode);

  return true;

}

bool CMetadataResolverVideo::ParseNumericTag(const CStdString& strTerm, const CStdString& strRegEx , int& iTag, int& iTagPosition, int& iTagLength)
{
  CRegExp reg;
  char* tagStr = NULL;

  reg.RegComp(strRegEx);
  int iPos = reg.RegFind(strTerm);
  if (iPos != -1)
  {
    tagStr = reg.GetReplaceString("\\1");
    iTagPosition = iPos;
    iTagLength = reg.GetFindLen();

  } else
  {
    return false;
  }

  if (!tagStr)
    return false;

  iTag = atoi(tagStr);
  free(tagStr);

  return true;
}


// try to extract daily/weekly series
// name + date
bool CMetadataResolverVideo::ExtractDailyShows(const CStdString& strTitle, int& iSeason,  CStdString& strDate, CStdString& strPrefix)
{
  CLog::Log(LOGDEBUG,"CMetadataResolverVideo::ExtractDailyShows - try to resolve tv show [%s] as daily show (ds)", strTitle.c_str());

  CStdString year;
  CStdString day;
  CStdString month;
  int iTagPosition = 0;
  int iTagLength = 0;

  CRegExp regDate;

  if (!regDate.RegFindDate(strTitle,year,month,day,iTagPosition,iTagLength))
  {
    return false;
  }
  strDate.Format("%s/%s/%s",month.c_str(), day.c_str(), year.c_str());

  strPrefix = strTitle.Left(iTagPosition);
  iSeason = -2;

  CLog::Log(LOGDEBUG,"CMetadataResolverVideo::ExtractDailyShows - resolve tv show by date [day=%s][month=%s][year=%s]->[episodeDate=%s][strPrefix=%s] (ds)",day.c_str(),month.c_str(),year.c_str(),strDate.c_str(),strPrefix.c_str());

  return true;
}

bool CMetadataResolverVideo::ExtractPathSeriesTag(const CStdString& strPath, const CStdString& strTitle,
                                                  int& iSeason, int& iEpisode, CStdString& strPrefix,
                                                  CStdString& strSuffix)
{

  //try to extract series from the Title
  if (ExtractSeriesTag(strTitle, iSeason, iEpisode, strPrefix, strSuffix))
  {
    return true;
  }

  CLog::Log(LOGDEBUG, "CMetadataResolverVideo::ExtractPathSeriesTag, try to resolve tv show by path %s title %s ", strPath.c_str(), strTitle.c_str());

  bool matchTag = false;
  int iTagPosition = 0;
  int iTagLength = 0;
  CStdString parentDir, parentTitle;

  parentDir = BXUtils::GetParentPath(strPath);
  BXUtils::RemoveSlashAtEnd(parentDir);
  parentTitle = CUtil::GetFileName(parentDir);

  std::vector<CStdString> vecExpressions;
  // foo.s01.e01, foo.s01_e01, S01E02 foo
  vecExpressions.push_back("^[Ss][Ee]?([0-9]+)$");
  // Sex And The City Season2 EP06
  vecExpressions.push_back("^season ?([0-9]+)$");
  // Sex And The City Season2 Episode6
  vecExpressions.push_back("^Season ?([0-9]+)$");

  CStdString strCopyWord = parentTitle;
  strCopyWord.ToLower();

  for (int i = 0; i < (int)vecExpressions.size()  && !matchTag; i++)
  {
    if (ParseNumericTag(strCopyWord,vecExpressions[i],iSeason, iTagPosition, iTagLength))
      matchTag = true;
  }

  if (!matchTag)
  {
    CLog::Log(LOGDEBUG,"could not extract season from parent folder %s", parentTitle.c_str());
    return false;
  }

  CLog::Log(LOGDEBUG,"Extract season %d - try to extract episode title  ", iSeason);

  vecExpressions.clear();
  // foo.s01.e01, foo.s01_e01, S01E02 foo
  vecExpressions.push_back("^[Ee][Pp]?[\\.-_ ]+([0-9]+)");
  // foo_[s01]_[e01]
  vecExpressions.push_back("^[[Ee][Pp]?[\\.-_ ]+([0-9]+)\\]?");
  // foo.1x09* or just /1x09*
  vecExpressions.push_back("^([0-9])+[.-]*");
  // Sex And The City Season2 Episode6
  vecExpressions.push_back("^Episode ?([0-9]+)");

  strCopyWord = strTitle;
  strCopyWord.ToLower();

  matchTag = false;
  for (int i = 0; i < (int)vecExpressions.size()  && !matchTag; i++)
  {
    if (ParseNumericTag(strCopyWord,vecExpressions[i],iEpisode, iTagPosition, iTagLength))
      matchTag = true;
  }

  if (!matchTag)
  {
    CLog::Log(LOGDEBUG,"could not extract episode from title");
    return false;
  }

  parentDir = BXUtils::GetParentPath(parentDir);
  BXUtils::RemoveSlashAtEnd(parentDir);
  strPrefix = CUtil::GetFileName(parentDir);

  strSuffix = strTitle.Right(strTitle.length() - (iTagPosition + iTagLength));

  CLog::Log(LOGDEBUG,"extract series by path: prefix %s suffix %s season %d episode %d ", strPrefix.c_str(), strSuffix.c_str(), iSeason, iEpisode) ;
  return true;
}


// Extracts the season and episode information and removes the detected tag from the name
// strTitle  - input title
// iSeason - extracted season if available, -1 otherwise
// iEpisode - extracted episode if available, -1 otherwise
// strPrefix - left part of the title, before the tag or the entire title if tag not detected
// strSuffix - right part of the title, after the tag or empty if tag not detected
bool CMetadataResolverVideo::ExtractSeriesTag(const CStdString& strTitle, int& iSeason, int& iEpisode, CStdString& strPrefix, CStdString& strSuffix)
{
  std::vector<CStdString> vecExpressions;

  // foo.s01.e01, foo.s01_e01, S01E02 foo
  vecExpressions.push_back("[Ss][Ee]?([0-9]+)[\\.-_ ]?[Ee][Pp]?([0-9]+)");
   // foo_[s01]_[e01]
  vecExpressions.push_back("\\[[Ss][Ee]?([0-9]+)\\]_\\[[Ee][Pp]?([0-9]+)\\]?([^\\\\/]*)$");
  // foo.1x09* or just /1x09*
  vecExpressions.push_back("[\\\\/\\._ \\[-]?([0-9]+)x([0-9]+)([^\\\\/]*)$");
  // foo.103*, 103 foo
  vecExpressions.push_back("[\\\\/\\._ -]([0-9]+)([0-9][0-9])([\\._ -][^\\\\/]*)?$");
  // Mad.Men.S01 blah blah E01.720p.HDTV.x264-FoV
  vecExpressions.push_back("[Ss][Ee]?(\\d{1,2}).*?[Ee][Pp]?(\\d{1,3})");
  // Sex And The City Season2 EP06
  vecExpressions.push_back("Season ?(\\d{1,2}).*?[Ee][Pp]?(\\d{1,3})");
  // Sex And The City Season2 Episode6
  vecExpressions.push_back("Season ?(\\d{1,2}).*?Episode ?(\\d{1,3})");

  CStdString strCopyWord = "*"+strTitle+"*"; // to make sure we find the pattern if it appears at the begin or end
  strCopyWord.ToLower();

  int iTagPosition = 0;
  int iTagLength = 0;

  for (int i = 0; i < (int)vecExpressions.size(); i++)
  {
    if (ParseSeriesTag(strCopyWord, vecExpressions[i], iSeason, iEpisode, iTagPosition, iTagLength))
  {
      strPrefix = strTitle.Left(iTagPosition - 2); // the -2 is here because we add an asterisk (*) to the original
      strSuffix = strTitle.Mid(iTagPosition + iTagLength - 2);
    return true;
  }
  }

  // Continue looking for tags of type ### or ####
  CRegExp reg;
  CStdString strRegExpr = "[^0-9xh][0-9][0-9][0-9][0-9]?[^0-9pxi]";
  reg.RegComp(strRegExpr);
  int iPos = reg.RegFind(strCopyWord);
  if (iPos != -1)
  {

    CStdString strSeasonNum ;
    CStdString strEpisodeNum ;

    int nNumericVal = atoi(strCopyWord.Mid(iPos+1).c_str());

    // find out if its 4 or 3 digits
    if (nNumericVal >= 1000)
    {
      strSeasonNum = strCopyWord.Mid(iPos+1,2);
      strEpisodeNum = strCopyWord.Mid(iPos+3,2);
    }
    else
    {
      strSeasonNum = strCopyWord.Mid(iPos+1,1);
      strEpisodeNum = strCopyWord.Mid(iPos+2,2);
    }

    iSeason = BXUtils::StringToInt(strSeasonNum);
    iEpisode = BXUtils::StringToInt(strEpisodeNum);
    if (iEpisode != 0 && iEpisode <= 30 && iSeason <= 20) // sanity - to discard years
    {
       strPrefix = strTitle.Left(iPos);
       strSuffix = "";
      return true;
  }
   }

  iSeason = -1;
  iEpisode = -1;
  strPrefix = strTitle;
  strSuffix = "";

  return false;
}

vector<std::string> CMetadataResolverVideo::RemoveBadWords(const vector<std::string>& vecWords)
{
  vector<std::string> vecGoodWords;
  for (unsigned int i = 0; i < vecWords.size(); i++) {
    if (!IsBadWord(vecWords[i])) {
      vecGoodWords.push_back(vecWords[i]);
    }
    else
      break;
  }

  return vecGoodWords;
}

bool CMetadataResolverVideo::CheckPart(const CStdString& strFilename, int & iPart)
{
  CStdString relevantFilename = strFilename;

#ifdef _WIN32
  const char folderSeparator = '\\';
#else
  const char folderSeparator = '/';
#endif

  int lastSlash = relevantFilename.find_last_of(folderSeparator);
  CStdString extractedParentPath = relevantFilename.Left(lastSlash);
  int prelastSlash = extractedParentPath.find_last_of(folderSeparator);
  relevantFilename = relevantFilename.Right(relevantFilename.size() - prelastSlash);

  CLog::Log(LOGDEBUG, "CMetadataResolverVideo::CheckPart, checking if its a stack of files/folders for [relevantFilename=%s][prelastSlash=%d][extractedParentPath=%s][lastSlash=%d] (stack)(resolve)", relevantFilename.c_str(),prelastSlash,extractedParentPath.c_str(),lastSlash);

  CStdString strCopyWord = "*"+relevantFilename+"*"; // to make sure we find the pattern if it appears at the begin or end
  strCopyWord.ToLower();

  CRegExp reg;

  CStdString strRegExpr = "cd|part([0-9]+)";
  reg.RegComp(strRegExpr);
  int iPos = reg.RegFind(strCopyWord);
  if (iPos != -1)
  {

    char* strPartNum = NULL;
    strPartNum = reg.GetReplaceString("\\1");

    // make sure that the number is not too big
    if (strlen(strPartNum) > 5)
    {
      CLog::Log(LOGDEBUG, "Currently we support up to 9999 files parts - cant resolve %s", relevantFilename.c_str());
    }

    if (strPartNum)
    {
      iPart = atoi(strPartNum);
      free (strPartNum);
    }

    //CLog::Log(LOGDEBUG, "LIBRARY, RESOLVER, LIB3, Part detected %d.", iPart);
    return true;
  }

  return false;
}

bool CMetadataResolverVideo::IsSample(const CStdString& strName1)
{
  bool bResult = false;

  CStdString strRegExpr = "sample";
  CRegExp reg;
  reg.RegComp(strRegExpr);

  CStdString strName = strName1;

  strName.ToLower();

  int iPos = reg.RegFind(strName);
  if (iPos != -1) {
    bResult = true;
  }

  return bResult;
}

bool CMetadataResolverVideo::IsTrailer(const CStdString& strName1)
{
  bool bResult = false;

  CStdString strRegExpr = "-[Tt][Rr][Aa][Ii][Ll][Ee][Rr]";
  CRegExp reg;
  reg.RegComp(strRegExpr);

  CStdString strName = strName1;

  strName.ToLower();

  int iPos = reg.RegFind(strName);
  if (iPos != -1) {
    bResult = true;
  }

  return bResult;
}

CStdString CMetadataResolverVideo::CleanName(const CStdString& strName)
{
  static const char *SEPARATORS = ",.-_/\\{}() ";

  // The second set of separators includes rectanguilar brackets to remove all excess brackets after initial cleanup phase
  //static const char *SEPARATORS2 = ",.-_/\\{}()[]";

  // 111111111111111111111111111111111111111
  // first thing we do is find where the [] words start.
  // we ignore the rest of the line from the first [] word. (huristic)
  CStdString strCopy = strName;

  strCopy.Trim();

  int idx = strName.Find('[');

  if (idx >= 3 /*sanity */)
  {
    strCopy = strName.Mid(0,idx); // Left(idx)
  }

  else if (idx >= 0) // found [] in the begining
  {
    // find the first word which is not in [] and remove everything before it
    CRegExp reg;
    reg.RegComp(CStdString("][") + SEPARATORS + CStdString("]*[^[]"));
    int nPos = reg.RegFind(strName);
    if (nPos > 0)
      strCopy = strName.Mid(nPos+1); 
  }

  // 2222222222222222222222222222222222
  // Second cleanup phase: tokenize by separators and remove bad words
  vector<std::string> vecWords = BXUtils::StringTokenize(strCopy, SEPARATORS);
  vector<std::string> vecGoodWords = RemoveBadWords(vecWords);

  CStdString strNewName = "";
  // Rebuild the name again
  for (unsigned int i = 0; i < vecGoodWords.size(); i++) {
    strNewName += vecGoodWords[i];
    strNewName.Trim();
    strNewName += " ";
  }

  strNewName.Trim();

  return strNewName;
}

bool CMetadataResolverVideo::IsDVDFilename(const CStdString& strPath)
{
  CStdString fileName = CUtil::GetFileName(strPath);

  CStdString strRegExpr = "VIDEO_TS|VTS_[0-9]?[0-9]?|VTS_[0-9][0-9]_[0-9]?[0-9]?";

  CRegExp reg;
  reg.RegComp(strRegExpr);

  int iPos = reg.RegFind(fileName);

  if (iPos != -1) {
    //CLog::Log(LOGDEBUG, "LIBRARY: Rebuilder, detected DVD filename = %s", fileName.c_str());

    return true;
  }
  return false;
}

bool CMetadataResolverVideo::IsDVDExtension(const CStdString& strPath)
{
  //CLog::Log(LOGDEBUG, "LIBRARY: Rebuilder, Checking file, path = %s", strPath.c_str());
  CStdString strExtension = CUtil::GetExtension(strPath);
  strExtension.ToLower();
  //CLog::Log(LOGDEBUG, "LIBRARY: Rebuilder, extension %s", strExtension.c_str());
  if (strExtension == ".vob" ||
      strExtension == ".bup" ||
      strExtension == ".ifo" ||
      strExtension == ".tag"
  ) {
    //CLog::Log(LOGDEBUG, "LIBRARY: Rebuilder, This is dvd file, path = %s, extension %s", strPath.c_str(), strExtension.c_str());
    return true;
  }
  return false;
}

bool CMetadataResolverVideo::IsDVDFolder(const CFileItemList& items)
{
  // Retrieve the list of all items from the path
  bool bIsDVDFolder = false;

  int iFileCount = items.Size();
  int iMiscFiles = 0;

  for (int i=0; i < iFileCount; i++)
  {
    CFileItemPtr pItem = items[i];
    CStdString strPath = pItem->m_strPath;

    CLog::Log(LOGDEBUG, "CMetadataResolverVideo::IsDVDFolder, checking path strPath = %s (dvd)", strPath.c_str());
    if (!IsDVDFilename(strPath) || !IsDVDExtension(strPath))
    {
      CLog::Log(LOGDEBUG, "CMetadataResolverVideo::IsDVDFolder, NON DVD path strPath = %s (dvd)", strPath.c_str());
      iMiscFiles++;
    }
    else
    {
      CLog::Log(LOGDEBUG, "CMetadataResolverVideo::IsDVDFolder, DVD path strPath = %s (dvd)", strPath.c_str());
      bIsDVDFolder = true;
    }
  }

  int iMiscFileCount = iFileCount / 10;
  bool retVal = (bIsDVDFolder && (iMiscFiles <= iMiscFileCount));

  CLog::Log(LOGDEBUG, "CMetadataResolverVideo::IsDVDFolder, folder %s is %s folder (dvd)", items.m_strPath.c_str(), retVal ? "DVD" : "NON DVD");
  return retVal;

}

bool CMetadataResolverVideo::IsBlurayExtension(const CStdString& strPath)
{
  CStdString strExtension = CUtil::GetExtension(strPath);
  strExtension.ToLower();

  if (strExtension == ".m2ts") 
  {
    return true;
  }
  return false;
}

bool CMetadataResolverVideo::IsOnBlurayFolder(const CStdString& strFile)
{
  CStdString strPath;

  CUtil::GetDirectory(strFile,strPath);
  CUtil::RemoveSlashAtEnd(strPath);

  if(CUtil::GetFileName(strPath) != "STREAM")
  {
    return false;
  }

  CUtil::GetDirectory(strPath,strPath);
  CUtil::RemoveSlashAtEnd(strPath);

  if(CUtil::GetFileName(strPath) != "BDMV")
  {
    return false;
  }
     
  return true;
}

bool CMetadataResolverVideo::IsBlurayFolder(const CFileItemList& items)
{
  // Retrieve the list of all items from the path
  bool bIsBlurayFolder = false;

  int iFileCount = items.Size();
  int iMiscFiles = 0;

  for (int i=0; i < iFileCount; i++)
  {
    CFileItemPtr pItem = items[i];
    CStdString strPath = pItem->m_strPath;

    CLog::Log(LOGDEBUG, "CMetadataResolverVideo::IsBlurayFolder, checking path strPath = %s (bluray)", strPath.c_str());
    if (!IsOnBlurayFolder(strPath) || !IsBlurayExtension(strPath))
    {
      CLog::Log(LOGDEBUG, "CMetadataResolverVideo::IsBlurayFolder, NON Bluray path strPath = %s (bluray)", strPath.c_str());
      iMiscFiles++;
    }
    else
    {
      CLog::Log(LOGDEBUG, "CMetadataResolverVideo::IsBlurayFolder, Bluray path strPath = %s (bluray)", strPath.c_str());
      bIsBlurayFolder = true;
    }
  }

  int iMiscFileCount = iFileCount / 10;
  bool retVal = (bIsBlurayFolder && (iMiscFiles <= iMiscFileCount));

  CLog::Log(LOGDEBUG, "CMetadataResolverVideo::IsBlurayFolder, folder %s is %s folder (bluray)", items.m_strPath.c_str(), retVal ? "Bluray" : "NON Bluray");
  return retVal;

}

CStdString CMetadataResolverVideo::RemoveYear(const CStdString& strName, CStdString& strYear)
{
  CStdString newName = strName;

  CStdString strRegExpr = "(19|20)[0-9][0-9]";
  CRegExp reg;
  reg.RegComp(strRegExpr);

  int iPos = reg.RegFind(strName);

  if (iPos != -1)
  {
    strYear = strName.Mid(iPos, 4);
    CStdString firstPart = strName.Left(iPos);

    firstPart = firstPart.Trim();
    CStdString secondPart = strName.Right(strName.GetLength() - iPos - 4);
    secondPart = secondPart.Trim();

    newName = firstPart;
    newName += " ";
    newName += secondPart;
  }

    CLog::Log(LOGDEBUG, "CMetadataResolverVideo::RemoveYear, name = %s, newName = %s, year = %s (cleanname)", strName.c_str(), newName.c_str(), strYear.c_str());
    return newName;
  }

CStdString CMetadataResolverVideo::RemoveAfterLastComma(const CStdString& strName)
{
  size_t pos = strName.find_last_of(",");
  if (pos != string::npos) {
    return strName.Left(pos);
  }
  else {
    return strName;
  }
}

bool CMetadataResolverVideo::PathIsShare(const CStdString& strPath)
{
  CStdString folderName = BXUtils::GetEffectiveFolderName(strPath);
  VECSOURCES* videoShares = g_settings.GetSourcesFromType("video");
  bool bParentIsShare = false;
  CUtil::GetMatchingSource(strPath, *videoShares, bParentIsShare);

  if (!bParentIsShare && (folderName == "incoming" || folderName == "movies" || folderName == "video" || folderName == "videos" ||
      folderName == "downloads"))
    bParentIsShare = true;

  return bParentIsShare;
}

bool CMetadataResolverVideo::GetFileData(const std::string& strFilePath, int64_t& iLength, int& iModDate)
{
  CURI url(strFilePath);
  struct __stat64 stat;

  if (XFILE::CFile::Stat(strFilePath, &stat) < 0)
    return false;

  // TODO: Check if date works correctly
#ifdef _WIN32
  iModDate = stat.st_ctime;
#elif defined (__APPLE__)
  iModDate = stat.st_ctimespec.tv_sec;
#else
  iModDate = stat.st_ctime;
#endif
  iLength = stat.st_size;

  return true;
}

bool CMetadataResolverVideo::LoadMovieInfo(const CFileItemList& list, CVideoFileContext& context, BXMetadata* pMetadata)
{
  if (list.IsEmpty() && !context.m_bVideoDetailsFromNFO)
  {
    //nothing was resolved, we must have an .nfo to continue
    CLog::Log(LOGDEBUG, "CMetadataResolverVideo::LoadMovieInfo - Resolving against Server failed and couldn't find relevant NFO for Movie (mrv)(resolver)(nfo)");
    return false;
  }

  CFileItemPtr item;

  if (list.Size() > 0)
    item = list.Get(0);

  CVideoInfoTag infoTag;

  if (context.m_bVideoDetailsFromNFO)
  {
    //if there's an nfo file, use it as base details
    infoTag = context.m_nfoVideoDetails;

    //merge the resolved info from the server in case there were any empty fields in the nfo
    if (item.get())
      infoTag.MergeFieldsFrom(*item->GetVideoInfoTag());
  }
  else
  {
    infoTag = *item->GetVideoInfoTag();
  }

  BXVideo* pVideo = (BXVideo*) pMetadata->GetDetail(MEDIA_DETAIL_VIDEO);
  if (item.get())
  {
    pVideo->m_strBoxeeId = item->GetProperty("boxeeid");
  }
  else if (context.m_bVideoDetailsFromNFO && !context.m_nfoVideoDetails.m_strTitle.IsEmpty())
  {
    //we're missing the movie details, generate the video-id as "local-%videoid%"
    CLog::Log(LOGDEBUG, "CMetadataResolverVideo::LoadMovieInfo, generating movie completely from NFO file.(nfo)(resolver)");

    CStdString localBoxeeId = "local-";
    localBoxeeId += context.m_nfoVideoDetails.m_strTitle;

    pVideo->m_strBoxeeId = localBoxeeId;
  }

  pVideo->m_strTitle = infoTag.m_strTitle;
  pVideo->m_strDirector = infoTag.m_strDirector;
  pVideo->m_strGenre = infoTag.m_strGenre;
  pVideo->m_strDescription = infoTag.m_strPlot;
  pVideo->m_strExtDescription = infoTag.m_strPlotOutline;
  pVideo->m_iDuration = BXUtils::StringToInt(infoTag.m_strRuntime);
  pVideo->m_strIMDBKey = infoTag.m_strIMDBNumber;
  pVideo->m_strMPAARating = infoTag.m_strMPAARating;
  pVideo->m_strCredits = infoTag.m_strWritingCredits;
  pVideo->m_strStudio = infoTag.m_strStudio;
  pVideo->m_strTagLine = infoTag.m_strTagLine;
  pVideo->m_iCriticsScore = infoTag.m_rottenTomatoDetails.iCriticsScore;
  pVideo->m_iAudienceScore = infoTag.m_rottenTomatoDetails.iAudienceScore;
  pVideo->m_strAudienceRating = infoTag.m_rottenTomatoDetails.strAudienceRating;
  pVideo->m_strCriticsRating = infoTag.m_rottenTomatoDetails.strCriticsRating;

  if (context.m_bVideoDetailsFromNFO && !infoTag.m_strPictureURL.GetFirstThumb().m_url.IsEmpty())
  {
    pVideo->m_strCover = infoTag.m_strPictureURL.GetFirstThumb().m_url;
  }
  else
  {
    if (item.get())
    {
      pVideo->m_strCover = item->GetThumbnailImage();
    }
  }

  if (item.get())
  {
    pVideo->m_strOriginalCover = item->GetThumbnailImage();
  }

  pVideo->m_iRating = infoTag.m_iRating;
  pVideo->m_iPopularity = infoTag.m_iPopularity;
  pVideo->m_iYear = infoTag.m_iYear;
  pVideo->m_strTrailerUrl = infoTag.m_strTrailer;
  pVideo->m_strCredits = infoTag.m_strWritingCredits;
  //pVideo->m_strShowTitle = infoTag.m_strShowTitle;
  pVideo->m_strTagLine = infoTag.m_strTagLine;
  pVideo->m_iDateModified = context.iModificationDate;
  pVideo->m_strNfoPath = context.m_nfoVideoDetails.m_strNFOPath;
  pVideo->m_iNfoAccessedTime = context.m_nfoVideoDetails.m_iNfoAccessTime;
  pVideo->m_iNfoModifiedTime = context.m_nfoVideoDetails.m_iNfoModifiedTime;

  if (item.get())
  {
    pVideo->m_iReleaseDate = BXUtils::StringToUnsignedLong(item->GetProperty("releasedateVal"));
  }
  //CLog::Log(LOGDEBUG,"CMetadataResolverVideo::LoadMovieInfo, releasedateVal : %s was converted to iReleaseDate : %d (releasedate)", item->GetProperty("releasedateVal").c_str(),pVideo->m_iReleaseDate);

  vector<SActorInfo>& cast = infoTag.m_cast;
  for (unsigned int i = 0; i < cast.size(); i++)
  {
    pVideo->m_vecActors.push_back((cast[i].strName));
  }

  if (item.get())
  {
    const CFileItemList* items = item->GetLinksList();
    if (items)
    {
      for (int i = 0 ; i < items->Size() ; i++)
      {
        const CFileItemPtr pItem = items->Get(i);
        if (pItem)
        {
          //pItem->Dump();
          if (CFileItemList::GetLinkBoxeeTypeAsEnum(pItem->GetProperty("link-boxeetype")) == CLinkBoxeeType::TRAILER)
          {
            BXVideoLink trailer;
            trailer.m_strProviderName = pItem->GetProperty("link-providername");
            trailer.m_strProvider = pItem->GetProperty("link-provider");
            trailer.m_strProviderThumb = pItem->GetProperty("link-providerthumb");

            trailer.m_strQuality = pItem->GetProperty("quality");
            trailer.m_strQualityLabel = pItem->GetProperty("quality-lbl");
            trailer.m_strTitle = pItem->GetProperty("link-title");
            trailer.m_strURL = pItem->GetProperty("link-url");
            trailer.m_strType = pItem->GetProperty("link-type");

            trailer.m_strCountryCodes = pItem->GetProperty("link-countrycodes");
            trailer.m_strCountryRel = pItem->GetPropertyBOOL("link-countryrel") ? "1" : "0";

            trailer.m_bIsHD = pItem->GetPropertyBOOL("is-hd");

            trailer.m_strBoxeeType = pItem->GetProperty("link-boxeetype");
            trailer.m_strOffer = pItem->GetProperty("link-boxeeoffer");

            pVideo->m_vecVideoLinks.push_back(trailer);
          }
        }
      }
    }
  }

  //put the NFO trailer as a link
  if (!infoTag.m_strTrailer.IsEmpty())
  {
    BXVideoLink nfoTrailer;
    nfoTrailer.m_strProviderName = "NFO";
    nfoTrailer.m_strProvider = "NFO";
    nfoTrailer.m_strTitle = "Trailer of NFO";
    nfoTrailer.m_strURL = infoTag.m_strTrailer;
    nfoTrailer.m_strType = "Trailer";
    nfoTrailer.m_strBoxeeType = "NFO";
    nfoTrailer.m_strOffer = "NFO";
    pVideo->m_vecVideoLinks.push_back(nfoTrailer);
  }

  //find all the files that are videos and has the postfix of -trailer (insensitive)
  CStdString lookupquery = CUtil::GetFileName(pVideo->m_strPath);
  CUtil::RemoveExtension(lookupquery);
  lookupquery += "-trailer";
  CStdString parentPath;
  CUtil::GetParentPath(pVideo->m_strPath, parentPath);
  CFileItemList trailerFileItems;
  BoxeeUtils::FindFiles(parentPath,lookupquery,trailerFileItems,true,true,false,false,true);

  if (!trailerFileItems.IsEmpty())
  {
    //we found local trailers files.. add them to the db as well, we look for FILENAME-trailer*
    BXVideoLink nfoTrailer;
    nfoTrailer.m_strProviderName = "LOCAL";
    nfoTrailer.m_strProvider = "LOCAL";
    nfoTrailer.m_strType = "LOCAL";
    nfoTrailer.m_strBoxeeType = "LOCAL";
    nfoTrailer.m_strOffer = "LOCAL";

    for (int i = 0 ; i < trailerFileItems.Size() ; i++)
    {
      const CFileItemPtr pItem = trailerFileItems.Get(i);
      if (pItem)
      {
        if (pVideo->m_strTrailerUrl.empty())
        {
          pVideo->m_strTrailerUrl = pItem->m_strPath;
        }

        nfoTrailer.m_strURL = pItem->m_strPath;
        //construct the title according to the filename .. use whats after the trailers '-'
        //if the filename is Batman-trailer.1, the title of the trailer would be trailer.1
        CStdString filename = CUtil::GetFileName(pItem->m_strPath);
        CUtil::RemoveExtension(filename);
        size_t pos = filename.rfind(L'-');
        nfoTrailer.m_strTitle = filename.substr(++pos);
        pVideo->m_vecVideoLinks.push_back(nfoTrailer);
      }
    }
  }

  pVideo->m_bMovie = true;

  if (!context.m_userVideoCoverPath.IsEmpty())
  {
    pVideo->m_strCover = context.m_userVideoCoverPath;
    CLog::Log(LOGDEBUG,"CMetadataResolverVideo::LoadMovieInfo - for movie [label=%s][boxeeId=%s] set UserVideoThumb [%s] (uvt)",pVideo->m_strTitle.c_str(),pVideo->m_strBoxeeId.c_str(),pVideo->m_strCover.c_str());
  }

  return true;
}

bool CMetadataResolverVideo::LoadTVEpisodeItem(const CFileItemPtr& episodeItem, CVideoFileContext& context, BOXEE::BXVideo* pVideo)
{
  // Load episode info
  // we treat the local nfo here as if it is read into m_nfoDetails
  CVideoInfoTag infoTag;

  if (!episodeItem->GetPropertyBOOL("locally-generated"))
  {
    if (context.m_bVideoDetailsFromNFO)
    {
      //if there's an nfo file, use it as base details
      infoTag = context.m_nfoVideoDetails;

      //merge the resolved info from the server in case there were any empty fields in the nfo
      infoTag.MergeFieldsFrom(*episodeItem->GetVideoInfoTag());
    }
    else
    {
      infoTag = *episodeItem->GetVideoInfoTag();
    }
  }
  else
  {
    infoTag = context.m_nfoVideoDetails;
  }

  pVideo->m_strBoxeeId = episodeItem->GetProperty("boxeeid");
  pVideo->m_strTitle = infoTag.m_strTitle;

  if (pVideo->m_strDirector.empty() || !infoTag.m_strDirector.IsEmpty())
  {
    //worst case inherit from tvshow.nfo
    pVideo->m_strDirector = infoTag.m_strDirector;
  }

  if (pVideo->m_strGenre.empty() || !infoTag.m_strGenre.IsEmpty())
  {
    //worst case inherit from tvshow.nfo
    pVideo->m_strGenre = infoTag.m_strGenre;
  }

  pVideo->m_strDescription = infoTag.m_strPlotOutline;
  pVideo->m_strExtDescription = infoTag.m_strPlot;
  pVideo->m_iDuration = BXUtils::StringToInt(infoTag.m_strRuntime);
  pVideo->m_strIMDBKey = infoTag.m_strIMDBNumber;
  pVideo->m_strMPAARating = infoTag.m_strMPAARating;
  pVideo->m_strCredits = infoTag.m_strWritingCredits;
  pVideo->m_strStudio = infoTag.m_strStudio;
  pVideo->m_strTagLine = infoTag.m_strTagLine;

  if (context.m_bVideoDetailsFromNFO && infoTag.m_iEpisode != -1)
  {
    pVideo->m_iEpisode = infoTag.m_iEpisode;
  }
  else
  {
    pVideo->m_iEpisode = context.iEpisode;
  }

  if (context.m_bVideoDetailsFromNFO && infoTag.m_iSeason != -1)
  {
    pVideo->m_iSeason = infoTag.m_iSeason;
  }
  else
  {
    pVideo->m_iSeason = context.iSeason;
  }

  if (pVideo->m_iSeason == -1)
  {
    pVideo->m_iSeason = 0;
  }

  if (context.m_bVideoDetailsFromNFO && !infoTag.m_strPictureURL.GetFirstThumb().m_url.IsEmpty())
  {
    pVideo->m_strCover = infoTag.m_strPictureURL.GetFirstThumb().m_url;
  }
  else
  {
    pVideo->m_strCover = episodeItem->GetThumbnailImage();
  }

  pVideo->m_strOriginalCover = episodeItem->GetThumbnailImage();
  pVideo->m_iRating = infoTag.m_iRating;
  pVideo->m_iPopularity = infoTag.m_iPopularity;
  pVideo->m_iYear = infoTag.m_iYear;
  pVideo->m_iDateModified = context.iModificationDate;
  pVideo->m_strNfoPath = context.m_nfoVideoDetails.m_strNFOPath;
  pVideo->m_iNfoAccessedTime = context.m_nfoVideoDetails.m_iNfoAccessTime;
  pVideo->m_iNfoModifiedTime = context.m_nfoVideoDetails.m_iNfoModifiedTime;

  time_t releaseDate;

  if (context.m_bVideoDetailsFromNFO)
  {
    CDateTime nfoTime;

    nfoTime.SetFromDBDate(infoTag.m_strFirstAired);
    nfoTime.GetAsTime(releaseDate);
  }
  else
  {
    episodeItem->m_dateTime.GetAsTime(releaseDate);
  }

  pVideo->m_iReleaseDate = (unsigned long) releaseDate;

  vector<SActorInfo>& cast = infoTag.m_cast;
  for (unsigned int i = 0; i < cast.size(); i++)
  {
    pVideo->m_vecActors.push_back((cast[i].strName));
  }

  if (!context.m_userVideoCoverPath.IsEmpty())
  {
    pVideo->m_strCover = context.m_userVideoCoverPath;
    CLog::Log(LOGDEBUG,"CMetadataResolverVideo::LoadTVEpisodeItem - for episode [label=%s][boxeeId=%s] set UserVideoThumb [%s] (uvt)",pVideo->m_strTitle.c_str(),pVideo->m_strBoxeeId.c_str(),pVideo->m_strCover.c_str());
  }

  return true;
}

bool CMetadataResolverVideo::LoadTVShowItem(const CFileItemPtr& tvshowItem, CVideoFileContext& context, BOXEE::BXSeries* pSeries, BOXEE::BXSeason* pSeason , BOXEE::BXVideo* pVideo)
{
  CStdString strTVShowPath = context.strPath;
  // we always take from the result we got from the server
  pVideo->m_strShowId = tvshowItem->GetProperty("boxeeid");
  pSeries->m_strBoxeeId = tvshowItem->GetProperty("boxeeid");

  CVideoInfoTag infoTag;

  if (tvshowItem->GetVideoInfoTag()->m_strTitle.IsEmpty())
    tvshowItem->GetVideoInfoTag()->m_strTitle = tvshowItem->GetLabel();

  //if it was locally generated, we do not merge it with anything, we just use what we have from the nfo
  if (!tvshowItem->GetPropertyBOOL("locally-generated"))
  {
    if (context.m_bShowDetailsFromNFO)
    {
      infoTag = context.m_nfoShowDetails;
      infoTag.MergeFieldsFrom(*tvshowItem->GetVideoInfoTag());
    }
    else
    {
      infoTag = *tvshowItem->GetVideoInfoTag();
    }
  }
  else
  {
    infoTag = context.m_nfoShowDetails;
  }

  //find the tvshow nfo and use it
  pVideo->m_strShowTitle = infoTag.m_strTitle;
  pSeries->m_strTitle = infoTag.m_strTitle;

  if (context.m_bShowDetailsFromNFO && !infoTag.m_strPictureURL.GetFirstThumb().m_url.IsEmpty())
  {
    pSeries->m_strCover = infoTag.m_strPictureURL.GetFirstThumb().m_url;
  }
  else
  {
    pSeries->m_strCover = tvshowItem->GetThumbnailImage();
  }

  pSeries->m_iYear = infoTag.m_iYear;
  pSeries->m_strGenre = infoTag.m_strGenre;

  pSeries->m_strDescription = infoTag.m_strPlot.IsEmpty() ? tvshowItem->GetProperty("description") : infoTag.m_strPlot ;

  // may be overridden later by the episode.nfo
  pVideo->m_strDescription = context.m_bShowDetailsFromNFO ? infoTag.m_strPlotOutline : tvshowItem->GetProperty("description");
  pVideo->m_strExtDescription = context.m_bShowDetailsFromNFO ? infoTag.m_strPlot : tvshowItem->GetProperty("description");
  pVideo->m_strTitle = infoTag.m_strTitle;
  pVideo->m_strDirector = infoTag.m_strDirector;
  pVideo->m_strGenre = infoTag.m_strGenre;
  pVideo->m_strCover = context.m_bShowDetailsFromNFO ? infoTag.m_strPictureURL.GetFirstThumb().m_url : tvshowItem->GetThumbnailImage();
  // episode.nfo end

  // loaded by the context, will be overridden if the episode .nfo is found later, also used incase there is only tvshow item without an episode
  if (context.m_bShowDetailsFromNFO && infoTag.m_iSeason != -1 )
  {
    pVideo->m_iSeason = infoTag.m_iSeason;
  }
  else
  {
    pVideo->m_iSeason = context.iSeason;
  }

  if (context.m_bShowDetailsFromNFO && infoTag.m_iEpisode != -1 )
  {
    pVideo->m_iEpisode = infoTag.m_iEpisode;
  }
  else
  {
    pVideo->m_iEpisode = context.iEpisode;
  }

  pVideo->m_episodeDate = context.m_bShowDetailsFromNFO ? infoTag.m_strPremiered : context.strEpisodeDate;
  pVideo->m_bMovie = false;
  pSeason->m_iSeasonNum = context.m_bShowDetailsFromNFO ? infoTag.m_iSeason : context.iSeason;
  pSeries->m_strNfoPath = context.m_nfoShowDetails.m_strNFOPath;
  pSeries->m_iNfoAccessedTime = context.m_nfoShowDetails.m_iNfoAccessTime;
  pSeries->m_iNfoModifiedTime = context.m_nfoShowDetails.m_iNfoModifiedTime;

  return true;
}

bool CMetadataResolverVideo::LoadEpisodeInfo(const CFileItemList& list, CVideoFileContext& context, BXMetadata* pMetadata)
{
  BXVideo* pVideo = (BXVideo*) pMetadata->GetDetail(MEDIA_DETAIL_VIDEO);
  BXSeries* pSeries = (BXSeries*)pMetadata->GetDetail(MEDIA_DETAIL_SERIES);
  BXSeason* pSeason = (BXSeason*)pMetadata->GetDetail(MEDIA_DETAIL_SEASON);

  CFileItemPtr tvshowItem;
  CFileItemPtr episodeItem;

  //we search for the tv show and the episode in the list we recieved
  for (int i = 0; i < list.Size(); i++)
  {
    CFileItemPtr item = list.Get(i);

    if (item->GetPropertyBOOL("isepisode"))
    {
      episodeItem = item;
    }
    else if (item->GetPropertyBOOL("istvshow"))
    {
      tvshowItem = item;
    }
    else
    {
      CLog::Log(LOGERROR, "CMetadataResolverVideo::LoadEpisodeInfo, invalid item type (mrv)");
      //item->Dump();
      return false;
    }
  }

  if (!tvshowItem.get() && context.m_bShowDetailsFromNFO && !context.m_nfoShowDetails.m_strTitle.IsEmpty())
  {
    //if we are missing the tvshow details, but we have an nfo with a title, generate the tvshow id as local-%TVSHOW-TITLE%
    CLog::Log(LOGDEBUG, "CMetadataResolverVideo::LoadEpisodeInfo, generating tvshow from NFO file.(nfo)(resolver)");

    CStdString localBoxeeId = "local-";
    localBoxeeId += context.m_nfoShowDetails.m_strTitle;

    tvshowItem.reset( new CFileItem() );
    tvshowItem->SetProperty("boxeeid",localBoxeeId);
    tvshowItem->SetProperty("locally-generated",true);
  }

  if (!episodeItem.get() && context.m_bVideoDetailsFromNFO)
  {
    //if we are missing the episode item, generate it from the NFO completely.

    CLog::Log(LOGDEBUG, "CMetadataResolverVideo::LoadEpisodeInfo, generating episode from NFO file.(nfo)(resolver)");

    episodeItem.reset( new CFileItem() );
    episodeItem->SetProperty("locally-generated",true);
  }

  // Load tvshow info first
  if (tvshowItem.get())
  {
    LoadTVShowItem(tvshowItem, context, pSeries, pSeason, pVideo);
  }
  else
  {
    CLog::Log(LOGDEBUG, "CMetadataResolverVideo::LoadEpisodeInfo, No TVShow Item, couldn't resolve TVShow.(nfo)(resolver)");
    return false;
  }

  if (episodeItem.get())
  {
    LoadTVEpisodeItem(episodeItem, context, pVideo);
  }
  else
  {
    CLog::Log(LOGDEBUG, "CMetadataResolverVideo::LoadEpisodeInfo, No Episode Item (nfo)(resolver)");
  }

  // if we assume that it is a movie, and the server return it as a TV show without season and episode we should
  // consider it as unresolved
  if (!context.m_bIsManualResolve && (pVideo->m_iEpisode == -1) && (pVideo->m_iSeason == -1))
  {
    CLog::Log(LOGDEBUG,"CMetadataResolverVideo::LoadEpisodeInfo - Server replies change type to tvshow without season or episode");
    return false;
  }

  return true;
}

// OLD DEPRECATED RESOLVER CODE

/*
bool CMetadataResolverVideo::ResolveVideoByName(CVideoContext* pContext, BXMetadata* pMetadata)
{
  if (!pMetadata || !pContext || pMetadata->GetType() != MEDIA_ITEM_TYPE_VIDEO)
  {
    CLog::Log(LOGERROR, "CMetadataResolverVideo::ResolveVideoByName, could not resolve video by name (vidoeresolver)");
    return false;
  }

  BXVideo* pVideo = (BXVideo*) pMetadata->GetDetail(MEDIA_DETAIL_VIDEO);

  // Get file name from the path
  CStdString strName = CUtil::GetFileName(pContext->strPath);
  CUtil::RemoveExtension(strName);

  // Get "clean" video file name, and update the context accordingly
  CStdString strNewName = GetCleanName(strName, pContext);

  bool bResult = false;
  CLog::Log(LOGDEBUG, "CMetadataResolverVideo::ResolveVideoByName, LIBRARY, RESOLVER, Resolving video name = %s", strNewName.c_str());

  if (strNewName != "") 
  {
    if (pContext->bMovie) 
    {
      bResult = ResolveMovieByName(strNewName, pContext->strYear, pMetadata);
    }
    else 
    {
      // Copy relevant detail from the context to the metadata
      pVideo->m_iEpisode = pContext->iEpisode;
      pVideo->m_iSeason = pContext->iSeason;
      bResult = ResolveTvShowByName(strNewName, pContext->strYear, pMetadata);
    }
  }

  if (!bResult) 
  {
    // Video was not resolved by name, attempt to resolve by folder
    CStdString strPath = pContext->strPath;

    // Calculate effective folder path
    strPath = BXUtils::GetEffectiveParentFolderPath(strPath);

    if(!strPath.IsEmpty() && !PathIsShare(strPath))
    {
      // Get effective folder name
      CStdString strFolderName;
      CUtil::GetDirectoryName(strPath, strFolderName);

      CVideoContext context;
      strFolderName = GetCleanName(strFolderName, &context);

      if (pContext->bMovie) 
      {
        CLog::Log(LOGDEBUG, "RESOLVER, Resolving movie by folder name %s", strFolderName.c_str());
        bResult = ResolveMovieByName(strFolderName, pContext->strYear, pMetadata);
      }
      else 
      {
        // Copy relevant detail from the context to the metadata
        pVideo->m_iEpisode = pContext->iEpisode;
        pVideo->m_iSeason = pContext->iSeason;
        CLog::Log(LOGDEBUG, "LIBRARY, RESOLVER, Resolving tv show by folder name %s, season = %d, episode = %d",
            strFolderName.c_str(), pVideo->m_iSeason, pVideo->m_iEpisode);
        bResult = ResolveTvShowByName(strFolderName, pContext->strYear, pMetadata);
      }

      if (bResult && pContext->bMovie) // Mark this file as resolved by folder
        pContext->bResolvedByFolder = true;

    }
  } // if !bResult

  pVideo->m_bMovie = pContext->bMovie;
  if (pContext->bMovie) 
  {
    // We reset the episode and season data, in case of a movie to avoid confusion in the future
    pVideo->m_iEpisode = -1;
    pVideo->m_iSeason = -1;
  }

  return bResult;
}



bool CMetadataResolverVideo::ResolveTvShowByName(const CStdString& strName, const CStdString& strYear, BXMetadata* pMetadata)
{
  if (strName.IsEmpty())
    return false;

  CLog::Log(LOGDEBUG, "LIBRARY, RESOLVER, LIB1, Resolving TV show %s", strName.c_str());
  SScraperInfo info;

  info.strPath = "tvdb.xml";
  info.strContent = "tvshows";

  bool retVal = ResolveVideoByNameImp(strName, strYear, info, pMetadata);

  return retVal;
}

bool CMetadataResolverVideo::ResolveMovieByName(const CStdString& strName, const CStdString& strYear, BXMetadata* pMetadata, bool bResolveById)  {

  if (strName.IsEmpty())
    return false;

  CLog::Log(LOGDEBUG, "LIBRARY, RESOLVER, LIB1, Resolving movie %s", strName.c_str());
  SScraperInfo info;
  info.strPath = "imdb.xml";
  info.strContent = "movies";

  return ResolveVideoByNameImp(strName, strYear, info, pMetadata, bResolveById);
}

bool CMetadataResolverVideo::GetVideoList(const CStdString& strName, SScraperInfo& info, IMDB_MOVIELIST& list)
{
  if (g_application.m_bStop) return false;
  CLog::Log(LOGDEBUG, "CMetadataResolverVideo::GetVideoList, RESOLVER, get videos for name = %s", strName.c_str());
  info.strTitle = strName;
  info.settings.LoadSettingsXML("special://xbmc/system/scrapers/video/"+info.strPath);

  CIMDB IMDB;
  IMDB.SetScraperInfo(info);

  CStdString strHeading;
  CScraperParser parser;
  SScraperInfo info2(info);
  parser.Load("special://xbmc/system/scrapers/video/" + info.strPath);
  info2.strTitle = parser.GetName();
  IMDB.SetScraperInfo(info2);

  bool bHasInfo = IMDB.FindMovie(strName, list, NULL);

  CLog::Log(LOGDEBUG, "CMetadataResolverVideo::GetVideoList, RESOLVER, got %lu videos for name = %s", list.size(), strName.c_str());
  return bHasInfo && list.size() > 0;
}

bool CMetadataResolverVideo::GetBestMovieUrl(const CStdString& strName, const CStdString& strYear, const IMDB_MOVIELIST& movielist, CScraperUrl& url)
{
  if (g_application.m_bStop) return false;

  if (strName.IsEmpty()) return false;

  CStdString movieName = strName;
  CStdString lowMovieName = movieName.ToLower();

  CStdString strTitleFromList;
  CStdString lowTitleFromList;
  CStdString strYearFromList;
  // Go over all results and check their relevance
  double relevance;
  double bestRelevance = 0.0f;
  int bestIndex = 0;
  CStdString bestTitle = "";
  for (size_t i = 0; i < movielist.size() && !g_application.m_bStop; i++) 
  {
    strTitleFromList = movielist[i].strTitle;
    strTitleFromList = RemoveYear(strTitleFromList, strYearFromList);
    strTitleFromList = RemoveAfterLastComma(strTitleFromList);
    strTitleFromList = CleanName(strTitleFromList);
    lowTitleFromList = strTitleFromList.ToLower();
    lowTitleFromList.Trim();

    // Sometimes the year appears in the result, we remove it since this might affect comparison
    // TODO: We could also compare the years, but we dont right now

    relevance = fstrcmp(lowMovieName, lowTitleFromList, 0.0f);
    //CLog::Log(LOGDEBUG, "LIBRARY, RESOLVER, LIB5: Compare:, Input: %s, Output: %s, Relevance: %f", lowMovieName.c_str(), lowTitle.c_str(), relevance);
    if (relevance > bestRelevance) {
      bestRelevance = relevance;
      bestIndex = i;
      bestTitle = strTitleFromList;
    }

    // If we found a 1.0f match, we found the best match and we can break from the for loop
    if (bestRelevance == 1.0f)
    {
      break;
    }
  }

  if (g_application.m_bStop) 
    return false;

  if (bestRelevance <= 0.75f)
  {
    return false;
  }

  if (MovieNameCompare(lowMovieName, bestTitle) == false) 
  {
    return false;
  }

  // Get the url of the best match movie
  url = movielist[bestIndex];

  return true;
}

bool CMetadataResolverVideo::GetMovieDetails(const CScraperUrl& url, const SScraperInfo& info, BXMetadata* pMetadata)
{
  if (!pMetadata) return false;

  if (g_application.m_bStop) return false;

  BXVideo* pVideo = (BXVideo*) pMetadata->GetDetail(MEDIA_DETAIL_VIDEO);
  BXSeries* pSeries = (BXSeries*) pMetadata->GetDetail(MEDIA_DETAIL_SERIES);
  BXSeason* pSeason = (BXSeason*) pMetadata->GetDetail(MEDIA_DETAIL_SEASON);

  // Get the season and the episode
  int iSeason = pVideo->m_iSeason;
  int iEpisode = pVideo->m_iEpisode;

  CVideoInfoTag movieDetails;
  CIMDB IMDB;
  IMDB.SetScraperInfo(info);

  bool bHasDetails = IMDB.GetDetails(url, movieDetails, NULL);

  if (bHasDetails) 
  {
    // Get information about specific episode
    if (info.strContent == "tvshows") 
    {
      IMDB_EPISODELIST episodes;
      CScraperUrl url;

      //convert m_strEpisodeGuide in url.m_scrURL
      url.ParseString(movieDetails.m_strEpisodeGuide);

      if (IMDB.GetEpisodeList(url,episodes)) {

        CVideoInfoTag episodeDetails;

        std::pair<int, int> key(iSeason, iEpisode);

        int ep=-1;
        for (size_t i=0;i<episodes.size();i++)
        {
          if (episodes[i].key == key)
          {
            ep = i;
            break;
          }
        }

        if ( ep < 0 )
        {
          CLog::Log(LOGWARNING,"CMetadataResolverVideo::GetMovieDetails, RESOLVER, no entries for key (season = %d, episode = %d) [path=%s]", iSeason, iEpisode, (pVideo->m_strPath).c_str());
          return false;
        }

        url = episodes[ep].cScraperUrl;
        if (IMDB.GetEpisodeDetails(url,episodeDetails,NULL)) {

          pVideo->m_strTitle = episodeDetails.m_strTitle;
          pVideo->m_iYear = episodeDetails.m_iYear;

          if (episodeDetails.m_strShowTitle.Trim() == "") {
            pVideo->m_strShowTitle = movieDetails.m_strTitle;
          }
          else {
            pVideo->m_strShowTitle = episodeDetails.m_strShowTitle;
          }
          pVideo->m_strDescription = episodeDetails.m_strPlot;
          pVideo->m_strExtDescription = episodeDetails.m_strPlotOutline;
          pVideo->m_strCover = episodeDetails.m_strPictureURL.GetFirstThumb().m_url;
          pVideo->m_strTagLine = episodeDetails.m_strTagLine;
          pVideo->m_strCredits = episodeDetails.m_strWritingCredits;
          pVideo->m_strMPAARating = episodeDetails.m_strMPAARating;
          pVideo->m_strIMDBKey = movieDetails.m_strIMDBNumber;
          pVideo->m_strStudio = movieDetails.m_strStudio;
          pVideo->m_iRating = (int)movieDetails.m_fRating;
          pVideo->m_iSeason = iSeason;
          pVideo->m_iEpisode = iEpisode;
          pVideo->m_strDirector= movieDetails.m_strDirector;
          pVideo->m_strFirstAired = episodeDetails.m_strFirstAired;          
          pVideo->m_iDuration = BXUtils::StringToInt(episodeDetails.m_strRuntime);
          pVideo->m_strGenre = movieDetails.m_strGenre;
          pVideo->m_bMovie = false;

          pSeason->m_iSeasonNum = iSeason;
          pSeason->m_strCover = movieDetails.m_strPictureURL.GetSeasonThumb(iSeason).m_url;
          if (pSeason->m_strCover == "") {
            pSeason->m_strCover = movieDetails.m_strPictureURL.GetFirstThumb().m_url;
          }

          pSeries->m_strTitle = movieDetails.m_strTitle;
          pSeries->m_strDescription = movieDetails.m_strPlot;
          pSeries->m_strCover = movieDetails.m_strPictureURL.GetSeasonThumb(1).m_url;
          if (pSeries->m_strCover == "") {
            pSeries->m_strCover = movieDetails.m_strPictureURL.GetFirstThumb().m_url;
          }

          vector<SActorInfo>& cast = episodeDetails.m_cast;
          for (unsigned int i = 0; i < cast.size(); i++) 
          {
            pVideo->m_vecActors.push_back((cast[i].strName));
          }
        }
      }
    }
    else 
    {
      pVideo->m_strTitle = movieDetails.m_strTitle;
      pVideo->m_strDirector= movieDetails.m_strDirector;
      pVideo->m_strGenre = movieDetails.m_strGenre;
      pVideo->m_strDescription = movieDetails.m_strPlot;
      pVideo->m_iDuration = BXUtils::StringToInt(movieDetails.m_strRuntime);
      pVideo->m_strExtDescription = movieDetails.m_strPlotOutline;
      pVideo->m_strIMDBKey = movieDetails.m_strIMDBNumber;
      pVideo->m_strMPAARating = movieDetails.m_strMPAARating;
      pVideo->m_strCredits = movieDetails.m_strWritingCredits;
      pVideo->m_strStudio = movieDetails.m_strStudio;
      pVideo->m_strTagLine = movieDetails.m_strTagLine;
      pVideo->m_strCover = movieDetails.m_strPictureURL.GetFirstThumb().m_url;
      if (pVideo->m_strCover.substr(0,7) != "http://")
        pVideo->m_strCover = "";
      pVideo->m_iYear = movieDetails.m_iYear;
      pVideo->m_iRating = (int)movieDetails.m_fRating;
      pVideo->m_bMovie = true;

      vector<SActorInfo>& cast = movieDetails.m_cast;
      for (unsigned int i = 0; i < cast.size(); i++) 
      {
        pVideo->m_vecActors.push_back((cast[i].strName));
      }
    }

    CLog::Log(LOGDEBUG, "CMetadataResolverVideo::GetMovieDetails, RESOLVER, Boxee Metadata Resolver, Resolved video, title =  %s ", pVideo->m_strTitle.c_str());

    return true;
  }
  else {
    return false;
  }
}

bool CMetadataResolverVideo::ResolveVideoByNameImp(const CStdString& strName, const CStdString& strYear, SScraperInfo& info, BXMetadata* pMetadata, bool bResolveById)
{
  if (g_application.m_bStop) return false;

  if (strName.IsEmpty())
    return false;

  CStdString movieName = strName;
  IMDB_MOVIELIST movielist;

  // ------ Get list of movies that match this name

  CLog::Log(LOGDEBUG, "CMetadataResolverVideo::ResolveVideoByNameImp, RESOLVER, get list of videos, title: %s", movieName.c_str());

  bool bHasInfo = GetVideoList(movieName, info, movielist);

  CLog::Log(LOGDEBUG, "LIBRARY: Boxee Metadata Resolver, Found video on the net, title: %s", movieName.c_str());

  // ------ Calculate the most relevant movie
  CScraperUrl url;
  if (bHasInfo)
  {
    CLog::Log(LOGDEBUG, "CMetadataResolverVideo::ResolveVideoByNameImp, RESOLVER, calculate best movie, title: %s", movieName.c_str());
    if (!bResolveById) 
    {      
      bool movieUrlWasFound = false;

      if(info.strContent == "tvshows")
      {
        ///////////////////////////////////////////
        // In case we resolve a TV show, we want //
        // to try and resolve with language code //
        ///////////////////////////////////////////

        CStdString uiLanguage = g_guiSettings.GetString("locale.language");

        CStdString languageCode = BoxeeUtils::GetCodeOfLanguage(uiLanguage);

        if (!languageCode.Equals(""))
        {
          //////////////////////////////////////
          // Try to find MovieUrl with region //
          //////////////////////////////////////

          CStdString movieNameWithLanguageCode = movieName;
          movieNameWithLanguageCode += " ";
          movieNameWithLanguageCode += languageCode;

          movieUrlWasFound = GetBestMovieUrl(movieNameWithLanguageCode, strYear, movielist, url);
        }

        if((movieUrlWasFound == false) && (!languageCode.Equals("en")))
        {
          ///////////////////////////////////////////
          // Try to find MovieUrl with region (en) //
          ///////////////////////////////////////////

          CStdString movieNameWithLanguageCode = movieName;
          movieNameWithLanguageCode += " ";
          movieNameWithLanguageCode += "(en)";

          movieUrlWasFound = GetBestMovieUrl(movieNameWithLanguageCode, strYear, movielist, url);
        }
      }

      if(movieUrlWasFound == false)
      {
        ////////////////////////////////////////////
        // In case we still failed to resolve,    //
        // try to find MovieUrl with its own name //
        ////////////////////////////////////////////

        movieUrlWasFound = GetBestMovieUrl(movieName, strYear, movielist, url);
      }

      if(movieUrlWasFound == false)
      {
        CLog::Log(LOGDEBUG, "CMetadataResolverVideo::ResolveVideoByNameImp, RESOLVER, could not calculate best movie, title: %s", movieName.c_str());
        return false;
      }
    }
    else 
    {
      url = movielist[0];
    }
  }
  else 
  {
    CLog::Log(LOGDEBUG, "CMetadataResolverVideo::ResolveVideoByNameImp, could not get list of videos, title: %s", movieName.c_str());
    return false;
  }

  // ------ Get movie details
  CLog::Log(LOGDEBUG, "CMetadataResolverVideo::ResolveVideoByNameImp, RESOLVER, get video details, title: %s", movieName.c_str());

  if (GetMovieDetails(url, info, pMetadata))
  {
    CLog::Log(LOGDEBUG, "CMetadataResolverVideo::ResolveVideoByNameImp, RESOLVER, got video details, title: %s", movieName.c_str());
    return true;
  }
  else 
  {
    CLog::Log(LOGDEBUG, "CMetadataResolverVideo::ResolveVideoByNameImp, RESOLVER, could not get video details, title: %s", movieName.c_str());
    return false;
  }
}

*/

// Uses open subtitles site to resolve video using file hash
uint64_t CMetadataResolverVideo::CalculateVideoHash(const CStdString& strPath)
{
  // We don't calculate hash for ZIP/RAR files or UPnP files
  if((CUtil::IsZIP(strPath)) || (CUtil::IsRAR(strPath)) || (CUtil::IsInArchive(strPath)))
  {
    CLog::Log(LOGDEBUG,"CMetadataResolverVideo::CalculateVideoHash - [strPath=%s] is RAR,ZIP or InRAR path and therefore we don't calculate hash for it. [IsZIP=%d][IsRAR=%d][IsInArchive=%d] (rar)",strPath.c_str(),CUtil::IsZIP(strPath),CUtil::IsRAR(strPath),CUtil::IsInArchive(strPath));
    return 0;
  }

  // Calculate hash
  CFile file;
  if (!file.Open(strPath))
  {
    CLog::Log(LOGWARNING,"failed to open file <%s> to calculate hash\n", strPath.c_str());
    return 0;
  }

  int64_t fsize; // signed. in order to get negative values on small files (second seek)
  uint64_t hash;

  fsize = file.GetLength();

  if (fsize <= 0) // sanity
    return 0;

  hash = (uint64_t)fsize;

  int bufferSize = 65536/sizeof(uint64_t);

  uint64_t *buffer = new uint64_t[bufferSize];
  memset(buffer,0, 65536);
  file.Read((char*)buffer, 65536);
  for(int i = 0; i < bufferSize; i++)
  {
    hash += buffer[i];
  }

  memset(buffer,0, 65536);
  if ( file.Seek(BXUtils::MAX_LONGLONG(0,fsize - 65536)) <= 0){
    delete [] buffer;    
    return hash;
  }

  file.Read((char*)buffer, 65536);
  for(int i = 0; i < bufferSize; i++)
  {
    hash += buffer[i];
  }

  delete [] buffer;
  file.Close();

  return hash;
}

/// UnitTest

bool CMetadataResolverVideo::RunUnitTest()
{
  bool bResult = true;

  bResult &= TestSeriesTagExtraction();

  return bResult;

}

class TagTestCase
{
public:
  TagTestCase(const CStdString& _path, const CStdString& _name, bool _hasTag, int _season, int _episode, const CStdString& _prefix,
              const CStdString& _suffix, const CStdString& _date)
  {
    path = _path;
    name = _name;
    hasTag = _hasTag;
    season = _season;
    episode = _episode;
    prefix = _prefix;
    suffix = _suffix;
    date   = _date;
  }

  CStdString path;
  CStdString name;
  bool hasTag;
  int season;
  int episode;
  CStdString prefix;
  CStdString suffix;
  CStdString date;

};

bool CMetadataResolverVideo::TestSeriesTagExtraction()
{
  CLog::Log(LOGDEBUG, "********** TestSeriesTagExtraction ***************");

  std::vector<TagTestCase> testCases;
  std::vector<TagTestCase> failedCases;

  CStdString strEpisodeName;

  int iSeason = -1;
  int iEpisode = -1;
  CStdString strPrefix;
  CStdString strSuffix;
  CStdString episodeDate;

  testCases.push_back(TagTestCase("","Mad.Men.S01 E01.720p.HDTV.x264-FoV", true, 1,1, "Mad.Men.", ".720p.HDTV.x264-FoV",""));
  testCases.push_back(TagTestCase("","Mad.Men.S01E01.720p.HDTV.x264-FoV", true, 1,1, "Mad.Men.", ".720p.HDTV.x264-FoV",""));
  testCases.push_back(TagTestCase("","Mad.Men.S22E23", true, 22,23, "Mad.Men.", "",""));
  testCases.push_back(TagTestCase("","Mad.Men.S1E01", true, 1,1, "Mad.Men.", "",""));
  testCases.push_back(TagTestCase("","Mad.Men.S1E1", true, 1,1, "Mad.Men.", "",""));
  testCases.push_back(TagTestCase("","Top.Gear.14x04.720p.HDTV.x264-FoV", true, 14,4, "Top.Gear.", ".720p.HDTV.x264-FoV",""));
  testCases.push_back(TagTestCase("","2x01", true, 2,1, "", "",""));
  testCases.push_back(TagTestCase("","American Dad S2E3", true, 2,3, "American Dad ", "",""));

  // daily shows
  testCases.push_back(TagTestCase("","The Daily Show With Jon Stewart 01/14/2008", true, -1,-1, "The Daily Show With Jon Stewart ", "","01/14/2008"));

  for (int i = 0; i < (int)testCases.size(); i++)
  {
  iSeason = -1; iEpisode = -1;
    strPrefix = ""; strSuffix = "";
    episodeDate = "";

    TagTestCase currentCase = testCases[i];

    bool bHasTag = ExtractPathSeriesTag("",currentCase.name, iSeason, iEpisode, strPrefix, strSuffix);
    if (!bHasTag)
    {
      bHasTag = ExtractDailyShows(currentCase.name, iSeason, episodeDate, strPrefix);
    }

    if (bHasTag != currentCase.hasTag || iSeason != currentCase.season || iEpisode != currentCase.episode || strPrefix != currentCase.prefix
        || strSuffix != currentCase.suffix  || episodeDate != currentCase.date)
  {
      CLog::Log(LOGERROR,"CMetadataResolverVideo::TestSeriesTagExtraction, UnitTest case failed: name = %s, season = %d, episode = %d, prefix = %s, suffix = %s  date=%s" ,
          currentCase.name.c_str(), iSeason, iEpisode, strPrefix.c_str(), strSuffix.c_str(),episodeDate.c_str());
      failedCases.push_back(currentCase);
    }
  }

  return (failedCases.size() == 0);

}

/// End UnitTest
