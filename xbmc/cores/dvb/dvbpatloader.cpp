#include "dvbpatloader.h"

#if defined(HAS_DVB) && defined(_LINUX) && !defined(__APPLE__)

#include "StdString.h"

#include <fcntl.h>
#include <dvbpsi.h>
#include <demux.h>
#include <descriptor.h>
#include <tables/pat.h>
#include <descriptors/dr.h>
//#include <util/log.h>
#include <psi.h>
#include <tables/pmt.h>
#include <iconv.h>
#include <pthread.h>
#include "Util.h"
#include <boost/foreach.hpp>
#include <linux/dvb/dmx.h>
#include <linux/dvb/version.h>
#include <linux/dvb/frontend.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

void AddPidToFilter(int fd, int pid, int type)
{
  struct dmx_pes_filter_params filter;

  if (pid < 0 || pid >= 0x1fff || (pid == 0 && type != DMX_PES_OTHER))
    return ;

  ioctl(fd, DMX_SET_BUFFER_SIZE, 128 * 1024);

  filter.pid = pid;
  filter.input = DMX_IN_FRONTEND;
  filter.output = DMX_OUT_TS_TAP;
  filter.pes_type = (dmx_pes_type_t) type;
  filter.flags = DMX_IMMEDIATE_START;

  if (ioctl(fd, DMX_SET_PES_FILTER, &filter) < 0)
  {
    printf("V4LHelper::AddPidToFilter: failed to add pid %d to filter. %s", pid, strerror(errno));
  }
  else
  {
    printf("V4LHelper::AddPidToFilter: Successfully added pid %d to the filter", pid);
  }
}

static void PATCallBack(void *p_cb_data, dvbpsi_pat_t* p_pat)
{
  dvbpsi_pat_program_t* p_program = p_pat->p_first_program;
  DvbPatLoader* patLoader = (DvbPatLoader*)p_cb_data;
  printf(  "\n");
  printf(  "New PAT\n");
  printf(  "  transport_stream_id : %d\n", p_pat->i_ts_id);
  printf(  "  version_number      : %d\n", p_pat->i_version);
  printf(  "    | program_number @ [NIT|PMT]_PID\n");

  //AddPidToFilter(patLoader->m_fd, p_program->i_pid, DMX_PES_OTHER);

  while(p_program)
  {

    printf("    | %14d @ 0x%x (%d)\n",
           p_program->i_number, p_program->i_pid, p_program->i_pid);
    p_program = p_program->p_next;

  }
  printf(  "  active              : %d\n", p_pat->b_current_next);

  dvbpsi_DeletePAT(p_pat);
}

DvbPatLoader::DvbPatLoader()
{
  m_dvbpsi = dvbpsi_AttachPAT(PATCallBack, this);
  m_fd = -1;
}

void DvbPatLoader::Open()
{
  CStdString demuxPath;
  demuxPath.Format("/dev/dvb/adapter0/demux0");

  m_fd = open(demuxPath.c_str(), O_RDWR);
}

void DvbPatLoader::Close()
{
  if(m_fd > 0)
    close(m_fd);

  m_fd = -1;
}

bool DvbPatLoader::ProcessPacket(uint8_t* data)
{
  uint16_t i_pid = ((uint16_t) (data[1] & 0x1f) << 8) + data[2];
  if (i_pid == 0x0)
  {
    dvbpsi_PushPacket(m_dvbpsi, data);
    return true;
  }
}

#elif defined(__APPLE__) && defined(HAS_DVB)

DvbPatLoader::DvbPatLoader()
{
}
#endif
