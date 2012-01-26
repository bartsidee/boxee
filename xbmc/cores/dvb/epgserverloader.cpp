  #include "system.h"

#ifdef HAS_DVB
#include <boost/foreach.hpp>

#include "epgserverloader.h"
#include "dvbmanager.h"
#include <time.h>
#include "bxcurl.h"
#include "boxee.h"
#include "bxconfiguration.h"
#include "bxutils.h"
#include "epgstore.h"
#include "../../Util.h"
#include "xbmc/utils/log.h"
#include "Application.h"

// 24 hours
#define FULL_EPG_PULL_INTERVAL (24 * 60 * 60)

// 5 Min
#define MAX_SERVER_ACCESS_TIME (5 * 60)

#define SOCIAL_PULL_INTERVAL (1 * 60)

EpgServerLoader::EpgServerLoader()
{
  m_lastFullEpgPull = 0;
  m_lastSocialPull = 0;
  m_requestLoad = false;
}

void EpgServerLoader::Process()
{
#ifdef HAS_SERVER_OTA
  time_t lastServerAccessTime = 0;

  while (!m_bStop)
  {
    Sleep(1000);

    if (!g_application.IsConnectedToInternet())
      continue;

    if (!DVBManager::GetInstance().GetChannels().HasChannels())
      continue;

    time_t now = time(NULL);

    lastServerAccessTime = now;

    if (m_requestLoad || m_lastFullEpgPull + FULL_EPG_PULL_INTERVAL < now)
    {
      PullEPG();
      m_lastFullEpgPull = now;
    }

    if (m_requestLoad || (m_epgVisible && m_lastSocialPull + SOCIAL_PULL_INTERVAL < now) || m_lastSocialPull == 0)
    {
      PullSocial();
      m_lastSocialPull = now;
    }

    m_requestLoad = false;
  }
#endif
}

std::string EpgServerLoader::TimeToString(time_t t)
{
  char result[64];
  struct tm *tms = gmtime(&t);
  strftime(result, sizeof(result)-1, "%Y-%m-%d%%20%H:%M:%S", tms);
  return result;
}

time_t EpgServerLoader::StringToTime(std::string str)
{
  struct tm tms;
  memset(&tms, 0, sizeof(tms));
  strptime(str.c_str(), "%Y-%m-%d %H:%M:%S", &tms);
  time_t result = timegm(&tms);
  return result;
}

void EpgServerLoader::NotifyEpgVisibilityChange(bool visible)
{
  m_epgVisible = visible;
}

bool EpgServerLoader::PullSocial()
{
  CLog::Log(LOGDEBUG, "Pull social");

  CStdString strUrl = BOXEE::BXConfiguration::GetInstance().GetURLParam("Boxee.ApiEpgProgram","http://app.boxee.tv/epg/watch");
  Json::Value jsonValue;
  int returnCode;
  if (!BOXEE::BXUtils::PerformJSONGetRequest(strUrl, jsonValue, returnCode, false))
  {
    CLog::Log(LOGERROR, "Got error return code from server for url: <%s>", strUrl.c_str());
    return false;
  }

#if 0
  printf("Got social:\n");
  Json::StyledWriter writer;
  std::string epgStr = writer.write(jsonValue);
  printf("%s\n", epgStr.c_str());
#endif

  if (!jsonValue.isArray())
  {
    CLog::Log(LOGERROR, "Error parsing social. Not an array.");
    return false;
  }

  CLog::Log(LOGINFO, "Pull social ok. got: %d items", jsonValue.size());
  if (jsonValue.size() == 0)
  {
    return true;
  }

  std::vector<EpgSocialInfo>* allSocialInfo = new std::vector<EpgSocialInfo>;

  for (Json::ArrayIndex j = 0; j < jsonValue.size(); j++)
  {
    Json::Value m = jsonValue[j];

    EpgSocialInfo socialInfo;

    Json::Value friends = m["friends"];
    if (friends != Json::nullValue && friends.isArray())
    {
      for (Json::ArrayIndex w = 0; w < friends.size(); w++)
      {
        Json::Value watcher = friends[w];

        EpgFriendInfo friendInfo;
        friendInfo.firstName = watcher["first_name"].asString();
        friendInfo.lastName = watcher["last_name"].asString();
        friendInfo.name = watcher["name"].asString();
        friendInfo.shortName = watcher["short_name"].asString();
        friendInfo.thumbSmallUrl = watcher["thumb_small"].asString();
        friendInfo.thumbUrl = watcher["thumb"].asString();
        friendInfo.user_id = watcher["user_id"].asString();
        socialInfo.friends.push_back(friendInfo);
      }
    }

    if (!m["watching"].isNull())
    {
      socialInfo.watching = m["watching"].asInt();
    }

//    socialInfo.watching = 12342;
//    EpgFriendInfo f;
//    f.firstName = "Nimrod";
//    f.thumbSmallUrl = "http://www.boxee.tv/htdocs/images/boxee_logo.png";
//    socialInfo.friends.push_back(f);
//    f.firstName = "Alon";
//    f.thumbSmallUrl = "https://secure.gravatar.com/avatar/401c9ea9450926cb7497750aebafd4f3?s=140&d=https://a248.e.akamai.net/assets.github.com%2Fimages%2Fgravatars%2Fgravatar-140.png";
//    socialInfo.friends.push_back(f);

    // Set the label
    int numOfFriendsWatching = socialInfo.friends.size();
    switch (numOfFriendsWatching)
    {
    case 1:
      // 1 friend is watching
      socialInfo.label.Format("[B][COLOR whiteheidi]%s[/COLOR][/B]", socialInfo.friends[0].firstName.c_str());
      break;

    case 2:
      // 2 friend is watching
      socialInfo.label.Format("[B][COLOR whiteheidi]%s, %s[/COLOR][/B]", socialInfo.friends[0].firstName.c_str(),
          socialInfo.friends[1].firstName.c_str());
      break;
    }

    int moreWatching = socialInfo.watching - numOfFriendsWatching;
    if (numOfFriendsWatching && moreWatching == 1)
      socialInfo.label += " and one other person are watching";
    else if (numOfFriendsWatching && moreWatching > 1)
      socialInfo.label.Format("%s and %d others are watching", socialInfo.label.c_str(), moreWatching);
    else if (numOfFriendsWatching == 1 && moreWatching == 0)
      socialInfo.label.Format("%s is watching", socialInfo.label.c_str());
    else if (numOfFriendsWatching == 2 && moreWatching == 0)
      socialInfo.label.Format("%s are watching", socialInfo.label.c_str());
    else if (numOfFriendsWatching == 0 && moreWatching == 1)
      socialInfo.label = "One other person is watching";
    else if (numOfFriendsWatching == 0 && moreWatching > 1)
      socialInfo.label.Format("%d others are watching", moreWatching);

    if (!m["program_id"].isNull())
      EpgStore::GetInstance().SetChannelSocialInfo(m["program_id"].asString(), socialInfo);
    else if (!m["service"].isNull())
      EpgStore::GetInstance().SetChannelSocialInfo(m["service"].asString(), socialInfo);

    allSocialInfo->push_back(socialInfo);
  }

  EpgStore::GetInstance().SetSocialInfo(allSocialInfo);

  CLog::Log(LOGDEBUG, "Pull social done");

  return true;
}


