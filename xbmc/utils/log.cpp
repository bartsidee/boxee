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

#include "system.h"
#include "log.h"
#include "RegExp.h"
#ifndef _LINUX
#include <share.h>
#endif
#include "CriticalSection.h"
#include "SingleLock.h"
#include "FileSystem/File.h"
#include "StdString.h"
#include "Settings.h"
#include "GUISettings.h"
#include "AdvancedSettings.h"
#include "Util.h"
#include "Thread.h"
XFILE::CFile* CLog::m_file = NULL;
int           CLog::m_logSize = 0;
int            CLog::m_logLevel = LOGINFO;
bool           CLog::m_showLogLine = false;
int            CLog::m_syslogFd = -1;
#ifndef _WIN32
struct sockaddr_in CLog::m_syslogAddr;
#endif

static CCriticalSection critSec;

static char levelNames[][8] =
{"DEBUG", "INFO", "NOTICE", "WARNING", "ERROR", "SEVERE", "FATAL", "NONE"};

#ifdef _WIN32
#define LINE_ENDING "\r\n"
#else
#define LINE_ENDING "\n"
#endif

// I want to compile this to binary all the time 
// to prevent link error when people define _USE_DBG_MACRO_
// on per-file basis.
#ifdef _LINUX  
struct timeval dbg_start_up_time = {0,0};
struct timeval dbg_last_time = {0,0};
#else 
time_t dbg_start_up_time = 0;
#endif
int   dbg_debug_level = 5;
int   dbg_max_msg = 100;
bool  dbg_print_pretty = false;


bool MESSGES      = false;
bool MAIN_THREAD  = false;
bool OPN_GL       = false;
bool DAUDIO       = true;
bool PAP          = false;
bool DFILES        = false;
bool DTEMP1        = true;
bool DTEMP2        = true;
bool DTEMP3        = true;

#define SYSLOG_PORT 514
char syslogStr[16384];

void DbgInit() {
#ifdef _LINUX  
  gettimeofday(&dbg_start_up_time, NULL);
#else
  dbg_start_up_time = time(NULL);  
#endif
}

bool CLog::RotateLog(void) {

    // g_stSettings.m_logFolder is initialized in the CSettings constructor
    // and changed in CApplication::Create()
    CStdString strLogFile, strLogFileOld;

    // avoid log file name with "(null)..."
    if (g_stSettings.m_logFolder.c_str() == NULL)
      return false;

    strLogFile.Format("%sboxee.log", g_stSettings.m_logFolder.c_str());
    strLogFileOld.Format("%sboxee.old.log", g_stSettings.m_logFolder.c_str());

	Close();

	m_file = new XFILE::CFile;
	if (!m_file)
	  return false;

    if(XFILE::CFile::Exists(strLogFileOld))
    	XFILE::CFile::Delete(strLogFileOld);
    if(XFILE::CFile::Exists(strLogFile))
    	XFILE::CFile::Rename(strLogFile, strLogFileOld);

    if(!m_file->OpenForWrite(strLogFile))
      return false;

    m_logSize = 0;
    return true;
}

CLog::CLog()
{}

CLog::~CLog()
{}

void CLog::Close()
{
  CSingleLock waitLock(critSec);
  if (m_file)
  {
    m_file->Close();
    delete m_file;
    m_file = NULL;
  }

#ifndef _WIN32
  if (m_syslogFd != -1)
  {
    close(m_syslogFd);
    m_syslogFd = -1;
}
#endif
}


