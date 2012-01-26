#include "system.h"

#ifdef HAS_DVB

#include <json/json.h>
#include <json/writer.h>

#include "SpecialProtocol.h"
#include "File.h"
#include "Util.h"
#include "utils/log.h"
#include "SingleLock.h"

#define DB_FILE "special://home/dvb/epg3.sqlite3"
#define DB_FILE_TMP "special://temp/epg3.sqlite3"

#define DB_FORMAT_VERSION 1

#define SYNC_CACHE_INTERVAL_SEC (5 * 60)

#include <time.h>
#include "sqlite3pp.h"
#include "epgstore.h"
#include "CharsetConverter.h"

using namespace XFILE;

EpgStore &EpgStore::GetInstance()
{
  static EpgStore instance;
  return instance;
}

EpgStore::EpgStore()
{
  CopyFile(_P(DB_FILE), _P(DB_FILE_TMP), false);

  m_db = new sqlite3pp::database;

  CStdString dbFilePath = _P(DB_FILE_TMP);
  m_db->connect(dbFilePath.c_str());

  // Check database
  std::string integrityCheckResponse;
  sqlite3pp::query qry(*m_db, "PRAGMA integrity_check");
  for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
  {
    (*i).getter() >> integrityCheckResponse;
  }

  CLog::Log(LOGDEBUG, "EPG database integrity check response: %s", integrityCheckResponse.c_str());

  if (integrityCheckResponse != "ok")
  {
    CLog::Log(LOGERROR, "EPG database corrupt, deleting it");

    m_db->disconnect();
    XFILE::CFile::Delete(DB_FILE);
    XFILE::CFile::Delete(DB_FILE_TMP);

    CStdString dbFilePath = _P(DB_FILE_TMP);
    m_db->connect(dbFilePath.c_str());
  }

  CreateTables();

  m_insertEventCmd = new sqlite3pp::command
      (*m_db, "INSERT OR REPLACE INTO programs (id, channel_id, source, start_time, end_time, data) "
             "   VALUES (?, ?, ?, ?, ?, ?)");

  m_socialInfo = NULL;

  m_hEpgDataEvent = CreateEvent(NULL, false, false, NULL);

  // since we store db file on ramdisk
  // we instruct here sqlite not to use cache
  // and write transactions directly to file
  m_db->execute("PRAGMA cache_size = 0");


  Create();
}

EpgStore::~EpgStore()
{
  m_bStop = true;
  SetEvent(m_hEpgDataEvent);
  StopThread(true);
  CloseHandle(m_hEpgDataEvent);

  delete m_insertEventCmd;
  delete m_db;

  if (m_socialInfo)
    delete m_socialInfo;

  CopyFile(_P(DB_FILE_TMP), _P(DB_FILE), false);
}

void EpgStore::CreateTables()
{
  m_db->execute(
      "CREATE TABLE IF NOT EXISTS programs "
      "("
      "  id VARCHAR(32), "
      "  channel_id VARCHAR(32), "
      "  source VARCHAR(16), "
      "  start_time VARCHAR(32), "
      "  end_time VARCHAR(32), "
      "  data VARCHAR(16384)"
      ")"
    );

  m_db->execute(
      "CREATE INDEX IF NOT EXISTS time_index ON programs "
      "("
      "  start_time,"
      "  end_time"
      ")"
    );

  m_db->execute(
      "CREATE INDEX IF NOT EXISTS channel_index ON programs "
      "("
      "  channel_id"
      ")"
    );

  m_db->execute(
      "CREATE UNIQUE INDEX IF NOT EXISTS event_index ON programs "
      "("
      "  channel_id,"
      "  id"
      ")"
    );
}

void* EpgStore::StartEventsTransaction()
{
  return new sqlite3pp::transaction(*m_db);
}

void EpgStore::CommitEventsTransaction(void* transaction)
{
  sqlite3pp::transaction* t = (sqlite3pp::transaction*) transaction;
  if (t)
  {
    t->commit();
    delete t;
  }
}

