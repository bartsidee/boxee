//
// C++ Implementation: BoxeeUtils
//
// Description:
//
//
// Author: Team XBMC <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "FileItem.h"
#include "MusicInfoTag.h"
#include "BoxeeUtils.h"
#include "VideoDatabase.h"
#include "MusicDatabase.h"
#include "VideoInfoTag.h"
#include "Util.h"
#include "utils/log.h"
#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxutils.h"
#include "FileSystem/File.h"
#include "GUIDialogProgress.h"
#include "GUIDialogSelect.h"
#include "GUIWindowVideoInfo.h"
#include "GUIWindowMusicInfo.h"
#include "GUIWindowBoxeeAlbumInfo.h"
#include "Util.h"
#include "utils/IMDB.h"
//#include "utils/HTTP.h"
#include "utils/RegExp.h"
#include "utils/GUIInfoManager.h"
#include "VideoInfoTag.h"
#include "MusicInfoTag.h"
#include "VideoInfoScanner.h"
#include "MusicInfoScanner.h"
#include "utils/MusicInfoScraper.h"
#include "GUIDialogSongInfo.h"
#include "GUIDialogProfileSettings.h"
#include "GUIDialogContextMenu.h"
#include "GUIWindowManager.h"
#include "GUIDialogOK.h"
#include "GUIDialogBoxeeUserInfo.h"
#include "FileSystem/Directory.h"
#include "bxconfiguration.h"
#include "BoxeeObjectCache.h"
#include "LastFmManager.h"
#include "Application.h"
#include "PictureThumbLoader.h"
#include "GUIDialogYesNo.h"
#include "tinyXML/tinyxml.h"
#include "MetadataResolver.h"
#include "MetadataResolverVideo.h"
#include "FileSystem/BoxeeDatabaseDirectory.h"
#include "URL.h"
#include "FileSystem/Directory.h"
#include "Picture.h"
#include "SpecialProtocol.h"
#include "SystemInfo.h"
#include <vector>
#include "LocalizeStrings.h"
#include "FactoryDirectory.h"
#include "GUIWindowSlideShow.h"
#include "GUISettings.h"

#define BOXEE_THUMB_GENERAL   "general"
#define BOXEE_THUMB_LIKE      "like"
#define BOXEE_THUMB_DONT_LIKE "dontlike"
#define BOXEE_THUMB_VIDEO     "video"
#define BOXEE_THUMB_MUSIC     "music"
#define BOXEE_THUMB_FRIEND_ADD "friendadd"
#define BOXEE_THUMB_FRIEND_REMOVE "friendremove"
#define BOXEE_THUMB_PHOTO     "photo"
#define BOXEE_THUMB_INTERNET  "internet"
#define BOXEE_THUMB_ONLINE    "online"
#define BOXEE_THUMB_OFFLINE   "offline"
#define BOXEE_THUMB_AWAY      "away"
#define BOXEE_THUMB_DOWNLOAD  "downloaded"
#define BOXEE_THUMB_RATE      "rate"
#define BOXEE_THUMB_RECOMMEND "recommend"

//#undef GetUserName

using namespace BOXEE;
using namespace XFILE;
using namespace VIDEO;
using namespace MUSIC_INFO;
using namespace MUSIC_GRABBER;
using namespace DIRECTORY;

#ifdef BOXEE_DEVICE
const char* BOXEE_PLATFORMS[] = {"osx","atv","lin","win", BOXEE_DEVICE};
#else
const char* BOXEE_PLATFORMS[] = {"osx","atv","lin","win"};
#endif

std::map<CStdString,CStdString> BoxeeUtils::m_LanguageToCodeMap;
bool BoxeeUtils::m_LanguageToCodeWasInit = false;

void BoxeeWatchdogListener::PathUpdate(const CStdString &strPath, bool bAvailable)
{
  BOXEE::Boxee::GetInstance().GetMetadataEngine().MarkFoldersAvailable(strPath, bAvailable);
}

BoxeeUtils::BoxeeUtils()
{
}


BoxeeUtils::~BoxeeUtils()
{
}

void BoxeeUtils::InitializeLanguageToCodeMap()
{
  if (m_LanguageToCodeWasInit)
  {
    return;
  }

  m_LanguageToCodeMap["catalan"] = "(ca)";
  m_LanguageToCodeMap["chinese"] = "(zh)";
  m_LanguageToCodeMap["croatian"] = "(hr)";
  m_LanguageToCodeMap["czech"] = "(cs)";
  m_LanguageToCodeMap["danish"] = "(da)";
  m_LanguageToCodeMap["dutch"] = "(nl)";
  m_LanguageToCodeMap["english"] = "(en)";
  m_LanguageToCodeMap["esperanto"] = "(eo)";
  m_LanguageToCodeMap["finnish"] = "(fi)";
  m_LanguageToCodeMap["french"] = "(fr)";
  m_LanguageToCodeMap["german"] = "(de)";
  m_LanguageToCodeMap["greek"] = "(el)";
  m_LanguageToCodeMap["hebrew"] = "(he)";
  m_LanguageToCodeMap["hungarian"] = "(hu)";
  m_LanguageToCodeMap["icelandic"] = "(is)";
  m_LanguageToCodeMap["indonesian"] = "(id)";
  m_LanguageToCodeMap["italian"] = "(it)";
  m_LanguageToCodeMap["japanese"] = "(ja)";
  m_LanguageToCodeMap["korean"] = "(ko)";
  m_LanguageToCodeMap["maltese"] = "(mt)";
  m_LanguageToCodeMap["norwegian"] = "(no)";
  m_LanguageToCodeMap["polish"] = "(pl)";
  m_LanguageToCodeMap["portuguese"] = "(pt)";
  m_LanguageToCodeMap["romanian"] = "(ro)";
  m_LanguageToCodeMap["russian"] = "(ru)";
  m_LanguageToCodeMap["serbian"] = "(sr)";
  m_LanguageToCodeMap["slovak"] = "(sk)";
  m_LanguageToCodeMap["slovenian"] = "(sl)";
  m_LanguageToCodeMap["spanish"] = "(es)";
  m_LanguageToCodeMap["swedish"] = "(sv)";
  m_LanguageToCodeMap["turkish"] = "(tr)";
  m_LanguageToCodeMap["ukrainian"] = "(uk)";

  m_LanguageToCodeWasInit = true;
}

CStdString BoxeeUtils::GetCodeOfLanguage(const CStdString& language)
{
  CStdString languageCode = "";

  CStdString tmpLanguage = language;
  CStdString lowerLanguage = tmpLanguage.ToLower();

  std::map<CStdString,CStdString>::iterator iter = m_LanguageToCodeMap.find(lowerLanguage);
  if(iter != m_LanguageToCodeMap.end())
  {
    languageCode = iter->second;
  }

  return languageCode;
}

BXObject BoxeeUtils::GetMediaObject(const BOXEE::BXGeneralMessage &msg)
{
  for (int i=0; i<msg.GetObjectCount();i++)
  {
    BXObject obj = msg.GetObject(i);
    std::string strType = obj.GetType();
    if (strType == MSG_OBJ_TYPE_MOVIE ||
        strType == MSG_OBJ_TYPE_TV ||
        strType == MSG_OBJ_TYPE_PICTURES ||
        strType == MSG_OBJ_TYPE_AUDIO_ALBUM ||
        strType == MSG_OBJ_TYPE_AUDIO_TRACK)
      return obj;
  }

  return BXObject(false);
}

BXObject BoxeeUtils::GetUserObject(const BOXEE::BXGeneralMessage &msg, int nIndex)
{
  int iUserObj=0;
  for (int i=0; i<msg.GetObjectCount();i++)
  {
    BXObject obj = msg.GetObject(i);
    std::string strType = obj.GetType();
    if (strType == MSG_OBJ_TYPE_USER)
    {
      if (iUserObj++ == nIndex)
        return obj;
    }
  }

  return BXObject(false);
}

std::string BoxeeUtils::GetMessageThumb(const BOXEE::BXGeneralMessage &msg) {
  std::string strThumb = BOXEE_THUMB_GENERAL;

  std::string strMsgType = msg.GetMessageType();
  if ( strMsgType == MSG_ACTION_TYPE_RECOMMEND )
    strThumb = BOXEE_THUMB_RECOMMEND;
  else if ( strMsgType == MSG_ACTION_TYPE_RATE )
    strThumb = BOXEE_THUMB_RATE;
  else if ( strMsgType == MSG_ACTION_TYPE_WATCH )
  {
    BXObject obj= GetMediaObject(msg);
    if (obj.IsValid()) {
      if (obj.GetType() == MSG_OBJ_TYPE_PICTURES)
        strThumb = BOXEE_THUMB_PHOTO;
      else
        strThumb = BOXEE_THUMB_VIDEO;
    }
  }
  else if ( strMsgType == MSG_ACTION_TYPE_LISTEN  )
  {
    strThumb = BOXEE_THUMB_MUSIC;
  }
  else if (strMsgType == MSG_ACTION_TYPE_ADD_FRIEND) {
    strThumb = BOXEE_THUMB_FRIEND_ADD;
  }

  return strThumb;
}

std::string BoxeeUtils::GetBoxeePresenceThumb(const std::string &strPresence)
{
  if (strPresence == MSG_PRESENCE_ONLINE)
    return BOXEE_THUMB_ONLINE;
  else if (strPresence == MSG_PRESENCE_AWAY)
    return BOXEE_THUMB_AWAY;
  else if (strPresence == MSG_PRESENCE_OFFLINE)
    return BOXEE_THUMB_OFFLINE;

  return BOXEE_THUMB_OFFLINE ;
}

std::string BoxeeUtils::GetBoxeeGeneralThumb()
{
  return BOXEE_THUMB_GENERAL;
}

std::string BoxeeUtils::AddCredentialsToLink(const std::string &strLink)
{
  CStdString newLink(strLink);
  int nProtocolPos = newLink.Find("://");

  CStdString strProtocol = "http";
  if (nProtocolPos > 0)
    strProtocol = newLink.Left(nProtocolPos);

  strProtocol += "://";

  CStdString username = BOXEE::Boxee::GetInstance().GetCredentials().GetUserName();
  CStdString password = BOXEE::Boxee::GetInstance().GetCredentials().GetPassword();

  CUtil::URLEncode(username);
  CUtil::URLEncode(password);

  CStdString creds = strProtocol + username + ":" + password + "@";

  if (newLink.Left(strProtocol.GetLength()).ToLower() == strProtocol.ToLower())
    newLink.Delete(0,strProtocol.GetLength());

  return creds+newLink;
}

CStdString BoxeeUtils::GetFormattedDesc(const BOXEE::BXGeneralMessage &msg, bool bSelected)
{
  CStdString strDesc = msg.GetValue(MSG_KEY_DESCRIPTION);
  CStdString strResult;
  size_t nPos = strDesc.Find("[object");
  size_t nLastPos = 0;

  while ((int)nPos<strDesc.GetLength() && nPos != CStdString::npos)
  {
    strResult += strDesc.Mid(nLastPos, nPos - nLastPos);
    nPos += 7;

    BXObject obj;
    CStdString strProp;
    while ((int)nPos<strDesc.GetLength() && strDesc[nPos] != '/' && strDesc[nPos] != ']')
    {
      while ((int)nPos<strDesc.GetLength() && strDesc[nPos] == ' ')
        nPos++;

      if (strDesc[nPos] == '/')
        break;

      CStdString strKey;
      while ((int)nPos<strDesc.GetLength() && strDesc[nPos] != '=')
        strKey += strDesc[nPos++];

      while (strDesc[nPos] != '"')
        nPos++;

      nPos++;

      CStdString strVal;
      while ((int)nPos<strDesc.GetLength() && strDesc[nPos] != '"')
        strVal += strDesc[nPos++];

      strKey.TrimRight('"');
      strKey.TrimLeft('"');
      strVal.TrimRight('"');
      strVal.TrimLeft('"');
      strKey.Trim();
      strVal.Trim();

      nPos++;

      if (strKey.ToLower().Equals("id"))
        obj = msg.GetObjectByID(strVal);
      else if (strKey.ToLower().Equals("property"))
        strProp = strVal;
    }

    if (obj.IsValid() && !strProp.IsEmpty())
    {
      CStdString strType = obj.GetType();
      //			CStdString strColor;
      //			strColor.Format("[COLOR %s]",conf.GetStringParam((bSelected?"FeedItemColorSelected.":"FeedItemColor.")+
      //					strProp+"."+strType,(bSelected?"ffffffff":"ff7daa4d")).c_str());
      //			strResult += strColor + obj.GetValue(strProp).c_str() + CStdString("[/COLOR]");
      strResult += obj.GetValue(strProp).c_str();
    }

    while ((int)nPos<strDesc.GetLength() && (strDesc[nPos] == '/' || strDesc[nPos] == ']'))
      nPos++;

    nLastPos = nPos;
    nPos = strDesc.Find("[object", nPos);
  }

  if (nLastPos != CStdString::npos && (int)nLastPos < strDesc.GetLength())
    strResult += strDesc.Mid(nLastPos);

  return strResult;
}

CStdString BoxeeUtils::StripDesc(const CStdString &strDesc)
{
  bool bInTag = false;
  CStdString strResult;
  for (int i=0; i<strDesc.GetLength(); i++)
  {
    if (!bInTag && strDesc[i] == '[')
      bInTag = true;
    if (!bInTag)
      strResult += strDesc[i];
    if (bInTag && strDesc[i] == ']')
      bInTag = false;
  }
  return strResult;
}

bool BoxeeUtils::VideoDetailsToObject(const CFileItem *pItem, BXObject &obj)
{
  obj.SetType(MSG_OBJ_TYPE_MOVIE);

  if (pItem->IsFlash() || !CUtil::IsHD(pItem->m_strPath))
  {
    obj.SetType(MSG_OBJ_TYPE_FLASH_STREAM_MOVIE);
  }

  const CVideoInfoTag *movieDetails = pItem->GetVideoInfoTag();

  if (movieDetails == NULL)
  {
    return true;
  }

  if (!movieDetails->m_strIMDBNumber.empty())
  {
    obj.SetValue(MSG_KEY_IMDB_NUM, movieDetails->m_strIMDBNumber);
  }

  if (!movieDetails->m_strGenre.empty())
  {

    CStdString strGenre = movieDetails->m_strGenre;
    strGenre.ToLower();

    // Filter Genres
    static std::vector<CStdString> m_badWords;
    m_badWords.push_back("porn");
    m_badWords.push_back("xxx");
    m_badWords.push_back("sex");
    m_badWords.push_back("adult");

    std::vector<CStdString>::const_iterator iter = m_badWords.begin();
    while (iter != m_badWords.end())
    {
      CRegExp reg;
      reg.RegComp(*iter);
      if (reg.RegFind(strGenre) != -1) {
        return false;
      }
      iter++;
    }

    obj.SetValue(MSG_KEY_GENRE, movieDetails->m_strGenre);
  }

  if (!movieDetails->m_strDirector.empty())
    obj.SetValue(MSG_KEY_DIRECTOR, movieDetails->m_strDirector);

  if (!movieDetails->m_strStudio.empty())
    obj.SetValue(MSG_KEY_STUDIO, movieDetails->m_strStudio);

  obj.SetValue(MSG_KEY_NAME, movieDetails->m_strTitle);

  if (!movieDetails->m_strShowTitle.IsEmpty())
  {
    obj.SetType(MSG_OBJ_TYPE_TV);

    if (pItem->IsFlash() || !CUtil::IsHD(pItem->m_strPath))
    {
      obj.SetType(MSG_OBJ_TYPE_FLASH_STREAM_TVSHOW);
    }

    char szEpisode[10];
    char szSeason[10];
    snprintf(szEpisode,10,"%d",movieDetails->m_iEpisode);
    snprintf(szSeason,10,"%d",movieDetails->m_iSeason);
    obj.SetValue(MSG_KEY_EPISODE, szEpisode);
    obj.SetValue(MSG_KEY_SEASON, szSeason);
    obj.SetValue(MSG_KEY_NAME, movieDetails->m_strShowTitle);
  }

  return true;
}

bool BoxeeUtils::Recommend(const CFileItem *pItem, const std::vector<BOXEE::BXFriend> &vecRecommendTo)
{
  if (!pItem)
  {
    return false;
  }

  if (vecRecommendTo.size() == 0)
    return false;

  BXGeneralMessage action;
  action.SetMessageType(MSG_ACTION_TYPE_RECOMMEND);

  // In case there is "referral" property -> Report it to the server
  if(pItem->HasProperty("referral"))
  {
    action.SetReferral(pItem->GetProperty("referral"));

    CLog::Log(LOGDEBUG,"BoxeeUtils::Recommend - For item [label=%s][referral=%s] Message was set with [referral=%s] (referral)",(pItem->GetLabel()).c_str(),(pItem->GetProperty("referral")).c_str(),(action.GetReferral()).c_str());
  }

  for (int i=0; i<(int)vecRecommendTo.size(); i++) {

    BXObject objFriend;
    objFriend.SetType(MSG_OBJ_TYPE_USER);
    BXFriend aFriend = vecRecommendTo[i];

    objFriend.SetID(aFriend.GetFriendId());
    action.AddObject(objFriend);
  }

  BXObject obj;
  const CFileItem *recommendedItem = pItem;
  if (pItem->HasExternlFileItem())
  {
    recommendedItem = pItem->GetExternalFileItem().get();
  }

  if(recommendedItem->GetPropertyBOOL("isTrailer") == true)
  {
    // In case of trailer object -> Need to add "(trailer)" to label
    // (happen if recommend without play)

    CLog::Log(LOGERROR,"BoxeeUtils::Recommend - Going to call AddTrailerStrToItemLabel with [Trailer] item [path=%s][label=%s] (tr)",(recommendedItem->m_strPath).c_str(),(recommendedItem->GetLabel()).c_str());

    CFileItem trailerItem(*recommendedItem);
    BoxeeUtils::AddTrailerStrToItemLabel(trailerItem);

    CLog::Log(LOGERROR,"BoxeeUtils::Recommend - After call to AddTrailerStrToItemLabel with [Trailer] item. [path=%s][label=%s] (tr)",(trailerItem.m_strPath).c_str(),(trailerItem.GetLabel()).c_str());

    obj = FileItemToObject(&trailerItem);
  }
  else
  {
    obj = FileItemToObject(recommendedItem);
  }

  if (obj.IsValid())
  {
    // For debug //
    BoxeeUtils::LogReportToServerAction(*recommendedItem,obj,CReportToServerActionType::RECOMMEND);
    ///////////////

    action.AddObject(obj);
    BOXEE::Boxee::GetInstance().SetUserAction_bg(action);
    return true;
  }
  else
  {
    CLog::Log(LOGERROR,"BoxeeUtils::Recommend - Object for recommend is invalid");
    return false;
  }
  return false;
}

void BoxeeUtils::Drop(const CFileItem *pItem)
{
  pItem->Dump();
  if (pItem->HasProperty("BoxeeDBvideoId"))
  {
    CLog::Log(LOGDEBUG,"BoxeeUtils::Drop, DROP, MANUAL, drop video, path = %s, id = %d", pItem->m_strPath.c_str(), pItem->GetPropertyInt("BoxeeDBvideoId"));
    BOXEE::Boxee::GetInstance().GetMetadataEngine().DropVideoById(pItem->GetPropertyInt("boxeeDBvideoId"));
  }
  else if (pItem->HasProperty("BoxeeDBalbumId"))
  {
    CLog::Log(LOGDEBUG,"BoxeeUtils::Drop, DROP, MANUAL, drop album, path = %s, id = %d", pItem->m_strPath.c_str(), pItem->GetPropertyInt("BoxeeDBalbumId"));
    BOXEE::Boxee::GetInstance().GetMetadataEngine().RemoveAlbumById(pItem->GetPropertyInt("BoxeeDBalbumId"));
  }
  else
  {
    CLog::Log(LOGERROR,"BoxeeUtils::Drop, DROP, MANUAL, unable to drop item. path = %s", pItem->m_strPath.c_str());
  }
}

void BoxeeUtils::Rate(const CFileItem *pItem, bool bThumbsUp) {
  Rate(pItem, std::string(bThumbsUp?"1":"0"));
}

void BoxeeUtils::Rate(const CFileItem *pItem, const std::string &strRate) {
  if (!pItem)
    return;

  BXGeneralMessage action;
  action.SetMessageType(MSG_ACTION_TYPE_RATE);

  // In case there is "referral" property -> Report it to the server
  if(pItem->HasProperty("referral"))
  {
    action.SetReferral(pItem->GetProperty("referral"));

    CLog::Log(LOGDEBUG,"BoxeeUtils::Rate - For item [label=%s][referral=%s] Message was set with [referral=%s] (referral)",(pItem->GetLabel()).c_str(),(pItem->GetProperty("referral")).c_str(),(action.GetReferral()).c_str());
  }

  BXObject obj;
  const CFileItem* rateItem = pItem;
  if (pItem->HasExternlFileItem())
  {
    rateItem = pItem->GetExternalFileItem().get();
  }

  obj = FileItemToObject(rateItem);

  if (obj.IsValid())
  {
    // For debug //
    BoxeeUtils::LogReportToServerAction(*rateItem,obj,CReportToServerActionType::RATE);
    ///////////////

    action.SetValue(MSG_KEY_LIKE,strRate);
    action.AddObject(obj);
    BOXEE::Boxee::GetInstance().SetUserAction_bg(action);
  }
  else
  {
    CLog::Log(LOGERROR,"BoxeeUtils::Rate - Object for rate is invalid");
  }
}

