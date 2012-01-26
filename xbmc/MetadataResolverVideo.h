#ifndef METADATARESOLVERVIDEO_H_
#define METADATARESOLVERVIDEO_H_

#include "FileItem.h"
#include "lib/libBoxee/bxmetadata.h" 
#include "utils/IMDB.h"
#include <vector>
#include "Util.h"

class CVideoFolderContext
{
public:
  CVideoFolderContext() {}
  CVideoFolderContext(const CStdString& _strPath)
  {
    strPath = _strPath;
  }

  void Load();

  CStdString strPath;

  CStdString strEffectiveParentPath; // path to effective parent folder (skipping CD1 or VIDEO_TS etc...)
  CStdString strName; // name extracted from the effective parent path
  CStdString strAdditionalInfo;
  CStdString strEpisodeDate;

  CStdString strYear; // extracted year

  int iSeason;
  int iEpisode;
};

/*
 * This class holds all data extracted from the video file
 * and used for the purpose of video resolving
 */
class CVideoFileContext
{
public:
  CVideoFileContext()
  {
    Reset();
  }

  CVideoFileContext(const CStdString& _strPath)
  {
    strPath = _strPath;
    Reset();
  }

  void Load(const CStdString& _strPath = "");
  void LoadWithTag( const CVideoInfoTag* vTag );
  void LoadNameFromTag( const CVideoInfoTag* vTag );
  void LoadNameFromPath();
  void LoadFileData();
  void LoadFileHash();
  void LoadFileNfo();
  bool LoadVideoInfoTag(const std::vector<CStdString>& strFilePaths, CVideoInfoTag& nfoDetails);

  void GetFileOptionsMap(std::map<CStdString, CStdString>& mapOptions);
  void GetFolderOptionsMap(std::map<CStdString, CStdString>& mapOptions);

  bool CanResolveByFile();
  bool CanResolveByFolder();

  CVideoFolderContext folderContext;

  CStdString strPath; // video path
  CStdString strName; // extracted title of the movie or tv show
  CStdString strYear; // extracted year
  CStdString strIMDBId; // IMDB id of the video
  CStdString strTVDBId; // tTVDB id of the episode
  CStdString strBoxeeId; // boxee id of the video

  CStdString strExternalIdName;
  CStdString strExternalIdValue;

  CVideoInfoTag m_nfoVideoDetails;
  bool          m_bVideoDetailsFromNFO; //used for movie / episode

  CVideoInfoTag m_nfoShowDetails;
  bool          m_bShowDetailsFromNFO;

  CStdString m_userVideoCoverPath;
  CStdString m_strNFOPath;

  CStdString strAdditionalInfo; // string that remains to the right of the season tag in tv shows

  uint64_t uiHash;    // calculated hash of the video file
  CStdString strHash; // hash as string

  int64_t iSize;      // file size
  CStdString strSize; // file size as string

  int iModificationDate; // modification date of the file

  bool bMovie; // flag that indicates whether the item was recognized as a movie or tv show
  bool bMovieChanged; //indicates if there was a change

  bool bUseHash;

  CPlayableFolderType::PlayableFolderEnums m_playableFolderType;
  bool m_bIsManualResolve;

  int iSeason;
  int iEpisode;
  CStdString strEpisodeDate;   // used to daily shows series

  bool bResolvedByFolder; // internal indication of whether the video was resolved by folder name
  bool m_bNFOParseFailed;

  void Reset()
  {
    m_playableFolderType = CPlayableFolderType::PF_NO;
    m_bIsManualResolve = false;
    m_bVideoDetailsFromNFO = false;
    m_bShowDetailsFromNFO = false;
    m_bNFOParseFailed = false;

    strName = "";
    strYear = "";
    strIMDBId = "";
    strBoxeeId = "";
    strAdditionalInfo = "";
    strHash = "";
    strSize = "";

    bResolvedByFolder = false;
    bMovie = true;
    bUseHash = true;
    bMovieChanged = false;
    m_bVideoDetailsFromNFO = false;
    m_bShowDetailsFromNFO = false;

    iSize = 0;
    uiHash = 0;
    iEpisode = -1;
    iSeason = -1;
    iModificationDate = 0;
    strEpisodeDate = "";
  }

private:

  bool GetNfoFilePaths(std::vector<CStdString>& nfoFilePathsVec);

};

class CMetadataResolverVideo
{
public:

  // Initializes static lists used by the video resolver
  static void InitializeVideoResolver();
  static void DeinitializeVideoResolver();
  static void SetBadWords(const std::vector<std::string>& vecBadWords);