void EpgStore::RollbackEventsTransaction(void* transaction)
{
  sqlite3pp::transaction* t = (sqlite3pp::transaction*) transaction;

  if (t)
  {
    t->rollback();
    delete t;
  }
}

CStdString EpgStore::CleanDescription(const CStdString& strDescription)
{
  CStdString description = strDescription;

  description.Replace("&nbsp;", " ");

  while (description.Find("<") != -1)
  {
    int start = description.Find("<");
    int end = description.Find(">");
    if (end > start)
      description.Delete(start, end-start+1);
    else
      description.Delete(start, description.GetLength() - start);
  }
  description.Trim();

  description.Replace("\\&", "&");
  description.Replace("&quot;", "\"");
  description.Replace("&amp;", "&");
  description.Replace("&nbsp;", " ");
  description.Replace("&gt;", ">");
  description.Replace("&lt;", "<");

  int i;
  while ((i = description.Find("&#")) >= 0)
  {
    CStdString src = "&#";
    int radix = 10;

    i += 2;
    if (description[i] == 'x' || description[i] == 'X')
    {
      src += description[i];
      i++;
      radix = 16;
    }

    CStdString numStr;
    unsigned long num;
    while (description[i] != ';' && numStr.length() <= 6)
    {
      numStr += description[i];
      src += description[i];
      i++;
    }

    // doesn't make sense....abort
    if (numStr.length() > 6)
    {
      break;
    }

    src += ';';
    char* end;

    if (numStr.IsEmpty())
      break;

    num = strtoul(numStr.c_str(), &end, radix);

    // doesn't make sense....abort
    if (num == 0)
    {
      break;
    }

    CStdStringW utf16;
    utf16 += (wchar_t) num;
    CStdStringA utf8;
    g_charsetConverter.wToUTF8(utf16, utf8);
    description.Replace(src, utf8);
  }

  return description;
}

void EpgStore::InsertEvent(void* transaction, CStdString source, EpgProgramInfo info)
{
  CStdString myTitle = info.title;
  myTitle.Trim();
  CStdString mySynopsis = info.synopsis;
  mySynopsis.Trim();

  Json::Value result;
  result["title"] = CleanDescription(myTitle);
  result["synopsis"] = CleanDescription(mySynopsis);
  if (info.rating.length())
    result["rating"] = info.rating;
  if (info.origAirDate.length())
    result["orig_airdate"] = info.origAirDate;
  if (info.seasonNumber.length())
    result["season_num"] = info.seasonNumber;
  if (info.episodeNumber.length())
    result["episode_num"] = info.episodeNumber;
  if (info.thumb.length())
    result["thumb"] = info.thumb;
  if (info.showRunNumber.length())
    result["show_num"] = info.showRunNumber;
  if (info.episodeTitle.length())
    result["episode_title"] = CleanDescription(info.episodeTitle);
  result["new"] = info.isNew;

  Json::FastWriter writer;
  CStdString data = writer.write(result);

  CSingleLock lock(m_lock);

  m_insertEventCmd->binder() << info.id << info.channelId << source << (sqlite3_int64) info.start << (sqlite3_int64) info.end << data;
  m_insertEventCmd->execute();
  m_insertEventCmd->reset();
}

void EpgStore::CleanOldEvents(void* transaction)
{
  time_t now = time(NULL);

  CSingleLock lock(m_lock);

  CStdString sql;
  sql.Format("DELETE FROM programs WHERE end_time < %lu", now);
  sqlite3pp::command cmd(*m_db, sql.c_str());
  cmd.execute();
}

void EpgStore::CleanEventsByTimeAndSource(void* transaction, const CStdString& source, const uint64_t start, const uint64_t end)
{
  CSingleLock lock(m_lock);

  CStdString sql;
  sql.Format("DELETE FROM programs WHERE start_time >= %llu AND end_time <= %llu AND source = '%s'", start, end, source.c_str());
  sqlite3pp::command cmd(*m_db, sql.c_str());
  cmd.execute();
}