bool BoxeeUtils::Queue(CFileItem* pItem, bool runInBG)
{
  bool succeeded = false;

  if (!pItem)
  {
    return false;
  }

  BXGeneralMessage action;
  action.SetMessageType(MSG_ACTION_TYPE_QUEUE);

  // In case there is "referral" property -> Report it to the server
  if(pItem->HasProperty("referral"))
  {
    action.SetReferral(pItem->GetProperty("referral"));
    CLog::Log(LOGDEBUG,"BoxeeUtils::Queue - For item [label=%s][referral=%s] Message was set with [referral=%s] (referral)",(pItem->GetLabel()).c_str(),(pItem->GetProperty("referral")).c_str(),(action.GetReferral()).c_str());
  }

  BXObject* queueObjPtr;
  BXObject obj;
  BXObject unknownObj;

  CFileItemPtr queueItem(new CFileItem(*pItem));

  //queueItem->SetProperty("report-to-queue",true);

  obj = FileItemToObject(queueItem.get());

  CStdString objType = obj.GetType();

  if ((objType == MSG_OBJ_TYPE_UNKNOWN) || (objType == MSG_OBJ_TYPE_UNKNOWN_AUDIO)|| (objType == MSG_OBJ_TYPE_UNKNOWN_VIDEO) || (!obj.IsValid()))
  {
    // failed to create the object -> treat it as UNKNOWN item

    unknownObj.SetName("local content");

    CStdString clientId = BXUtils::GetMD5Hex(pItem->m_strPath);
    pItem->SetProperty("clientid",clientId);
    unknownObj.SetValue(MSG_KEY_CLIENT_ID, clientId);

    CLog::Log(LOGDEBUG,"BoxeeUtils::Queue - For item [label=%s][path=%s] a clientId was set [clientId=%s] (queue)",pItem->GetLabel().c_str(),pItem->m_strPath.c_str(),clientId.c_str());

    unknownObj.SetType(MSG_OBJ_TYPE_UNKNOWN);

    unknownObj.SetValid(true);

    // add UNKNOWN item to DB before send to server

    BXMetadataEngine& MDE = BOXEE::Boxee::GetInstance().GetMetadataEngine();
    succeeded = MDE.AddQueueItem(pItem->GetLabel(), pItem->m_strPath, pItem->GetThumbnailImage(), clientId);

    if (!succeeded)
    {
      CLog::Log(LOGERROR,"BoxeeUtils::Queue - FAILED to add item [label=%s][path=%s] with [clientId=%s] to DB for queue (queue)",pItem->GetLabel().c_str(),pItem->m_strPath.c_str(),clientId.c_str());
      return succeeded;
    }

    queueObjPtr = &unknownObj;
  }
  else
  {
    queueObjPtr = &obj;
  }

  // For debug //
  BoxeeUtils::LogReportToServerAction(*queueItem,*queueObjPtr,CReportToServerActionType::QUEUE);
  ///////////////

  action.AddObject(*queueObjPtr);

  if(runInBG)
  {
    class AddToQueueJob: public IRunnable
    {
    public:
      AddToQueueJob(const BXGeneralMessage& action){m_action = action;};
      virtual void Run()
      {
        m_bJobResult = Boxee::GetInstance().SetUserAction(m_action);

        if (!m_bJobResult)
        {
          CLog::Log(LOGERROR,"AddToQueueJob::Run - FAILED to run user action (queue)");
        }
      }
    private:
      BXGeneralMessage m_action;
    };

    AddToQueueJob* addToQueuejob = new AddToQueueJob(action);
    succeeded = CUtil::RunInBG(addToQueuejob);

  }
  else
  {
    succeeded = BOXEE::Boxee::GetInstance().SetUserAction(action);
  }

  // update the queue
  BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().UpdateQueueNow();

  CLog::Log(LOGDEBUG,"[QUEUE] - Report to server for [%s] returned [succeeded=%d]. [runInBG=%d] (rts)(queue)",action.GetMessageType().c_str(),succeeded,runInBG);

  return succeeded;
}

bool BoxeeUtils::Dequeue(const CFileItem* pItem, bool runInBG)
{
  bool succeeded = false;

  if (!pItem)
  {
    return succeeded;
  }

  BXGeneralMessage action;
  action.SetMessageType(MSG_ACTION_TYPE_DEQUEUE);

  // In case there is "referral" property -> Report it to the server
  if(pItem->GetProperty("referral").IsEmpty())
  {
    CLog::Log(LOGERROR,"BoxeeUtils::Dequeue - Item [label=%s] doesn't has [referral=%s] property so it can't be dequeue (queue)(referral)",(pItem->GetLabel()).c_str(),pItem->GetProperty("referral").c_str());
    return false;
  }

  action.SetReferral(pItem->GetProperty("referral"));
  CLog::Log(LOGDEBUG,"BoxeeUtils::Dequeue - For item [label=%s][referral=%s] Message was set with [referral=%s] (referral)",(pItem->GetLabel()).c_str(),(pItem->GetProperty("referral")).c_str(),(action.GetReferral()).c_str());

  if(runInBG)
  {
    class RemoveFromQueueJob: public IRunnable
    {
    public:
      RemoveFromQueueJob(const BXGeneralMessage& action){m_action = action;};
      virtual void Run()
      {
        m_bJobResult = Boxee::GetInstance().SetUserAction(m_action);

        if (!m_bJobResult)
        {
          CLog::Log(LOGERROR,"RemoveFromQueueJob::Run - FAILED to run user action (queue)");
        }
      }
    private:
      BXGeneralMessage m_action;
    };

    RemoveFromQueueJob* removeFromQueuejob = new RemoveFromQueueJob(action);
    succeeded = CUtil::RunInBG(removeFromQueuejob);
  }
  else
  {
    succeeded = BOXEE::Boxee::GetInstance().SetUserAction(action);
  }

  // update the queue
  BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().UpdateQueueNow();

  CLog::Log(LOGDEBUG,"[DEQUEUE] - Report to server for [%s] returned [succeeded=%d]. [runInBG=%d] (rts)(queue)",action.GetMessageType().c_str(),succeeded,runInBG);

  if (succeeded)
  {
    // in case dequeue from server succeeded and the item has "clientid" property -> remove its entry in the DB

    CStdString clientId = pItem->GetProperty("clientid");
    BXMetadataEngine& MDE = BOXEE::Boxee::GetInstance().GetMetadataEngine();
    bool removeFromDbSucceeded = MDE.RemoveQueueItem(clientId);

    if (!removeFromDbSucceeded)
    {
      CLog::Log(LOGWARNING,"BoxeeUtils::Dequeue - FAILED to remove [clientid=%s] from DB (queue)",clientId.c_str());
    }
  }

  return succeeded;
}

bool BoxeeUtils::Subscribe(BOXEE::CSubscriptionType::SubscriptionTypeEnums subscriptionType, const std::string& src, const std::string& strShowTitle, bool runInBG)
{
  bool succeeded = false;

  BXGeneralMessage action;
  action.SetMessageType(MSG_ACTION_TYPE_SUBSCRIBE);

  BXObject obj(false);

  CStdString subscribeTypeStr = BOXEE::BXSubscriptionsManager::GetSubscriptionTypeAsStr(subscriptionType);

  if(subscribeTypeStr.IsEmpty())
  {
    CLog::Log(LOGERROR,"BoxeeUtils::Subscribe - FAILED to translate enum [subscriptionType=%d] (rts)(subs)",subscriptionType);
    return succeeded;
  }

  obj.SetType(subscribeTypeStr);
  obj.SetValue(MSG_KEY_SRC, src);
  obj.SetValue(MSG_KEY_NAME, strShowTitle);
  obj.SetValid(true);

  if (obj.IsValid())
  {
    CLog::Log(LOGDEBUG,"[SUBSCRIBE] - Going to report [%s] to server. [objType=%s][objId=%s][objSrc=%s]. [runInBG=%d] (rts)(subs)",action.GetMessageType().c_str(),obj.GetType().c_str(),obj.GetID().c_str(),obj.GetValue(MSG_KEY_SRC).c_str(),runInBG);

    action.AddObject(obj);

    if(runInBG)
    {
      succeeded = BOXEE::Boxee::GetInstance().SetUserAction_bg(action);
    }
    else
    {
      succeeded = BOXEE::Boxee::GetInstance().SetUserAction(action);

      // in case of Subscribe action ISN'T in BG -> Update the SubscribeList
      BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().UpdateSubscriptionsListNow();
    }

    CLog::Log(LOGDEBUG,"[SUBSCRIBE] - Report to server for [%s] returned [succeeded=%d]. [runInBG=%d] (rts)(subs)",action.GetMessageType().c_str(),succeeded,runInBG);
  }
  else
  {
    CLog::Log(LOGERROR,"BoxeeUtils::Subscribe - Object for subscribe is invalid");
  }

  return succeeded;
}

bool BoxeeUtils::Unsubscribe(BOXEE::CSubscriptionType::SubscriptionTypeEnums subscriptionType, const std::string& src, const std::string& strShowTitle, bool runInBG)
{
  bool succeeded = false;

  BXGeneralMessage action;
  action.SetMessageType(MSG_ACTION_TYPE_UNSUBSCRIBE);

  BXObject obj(false);

  CStdString subscribeTypeStr = BOXEE::BXSubscriptionsManager::GetSubscriptionTypeAsStr(subscriptionType);

  if(subscribeTypeStr.IsEmpty())
  {
    CLog::Log(LOGERROR,"BoxeeUtils::Unsubscribe - FAILED to translate enum [subscriptionType=%d] (rts)",subscriptionType);
    return succeeded;
  }

  obj.SetType(subscribeTypeStr);
  obj.SetValue(MSG_KEY_SRC, src);
  obj.SetValue(MSG_KEY_NAME, strShowTitle);
  obj.SetValid(true);

  if (obj.IsValid())
  {
    CLog::Log(LOGDEBUG,"[UNSUBSCRIBE] - Going to report [%s] to server. [objType=%s][objSrc=%s] (rts)",action.GetMessageType().c_str(),obj.GetType().c_str(),obj.GetValue(MSG_KEY_SRC).c_str());

    action.AddObject(obj);

    if(runInBG)
    {
      succeeded = BOXEE::Boxee::GetInstance().SetUserAction_bg(action);
    }
    else
    {
      succeeded = BOXEE::Boxee::GetInstance().SetUserAction(action);

      // in case of Unsubscribe action ISN'T in BG -> Update the SubscribeList
      BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().UpdateSubscriptionsListNow();
    }
  }
  else
  {
    CLog::Log(LOGERROR,"BoxeeUtils::Unsubscribe - Object for unsubscribe is invalid");
  }

  return succeeded;
}

bool BoxeeUtils::IsSubscribe(const std::string& src)
{
  return Boxee::GetInstance().GetBoxeeClientServerComManager().IsInSubscriptions(src);
}

bool BoxeeUtils::GetSubscriptionIds(BOXEE::CSubscriptionType::SubscriptionTypeEnums subscriptionType,std::vector<std::string>& subscriptionsIdsVec)
{
  return Boxee::GetInstance().GetBoxeeClientServerComManager().GetSubscriptionIds(subscriptionType,subscriptionsIdsVec);
}

bool BoxeeUtils::GetSubscriptions(CSubscriptionType::SubscriptionTypeEnums subscriptionType, std::vector<BXSubscriptionItem>& subscriptionsVec)
{
  return Boxee::GetInstance().GetBoxeeClientServerComManager().GetSubscriptions(subscriptionType,subscriptionsVec);
}

CStdString BoxeeUtils::BuildParameterString(const std::map<CStdString, CStdString>& mapParameters)
{
  std::map<CStdString, CStdString>::const_iterator it = mapParameters.begin();

  if (it == mapParameters.end()) // map is empty
    return "";

  CStdString strPath = "?";

  // Add first parameter
  strPath += it->first;
  strPath += "=";
  strPath += it->second;

  it++;

  // Add other parameters
  while (it != mapParameters.end())
  {
    strPath += "&";
    strPath += it->first;
    strPath += "=";
    strPath += it->second;
    it++;
  }

  return strPath;
}

CStdString BoxeeUtils::AppendParameters(const CStdString& _strPath, const std::map<CStdString, CStdString>& mapParameters)
{
  CStdString strPath = _strPath;

  std::map<CStdString, CStdString>::const_iterator it = mapParameters.begin();

  if (it == mapParameters.end()) // map is empty
    return "";

  // Add other parameters
  while (it != mapParameters.end())
  {
    strPath += "&";
    strPath += it->first;
    strPath += "=";
    strPath += it->second;
    it++;
  }

  return strPath;
}

void BoxeeUtils::ReportLaunchApp(const std::string& id, const CFileItem* pItem)
{
  BXGeneralMessage action;
  action.SetMessageType(MSG_ACTION_TYPE_LAUNCH);

  BXObject obj(false);

  obj.SetType(MSG_OBJ_TYPE_APPLICATION);
  obj.SetValue(MSG_KEY_ID, id);
  SetActivatedFromToObj(pItem, obj);
  obj.SetValid(true);

  if (obj.IsValid())
  {
    CLog::Log(LOGDEBUG,"[LAUNCH_APP] - Going to report [%s] to server. [objType=%s][objId=%s][activated_from=%s] (rts)",action.GetMessageType().c_str(),obj.GetType().c_str(),obj.GetValue(MSG_KEY_ID).c_str(),obj.GetValue(MSG_KEY_ACTIVATED_FROM).c_str());

    action.AddObject(obj);
    BOXEE::Boxee::GetInstance().SetUserAction_bg(action);
  }
  else
  {
    CLog::Log(LOGERROR,"BoxeeUtils::ReportLaunchApp - Object for launch is invalid");
  }
}

bool BoxeeUtils::ReportInstallRss(const VECSOURCES& allRssSources, bool runInBG)
{
  CLog::Log(LOGDEBUG,"BoxeeUtils::ReportInstallRss - Enter function with [allRssSources=%d][runInBG=%d] (rtspf)",(int)allRssSources.size(),runInBG);

  bool succeeded = false;

  CStdString strData = BuildInstallRssData(allRssSources);

  if(runInBG)
  {
    succeeded = BOXEE::Boxee::GetInstance().ReportInstallRss_bg(strData);
  }
  else
  {
    succeeded = BOXEE::Boxee::GetInstance().ReportInstallRss(strData);
  }

  return succeeded;
}

bool BoxeeUtils::ReportInstalledApps(const VECSOURCES& appsVec, bool runInBG)
{
  CLog::Log(LOGDEBUG,"BoxeeUtils::ReportInstallMultiApp - Enter function with [appsVecSize=%d] (rtspf)",(int)appsVec.size());

  bool succeeded = false;

  BXGeneralMessage action;
  action.SetMessageType(MSG_ACTION_TYPE_INSTALL);

  for(size_t i=0; i<appsVec.size(); i++)
  {
    BXObject obj(false);

    CURL url(appsVec[i].strPath);

    if (CUtil::IsLastFM(appsVec[i].strPath) || CUtil::IsShoutCast(appsVec[i].strPath))
    {
      succeeded = AddAppObjToAction(action, obj, appsVec[i].strName, url.GetProtocol(), appsVec[i].strPath, appsVec[i].m_strThumbnailImage);
    }
    else
    {
      succeeded = AddAppObjToAction(action, obj, appsVec[i].strName, url.GetHostName(), appsVec[i].strPath, appsVec[i].m_strThumbnailImage);
    }

    if (succeeded)
    {
      CLog::Log(LOGDEBUG,"BoxeeUtils::ReportInstallApps - [%d/%d] - After adding obj [objType=%s][objName=%s][objId=%s][objUrl=%s][objThumb=%s] (rtspf)",(int)(i+1),(int)appsVec.size(),obj.GetType().c_str(),obj.GetValue(MSG_KEY_NAME).c_str(),obj.GetValue(MSG_KEY_ID).c_str(),obj.GetValue(MSG_KEY_URL).c_str(),obj.GetValue(MSG_KEY_THUMB).c_str());
    }
    else
    {
      // error log was written in AddAppObjToAction()
    }
  }

  if (action.GetObjectCount() < 1)
  {
    CLog::Log(LOGERROR,"BoxeeUtils::ReportInstallApps - WON'T report to server because action contain no objects. [NumOfObjInAction=%d] (rtspf)",action.GetObjectCount());
    return false;
  }

  CLog::Log(LOGDEBUG,"[INSTALL_APP] - Going to report [%d] multi apps to server (rts)(rtspf)",action.GetObjectCount());

  if(runInBG)
  {
    succeeded = BOXEE::Boxee::GetInstance().SetUserAction_bg(action);
  }
  else
  {
    succeeded = BOXEE::Boxee::GetInstance().SetUserAction(action);

    // in case of ReportInstallApp action ISN'T in BG -> Update the UserApplicationsList
    BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().UpdateUserApplicationsListNow();
  }

  return succeeded;
}

bool BoxeeUtils::ReportInstallApp(const CAppDescriptor& descriptor, bool runInBG)
{
  return ReportInstallApp(descriptor.GetName(),descriptor.GetId(),descriptor.GetURL(),descriptor.GetThumb(),runInBG);
}

bool BoxeeUtils::ReportInstallApp(const std::string& name, const std::string& id, const std::string& url, const std::string& thumb, bool runInBG)
{
  bool succeeded = false;

  BXGeneralMessage action;
  action.SetMessageType(MSG_ACTION_TYPE_INSTALL);

  BXObject obj(false);

  succeeded = AddAppObjToAction(action, obj, name, id, url, thumb);

  if (succeeded)
  {
    CLog::Log(LOGDEBUG,"[INSTALL_APP] - Going to report [%s] to server. [objType=%s][objName=%s][objId=%s][objUrl=%s][objThumb=%s] (rts)",action.GetMessageType().c_str(),obj.GetType().c_str(),obj.GetValue(MSG_KEY_NAME).c_str(),obj.GetValue(MSG_KEY_ID).c_str(),obj.GetValue(MSG_KEY_URL).c_str(),obj.GetValue(MSG_KEY_THUMB).c_str());

    if(runInBG)
    {
      succeeded = BOXEE::Boxee::GetInstance().SetUserAction_bg(action);
    }
    else
    {
      succeeded = BOXEE::Boxee::GetInstance().SetUserAction(action);

      // in case of ReportInstallApp action ISN'T in BG -> Update the UserApplicationsList
      BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().UpdateUserApplicationsListNow();
    }
  }
  else
  {
    // error log was written in AddAppObjToAction()
  }

  return succeeded;
}

bool BoxeeUtils::AddAppObjToAction(BXGeneralMessage& action, BXObject& obj, const std::string& name, const std::string& id, const std::string& url, const std::string& thumb)
{
  if (name.empty() || id.empty())
  {
    CLog::Log(LOGERROR,"BoxeeUtils::AddAppObjToAction - FAILED to add app because there are one or more empty parameters. [name=%s][id=%s][url=%s][thumb=%s] (rtspf)",name.c_str(),id.c_str(),url.c_str(),thumb.c_str());
    return false;
  }

  obj.SetType(MSG_OBJ_TYPE_APPLICATION);
  obj.SetValue(MSG_KEY_NAME, name);
  obj.SetValue(MSG_KEY_ID, id);

  if (!url.empty())
  {
    obj.SetValue(MSG_KEY_URL, url);
  }

  if (!thumb.empty())
  {
    obj.SetValue(MSG_KEY_THUMB, thumb);
  }

  obj.SetValid(true);

  action.AddObject(obj);

  return true;
}

bool BoxeeUtils::ReportRemoveApp(const std::string& id, bool runInBG)
{
  bool succeeded = false;

  BXGeneralMessage action;
  action.SetMessageType(MSG_ACTION_TYPE_REMOVE);

  BXObject obj(false);

  obj.SetType(MSG_OBJ_TYPE_APPLICATION);
  obj.SetValue(MSG_KEY_ID, id);
  obj.SetValid(true);

  if (obj.IsValid())
  {
    CLog::Log(LOGDEBUG,"[REMOVE_APP] - Going to report [%s] to server. [objType=%s][objId=%s] (rts)",action.GetMessageType().c_str(),obj.GetType().c_str(),obj.GetValue(MSG_KEY_ID).c_str());

    action.AddObject(obj);

    if(runInBG)
    {
      succeeded = BOXEE::Boxee::GetInstance().SetUserAction_bg(action);
    }
    else
    {
      succeeded = BOXEE::Boxee::GetInstance().SetUserAction(action);

      // in case of ReportRemoveApp action ISN'T in BG -> Update the UserApplicationsList
      BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().UpdateUserApplicationsListNow();
    }
  }
  else
  {
    CLog::Log(LOGERROR,"BoxeeUtils::ReportRemoveApp - Object for remove is invalid");
  }

  return succeeded;
}

bool BoxeeUtils::ReportInstalledRepositories(const std::vector<CAppRepository>& repositories, bool runInBG)
{
  CLog::Log(LOGDEBUG,"BoxeeUtils::ReportInstalledRepositories - Enter function with [repositoriesVecSize=%d] (rtspf)",(int)repositories.size());

  bool succeeded = false;

  BXGeneralMessage action;
  action.SetMessageType(MSG_ACTION_TYPE_INSTALL);

  for(size_t i=0; i<repositories.size(); i++)
  {
    BXObject obj(false);

    succeeded = AddRepositoryObjToAction(action, obj, repositories[i].GetName(), repositories[i].GetID(), repositories[i].GetURL());

    if (succeeded)
    {
      CLog::Log(LOGDEBUG,"BoxeeUtils::ReportInstalledRepositories - [%d/%d] - After adding obj [objType=%s][objName=%s][objId=%s][objUrl=%s] (rtspf)",(int)(i+1),(int)repositories.size(),obj.GetType().c_str(),obj.GetValue(MSG_KEY_NAME).c_str(),obj.GetValue(MSG_KEY_ID).c_str(),obj.GetValue(MSG_KEY_URL).c_str());
    }
    else
    {
      // error log was written in AddRepositoryObjToAction()
    }
  }

  if (action.GetObjectCount() < 1)
  {
    CLog::Log(LOGERROR,"BoxeeUtils::ReportInstalledRepositories - WON'T report to server because action contain no objects. [NumOfObjInAction=%d] (rtspf)",action.GetObjectCount());
    return false;
  }

  CLog::Log(LOGDEBUG,"[INSTALL_REP] - Going to report [%d] multi repositories to server (rts)(rtspf)",action.GetObjectCount());

  if(runInBG)
  {
    succeeded = BOXEE::Boxee::GetInstance().SetUserAction_bg(action);
  }
  else
  {
    succeeded = BOXEE::Boxee::GetInstance().SetUserAction(action);

    // in case of InstallRepository action ISN'T in BG -> Update the RepositoriesList and UserApplicationsList
    BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().UpdateAppBoxRepositoriesListNow();
    BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().UpdateUserApplicationsListNow();
  }

  return succeeded;
}

bool BoxeeUtils::ReportInstallRepository(const CAppRepository& repository, bool runInBG)
{
  return ReportInstallRepository(repository.GetName(),repository.GetID(),repository.GetURL(),runInBG);
}

bool BoxeeUtils::ReportInstallRepository(const std::string& name, const std::string& id, const std::string& url, bool runInBG)
{
  bool succeeded = false;

  BXGeneralMessage action;
  action.SetMessageType(MSG_ACTION_TYPE_INSTALL);

  BXObject obj(false);

  AddRepositoryObjToAction(action, obj, name, id, url);

  if (obj.IsValid())
  {
    CLog::Log(LOGDEBUG,"[INSTALL_REP] - Going to report [%s] to server. [objType=%s][objName=%s][objId=%s][objUrl=%s] (rts)",action.GetMessageType().c_str(),obj.GetType().c_str(),obj.GetValue(MSG_KEY_NAME).c_str(),obj.GetValue(MSG_KEY_ID).c_str(),obj.GetValue(MSG_KEY_URL).c_str());

    action.AddObject(obj);

    if(runInBG)
    {
      succeeded = BOXEE::Boxee::GetInstance().SetUserAction_bg(action);
    }
    else
    {
      succeeded = BOXEE::Boxee::GetInstance().SetUserAction(action);

      // in case of InstallRepository action ISN'T in BG -> Update the RepositoriesList and UserApplicationsList
      BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().UpdateAppBoxRepositoriesListNow();
      BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().UpdateUserApplicationsListNow();
    }
  }
  else
  {
    CLog::Log(LOGERROR,"BoxeeUtils::InstallRepository - Object for install is invalid");
  }

  return succeeded;
}

bool BoxeeUtils::AddRepositoryObjToAction(BXGeneralMessage& action, BXObject& obj, const std::string& name, const std::string& id, const std::string& url)
{
  if (name.empty() || id.empty() || url.empty())
  {
    CLog::Log(LOGERROR,"BoxeeUtils::AddRepositoryObjToAction - FAILED to add repository because there are one or more empty parameters. [name=%s][id=%s][url=%s] (rtspf)",name.c_str(),id.c_str(),url.c_str());
    return false;
  }

  obj.SetType("repository");
  obj.SetValue(MSG_KEY_NAME, name);
  obj.SetValue(MSG_KEY_ID, id);
  obj.SetValue(MSG_KEY_URL, url);

  obj.SetValid(true);

  action.AddObject(obj);

  return true;
}

