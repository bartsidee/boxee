#pragma once

#include "system.h"

#ifdef HAS_DVB

#include <vector>
#include "StdString.h"
#include "json/value.h"
#include "dvbchannel.h"
#include "SingleLock.h"

class DvbChannels
{
public:
  DvbChannels();
  virtual ~DvbChannels();

  void Clear();
  size_t Size();

  void Lock();
  void Unlock();

  bool LoadScannedChannels();
  bool SaveChannelsToServer();
  bool LoadChannelsFromServer();

  DvbChannelPtr GetChannelByIndex(size_t index);
  DvbChannelPtr GetChannelByServerId(const CStdString& serverId);

  DvbChannelPtr GetChannel(const CStdString &name);
  DvbChannelPtr GetChannelByDvbTriplet(int NetworkId, int TsId, int ServiceId);
  DvbChannelPtr GetChannel(int ServiceId);

  bool HasChannels();

private:
  void Sort();
  CCriticalSection m_lock;
  std::vector<DvbChannelPtr> m_channels;
  std::map<CStdString, DvbChannelPtr> m_channelsByServerId;

  bool FromJSon(const Json::Value& j);
  Json::Value ToJSon();
};

#endif
