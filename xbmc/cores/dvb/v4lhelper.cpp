/*
 * All Rights Reserved, Boxee.tv
 */

#include "v4lhelper.h"

#if defined(HAS_DVB) && defined(_LINUX) && !defined(__APPLE__)

#include <linux/dvb/dmx.h>
#include <linux/dvb/version.h>
#include <linux/dvb/frontend.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/poll.h>
#include <string.h>

#include "StringUtils.h"
#include "SingleLock.h"
#include "utils/log.h"
#include "Util.h"

V4LHelper::V4LHelper(int tunerId, int flags) : IDvbHelper(tunerId, flags)
{
  m_frontendHandle = -1;
  m_dvr = -1;
  m_tuningState = DvbTuner::TUNING_STATE_IDLE;
  m_lastSignalCheckTicks = 0;

  m_type = DvbTuner::TUNER_TYPE_UNINITIALIZED;
  m_tunerId = tunerId;

  m_trValue["INVERSION_OFF"] = INVERSION_OFF;
  m_trValue["INVERSION_ON"] = INVERSION_ON;
  m_trValue["INVERSION_AUTO"] = INVERSION_AUTO;

  m_trValue["BANDWIDTH_6_MHZ"] = BANDWIDTH_6_MHZ;
  m_trValue["BANDWIDTH_7_MHZ"] = BANDWIDTH_7_MHZ;
  m_trValue["BANDWIDTH_8_MHZ"] = BANDWIDTH_8_MHZ;

  m_trValue["FEC_1_2"] = FEC_1_2;
  m_trValue["FEC_2_3"] = FEC_2_3;
  m_trValue["FEC_3_4"] = FEC_3_4;
  m_trValue["FEC_4_5"] = FEC_4_5;
  m_trValue["FEC_5_6"] = FEC_5_6;
  m_trValue["FEC_6_7"] = FEC_6_7;
  m_trValue["FEC_7_8"] = FEC_7_8;
  m_trValue["FEC_8_9"] = FEC_8_9;
  m_trValue["FEC_AUTO"] = FEC_AUTO;
  m_trValue["FEC_NONE"] = FEC_NONE;

  m_trValue["GUARD_INTERVAL_1_16"] = GUARD_INTERVAL_1_16;
  m_trValue["GUARD_INTERVAL_1_16"] = GUARD_INTERVAL_1_32;
  m_trValue["GUARD_INTERVAL_1_4"] = GUARD_INTERVAL_1_4;
  m_trValue["GUARD_INTERVAL_1_8"] = GUARD_INTERVAL_1_8;
  m_trValue["GUARD_INTERVAL_AUTO"] = GUARD_INTERVAL_AUTO;

  m_trValue["HIERARCHY_1"] = HIERARCHY_1;
  m_trValue["HIERARCHY_2"] = HIERARCHY_2;
  m_trValue["HIERARCHY_4"] = HIERARCHY_4;
  m_trValue["HIERARCHY_NONE"] = HIERARCHY_NONE;
  m_trValue["HIERARCHY_AUTO"] = HIERARCHY_AUTO;

  m_trValue["QPSK"] = QPSK;
  m_trValue["QAM_128"] = QAM_128;
  m_trValue["QAM_16"] = QAM_16;
  m_trValue["QAM_256"] = QAM_256;
  m_trValue["QAM_32"] = QAM_32;
  m_trValue["QAM_64"] = QAM_64;
  m_trValue["QAM_AUTO"] = QAM_AUTO;

  m_trValue["TRANSMISSION_MODE_2K"] = TRANSMISSION_MODE_2K;
  m_trValue["TRANSMISSION_MODE_8K"] = TRANSMISSION_MODE_8K;
  m_trValue["TRANSMISSION_MODE_AUTO"] = TRANSMISSION_MODE_AUTO;

  // ATSC
  m_trValue["8VSB"] = VSB_8;
  m_trValue["VSB_8"] = VSB_8;
  m_trValue["16VSB"] = VSB_16;
  m_trValue["VSB_16"] = VSB_16;
  m_trValue["QAM_64"] = QAM_64;
  m_trValue["QAM_256"] = QAM_256;

  memset(&m_frontend_settings, 0, sizeof(struct dvb_frontend_parameters));

  m_nFlags = flags;
}

V4LHelper::~V4LHelper()
{
  Stop();
}