bool BoxeeUtils::ReportRemoveRepository(const std::string& id, bool runInBG)
{
  bool succeeded = false;

  BXGeneralMessage action;
  action.SetMessageType(MSG_ACTION_TYPE_REMOVE);

  BXObject obj(false);

  obj.SetType("repository");
  obj.SetValue(MSG_KEY_ID, id);
  obj.SetValid(true);

  if (obj.IsValid())
  {
    CLog::Log(LOGDEBUG,"[REMOVE_REP] - Going to report [%s] to server. [objType=%s][objId=%s] (rts)",action.GetMessageType().c_str(),obj.GetType().c_str(),obj.GetValue(MSG_KEY_ID).c_str());

    action.AddObject(obj);

    if(runInBG)
    {
      succeeded = BOXEE::Boxee::GetInstance().SetUserAction_bg(action);
    }
    else
    {
      succeeded = BOXEE::Boxee::GetInstance().SetUserAction(action);

      // in case of RemoveRepository action ISN'T in BG -> Update the RepositoriesList and UserApplicationsList
      BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().UpdateAppBoxRepositoriesListNow();
      BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().UpdateUserApplicationsListNow();
    }
  }
  else
  {
    CLog::Log(LOGERROR,"BoxeeUtils::RemoveRepository - Object for remove is invalid");
  }

  return succeeded;
}

void BoxeeUtils::Share(const CFileItem* pItem, const std::vector<BOXEE::BXFriend> &vecRecommendTo, bool bInBoxee, const std::string& userText)
{
  Share(pItem,vecRecommendTo,std::string(bInBoxee?"1":"0"),userText);
}

void BoxeeUtils::Share(const CFileItem* pItem, const std::vector<BOXEE::BXFriend> &vecRecommendTo, const std::string& inBoxee, const std::string& userText)
{
  if (!pItem)
  {
    CLog::Log(LOGDEBUG,"BoxeeUtils::Share - Enter function with a NULL item (share)");
    return;
  }

  BXGeneralMessage action;
  action.SetMessageType(MSG_ACTION_TYPE_SHARE);

  // In case there is "referral" property -> Report it to the server
  if(pItem->HasProperty("referral"))
  {
    action.SetReferral(pItem->GetProperty("referral"));

    CLog::Log(LOGDEBUG,"BoxeeUtils::Share - For item [label=%s][referral=%s] Message was set with [referral=%s] (referral)",(pItem->GetLabel()).c_str(),(pItem->GetProperty("referral")).c_str(),(action.GetReferral()).c_str());
  }

  for (int i=0; i<(int)vecRecommendTo.size(); i++)
  {
    BXObject objFriend;
    objFriend.SetType(MSG_OBJ_TYPE_USER);
    BXFriend aFriend = vecRecommendTo[i];
    objFriend.SetID(aFriend.GetFriendId());
    action.AddObject(objFriend);
  }

  BXObject obj;
  const CFileItem* shareItem = pItem;
  if (pItem->HasExternlFileItem())
  {
    shareItem = pItem->GetExternalFileItem().get();
  }

  obj = FileItemToObject(shareItem);

  if (obj.IsValid())
  {
    // For debug //
    BoxeeUtils::LogReportToServerAction(*shareItem,obj,CReportToServerActionType::SHARE);
    ///////////////

    action.SetValue(MSG_KEY_IN_BOXEE,inBoxee);
    action.SetValue(MSG_KEY_USER_TEXT,userText);

    CLog::Log(LOGDEBUG,"BoxeeUtils::Share - Report SHARE with [userText=%s] (share)(rts)",userText.c_str());

    action.AddObject(obj);
    BOXEE::Boxee::GetInstance().SetUserAction_bg(action);
  }
  else
  {
    CLog::Log(LOGERROR,"BoxeeUtils::Share - Object for rate is invalid");
  }
}

std::string BoxeeUtils::FormatTime(time_t tmWhen) {
  time_t tmNow = time(NULL);
  int nDiff = DayDiff(tmNow, tmWhen);
  struct tm *tmp = localtime(&tmWhen);

  CStdString strResult;

  bool bNeedTime=true;
  if (nDiff == 0)
    strResult == "today, ";
  else if (nDiff == 1)
    strResult == "yesterday, ";
  else {
    char szDate[256];
    strftime(szDate,256,"%m/%d/%y",tmp);
    strResult += szDate;
    bNeedTime = false;
  }

  if (bNeedTime) {
    char szTime[256];
    strftime(szTime,256,"%H:%M:%S",tmp);
    strResult += szTime;
  }
  return strResult;
}

int BoxeeUtils::DayDiff(time_t tmNow, time_t tmOther) {
  int nDays1 = tmNow / (60*60*24);
  int nDays2 = tmOther / (60*60*24);
  return nDays1 - nDays2;
}

time_t BoxeeUtils::GetModTime(const CStdString& strPath)
{
  struct __stat64 buffer = {0};
  CFile::Stat(strPath, &buffer);
#ifdef __APPLE__
  struct timespec iModTime = buffer.st_mtimespec;
  return iModTime.tv_sec;
#elif defined(_WIN32) || defined(_LINUX)
  return buffer.st_mtime;
#endif
}

void BoxeeUtils::OnTuneIn(const BOXEE::BXObject &userObj, bool bForce)
{
  if (!g_application.IsPlaying() || bForce)
  {
    if (userObj.GetName() == "lastfm")
    {
      CLastFmManager::GetInstance()->StopRadio();
      CLastFmManager::GetInstance()->ChangeStation(CURL(userObj.GetValue(MSG_KEY_URL)));
    }
    else if (userObj.GetName() == "shout" || userObj.GetName() == "shoutcast")
    {
      CFileItem item(userObj.GetValue(MSG_KEY_STATION));
      item.m_strPath = userObj.GetValue(MSG_KEY_URL);
      CLog::Log(LOGDEBUG,"%s - PLAY ITEM PATH %s", __FUNCTION__, item.m_strPath.c_str());
      g_application.PlayMedia(item);
    }
    else if (userObj.GetName() == "rss")
    {
      CFileItem item(userObj.GetValue(MSG_KEY_TITLE));
      item.m_strPath = userObj.GetValue(MSG_KEY_URL);
      CLog::Log(LOGDEBUG,"%s - PLAY ITEM PATH %s", __FUNCTION__, item.m_strPath.c_str());
      g_application.PlayMedia(item);
    }
  }
  else
  {
    CStdString strQ;
    CStdString strStation = userObj.GetValue(MSG_KEY_STATION);
    CUtil::UrlDecode(strStation);
    strQ.Format("Would you like to tune into %s?", strStation.c_str());
    if (CGUIDialogYesNo::ShowAndGetInput("Tune In", strQ, "", "" ))
      OnTuneIn(userObj, true); // call ourselves but this time - force playback
  }
}

std::string BoxeeUtils::VerifyBoxeeUrl(const std::string &strUrl)
{
  if (strUrl.substr(0,4) != "http")
    return BXConfiguration::GetInstance().GetStringParam("Boxee.Server","http://app.boxee.tv") + strUrl;

  return strUrl;
}

//void BoxeeUtils::ShowInfoOnVideoItem(CFileItemPtr pItem, bool bKeepData /*=false*/)
//{
//    if (!pItem)
//      return;
//
//    CFileItemPtr itemCopy(new CFileItem(*pItem));
//    CGUIWindowVideoInfo* pDlgInfo = (CGUIWindowVideoInfo*)g_windowManager.GetWindow(WINDOW_VIDEO_INFO);
//
//    FillVideoItemDetails(itemCopy);
//
//    if (itemCopy->HasVideoInfoTag() && itemCopy->GetVideoInfoTag()->m_strFileNameAndPath.IsEmpty())
//      itemCopy->GetVideoInfoTag()->m_strFileNameAndPath = pItem->m_strPath;
//
//    pDlgInfo->ClearAll();
//    pDlgInfo->SetMovie(itemCopy.get());
//    do {
//      pDlgInfo->DoModal();
//    }
//    while (pDlgInfo->NeedRefresh());
//
//    if (bKeepData)
//      *pItem = *(itemCopy.get());
//}

bool BoxeeUtils::ConvertBXAudioToCSong(const BXAudio* pAudio, CSong& song)
{
  song.strFileName = pAudio->m_strPath;
  song.strTitle = pAudio->m_strTitle;
  song.strAlbum = pAudio->m_strAlbum;
  song.strGenre = pAudio->m_strGenre;
  song.iTrack = pAudio->m_iTrackNumber;
  song.iDuration = pAudio->m_iDuration;
  song.iYear = pAudio->m_iYear;

  return true;
}

bool BoxeeUtils::ConvertBXAlbumToCAlbum(BXAlbum* pAlbum, CAlbum& album)
{
  CScraperUrl url(pAlbum->m_strArtwork);

  album.strAlbum = pAlbum->m_strTitle;
  album.strArtist = pAlbum->m_strArtist;
  album.thumbURL = url;
  album.iYear = pAlbum->m_iYear;
  album.strGenre = pAlbum->m_strGenre;
  album.strReview = pAlbum->m_strDescription;
  album.iRating = pAlbum->m_iRating;

  return true;
}

bool BoxeeUtils::ConvertCAlbumToBXAlbum(const MUSIC_GRABBER::CMusicAlbumInfo& info, BOXEE::BXMetadata* pAlbum)
{
  return true;
}

bool BoxeeUtils::ConvertBXMetadataToAlbumInfo(const BOXEE::BXMetadata* pAlbumMetadata, MUSIC_GRABBER::CMusicAlbumInfo& info)
{
  if (!pAlbumMetadata || pAlbumMetadata->GetType() != MEDIA_ITEM_TYPE_ALBUM)
    return false;

  BXAlbum* pAlbum = (BXAlbum*)pAlbumMetadata->GetDetail(MEDIA_DETAIL_ALBUM);

  // TODO: Find out what to do with the artist
  //BXArtist* pArtist = (BXArtist*)pAlbumMetadata->GetDetail(MEDIA_DETAIL_ARTIST);

  CAlbum album;
  ConvertBXAlbumToCAlbum(pAlbum, album);
  info.SetAlbum(album);

  // Convert songs
  VECSONGS songs;
  for (size_t i = 0; i < pAlbum->m_vecSongs.size(); i++)
  {
    BXAudio* pSong = pAlbum->m_vecSongs[i];
    CSong song;
    ConvertBXAudioToCSong(pSong, song);
    song.strArtist = pAlbum->m_strArtist;
    song.strComment = pAlbum->m_strDescription;
    CLog::Log(LOGDEBUG, "BoxeeUtils::ConvertBXMetadataToAlbumInfo, adding song, title = %s, album = %s, artist = %s",
        song.strTitle.c_str(), song.strAlbum.c_str(), song.strArtist.c_str());

    songs.push_back(song);
  }
  info.SetSongs(songs);

  return true;
}

bool BoxeeUtils::AddUnrecognizedTracksFromPath(const CStdString& strPath, const CMusicAlbumInfo& info, CFileItemList& tracks)
{
  // Get other tracks in the same directory and filter out those that have already been recognized by the library
  CFileItemList items;
  VECSONGS songs = info.GetSongs();
  int songSize = songs.size();

  tracks.Clear();

  if (DIRECTORY::CDirectory::GetDirectory(strPath, items))
  {
    // Sort the retreived files by filename
    items.Sort(SORT_METHOD_FILE, SORT_ORDER_ASC);

    // Go over all files in the directory and add those that do not appear in the list of songs
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr pItem = items[i];
      if (pItem->IsAudio())
      {
        bool bShouldAdd = true;
        for ( int j = 0; j < songSize; j++)
        {
          if (songs[j].strFileName == pItem->m_strPath)
          {
            // song already exists in the list of recognized tracks
            bShouldAdd = false;
            break;
          }
        }

        if (bShouldAdd) {
          CLog::Log(LOGDEBUG,"BoxeeUtils::AddUnrecognizedTracksFromPath, SONG, Adding track, title = %s", pItem->GetLabel().c_str());
          tracks.Add(pItem);
        }
      }
    }
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "BoxeeUtils::AddUnrecognizedTracksFromPath, could not get tracks from path = %s", strPath.c_str());
    return false;
  }
}

// Originally the music info tag is used for individual tracks and not albums
// But conventions has never stopped us before
bool BoxeeUtils::ConvertBXAlbumToMusicInfoTag(const BXAlbum* pAlbum, CMusicInfoTag& infoTag)
{
  infoTag.SetTitle("");
  infoTag.SetAlbum(pAlbum->m_strTitle);
  infoTag.SetArtist(pAlbum->m_strArtist);
  infoTag.SetAlbumArtist(pAlbum->m_strArtist);
  // Use only the first genre at this point
  infoTag.SetGenre(pAlbum->m_strGenre);
  infoTag.SetComment(pAlbum->m_strDescription);
  infoTag.SetYear(pAlbum->m_iYear);
  infoTag.SetRating((char)pAlbum->m_iRating);

  return true;
}

bool BoxeeUtils::UserObjToFileItem(const BOXEE::BXObject &obj, CFileItem* pItem)
{
  pItem->SetProperty("user_thumb", BoxeeUtils::AddCredentialsToLink(obj.GetValue(MSG_KEY_THUMB)).c_str());
  pItem->SetProperty("user_id", obj.GetID().c_str());
  pItem->SetProperty("user_name", obj.GetName().c_str());
  pItem->SetProperty("isuser","1");

  return true;
}

bool BoxeeUtils::ObjToFileItem(const BOXEE::BXObject &obj, CFileItem* pItem)
{
  if (!pItem) {
    CLog::Log(LOGERROR, "BoxeeUtils::ObjToFileItem, FEEDLOADER, item is NULL");
    return false;
  }

  bool bResult = false;

  CStdString objType = obj.GetType();

  if (objType.CompareNoCase(MSG_OBJ_TYPE_USER) == 0)
  {
    return UserObjToFileItem(obj, pItem);
  }

  if (!obj.GetValue(MSG_KEY_TITLE).empty())
  {
    pItem->SetLabel(obj.GetValue(MSG_KEY_TITLE));
  }
  else
  {
    pItem->SetLabel(obj.GetName());
  }

  if (objType.CompareNoCase(MSG_OBJ_TYPE_MOVIE) == 0 || objType.CompareNoCase(MSG_OBJ_TYPE_TV) == 0)
  {
    CLog::Log(LOGDEBUG, "BoxeeUtils::ObjToFileItem, FEEDLOADER, obj of movie type");
    pItem->SetProperty("isvideo",true);
    pItem->SetProperty("isfeedvideo",true);

    pItem->SetProperty("feedId", obj.GetID());

    // Set relevant properties for background loader
    pItem->SetProperty("feedVideoName", obj.GetName());
    pItem->SetProperty("feedVideoType", obj.GetType());
    pItem->SetProperty("feedVideoId", obj.GetID());
    pItem->SetProperty("feedVideoIMDB", obj.GetValue(MSG_KEY_IMDB_NUM));

    pItem->SetProperty("feedVideoSeason", obj.GetValue(MSG_KEY_SEASON));
    pItem->SetProperty("feedVideoEpisode", obj.GetValue(MSG_KEY_EPISODE));

    int numOfExtensions = obj.HasExtensions();
    if(numOfExtensions > 0)
    {
      for(int i=0;i<numOfExtensions;i++)
      {
        const Extension* extension = obj.GetExtension(i);
        if(extension)
        {
          CStdString label;
          label.Format("contextmenulabel-%i",i);
          pItem->SetProperty(label, (extension->GetFromMap("label")));

          CStdString url;
          url.Format("contextmenuurl-%i",i);
          pItem->SetProperty(url, extension->GetFromMap("url"));

          CStdString isFolder;
          isFolder.Format("contextmenuisfolder-%i",i);
          bool urlPlayable = IsUrlPlayableForCustomButton(extension->GetFromMap("url"));
          pItem->SetProperty(isFolder, !urlPlayable);

          CStdString thumb;
          thumb.Format("contextmenuthumb-%i",i);
          // Note: We set "provider_thumb" to "contextmenuthumb-%d" because of skin issues
          pItem->SetProperty(thumb, extension->GetFromMap("provider_thumb"));

          CStdString providerThumbOn;
          providerThumbOn.Format("contextmenuproviderthumbon-%i",i);
          pItem->SetProperty(providerThumbOn, extension->GetFromMap("provider_thumb_on"));


          // Not in use at the moment
          //CStdString provider_thumb;
          //provider_thumb.Format("contextmenuprovider_thumb-%i",i);
          //pItem->SetProperty(provider_thumb, extension->GetFromMap("provider_thumb"));


          CStdString priority;
          priority.Format("contextmenupriority-%i",i);
          pItem->SetProperty(priority, extension->GetFromMap("priority"));

          CStdString provider;
          provider.Format("contextmenuprovider-%i",i);
          pItem->SetProperty(provider, extension->GetFromMap("provider"));

          CStdString contentType = extension->GetFromMap("content_type");
          if(contentType.IsEmpty() == false)
          {
            pItem->SetContentType(contentType);
          }
        }
      }

      pItem->SetProperty("NumOfCustomButtons", numOfExtensions);
    }

    if (obj.HasValue(MSG_KEY_TRAILER))
    {
      pItem->SetProperty("HasTrailer", "1");
      pItem->SetProperty("trailer", obj.GetValue(MSG_KEY_TRAILER).c_str());
    }

    pItem->SetProperty("IsTVShow",obj.GetType() == MSG_OBJ_TYPE_TV);
    pItem->SetProperty("IsMovie",obj.GetType() == MSG_OBJ_TYPE_MOVIE);

    CVideoInfoTag* videoTag = pItem->GetVideoInfoTag();
    videoTag->m_strTitle = obj.GetValue(MSG_KEY_TITLE);
    videoTag->m_strPlot = obj.GetDescription();
    videoTag->m_strGenre = obj.GetValue(MSG_KEY_GENRE);

    if(obj.HasValue(MSG_KEY_IMDB_NUM))
    {
      videoTag->m_strIMDBNumber = obj.GetValue(MSG_KEY_IMDB_NUM);
    }

    if(obj.HasValue(MSG_KEY_RUN_TIME))
    {
      videoTag->m_strRuntime = obj.GetValue(MSG_KEY_RUN_TIME);
    }

    if (obj.GetType() == MSG_OBJ_TYPE_TV)
    {
      pItem->SetLabel(obj.GetName());

      videoTag->m_strShowTitle = obj.GetName();

      if(obj.HasValue(MSG_KEY_EPISODE_NAME))
      {
        videoTag->m_strTitle = obj.GetValue(MSG_KEY_EPISODE_NAME);
      }

      if(obj.HasValue(MSG_KEY_SEASON))
      {
        videoTag->m_iSeason = atoi(obj.GetValue(MSG_KEY_SEASON).c_str());
      }

      if(obj.HasValue(MSG_KEY_EPISODE))
      {
        videoTag->m_iEpisode = atoi(obj.GetValue(MSG_KEY_EPISODE).c_str());
      }
    }

    bResult = true;

  }
  else if (objType.CompareNoCase(MSG_OBJ_TYPE_AUDIO_TRACK) == 0 || objType.CompareNoCase(MSG_OBJ_TYPE_AUDIO_ALBUM) == 0) {

    pItem->SetProperty("isalbum",true);
    pItem->SetProperty("isfeedalbum",true);

    CStdString strAlbum = obj.GetValue(MSG_KEY_ALBUM);
    if (strAlbum.IsEmpty())
      strAlbum =  obj.GetValue(MSG_KEY_NAME);

    CStdString strArtist = obj.GetValue(MSG_KEY_ARTIST);
    pItem->SetProperty("feedAlbum", strAlbum);
    pItem->SetProperty("feedArtist", strArtist);
    pItem->SetProperty("feedId", obj.GetID());

    pItem->GetMusicInfoTag()->SetAlbum(strAlbum);
    pItem->GetMusicInfoTag()->SetArtist(strArtist);

    bResult = true;

    pItem->SetProperty("ismusic",true);
  }
  else if (objType.CompareNoCase(MSG_OBJ_TYPE_RADIO) == 0 && !obj.GetValue(MSG_KEY_URL).empty())
  {
    pItem->SetProperty("station",obj.GetValue(MSG_KEY_STATION).c_str());
    pItem->SetProperty("isradio","true");
    if (obj.GetName() == "lastfm")
      pItem->SetProperty("islastfm","true");
    else if (obj.GetName() == "shoutcast" || obj.GetName() == "shout")
      pItem->SetProperty("isshoutcast","true");
    pItem->m_strPath = obj.GetValue(MSG_KEY_URL);
    bResult = true;
  }
  else if (objType.CompareNoCase(MSG_OBJ_TYPE_AUDIO_STREAM) == 0 && !obj.GetValue(MSG_KEY_URL).empty())
  {
    pItem->SetProperty("isinternetstream", true);
    pItem->SetProperty("isaudio", true);
    pItem->SetProperty("description", obj.GetDescription().c_str());
    pItem->SetProperty("hasdescription",true);
    pItem->SetProperty("title", obj.GetValue(MSG_KEY_TITLE).c_str());
    pItem->m_strPath = obj.GetValue(MSG_KEY_URL);
    bResult = true;
  }
  else if ( (objType.CompareNoCase(MSG_OBJ_TYPE_FLASH_STREAM_MOVIE) == 0 ||
      objType.CompareNoCase(MSG_OBJ_TYPE_FLASH_STREAM_TVSHOW) == 0 ||
      objType.CompareNoCase(MSG_OBJ_TYPE_STREAM_MOVIE) == 0 ||
      objType.CompareNoCase(MSG_OBJ_TYPE_STREAM_TV_SHOW) == 0) && !obj.GetValue(MSG_KEY_URL).empty())
  {
    pItem->SetProperty("isinternetstream", true);
    pItem->SetProperty("isvideo", true);
    pItem->SetProperty("description", obj.GetDescription().c_str());
    pItem->SetProperty("title", obj.GetValue(MSG_KEY_TITLE).c_str());
    pItem->SetProperty("hasdescription",true);
    pItem->m_strPath = obj.GetValue(MSG_KEY_URL);

    CVideoInfoTag *tag = pItem->GetVideoInfoTag();
    tag->m_strTitle = obj.GetValue(MSG_KEY_TITLE);
    tag->m_strPlot = obj.GetDescription();
    tag->m_strGenre = obj.GetValue(MSG_KEY_GENRE);
    if (objType.CompareNoCase(MSG_OBJ_TYPE_FLASH_STREAM_TVSHOW) == 0 || objType.CompareNoCase(MSG_OBJ_TYPE_STREAM_TV_SHOW) == 0)
    {
      pItem->SetLabel(obj.GetName());

      tag->m_strShowTitle = obj.GetName();

      if(obj.HasValue(MSG_KEY_EPISODE_NAME))
      {
        tag->m_strTitle = obj.GetValue(MSG_KEY_EPISODE_NAME);
      }

      tag->m_iSeason = atoi(obj.GetValue(MSG_KEY_SEASON).c_str());
      tag->m_iEpisode = atoi(obj.GetValue(MSG_KEY_EPISODE).c_str());
    }

    if(obj.HasValue(MSG_KEY_RUN_TIME))
    {
      tag->m_strRuntime = obj.GetValue(MSG_KEY_RUN_TIME);
    }

    bResult = true;
  }
  else if ( objType.CompareNoCase(MSG_OBJ_TYPE_VIDEO_STREAM) == 0 && !obj.GetValue(MSG_KEY_URL).empty())
  {
    pItem->SetProperty("isinternetstream", true);
    pItem->SetProperty("isvideo", true);
    pItem->SetProperty("description", obj.GetDescription().c_str());
    pItem->SetProperty("title", obj.GetValue(MSG_KEY_TITLE).c_str());
    pItem->SetProperty("hasdescription",true);
    pItem->m_strPath = obj.GetValue(MSG_KEY_URL);
    bResult = true;

    if ((pItem->GetProperty("title")).Right(8) == "Trailer)")
    {
      pItem->SetProperty("isTrailer",true);
    }
  }
  else if (objType.CompareNoCase(MSG_OBJ_TYPE_PICTURE) == 0 && !obj.GetValue(MSG_KEY_URL).empty())
  {
    pItem->m_strPath = obj.GetValue(MSG_KEY_URL);
    pItem->SetProperty("title", (obj.GetName()).c_str());
    pItem->SetProperty("isPicture", true);
  }
  /*
  else if (objType.CompareNoCase(MSG_OBJ_TYPE_APPLICATION) == 0  && !obj.GetValue(MSG_KEY_URL).empty())
  {
    pItem->SetProperty("isApp", true);
    pItem->m_strPath = obj.GetValue(MSG_KEY_URL);
  }
   */

  // In case BXObject has MSG_KEY_THUMB -> Set the item Thumbnail
  if (!obj.GetValue(MSG_KEY_THUMB).empty())
  {
    pItem->SetThumbnailImage(obj.GetValue(MSG_KEY_THUMB));
    pItem->SetProperty("OriginalThumb",pItem->GetThumbnailImage());
  }

  // In case BXObject has MSG_KEY_RELEASE_DATE -> Set the item "releasedate" property
  if (obj.HasValue(MSG_KEY_RELEASE_DATE))
  {
    pItem->SetProperty("releasedate",obj.GetValue(MSG_KEY_RELEASE_DATE).c_str());

    //CLog::Log(LOGDEBUG,"BoxeeUtils::ObjToFileItem - In Item [path=%s][label=%s] property was set [releasedate=%s] (rdate)",(pItem->m_strPath).c_str(),(pItem->GetLabel()).c_str(),(pItem->GetProperty("releasedate")).c_str());
  }

  // In case BXObject has MSG_KEY_RELEASE_YEAR -> Set the item "releaseyear" property
  if (obj.HasValue(MSG_KEY_RELEASE_DATE))
  {
    pItem->SetProperty("releaseyear",obj.GetValue(MSG_KEY_RELEASE_YEAR).c_str());

    //CLog::Log(LOGDEBUG,"BoxeeUtils::ObjToFileItem - In Item [path=%s][label=%s] property was set [releasedate=%s] (rdate)",(pItem->m_strPath).c_str(),(pItem->GetLabel()).c_str(),(pItem->GetProperty("releasedate")).c_str());
  }

  // In case BXObject has MSG_KEY_CONTENT_TYPE -> Set the item ContentType
  if (obj.HasValue(MSG_KEY_CONTENT_TYPE))
  {
    pItem->SetContentType(obj.GetValue(MSG_KEY_CONTENT_TYPE));

    //CLog::Log(LOGDEBUG,"BoxeeUtils::ObjToFileItem - In Item [path=%s][label=%s] ContentType was set to [%s] (ctype)",(pItem->m_strPath).c_str(),(pItem->GetLabel()).c_str(),(pItem->GetContentType()).c_str());
  }

  // In case BXObject has PLAY provider data
  if(obj.HasValue(MSG_KEY_PLAY_PROVIDER_LABEL))
  {
    pItem->SetProperty("play_provider_label",obj.GetValue(MSG_KEY_PLAY_PROVIDER_LABEL));
    //CLog::Log(LOGDEBUG,"BoxeeUtils::ObjToFileItem - In Item [path=%s][label=%s] after setting property [play_provider_label=%s] (pprov)",(pItem->m_strPath).c_str(),(pItem->GetLabel()).c_str(),(pItem->GetProperty("play_provider_label")).c_str());
  }

  if(obj.HasValue(MSG_KEY_PROVIDER))
  {
    pItem->SetProperty("provider",obj.GetValue(MSG_KEY_PROVIDER));
    //CLog::Log(LOGDEBUG,"BoxeeUtils::ObjToFileItem - In Item [path=%s][label=%s] after setting property [provider=%s] (pprov)",(pItem->m_strPath).c_str(),(pItem->GetLabel()).c_str(),(pItem->GetProperty("provider")).c_str());
  }

  if(obj.HasValue(MSG_KEY_PLAY_PROVIDER_THUMB))
  {
    pItem->SetProperty("play_provider_thumb",obj.GetValue(MSG_KEY_PLAY_PROVIDER_THUMB));
    //CLog::Log(LOGDEBUG,"BoxeeUtils::ObjToFileItem - In Item [path=%s][label=%s] after setting property [play_provider_thumb=%s] (pprov)",(pItem->m_strPath).c_str(),(pItem->GetLabel()).c_str(),(pItem->GetProperty("play_provider_thumb")).c_str());
  }

  if(obj.HasValue(MSG_KEY_PLAY_PROVIDER_THUMB_ON))
  {
    pItem->SetProperty("play_provider_thumb_on",obj.GetValue(MSG_KEY_PLAY_PROVIDER_THUMB_ON));

    //CLog::Log(LOGDEBUG,"BoxeeUtils::ObjToFileItem - In Item [path=%s][label=%s] after setting property [play_provider_thumb_on=%s] (pprov)",(pItem->m_strPath).c_str(),(pItem->GetLabel()).c_str(),(pItem->GetProperty("play_provider_thumb_on")).c_str());
  }

  if (obj.HasValue(MSG_KEY_ADULT))
  {
    CStdString adultValue = obj.GetValue(MSG_KEY_ADULT);
    adultValue.ToLower();
    pItem->SetAdult((adultValue == "true" || adultValue == "1") ? true : false);
  }

  if (obj.HasValue(MSG_KEY_COUNTRY) && obj.HasValue(MSG_KEY_COUNTRY_ALLOW))
  {
    CStdString allow = obj.GetValue(MSG_KEY_COUNTRY_ALLOW);
    allow.ToLower();
    pItem->SetCountryRestriction(obj.GetValue(MSG_KEY_COUNTRY), (allow == "true" || allow == "1") ? true : false);
  }

  if (obj.HasValue(MSG_KEY_BOXEE_ID))
  {
    pItem->SetProperty("boxeeid", obj.GetValue(MSG_KEY_BOXEE_ID));
  }

  if (obj.HasValue(MSG_KEY_SHOW_ID))
  {
    pItem->SetProperty("showid", obj.GetValue(MSG_KEY_SHOW_ID));
  }

  if (obj.HasValue(MSG_KEY_SHOW_NAME))
  {
    pItem->SetProperty("showname", obj.GetValue(MSG_KEY_SHOW_NAME));
  }

  return bResult;
}

