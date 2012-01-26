#pragma once

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

#include <stdio.h>

#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h> 
#include <sys/socket.h>
#endif

#define LOG_LEVEL_NONE         -1 // nothing at all is logged
#define LOG_LEVEL_NORMAL        0 // shows notice, error, severe and fatal
//#define LOG_LEVEL_DEBUG         1 // shows all
#define LOG_LEVEL_DEBUG_FREEMEM 2 // shows all + shows freemem on screen
#define LOG_LEVEL_DEBUG_SAMBA   3 // shows all + freemem on screen + samba debugging
#define LOG_LEVEL_MAX           LOG_LEVEL_DEBUG_SAMBA

// ones we use in the code
#define LOGDEBUG   0
#define LOGINFO    1
#define LOGNOTICE  2
#define LOGWARNING 3
#define LOGERROR   4
#define LOGSEVERE  5
#define LOGFATAL   6
#define LOGNONE    7

#ifdef __GNUC__
#define ATTRIB_LOG_FORMAT __attribute__((format(printf,2,3)))
#else
#define ATTRIB_LOG_FORMAT
#endif

namespace XFILE {
  class CFile;
}

class CLog
{
public:
  CLog();
  virtual ~CLog(void);
  static void Close();
  static void Log(int loglevel, const char *format, ... ) ATTRIB_LOG_FORMAT;
  static void DebugLog(const char *format, ...);
  static void MemDump(char *pData, int length);
  static void DebugLogMemory();
  static void ResetSyslogServer();

  static int           m_logLevel;
  static bool          m_showLogLine;
  
private:
  static int           m_syslogFd;
  static bool RotateLog(void);
  static XFILE::CFile *m_file;
  static int           m_logSize;
  
#ifndef _WIN32
  static struct sockaddr_in m_syslogAddr;
#endif
};

// GL Error checking macro
// this function is useful for tracking down GL errors, which otherwise
// just result in undefined behavior and can be difficult to track down.
//
// Just call it 'VerifyGLState()' after a sequence of GL calls
// 
// if _DEBUG and HAS_GL are defined, the function checks
// for GL errors and prints the current state of the various matrices;
// if not it's just an empty inline stub, and thus won't affect performance
// and will be optimized out.

#undef GL_DEBUGGING

void _VerifyGLState(const char* szfile, const char* szfunction, int lineno);
#if defined(GL_DEBUGGING) && defined(_DEBUG) && defined(HAS_GL)
#define VerifyGLState() _VerifyGLState(__FILE__, __FUNCTION__, __LINE__)
#else
#define VerifyGLState()
#endif

void LogGraphicsInfo();

extern void DbgInit();

//#define _USE_DBG_MACRO_
#ifdef _USE_DBG_MACRO_
#ifdef _LINUX  
extern struct timeval dbg_start_up_time;
extern struct timeval dbg_last_time;
#else
extern time_t dbg_start_up_time;
#endif
extern int  dbg_debug_level;
extern int  dbg_max_msg;
extern bool dbg_print_pretty;

extern bool MAIN_THREAD;
extern bool MESSGES;
extern bool OPN_GL;
extern bool DAUDIO;
extern bool PAP;
extern bool DFILES;
extern bool DTEMP1;
extern bool DTEMP2;
extern bool DTEMP3;



//Variadic Macro
// Component = debuged component
// Level = 0 bulshit, 10 critical, 5 normal.

#ifdef _WIN32
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif

#define DBG_FILE                        stdout
#ifdef _LINUX  
#define DBG_TIME()                      {struct timeval _tv; gettimeofday(&_tv, NULL); lenlen += fprintf(DBG_FILE, "%d:%6d (+%7d) ",(int)(_tv.tv_sec - dbg_start_up_time.tv_sec), (int)_tv.tv_usec, int((_tv.tv_sec - dbg_last_time.tv_sec)*1000000 + (_tv.tv_usec - dbg_last_time.tv_usec))); dbg_last_time = _tv;};
#else
#define DBG_TIME()                      {time_t _tv = time(NULL); lenlen += fprintf(DBG_FILE, "%d | ", (int)(_tv - dbg_start_up_time));}
#endif
#define DBG_META(D_COMPONENT, LEVEL)   {fprintf(DBG_FILE, "%s:%d   %s(%s:%d)\n", #D_COMPONENT, LEVEL , ((dbg_print_pretty)?__PRETTY_FUNCTION__:__FUNCTION__), __FILE__, __LINE__);}
#define DBG(D_COMPONENT, LEVEL, ...)   {if (D_COMPONENT && LEVEL >= dbg_debug_level) {int lenlen = 0; DBG_TIME(); lenlen += fprintf(DBG_FILE, __VA_ARGS__); fprintf(DBG_FILE,"          "); while(++lenlen < dbg_max_msg) fprintf(DBG_FILE," "); DBG_META(D_COMPONENT, LEVEL);}}
#else

#define DBG(X, ...) 

#endif

