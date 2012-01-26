//
// C++ Interface: BoxeeUtils
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEUTILS_H
#define BOXEEUTILS_H

#include <string>
#include "FileItem.h"
#include "lib/libBoxee/bxmessages.h"
#include "lib/libBoxee/bxfriend.h"
#include "lib/libBoxee/bxboxeefeed.h"
#include "lib/libBoxee/bxobject.h"
#include "lib/libBoxee/bxmetadata.h"
#include "lib/libBoxee/bxsubscriptionsmanager.h"
#include "WatchDog.h"
#include "AppRepository.h"
#include "lib/libBoxee/bxwebfavoritesmanager.h"
#include "lib/libjson/include/json/value.h"
#include "FileSystem/IDirectory.h"
#include "URL.h"
#include "Util.h"

class CBoxeeMediaSource;

namespace MUSIC_GRABBER {
  class CMusicAlbumInfo;
}

namespace MUSIC_INFO {
  class CMusicInfoTag;
}

class BoxeeWatchdogListener : public IWatchDogListener
{
  virtual void PathUpdate(const CStdString &stdPath, bool bAvailable);
};

class CQualifyPicNameAsThumb
{
public:
  enum QualifyPicNameAsThumbEnums
  {
    FOLDER=0,
    PREVIEW=1,
    FOLDER_NAME=2,
    COVER=3,
    FRONT=4,
    NUM_OF_QUALIFY_PIC_NAME_AS_THUMB=5
  };
};

class CReportToServerActionType
{
public:
  enum ReportToServerActionTypeEnums
  {
    PLAY=0,
    RATE=1,
    RECOMMEND=2,
    SHARE=3,
    QUEUE=4,
    DEQUEUE=5,
    SUBSCRIBE=6,
    UNSUBSCRIBE=7,
    INSTALL=8,
    REMOVE=9,
    LAUNCH=10,
    NUM_OF_REPORT_TO_SERVER_ACTIONS=11
  };
};

class CFileItemTypes
{
public:
  enum FileItemTypesEnums
  {
    LOCAL_RESOLVE_MOVIE=0,
    LOCAL_RESOLVE_TVSHOW_EPISODE=1,
    LOCAL_RESOLVE_AUDIO=2,
    LOCAL_RESOLVE_PICTURE=3,
    LOCAL_UNKNOWN_VIDEO=4,
    LOCAL_UNKNOWN_AUDIO=5,
    LOCAL_UNKNOWN_PICTURE=6,
    LOCAL_UNKNOWN=7,
    WEB_RESOLVE_VIDEO=8,
    WEB_RESOLVE_MOVIE=9,
    WEB_RESOLVE_TVSHOW_EPISODE=10,
    WEB_RESOLVE_AUDIO=11,
    WEB_RESOLVE_PICTURE=12,
    WEB_UNKNOWN_VIDEO=13,
    WEB_UNKNOWN_AUDIO=14,
    WEB_UNKNOWN_PICTURE=15,
    WEB_UNKNOWN=16,
    UNKNOWN=17,
    LIVETV=18,
    NUM_OF_FILE_ITEM_TYPES=19
  };
};

class CRating
{
public:
  enum RatingEnums
  {
    MPAA=0,
    TV=1,
    V_CHIP=2,
    SIMPLE=3,
  };
};

class BoxeeUtils{
public:
    BoxeeUtils();
    virtual ~BoxeeUtils();

    static bool BuildItemInfo(CFileItem& item,CFileItemList& items, bool loadLocalLinks);
    static bool ResolveItem(const CStdString& strBoxeeId, CFileItemList& items);
    static bool GetLocalVideoMetadata(CFileItem& item);
    static void GetLocalLinks(CFileItem& item);

    // returns thumb file for the message
    static std::string GetMessageThumb(const BOXEE::BXGeneralMessage &msg);

    // extract first media related object (movie, album, track, photo) from the message
    static BOXEE::BXObject GetMediaObject(const BOXEE::BXGeneralMessage &msg);
  
    // extract user object "nIndex" from the message. 0 based index.
    static BOXEE::BXObject GetUserObject(const BOXEE::BXGeneralMessage &msg, int nIndex = 0);

    // return a string containing the description with format tags according to the objects in the message
    static CStdString GetFormattedDesc(const BOXEE::BXGeneralMessage &msg, bool bSelected);
    static CStdString StripDesc(const CStdString &strDesc);

    // returns thumbnail according to presence string
    static std::string GetBoxeePresenceThumb(const std::string &strPresence);

    static std::string GetBoxeeGeneralThumb();

    static std::string AddCredentialsToLink(const std::string &strLink);

    static bool Recommend(const CFileItem *pItem, const std::vector<BOXEE::BXFriend> &vecRecommendTo);
    static void Rate(const CFileItem *pItem, bool bThumbsUp);