bool BoxeeUtils::ConvertAlbumInfoToBXMetadata(const CMusicAlbumInfo& info, BXMetadata* pMetadata)
{
  if (!pMetadata || pMetadata->GetType() != MEDIA_ITEM_TYPE_AUDIO) return false;

  BXAlbum* pAlbum = (BXAlbum*)pMetadata->GetDetail(MEDIA_DETAIL_ALBUM);
  BXArtist* pArtist = (BXArtist*)pMetadata->GetDetail(MEDIA_DETAIL_ARTIST);
  BXAudio* pAudio = (BXAudio*)pMetadata->GetDetail(MEDIA_DETAIL_AUDIO);

  if (!pAlbum || !pArtist || !pAudio)
  {
    CLog::Log(LOGERROR, "%s - one of the details is NULL!! album: %p, artist: %p, audio: %p. ", __FUNCTION__, pAlbum, pArtist, pAudio);
    return false;
  }

  CAlbum album  = info.GetAlbum();
  pAlbum->m_strTitle = album.strAlbum;
  pAlbum->m_strArtist = album.strArtist;

  if (album.thumbURL.m_url.size() > 0)
    pAlbum->m_strArtwork = album.thumbURL.m_url[0].m_url;

  pAlbum->m_strGenre = album.strGenre;
  pAlbum->m_strDescription = album.strReview;

  // Perform some sanity checks, TODO: Limited range, may be problematic
  if (album.iYear > 0 && album.iYear < 2030)
  {
    pAlbum->m_iYear = album.iYear;
  }
  else
  {
    pAlbum->m_iYear = 1970;
  }

  pAlbum->m_iNumTracs = info.GetSongs().size();

  if (album.iRating > 0)
  {
    pAlbum->m_iRating = album.iRating;
  }
  else
  {
    pAlbum->m_iRating = 0;
  }

  pAudio->m_strAlbum = album.strAlbum;
  pArtist->m_strName = album.strArtist;

  // Add songs
  VECSONGS songs = info.GetSongs();
  for (unsigned int i = 0; i < songs.size(); i++)
  {
    CSong song = songs[i];

    // Add another audio detail for the list of songs in the album
    BXAudio* pAudio = new BXAudio();
    pAudio->m_strTitle = song.strTitle;
    if (song.iDuration > 0)
    {
      pAudio->m_iDuration = song.iDuration;
    }
    else
    {
      pAudio->m_iDuration = 0;
    }

    pAudio->m_iYear = song.iYear;

    if (song.iTrack > 0)
    {
      pAudio->m_iTrackNumber = song.iTrack;
    }
    else
    {
      pAudio->m_iTrackNumber = 0;
    }
    pAudio->m_strDescription = song.strComment;
    pAudio->m_strGenre = album.strGenre;

    pAlbum->m_vecSongs.push_back(pAudio);
  }

  return true;
}

/*
bool BoxeeUtils::ConvertAlbumInfoToBXFolder(const MUSIC_GRABBER::CMusicAlbumInfo& albumInfo, const CFileItemList& albumFileItems, BOXEE::BXFolder& folder)
{
  // Create bxfolder from the album description and the list of files representing the tracks
  BXMetadata metadata(MEDIA_ITEM_TYPE_AUDIO);
  ConvertAlbumInfoToBXMetadata(albumInfo, &metadata);

  BXAlbum* pAlbum = (BXAlbum*)metadata.GetDetail(MEDIA_DETAIL_ALBUM);
  BXArtist* pArtist = (BXArtist*)metadata.GetDetail(MEDIA_DETAIL_ARTIST);

  folder.SetDetail(MEDIA_DETAIL_ALBUM, new BXAlbum(*pAlbum));
  folder.SetDetail(MEDIA_DETAIL_ARTIST, new BXArtist(*pArtist));

  folder.SetPath(albumFileItems.m_strPath);

  // Create album songs, take only those songs that have a matching file
  for (size_t i = 0; i < pAlbum->m_vecSongs.size(); i++)
  {
    BXMetadata* pTrackMetadata = new BXMetadata(MEDIA_ITEM_TYPE_AUDIO);
    pTrackMetadata->SetDetail(MEDIA_DETAIL_ALBUM, new BXAlbum(*pAlbum));
    pTrackMetadata->SetDetail(MEDIA_DETAIL_ARTIST, new BXArtist(*pArtist));

    BXAudio* pSong = pAlbum->m_vecSongs[i];

    // go over album items, and find the one that matches
    int j = 0;
    for (; j < albumFileItems.Size(); j++)
    {
      if (albumFileItems.Get(j)->HasMusicInfoTag() && pSong->m_iTrackNumber == albumFileItems.Get(j)->GetMusicInfoTag()->GetTrackNumber())
      {
        // Set the path to the matched song
        pSong->m_strPath = albumFileItems.Get(j)->m_strPath;

        pTrackMetadata->SetDetail(MEDIA_DETAIL_AUDIO, new BXAudio(*(pSong)));
        folder.m_vecFiles.push_back(pTrackMetadata);
        break;
      }
    }

    if (j == albumFileItems.Size())
    {
      // song was not matched, add the next path
      if ((int)i < j) {
        pSong->m_strPath = albumFileItems.Get(i)->m_strPath;
        pTrackMetadata->SetDetail(MEDIA_DETAIL_AUDIO, new BXAudio(*(pSong)));
        folder.m_vecFiles.push_back(pTrackMetadata);
      }
    }
  }

  return true;
}
 */


bool BoxeeUtils::ConvertBXVideoToVideoInfoTag(const BXVideo* pVideo, CVideoInfoTag &info)
{
  info.m_strTitle = pVideo->m_strTitle;
  info.m_strDirector = pVideo->m_strDirector;
  info.m_iYear = pVideo->m_iYear;
  info.m_strPlot = pVideo->m_strExtDescription;
  info.m_strPlotOutline = pVideo->m_strDescription;
  info.m_strGenre = pVideo->m_strGenre;
  info.m_strMPAARating = pVideo->m_strMPAARating;
  info.m_strIMDBNumber = pVideo->m_strIMDBKey;
  info.m_strStudio = pVideo->m_strStudio;
  info.m_strTagLine = pVideo->m_strTagLine;
  info.m_strWritingCredits = pVideo->m_strCredits;

  if(pVideo->m_iDuration > 0)
  {
    info.m_strRuntime = BXUtils::IntToString(pVideo->m_iDuration);
  }

  CScraperUrl scraperUrl(pVideo->m_strCover);
  info.m_strPictureURL = scraperUrl;
  std::vector< SActorInfo > cast;

  info.m_strShowTitle = pVideo->m_strShowTitle;

  info.m_strTrailer = pVideo->m_strTrailerUrl;

  info.m_iSeason = pVideo->m_iSeason;
  info.m_iEpisode = pVideo->m_iEpisode;
  info.m_fRating = (float)pVideo->m_iRating;

  // Go over all actors and add them to the cast
  for (unsigned int i = 0; i < pVideo->m_vecActors.size(); i++) {
    SActorInfo info;
    info.strName = pVideo->m_vecActors[i];
    cast.push_back(info);
  }
  info.m_cast = cast;

  return true;

}



void BoxeeUtils::FillVideoItemDetails(CFileItemPtr pItem)
{
  if (!pItem->GetPropertyBOOL("HasMetaData") && !pItem->HasVideoInfoTag() && !pItem->m_strPath.IsEmpty())
  {
    BOXEE::BXMetadataEngine &engine  = Boxee::GetInstance().GetMetadataEngine();
    BXMetadata metadata(MEDIA_ITEM_TYPE_VIDEO);
    if (engine.GetVideoByPath(pItem->m_strPath, &metadata) == MEDIA_DATABASE_OK)
    {
      DIRECTORY::CBoxeeDatabaseDirectory::CreateVideoItem(&metadata, pItem.get());
    }

    pItem->SetCachedVideoThumb();
    pItem->SetProperty("HasMetaData","1");
  }
}


void BoxeeUtils::BuildPlaylist(const CFileItem *pRoot, CFileItemList &items, bool bAddAudio, bool bAddVideo, bool bAddPictures)
{
  if (!pRoot || pRoot->IsParentFolder() || pRoot->m_strPath.IsEmpty())
    return;

  if (pRoot->m_bIsFolder)
  {
    CFileItemList files;
    DIRECTORY::IDirectory *pDir = DIRECTORY::CFactoryDirectory::Create(pRoot->m_strPath);
    if (pDir)
    {
      pDir->GetDirectory(pRoot->m_strPath, files);
      for (int i=0; i<files.Size(); i++)
        BuildPlaylist(files[i].get(), items, bAddAudio, bAddVideo, bAddPictures);
      delete pDir;
    }
  }
  else
  {
    CFileItemPtr newItem(new CFileItem(*pRoot));
    if (bAddAudio && pRoot->IsAudio() && !pRoot->IsPlayList())
      items.Add(newItem);
    else if (bAddVideo && pRoot->IsVideo() && !pRoot->IsPlayList())
      items.Add(newItem);
    else if (bAddPictures && pRoot->IsPicture() && !pRoot->IsPlayList())
      items.Add(newItem);
  }
}

void BoxeeUtils::SetDefaultIcon(CFileItem *pItem)
{
  if (pItem->GetPropertyBOOL("IsArtist"))
  {
    pItem->SetIconImage("defaultartist.png");
  }
  else if (pItem->GetPropertyBOOL("IsShare"))
  {
    if (pItem->GetPropertyBOOL("IsVideo"))
      pItem->SetIconImage("defaultinternetvideo.png");
    else if (pItem->GetPropertyBOOL("IsMusic"))
      pItem->SetIconImage("defaultinternetmusic.png");
    else if (pItem->GetPropertyBOOL("IsAlbum"))
      pItem->SetIconImage("defaultmusicalbum.png");
    else if (pItem->GetPropertyBOOL("IsPicture"))
      pItem->SetIconImage("defaultinternetpictures.png");
    else if (pItem->GetPropertyBOOL("IsRemovable"))
      pItem->SetIconImage("defaultremovable.png");
    else if (pItem->GetPropertyBOOL("isNetwork"))
      pItem->SetIconImage("defaultnetworksource.png");
    else if (pItem->GetPropertyBOOL("isSMB"))
      pItem->SetIconImage("defaultnetworksource.png");
    else if (pItem->GetPropertyBOOL("isUPNP"))
      pItem->SetIconImage("defaultnetworksource.png");
    else if (pItem->GetPropertyBOOL("isFTP"))
      pItem->SetIconImage("defaultnetworksource.png");
    else if (pItem->GetPropertyBOOL("addshare"))
      pItem->SetIconImage("defaultaddsource.png");
    else
      pItem->SetIconImage("defaultharddisk.png");
  }
  else if (pItem->m_strPath.Left(7) == "hulu://")
  {
    pItem->SetIconImage("hulu.png");
  }
  else if (pItem->GetPropertyBOOL("isinternetstream") && pItem->GetPropertyBOOL("isvideo")) {
    pItem->SetIconImage("defaultstreamvideo.png");
  }
  else if (pItem->GetPropertyBOOL("isinternetstream") && pItem->GetPropertyBOOL("isaudio")) {
    pItem->SetIconImage("defaultstreammusic.png");
  }
  else if (pItem->IsRSS() || pItem->GetPropertyBOOL("isrss"))
  {
    if (pItem->GetPropertyBOOL("IsVideo"))
      pItem->SetIconImage("defaultrssvideo.png");
    else if (pItem->GetPropertyBOOL("IsMusic"))
      pItem->SetIconImage("defaultrssmusic.png");
    else
      pItem->SetIconImage("defaultrss.png");
  }
  else if (pItem->IsLastFM() || pItem->GetPropertyBOOL("islastfm"))
  {
    pItem->SetIconImage(pItem->m_bIsFolder?"lastfm_folder.png":"lastfm_station.png");
  }
  else if (pItem->IsShoutCast())
  {
    pItem->SetIconImage(pItem->m_bIsFolder?"shoutcast_folder.png":"shoutcast_station.png");
  }
  else if (pItem->GetPropertyBOOL("IsMusic") && pItem->GetPropertyBOOL("IsAlbum"))
  {
    pItem->SetIconImage("defaultmusicalbum.png");
  }
  else if (pItem->GetPropertyBOOL("isvideo") || pItem->GetProperty("isvideo") == "true")
  {
    pItem->SetIconImage("defaultVideo.png");
  }
  else if (pItem->m_bIsFolder)
  {
    if (pItem->IsVideo() || pItem->GetPropertyBOOL("IsVideoFolder") || pItem->GetPropertyBOOL("IsTVShowFolder"))
      pItem->SetIconImage("defaultvideofolder.png");
    else if (pItem->IsAudio() || pItem->GetPropertyBOOL("IsMusicFolder"))
      pItem->SetIconImage("defaultaudiofolder.png");
    else if (pItem->IsPicture() || pItem->GetPropertyBOOL("IsPictureFolder"))
      pItem->SetIconImage("defaultpicturefolder.png");
  }
  else if (pItem->GetPropertyBOOL("IsMusic"))
  {
    pItem->SetIconImage("defaultaudiotrack.png");
  }
  else if (pItem->IsPicture() || pItem->GetPropertyBOOL("IsPicture"))
  {
    pItem->SetIconImage("defaultpicture.png");
  }
}

bool BoxeeUtils::ParseBoxeeDbUrl(const std::string& strURL, std::string& strDir, std::string& strFile, std::map<std::string, std::string>& mapParams)
{
  // We expect something like protocol://dir/file/?param=value&param=value ...
  // so first lets chop the protocol:// prefix
  size_t pos = strURL.find_first_of( "://" );

  if( pos == std::string::npos )
    return false;

  CStdString strDirectory( strURL.c_str() + pos + strlen( "://" ) );

  CStdStringArray arrDirAndParams;
  CStdStringArray arrParamPairs;
  CStdStringArray arrKeyAndValue;

  CStdString strParams;
  CStdString strParamPair;

  // Remove final slash if exists
  if (CUtil::HasSlashAtEnd(strDirectory))
    CUtil::RemoveSlashAtEnd(strDirectory);

  StringUtils::SplitString(strDirectory, "?", arrDirAndParams);

  if (arrDirAndParams.size() == 2) {
    strDirectory = arrDirAndParams[0];
    strParams = arrDirAndParams[1];

    StringUtils::SplitString(strParams, "&", arrParamPairs);
    // Go over all param pairs
    for (size_t i = 0; i < arrParamPairs.size(); i++) {
      strParamPair = arrParamPairs[i];
      StringUtils::SplitString(strParamPair, "=", arrKeyAndValue);
      if (arrKeyAndValue.size() != 2) {
        CLog::Log(LOGERROR, "Invalid URL %s", strURL.c_str());
        return false;
      }
      mapParams[arrKeyAndValue[0]] = arrKeyAndValue[1];
    }
  }
  else if (arrDirAndParams.size() == 1) {
    // No parameters
    strDirectory = arrDirAndParams[0];
  }
  else {
    CLog::Log(LOGERROR, "Invalid URL %s", strURL.c_str());
    return false;
  }

  // Delete the last slash
  if (CUtil::HasSlashAtEnd(strDirectory))
    strDirectory.Delete(strDirectory.size()-1);

  CStdStringArray Path;

  StringUtils::SplitString( strDirectory, "/", Path );
  if( Path.size() == 1 )  // only dir, no file
  {
    strFile.clear();
    strDir = Path[ 0 ];
  }
  else
  {
    strFile = Path[ Path.size() - 1 ];
    strDir  = Path[ Path.size() - 2 ];
  }

  return true;
}


bool BoxeeUtils::IsUrlPlayableForCustomButton(const CStdString& url)
{
  return (url.Left(8) == "flash://" ||
      url.Left(7) == "http://"  ||
      url.Left(6) == "app://");
}