  // Resolves video metadata using boxee server
  static bool ResolveVideo(CVideoFileContext& context, BOXEE::BXMetadata* pMetadata);

  static bool ResolveVideoSendServerReq(CVideoFileContext& context, bool file , CFileItemList& list);

  //static bool LoadVideoContext(const CStdString& strPath, CVideoContext& context);

  static bool ExtractDailyShows(const CStdString& strTitle, int& iSeason, CStdString& strDate, CStdString& strPrefix);
  //static void GetCleanName(const CStdString& strVideoPath, CVideoContext& context);
  static bool ExtractPathSeriesTag(const CStdString& strPath, const CStdString& strTitle,
                                   int& iSeason, int& iEpisode, CStdString& strPrefix,
                                   CStdString& strSuffix);

  static bool ExtractSeriesTag(const CStdString& strTitle, int& iSeason, int& iEpisode, CStdString& strPrefix, CStdString& strSuffix);

  static CStdString CleanName(const CStdString& strName);

  // Auxiliary functions
  static bool IsSample(const CStdString& strName);
  static bool IsTrailer(const CStdString& strName1);
  static bool IsDVDFolder(const CFileItemList& items);
  static bool IsDVDFilename(const CStdString& strPath);
  static bool IsDVDExtension(const CStdString& strPath);

  static bool IsBlurayFolder(const CFileItemList& items);
  static bool IsBlurayExtension(const CStdString& strPath);
  static bool IsOnBlurayFolder(const CStdString& strPath);

  static bool CheckPart(const CStdString& strFilename, int & iPart);

  static bool GetFileData(const std::string& strFilePath, int64_t& iLength, int& iModDate);
  static uint64_t CalculateVideoHash(const CStdString& strPath);
  static CStdString RemoveYear(const CStdString& strName, CStdString& strYear);

  static bool RunUnitTest();
  static bool TestSeriesTagExtraction();

  static bool LoadMovieInfo(const CFileItemList& list, CVideoFileContext& context, BOXEE::BXMetadata* pMetadata);
  static bool LoadEpisodeInfo(const CFileItemList& list, CVideoFileContext& context, BOXEE::BXMetadata* pMetadata);

  static bool AddLocalShowsGenres(const std::string& strGenre);
  static bool AddLocalMoviesGenres(const std::string& strGenre);
  static bool InitializeGenres();

  static void GetLocalShowsGenres(std::set<std::string>& outShowGenres);
  static void GetLocalMoviesGenres(std::set<std::string>& outMovieGenres);

  static void LockLocalShowsGenres();
  static void UnlockLocalShowsGenres();

  static void LockLocalMoviesGenres();
  static void UnlockLocalMoviesGenres();

  static bool LoadTVShowItem(const CFileItemPtr& tvshowItem, CVideoFileContext& context, BOXEE::BXSeries* pSeries, BOXEE::BXSeason* pSeason , BOXEE::BXVideo* pVideo);
  static bool LoadTVEpisodeItem(const CFileItemPtr& episodeItem, CVideoFileContext& context, BOXEE::BXVideo* pVideo);

private:

  // checks whether provided path is a top level path of any share defined by the user
  static bool PathIsShare(const CStdString& strPath);
  static bool IsBadWord(const CStdString& strWord);



  static bool ParseSeriesTag(const CStdString& strTerm, const CStdString& strRegEx, int& iSeason, int& iEpisode, int& iTagPosition, int& iTagLength);
  static bool ParseNumericTag(const CStdString& strTerm, const CStdString& strRegEx, int& iTag, int& iTagPosition, int& iTagLength);

  static std::vector<std::string> RemoveBadWords(const std::vector<std::string>& vecWords);

  // Old Resolver functions

  static bool ResolveVideoByNameImp(const CStdString& strName, const CStdString& strYear, SScraperInfo& info, BOXEE::BXMetadata* pMetadata, bool bResolveById = false);
  static CStdString RemoveAfterLastComma(const CStdString& strVideoPath);

  static std::vector<std::string> m_badWords;
  static std::vector<CStdString> m_commonWords;
  static std::set<std::string> m_localMoviesGenres;
  static std::set<std::string> m_localShowsGenres;

  static SDL_mutex* m_localMoviesGenresGuard;
  static SDL_mutex* m_localShowsGenresGuard;

  static bool m_bInitialized;
};

#endif /*METADATARESOLVERVIDEO_H_*/
