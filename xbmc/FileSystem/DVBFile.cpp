#include "system.h"

#ifdef HAS_DVB

#include "DVBFile.h"
#include "Util.h"
#include "URL.h"
#include "FileFactory.h"
#include "utils/log.h"
#include "Application.h"
#include <boost/foreach.hpp>

using namespace XFILE;
using namespace std;

CDVBFile::CDVBFile()
{
  m_bOpened = false;
}

CDVBFile::~CDVBFile()
{
  Close();
}
void CDVBFile::Close()
{
  std::vector<DvbTunerPtr> tuners = DVBManager::GetInstance().GetTuners();
  if (tuners.size() == 0)
  {
    CLog::Log(LOGERROR, "CDVBFile::Close no tuners found");
    return;
  }

  DvbTunerPtr tuner = tuners[0];
  tuner->Untune();

  m_bOpened = false;
}

bool CDVBFile::Open(const CURI& url)
{
  if (m_bOpened)
    Close();

  if (url.GetProtocol() != "dvb")
  {
    CLog::Log(LOGERROR, "CDVBFile::Open - invalid protocol %s", url.GetProtocol().c_str());
    return false;
  }

  std::vector<DvbTunerPtr> tuners = DVBManager::GetInstance().GetTuners();
  if (tuners.size() == 0)
  {
    CLog::Log(LOGERROR, "CDVBFile::Open no tuners found");
    return false;
  }

  DvbTunerPtr tuner = tuners[0];

  DvbChannelPtr channel = DVBManager::GetInstance().GetCurrentChannel();
  if (channel.get() == NULL)
  {
    CLog::Log(LOGERROR, "CDVBFile::Open - failed to get channel from DVB manager");
    return false;
  }

  tuner->Tune(*channel);

  m_bOpened = true;

  return true;
}

unsigned int CDVBFile::Read(void* buffer, int64_t size)
{
  if (!m_bOpened) return -1;

  std::vector<DvbTunerPtr> tuners = DVBManager::GetInstance().GetTuners();
  if (tuners.size() == 0)
  {
    CLog::Log(LOGERROR, "CDVBFile::Open no tuners found");
    return false;
  }

  DvbTunerPtr tuner = tuners[0];

  return tuner->Read((char *)buffer, size);
}

int64_t CDVBFile::Seek(int64_t pos, int whence)
{
  return -1;
}

bool CDVBFile::NextChannel()
{
  return true;
}

bool CDVBFile::PrevChannel()
{
  return true;
}

bool CDVBFile::SelectChannel(unsigned int channelIndex)
{
  DvbChannels& channels = DVBManager::GetInstance().GetChannels();
  if (channelIndex > channels.Size() - 1 )
  {
    CLog::Log(LOGERROR, "CDVBFile::SelectChannel no such channel %d", channelIndex);
    return false;
  }

  DvbChannelPtr channelPtr = channels.GetChannelByIndex(channelIndex);
  if (channelPtr.get() == NULL)
  {
    CLog::Log(LOGERROR, "CDVBFile::SelectChannel - failed to get channel info for %d", channelIndex);
    return false;
  }

  std::vector<DvbTunerPtr> tuners = DVBManager::GetInstance().GetTuners();
  if (tuners.size() == 0)
  {
    CLog::Log(LOGERROR, "CDVBFile::SelectChannel no tuners found");
    return false;
  }

  DvbTunerPtr tuner = tuners[0];

  if (!tuner->Tune(*channelPtr))
  {
    CLog::Log(LOGERROR, "CDVBFile::SelectChannel - failed to tune to %d", channelIndex);
    return false;
  }

  return true;
}

int CDVBFile::GetProgramId()
{
  std::vector<DvbTunerPtr> tuners = DVBManager::GetInstance().GetTuners();
  if (tuners.size() == 0)
  {
    CLog::Log(LOGERROR, "CDVBFile::GetProgramId no tuners found");
    return 0;
  }

  DvbTunerPtr tuner = tuners[0];

  DvbChannelPtr curChannel = DVBManager::GetInstance().GetCurrentChannel();
  return curChannel->GetIndex();
}

#endif