void BoxeeUtils::AddTrailerStrToItemLabel(CFileItem& item)
{
  if((item.GetLabel()).Right(8) != "Trailer)")
  {
    CLog::Log(LOGDEBUG,"BoxeeUtils::AddTrailerStrToItemLabel - Going to add (Trailer) to item label. [path=%s][label=%s] (tr)",(item.m_strPath).c_str(),(item.GetLabel()).c_str());

    CStdString trailerTitle = item.GetLabel();
    trailerTitle += " (";

    CStdString itemPath = item.m_strPath;
    if(itemPath.Find("480p") != -1)
    {
      CLog::Log(LOGDEBUG,"BoxeeUtils::AddTrailerStrToItemLabel - Item is 480p Trailer. [path=%s][label=%s] (tr)",(item.m_strPath).c_str(),(item.GetLabel()).c_str());

      trailerTitle += "480p ";
    }
    else if(itemPath.Find("720p") != -1)
    {
      CLog::Log(LOGDEBUG,"BoxeeUtils::AddTrailerStrToItemLabel - Item is 720p Trailer. [path=%s][label=%s] (tr)",(item.m_strPath).c_str(),(item.GetLabel()).c_str());

      trailerTitle += "720p ";
    }
    else if(itemPath.Find("1080p") != -1)
    {
      CLog::Log(LOGDEBUG,"BoxeeUtils::AddTrailerStrToItemLabel - Item is 1080p Trailer. [path=%s][label=%s] (tr)",(item.m_strPath).c_str(),(item.GetLabel()).c_str());

      trailerTitle += "1080p ";
    }
    else
    {
      // Add nothing
    }

    trailerTitle += g_localizeStrings.Get(20410);
    trailerTitle += ")";

    item.SetLabel(trailerTitle);

    CVideoInfoTag* videoInfoTag = item.GetVideoInfoTag();
    videoInfoTag->m_strTitle = trailerTitle;

    CLog::Log(LOGDEBUG,"BoxeeUtils::AddTrailerStrToItemLabel - After adding (Trailer) to item label. [path=%s][label=%s][VideoInfoTagTitle=%s] (tr)",(item.m_strPath).c_str(),(item.GetLabel()).c_str(),(videoInfoTag->m_strTitle).c_str());
  }
  else
  {
    CLog::Log(LOGDEBUG,"BoxeeUtils::AddTrailerStrToItemLabel - Item label already contain (Trailer) therefore no need to add. [path=%s][label=%s] (tr)",(item.m_strPath).c_str(),(item.GetLabel()).c_str());
  }
}

bool BoxeeUtils::CreateDirectoryThumbnail(CFileItem& dir,CFileItemList &items)
{
  CLog::Log(LOGDEBUG,"BoxeeUtils::CreateDirectoryThumbnail - Enter function with directory item [path=%s][ItemsSize=%d] (foldert)",(dir.m_strPath).c_str(),items.Size());

  CStdString dirCacheThumbPath = dir.GetCachedPictureThumb();

  if(!CFile::Exists(dirCacheThumbPath))
  {
    CLog::Log(LOGDEBUG,"BoxeeUtils::CreateDirectoryThumbnail - For directory item [path=%s] the CachePictureThumbPath [%s] DOESN'T EXIST. Going to search if an item in the directory can be a thumb (foldert)",(dir.m_strPath).c_str(),dirCacheThumbPath.c_str());

    CFileItemList dirItems;
    dirItems.Clear();

    CStdString picturesQualifyAsThumbnailArray[CQualifyPicNameAsThumb::NUM_OF_QUALIFY_PIC_NAME_AS_THUMB] = {"","",""};
    bool picturesQualifyAsThumbnailWasFound = false;

    // count the number of images in the directory
    for (int i=0; i < items.Size();i++)
    {
      //CLog::Log(LOGDEBUG,"BoxeeUtils::CreateDirectoryThumbnail - [%d] In directory item [path=%s] check if item [path=%s] can be a thumb (foldert)",i,(dir.m_strPath).c_str(),(items[i]->m_strPath).c_str());

      if (!items[i]->IsPicture() || items[i]->IsZIP() || items[i]->IsRAR() || items[i]->IsPlayList())
      {
        //CLog::Log(LOGDEBUG,"BoxeeUtils::CreateDirectoryThumbnail - [%d] In directory item [path=%s] item [path=%s] is NOT a picture -> Cannot be a thumb (foldert)",i,(dir.m_strPath).c_str(),(items[i]->m_strPath).c_str());
      }
      else
      {
        CFileItemPtr newItem = items[i];
        dirItems.Add(newItem);

        //CLog::Log(LOGDEBUG,"BoxeeUtils::CreateDirectoryThumbnail - [%d] In directory item [path=%s] after adding newItem [path=%s] to dirItems [size=%d] going to check is it can be a thumb -> call IsPictureQulifyAsThumb() (foldert)",i,(dir.m_strPath).c_str(),(newItem->m_strPath).c_str(),dirItems.Size());

        bool retVal = IsPictureQulifyAsThumb(newItem,picturesQualifyAsThumbnailArray);

        if((retVal == true) && (picturesQualifyAsThumbnailWasFound == false))
        {
          picturesQualifyAsThumbnailWasFound = true;
        }

        //CLog::Log(LOGDEBUG,"BoxeeUtils::CreateDirectoryThumbnail - [%d] In directory item [path=%s] check if newItem [path=%s] can be a thumb by calling to IsPictureQulifyAsThumb() returned [%d]. [picturesQualifyAsThumbnailWasFound=%d] (foldert)",i,(dir.m_strPath).c_str(),(newItem->m_strPath).c_str(),retVal,picturesQualifyAsThumbnailWasFound);
      }
    }

    if(dirItems.Size() < 1)
    {
      CLog::Log(LOGDEBUG,"BoxeeUtils::CreateDirectoryThumbnail - In directory item [path=%s] there are no picture items that could be thumb (foldert)",(dir.m_strPath).c_str());

      return false;
    }
    else
    {
      CLog::Log(LOGDEBUG,"BoxeeUtils::CreateDirectoryThumbnail - In directory item [path=%s] there are [%d] picture items that can be thumb (foldert)",(dir.m_strPath).c_str(),dirItems.Size());
    }

    if(picturesQualifyAsThumbnailWasFound)
    {
      // We found a picture in the directory that qualify as thumbnail

      CStdString thumbPath = GetPicturePathQulifyAsThumb(picturesQualifyAsThumbnailArray);

      CLog::Log(LOGDEBUG,"BoxeeUtils::CreateDirectoryThumbnail - [picturesQualifyAsThumbnailWasFound=%d] -> Call to GetPicturePathQulifyAsThumb() returned [thumbPath=%s] (foldert)",picturesQualifyAsThumbnailWasFound,thumbPath.c_str());

      if(thumbPath.IsEmpty())
      {
        // Error log was written in GetPicturePathQulifyAsThumb()

        return false;
      }

      CLog::Log(LOGDEBUG,"BoxeeUtils::CreateDirectoryThumbnail - In directory item [path=%s] found a picture that can be a thumb [%s] (foldert)",(dir.m_strPath).c_str(),thumbPath.c_str());

      if(CPicture::CreateThumbnail(thumbPath, dirCacheThumbPath))
      {
        CLog::Log(LOGDEBUG,"BoxeeUtils::CreateDirectoryThumbnail - For item [path=%s] a thumb [%s] was created (foldert)",(dir.m_strPath).c_str(),dirCacheThumbPath.c_str());

        return true;
      }
      else
      {
        CLog::Log(LOGERROR,"BoxeeUtils::CreateDirectoryThumbnail - For item [path=%s] failed to create thumb [%s] (foldert)",(dir.m_strPath).c_str(),dirCacheThumbPath.c_str());

        return false;
      }
    }
    else
    {
      CLog::Log(LOGDEBUG,"BoxeeUtils::CreateDirectoryThumbnail - In directory item [path=%s] DIDN'T found a picture that can be a thumb. Going to create one (foldert)",(dir.m_strPath).c_str());

      // randomize them
      dirItems.Randomize();

      CStdString folderThumb;

      if (dirItems.Size() < 4)
      {
        // less than 4 images, so just grab a single random thumb

        CLog::Log(LOGDEBUG,"BoxeeUtils::CreateDirectoryThumbnail - Folder contain [%d] pictures and the cache returned [%s] for thumb. Going to create FolderThumb from picture [%s] (foldert)(th)",dirItems.Size(),dirCacheThumbPath.c_str(),(dirItems[0]->m_strPath).c_str());

        CPicture::CreateThumbnail(dirItems[0]->m_strPath, dirCacheThumbPath, true);
      }
      else
      {
        CLog::Log(LOGDEBUG,"BoxeeUtils::CreateDirectoryThumbnail - Folder contain [%d] pictures and the cache returned [%s] for thumb. Going to create FolderThumb from the following pictures (foldert)(th)",dirItems.Size(),dirCacheThumbPath.c_str());

        // ok, now we've got the files to get the thumbs from, lets create it...
        // we basically load the 4 thumbs, resample to 62x62 pixels, and add them
        CStdString strFiles[4];
        for (int thumb = 0; thumb < 4; thumb++)
        {
          strFiles[thumb] = dirItems[thumb]->m_strPath;

          CLog::Log(LOGDEBUG,"BoxeeUtils::CreateDirectoryThumbnail - [%d] [%s] (foldert)(th)",thumb,(strFiles[thumb]).c_str());
        }

        CPicture::CreateFolderThumb(strFiles, dirCacheThumbPath);
      }

      return true;
    }
  }
  else
  {
    CLog::Log(LOGDEBUG,"BoxeeUtils::CreateDirectoryThumbnail - For item [path=%s] the CachePictureThumbPath [%s] EXIST (foldert)",(dir.m_strPath).c_str(),dirCacheThumbPath.c_str());

    return true;
  }
}

bool BoxeeUtils::IsPictureQulifyAsThumb(CFileItemPtr item,CStdString* picturesQualifyAsThumbnailArray)
{
  bool retVal = false;
  CStdString fullPath = item->m_strPath;
  CStdString picFileName = CUtil::GetFileName(fullPath);
  picFileName.ToLower();

  CLog::Log(LOGDEBUG,"BoxeeUtils::IsPictureQulifyAsThumb - Enter function with item [path=%s]. [picFileNameLower=%s] (foldert)",(item->m_strPath).c_str(),picFileName.c_str());

  if(picFileName == "folder.jpg")
  {
    picturesQualifyAsThumbnailArray[CQualifyPicNameAsThumb::FOLDER] =  fullPath;
    retVal = true;
    CLog::Log(LOGDEBUG,"BoxeeUtils::IsPictureQulifyAsThumb - Path of PictureItem [path=%s][picFileNameLower=%s] was entered to array under key [FOLDER] (foldert)",fullPath.c_str(),picFileName.c_str());
  }
  else if(picFileName == "preview.jpg")
  {
    picturesQualifyAsThumbnailArray[CQualifyPicNameAsThumb::PREVIEW] =  fullPath;
    retVal = true;
    CLog::Log(LOGDEBUG,"BoxeeUtils::IsPictureQulifyAsThumb - Path of PictureItem [path=%s][picFileNameLower=%s] was entered to array under key [PREVIEW] (foldert)",fullPath.c_str(),picFileName.c_str());
  }
  else
  {
    CStdString picFileFolderName = BXUtils::GetFolderName(fullPath);
    picFileFolderName.ToLower();
    CLog::Log(LOGDEBUG,"BoxeeUtils::IsPictureQulifyAsThumb - For directory item [path=%s] the [picFileFolderName=%s]. [picFileNameLower=%s] (foldert)",(item->m_strPath).c_str(),picFileFolderName.c_str(),picFileName.c_str());

    if(picFileName == (picFileFolderName + ".jpg"))
    {
      picturesQualifyAsThumbnailArray[CQualifyPicNameAsThumb::FOLDER_NAME] =  fullPath;
      retVal = true;
      CLog::Log(LOGDEBUG,"BoxeeUtils::IsPictureQulifyAsThumb - Path of PictureItem [path=%s][picFileNameLower=%s] was entered to array under key [FOLDER_NAME] (foldert)",fullPath.c_str(),picFileName.c_str());
    }
    else if(picFileName == "cover.jpg")
    {
      picturesQualifyAsThumbnailArray[CQualifyPicNameAsThumb::COVER] =  fullPath;
      retVal = true;
      CLog::Log(LOGDEBUG,"BoxeeUtils::IsPictureQulifyAsThumb - Path of PictureItem [path=%s][picFileNameLower=%s] was entered to array under key [COVER] (foldert)",fullPath.c_str(),picFileName.c_str());
    }
    else if(picFileName == "front.jpg")
    {
      picturesQualifyAsThumbnailArray[CQualifyPicNameAsThumb::FRONT] =  fullPath;
      retVal = true;
      CLog::Log(LOGDEBUG,"BoxeeUtils::IsPictureQulifyAsThumb - Path of PictureItem [path=%s][picFileNameLower=%s] was entered to array under key [FRONT] (foldert)",fullPath.c_str(),picFileName.c_str());
    }
  }

  return retVal;
}

CStdString BoxeeUtils::GetPicturePathQulifyAsThumb(CStdString* picturesQualifyAsThumbnailArray)
{
  CLog::Log(LOGDEBUG,"BoxeeUtils::GetPicturePathQulifyAsThumb - Enter function with array (foldert)");

  CStdString picturePathQulifyAsThumb = "";

  for(int picNameAsThumb=0; picNameAsThumb<CQualifyPicNameAsThumb::NUM_OF_QUALIFY_PIC_NAME_AS_THUMB; picNameAsThumb++)
  {
    if(picturesQualifyAsThumbnailArray[picNameAsThumb] != "")
    {
      picturePathQulifyAsThumb = picturesQualifyAsThumbnailArray[picNameAsThumb];

      switch(picNameAsThumb)
      {
      case CQualifyPicNameAsThumb::FOLDER:
      {
        CLog::Log(LOGDEBUG,"BoxeeUtils::GetPicturePathQulifyAsThumb - In [i=%d=FOLDER] found a picturesQualifyAsThumbnail [%s] (foldert)",picNameAsThumb,picturePathQulifyAsThumb.c_str());
      }
      break;
      case CQualifyPicNameAsThumb::PREVIEW:
      {
        CLog::Log(LOGDEBUG,"BoxeeUtils::GetPicturePathQulifyAsThumb - In [i=%d=PREVIEW] found a picturesQualifyAsThumbnail [%s] (foldert)",picNameAsThumb,picturePathQulifyAsThumb.c_str());
      }
      break;
      case CQualifyPicNameAsThumb::FOLDER_NAME:
      {
        CLog::Log(LOGDEBUG,"BoxeeUtils::GetPicturePathQulifyAsThumb - In [i=%d=FOLDER_NAME] found a picturesQualifyAsThumbnail [%s] (foldert)",picNameAsThumb,picturePathQulifyAsThumb.c_str());
      }
      break;
      case CQualifyPicNameAsThumb::COVER:
      {
        CLog::Log(LOGDEBUG,"BoxeeUtils::GetPicturePathQulifyAsThumb - In [i=%d=COVER] found a picturesQualifyAsThumbnail [%s] (foldert)",picNameAsThumb,picturePathQulifyAsThumb.c_str());
      }
      break;
      case CQualifyPicNameAsThumb::FRONT:
      {
        CLog::Log(LOGDEBUG,"BoxeeUtils::GetPicturePathQulifyAsThumb - In [i=%d=FRONT] found a picturesQualifyAsThumbnail [%s] (foldert)",picNameAsThumb,picturePathQulifyAsThumb.c_str());
      }
      break;
      default:
      {
        CLog::Log(LOGERROR,"BoxeeUtils::GetPicturePathQulifyAsThumb - Enter switch with wrong value [picNameAsThumb=%d]. [picturePathQulifyAsThumb=%s] (foldert)",picNameAsThumb,picturePathQulifyAsThumb.c_str());
      }
      }

      break;
    }
  }

  CLog::Log(LOGDEBUG,"BoxeeUtils::GetPicturePathQulifyAsThumb - Exit function and return [retVal=%s] (foldert)",picturePathQulifyAsThumb.c_str());

  return picturePathQulifyAsThumb;
}

bool BoxeeUtils::SafeDownload(const CStdString &url, const CStdString &target, const CStdString &hash)
{
  CStdString targetPath = target;

  if(CUtil::IsSpecial(targetPath))
  {
    targetPath = _P(targetPath);
  }

  BOXEE::BXCurl curl;
  bool bOk = false;
  CStdString tmpFile = _P("special://temp/");
  tmpFile += CUtil::GetFileName(targetPath);
  if (curl.HttpDownloadFile(url.c_str(), tmpFile.c_str(), ""))
  {
    if (hash == CUtil::MD5File(tmpFile))
    {
      bOk = ::CopyFile(tmpFile.c_str(), targetPath.c_str(), false);
      ::DeleteFile(tmpFile);
    }
  }

  return bOk;
}

bool BoxeeUtils::RemoveMatchingPatternFromString(CStdString& str, const CStdString regExPattern)
{
  CRegExp reg;

  if (!reg.RegComp(regExPattern))
  {
    CLog::Log(LOGERROR,"BoxeeUtils::RemoveMatchingPatternFromString - FAILED to compile [regExPattern=%s] with RegExp",regExPattern.c_str());
    return false;
  }

  int findStart = -1;
  while ((findStart = reg.RegFind(str.c_str())) >= 0)
  {
    // we've found a match
    CStdString matchStr = reg.GetMatch(0);
    str.Delete(findStart,matchStr.length());
  }

  return true;
}

const char* BoxeeUtils::GetPlatformStr()
{
#if defined(BOXEE_DEVICE)
  return BOXEE_PLATFORMS[4];
#elif defined(_LINUX) && !defined(__APPLE__)
  return BOXEE_PLATFORMS[2];
#elif defined(__APPLE__)
  if(CSysInfo::IsAppleTV())
  {
    return BOXEE_PLATFORMS[1];
  }
  else
  {
    return BOXEE_PLATFORMS[0];
  }
#elif defined(_WIN32)
  return BOXEE_PLATFORMS[3];
#endif
}

bool BoxeeUtils::LogReportToServerAction(const CFileItem& pItem, const BXObject& obj, CReportToServerActionType::ReportToServerActionTypeEnums reportToServerActionType)
{
  CStdString reportToServerActionTypeStr = "";

  switch(reportToServerActionType)
  {
  case CReportToServerActionType::PLAY:
    reportToServerActionTypeStr = "PLAY";
    break;
  case CReportToServerActionType::RATE:
    reportToServerActionTypeStr = "RATE";
    break;
  case CReportToServerActionType::RECOMMEND:
    reportToServerActionTypeStr = "RECOMMEND";
    break;
  case CReportToServerActionType::SHARE:
    reportToServerActionTypeStr = "SHARE";
    break;
  case CReportToServerActionType::QUEUE:
    reportToServerActionTypeStr = "QUEUE";
    break;
  case CReportToServerActionType::DEQUEUE:
    reportToServerActionTypeStr = "DEQUEUE";
    break;
  case CReportToServerActionType::SUBSCRIBE:
    reportToServerActionTypeStr = "SUBSCRIBE";
    break;
  case CReportToServerActionType::UNSUBSCRIBE:
    reportToServerActionTypeStr = "UNSUBSCRIBE";
    break;
  case CReportToServerActionType::INSTALL:
    reportToServerActionTypeStr = "INSTALL";
    break;
  case CReportToServerActionType::REMOVE:
    reportToServerActionTypeStr = "REMOVE";
    break;
  case CReportToServerActionType::LAUNCH:
    reportToServerActionTypeStr = "LAUNCH";
    break;
  default:
    CLog::Log(LOGERROR,"BoxeeUtils::LogReportToServerAction - Wrong reportToServerActionType enum [reportToServerActionType=%d], so not going to report action to server. Item [label=%s][IsInternetStream=%d]. Obj [MSG_KEY_NAME=%s][MSG_KEY_TITLE=%s][TYPE=%s][MSG_KEY_URL=%s][MSG_KEY_THUMB=%s][dont-report=%d=%s] (rts)",reportToServerActionType,(pItem.GetLabel()).c_str(),pItem.IsInternetStream(),(obj.GetName()).c_str(),(obj.GetValue(MSG_KEY_TITLE)).c_str(),(obj.GetType()).c_str(),(obj.GetValue(MSG_KEY_URL)).c_str(),(obj.GetValue(MSG_KEY_THUMB)).c_str(),pItem.GetPropertyBOOL("dont-report"),(pItem.GetProperty("dont-report")).c_str());
    return false;
    break;
  }

  if((obj.GetName()).empty())
  {
    CLog::Log(LOGERROR,"[ActionType=%s] - Item [label=%s][IsInternetStream=%d] is going to be reported to the server although its MSG_KEY_NAME is EMPTY. [dont-report=%d=%s]. Obj parameters are [type=%s][name=%s][url=%s][content_type=%s][stream_type=%s][thumb=%s][show_name=%s][episode=%s][season=%s][provider=%s][app_id=%s][adult=%s][country=%s][country_allow=%s][genre=%s][release_date=%s][boxee_id=%s][show_id=%s][show_name=%s][imdb_id=%s][release_year=%s][run_time=%s][hasDesc=%d][activate_from=%s][func_ref=%s] (rts)",reportToServerActionTypeStr.c_str(),(pItem.GetLabel()).c_str(),pItem.IsInternetStream(),pItem.GetPropertyBOOL("dont-report"),(pItem.GetProperty("dont-report")).c_str(),obj.GetType().c_str(),obj.GetName().c_str(),obj.GetValue(MSG_KEY_URL).c_str(),obj.GetValue(MSG_KEY_CONTENT_TYPE).c_str(),obj.GetValue(MSG_KEY_STREAM_TYPE).c_str(),obj.GetValue(MSG_KEY_THUMB).c_str(),obj.GetValue(MSG_KEY_SHOW_NAME).c_str(),obj.GetValue(MSG_KEY_EPISODE).c_str(),obj.GetValue(MSG_KEY_SEASON).c_str(),obj.GetValue(MSG_KEY_PROVIDER).c_str(),obj.GetValue(MSG_KEY_APP_ID).c_str(),obj.GetValue(MSG_KEY_ADULT).c_str(),obj.GetValue(MSG_KEY_COUNTRY).c_str(),obj.GetValue(MSG_KEY_COUNTRY_ALLOW).c_str(),obj.GetValue(MSG_KEY_GENRE).c_str(),obj.GetValue(MSG_KEY_RELEASE_DATE).c_str(),obj.GetValue(MSG_KEY_BOXEE_ID).c_str(),obj.GetValue(MSG_KEY_SHOW_ID).c_str(),obj.GetValue(MSG_KEY_SHOW_NAME).c_str(),obj.GetValue(MSG_KEY_IMDB_NUM).c_str(),obj.GetValue(MSG_KEY_RELEASE_YEAR).c_str(),obj.GetValue(MSG_KEY_RUN_TIME).c_str(),obj.GetValue(MSG_KEY_DESCRIPTION).empty()?false:true,obj.GetValue(MSG_KEY_ACTIVATED_FROM).c_str(),obj.GetValue("func_ref").c_str());
  }
  else
  {
    CLog::Log(LOGDEBUG,"[ActionType=%s] - Item [label=%s][IsInternetStream=%d] is going to be reported to the server. [dont-report=%d=%s]. Obj parameters are [type=%s][name=%s][url=%s][content_type=%s][stream_type=%s][thumb=%s][show_name=%s][episode=%s][season=%s][provider=%s][app_id=%s][adult=%s][country=%s][country_allow=%s][genre=%s][release_date=%s][boxee_id=%s][show_id=%s][show_name=%s][imdb_id=%s][release_year=%s][run_time=%s][hasDesc=%d][activate_from=%s][func_ref=%s] (rts)",reportToServerActionTypeStr.c_str(),(pItem.GetLabel()).c_str(),pItem.IsInternetStream(),pItem.GetPropertyBOOL("dont-report"),(pItem.GetProperty("dont-report")).c_str(),obj.GetType().c_str(),obj.GetName().c_str(),obj.GetValue(MSG_KEY_URL).c_str(),obj.GetValue(MSG_KEY_CONTENT_TYPE).c_str(),obj.GetValue(MSG_KEY_STREAM_TYPE).c_str(),obj.GetValue(MSG_KEY_THUMB).c_str(),obj.GetValue(MSG_KEY_SHOW_NAME).c_str(),obj.GetValue(MSG_KEY_EPISODE).c_str(),obj.GetValue(MSG_KEY_SEASON).c_str(),obj.GetValue(MSG_KEY_PROVIDER).c_str(),obj.GetValue(MSG_KEY_APP_ID).c_str(),obj.GetValue(MSG_KEY_ADULT).c_str(),obj.GetValue(MSG_KEY_COUNTRY).c_str(),obj.GetValue(MSG_KEY_COUNTRY_ALLOW).c_str(),obj.GetValue(MSG_KEY_GENRE).c_str(),obj.GetValue(MSG_KEY_RELEASE_DATE).c_str(),obj.GetValue(MSG_KEY_BOXEE_ID).c_str(),obj.GetValue(MSG_KEY_SHOW_ID).c_str(),obj.GetValue(MSG_KEY_SHOW_NAME).c_str(),obj.GetValue(MSG_KEY_IMDB_NUM).c_str(),obj.GetValue(MSG_KEY_RELEASE_YEAR).c_str(),obj.GetValue(MSG_KEY_RUN_TIME).c_str(),obj.GetValue(MSG_KEY_DESCRIPTION).empty()?false:true,obj.GetValue(MSG_KEY_ACTIVATED_FROM).c_str(),obj.GetValue("func_ref").c_str());
  }

  return true;
}