bool V4LHelper::OpenFrontEnd()
{
  if (m_frontendHandle > 0)
    return true;

  CLog::Log(LOGINFO, "Opening Front End");

  CStdString frontendPath;
  frontendPath.Format("/dev/dvb/adapter%d/frontend0", m_tunerId);

  m_frontendHandle = open(frontendPath.c_str(), O_RDWR);
  if (m_frontendHandle < 0)
  {
    CLog::Log(LOGERROR, "V4LHelper::Open: failed to open frontend! %s",
        strerror(errno));
    return false;
  }

  if (fcntl(m_frontendHandle, F_SETFL, O_NONBLOCK))
  {
    CLog::Log(LOGWARNING,
        "V4LHelper::Open: failed set O_NONBLOCK on m_frontendHandle device");
    return false;
  }

  GetTunerType(m_tunerId);

  return true;
}

void V4LHelper::CloseFrontEnd()
{
  if (m_frontendHandle < 0)
    return;

  CLog::Log(LOGINFO, "Closing Front End");

  close(m_frontendHandle);
  m_frontendHandle = -1;
}

bool V4LHelper::OpenDVR()
{
  if (m_dvr > 0)
    return true;

  bool bBudgetMode  = m_nFlags & TUNER_FLAGS_BUDGET;

  CLog::Log(LOGINFO, "V4LHelper::OpenDVR. budget mode %d", bBudgetMode);

  CStdString dvrPath;
  if(bBudgetMode)
    dvrPath.Format("/dev/dvb/adapter%d/demux0", m_tunerId);
  else
    dvrPath.Format("/dev/dvb/adapter%d/dvr0", m_tunerId);

  m_dvr = open(dvrPath.c_str(), O_RDONLY);
  if (m_dvr < 0)
  {
    CLog::Log(LOGERROR, "V4LHelper::OpenDVR: failed to open dvr! %s", strerror(
        errno));
    return false;
  }

  if (fcntl(m_dvr, F_SETFL, O_NONBLOCK))
  {
    CLog::Log(LOGWARNING,
        "V4LHelper::Open: failed set O_NONBLOCK on dvr device");
  }

  if(bBudgetMode)
  {
    if (ioctl(m_dvr, DMX_SET_BUFFER_SIZE, 1 << 20) < 0)
    {
      CLog::Log(LOGERROR, "V4LHelper::OpenDVR: cannot expand demultiplexing buffer %s",
        strerror(errno));
      return false;
    }

    struct dmx_pes_filter_params param;

    param.pid = 0x2000;
    param.input = DMX_IN_FRONTEND;
    param.output = DMX_OUT_TSDEMUX_TAP;
    param.pes_type = DMX_PES_OTHER;
    param.flags = DMX_IMMEDIATE_START;
    if (ioctl(m_dvr, DMX_SET_PES_FILTER, &param) < 0)
    {
      CLog::Log(LOGERROR, "V4LHelper::OpenDVR: cannot setup TS demultiplexer %s",
            strerror(errno));
        return false;
    }
  }

  return true;
}

void V4LHelper::CloseDVR()
{
  if (m_dvr < 0)
    return;

  CLog::Log(LOGINFO, "Closing DVR");

  close(m_dvr);
  m_dvr = -1;
}

void V4LHelper::RemoveAllPidFilters()
{
  bool bBudgetMode  = m_nFlags & TUNER_FLAGS_BUDGET;

  if(bBudgetMode)
  {
    // in budget mode we only use PID 0x200,
    // so we can just close the DVR
    CloseDVR();
    Sleep(100);
    OpenDVR();
    return;
  }

  if(m_pidHandles.size() == 0)
    return;

  for(std::map<int, int>::const_iterator itr = m_pidHandles.begin(); itr != m_pidHandles.end(); ++itr)
  {
    ClosePID(itr->first);
  }

  m_pidHandles.clear();

  if(m_pidHandles.size() == 0)
  {
    Sleep(1000);
  }
}

void V4LHelper::CloseAll()
{
  CSingleLock lock(m_lock);

  RemoveAllPidFilters();
  CloseFrontEnd();
  CloseDVR();
}