    static void Share(const CFileItem *pItem, const std::vector<BOXEE::BXFriend> &vecRecommendTo, CFileItemList& listPublishToServices  , bool bInBoxee, const std::string& userText = "");
    static void GetShareServicesXML(CFileItemList& list); //not used
    static Job_Result GetShareServicesJson(Json::Value& response, int& retCode, bool runInBG=false);
    static int  ParseJsonShareServicesToFileItems(const Json::Value& jsonValue, CFileItemList& outputList);
    static void LoadSocialShareServicesStatus(CFileItemList& list);
    static bool PrecacheShareServicesImages(const Json::Value& response);

    static bool Queue(CFileItem* pItem, bool runInBG = false);
    static bool Dequeue(const CFileItem* pItem, bool runInBG = false);

    static bool Subscribe(BOXEE::CSubscriptionType::SubscriptionTypeEnums subscriptionType, const std::string& src, const std::string& strShowTitle, bool runInBG = false);
    static bool Unsubscribe(BOXEE::CSubscriptionType::SubscriptionTypeEnums subscriptionType, const std::string& src, const std::string& strShowTitle, bool runInBG = false);
    static bool IsSubscribe(const std::string& src);
    static bool GetSubscriptionIds(BOXEE::CSubscriptionType::SubscriptionTypeEnums subscriptionType,std::vector<std::string>& subscriptionsIdsVec);
    static bool GetSubscriptions(BOXEE::CSubscriptionType::SubscriptionTypeEnums subscriptionType, std::vector<BOXEE::BXSubscriptionItem>& subscriptionsVec);

    static bool AddWebFavorite(const CStdString& src, const CStdString& urlTitle, bool runInBG = false);
    static bool RemoveWebFavorite(int index);
    static bool IsWebFavorite(const std::string& src);
    static bool GetWebFavorites(std::vector<BOXEE::BXObject>& webFavoritesVec);

    static void ReportLaunchApp(const std::string& id, const CFileItem* pItem);
    static bool ReportInstalledApps(const VECSOURCES& appsVec, bool runInBG = false);
    static bool ReportInstallApp(const CAppDescriptor& descriptor, bool runInBG = false);
    static bool ReportInstallApp(const std::string& name, const std::string& id, const std::string& url, const std::string& thumb, bool runInBG = false);
    static bool ReportRemoveApp(const std::string& id, bool runInBG = false);

    static bool ReportInstallRss(const VECSOURCES& allRssSources, bool runInBG = false);

    static bool ReportInstalledRepositories(const std::vector<CAppRepository>& repositories, bool runInBG = false);
    static bool ReportInstallRepository(const CAppRepository& repository, bool runInBG = false);
    static bool ReportInstallRepository(const std::string& name, const std::string& id, const std::string& url, bool runInBG = false);
    static bool ReportRemoveRepository(const std::string& id, bool runInBG = false);

    static bool CanPlay(const CFileItem& item);
    static bool HasMoreInfo(const CFileItem& item);
    static bool HasTrailer(const CFileItem& item);
    static bool HasLastFm(const CFileItem& item);
    static bool CanDelete(const CFileItem& item);
    static bool CanBrowse(const CFileItem& item);
    static bool CanEject(const CFileItem& item);
    static bool CanRecognize(const CFileItem& item);
    static bool CanRemove(const CFileItem& item);
    static bool CanResume(const CFileItem& item);
    static bool CanRecommend(const CFileItem& item, bool checkIsLoaded = true);
    static bool CanRate(const CFileItem& item);
    static bool CanQueue(const CFileItem& item, CStdString& referral);
    static bool CanMarkWatched(const CFileItem& item);
    static void MarkWatched(CFileItem* item);
    static bool CanShare(const CFileItem& item, bool checkIsLoaded = true);
    static bool HasDescription(const CFileItem& item);

    static bool IsPathLocal(const CStdString& path);

    // drops the metadata for the provided file item from the database
    //static void Drop(const CFileItem *pItem);

    static std::string FormatTime(time_t tmWhen);
    static int         DayDiff(time_t tmNow, time_t tmOther);

    static time_t GetModTime(const CStdString& strPath);
    
    static bool VideoDetailsToObject(const CFileItem *pItem, BOXEE::BXObject &obj);
    static BOXEE::BXObject FileItemToObject(const CFileItem *pItem);

    static bool ObjToFileItem(const BOXEE::BXObject &obj, CFileItem* pItem);

    static bool AddUnrecognizedTracksFromPath(const CStdString& strPath, const MUSIC_GRABBER::CMusicAlbumInfo& info, CFileItemList& tracks);

