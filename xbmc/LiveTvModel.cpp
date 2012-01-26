#include "LiveTvModel.h"

#ifdef HAS_DVB

#include "dvbmanager.h"
#include "dvbchannels.h"
#include "dvbchannel.h"
#include "epgstore.h"
#include "FileItem.h"
#include "utils/log.h"

#include <boost/foreach.hpp>
#include "Util.h"
#include "../guilib/LocalizeStrings.h"

// Time slot is 30 minutes
#define TIME_SLOT_DURATION 30

LiveTvModel::LiveTvModel()
{
  SetCurrentTimeSlot(0);
}

LiveTvModelChannelsType LiveTvModel::GetChannels()
{
  LiveTvModelChannelsType result;
  DvbChannels& channels = DVBManager::GetInstance().GetChannels();
  channels.Lock();

  for (size_t i = 0; i < channels.Size(); i++)
  {
    DvbChannelPtr dvbChannel = channels.GetChannelByIndex(i);

    LiveTvModelChannel channel;
    channel.id = i;
    channel.enabled = dvbChannel->IsEnabled();
    channel.label = dvbChannel->GetChannelLabel();
    channel.number = dvbChannel->GetChannelNumber();
    result.push_back(channel);
  }

  channels.Unlock();

  return result;
}

LiveTvModelProgramsType LiveTvModel::GetPrograms()
{
  return GetProgramsForSlot(m_currentStartTimeSlot);
}

LiveTvModelProgramsType LiveTvModel::GetProgramsForSlot(time_t slot)
{
  time_t currentTime = time(NULL);

  time_t realTimeSlot = slot;
  if (realTimeSlot == 0)
    realTimeSlot = currentTime;
  time_t endTimeSlot = CalculateEndTimeSlot(slot);

  LiveTvModelProgramsType result;
  for (size_t i = 0; i < DVBManager::GetInstance().GetChannels().Size(); i++)
  {
    std::vector<LiveTvModelProgram> pgms;
    result.push_back(pgms);
  }

  std::vector<EpgProgramInfo> epgInfo = EpgStore::GetInstance().QueryAllChannelsForPrograms(realTimeSlot, endTimeSlot);

  foreach (EpgProgramInfo epgProgram, epgInfo)
  {
    DvbChannelPtr channel = DVBManager::GetInstance().GetChannels().GetChannel(epgProgram.channelId);
    if (channel.get() == 0)
      continue;

    LiveTvModelProgram program;
    program.info = epgProgram;
    program.isNow = (program.info.start <= currentTime);

//    printf("loading program: serverid=[%s] channel=[%s] title=[%s] isNow=[%d] start=[%lu] end=[%lu] slot=[%lu]\n",
//        channel->GetChannelId().c_str(), channel->GetChannelLabel().c_str(), epgProgram.title.c_str(), program.isNow,
//        program.start, program.end, realTimeSlot);

    if (program.isNow)
    {
      EpgStore::GetInstance().GetChannelSocialInfo(epgProgram.id, channel->GetServiceName(), program.social);
    }

    result[channel->GetIndex()].push_back(program);
  }

  return result;
}

void LiveTvModel::GetCurrentTimeSlot(time_t& startSlot, time_t& endSlot)
{
  startSlot = m_currentStartTimeSlot;
  endSlot = m_currentEndTimeSlot;
}

bool LiveTvModel::IsCurrentTimeSlotFirst()
{
  return (m_currentStartTimeSlot == 0);
}

void LiveTvModel::SetCurrentTimeSlot(const time_t& slot)
{
  m_currentStartTimeSlot = slot;
  m_currentEndTimeSlot = CalculateEndTimeSlot(m_currentStartTimeSlot);
}

time_t LiveTvModel::CalculateEndTimeSlot(time_t& startSlot)
{
  time_t realTimeSlot = startSlot;
  if (realTimeSlot == 0)
    realTimeSlot = time(NULL);

  CDateTimeSpan nextSlotSpan(0, 0, TIME_SLOT_DURATION, 0);
  CDateTime endDateTime(realTimeSlot);
  endDateTime += nextSlotSpan;

  // Zero the seconds, if needed
  if (endDateTime.GetSecond())
  {
    CDateTimeSpan removeSecondsTimeSpan(0, 0, 0, endDateTime.GetSecond());
    endDateTime -= removeSecondsTimeSpan;
  }

  if (endDateTime.GetMinute() != 0 && endDateTime.GetMinute() != 30)
  {
    int minutesToReduce = endDateTime.GetMinute();
    if (minutesToReduce > 30)
      minutesToReduce -= 30;
    CDateTimeSpan removeSecondsTimeSpan(0, 0, minutesToReduce, 0);
    endDateTime -= removeSecondsTimeSpan;
  }

  time_t result;
  endDateTime.GetAsTime(result);

  return result;
}

bool LiveTvModel::SetNextTimeSlot()
{
  time_t startSlot, endSlot;
  GetCurrentTimeSlot(startSlot, endSlot);

  if (endSlot >= time(NULL) + (24 * 60 * 60))
    return false;

  LiveTvModelProgramsType programs = GetProgramsForSlot(endSlot);
  for (size_t i = 0; i < programs.size(); i++)
  {
    // Make sure we have at least 1 program in the list
    if (programs[i].size() > 0)
    {
      SetCurrentTimeSlot(endSlot);
      return true;
    }
  }

  return false;
}

bool LiveTvModel::HasNextTimeSlot()
{
  time_t startSlot, endSlot;
  GetCurrentTimeSlot(startSlot, endSlot);

  if (endSlot >= time(NULL) + (24 * 60 * 60))
    return false;

  LiveTvModelProgramsType programs = GetProgramsForSlot(endSlot);
  for (size_t i = 0; i < programs.size(); i++)
  {
    // Make sure we have at least 1 program in the list
    if (programs[i].size() > 0)
    {
      return true;
    }
  }

  return false;
}

bool LiveTvModel::SetPrevTimeSlot()
{
  if (IsCurrentTimeSlotFirst())
    return false;

  time_t prevSlot;
  CDateTimeSpan prevSlotSpan(0, 0, TIME_SLOT_DURATION, 0);
  CDateTime dateTime(m_currentStartTimeSlot);
  dateTime -= prevSlotSpan;
  dateTime.GetAsTime(prevSlot);

  if (prevSlot < time(NULL))
  {
    SetCurrentTimeSlot(0);
  }
  else
  {
    SetCurrentTimeSlot(prevSlot);
  }

  return true;
}

bool LiveTvModel::HasPrevTimeSlot()
{
  if (IsCurrentTimeSlotFirst())
    return false;

  return true;
}

#endif