CStdString BoxeeUtils::BuildInstallRssData(const VECSOURCES& allRssSources)
{
  CStdString data = "<rss>";

  for (int i=0; i<(int)allRssSources.size(); i++)
  {
    data += "<feed type=\"";

    CStdString shortType = BoxeeUtils::GetShortRssType(allRssSources[i].m_type);

    if (shortType.IsEmpty())
    {
      CLog::Log(LOGWARNING,"BoxeeUtils::BuildInstallRssData - [%d/%d] - FAILED to convert type of rss [name=%s][path=%s][type=%s]. Continue to next rss (rtspf)",i+1,(int)allRssSources.size(),allRssSources[i].strName.c_str(),allRssSources[i].strPath.c_str(),allRssSources[i].m_type.c_str());
      continue;
    }

    data += shortType;
    data += "\">";

    data += allRssSources[i].strPath;

    data += "</feed>";

    CLog::Log(LOGDEBUG,"BoxeeUtils::BuildInstallRssData - [%d/%d] - After add rss [name=%s][path=%s][type=%s] (rtspf)",i+1,(int)allRssSources.size(),allRssSources[i].strName.c_str(),allRssSources[i].strPath.c_str(),allRssSources[i].m_type.c_str());
  }

  data += "</rss>";

  return data;
}

CStdString BoxeeUtils::GetShortRssType(CStdString rssType)
{
  rssType.ToLower();

  if (rssType == "video")
  {
    return "v";
  }
  else if (rssType == "music")
  {
    return "a";
  }
  else if (rssType == "pictures")
  {
    return "p";
  }
  else
  {
    return "";
  }
}

bool BoxeeUtils::ResolveItem(const CStdString& strBoxeeId, CFileItemList& items)
{
  // Real production URL
  CStdString strBoxeeServerUrl = BXConfiguration::GetInstance().GetStringParam("Boxee.Resolver.Server","http://res.boxee.tv");
  CStdString strLink = strBoxeeServerUrl;
  strLink += "/title/movie/?id=";
  strLink += strBoxeeId;

  CRssFeed feed;
  feed.Init(strLink,strLink);
  feed.ReadFeed();

  feed.GetItemList(items);

  CLog::Log(LOGDEBUG, "BoxeeUtils::ResolveItem, for boxee id = %s, items size = %d (ri)", strBoxeeId.c_str(), items.Size());

  if (items.Size() == 0)
  {
    return false;
  }

  for (int j = 0; j < items.Size(); j++)
  {
    CFileItemPtr pItem = items[j];

    CStdString itemId = pItem->GetProperty("boxeeId");
    if (itemId == strBoxeeId)
    {
      GetLocalLinks(pItem);
    }
  }

  return true;
}

bool BoxeeUtils::GetLocalVideoMetadata(CFileItem& item)
{
  BXMetadata metadata(MEDIA_ITEM_TYPE_VIDEO);

  if (BOXEE::Boxee::GetInstance().GetMetadataEngine().GetVideoByPath(_P(item.m_strPath), &metadata) == MEDIA_DATABASE_OK)
  {
    BXVideo* pVideo = (BXVideo*)metadata.GetDetail(MEDIA_DETAIL_VIDEO);
    BXSeries* pSeries = (BXSeries*)metadata.GetDetail(MEDIA_DETAIL_SERIES);

    CVideoInfoTag infoTag;
    BoxeeUtils::ConvertBXVideoToVideoInfoTag(pVideo, infoTag);

    infoTag.m_strShowTitle = pSeries->m_strTitle;

    *item.GetVideoInfoTag() = infoTag;

    if (!pVideo->m_strBoxeeId.empty())
    {
      item.SetProperty("boxeeId", pVideo->m_strBoxeeId);
    }

    if (!pVideo->m_strShowId.empty())
    {
      item.SetProperty("showId", pVideo->m_strShowId);
    }

    item.SetProperty("OriginalThumb", pVideo->m_strCover);

    // Set relevant properties
    CBoxeeDatabaseDirectory::FillItemDetails(&item);
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG,"In CGUIDialogBoxeeMediaAction::OnPlay - file not recognized, path = %s", item.m_strPath.c_str());
    item.SetProperty("isFolderItem", true);
    return false;
  }
}

void BoxeeUtils::GetLocalLinks(CFileItemPtr pItem)
{
  CStdString strBoxeeId = pItem->GetProperty("boxeeId");
  // Get all local links to this item from the database
  std::vector<BXPath> vecLinks;
  BOXEE::Boxee::GetInstance().GetMetadataEngine().GetLinks(strBoxeeId, vecLinks);
  for (size_t i = 0; i < vecLinks.size(); i++)
  {
    CStdString strPath = CBoxeeDatabaseDirectory::ConstructVideoPath(vecLinks[i]);

    CLog::Log(LOGDEBUG, "BoxeeUtils::ResolveItem, add local path = %s (ri)", strPath.c_str());
    pItem->AddLink(pItem->GetLabel(), strPath, pItem->GetContentType(true), CLinkBoxeeType::LOCAL, "", "", "", "all", true,"",0);
  }
}

void BoxeeUtils::PlayPicture(const CFileItem& itemPath)
{
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);

  if (!pSlideShow)
    return;

  if (g_application.IsPlayingVideo())
    g_application.StopPlaying();

  pSlideShow->Reset();
  pSlideShow->Add(&itemPath);
  pSlideShow->PauseSlideShow();
  g_windowManager.ActivateWindow(WINDOW_SLIDESHOW);
  return;
}

BXObject BoxeeUtils::FileItemToObject(const CFileItem *pItem)
{
  if (!pItem)
  {
    CLog::Log(LOGERROR,"%s - item is empty", __FUNCTION__);
    return BXObject(false);
  }

  ///////////////////////
  // get the item type //
  ///////////////////////

  CFileItemTypes::FileItemTypesEnums rawTypeEnum = GetRawFileItemType(pItem);

  CLog::Log(LOGDEBUG,"BoxeeUtils::FileItemToObject - For item [label=%s][path=%s][HasVideoInfoTag=%d][HasMusicInfoTag=%d][IsVideo=%d][IsTvShow=%d][IsAudio=%d] got [RawType=%s=%d] (rts)(fito)",pItem->GetLabel().c_str(),pItem->m_strPath.c_str(),pItem->HasVideoInfoTag(),pItem->HasMusicInfoTag(),pItem->GetPropertyBOOL("isVideo"),pItem->GetPropertyBOOL("isTvShow"),pItem->GetPropertyBOOL("isAudio"),BoxeeUtils::GetFileItemTypesEnumsAsString(rawTypeEnum).c_str(),rawTypeEnum);

  BXObject obj(false);

  switch (rawTypeEnum)
  {
  case CFileItemTypes::LOCAL_RESOLVE_MOVIE:
  {
    HandleLocalResolvedMovieItem(pItem, obj);
  }
  break;
  case CFileItemTypes::LOCAL_RESOLVE_TVSHOW_EPISODE:
  {
    HandleLocalResolvedTvShowEpisodeItem(pItem, obj);
  }
  break;
  case CFileItemTypes::LOCAL_RESOLVE_AUDIO:
  {
    HandleLocalResolvedAudioItem(pItem, obj);
  }
  break;
  case CFileItemTypes::LOCAL_RESOLVE_PICTURE:
  {
    HandleLocalResolvedPictureItem(pItem, obj);
  }
  break;
  case CFileItemTypes::LOCAL_UNKNOWN_VIDEO:
  {
    HandleLocalUnknownVideoItem(pItem, obj);
  }
  break;
  case CFileItemTypes::LOCAL_UNKNOWN_AUDIO:
  {
    HandleLocalUnknownAudioItem(pItem, obj);
  }
  break;
  case CFileItemTypes::LOCAL_UNKNOWN_PICTURE:
  {
    HandleLocalUnknownPictureItem(pItem, obj);
  }
  break;
  case CFileItemTypes::LOCAL_UNKNOWN:
  {
    HandleLocalUnknownItem(pItem, obj);
  }
  break;
  case CFileItemTypes::WEB_RESOLVE_VIDEO:
  {
    HandleWebResolvedVideoItem(pItem, obj);
  }
  break;
  case CFileItemTypes::WEB_RESOLVE_MOVIE:
  {
    HandleWebResolvedMovieItem(pItem, obj);
  }
  break;
  case CFileItemTypes::WEB_RESOLVE_TVSHOW_EPISODE:
  {
    HandleWebResolvedTvShowEpisodeItem(pItem, obj);
  }
  break;
  case CFileItemTypes::WEB_RESOLVE_AUDIO:
  {
    HandleWebResolvedAudioItem(pItem, obj);
  }
  break;
  case CFileItemTypes::WEB_RESOLVE_PICTURE:
  {
    HandleWebResolvedPictureItem(pItem, obj);
  }
  break;
  case CFileItemTypes::WEB_UNKNOWN_VIDEO:
  {
    HandleWebUnknownVideoItem(pItem, obj);
  }
  break;
  case CFileItemTypes::WEB_UNKNOWN_AUDIO:
  {
    HandleWebUnknownAudioItem(pItem, obj);
  }
  break;
  case CFileItemTypes::WEB_UNKNOWN_PICTURE:
  {
    HandleWebUnknownPictureItem(pItem, obj);
  }
  break;
  case CFileItemTypes::WEB_UNKNOWN:
  {
    HandleWebUnknownItem(pItem, obj);
  }
  break;
  case CFileItemTypes::UNKNOWN:
  {
    HandleUnknownItem(pItem, obj);
  }
  break;
  default:
  {
    CLog::Log(LOGDEBUG,"BoxeeUtils::FileItemToObject - Not handling [RawType=%s=%d] for item [label=%s][path=%s][HasVideoInfoTag=%d][HasMusicInfoTag=%d][IsVideo=%d][IsTvShow=%d][IsAudio=%d] (fito)",BoxeeUtils::GetFileItemTypesEnumsAsString(rawTypeEnum).c_str(),rawTypeEnum,pItem->GetLabel().c_str(),pItem->m_strPath.c_str(),pItem->HasVideoInfoTag(),pItem->HasMusicInfoTag(),pItem->GetPropertyBOOL("isVideo"),pItem->GetPropertyBOOL("isTvShow"),pItem->GetPropertyBOOL("isAudio"));
  }
  break;
  }

  obj.SetValue("func_ref","new");

  if (obj.IsValid())
  {
    SetActivatedFromToObj(pItem, obj);
    CLog::Log(LOGDEBUG,"BoxeeUtils::FileItemToObject - For item [label=%s][path=%s] going to return a VALID obj. [name=%s][type=%s][url=%s] (fito)",pItem->GetLabel().c_str(),pItem->m_strPath.c_str(),obj.GetValue(MSG_KEY_NAME).c_str(),obj.GetType().c_str(),obj.GetValue(MSG_KEY_URL).c_str());
  }
  else
  {
    CLog::Log(LOGDEBUG,"BoxeeUtils::FileItemToObject - For item [label=%s][path=%s] going to return a NOT-VALID obj (fito)",pItem->GetLabel().c_str(),pItem->m_strPath.c_str());
  }

  return obj;
}

void BoxeeUtils::SetActivatedFromToObj(const CFileItem* pItem, BXObject& obj)
{
  if (!pItem)
  {
    CLog::Log(LOGERROR,"BoxeeUtils::SetActivatedFromToObj - Enter function with NULL FileItem (af)(bapps)");
    return;
  }

  CLog::Log(LOGDEBUG,"BoxeeUtils::SetActivatedFromToObj - Enter function with FileItem [parentPath=%s]. [label=%s][path=%s] (af)",pItem->GetProperty("parentPath").c_str(),pItem->GetLabel().c_str(),pItem->m_strPath.c_str());

  CStdString parentPath = pItem->GetProperty("parentPath");
  if (!parentPath.IsEmpty())
  {
    CStdString activateFrom = "";

    CURL url(parentPath);
    CStdString protocol = url.GetProtocol();

    if (protocol.CompareNoCase("feed") == 0)
    {
      CStdString hostName = url.GetHostName();

      if (hostName.CompareNoCase(MSG_ACTION_TYPE_QUEUE) == 0)
      {
        activateFrom = MSG_ACTION_TYPE_QUEUE;
      }
      else if (hostName.CompareNoCase(MSG_ACTION_TYPE_FEATURED) == 0)
      {
        activateFrom = MSG_ACTION_TYPE_FEATURED;
      }
      else if (hostName.CompareNoCase(MSG_ACTION_TYPE_SHARE) == 0 || hostName.CompareNoCase(MSG_ACTION_TYPE_RECOMMEND) == 0)
      {
        activateFrom = MSG_KEY_FEED;
      }
    }
    else if (protocol.CompareNoCase("apps") == 0 || protocol.CompareNoCase("appbox") == 0)
    {
      activateFrom = "apps";
    }
    else if (protocol.CompareNoCase("boxee") == 0)
    {
      CStdString hostName = url.GetHostName();

      if (hostName.CompareNoCase("movies") == 0)
      {
        activateFrom = "movies";
      }
      else if (hostName.CompareNoCase("tvshows") == 0)
      {
        activateFrom = "tvshows";
      }
    }

    if (!activateFrom.IsEmpty())
    {
      obj.SetValue(MSG_KEY_ACTIVATED_FROM, activateFrom);
    }
  }
}

void BoxeeUtils::HandleLocalResolvedMovieItem(const CFileItem* pItem, BOXEE::BXObject& obj)
{
  if (!pItem)
  {
    CLog::Log(LOGERROR,"BoxeeUtils::HandleLocalResolvedMovieItem - Enter function with a NULL item (new)");
    return;
  }

  obj.SetType(MSG_OBJ_TYPE_MOVIE);

  const CVideoInfoTag* videoInfoTag = pItem->GetVideoInfoTag();
  if (videoInfoTag)
  {
    if (!videoInfoTag->m_strTitle.IsEmpty())
    {
      obj.SetValue(MSG_KEY_NAME, videoInfoTag->m_strTitle);
    }
  }
  else
  {
    CLog::Log(LOGERROR,"BoxeeUtils::HandleLocalResolvedMovieItem - For item [label=%s][path=%s] there ISN'T CVideoInfoTag (new)",pItem->GetLabel().c_str(),pItem->m_strPath.c_str());
  }

  if (obj.GetValue(MSG_KEY_NAME).empty())
  {
    // in case MSG_KEY_NAME wasn't set -> set it with the item label
    obj.SetValue(MSG_KEY_NAME, pItem->GetLabel());
  }

  SetObjParametersFromItem(pItem, obj, false/*ContentType*/, false/*Provider*/, false/*AppId*/, false/*StreamType*/, true/*Adult*/, false/*Countries*/, true/*Thumb*/, true/*Genre*/, true/*ReleaseDate*/, true/*Description*/, true/*BoxeeId*/, false/*ShowId*/, false/*ShowName*/, true/*ImdbId*/, true/*Year*/, true/*Runtime*/);

  obj.SetValid(true);
}

void BoxeeUtils::HandleLocalResolvedTvShowEpisodeItem(const CFileItem *pItem, BOXEE::BXObject& obj)
{
  if (!pItem)
  {
    CLog::Log(LOGERROR,"BoxeeUtils::HandleLocalResolvedTvShowEpisodeItem - Enter function with a NULL item (new)");
    return;
  }

  obj.SetType(MSG_OBJ_TYPE_TV);

  const CVideoInfoTag* videoInfoTag = pItem->GetVideoInfoTag();
  if (videoInfoTag)
  {
    char szEpisode[10];
    char szSeason[10];
    snprintf(szEpisode,10,"%d",videoInfoTag->m_iEpisode);
    snprintf(szSeason,10,"%d",videoInfoTag->m_iSeason);
    obj.SetValue(MSG_KEY_EPISODE, szEpisode);
    obj.SetValue(MSG_KEY_SEASON, szSeason);

    if (!videoInfoTag->m_strTitle.IsEmpty())
    {
      obj.SetValue(MSG_KEY_NAME, videoInfoTag->m_strTitle);
    }
    else if (!videoInfoTag->m_strShowTitle.IsEmpty())
    {
      obj.SetValue(MSG_KEY_NAME, videoInfoTag->m_strShowTitle);
    }
  }
  else
  {
    CLog::Log(LOGERROR,"BoxeeUtils::HandleLocalResolvedTvShowEpisodeItem - For item [label=%s][path=%s] there ISN'T CVideoInfoTag (new)",pItem->GetLabel().c_str(),pItem->m_strPath.c_str());
  }

  SetObjParametersFromItem(pItem, obj, false/*ContentType*/, false/*Provider*/, false/*AppId*/, false/*StreamType*/, true/*Adult*/, false/*Countries*/, true/*Thumb*/, true/*Genre*/, true/*ReleaseDate*/, true/*Description*/, true/*BoxeeId*/, true/*ShowId*/, true/*ShowName*/, true/*ImdbId*/, true/*Year*/, true/*Runtime*/);

  if (obj.GetValue(MSG_KEY_NAME).empty())
  {
    // in case MSG_KEY_NAME wasn't set -> set it with "MSG_KEY_SHOW_NAME" or the item label

    CStdString showName = obj.GetValue(MSG_KEY_SHOW_NAME);
    if (!showName.IsEmpty())
    {
      obj.SetValue(MSG_KEY_NAME, showName);
    }
    else
    {
      obj.SetValue(MSG_KEY_NAME, pItem->GetLabel());
    }
  }

  obj.SetValid(true);
}

void BoxeeUtils::HandleLocalResolvedAudioItem(const CFileItem* pItem, BOXEE::BXObject& obj)
{
  if (!pItem)
  {
    CLog::Log(LOGERROR,"BoxeeUtils::HandleLocalResolvedAudioItem - Enter function with a NULL item (new)");
    return;
  }

  obj.SetType(MSG_OBJ_TYPE_AUDIO_ALBUM);

  const CMusicInfoTag* musicInfoTag = pItem->GetMusicInfoTag();

  if (!musicInfoTag)
  {
    CLog::Log(LOGERROR,"BoxeeUtils::HandleLocalResolvedAudioItem - FAILED to get MusicInfoTag from item (new)");
    return;
  }

  obj.SetValue(MSG_KEY_NAME, musicInfoTag->GetAlbum());
  obj.SetValue(MSG_KEY_ALBUM, musicInfoTag->GetAlbum());
  obj.SetValue(MSG_KEY_GENRE, musicInfoTag->GetGenre());
  obj.SetValue(MSG_KEY_ARTIST, musicInfoTag->GetArtist());
  obj.SetValue(MSG_KEY_ALBUM_ARTIST, musicInfoTag->GetAlbumArtist());

  CStdString originalThumb = pItem->GetProperty("OriginalThumb");
  if (!originalThumb.IsEmpty())
  {
    obj.SetValue(MSG_KEY_THUMB, originalThumb);
  }

  obj.SetValid(true);
}

void BoxeeUtils::HandleLocalResolvedPictureItem(const CFileItem* pItem, BOXEE::BXObject& obj)
{
  CLog::Log(LOGDEBUG,"BoxeeUtils::HandleLocalResolvedPictureItem - Not handling (new)");
  return;
}

void BoxeeUtils::HandleLocalUnknownVideoItem(const CFileItem *pItem, BOXEE::BXObject& obj)
{
  if (!pItem)
  {
    CLog::Log(LOGERROR,"BoxeeUtils::HandleLocalUnknownVideoItem - Enter function with a NULL item (new)");
    return;
  }

  obj.SetType(MSG_OBJ_TYPE_UNKNOWN_VIDEO);

  obj.SetValid(true);
}

void BoxeeUtils::HandleLocalUnknownAudioItem(const CFileItem *pItem, BOXEE::BXObject& obj)
{
  if (!pItem)
  {
    CLog::Log(LOGERROR,"BoxeeUtils::HandleLocalUnknownAudioItem - Enter function with a NULL item (new)");
    return;
  }

  obj.SetType(MSG_OBJ_TYPE_UNKNOWN_AUDIO);

  obj.SetValid(true);
}

void BoxeeUtils::HandleLocalUnknownPictureItem(const CFileItem *pItem, BOXEE::BXObject& obj)
{
  CLog::Log(LOGDEBUG,"BoxeeUtils::HandleLocalUnknownPictureItem - Not handling (new)");
  return;
}

void BoxeeUtils::HandleLocalUnknownItem(const CFileItem *pItem, BOXEE::BXObject& obj)
{
  if (!pItem)
  {
    CLog::Log(LOGERROR,"BoxeeUtils::HandleLocalUnknownItem - Enter function with a NULL item (new)");
    return;
  }

  obj.SetType(MSG_OBJ_TYPE_UNKNOWN);

  obj.SetValid(true);
}

void BoxeeUtils::HandleWebResolvedVideoItem(const CFileItem *pItem, BOXEE::BXObject& obj)
{
  if (!pItem)
  {
    CLog::Log(LOGERROR,"BoxeeUtils::HandleWebResolvedVideoItem - Enter function with a NULL item (new)");
    return;
  }

  obj.SetType(MSG_OBJ_TYPE_VIDEO_STREAM);
  obj.SetValue(MSG_KEY_URL, BoxeeUtils::GetUrlFromItemForRepostToServer(*pItem));

  const CVideoInfoTag* videoInfoTag = pItem->GetVideoInfoTag();
  if (videoInfoTag)
  {
    if (!videoInfoTag->m_strTitle.IsEmpty())
    {
      obj.SetValue(MSG_KEY_NAME, videoInfoTag->m_strTitle);
    }
  }
  else
  {
    CLog::Log(LOGERROR,"BoxeeUtils::HandleWebResolvedVideoItem - For item [label=%s][path=%s] there ISN'T CVideoInfoTag (new)",pItem->GetLabel().c_str(),pItem->m_strPath.c_str());
  }

  if (obj.GetValue(MSG_KEY_NAME).empty())
  {
    // in case MSG_KEY_NAME wasn't set -> set it with "title" or the item label

    if (!pItem->GetProperty("title").IsEmpty())
    {
      obj.SetValue(MSG_KEY_NAME, pItem->GetProperty("title"));
    }
    else if (!pItem->GetLabel().IsEmpty())
    {
      obj.SetValue(MSG_KEY_NAME, pItem->GetLabel());
    }
    else
    {
      CLog::Log(LOGERROR,"BoxeeUtils::HandleWebResolvedVideoItem - FAILED to set obj name for item [label=%s][path=%s] (new)",pItem->GetLabel().c_str(),pItem->m_strPath.c_str());
      return;
    }
  }

  SetObjParametersFromItem(pItem, obj, true/*ContentType*/, true/*Provider*/, true/*AppId*/, true/*StreamType*/, true/*Adult*/, true/*Countries*/, true/*Thumb*/, true/*Genre*/, true/*ReleaseDate*/, true/*Description*/, true/*BoxeeId*/, false/*ShowId*/, false/*ShowName*/, true/*ImdbId*/, true/*Year*/, true/*Runtime*/);

  obj.SetValid(true);
}