DvbTuner::TunerType V4LHelper::GetTunerType(int tunerId)
{
  CSingleLock lock(m_lock);

  if (m_type != DvbTuner::TUNER_TYPE_UNINITIALIZED)
    return m_type;

  m_type = DvbTuner::TUNER_TYPE_UNKNOWN;

  //
  // query frontend for its type
  //
  struct dvb_frontend_info fe_info;
  CStdString frontendPath;
  frontendPath.Format("/dev/dvb/adapter%d/frontend0", tunerId);
  int fd = m_frontendHandle;
  if (fd == -1)
    fd = open(frontendPath.c_str(), O_RDONLY);

  if (fd < 0)
  {
    CLog::Log(LOGERROR, "V4LHelper::GetTunerType: failed to open frontend!");
    return m_type;
  }

  int rc = ioctl(fd, FE_GET_INFO, &fe_info);
  if (rc < 0)
  {
    CLog::Log(LOGERROR, "V4LHelper::GetTunerType: failed to get frontend info!");
    if (m_frontendHandle < 0)
      close(fd);
    return m_type;
  }

  CLog::Log(LOGINFO, "V4LHelper::GetTunerType: detected frontend (%s) type %d",
      fe_info.name, fe_info.type);
  if (fe_info.type == FE_ATSC)
    m_type = DvbTuner::TUNER_TYPE_ATSC;
  else if (fe_info.type == FE_OFDM)
    m_type = DvbTuner::TUNER_TYPE_DVBT;
  else
    CLog::Log(LOGERROR,
        "V4LHelper::GetTunerType: unknown frontend type: %s %d", fe_info.name,
        fe_info.type);

  if (m_frontendHandle < 0)
    close(fd);

  return m_type;
}

bool V4LHelper::Tune(const DvbChannel &channel)
{
  bool bNoAudio = false;
  bool bNoVideo = false;

  CSingleLock lock(m_lock);

  CLog::Log(LOGDEBUG, "V4LHelper::Tune: about to tune into %s",
      channel.GetServiceName().c_str());

  // Sanity check on the requested channel
  if (channel.GetVideoPid() == 0)
  {
    CLog::Log(LOGWARNING, "V4LHelper::Tune: no video pid");
    bNoVideo = true;
  }

  const std::vector<DvbAudioInfo>& audioInfo = channel.GetAudioPids();
  if (audioInfo.size() == 0 || audioInfo[0].pid == 0)
  {
    CLog::Log(LOGWARNING, "V4LHelper::Tune: no audio pid");
    bNoAudio = true;
  }

  if(bNoAudio && bNoVideo)
  {
    CLog::Log(LOGERROR, "V4LHelper::Tune: no audio or video pids found. Aborting tune");
    m_tuningState = DvbTuner::TUNING_STATE_TUNE_FAILED;
    return false;
  }

  // Open Front End if needed
  if (m_frontendHandle <= 0)
    OpenFrontEnd();

  // Remove old pids, if they exist
  bool bBudgetMode = m_nFlags & TUNER_FLAGS_BUDGET;

  if(!bBudgetMode)
    RemoveAllPidFilters();

  m_signalOk = true;
  bool bForceTune = m_nFlags & TUNER_FLAGS_FORCE_TUNE;

  if (m_lastFreq != channel.GetFreq() || bForceTune)
  {
    m_tuningState = DvbTuner::TUNING_STATE_TUNING;

    CloseDVR();

    struct dvb_frontend_parameters frontend;
    memset(&frontend, 0, sizeof(struct dvb_frontend_parameters));

    frontend.frequency = atoi(channel.GetFreq().c_str());

    if (m_type == DvbTuner::TUNER_TYPE_ATSC)
    {
      frontend.u.vsb.modulation = (fe_modulation_t) m_trValue[channel.GetTuningParams()];
    }
    else if (m_type == DvbTuner::TUNER_TYPE_DVBT)
    {
      CStdStringArray tuningParams;
      StringUtils::SplitString(channel.GetTuningParams(), ",", tuningParams);

      frontend.inversion = (fe_spectral_inversion_t) m_trValue[tuningParams[0]];
      frontend.u.ofdm.bandwidth = (fe_bandwidth_t) m_trValue[tuningParams[1]];
      frontend.u.ofdm.code_rate_HP
          = (fe_code_rate_t) m_trValue[tuningParams[2]];
      frontend.u.ofdm.code_rate_LP
          = (fe_code_rate_t) m_trValue[tuningParams[3]];
      frontend.u.ofdm.constellation
          = (fe_modulation_t) m_trValue[tuningParams[4]];
      frontend.u.ofdm.transmission_mode
          = (fe_transmit_mode_t) m_trValue[tuningParams[5]];
      frontend.u.ofdm.guard_interval
          = (fe_guard_interval_t) m_trValue[tuningParams[6]];
      frontend.u.ofdm.hierarchy_information
          = (fe_hierarchy_t) m_trValue[tuningParams[7]];
      if (frontend.u.ofdm.code_rate_HP == FEC_NONE)
        frontend.u.ofdm.code_rate_HP = FEC_AUTO;
      if (frontend.u.ofdm.code_rate_LP == FEC_NONE)
        frontend.u.ofdm.code_rate_LP = FEC_AUTO;
    }
    else
    {
      CLog::Log(LOGERROR,
          "V4LHelper::Tune: unknown format type. tuning failed.");
      m_tuningState = DvbTuner::TUNING_STATE_TUNE_FAILED;
      return false;
    }

    CLog::Log(LOGINFO, "V4LHelper::Tune: tuning frontend to freq: %d force tune %d",
        frontend.frequency, bForceTune);

    if(!Tune(frontend))
      return false;
  }
  else
  {
    m_tuningState = DvbTuner::TUNING_STATE_TUNING_PROGRAM;
  }

  m_lastFreq = channel.GetFreq();

  if (m_tuningState == DvbTuner::TUNING_STATE_TUNING_PROGRAM)
  {
    m_tuningState = DvbTuner::TUNING_STATE_TUNED;
  }

  return true;
}

