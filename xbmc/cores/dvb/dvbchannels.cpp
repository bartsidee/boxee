#include "dvbchannels.h"

#ifdef HAS_DVB

#include <boost/foreach.hpp>

#include "FileSystem/File.h"
#include "utils/log.h"
#include "bxconfiguration.h"
#include "bxutils.h"
#include "GUISettings.h"
#include "Application.h"

#define SCANNED_CHANNELS_FILE "special://home/dvb/channels.conf"
#define SCANNED_CHANNELS_SERVER_FILE "special://home/dvb/channels.json"

DvbChannels::DvbChannels()
{
}

DvbChannels::~DvbChannels()
{
}

bool DvbChannels::LoadScannedChannels()
{
  CSingleLock lock(m_lock);

  XFILE::CFile channelsFile;
  if (!XFILE::CFile::Exists(SCANNED_CHANNELS_FILE))
  {
    return false;
  }

  CLog::Log(LOGDEBUG, "DVBManager: has channels file. loading channels");

  if (!channelsFile.Open(SCANNED_CHANNELS_FILE))
  {
    CLog::Log(LOGWARNING,"DVBManager: cant open channels file. erasing it");
    XFILE::CFile::Delete(SCANNED_CHANNELS_FILE);
    return false;
  }

  m_channels.clear();
  char line[1024];
  int counter = 0;
  while (channelsFile.ReadString(line, sizeof(line)))
  {
    CLog::Log(LOGDEBUG,  "DVBManager: channels.conf line: %s", line);

    DvbChannelPtr channel(new DvbChannel(counter));
    if (channel->FromConfRecord(line))
    {
      CLog::Log(LOGDEBUG,  "DVBManager: adding channel %s", channel->GetServiceName().c_str());
      m_channels.push_back(channel);

      if (channel->GetServerId())
        m_channelsByServerId[channel->GetServerId()] = channel;
    }
    counter++;
  }

  channelsFile.Close();

  Sort();

  return true;
}

bool DvbChannels::HasChannels()
{
  CSingleLock lock(m_lock);

  return m_channels.size() > 0;
}


bool DvbChannels::SaveChannelsToServer()
{
#ifdef HAS_SERVER_OTA
  CSingleLock lock(m_lock);

  CStdString strUrl = BOXEE::BXConfiguration::GetInstance().GetURLParam("Boxee.ApiChannels", "http://app.boxee.tv/epg/user-channel");
  Json::Value jResponse;
  int returnCode;

  Json::Value jBody = ToJSon();

#if 0
  Json::StyledWriter writer2;
  std::string strPostData = writer2.write(jBody);
  printf("post: %s\n", strPostData.c_str());
#endif

  bool result = BOXEE::BXUtils::PerformJSONPostRequest(strUrl, jBody, jResponse, returnCode);
  if (!result)
  {
    return false;
  }

  // Cache server response for offline mode
  Json::StyledWriter writer;
  std::string channelsJson = writer.write(jResponse);

  XFILE::CFile channelsFile;
  if (!channelsFile.OpenForWrite(SCANNED_CHANNELS_SERVER_FILE, true))
  {
    CLog::Log(LOGWARNING,"DvbChannels::SaveChannelsToServer: can't open channels file");
    return false;
  }
  channelsFile.Write(channelsJson.c_str(), channelsJson.size());
  channelsFile.Close();

#if 0
  printf("got: %s\n", channelsJson.c_str());
#endif

  if (!FromJSon(jResponse))
  {
    return false;
  }

  Sort();
#endif

  return true;
}

bool DvbChannels::LoadChannelsFromServer()
{
#ifdef HAS_SERVER_OTA
  CSingleLock lock(m_lock);

  Json::Value jResponse;

  if (g_application.IsConnectedToInternet(false))
  {
    CStdString strUrl = BOXEE::BXConfiguration::GetInstance().GetURLParam("Boxee.ApiChannels", "http://app.boxee.tv/epg/user-channel");
    int returnCode;

    bool result = BOXEE::BXUtils::PerformJSONGetRequest(strUrl, jResponse, returnCode);
    if (!result)
    {
      return false;
    }
  }
  else
  {
    CLog::Log(LOGINFO, "DvbChannels::LoadChannelsFromServer: no internet, loading local list");

    XFILE::CFile channelsFile;
    if (!XFILE::CFile::Exists(SCANNED_CHANNELS_SERVER_FILE))
    {
      CLog::Log(LOGERROR, "DvbChannels::LoadChannelsFromServer: can't find channels file");
      return false;
    }

    if (!channelsFile.Open(SCANNED_CHANNELS_SERVER_FILE))
    {
      CLog::Log(LOGERROR, "DvbChannels::LoadChannelsFromServer: can't open channels file");
      return false;
    }

    int64_t fileSize = channelsFile.GetLength();
    char* jsonStr = new char[fileSize];
    if (channelsFile.Read(jsonStr, fileSize) != fileSize)
    {
      delete [] jsonStr;
      CLog::Log(LOGWARNING,"DvbChannels::LoadChannelsFromServer: can't read channels file");
      return false;
    }

    std::string jsonDoc = jsonStr;
    delete jsonStr;

    Json::Reader reader;
    if (!reader.parse(jsonDoc, jResponse))
    {
      CLog::Log(LOGERROR, "DvbChannels::LoadChannelsFromServer: Failed to parse channels file. Error: %s", reader.getFormatedErrorMessages().c_str());
     return false;
    }
  }

#if 0
  Json::StyledWriter writer2;
  std::string strPostData2 = writer2.write(jResponse);
  printf("got: %s\n", strPostData2.c_str());
#endif

  if (!FromJSon(jResponse))
  {
    return false;
  }

  Sort();
#endif

  return true;
}

