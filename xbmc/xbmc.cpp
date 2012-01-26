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


// XBMC
//
// libraries:
//   - CDRipX   : doesnt support section loading yet
//   - xbfilezilla : doesnt support section loading yet
//

#include "system.h"
#include "Application.h"
#include "lib/libBoxee/boxee.h"
#include "utils/Base64.h"
#include "AdvancedSettings.h"
#include "FileItem.h"
#include "UpdateSourceFile.h"
#include "PlayListPlayer.h"
#ifdef _LINUX
#include <sys/resource.h>
#include <signal.h>
#endif
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>
#include "Util.h"
#endif
#ifdef HAS_LIRC
#include "common/LIRC.h"
#endif

#if defined(_LINUX) && !defined(__APPLE__) && !defined(CANMORE)
#include <dbus/dbus.h>
#endif

#if defined(HAS_EMBEDDED) && !defined(__APPLE__)
#include <linux/capability.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <ucontext.h>
#include <dlfcn.h>
#include <sys/sysinfo.h>
#include <cxxabi.h>
#include <execinfo.h>
#include <sys/user.h>

#include "FileSystem/Directory.h"
#include "Util.h"
#include "utils/GUIInfoManager.h"
#include "lib/libBoxee/boxee.h"
#include "HalServices.h"
#include "File.h"

using __cxxabiv1::__cxa_demangle;

static std::map<pid_t, CStdString> g_threadStacks;
HANDLE hDumpDoneEvent = NULL;

#ifndef capget 
#define capget(hdr, data) syscall(__NR_capget, hdr, data)
#endif

#ifndef capset 
#define capset(hdr, data) syscall(__NR_capset, hdr, data)
#endif

#ifndef gettid
#define gettid() syscall(__NR_gettid)
#endif
#endif

#include "osx/CocoaInterface.h"

#include "utils/log.h"

CApplication g_application;

#ifdef __APPLE__
pascal OSStatus switchEventsHandler(EventHandlerCallRef nextHandler, EventRef switchEvent, void* userData)
{
  return 0;
}
pascal OSStatus rawKeyDownEventsHandler(EventHandlerCallRef nextHandler, EventRef rawKeyEvent, void* userData)
{
  return 0;
}

#ifdef __LP64__
pascal OSErr openEventsHandler(const AppleEvent *openEvent, AppleEvent* reply, void * refCon)
#else
pascal OSErr openEventsHandler(const AppleEvent *openEvent, AppleEvent* reply, long refCon)
#endif
{
  AEDescList docList;
  AEKeyword keywd;
  DescType returnedType;
  Size actualSize;
  long itemsInList;
  OSErr err;
  short i;

  CFileItemList playlist;

  err = AEGetParamDesc(openEvent, keyDirectObject, typeAEList,&docList);
  if (err != noErr)
  {
    return err;
  }
  err = AECountItems(&docList, &itemsInList);
  if (err != noErr)
    return err;
  
  FSRef theRef ;  
  for (i = 1; i <= itemsInList; i++)
  {
    AEGetNthPtr(&docList, i, typeFSRef, &keywd, &returnedType, (Ptr)&theRef, sizeof(theRef), &actualSize);
    UInt8 item[2048];
    if (FSRefMakePath(&theRef, item, 2048) == noErr) 
    {
      CFileItemPtr pItem(new CFileItem((const char *)item));
      pItem->m_strPath = (const char *)item;
      playlist.Add(pItem);
    }
  }

  if (playlist.Size() > 0)
  {
    ProcessSerialNumber psn;
    if ( GetCurrentProcess( &psn ) == noErr )
      (void) SetFrontProcess( &psn );
    
    g_playlistPlayer.Clear();
    g_playlistPlayer.Add(0,playlist);
    g_playlistPlayer.SetCurrentPlaylist(0);
    ThreadMessage tMsg(TMSG_PLAYLISTPLAYER_PLAY, (DWORD) -1);
    g_application.getApplicationMessenger().SendMessage(tMsg, false);
  }
  
  return noErr;
}
#endif

