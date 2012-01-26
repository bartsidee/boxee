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

pascal OSErr openEventsHandler(const AppleEvent *openEvent, AppleEvent* reply, long refCon)
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
    ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_PLAY, (DWORD) -1};
    g_application.getApplicationMessenger().SendMessage(tMsg, false);
  }
  
  return noErr;
}
#endif

int main(int argc, char* argv[])
{
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
  
  // Run Boxee test function before starting the application
  BOXEE::Boxee::GetInstance().RunTest();
  
  CFileItemList playlist;
#ifdef _LINUX

#if 0
  struct rlimit rlim;
  rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
  if (setrlimit(RLIMIT_CORE, &rlim) == -1)
    CLog::Log(LOGDEBUG, "Failed to set core size limit (%s)", strerror(errno));
#endif

  // Prevent child processes from becoming zombies on exit if not waited upon. See also Util::Command
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));

  sa.sa_flags = SA_NOCLDWAIT;
  sa.sa_handler = SIG_IGN;
  sigaction(SIGCHLD, &sa, NULL);
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
      
      else if (strnicmp(argv[i], "-v", 2) == 0)
      {
        g_application.SetVerbose(true);
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
      else if (strnicmp(argv[i], "-l", 2) == 0 || strnicmp(argv[i], "--lircdev", 9) == 0)
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
      else if (strnicmp(argv[i], "-n", 2) == 0 || strnicmp(argv[i], "--nolirc", 8) == 0)
         g_RemoteControl.setUsed(false);
#endif
      else if (argv[i][0] != '-')
      {
        CFileItemPtr pItem(new CFileItem(argv[i]));
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
    ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_PLAY, (DWORD) -1};
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