bool V4LHelper::Tune(struct dvb_frontend_parameters& frontend)
{
  /* Empty the event queue */
  for (;;)
  {
    struct dvb_frontend_event event;
    if (ioctl(m_frontendHandle, FE_GET_EVENT, &event) < 0
        && errno == EWOULDBLOCK
      )
      break;
  }

  if (ioctl(m_frontendHandle, FE_SET_FRONTEND, &frontend) < 0)
  {
    CLog::Log(LOGERROR, "V4LHelper::Tune: failed to set up frontend!");
    m_tuningState = DvbTuner::TUNING_STATE_TUNE_FAILED;
    return false;
  }

  m_frontend_settings = frontend;

  return true;
}

bool V4LHelper::IsSignalOk()
{
  if (m_tuningState != DvbTuner::TUNING_STATE_TUNED)
    return true;

  return m_signalOk;
}

bool V4LHelper::Stop()
{
  CSingleLock lock(m_lock);

  CloseAll();
  m_lastFreq = "";
  m_tuningState = DvbTuner::TUNING_STATE_IDLE;

  return true;
}

bool V4LHelper::IsTuneDone(int tmout)
{
  if (m_frontendHandle <= 0)
    return false;

  fe_status_t status;
  struct pollfd pfd[1];
  time_t tm1, tm2;
  bool ok = false;
  int locks = 0;

  pfd[0].fd = m_frontendHandle;
  pfd[0].events = POLLIN | POLLPRI;

  tm1 = tm2 = time((time_t*) NULL);

  while (!ok)
  {
    status = (fe_status_t) 0;
    if (poll(pfd, 1, tmout * 1000) > 0)
    {
      if (pfd[0].revents)
      {
        if (ioctl(m_frontendHandle, FE_READ_STATUS, &status) >= 0)
        {
          if (status == (FE_HAS_SIGNAL | FE_HAS_CARRIER | FE_HAS_VITERBI | FE_HAS_SYNC | FE_HAS_LOCK))
            locks++;
        }
      }
    }
    usleep(10000);
    tm2 = time((time_t*) NULL);
    if ((status & FE_TIMEDOUT) || (locks >= 2) || (tm2 - tm1 >= tmout))
      ok = true;
  }

  if (status == (FE_HAS_SIGNAL | FE_HAS_CARRIER | FE_HAS_VITERBI | FE_HAS_SYNC | FE_HAS_LOCK))
  {
    CLog::Log(LOGINFO, "FE locked.");
    return true;
  }

  return false;
}

uint16_t V4LHelper::GetSignal()
{
  CSingleLock lock(m_lock);
  if (m_frontendHandle <= 0)
    return 0;

  uint16_t sig = 0;
  ioctl(m_frontendHandle, FE_READ_SIGNAL_STRENGTH, &sig);

  return sig;
}