#if defined(__i386__) && defined (HAS_EMBEDDED)
int make_secure()
{
  int retval = -1;	

  do 
  {
    struct passwd *user = getpwnam("boxee");
    if (!user)
    {
      CLog::Log(LOGERROR,"User 'boxee' does not exist!\n");
      break;
    }


    uid_t uid = user->pw_uid;

    struct group *group = getgrnam("boxee");
    if (!group)
    {
      CLog::Log(LOGERROR,"Group 'boxee' does not exist!\n");
      break;
    }

    gid_t gid = group->gr_gid;

    struct __user_cap_header_struct hdr;
    struct __user_cap_data_struct data;
    
    memset(&hdr, 0, sizeof(hdr));
    hdr.version = _LINUX_CAPABILITY_VERSION;

    if(capget(&hdr, &data) < 0)
    {
      CLog::Log(LOGERROR,"Unable to get capabilities of 'boxee': %s\n", strerror(errno));
      break;
    }

     /* Tell kernel not clear capabilities when dropping root */
    if (prctl(PR_SET_KEEPCAPS, 1) < 0)
    {
      CLog::Log(LOGERROR,"Unable to keep capabilities of 'boxee: %s\n", strerror(errno));
      break;
    }

    data.effective = data.permitted = CAP_TO_MASK(CAP_SYS_RAWIO) | CAP_TO_MASK(CAP_SETUID) | CAP_TO_MASK(CAP_SETGID) |  CAP_TO_MASK(CAP_SETPCAP);
  
    if (capset(&hdr, &data) < 0)
    {
      CLog::Log(LOGERROR,"Unable to set capabilities of 'boxee: %s\n", strerror(errno));
      break;
    }

    if (setgid(gid) || setuid(uid))
    {
      CLog::Log(LOGERROR,"Unable to set uid or gid to 'boxee': %s\n", strerror(errno));
      break;
    }

    data.effective = CAP_TO_MASK(CAP_SYS_RAWIO) | CAP_TO_MASK(CAP_SETPCAP);
    if (capset(&hdr, &data) < 0)
    {
      CLog::Log(LOGERROR,"Unable to set effective capabilities of 'boxee: %s\n", strerror(errno));
      break;
    }

    retval = 0;
  
  } while(false);

  
  return retval;

}

static void sigusr1_handler(int signum, siginfo_t* info, void*ptr)
{
  g_threadStacks[gettid()] = CUtil::DumpStack();

  SetEvent(hDumpDoneEvent);
}

static void crash_handler(int signum, siginfo_t* info, void*ptr) 
{
  pid_t pid = getpid();
  char file_name[256];
  FILE *fp = NULL;
  CStdString msg;
  void *ip;
  void **bp;
  ucontext_t *ucontext = (ucontext_t*)ptr;

  //CUtil::DumpStack(true);

  // Don't get into an infinite loop
  signal(SIGSEGV, SIG_DFL);
  signal(SIGQUIT, SIG_DFL);

  hDumpDoneEvent = CreateEvent(NULL, true, false, NULL);

  sprintf(file_name, "/tmp/bt.%d", pid);

  fp = fopen(file_name, "wt");
  if(!fp)
  {
    CLog::Log(LOGERROR, "%s- unable to create %s\n", __func__, file_name);
    exit(-1);
  }

  ip = (void*)ucontext->uc_mcontext.gregs[REG_EIP];
  bp = (void**)ucontext->uc_mcontext.gregs[REG_EBP]; 

  CLog::Log(LOGERROR, "\nSegmentation Fault!!!\n");

  CStdString line;

  line.Format("Username: %s\n", BOXEE::Boxee::GetInstance().GetCredentials().GetUserName());
  msg += line;

  line.Format("Boxee Version: %s\n", g_infoManager.GetVersion());
  msg += line;

  CDateTime time=CDateTime::GetCurrentDateTime();
  CStdString strCrashTime = time.GetAsLocalizedDateTime(false, false);
  line.Format("Crash time: %s\n", strCrashTime.c_str()); 
  msg += line;

  struct sysinfo s_info;
  sysinfo(&s_info);
  line.Format("Uptime: %u secs\n", s_info.uptime);
  msg += line;
 
  IHalServices& client = CHalServicesFactory::GetInstance();
  
  CHalEthernetInfo ethInfo;
  client.GetEthernetInfo(0,ethInfo);
  line.Format("MAC: %s\n", ethInfo.mac_address);
  msg += line;

  CHalHardwareInfo hwInfo;
  client.GetHardwareInfo(hwInfo);
  line.Format("Serial Number: %s\n", hwInfo.serialNumber);
  msg += line;
 
  int temperature;
  client.GetCPUTemperature(temperature);
  line.Format("Temperature: %d\n", temperature);
  msg += line;

  line.Format("\nStack trace (faulting thread [%d]):\n", gettid());
  msg += line;

  CStdString stack = CUtil::DumpStack();
  msg += stack;

  // dump stack for rest of the threads
  CFileItemList items;
  DIRECTORY::CDirectory::GetDirectory("/proc/self/task", items);

  for(int i=0; i<items.Size(); i++)
  {
    CUtil::RemoveSlashAtEnd(items[i]->m_strPath);
    CStdString strPid = CUtil::GetFileName(items[i]->m_strPath);
    pid_t tid = atoi(strPid.c_str());

    // sending the signal causing the kernel execute the signal handler in context of the thread 
    // so we able to see his stack there  
    if(kill(tid, SIGUSR1) == -1)
    {
      CLog::Log(LOGERROR, "error in kill - %s", strerror(errno));
      continue;
    }

    WaitForSingleObject(hDumpDoneEvent, 1000);
    ResetEvent(hDumpDoneEvent);
  } 
  
  for(std::map<pid_t, CStdString>::iterator it = g_threadStacks.begin(); it != g_threadStacks.end(); it++)
  {
    pid_t tid = it->first;
    CStdString stack = it->second;
    
    line.Format("\nThread %d:\n", tid);
    msg += line; 
    msg += stack;
  }
  

  fwrite(msg.c_str(), msg.length(), 1, fp);  
  fclose(fp);

  printf("%s", msg.c_str());

  printf("Backtrace file goes to %s", file_name);

  raise(SIGQUIT);
  //exit(-1);
}