void BoxeeUtils::HandleWebResolvedMovieItem(const CFileItem *pItem, BOXEE::BXObject& obj)
{
  if (!pItem)
  {
    CLog::Log(LOGERROR,"BoxeeUtils::HandleWebResolvedMovieItem - Enter function with a NULL item (new)");
    return;
  }

  obj.SetType(MSG_OBJ_TYPE_STREAM_MOVIE);
  obj.SetValue(MSG_KEY_URL, BoxeeUtils::GetUrlFromItemForRepostToServer(*pItem));

  const CVideoInfoTag* videoInfoTag = pItem->GetVideoInfoTag();
  if (videoInfoTag)
  {
    if (!videoInfoTag->m_strTitle.IsEmpty())
    {
      obj.SetValue(MSG_KEY_NAME, videoInfoTag->m_strTitle);
    }
  }
  else
  {
    CLog::Log(LOGERROR,"BoxeeUtils::HandleWebResolvedMovieItem - For item [label=%s][path=%s] there is CVideoInfoTag (new)",pItem->GetLabel().c_str(),pItem->m_strPath.c_str());
  }

  if (obj.GetValue(MSG_KEY_NAME).empty())
  {
    // in case MSG_KEY_NAME wasn't set -> set it with "title" or the item label

    if (!pItem->GetProperty("title").IsEmpty())
    {
      obj.SetValue(MSG_KEY_NAME, pItem->GetProperty("title"));
    }
    else if (!pItem->GetLabel().IsEmpty())
    {
      obj.SetValue(MSG_KEY_NAME, pItem->GetLabel());
    }
    else
    {
      CLog::Log(LOGERROR,"BoxeeUtils::HandleWebResolvedMovieItem - FAILED to set obj name for item [label=%s][path=%s] (new)",pItem->GetLabel().c_str(),pItem->m_strPath.c_str());
      return;
    }
  }

  SetObjParametersFromItem(pItem, obj, true/*ContentType*/, true/*Provider*/, true/*AppId*/, true/*StreamType*/, true/*Adult*/, true/*Countries*/, true/*Thumb*/, true/*Genre*/, true/*ReleaseDate*/, true/*Description*/, true/*BoxeeId*/, false/*ShowId*/, false/*ShowName*/, true/*ImdbId*/, true/*Year*/, true/*Runtime*/);

  obj.SetValid(true);
}

void BoxeeUtils::HandleWebResolvedTvShowEpisodeItem(const CFileItem *pItem, BOXEE::BXObject& obj)
{
  if (!pItem)
  {
    CLog::Log(LOGERROR,"BoxeeUtils::HandleWebResolvedTvShowEpisodeItem - Enter function with a NULL item (new)");
    return;
  }

  obj.SetType(MSG_OBJ_TYPE_STREAM_TV_SHOW);
  obj.SetValue(MSG_KEY_URL, BoxeeUtils::GetUrlFromItemForRepostToServer(*pItem));

  const CVideoInfoTag* videoInfoTag = pItem->GetVideoInfoTag();
  if (videoInfoTag)
  {
    char szEpisode[10];
    char szSeason[10];
    snprintf(szEpisode,10,"%d",videoInfoTag->m_iEpisode);
    snprintf(szSeason,10,"%d",videoInfoTag->m_iSeason);
    obj.SetValue(MSG_KEY_EPISODE, szEpisode);
    obj.SetValue(MSG_KEY_SEASON, szSeason);

    if (!videoInfoTag->m_strShowTitle.IsEmpty())
    {
      obj.SetValue(MSG_KEY_NAME, videoInfoTag->m_strShowTitle);
    }
  }
  else
  {
    CLog::Log(LOGERROR,"BoxeeUtils::HandleWebResolvedTvShowEpisodeItem - For item [label=%s][path=%s] there is CVideoInfoTag (new)",pItem->GetLabel().c_str(),pItem->m_strPath.c_str());
  }

  SetObjParametersFromItem(pItem, obj, true/*ContentType*/, true/*Provider*/, true/*AppId*/, true/*StreamType*/, true/*Adult*/, true/*Countries*/, true/*Thumb*/, true/*Genre*/, true/*ReleaseDate*/, true/*Description*/, true/*BoxeeId*/, true/*ShowId*/, true/*ShowName*/, true/*ImdbId*/, true/*Year*/, true/*Runtime*/);

  if (obj.GetValue(MSG_KEY_NAME).empty())
  {
    // in case MSG_KEY_NAME wasn't set -> set it with "showname" or the item label

    CStdString showName = obj.GetValue(MSG_KEY_SHOW_NAME);
    if (!showName.IsEmpty())
    {
      obj.SetValue(MSG_KEY_NAME, showName);
    }
    else if (!pItem->GetLabel().IsEmpty())
    {
      obj.SetValue(MSG_KEY_NAME, pItem->GetLabel());
    }
    else
    {
      CLog::Log(LOGERROR,"BoxeeUtils::HandleWebResolvedTvShowEpisodeItem - FAILED to set obj name for item [label=%s][path=%s] (new)",pItem->GetLabel().c_str(),pItem->m_strPath.c_str());
      return;
    }
  }

  obj.SetValid(true);
}

void BoxeeUtils::HandleWebResolvedAudioItem(const CFileItem *pItem, BOXEE::BXObject& obj)
{
  CLog::Log(LOGDEBUG,"BoxeeUtils::HandleWebResolvedAudioItem -  Going to call HandleWebResolvedAudioItem() (new)");

  return HandleWebUnknownAudioItem(pItem, obj);
}

void BoxeeUtils::HandleWebResolvedPictureItem(const CFileItem *pItem, BOXEE::BXObject& obj)
{
  CLog::Log(LOGDEBUG,"BoxeeUtils::HandleWebResolvedPictureItem - Going to call WebUnknownPicture() (new)");

  return HandleWebUnknownPictureItem(pItem, obj);
}

void BoxeeUtils::HandleWebUnknownVideoItem(const CFileItem *pItem, BOXEE::BXObject& obj)
{
  if (!pItem)
  {
    CLog::Log(LOGERROR,"BoxeeUtils::HandleWebUnknownVideoItem - Enter function with a NULL item (new)");
    return;
  }

  obj.SetType(MSG_OBJ_TYPE_VIDEO_STREAM);
  obj.SetValue(MSG_KEY_URL, BoxeeUtils::GetUrlFromItemForRepostToServer(*pItem));
  obj.SetValue(MSG_KEY_NAME, pItem->GetLabel());

  SetObjParametersFromItem(pItem, obj, true/*ContentType*/, true/*Provider*/, true/*AppId*/, true/*StreamType*/, true/*Adult*/, true/*Countries*/, true/*Thumb*/, true/*Genre*/, true/*ReleaseDate*/, true/*Description*/, true/*BoxeeId*/, true/*ShowId*/, true/*ShowName*/, true/*ImdbId*/, true/*Year*/, true/*Runtime*/);

  obj.SetValid(true);
}

void BoxeeUtils::HandleWebUnknownAudioItem(const CFileItem *pItem, BOXEE::BXObject& obj)
{
  if (!pItem)
  {
    CLog::Log(LOGERROR,"BoxeeUtils::HandleWebUnknownAudioItem - Enter function with a NULL item (new)");
    return;
  }

  obj.SetType(MSG_OBJ_TYPE_AUDIO_STREAM);
  obj.SetValue(MSG_KEY_URL, BoxeeUtils::GetUrlFromItemForRepostToServer(*pItem));
  obj.SetValue(MSG_KEY_NAME, pItem->GetLabel());

  SetObjParametersFromItem(pItem, obj, true/*ContentType*/, true/*Provider*/, true/*AppId*/, true/*StreamType*/, true/*Adult*/, true/*Countries*/, true/*Thumb*/, true/*Genre*/, true/*ReleaseDate*/, true/*Description*/, true/*BoxeeId*/, true/*ShowId*/, true/*ShowName*/, true/*ImdbId*/, true/*Year*/, true/*Runtime*/);

  obj.SetValid(true);
}

void BoxeeUtils::HandleWebUnknownPictureItem(const CFileItem *pItem, BOXEE::BXObject& obj)
{
  if (!pItem)
  {
    CLog::Log(LOGERROR,"BoxeeUtils::HandleWebUnknownPictureItem - Enter function with a NULL item (new)");
    return;
  }

  obj.SetType(MSG_OBJ_TYPE_PICTURE);
  obj.SetValue(MSG_KEY_URL, BoxeeUtils::GetUrlFromItemForRepostToServer(*pItem));

  CStdString strTitle;
  if (pItem->HasProperty("title") && !(pItem->GetProperty("title").IsEmpty()))
  {
    strTitle = pItem->GetProperty("title");
  }
  else if (!pItem->m_strTitle.IsEmpty())
  {
    strTitle = pItem->m_strTitle;
  }
  else
  {
    strTitle = pItem->GetLabel();
  }

  if (!strTitle.IsEmpty())
  {
    obj.SetName(strTitle);
  }
  else
  {
    CLog::Log(LOGERROR,"BoxeeUtils::HandleWebUnknownPictureItem - FAILED to set obj name. [strTitle=%s] (new)",strTitle.c_str());
    return;
  }

  SetObjParametersFromItem(pItem, obj, false/*ContentType*/, true/*Provider*/, true/*AppId*/, false/*StreamType*/, true/*Adult*/, false/*Countries*/, false/*Thumb*/, false/*Genre*/, false/*ReleaseDate*/, true/*Description*/, false/*BoxeeId*/, false/*ShowId*/, false/*ShowName*/, false/*ImdbId*/, false/*Year*/, false/*Runtime*/);

  obj.SetValid(true);
}

void BoxeeUtils::HandleWebUnknownItem(const CFileItem *pItem, BOXEE::BXObject& obj)
{
  if (!pItem)
  {
    CLog::Log(LOGERROR,"BoxeeUtils::HandleWebUnknownItem - Enter function with a NULL item (new)");
    return;
  }

  obj.SetType(MSG_OBJ_TYPE_STREAM);
  obj.SetValue(MSG_KEY_URL, BoxeeUtils::GetUrlFromItemForRepostToServer(*pItem));
  obj.SetValue(MSG_KEY_NAME, pItem->GetLabel());

  SetObjParametersFromItem(pItem, obj, true/*ContentType*/, true/*Provider*/, true/*AppId*/, true/*StreamType*/, true/*Adult*/, true/*Countries*/, true/*Thumb*/, true/*Genre*/, true/*ReleaseDate*/, true/*Description*/, true/*BoxeeId*/, true/*ShowId*/, true/*ShowName*/, true/*ImdbId*/, true/*Year*/, true/*Runtime*/);

  obj.SetValid(true);
}

void BoxeeUtils::HandleUnknownItem(const CFileItem *pItem, BOXEE::BXObject& obj)
{
  if (!pItem)
  {
    CLog::Log(LOGERROR,"BoxeeUtils::HandleUnknownItem - Enter function with a NULL item (new)");
    return;
  }

  obj.SetType(MSG_OBJ_TYPE_UNKNOWN);
}

void BoxeeUtils::SetObjParametersFromItem(const CFileItem* pItem, BOXEE::BXObject& obj,bool setContentType, bool setProvider, bool setAppId, bool setStreamType, bool setAdult, bool setCountries, bool setThumb, bool setGenre, bool setReleaseDate, bool setDescription, bool setBoxeeId, bool setShowId, bool setShowName, bool setImdbId, bool setYear, bool setRuntime)
{
  if (!pItem)
  {
    CLog::Log(LOGERROR,"BoxeeUtils::SetObjParametersFromItem - Enter function with a NULL item (new)");
    return;
  }

  //pItem->Dump();

  const CVideoInfoTag* videoInfoTag = pItem->GetVideoInfoTag();

  if (setContentType)
  {
    /////////////////
    // ContentType //
    /////////////////

    CStdString strContentType = pItem->GetContentType();
    if (!strContentType.IsEmpty())
    {
      obj.SetValue(MSG_KEY_CONTENT_TYPE, strContentType);
    }
  }

  if (setProvider)
  {
    //////////////
    // Provider //
    //////////////

    CStdString provider = "";
    if (!pItem->GetProperty("link-provider").IsEmpty())
    {
      provider = pItem->GetProperty("link-provider");
    }
    else if(!pItem->GetProperty("provider_source").IsEmpty())
    {
      provider = pItem->GetProperty("provider_source");
    }
    else if(!pItem->GetProperty("provider").IsEmpty())
    {
      provider = pItem->GetProperty("provider");
    }

    if (!provider.IsEmpty())
    {
      obj.SetValue(MSG_KEY_PROVIDER, provider);
    }
  }

  if (setAppId)
  {
    ////////////
    // app-id //
    ////////////

    if (!pItem->GetProperty("appid").IsEmpty())
    {
      obj.SetValue(MSG_KEY_APP_ID, pItem->GetProperty("appid"));
    }
  }

  if (setStreamType)
  {
    /////////////////
    // stream-type //
    /////////////////

    if (!pItem->GetProperty("link-boxeetype").IsEmpty())
    {
      obj.SetValue(MSG_KEY_STREAM_TYPE, pItem->GetProperty("link-boxeetype"));
    }
  }

  if (setAdult)
  {
    if(pItem->IsAdult())
    {
      obj.SetValue(MSG_KEY_ADULT, "true");
    }
  }

  if (setCountries)
  {
    CStdString countries = "all";
    bool countriesAllow = true;
    pItem->GetCountryRestriction(countries, countriesAllow);
    if (countries != "all" || !countriesAllow)
    {
      obj.SetValue(MSG_KEY_COUNTRY, countries);
      obj.SetValue(MSG_KEY_COUNTRY_ALLOW, countriesAllow ? "true" : "false");
    }
  }

  if (setThumb)
  {
    bool thumbWasSet = false;
    CStdString strThumb;

    if (!pItem->GetProperty("OriginalThumb").IsEmpty())
    {
      strThumb = pItem->GetProperty("OriginalThumb");

      if (!BoxeeUtils::IsPathLocal(strThumb))
      {
        obj.SetValue(MSG_KEY_THUMB, strThumb);
        thumbWasSet = true;
      }
    }

    if (!thumbWasSet && !pItem->GetProperty("parentThumb").IsEmpty())
    {
      strThumb = pItem->GetProperty("parentThumb");

      if (!BoxeeUtils::IsPathLocal(strThumb))
      {
        obj.SetValue(MSG_KEY_THUMB, strThumb);
        thumbWasSet = true;
      }
    }

    if (!thumbWasSet)
    {
      strThumb = pItem->GetThumbnailImage();

      if (!BoxeeUtils::IsPathLocal(strThumb))
      {
        obj.SetValue(MSG_KEY_THUMB, strThumb);
        thumbWasSet = true;
      }
    }
  }

  if (setGenre)
  {
    if (videoInfoTag)
    {
      if (!videoInfoTag->m_strGenre.empty())
      {
        CStdString strGenre = videoInfoTag->m_strGenre;
        strGenre.ToLower();

        // Filter Genres
        static std::vector<CStdString> m_badWords;
        m_badWords.push_back("porn");
        m_badWords.push_back("xxx");
        m_badWords.push_back("sex");
        m_badWords.push_back("adult");

        bool validGenre = true;
        std::vector<CStdString>::const_iterator iter = m_badWords.begin();
        while (iter != m_badWords.end())
        {
          CRegExp reg;
          reg.RegComp(*iter);
          if (reg.RegFind(strGenre) != -1)
          {
            validGenre = false;
            break;
          }

          iter++;
        }

        if (validGenre)
        {
          obj.SetValue(MSG_KEY_GENRE, videoInfoTag->m_strGenre);
        }
      }
    }
  }

  if (setReleaseDate)
  {
    CStdString strReleaseDate = pItem->GetProperty("releasedate");
    if (!strReleaseDate.IsEmpty())
    {
      obj.SetValue(MSG_KEY_RELEASE_DATE, strReleaseDate);
    }
  }

  if (setDescription)
  {
    if (!pItem->GetProperty("description").IsEmpty())
    {
      obj.SetValue(MSG_KEY_DESCRIPTION, pItem->GetProperty("description"));
    }
    else if (videoInfoTag && (!videoInfoTag->m_strPlot.IsEmpty()))
    {
      obj.SetValue(MSG_KEY_DESCRIPTION, videoInfoTag->m_strPlot);
    }
  }

  if (setBoxeeId)
  {
    if (!pItem->GetProperty("boxeeid").IsEmpty())
    {
      obj.SetValue(MSG_KEY_BOXEE_ID, pItem->GetProperty("boxeeid"));
    }
  }

  if (setShowId)
  {
    if (!pItem->GetProperty("showid").IsEmpty())
    {
      obj.SetValue(MSG_KEY_SHOW_ID, pItem->GetProperty("showid"));
    }
  }

  if (setShowName)
  {
    if (videoInfoTag)
    {
      if (!videoInfoTag->m_strShowTitle.empty())
      {
        obj.SetValue(MSG_KEY_SHOW_NAME, videoInfoTag->m_strShowTitle);
      }
    }
    else
    {
      if (!pItem->GetProperty("showname").IsEmpty())
      {
        obj.SetValue(MSG_KEY_SHOW_NAME, pItem->GetProperty("showname"));
      }
    }
  }

  if (setImdbId)
  {
    if (videoInfoTag)
    {
      if (!videoInfoTag->m_strIMDBNumber.empty())
      {
        obj.SetValue(MSG_KEY_IMDB_NUM, videoInfoTag->m_strIMDBNumber);
      }
    }
  }

  if (setYear)
  {
    CStdString strReleaseYear = pItem->GetProperty("releaseyear");
    if (!strReleaseYear.IsEmpty())
    {
      obj.SetValue(MSG_KEY_RELEASE_YEAR, strReleaseYear);
    }
    else
    {
      if (videoInfoTag)
      {
        if (videoInfoTag->m_iYear > 0)
        {
          CStdString strYear;
          strYear.Format("%i", videoInfoTag->m_iYear);
          obj.SetValue(MSG_KEY_RELEASE_YEAR, strYear);
        }
      }
    }
  }

  if (setRuntime)
  {
    if (videoInfoTag)
    {
      if (!videoInfoTag->m_strRuntime.empty())
      {
        obj.SetValue(MSG_KEY_RUN_TIME, videoInfoTag->m_strRuntime);
      }
    }
  }
}

CFileItemTypes::FileItemTypesEnums BoxeeUtils::GetRawFileItemType(const CFileItem* pItem)
{
  if (!pItem)
  {
    CLog::Log(LOGDEBUG,"BoxeeUtils::GetRawFileItemType - Enter function with a NULL item (new)");
    return CFileItemTypes::UNKNOWN;
  }

  CStdString itemLabel = pItem->GetLabel();
  CStdString itemPath = pItem->m_strPath;
  bool itemHasVideoInfoTag = pItem->HasVideoInfoTag();
  bool itemHasMusicInfoTag = pItem->HasMusicInfoTag();

  CLog::Log(LOGDEBUG,"BoxeeUtils::GetRawFileItemType - Enter function with item [label=%s][path=%s][HasVideoInfoTag=%d][HasMusicInfoTag=%d][PropertyIsVideo=%d][PropertyIsAudio=%d] (new)",itemLabel.c_str(),itemPath.c_str(),itemHasVideoInfoTag,itemHasMusicInfoTag,pItem->GetPropertyBOOL("isVideo"),pItem->GetPropertyBOOL("isAudio"));

  ////////////////////////////
  // check if item is local //
  ////////////////////////////

  bool isLocal = false;
  if (itemPath.IsEmpty())
  {
    // item doesn't have a path -> links are still in the LinksFileItemList (can be in case of share) -> consider as local=TRUE (because we don't know which link the user will choose)

    isLocal = true;
  }
  else if ( CUtil::IsHD(itemPath) ||
      CUtil::IsSmb(itemPath) ||
      CUtil::IsUPnP(itemPath) ||
      CUtil::IsStack(itemPath) ||
      CUtil::IsBoxeeDb(itemPath) ||
      itemPath.Left(6).Equals("rar://") ||
      itemPath.Left(6).Equals("zip://") )
  {
    isLocal = true;
  }

  ///////////////////////////////
  // check if item is resolved //
  ///////////////////////////////

  bool isResolved = false;
  if (itemHasVideoInfoTag || itemHasMusicInfoTag)
  {
    isResolved = true;
  }

  ///////////////////////////////
  // check for item media type //
  ///////////////////////////////

  bool isVideo = false;
  bool isMovie = false;
  bool isTvShow = false;

  bool isAudio = false;

  bool isPicture = false;

  CStdString rtsMediaTypeProperty = pItem->GetProperty("rts-mediatype");
  if (rtsMediaTypeProperty == "video")
  {
    isVideo = true;
  }
  else if (rtsMediaTypeProperty == "movie")
  {
    isVideo = true;
    isMovie = true;
  }
  else if (rtsMediaTypeProperty == "tv_show")
  {
    isVideo = true;
    isTvShow = true;
  }
  else if (pItem->GetPropertyBOOL("isVideo") || itemHasVideoInfoTag)
  {
    isVideo = true;

    if (pItem->GetPropertyBOOL("isMovie"))
    {
      isMovie = true;
    }
    else if (pItem->GetPropertyBOOL("isTvShow"))
    {
      isTvShow = true;
    }
  }
  else if (pItem->GetPropertyBOOL("isAudio") || itemHasMusicInfoTag)
  {
    isAudio = true;
  }
  else if (pItem->GetPropertyBOOL("isPicture"))
  {
    isPicture = true;
  }
  else
  {
    if (itemPath.IsEmpty())
    {
      //CLog::Log(LOGDEBUG,"BoxeeUtils::GetRawFileItemType - For Item [label=%s][path=%s], because the path is empty, going to return [UNKNOWN]. [isLocal=%d][isResolved=%d][isVideo=%d][isMovie=%d][isTvShow=%d][isAudio=%d] (new)",itemLabel.c_str(),itemPath.c_str(),isLocal,isResolved,isVideo,isMovie,isTvShow,isAudio);
      return CFileItemTypes::UNKNOWN;
    }
  }

  if (isLocal)
  {
    ///////////
    // LOCAL //
    ///////////

    if (isResolved)
    {
      //////////////////////
      // LOCAL - RESOLVED //
      //////////////////////

      if (isMovie)
      {
        //////////////////////////////
        // LOCAL - RESOLVED - MOVIE //
        //////////////////////////////

        return CFileItemTypes::LOCAL_RESOLVE_MOVIE;

      }
      else if (isTvShow)
      {
        ////////////////////////////////
        // LOCAL - RESOLVED - TV_SHOW //
        ////////////////////////////////

        return CFileItemTypes::LOCAL_RESOLVE_TVSHOW_EPISODE;
      }
      /*
      else if (isVideo)
      {
        // should be video under local ?
      }
       */
      else if (isAudio)
      {
        //////////////////////////////
        // LOCAL - RESOLVED - AUDIO //
        //////////////////////////////

        return CFileItemTypes::LOCAL_RESOLVE_AUDIO;
      }
      else if (isPicture)
      {
        ////////////////////////////////
        // LOCAL - RESOLVED - PICTURE //
        ////////////////////////////////

        return CFileItemTypes::LOCAL_RESOLVE_PICTURE;
      }
      else
      {
        CLog::Log(LOGERROR,"BoxeeUtils::GetRawFileItemType - Item [label=%s][path=%s] is LOCAL-RESOVED-UNKNOWN. Going to return [UNKNOWN]. [isLocal=%d][isResolved=%d][isVideo=%d][isMovie=%d][isTvShow=%d][isAudio=%d] (new)",itemLabel.c_str(),itemPath.c_str(),isLocal,isResolved,isVideo,isMovie,isTvShow,isAudio);
        return CFileItemTypes::UNKNOWN;
      }
    }
    else
    {
      ////////////////////////
      // LOCAL - UNRESOLVED //
      ////////////////////////

      if (isVideo)
      {
        ////////////////////////////////
        // LOCAL - UNRESOLVED - VIDEO //
        ////////////////////////////////

        return CFileItemTypes::LOCAL_UNKNOWN_VIDEO;

      }
      else if (isAudio)
      {
        ////////////////////////////////
        // LOCAL - UNRESOLVED - AUDIO //
        ////////////////////////////////

        return CFileItemTypes::LOCAL_UNKNOWN_AUDIO;
      }
      else if (isPicture)
      {
        //////////////////////////////////
        // LOCAL - UNRESOLVED - PICTURE //
        //////////////////////////////////

        return CFileItemTypes::LOCAL_UNKNOWN_PICTURE;
      }
      else
      {
        return CFileItemTypes::LOCAL_UNKNOWN;
      }
    }
  }
  else
  {
    /////////
    // WEB //
    /////////

    if (isResolved)
    {
      ////////////////////
      // WEB - RESOLVED //
      ////////////////////

      if (isMovie)
      {
        ////////////////////////////
        // WEB - RESOLVED - MOVIE //
        ////////////////////////////

        return CFileItemTypes::WEB_RESOLVE_MOVIE;

      }
      else if (isTvShow)
      {
        //////////////////////////////
        // WEB - RESOLVED - TV_SHOW //
        //////////////////////////////

        return CFileItemTypes::WEB_RESOLVE_TVSHOW_EPISODE;
      }
      else if (isVideo)
      {
        ////////////////////////////
        // WEB - RESOLVED - VIDEO //
        ////////////////////////////

        return CFileItemTypes::WEB_RESOLVE_VIDEO;
      }
      else if (isAudio)
      {
        ////////////////////////////
        // WEB - RESOLVED - AUDIO //
        ////////////////////////////

        return CFileItemTypes::WEB_RESOLVE_AUDIO;
      }
      else if (isPicture)
      {
        //////////////////////////////
        // WEB - RESOLVED - PICTURE //
        //////////////////////////////

        return CFileItemTypes::WEB_RESOLVE_PICTURE;
      }
      else
      {
        CLog::Log(LOGERROR,"BoxeeUtils::GetRawFileItemType - Item [label=%s][path=%s] is WEB-RESOVED-UNKNOWN. Going to return [UNKNOWN]. [isLocal=%d][isResolved=%d][isVideo=%d][isMovie=%d][isTvShow=%d][isAudio=%d] (new)",itemLabel.c_str(),itemPath.c_str(),isLocal,isResolved,isVideo,isMovie,isTvShow,isAudio);
        return CFileItemTypes::UNKNOWN;
      }
    }
    else
    {
      //////////////////////
      // WEB - UNRESOLVED //
      //////////////////////

      if (isVideo)
      {
        //////////////////////////////
        // WEB - UNRESOLVED - VIDEO //
        //////////////////////////////

        return CFileItemTypes::WEB_UNKNOWN_VIDEO;

      }
      else if (isAudio)
      {
        //////////////////////////////
        // WEB - UNRESOLVED - AUDIO //
        //////////////////////////////

        return CFileItemTypes::WEB_UNKNOWN_AUDIO;
      }
      else if (isPicture)
      {
        ////////////////////////////////
        // WEB - UNRESOLVED - PICTURE //
        ////////////////////////////////

        return CFileItemTypes::WEB_UNKNOWN_PICTURE;
      }
      else
      {
        return CFileItemTypes::WEB_UNKNOWN;
      }
    }
  }
}