bool EpgServerLoader::PullEPG()
{
  CStdString strUrl = BOXEE::BXConfiguration::GetInstance().GetURLParam("Boxee.ApiEpgProgram","http://app.boxee.tv/epg/grid");

  CLog::Log(LOGINFO, "Pull epg start from URL %s", strUrl.c_str());

  Json::Value jsonValue;
  int returnCode;
  if (!BOXEE::BXUtils::PerformJSONGetRequest(strUrl, jsonValue, returnCode, false))
  {
    CLog::Log(LOGERROR, "EpgServerLoader::PullEPG: Got error return code from server for url: <%s>", strUrl.c_str());
    return false;
  }

  CLog::Log(LOGINFO, "Pull epg complete, parsing response");

#if 0
  printf("Got EPG\n");
  Json::StyledWriter writer;
  std::string epgStr = writer.write(jsonValue);
  printf("%s\n", epgStr.c_str());
#endif

  if (!jsonValue.isArray())
  {
    CLog::Log(LOGERROR, "EpgServerLoader::PullEPG: Error parsing epg. Not array..");
    return false;
  }

  if (jsonValue.size() == 0)
  {
    CLog::Log(LOGERROR, "EpgServerLoader::PullEPG: Error parsing epg. Empty response.");
    return true;
  }

  CLog::Log(LOGINFO, "Pull epg ok. got: %d items", jsonValue.size());

  void* tx = NULL;
  try
  {
    tx = EpgStore::GetInstance().StartEventsTransaction();

    EpgStore::GetInstance().CleanEventsBySource(tx, EPG_SOURCE_ID_SERVER);

    for (Json::ArrayIndex j = 0; j < jsonValue.size(); j++)
    {
      Json::Value m = jsonValue[j];

      time_t start = StringToTime(m["start"].asString());
      time_t end = StringToTime(m["end"].asString());

      CStdString programId;
      programId.Format("%s-%lu", m["program_id"].asString().c_str(), start);

      EpgProgramInfo info;
      info.id = programId;
      info.channelId = m["channel_id"].asString();
      info.start = start;
      info.end = end;
      info.title = m["title"].asString();
      info.synopsis = m["synopsis"].asString();
      info.rating = m["rating"].asString();
      info.origAirDate = m["orig_airdate"].asString();
      info.seasonNumber = m["season"].asString();
      info.episodeNumber = m["episode"].asString();
      info.thumb = m["thumb"].asString();
      info.isNew = m["new"].asBool();
      info.thumb = m["thumb"].asString();
      info.showRunNumber = m["ep_syn_num"].asString();
      info.episodeTitle = m["ep_title"].asString();

      EpgStore::GetInstance().InsertEvent(tx, EPG_SOURCE_ID_SERVER, info);
    }

    EpgStore::GetInstance().CommitEventsTransaction(tx);
  }
  catch (std::exception& ex)
  {
    if (tx)
      EpgStore::GetInstance().RollbackEventsTransaction(tx);

    CLog::Log(LOGERROR, "EpgServerLoader::PullEPG: Got exception: %s", ex.what());

    return false;
  }

  CLog::Log(LOGDEBUG, "Pull epg done");

  return true;
}

#endif