void EpgStore::CleanEventsBySource(void* transaction, const CStdString& source)
{
  CSingleLock lock(m_lock);

  CStdString sql;
  sql.Format("DELETE FROM programs WHERE source = '%s'", source.c_str());
  sqlite3pp::command cmd(*m_db, sql.c_str());
  cmd.execute();
}

void EpgStore::FromJson(std::string strJson, EpgProgramInfo& epgInfo)
{
  if (strJson.length() == 0)
  {
    return;
  }

  Json::Value json;
  Json::Reader reader;
  if (!reader.parse(strJson, json))
  {
    CLog::Log(LOGERROR, "Error parsing json: %s", reader.getFormatedErrorMessages().c_str());
    return;
  }

  epgInfo.title = json["title"].asString();
  epgInfo.synopsis = json["synopsis"].asString();
  epgInfo.rating = json["rating"].asString();
  epgInfo.origAirDate = json["orig_airdate"].asString();
  epgInfo.seasonNumber = json["season_num"].asString();
  epgInfo.episodeNumber = json["episode_num"].asString();
  epgInfo.thumb = json["thumb"].asString();
  epgInfo.showRunNumber = json["show_num"].asString();
  epgInfo.episodeTitle = json["episode_title"].asString();
  epgInfo.isNew = json["new"].asBool();
}

std::vector<EpgProgramInfo> EpgStore::QueryByChannelForPrograms(const CStdString& channel_id, const uint64_t startSlot, const uint64_t endSlot)
{
  std::vector<EpgProgramInfo> result;

  try
  {
    CStdString queryString;
    queryString.Format(
        "SELECT id, channel_id, source, start_time, end_time, data "
        "FROM programs "
        "WHERE channel_id = '%s' AND start_time >= %llu AND start_time < %llu "
        "ORDER BY start_time", channel_id.c_str(), startSlot, endSlot
       );

    sqlite3pp::query qry(*m_db, queryString);

    std::string data;

    for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
    {
      EpgProgramInfo p;
      (*i).getter() >> p.id >> p.channelId >> p.sourceId >> p.start >> p.end >> data;
      FromJson(data, p);
      result.push_back(p);
    }
  }
  catch (std::exception& ex)
  {
    CLog::Log(LOGERROR,"EpgStore::QueryByChannelForPrograms - got exception: %s", ex.what());
  }

  return result;
}

std::vector<EpgProgramInfo> EpgStore::QueryAllChannelsForPrograms(const uint64_t timeSlotStart, const uint64_t timeSlotEnd)
{
  std::vector<EpgProgramInfo> result;

  try
  {
    CStdString queryString;
    queryString.Format(
        "SELECT id, channel_id, source, start_time, end_time, data "
        "FROM programs "
        "WHERE start_time < %llu AND end_time > %llu ",
        timeSlotEnd, timeSlotStart
       );

//    printf("QueryAllChannelsForPrograms: %s\n", queryString.c_str());

    sqlite3pp::query qry(*m_db, queryString);

    std::string data;

    for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
    {
      EpgProgramInfo p;
      (*i).getter() >> p.id >> p.channelId >> p.sourceId >> p.start >> p.end >> data;
      FromJson(data, p);
      result.push_back(p);
    }
  }
  catch (std::exception& ex)
  {
    CLog::Log(LOGERROR,"EpgStore::QueryAllChannelsForPrograms - got exception: %s", ex.what());
  }

  return result;
}

std::vector<EpgProgramInfo> EpgStore::QueryByChannelNow(const CStdString& channel_id)
{
  std::vector<EpgProgramInfo> result;

  try
  {
    time_t now = time(NULL);

    CStdString queryString;
    queryString.Format(
        "SELECT id, channel_id, source, start_time, end_time, data "
        "FROM programs "
        "WHERE start_time <= %lu AND end_time > %lu AND channel_id = '%s'",
        now, now, channel_id.c_str()
       );

    sqlite3pp::query qry(*m_db, queryString);

    std::string data;

    for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
    {
      EpgProgramInfo p;
      (*i).getter() >> p.id >> p.channelId >> p.sourceId >> p.start >> p.end >> data;
      FromJson(data, p);
      result.push_back(p);
    }
  }
  catch (std::exception& ex)
  {
    CLog::Log(LOGERROR,"EpgStore::QueryByChannelNow - got exception: %s", ex.what());
  }

  return result;
}