uint16_t V4LHelper::GetSNR()
{
  CSingleLock lock(m_lock);
  if (m_frontendHandle <= 0)
    return 0;

  uint16_t snr = 0;
  ioctl(m_frontendHandle, FE_READ_SNR, &snr);

  return snr;
}

uint16_t V4LHelper::GetBER()
{
  CSingleLock lock(m_lock);
  if (m_frontendHandle <= 0)
    return 0;

  uint16_t ber = 0;
  ioctl(m_frontendHandle, FE_READ_BER, &ber);

  return ber;
}

uint16_t V4LHelper::GetUCB()
{
  CSingleLock lock(m_lock);
  if (m_frontendHandle <= 0)
    return 0;

  uint16_t ucb = 0;
  ioctl(m_frontendHandle, FE_READ_UNCORRECTED_BLOCKS, &ucb);

  return ucb;
}

void V4LHelper::AddPidFilter(int pid, DvbTuner::DvbFilter type)
{
  bool bBudgetMode  = m_nFlags & TUNER_FLAGS_BUDGET;

  if(bBudgetMode)
    return;

  CLog::Log(LOGDEBUG, "Added pid %d to the filter", pid);

  if (m_pidHandles.find(pid) != m_pidHandles.end())
  {
    CLog::Log(LOGWARNING, "Request to add PID already exist. Ignoring=%d", pid);
    return;
  }

  CSingleLock lock(m_lock);

  CStdString demuxPath;
  demuxPath.Format("/dev/dvb/adapter%d/demux0", m_tunerId);

  int fd = open(demuxPath.c_str(), O_RDWR);
  if (fd < 0)
  {
    CLog::Log(LOGERROR, "V4LHelper::AddPidFilter: failed to open demuxer for pid: %d", pid);
    return;
  }

  ioctl(fd, DMX_SET_BUFFER_SIZE, 256 * 1024);

  dmx_pes_type_t pes_type = DMX_PES_OTHER;
  switch (type)
  {
  case DvbTuner::DVB_FILTER_VIDEO: pes_type = DMX_PES_VIDEO0; break;
  case DvbTuner::DVB_FILTER_AUDIO: pes_type = DMX_PES_AUDIO0; break;
  case DvbTuner::DVB_FILTER_OTHER: pes_type = DMX_PES_OTHER; break;
  }

  struct dmx_pes_filter_params filter;
  filter.pid = pid;
  filter.input = DMX_IN_FRONTEND;
  filter.output = DMX_OUT_TS_TAP;
  filter.pes_type = pes_type;
  filter.flags = DMX_IMMEDIATE_START;

  if (ioctl(fd, DMX_SET_PES_FILTER, &filter) < 0)
  {
    CLog::Log(LOGERROR,
        "V4LHelper::AddPidToFilter: failed to add pid %d to filter. %s", pid,
        strerror(errno));
    close (fd);
    return;
  }

  m_pidHandles[pid] = fd;
}

void V4LHelper::RemovePidFilter(int pid)
{
  bool bBudgetMode  = m_nFlags & TUNER_FLAGS_BUDGET;

  if(bBudgetMode)
    return;

  printf("V4LHelper::RemovePidFilter pid %d size %d\n", pid, m_pidHandles.size());

  if(m_pidHandles.size() == 0)
    return;

  CSingleLock lock(m_lock);

  ClosePID(pid);
  m_pidHandles.erase(pid);

  if(m_pidHandles.size() == 0)
  {
    Sleep(1000);
  }
}

void V4LHelper::ClosePID(int pid)
{
  // make sure the pid exists
  if (m_pidHandles.find(pid) == m_pidHandles.end())
  {
    CLog::Log(LOGWARNING, "Request to remove pid from the filter which does not exist: pid=%d", pid);
    return;
  }

  int fd = m_pidHandles[pid];

  CLog::Log(LOGNONE, "Remove pid from the filter: pid=%d fd=%d", pid, fd);

  if (ioctl(fd, DMX_STOP) < 0)
  {
    CLog::Log(LOGWARNING, "failed to stop dmx pid %d\n", fd);
  }

  close(fd);
}