#endif

int main(int argc, char* argv[])
{
#if defined(_LINUX) && !defined(__APPLEPP_)
  std::set_terminate(__gnu_cxx::__verbose_terminate_handler);
#endif

#if defined(_LINUX) && !defined(__APPLE__) && !defined(CANMORE)
  dbus_threads_init_default();
#endif

  unsetenv("LD_PRELOAD");
  CStdString sUrlSource("other");

#ifdef __APPLE__
  OSStatus  err = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, openEventsHandler, 0L, false);
  
  void* pool = Cocoa_Create_AutoReleasePool();

  const EventTypeSpec applicationEvents[]  = { { kEventClassApplication, kEventAppFrontSwitched } };
  InstallApplicationEventHandler(NewEventHandlerUPP(switchEventsHandler), GetEventTypeCount(applicationEvents), applicationEvents, 0, NULL);

  const EventTypeSpec keyboardEvents[]  = { { kEventClassKeyboard, kEventRawKeyDown } };
  InstallApplicationEventHandler(NewEventHandlerUPP(rawKeyDownEventsHandler), GetEventTypeCount(keyboardEvents), keyboardEvents, 0, NULL);

  FSRef     fileRef;
  err = FSPathMakeRef((StringPtr)"/Applications/Boxee.app", &fileRef, NULL);
  if (err == noErr)
    err = LSRegisterFSRef(&fileRef, true);
  
#endif
  
  CFileItemList playlist;
#ifdef _LINUX

  struct rlimit rlim;

#if 0
  rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
  if (setrlimit(RLIMIT_CORE, &rlim) == -1)
    CLog::Log(LOGDEBUG, "Failed to set core size limit (%s)", strerror(errno));
#endif

  rlim.rlim_cur = rlim.rlim_max = 1024 * 1024;
  if (setrlimit(RLIMIT_STACK, &rlim) == -1)
    CLog::Log(LOGERROR, "Failed to set stack size limit (%s)", strerror(errno));

  // Prevent child processes from becoming zombies on exit if not waited upon. See also Util::Command
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));

  sa.sa_flags = SA_NOCLDWAIT;
  sa.sa_handler = SIG_IGN;
  sigaction(SIGCHLD, &sa, NULL);
#if defined(__i386__) && defined (HAS_EMBEDDED)
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = crash_handler;
  sigaction(SIGSEGV,&sa, NULL);
  sigaction(SIGFPE,&sa, NULL);
  sigaction(SIGILL,&sa, NULL);
  sigaction(SIGABRT,&sa, NULL);
  sigaction(SIGQUIT,&sa, NULL);
  sa.sa_sigaction = sigusr1_handler;
  sigaction(SIGUSR1, &sa, NULL);

  if(!XFILE::CFile::Exists(".run_as_root"))
  {
    make_secure();
  }