void EpgStore::SetSocialInfo(std::vector<EpgSocialInfo>* socialInfo)
{
  if (m_socialInfo)
    delete m_socialInfo;

  m_socialInfo = socialInfo;
}

void EpgStore::SetChannelSocialInfo(const CStdString& id, EpgSocialInfo& socialInfo)
{
  m_mapProgramSocialInfo[id] = socialInfo;
}

bool EpgStore::GetChannelSocialInfo(const CStdString& programId, const CStdString& serviceId, EpgSocialInfo& socialInfo)
{
  CStdString myProgramId;

  CStdStringArray splitProgramId;
  StringUtils::SplitString(programId, "-", splitProgramId);
  if (splitProgramId.size() > 0)
  {
    myProgramId = splitProgramId[0];
  }

  std::map<CStdString,EpgSocialInfo>::iterator it = m_mapProgramSocialInfo.find(myProgramId);

  if (it != m_mapProgramSocialInfo.end())
  {
    socialInfo = it->second;
    return true;
  }

  it = m_mapProgramSocialInfo.find(serviceId);

  if (it != m_mapProgramSocialInfo.end())
  {
    socialInfo = it->second;
    return true;
  }

  return false;
}

EpgStartEndTime EpgStore::QueryAllChannelsForTime(const uint64_t start, const uint64_t end)
{
  EpgStartEndTime result;

  try
  {
    int v1 = 0;
    int v2 = 0;

    CStdString queryString;
    queryString.Format(
        "SELECT MIN(start_time), MAX(start_time) "
        "FROM programs "
        "WHERE start_time >= %llu AND start_time < %llu ",
        start, end
       );

    sqlite3pp::query qry(*m_db, queryString);

    for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
    {
      (*i).getter() >> v1 >> v2;
    }

    result.start = v1;
    result.end = v2;
  }
  catch (std::exception& ex)
  {
    CLog::Log(LOGERROR,"EpgStore::QueryAllChannelsForTime - got exception: %s", ex.what());
  }

  return result;
}

EpgStartEndTime EpgStore::QueryByChannelForTime(const CStdString& channel_id, const uint64_t start, const uint64_t end)
{
  EpgStartEndTime result;

  try
  {
    int v1 = 0;
    int v2 = 0;

    CStdString queryString;
    queryString.Format(
       "SELECT MIN(start_time), MAX(start_time) "
       "FROM programs "
       "WHERE channel_id = '%s' AND start_time >= %llu AND start_time < %llu ",
       channel_id.c_str(), start, end
      );

    sqlite3pp::query qry(*m_db, queryString);

    for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
    {
      (*i).getter() >> v1 >> v2;
    }

    result.start = v1;
    result.end = v2;
  }
  catch (std::exception& ex)
  {
    CLog::Log(LOGERROR,"EpgStore::QueryByChannelForTime - got exception: %s", ex.what());
  }

  return result;
}

void EpgStore::Process()
{
  while(!m_bStop)
  {
    ::WaitForSingleObject(m_hEpgDataEvent, SYNC_CACHE_INTERVAL_SEC * 1000);

    void* tx = NULL;
    try
    {
      tx = EpgStore::GetInstance().StartEventsTransaction();
      CleanOldEvents(tx);
      EpgStore::GetInstance().CommitEventsTransaction(tx);
    }
    catch (std::exception& ex)
    {
      if (tx)
        EpgStore::GetInstance().RollbackEventsTransaction(tx);
    }

    {
      CSingleLock lock(m_lock);
      CopyFile(_P(DB_FILE_TMP), _P(DB_FILE), false);
    }
  }
}
#endif