CStdString BoxeeUtils::GetFileItemTypesEnumsAsString(CFileItemTypes::FileItemTypesEnums fileItemTypeEnum)
{
  switch(fileItemTypeEnum)
  {
  case CFileItemTypes::LOCAL_RESOLVE_MOVIE/*0*/:
    return "LOCAL_RESOLVE_MOVIE";
    break;
  case CFileItemTypes::LOCAL_RESOLVE_TVSHOW_EPISODE/*=1*/:
    return "LOCAL_RESOLVE_TVSHOW_EPISODE";
    break;
  case CFileItemTypes::LOCAL_RESOLVE_AUDIO/*2*/:
    return "LOCAL_RESOLVE_AUDIO";
    break;
  case CFileItemTypes::LOCAL_RESOLVE_PICTURE/*3*/:
    return "LOCAL_RESOLVE_PICTURE";
    break;
  case CFileItemTypes::LOCAL_UNKNOWN_VIDEO/*4*/:
    return "LOCAL_UNKNOWN_VIDEO";
    break;
  case CFileItemTypes::LOCAL_UNKNOWN_AUDIO/*5*/:
    return "LOCAL_UNKNOWN_AUDIO";
    break;
  case CFileItemTypes::LOCAL_UNKNOWN_PICTURE/*6*/:
    return "LOCAL_UNKNOWN_PICTURE";
    break;
  case CFileItemTypes::LOCAL_UNKNOWN/*7*/:
    return "LOCAL_UNKNOWN";
    break;
  case CFileItemTypes::WEB_RESOLVE_VIDEO/*8*/:
    return "WEB_RESOLVE_VIDEO";
    break;
  case CFileItemTypes::WEB_RESOLVE_MOVIE/*9*/:
    return "WEB_RESOLVE_MOVIE";
    break;
  case CFileItemTypes::WEB_RESOLVE_TVSHOW_EPISODE/*10*/:
    return "WEB_RESOLVE_TVSHOW_EPISODE";
    break;
  case CFileItemTypes::WEB_RESOLVE_AUDIO/*11*/:
    return "WEB_RESOLVE_AUDIO";
    break;
  case CFileItemTypes::WEB_RESOLVE_PICTURE/*12*/:
    return "WEB_RESOLVE_PICTURE";
    break;
  case CFileItemTypes::WEB_UNKNOWN_VIDEO/*13*/:
    return "WEB_UNKNOWN_VIDEO";
    break;
  case CFileItemTypes::WEB_UNKNOWN_AUDIO/*14*/:
    return "WEB_UNKNOWN_AUDIO";
    break;
  case CFileItemTypes::WEB_UNKNOWN_PICTURE/*15*/:
    return "WEB_UNKNOWN_AUDIO";
    break;
  case CFileItemTypes::WEB_UNKNOWN/*16*/:
    return "WEB_UNKNOWN";
    break;
  default:
    CLog::Log(LOGDEBUG,"BoxeeUtils::GetFileItemTypesEnumsAsString - Unknown FileItemTypesEnums enum [%d]. Going to return [UNKNOWN] as type (fito)",fileItemTypeEnum);
    return "UNKNOWN"/*17*/;
    break;
  }
}

bool BoxeeUtils::CanBrowse(const CFileItem& item)
{
  if(item.IsPlayList())
  {
    return false;
  }

  if (item.HasProperty("parentfolder"))
  {
    return true;
  }

  if (item.GetPropertyBOOL("isdvd"))
  {
    return true;
  }

  if (item.GetPropertyBOOL("istvshowfolder"))
  {
    return true;
  }

  if (item.m_bIsFolder)
  {
    if((item.GetPropertyBOOL("ishistory")))
    {
      if(!item.GetPropertyBOOL("isalbum"))
      {
        return true;
      }
    }
    else
    {
      if (item.GetPropertyBOOL("isDvdFolder"))
      {
        return true;
      }
    }
  }

  if (item.GetPropertyBOOL("IsGroup"))
  {
    return true;
  }

  return false;
}

bool BoxeeUtils::CanEject(const CFileItem& item)
{
  if (item.GetPropertyBOOL("isdvd")) {
    return true;
  }
  return false;
}

bool BoxeeUtils::CanRecognize(const CFileItem& item)
{
  if (item.HasProperty("isfeeditem"))
    return false;

  if (item.IsVideo() && item.GetPropertyBOOL("isFolderItem") && g_settings.IsPathOnSource(item.m_strPath))
  {
    return true;
  }

  if (item.IsVideo() && item.HasProperty("boxeeDBvideoId"))
  {
    return true;
  }

  return false;
}

bool BoxeeUtils::CanRemove(const CFileItem& item)
{
  if (item.IsApp())
    return false;

  if (item.HasProperty("isfeeditem"))
    return false;

  if (item.HasProperty("istvshowfolder"))
    return false;

  if (item.HasProperty("isinternetstream"))
    return false;

  if (item.HasProperty("ismovie") || item.HasProperty("istvshow") || item.HasProperty("isalbum"))
    return true;

  return false;
}

bool BoxeeUtils::CanShare(const CFileItem& item, bool checkIsLoaded)
{
  return CanRecommend(item, checkIsLoaded);
}

bool BoxeeUtils::CanRecommend(const CFileItem& item, bool checkIsLoaded)
{
  if (g_application.IsOfflineMode())
  {
    return false;
  }

  if (checkIsLoaded && !item.GetPropertyBOOL("isloaded"))
    return false;

  if (item.GetPropertyBOOL("istvshowfolder"))
    return false;

  if (item.HasVideoInfoTag() || item.HasMusicInfoTag() || item.IsRSS() || item.IsInternetStream())
    return true;

  return false;
}

bool BoxeeUtils::CanRate(const CFileItem& item)
{
  // Function is not implemented at the moment
  return true;
}

bool BoxeeUtils::CanResume(const CFileItem& item)
{
  if (item.IsInternetStream())
    return false;

  if (item.HasProperty("isresumable"))
  {
    return item.GetPropertyBOOL("isresumable");
  }

  if (item.IsVideo())
    return true;

  return false;
}

bool BoxeeUtils::CanPlay(const CFileItem& item)
{
  if (item.HasLinksList())
  {
    return true;
  }

  // This is a series folder, should not be played
  if (item.GetPropertyBOOL("istvshowfolder"))
    return false;

  if (item.GetPropertyBOOL("ishistory"))
  {
    if ((item.IsAudio() || item.IsVideo() || item.IsPicture()))
    {
      return true;
    }
    else if ((item.m_bIsFolder) && (item.GetPropertyBOOL("isDvdFolder")))
    {
      return true;
    }
    else if(item.GetPropertyBOOL("isalbum"))
    {
      if (item.GetPropertyBOOL("isvirtual"))
        return false;
      else
        return true;
    }
    else if(item.IsPlugin())
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  if (item.GetPropertyBOOL("isdvd"))
  {
    return true;
  }

  if (item.GetProperty("isshoutcast") == "true" || item.IsShoutCast())
    return true;

  if (item.GetProperty("islastfm") == "true" || item.IsLastFM())
    return true;

  if (item.GetPropertyBOOL("isplugin") && item.GetPropertyBOOL("hasvideoinfo"))
    return true;

  if ((item.m_bIsFolder) && (item.GetPropertyBOOL("isDvdFolder")))
  {
    return true;
  }

  if (item.GetPropertyBOOL("isvideo") || (item.GetProperty("isvideo") == "true"))
  {
    if (item.m_strPath.Left(7) == "feed://")
    {
      return false;
    }
    else if (item.m_strPath == "")
    {
      return false;
    }
    else
    {
      return true;
    }
  }

  if (item.GetPropertyBOOL("isinternetstream"))
  {
    if (item.GetPropertyBOOL("ismusic") && item.m_strPath != "")
    {
      return true;
    }

    if ((item.IsAudio() || item.IsVideo() || item.IsPicture()) && item.m_strPath != "")
    {
      return true;
    }

    return false;
  }

  if(item.GetPropertyBOOL("isalbum") && item.HasMusicInfoTag())
  {
    // If the album was found in the database and the path is currently available
    CStdString albumPath =  item.GetProperty("AlbumFolderPath");
    if (item.GetPropertyBOOL("isplayable") && item.HasProperty("AlbumFolderPath") &&
        g_application.IsPathAvailable(item.GetProperty("AlbumFolderPath")))
    {
      return true;
    }
    if (item.GetPropertyBOOL("isfeeditem"))
    {
      return false;
    }
    return true;
  }

  // Any video or audio item can be played
  if (item.IsAudio() || item.IsVideo())
  {
    return true;
  }

  // In case of picture in feed
  if (item.IsPicture())
    return true;

  return false;
}

bool BoxeeUtils::CanQueue(const CFileItem& item, CStdString& referral)
{
  if (BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().IsInQueue(item.GetProperty("boxeeid"),item.m_strPath,referral))
  {
    return false;
  }
  else
  {
    return true;
  }
}

bool BoxeeUtils::HasLastFm(const CFileItem& item)
{
  if(item.GetPropertyBOOL("isalbum") && item.HasMusicInfoTag())
    return true;
  return false;
}

bool BoxeeUtils::CanDelete(const CFileItem& item)
{
  if(item.GetPropertyBOOL("ishistory"))
    return true;
  return false;
}
bool BoxeeUtils::CanMarkWatched(const CFileItem& item)
{
  if (item.IsVideo() || item.IsInternetStream())
  {
    return true;
  }
  return false;
}

void BoxeeUtils::MarkWatched(CFileItem* pItem)
{
  if (CanMarkWatched(*pItem))
  {
    // get watched status of the item and set the "watched" property

    CStdString strBoxeeId = pItem->GetProperty("boxeeid");
    bool bWatched = false;
    if (strBoxeeId.IsEmpty())
    {
      bWatched = BOXEE::Boxee::GetInstance().GetMetadataEngine().IsWatchedByPath(pItem->m_strPath);
    }
    else
    {
      bWatched = BOXEE::Boxee::GetInstance().GetMetadataEngine().IsWatchedById(strBoxeeId);
    }

    if (bWatched)
    {
      CLog::Log(LOGDEBUG,"BoxeeUtils::MarkWatched, watched path = %s (watched)", pItem->m_strPath.c_str());
    }

    pItem->SetProperty("watched", bWatched);
  }
}

bool BoxeeUtils::HasMoreInfo(const CFileItem& item)
{
  if (item.HasVideoInfoTag() || item.HasMusicInfoTag() || item.HasPictureInfoTag())
  {
    return true;
  }

  return false;
}

bool BoxeeUtils::HasDescription(const CFileItem& item)
{
  if (!item.GetProperty("description").IsEmpty())
  {
    return true;
  }

  if (item.HasVideoInfoTag() && !(item.GetVideoInfoTag()->m_strPlotOutline.IsEmpty()))
  {
    return true;
  }

  if (item.HasMusicInfoTag() && !item.GetProperty("description").IsEmpty())
  {
    return true;
  }

  if ((item.HasPictureInfoTag() || item.IsPicture()) && !item.GetProperty("description").IsEmpty())
  {
    return true;
  }

  return false;
}

bool BoxeeUtils::HasTrailer(const CFileItem& item)
{
  if (item.GetPropertyBOOL("isfeeditem"))
  {
    return false;
  }
  if (item.GetPropertyBOOL("istrailer"))
  {
    return false;
  }
  if (item.HasVideoInfoTag())
  {
    if (item.GetVideoInfoTag()->m_strTrailer != "")
    {
      return true;
    }
  }

  return false;
}

bool BoxeeUtils::IsPathLocal(const CStdString& path)
{
  if (!CUtil::IsHD(path) && !CUtil::IsSmb(path) && !CUtil::IsUPnP(path))
  {
    return false;
  }

  return true;
}

void BoxeeUtils::UpdateStackFileForServerReport(const CFileItem& item, CFileItem& stackItem)
{
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // stack file is a divided movie file -> set relevant properties from original item in the stack item for correct server report //
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  if (item.HasVideoInfoTag())
  {
    *(stackItem.GetVideoInfoTag()) = *(item.GetVideoInfoTag());

    stackItem.SetLabel(stackItem.GetVideoInfoTag()->m_strTitle);
  }

  if (item.HasProperty("boxeeid"))
  {
    stackItem.SetProperty("boxeeid",item.GetProperty("boxeeid"));
  }

  if (item.HasProperty("rts-mediatype"))
  {
    stackItem.SetProperty("rts-mediatype",item.GetProperty("rts-mediatype"));
  }

  if (item.HasProperty("isVideo"))
  {
    stackItem.SetProperty("isVideo",item.GetProperty("isVideo"));
  }

  if (item.HasProperty("isMovie"))
  {
    stackItem.SetProperty("isMovie",item.GetProperty("isMovie"));
  }
}

CStdString BoxeeUtils::GetUrlFromItemForRepostToServer(const CFileItem& item)
{
  CStdString itemUrlForReport = "";

  if (!item.m_strPath.IsEmpty() && !CUtil::IsHD(item.m_strPath) && !CUtil::IsSmb(item.m_strPath) && !CUtil::IsUPnP(item.m_strPath))
  {
    itemUrlForReport = item.m_strPath;
  }

  return itemUrlForReport;
}

int BoxeeUtils::StringTokenize(const CStdString& path, std::set<CStdString>& tokens, const CStdString& delimiters, bool shouldTrim, bool shouldLower)
{
  // Tokenize ripped from http://www.linuxselfhelp.com/HOWTO/C++Programming-HOWTO-7.html
  // Skip delimiters at beginning.
  std::string::size_type lastPos = path.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  std::string::size_type pos = path.find_first_of(delimiters, lastPos);

  while (std::string::npos != pos || std::string::npos != lastPos)
  {
    // Found a token, add it to the set.
    CStdString token = path.substr(lastPos, pos - lastPos);

    if (shouldTrim)
    {
      token.Trim();
    }

    if (shouldLower)
    {
      token.ToLower();
    }

    tokens.insert(token);

    // Skip delimiters.  Note the "not_of"
    lastPos = path.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = path.find_first_of(delimiters, lastPos);
  }

  return (int)tokens.size();
}

CStdString BoxeeUtils::URLEncode(const CURL &url)
{
  /* due to smb wanting encoded urls we have to build it manually */

  CStdString flat = "smb://";

  if(url.GetDomain().length() > 0)
  {
    flat += url.GetDomain();
    flat += ";";
  }

  /* samba messes up of password is set but no username is set. don't know why yet */
  /* probably the url parser that goes crazy */
  if(url.GetUserName().length() > 0 /* || url.GetPassWord().length() > 0 */)
  {
    flat += url.GetUserName();
    flat += ":";
    flat += url.GetPassWord();
    flat += "@";
  }
  else if( !url.GetHostName().IsEmpty() && !g_guiSettings.GetString("smb.username").IsEmpty() )
  {
    /* okey this is abit uggly to do this here, as we don't really only url encode */
    /* but it's the simplest place to do so */
    flat += g_guiSettings.GetString("smb.username");
    flat += ":";
    flat += g_guiSettings.GetString("smb.password");
    flat += "@";
  }

  flat += url.GetHostName();

  /* okey sadly since a slash is an invalid name we have to tokenize */
  std::vector<CStdString> parts;
  std::vector<CStdString>::iterator it;
  CUtil::Tokenize(url.GetFileName(), parts, "/");
  for( it = parts.begin(); it != parts.end(); it++ )
  {
    flat += "/";
    flat += (*it);
  }

  /* okey options should go here, thou current samba doesn't support any */

  return flat;
}

// support strings from the following format:
// {__id:__string}
// replace the string with the matching id from the Strings file,
// if it doesn't exit use __string as default
CStdString BoxeeUtils::TranslateStringById(const CStdString  &srcStr)
{
  CStdString tmpStr = srcStr;
  CStdString resStr = srcStr;
  CStdString defaultVal;
  int        strKey;

  CStdString  strRegExpr("{[0-9]+:[^}]+}");
  CRegExp reg;

  reg.RegComp(strRegExpr);

  int iPos = reg.RegFind(srcStr);

  if (iPos != -1)
  {

    // get the String Key it placed between the '{' and the ':'
    int keyLength;
    int endPos;
    endPos = srcStr.find(':');
    keyLength = endPos - iPos;
    strKey = BOXEE::BXUtils::StringToInt(srcStr.Mid(iPos + 1, keyLength));

    // keep the right side of the string
    CStdString firstPart = srcStr.Left(iPos);
    tmpStr = srcStr.Right(srcStr.length() - (iPos + keyLength));

    // keep the left side of the string
    iPos = tmpStr.find('}');
    CStdString secondPart = tmpStr.Right(tmpStr.length() - iPos - 1);
    secondPart = secondPart.Left(tmpStr.length() -1);

    // check if the key exists in the strings db
    defaultVal = g_localizeStrings.Get(strKey);

    // if not - get the default string value which is placed
    // between the ':' and the '}'
    if (defaultVal.IsEmpty())
    {

      CStdString  defulStrRegExpr(":[^}]+}");
      reg.RegComp(defulStrRegExpr);

      iPos = reg.RegFind(tmpStr);
      if (iPos != -1 )
      {
        iPos = tmpStr.find('}');
        defaultVal = tmpStr.Left(iPos);
        defaultVal = defaultVal.Right(defaultVal.length() -1 );

      } else {
        CLog::Log(LOGERROR,"Error occurred while trying to parse string");
        return resStr;
      }
    } else
    {
      CLog::Log(LOGDEBUG,"String Key %s doesnt exist use the default value ", defaultVal.c_str());
    }

    // concat the result
    resStr = firstPart;
    resStr += defaultVal;
    resStr += secondPart;
  }

  return resStr;
}

void BoxeeUtils::UpdateProfile(int profileIndex, BOXEE::BXObject& userObj)
{
  if(profileIndex < (int)g_settings.m_vecProfiles.size())
  {
    CProfile& p = g_settings.m_vecProfiles[profileIndex];

    // Update user thumb (if necessary)
    CStdString currentThumb = p.getThumb();
    CStdString userThumbOnServer = userObj.GetValue(MSG_KEY_THUMB);
    if(currentThumb != userThumbOnServer)
    {
      p.setThumb(userThumbOnServer);
      CLog::Log(LOGDEBUG,"For profile [profileIndex=%d] thumb was update from [currentThumb=%s] to [newThumb=%s] (login)",profileIndex,currentThumb.c_str(),p.getThumb().c_str());
    }

    // Update user display name (if necessary)
    CStdString currentName = p.getName();
    CStdString nameOnServer = userObj.GetValue(MSG_KEY_NAME);
    if(currentName != nameOnServer)
    {
      p.setName(nameOnServer);
      CLog::Log(LOGDEBUG,"For profile [profileIndex=%d] thumb was update from [currentName=%s] to [newName=%s] (login)",profileIndex,currentName.c_str(),p.getName().c_str());
    }

    g_settings.SaveProfiles(PROFILES_FILE);
  }
  else
  {
    CLog::Log(LOGWARNING,"Failed to update user profile because [profileIndex=%d] and [ProfileVecSize=%d] (login)",profileIndex,(int)g_settings.m_vecProfiles.size());
  }
}