    // CONVERSION FUNCTIONS TODO: Consider moving to a separate class
    // Auxiliary conversion functions
    static bool ConvertBXVideoToVideoInfoTag(const BOXEE::BXVideo* pVideo, CVideoInfoTag &info);
    static bool ConvertBXAudioToCSong(const BOXEE::BXAudio* pAudio, CSong& song);
    static bool ConvertBXAlbumToCAlbum(BOXEE::BXAlbum* pAlbum, CAlbum& album);
    static bool ConvertCAlbumToBXAlbum(const MUSIC_GRABBER::CMusicAlbumInfo& info, BOXEE::BXMetadata* pAlbum);
    static bool ConvertBXAlbumToMusicInfoTag(const BOXEE::BXAlbum* pAlbum, MUSIC_INFO::CMusicInfoTag& infoTag);
    
    // converts complete album info including songs and artist to bxmetadata
    static bool ConvertAlbumInfoToBXMetadata(const MUSIC_GRABBER::CMusicAlbumInfo& info, BOXEE::BXMetadata* pAlbum);
    // convert complete bxmetadata to album info
    static bool ConvertBXMetadataToAlbumInfo(const BOXEE::BXMetadata* pAlbumMetadata, MUSIC_GRABBER::CMusicAlbumInfo& info);

    // END OF CONVERSION FUNCTIONS

    static void OnTuneIn(const BOXEE::BXObject &userObj, bool bForce);
    
    static void FillVideoItemDetails(CFileItemPtr pItem);
    static void SetDefaultIcon(CFileItem *pItem);

    static std::string VerifyBoxeeUrl(const std::string &strUrl);

    static void BuildPlaylist(const CFileItem *pRoot, CFileItemList &items, bool bAddAudio, bool bAddVideo, bool bAddPictures);
    
    // parses the url in form boxeedb://folder/file?param1=x&param2=y
    static bool ParseBoxeeDbUrl(const std::string& strURL, std::string& strDir, std::string& strFile, std::map<std::string, std::string>& mapParams);

    static CStdString BuildParameterString(const std::map<CStdString, CStdString>& mapParameters);
    static CStdString AppendParameters(const CStdString& _strPath, const std::map<CStdString, CStdString>& mapParameters);

    static void InitializeLanguageToCodeMap();
    static CStdString GetCodeOfLanguage(const CStdString& language);

    static void AddTrailerStrToItemLabel(CFileItem& pItem);

    static bool CreateDirectoryThumbnail(CFileItem& dir,CFileItemList &items);

    static bool SafeDownload(const CStdString &url, const CStdString &target, const CStdString &hash);

    static bool LogReportToServerAction(const CFileItem& pItem, const BOXEE::BXObject& obj, CReportToServerActionType::ReportToServerActionTypeEnums reportToServerActionType);

    static const char* GetPlatformStr();

    static bool RemoveMatchingPatternFromString(CStdString& str, const CStdString regExPattern);

    static void UpdateStackFileForServerReport(const CFileItem& item, CFileItem& stackItem);

    static int StringTokenize(const CStdString& path, std::set<CStdString>& tokens, const CStdString& delimiters, bool shouldTrim, bool shouldLower);

    static void PlayPicture(const CFileItem& itemPath);
    static CStdString URLEncode(const CURI &url);
    static CStdString TranslateStringById(const CStdString  &srcStr);

    static void UpdateProfile(int profileIndex, BOXEE::BXObject& userObj);

    static bool GetAvailableLanguages(CFileItemList& listLanguages);

    static bool GetWeatherCitiesResults(const CStdString &strSearch, std::vector<CStdString>& resultVec);
    static bool SetWeatherLocation(const CStdString& cityName, const CStdString& countryCode);

    static void IndexItems(std::map< CStdString, CFileItemPtr >& indexMap , CFileItemList& vector);

    static bool DoYouWantToScan(const CStdString& path);
    static bool DoYouWantToScan(CBoxeeMediaSource* source);

    static bool GetSourceTotalScanResolveData(CBoxeeMediaSource* source, int& resolved_count, int& unresolved_count, int& new_count, int& total_count);

    static CStdString GetTimeAddedToDirectoryLabel(int tmAdded);

    static bool FindFiles(const CStdString& directory, const CStdString& startsWith, CFileItemList& output, bool lookupHidden = true, bool lookupVideo = true, bool lookupAudio = true, bool lookupPictures = true, bool insensitive = true);
    static CStdString GetUserVideoThumbPath(const CStdString& videoFilePath, const CFileItemList* pathsList);

    static CStdString GetPlatformDefaultHostName();

    static bool IsAdult(const std::string& rating,CRating::RatingEnums type);

    static bool LaunchBrowser(const CStdString& url = "http://about:blank");

    static bool RefreshCountryCode();

    static CStdString GetFilesButtonPathToExecute();