void CLog::Log(int loglevel, const char *format, ... )
{
  if (loglevel >= m_logLevel)
  {
    CSingleLock waitLock(critSec);
    if (!m_file && !CLog::RotateLog())
    {
      return;
    }

    SYSTEMTIME time;
    GetLocalTime(&time);

    MEMORYSTATUS stat;
    GlobalMemoryStatus(&stat);

    CStdString strPrefix, strData;

    strPrefix.Format("%02.2d:%02.2d:%02.2d.%03.3d T:%"PRIu64" M:%9"PRIu64" %7s: ", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds, (uint64_t)CThread::GetCurrentThreadId(), (uint64_t)stat.dwAvailPhys, levelNames[loglevel]);

    strData.reserve(16384);

#ifdef _WIN32
	// Replace all %z with %I, since VS CRT is not C99 complaint:
	CStdString format_str(format);
	format_str.Replace("%z", "%I");
	format = format_str.c_str();
#endif
    va_list va;
    va_start(va, format);
    strData.FormatV(format,va);    
    va_end(va);


    unsigned int length = 0;
    while ( length != strData.length() )
    {
      length = strData.length();
      strData.TrimRight(" ");
      strData.TrimRight('\n');      
      strData.TrimRight("\r");
    }

    if (!length)
      return;

    // remove password
    CUtil::RemovePasswordFromPath(strData);

#if !defined(_LINUX) && (defined(_DEBUG) || defined(PROFILE))
    OutputDebugString(strData.c_str());
    OutputDebugString("\n");
#endif

#if defined(_LINUX)
    // temporary to ease debugging
    printf("%2.2d:%2.2d:%2.2d.%3.3d %7s: %s\n", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds, levelNames[loglevel], strData.c_str());
#endif

    /* fixup newline alignment, number of spaces should equal prefix length */
    strData.Replace("\n", LINE_ENDING"                                            ");
    strData += LINE_ENDING;

    m_logSize = m_logSize + strPrefix.size() + strData.size();
    if (m_logSize > g_advancedSettings.m_logFileSize) 
    {
      if (!CLog::RotateLog())
        return;
    }
    m_file->Write(strPrefix.c_str(), strPrefix.size());
    m_file->Write(strData.c_str(), strData.size());

#ifndef _WIN32
    if (m_syslogFd != -1)
    {
      int level = 0;
      switch (loglevel)
      {
      case LOGNONE:     level = 6; break;
      case LOGDEBUG:    level = 7; break;
      case LOGINFO:     level = 6; break;
      case LOGNOTICE:   level = 5; break;
      case LOGWARNING:  level = 4; break;
      case LOGERROR:    level = 3; break;
      case LOGSEVERE:   level = 2; break;
      case LOGFATAL:    level = 0; break;
      }

      level += 8 * 16 /* LOCAL0 */;

      static const char* MONTHS[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
      sprintf(syslogStr, "<%d>%s %d %02d:%02d:%02d boxee %s", level, MONTHS[time.wMonth - 1], time.wDay, time.wHour, time.wMinute, time.wSecond, strData.c_str());
      sendto(m_syslogFd, syslogStr, strlen(syslogStr), 0, (struct sockaddr *)&m_syslogAddr, sizeof(m_syslogAddr));
    }
#endif
  }
}

void CLog::DebugLog(const char *format, ... )
{
#ifdef _DEBUG
  CSingleLock waitLock(critSec);

  CStdString strData;
  strData.reserve(16384);

  va_list va;
  va_start(va, format);
  strData.FormatV(format, va);    
  va_end(va);
  
  OutputDebugString(strData.c_str());
  if( strData.Right(1) != "\n" )
    OutputDebugString("\n");
#endif
}

void CLog::DebugLogMemory()
{
  CSingleLock waitLock(critSec);
  MEMORYSTATUS stat;
  CStdString strData;

  GlobalMemoryStatus(&stat);
#ifdef __APPLE__
  strData.Format("%ju bytes free\n", stat.dwAvailPhys);
#else
  strData.Format("%lu bytes free\n", stat.dwAvailPhys);
#endif
  OutputDebugString(strData.c_str());
}

void CLog::MemDump(char *pData, int length)
{
  Log(LOGDEBUG, "MEM_DUMP: Dumping from %p", pData);
  for (int i = 0; i < length; i+=16)
  {
    CStdString strLine;
    strLine.Format("MEM_DUMP: %04x ", i);
    char *alpha = pData;
    for (int k=0; k < 4 && i + 4*k < length; k++)
    {
      for (int j=0; j < 4 && i + 4*k + j < length; j++)
      {
        CStdString strFormat;
        strFormat.Format(" %02x", *pData++);
        strLine += strFormat;
      }
      strLine += " ";
    }
    // pad with spaces
    while (strLine.size() < 13*4 + 16)
      strLine += " ";
    for (int j=0; j < 16 && i + j < length; j++)
    {
      CStdString strFormat;
      if (*alpha > 31)
        strLine += *alpha;
      else
        strLine += '.';
      alpha++;
    }
    Log(LOGDEBUG, "%s", strLine.c_str());
  }
}

void CLog::ResetSyslogServer()
{
#ifndef _WIN32
  if (m_syslogFd != -1)
  {
    close(m_syslogFd);
    m_syslogFd = -1;    
  }

  if (!g_guiSettings.GetBool("debug.syslogenabled"))
  {
    CLog::Log(LOGINFO, "Writing to syslog disabled");
    m_syslogFd = -1;
    return;
  }

  if (g_guiSettings.GetString("debug.syslogaddr").length() == 0)
  {
    CLog::Log(LOGINFO, "Writing to syslog disabled");
    return;
  }  
      
  struct hostent *hp;
  if ((hp = gethostbyname(g_guiSettings.GetString("debug.syslogaddr").c_str())) == 0) 
  {
    CLog::Log(LOGNOTICE, "Could not determine syslog server name");
    return;
  }
    
  memset(&m_syslogAddr, 0, sizeof(m_syslogAddr));
  m_syslogAddr.sin_family = AF_INET;
  m_syslogAddr.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
  m_syslogAddr.sin_port = htons(SYSLOG_PORT);
  
  m_syslogFd = socket(AF_INET, SOCK_DGRAM, 0);

  CLog::Log(LOGINFO, "Writing to syslog enabled");
#endif
}