#endif
#endif

  setlocale(LC_NUMERIC, "C");

  if (argc > 1)
  {
    for (int i = 1; i < argc; i++)
    {
      if (strnicmp(argv[i], "-usf", 4) == 0)
      {
        printf("main - Going to call UpdateSourceFile.UpdateProfilesSourceFile()\n");

        CUpdateSourceFile UpdateSourceFile;
        if (UpdateSourceFile.UpdateProfilesSourceFile())
        {
          printf("main - SUCCESSFULLY updated the profiles source files\n");
        }
        else
        {
          printf("main - FAILED to update profiles source files\n");
        }
        
        if (UpdateSourceFile.UpgradeSettings())
        {
          printf("main - SUCCESSFULLY updated the setting files\n");
        }
        else
        {
          printf("main - FAILED to update the setting files\n");
        }

        exit(0);
      }
#ifdef HAS_EMBEDDED
      else if (strnicmp(argv[i], "-nftu", 5) == 0)
      {
        g_application.SetEnableFTU(false);
      }
#endif
      else if (strnicmp(argv[i], "-v", 2) == 0)
      {
        g_application.SetVerbose(true);
      }
      else if (strnicmp(argv[i], "-dcb", 4) == 0 || strnicmp(argv[i], "-wireframe", 10) == 0)
      {
        g_application.SetDrawControlBorders(true);
      }
    
      if (strnicmp(argv[i], "-fs", 3) == 0 || strnicmp(argv[i], "--fullscreen", 12) == 0)
      {
        g_advancedSettings.m_startFullScreen = true;
      }
      else if (strnicmp(argv[i], "-h", 2) == 0 || strnicmp(argv[i], "--help", 6) == 0)
      {
        printf("Usage: %s [OPTION]... [FILE]...\n\n", argv[0]);
        printf("Arguments:\n");
        printf("  -fs\t\t\tRuns XBMC in full screen\n");
        printf("  --standalone\t\tXBMC runs in a stand alone environment without a window \n");
        printf("\t\t\tmanager and supporting applications. For example, that\n");
        printf("\t\t\tenables network settings.\n");
        printf("  -p or --portable\tXBMC will look for configurations in install folder instead of ~/.xbmc\n");
        printf("  --legacy-res\t\tEnables screen resolutions such as PAL, NTSC, etc.\n");
#ifdef HAS_LIRC
        printf("  -l or --lircdev\tLircDevice to use default is /dev/lircd .\n");
        printf("  -n or --nolirc\tdo not use Lirc, aka no remote input.\n");
#endif
        exit(0);
      }
      else if (strnicmp(argv[i], "--standalone", 12) == 0)
      {
        g_application.SetStandAlone(true);
      }
      else if (strnicmp(argv[i], "-p", 2) == 0 || strnicmp(argv[i], "--portable", 10) == 0)
      {
        g_application.EnablePlatformDirectories(false);
      }
      else if (strnicmp(argv[i], "--legacy-res", 12) == 0)
      {
        g_application.SetEnableLegacyRes(true);
      }
#ifdef HAS_LIRC
      else if (strnicmp(argv[i], "--lircdev", 9) == 0)
      {
        // check the next arg with the proper value.
        int next=i+1;
        if (next < argc)
        {
          if ((argv[next][0] != '-' ) && (argv[next][0] == '/' ))
          {
            g_RemoteControl.setDeviceName(argv[next]);
            i++;
          }
        }
      }
      else if (strnicmp(argv[i], "--nolirc", 8) == 0)
         g_RemoteControl.setUsed(false);
#endif
      else if (strnicmp(argv[i], "-nojs", 5) == 0)
      {
        sUrlSource = "browser-app";
      }
      else if (argv[i][0] != '-')
      {
        CFileItemPtr pItem(new CFileItem(argv[i]));
        pItem->SetProperty("url_source", sUrlSource);
        pItem->m_strPath = argv[i];
        playlist.Add(pItem);
      }
    }
  }

  // BOXEE
  g_application.EnablePlatformDirectories();
  // End Boxee
  
  g_application.Preflight();
  if (g_application.Create(NULL) != S_OK)
  {
    fprintf(stderr, "ERROR: Unable to create application. Exiting\n");
    return -1;
  }

  if (playlist.Size() > 0)
  {
    g_playlistPlayer.Add(0,playlist);
    g_playlistPlayer.SetCurrentPlaylist(0);
    ThreadMessage tMsg(TMSG_PLAYLISTPLAYER_PLAY, (DWORD) -1);
    g_application.getApplicationMessenger().SendMessage(tMsg, false);    
  }

  try
  {
    while (1)
    {
      g_application.Run();
    }
  }
  catch(...)
  {
    fprintf(stderr, "ERROR: Exception caught on main loop. Exiting\n");
  return -1;
  }

#ifdef __APPLE__
    Cocoa_Destroy_AutoReleasePool(pool);
#endif

  return 0;
}

extern "C"
{
  void mp_msg( int x, int lev, const char *format, ... )
  {
    va_list va;
    static char tmp[2048];
    va_start(va, format);
#ifndef _LINUX
    _vsnprintf(tmp, 2048, format, va);
#else
    vsnprintf(tmp, 2048, format, va);
#endif
    va_end(va);
    tmp[2048 - 1] = 0;

    OutputDebugString(tmp);
  }
}