    static bool LaunchGetFacebookExtraCredentials(bool queryDataBase = true);

private:

  static CFileItemTypes::FileItemTypesEnums GetRawFileItemType(const CFileItem* pItem);
  static CStdString GetFileItemTypesEnumsAsString(CFileItemTypes::FileItemTypesEnums fileItemTypeEnum);
  static void HandleLocalResolvedMovieItem(const CFileItem *pItem, BOXEE::BXObject& obj);
  static void HandleLocalResolvedTvShowEpisodeItem(const CFileItem *pItem, BOXEE::BXObject& obj);
  static void HandleLocalResolvedAudioItem(const CFileItem *pItem, BOXEE::BXObject& obj);
  static void HandleLocalResolvedPictureItem(const CFileItem *pItem, BOXEE::BXObject& obj);
  static void HandleLocalUnknownVideoItem(const CFileItem *pItem, BOXEE::BXObject& obj);
  static void HandleLocalUnknownAudioItem(const CFileItem *pItem, BOXEE::BXObject& obj);
  static void HandleLocalUnknownPictureItem(const CFileItem *pItem, BOXEE::BXObject& obj);
  static void HandleLocalUnknownItem(const CFileItem *pItem, BOXEE::BXObject& obj);
  static void HandleWebResolvedVideoItem(const CFileItem *pItem, BOXEE::BXObject& obj);
  static void HandleWebResolvedMovieItem(const CFileItem *pItem, BOXEE::BXObject& obj);
  static void HandleWebResolvedTvShowEpisodeItem(const CFileItem *pItem, BOXEE::BXObject& obj);
  static void HandleWebResolvedAudioItem(const CFileItem *pItem, BOXEE::BXObject& obj);
  static void HandleWebResolvedPictureItem(const CFileItem *pItem, BOXEE::BXObject& obj);
  static void HandleWebUnknownVideoItem(const CFileItem *pItem, BOXEE::BXObject& obj);
  static void HandleWebUnknownAudioItem(const CFileItem *pItem, BOXEE::BXObject& obj);
  static void HandleWebUnknownPictureItem(const CFileItem *pItem, BOXEE::BXObject& obj);
  static void HandleWebUnknownItem(const CFileItem *pItem, BOXEE::BXObject& obj);
  static void HandleLiveTvItem(const CFileItem *pItem, BOXEE::BXObject& obj);
  static void HandleUnknownItem(const CFileItem *pItem, BOXEE::BXObject& obj);

  static CStdString GetUrlFromItemForRepostToServer(const CFileItem& item);

  static void SetObjParametersFromItem(const CFileItem *pItem, BOXEE::BXObject& obj,bool setContentType, bool setProvider, bool setAppId, bool setStreamType, bool setAdult, bool setCountries, bool setThumb, bool setGenre, bool setReleaseDate, bool setDescription, bool setBoxeeId, bool setShowId, bool setShowName, bool setImdbId, bool setYear, bool setRuntime);
  static void SetActivatedFromToObj(const CFileItem *pItem, BOXEE::BXObject& obj);

  static void Rate(const CFileItem *pItem, const std::string &strRate);
  static void Share(const CFileItem *pItem, const std::vector<BOXEE::BXFriend> &vecRecommendTo, CFileItemList& listPublishToServices , const std::string& inBoxee, const std::string& userText = "");

  static bool AddAppObjToAction(BOXEE::BXGeneralMessage& action, BOXEE::BXObject& obj, const std::string& name, const std::string& id, const std::string& url, const std::string& thumb);
  static bool AddRepositoryObjToAction(BOXEE::BXGeneralMessage& action, BOXEE::BXObject& obj, const std::string& name, const std::string& id, const std::string& url);

  static bool IsPictureQulifyAsThumb(CFileItemPtr item,CStdString* picturesQualifyAsThumbnailArray);
  static CStdString GetPicturePathQulifyAsThumb(CStdString* picturesQualifyAsThumbnailArray);

  static bool IsUrlPlayableForCustomButton(const CStdString& url);

  static bool ReportInstallRssActionBG(const VECSOURCES& videoRssVec, const VECSOURCES& musicRssVec, const VECSOURCES& pictureRssVec);
  static bool ReportInstallRssAction(const VECSOURCES& videoRssVec, const VECSOURCES& musicRssVec, const VECSOURCES& pictureRssVec);
  static CStdString BuildInstallRssData(const VECSOURCES& allRssSources);
  static CStdString GetShortRssType(CStdString rssType);

  static bool UserObjToFileItem(const BOXEE::BXObject &obj, CFileItem* pItem);

  static std::map<CStdString,CStdString> m_LanguageToCodeMap;
  static bool m_LanguageToCodeWasInit;
};

#endif
