#pragma once

#include "system.h"
#include "sqlite3.h"
#include "Thread.h"

#ifdef HAS_DVB

namespace sqlite3pp
{
  class database;
  class command;
}

#define EPG_SOURCE_ID_SERVER "srv"
#define EPG_SOURCE_ID_OTA    "ota"

class EpgProgramInfo
{
public:
  EpgProgramInfo() { Clear(); }
  CStdString id;
  CStdString channelId;
  CStdString sourceId;
  sqlite3_int64 start;
  sqlite3_int64 end;
  CStdString title;
  CStdString synopsis;
  CStdString rating;
  bool isNew;
  CStdString origAirDate;
  CStdString seasonNumber;
  CStdString episodeNumber;
  CStdString thumb;
  CStdString showRunNumber;
  CStdString episodeTitle;

  bool IsInfoFromServer() { return sourceId == EPG_SOURCE_ID_SERVER; }

  bool IsSet()
  {
    return (start != 0 && end != 0);
  }

  void Clear()
  {
    start = 0;
    end = 0;
    isNew = false;
  }
};

struct EpgFriendInfo
{
  CStdString user_id;
  CStdString name;
  CStdString shortName;
  CStdString thumbUrl;
  CStdString thumbSmallUrl;
  CStdString firstName;
  CStdString lastName;
};

struct EpgSocialInfo
{
  EpgSocialInfo()
  { Clear(); }

  void Clear()
  {
    watching = 0;
    friends.clear();
    label = "";
  }

  int watching;
  std::vector<EpgFriendInfo> friends;
  CStdString label;
};

struct EpgStartEndTime
{
  time_t start;
  time_t end;
};

class EpgStore : public CThread
{
public:
  static EpgStore &GetInstance();
  virtual ~EpgStore();

  void CreateTables();
  void* StartEventsTransaction();
  void CommitEventsTransaction(void* transaction);
  void InsertEvent(void* transaction, CStdString source, EpgProgramInfo info);
  void RollbackEventsTransaction(void* transaction);
  void CleanOldEvents(void* transaction);
  void CleanEventsByTimeAndSource(void* transaction, const CStdString& source, const uint64_t start, const uint64_t end);
  void CleanEventsBySource(void* transaction, const CStdString& source);

  std::vector<EpgProgramInfo> QueryAllChannelsForPrograms(const uint64_t timeSlotStart, const uint64_t timeSlotEnd);
  std::vector<EpgProgramInfo> QueryByChannelForPrograms(const CStdString& channel_id, const uint64_t start, const uint64_t end);

  std::vector<EpgProgramInfo> QueryByChannelNow(const CStdString& channel_id);

  EpgStartEndTime QueryAllChannelsForTime(const uint64_t start, const uint64_t end);
  EpgStartEndTime QueryByChannelForTime(const CStdString& channel_id, const uint64_t start, const uint64_t end);

  void SetSocialInfo(std::vector<EpgSocialInfo>* socialInfo);
  void SetChannelSocialInfo(const CStdString& id, EpgSocialInfo& socialInfo);
  bool GetChannelSocialInfo(const CStdString& programId, const CStdString& serviceId, EpgSocialInfo& socialInfo);

  virtual void Process();

private:
  EpgStore();
  void FromJson(std::string json, EpgProgramInfo& epgInfo);
  CStdString CleanDescription(const CStdString& strDescription);

  std::vector<EpgSocialInfo>* m_socialInfo;
  std::map<CStdString,EpgSocialInfo> m_mapProgramSocialInfo;
  sqlite3pp::database* m_db;
  sqlite3pp::command* m_insertEventCmd;

  HANDLE m_hEpgDataEvent;
  CCriticalSection m_lock;
};


#endif
