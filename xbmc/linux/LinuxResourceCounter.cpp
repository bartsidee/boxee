/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "PlatformInclude.h"
#include "LinuxResourceCounter.h"
#include "utils/log.h"

#include <errno.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach_time.h>
#include <mach/mach.h>
#include <mach/clock.h>
#endif

double timediff(const struct timespec *ta,const struct timespec *tb)
{
  double us = ((double)ta->tv_sec-tb->tv_sec)*1000000.0 + ((double)ta->tv_nsec/1000.0 - (double)tb->tv_nsec/1000.0);
  return us;
}

CLinuxResourceCounter::CLinuxResourceCounter()
{
  Reset();
}

CLinuxResourceCounter::~CLinuxResourceCounter()
{
}

double CLinuxResourceCounter::GetCPUUsage()
{
  struct timespec tmNow;

  int retVal;

#if defined(__APPLE__) && defined(__MACH__)
  clock_serv_t cclock;
  mach_timespec_t mts;

  retVal = host_get_clock_service(mach_host_self(), REALTIME_CLOCK, &cclock);
  if (retVal != KERN_SUCCESS)
  {
    CLog::Log(LOGERROR,"CLinuxResourceCounter::GetCPUUsage - error [%d] in host_get_clock_service. [%s]",retVal,mach_error_string(retVal));
    return m_dLastUsage;
  }

  retVal = clock_get_time(cclock, &mts);
  if (retVal != KERN_SUCCESS)
  {
    CLog::Log(LOGERROR,"CLinuxResourceCounter::GetCPUUsage - error [%d] in clock_get_time. [%s]",retVal,mach_error_string(retVal));
    return m_dLastUsage;
  }

  retVal = mach_port_deallocate(mach_task_self(), cclock);
  if (retVal != KERN_SUCCESS)
  {
    CLog::Log(LOGERROR,"CLinuxResourceCounter::GetCPUUsage - error [%d] in mach_port_deallocate. [%s]",retVal,mach_error_string(retVal));
    return m_dLastUsage;
  }

  tmNow.tv_sec = mts.tv_sec;
  tmNow.tv_nsec = mts.tv_nsec;
#else
  retVal = clock_gettime(CLOCK_REALTIME, &tmNow);
  if (retVal == -1)
  {
    CLog::Log(LOGERROR,"CLinuxResourceCounter::GetCPUUsage - error %d in clock_gettime", errno);
    return m_dLastUsage;
  }
#endif

  double dElapsed = timediff(&tmNow,&m_tmLastCheck);

  if (dElapsed >= 1000000.0)
  {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == -1)
      CLog::Log(LOGERROR,"CLinuxResourceCounter::GetCPUUsage - error %d in getrusage", errno);
    else
    {
      double dUser = ( ((double)usage.ru_utime.tv_sec + (double)usage.ru_utime.tv_usec / 1000000.0) -
                       ((double)m_usage.ru_utime.tv_sec + (double)m_usage.ru_utime.tv_usec / 1000000.0) );
      double dSys  = ( ((double)usage.ru_stime.tv_sec + (double)usage.ru_stime.tv_usec / 1000000.0) -
                ((double)m_usage.ru_stime.tv_sec + (double)m_usage.ru_stime.tv_usec / 1000000.0) );

      m_tmLastCheck = tmNow;
      m_usage = usage;

      m_dLastUsage = ((dUser+dSys) / (dElapsed/1000000.0)) * 100.0;

      return m_dLastUsage;
    }
  }

  return m_dLastUsage;
}

void CLinuxResourceCounter::Reset()
{
  int retVal;

#if defined(__APPLE__) && defined(__MACH__)
  clock_serv_t cclock;
  mach_timespec_t mts;

  retVal = host_get_clock_service(mach_host_self(), REALTIME_CLOCK, &cclock);
  if (retVal != KERN_SUCCESS)
  {
    CLog::Log(LOGERROR,"CLinuxResourceCounter::Reset - error [%d] in host_get_clock_service. [%s]",retVal,mach_error_string(retVal));
  }

  retVal = clock_get_time(cclock, &mts);
  if (retVal != KERN_SUCCESS)
  {
    CLog::Log(LOGERROR,"CLinuxResourceCounter::Reset - error [%d] in clock_get_time. [%s]",retVal,mach_error_string(retVal));
  }

  retVal = mach_port_deallocate(mach_task_self(), cclock);
  if (retVal != KERN_SUCCESS)
  {
    CLog::Log(LOGERROR,"CLinuxResourceCounter::Reset - error [%d] in mach_port_deallocate. [%s]",retVal,mach_error_string(retVal));
  }

  m_tmLastCheck.tv_sec = mts.tv_sec;
  m_tmLastCheck.tv_nsec = mts.tv_nsec;

#else
  retVal = clock_gettime(CLOCK_REALTIME,&m_tmLastCheck);
  if (retVal == -1)
  {
    CLog::Log(LOGERROR,"CLinuxResourceCounter::Reset - error %d in clock_gettime", errno);
  }
#endif

  if (getrusage(RUSAGE_SELF, &m_usage) == -1)
    CLog::Log(LOGERROR,"CLinuxResourceCounter::Reset - error %d in getrusage", errno);

  m_dLastUsage = 0.0;
}