int V4LHelper::Read(char * buf, int size)
{
  CSingleLock lock(m_lock);

  static bool timeout = false;

  if (m_frontendHandle <= 0)
    return -1;

  // Check the signal once a second
  if (m_tuningState == DvbTuner::TUNING_STATE_TUNED && SDL_GetTicks() - m_lastSignalCheckTicks > 1000)
  {
    uint16_t signal = GetSignal();
    uint16_t snr = GetSNR();

    if ((signal > 140 || signal == 0) && (snr > 100 || snr == 0))
      m_signalOk = true;
    else
      m_signalOk = false;

//    printf("signal=%d snr=%d signalok=%s\n", signal, snr, m_signalOk ? "true" : "false");

    m_lastSignalCheckTicks = SDL_GetTicks();
  }

  struct pollfd ufd[2];
  int n;

  ufd[0].fd = m_frontendHandle;
  ufd[0].events = POLLIN;
  n = 1;
  if(m_dvr > 0)
  {
    ufd[1].fd = m_dvr;
    ufd[1].events = POLLIN;
    n = 2;
  }

  // we put one second timeout for video to come out.
  // if it doesn't then we reopen devices
  int rc = poll(ufd, n, 1000);
  if (rc == -1 &&  errno == EINTR)
  {
    return 0;
  }

  if (rc < 0)
  {
    CLog::Log(LOGERROR, "%s poll failed: %s", __func__, strerror(errno));
    return -1;
  }

  // if dvr is open and we get a timeout, reopen devices
  if(m_dvr > 0 && rc == 0)
  {
    CLog::Log(LOGWARNING, "V4LHelper::Read DVR timeout. Reopening frontend");
    CloseFrontEnd();
    CloseDVR();
    Sleep(100);
    OpenFrontEnd();
    m_tuningState = DvbTuner::TUNING_STATE_TUNING;
    Tune(m_frontend_settings);
  }

  int bytesRead = 0;

  struct dvb_frontend_event ev;

  if (ufd[0].revents)
  {
    if (ioctl(m_frontendHandle, FE_GET_EVENT, &ev) < 0)
    {
      if (errno == EOVERFLOW)
      {
        CLog::Log(LOGWARNING, "Cannot dequeue events fast enough from frontend. Doing nothing");
        return 0;
      }
      else
        CLog::Log(LOGERROR, "Cannot dequeue frontend event: %s", strerror(errno));
    }
    else
    {
      CLog::Log(LOGINFO, "Got frontend event. status: 0x%02X", (unsigned) ev.status);

      if (ev.status == (FE_HAS_SIGNAL | FE_HAS_CARRIER | FE_HAS_VITERBI | FE_HAS_SYNC | FE_HAS_LOCK))
      {
        m_tuningState = DvbTuner::TUNING_STATE_TUNED;
        CloseDVR();
      }
      else if (m_tuningState == DvbTuner::TUNING_STATE_TUNED)
        m_tuningState = DvbTuner::TUNING_STATE_TUNE_FAILED;
    }
  }

  if (m_dvr > 0 && n == 2 && ufd[1].revents)
  {
    ssize_t val = read(m_dvr, buf, size);

    if (val == -1 && (errno != EAGAIN && errno != EINTR))
    {
      if (errno == EOVERFLOW)
      {
        CLog::Log(LOGWARNING, "Cannot read dvr data fast enough. Doing nothing");
        return 0;
      }

      CLog::Log(LOGERROR, "Error reading from dvr: %d %s", m_dvr, strerror(errno) );
      bytesRead = -1;
    }
    else if (val > 0)
    {
      bytesRead = val;
    }
  }

  if(m_tuningState == DvbTuner::TUNING_STATE_TUNED)
  {
    if(m_dvr < 0)
    {
      Sleep(100);
      OpenDVR();
    }
  }

  return bytesRead;
}

DvbTuner::TuningState V4LHelper::GetTuningState()
{
  return m_tuningState;
}

DvbTunerTechnicalInfo V4LHelper::GetTechnicalInfo()
{
  DvbTunerTechnicalInfo result;
  memset(&result, 0, sizeof(result));

  if (m_frontendHandle == -1)
    return result;

  fe_status_t status;

  if (ioctl(m_frontendHandle, FE_READ_STATUS, &status) >= 0)
  {
    result.fe_status = (unsigned int) status;
  }

  result.signal = GetSignal();
  result.snr = GetSNR();
  result.ber = GetBER();
  result.ucb = GetUCB();

  return result;

}

#endif