bool DvbChannels::FromJSon(const Json::Value& j)
{
  if (j["channels"].isNull() && !j["channels"].isArray())
    return false;

  if (j["channels"].size() == 0)
    return true;

  m_channels.clear();

  int counter = 0;
  for (Json::ArrayIndex i = 0; i < j["channels"].size(); i++)
  {
    DvbChannelPtr channel(new DvbChannel(counter));
    if (channel->FromJson(j["channels"][i]))
    {
      CLog::Log(LOGDEBUG,  "DVBManager: adding channel %s", channel->GetServiceName().c_str());
      m_channels.push_back(channel);
      if (channel->GetServerId())
        m_channelsByServerId[channel->GetServerId()] = channel;
    }
    counter++;
  }

  return true;
}

Json::Value DvbChannels::ToJSon()
{
  Json::Value result;
  result["country"] = g_guiSettings.GetString("ota.countrycode");

  if (!g_guiSettings.GetString("ota.zipcode").IsEmpty())
    result["postal_code"] = g_guiSettings.GetString("ota.zipcode");

  if (g_guiSettings.GetString("ota.countrycode") == "US" ||
      g_guiSettings.GetString("ota.countrycode") == "CA")
  {
    if (g_guiSettings.GetBool("ota.selectedcable"))
      result["modulation"] = "qam";
    else
      result["modulation"] = "vsb";
  }
  else
  {
    result["modulation"] = "ofdm";
  }

  Json::Value channels;
  for (size_t i = 0; i < m_channels.size(); i++)
  {
    channels.append(m_channels[i]->ToJson());
  }
  result["channels"] = channels;

  return result;
}

size_t DvbChannels::Size()
{
  CSingleLock lock(m_lock);

  return m_channels.size();
}

DvbChannelPtr DvbChannels::GetChannelByIndex(size_t index)
{
  CSingleLock lock(m_lock);
  if (index > m_channels.size())
  {
    return DvbChannelPtr();
  }

  return m_channels[index];
}

DvbChannelPtr DvbChannels::GetChannelByServerId(const CStdString& serverId)
{
  CSingleLock lock(m_lock);

  if (m_channelsByServerId.find(serverId) == m_channelsByServerId.end())
    return DvbChannelPtr();

  return m_channelsByServerId.find(serverId)->second;
}

DvbChannelPtr DvbChannels::GetChannel(const CStdString &name)
{
  CSingleLock lock(m_lock);

  foreach (DvbChannelPtr channel, m_channels)
  {
    if (channel->GetServiceName() == name || channel->GetVirtualChannel() == name || channel->GetServerId() == name)
      return channel;
  }

  return DvbChannelPtr();
}

DvbChannelPtr DvbChannels::GetChannelByDvbTriplet(int NetworkId, int TsId, int ServiceId)
{
  CSingleLock lock(m_lock);

  foreach (DvbChannelPtr channel, m_channels)
  {
    if (channel->GetOriginalNetworkId() == NetworkId &&
        channel->GetTransportStreamId() == TsId &&
        channel->GetServiceId() == ServiceId)
      return channel;
  }

  return DvbChannelPtr();
}

DvbChannelPtr DvbChannels::GetChannel(int ServiceId)
{
  CSingleLock lock(m_lock);

  foreach (DvbChannelPtr channel, m_channels)
  {
    if (channel->GetServiceId() == ServiceId)
      return channel;
  }

  return DvbChannelPtr();
}


void DvbChannels::Clear()
{
  CSingleLock lock(m_lock);
  m_channels.clear();
}

void DvbChannels::Lock()
{
  EnterCriticalSection(m_lock);
}

void DvbChannels::Unlock()
{
  ExitCriticalSection(m_lock);
}

static bool ChannelComparator(const DvbChannelPtr &left, const DvbChannelPtr &right)
{
  // Sort by channel number
  if (!left->GetChannelNumber().IsEmpty() && !right->GetChannelNumber().IsEmpty())
      return (CUtil::VersionCompare(left->GetChannelNumber(), right->GetChannelNumber()) < 0);
  // Channels with numbers should be first
  else if (!left->GetChannelNumber().IsEmpty() && right->GetChannelNumber().IsEmpty())
    return true;
  else if (left->GetChannelNumber().IsEmpty() && !right->GetChannelNumber().IsEmpty())
    return false;
  // Otherwise, compare by string
  else
    return (left->GetChannelLabel().compare(right->GetChannelLabel()) < 0);
}

void DvbChannels::Sort()
{
  std::sort(m_channels.begin(), m_channels.end(), ChannelComparator);

  for (size_t i = 0; i < m_channels.size(); i++)
  {
    m_channels[i]->SetIndex(i);
  }
}

#endif
