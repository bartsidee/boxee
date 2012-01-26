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

#define BOXEE_APP 1

/*****************
 * All platforms
 *****************/
#ifndef HAS_SDL
#define HAS_SDL
#endif

#define HAS_DVD_DRIVE
#define HAS_DVD_SWSCALE
#define HAS_DVDPLAYER
#ifndef _WIN32
#define HAS_EVENT_SERVER
#endif
#define HAS_KARAOKE
#define HAS_RAR
#define HAS_SCREENSAVER
#define HAS_PYTHON
#define HAS_SHOUTCAST
#define HAS_SYSINFO
#define HAS_UPNP
#define HAS_VIDEO_PLAYBACK
#define HAS_VISUALISATION
#define HAS_WEB_SERVER

#define HAS_AC3_CODEC
#ifdef  HAS_DVD_LIBDTS_CODEC
#define HAS_DTS_CODEC
#endif
#define HAS_CDDA_RIPPER

#define HAS_FILESYSTEM
#define HAS_FILESYSTEM_SMB
#define HAS_FILESYSTEM_CDDA
#define HAS_FILESYSTEM_RTV
#define HAS_FILESYSTEM_DAAP
#define HAS_FILESYSTEM_SAP
#define HAS_FILESYSTEM_VTP
#define HAS_FILESYSTEM_HTSP
#define HAS_FILESYSTEM_MMS
#define HAS_CCXSTREAM
#define HAS_NATIVE_APPS
#define HAS_LOCAL_MEDIA
#define HAS_FILESYSTEM_MYTH
#define HAS_FILESYSTEM_HDHOMERUN
#define HAS_FILESYSTEM_TUXBOX

#define _BOXEE_

/*****************
 * Win32 Specific
 *****************/

#ifdef _WIN32
#define HAS_DVD_DRIVE
#define HAS_SDL_JOYSTICK
#define HAS_WIN32_NETWORK
#define HAS_IRSERVERSUITE
#define HAS_AUDIO
#define HAVE_LIBCRYSTALHD 0
#define HAS_WEB_SERVER
#define HAS_WEB_INTERFACE
#define HAVE_LIBSSH
#define HAS_LIBRTMP
#define HAVE_LIBBLURAY
#define HAS_ASAP_CODEC

//#undef HAS_PYTHON // crash win32

#endif

/*****************
 * Mac Specific
 *****************/

#ifdef __APPLE__
//#define HAS_EMBEDDED
//#define HAS_BOXEE_HAL
//#define HAS_DVB
#define HAS_ZEROCONF
#define HAS_AIRPLAY
#define HAS_AIRTUNES
#define HAS_GL
#define HAS_GL2
#define HAS_GLEW
#define HAS_LINUX_NETWORK
#define HAS_SDL_AUDIO
#define HAS_SDL_OPENGL
#define HAS_SDL_WIN_EVENTS
#define HAVE_LIBVDADECODER
#define HAS_JSONRPC
#undef  HAS_EXTERNAL_SAMBA
#define HAS_HARFBUZZ_NG
#define HAS_FRAMELIMITER
#define HAVE_LIBBLURAY
#endif

/*****************
 * Linux Specific
 *****************/

#if defined(_LINUX) && !defined(__APPLE__) && !defined(EMPOWER) && !defined(CANMORE)
#ifndef HAS_SDL_OPENGL
#define HAS_SDL_OPENGL
#endif
#ifdef HAS_AVAHI
#define HAS_ZEROCONF
#endif
#define HAS_LCD
#define HAS_HAL
#define HAS_DBUS
#define HAS_DBUS_SERVER
#define HAS_GL2
#define HAS_GL
#define HAS_GLEW
#define HAS_GLX
#define HAS_LINUX_NETWORK
#define HAS_SDL_AUDIO
#define HAS_LIRC
#define HAS_SDL_WIN_EVENTS
#undef  HAS_EXTERNAL_SAMBA
#define HAS_ALSA
#define HAS_AIRPLAY
#define HAS_AIRTUNES
//#define HAS_DVB
#endif

/*****************
 * Tegra2 Specific
 *****************/

#if defined(EMPOWER)
// HAS_X11 or HAS_OPENKODE is passed in CFLAGS by configure
#ifdef HAS_X11
#define HAS_XCOMPOSITE
#define HAS_X11_EVENTS
#endif
#ifdef HAS_OPENKODE
#define HAS_LINUX_EVENTS
#endif
#define HAS_EGL
#define HAS_GLES 2
#define HAS_OPENMAX
#define HAS_LINUX_NETWORK
#define HAS_LIRC
#define HAS_EMBEDDED
#define HAS_EXTERNAL_SAMBA
#define HAS_ALSA
#undef HAS_DVD_DRIVE
#undef HAS_ZEROCONF
#undef HAS_AVAHI
#undef HAS_SDL_JOYSTICK
#undef HAS_DBUS
#undef HAS_DBUS_SERVER
#undef HAS_CDDA_RIPPER
#undef HAS_RTORRENT
#undef HAS_NATIVE_APPS
#endif

/********************
 * Canmore Specific *
 ********************/

#if defined(CANMORE)
#define DEVICE_PLATFORM "intel.ce4100"
#define EXTERNAL_PYTHON_HOME   "/opt/local"
#define EXTERNAL_PYTHON_PREFIX "/opt/local/lib/python2.4"
#define EXTERNAL_PYTHON_ZIP    "/opt/local/lib/python2.4/python24.zip"
#define HAS_EGL
#define HAS_GLES 2
#define HAS_GDL
//#define USE_EGL_IMAGE //disabled, creates artifacts on textures
#define HAS_LINUX_NETWORK
#define HAS_EMBEDDED
#define HAS_LINUX_EVENTS
#define HAS_EXTERNAL_SAMBA
#define HAS_INTEL_SMD
#define HAS_INTEL_SMD_DD_DECODER
#define HAS_INTEL_SMD_DDPLUS_DECODER
#define HAS_INTEL_SMD_TRUEHD_DECODER
#define HAS_INTEL_SMD_DTS_DECODER
#define HAS_AUDIO_HDMI
#define HAS_INTELCE
#define HAS_LIRC
#define HAS_AIRPLAY
#define HAS_AIRTUNES
#define HAS_JSONRPC
#define HAS_HARFBUZZ_NG
#ifdef HAS_AVAHI
#define HAS_ZEROCONF
#endif
#undef HAS_REMOTECONTROL
#undef HAS_DVD_DRIVE
#undef HAS_SDL_JOYSTICK
#undef HAS_DBUS
#undef HAS_DBUS_SERVER
#undef HAS_CDDA_RIPPER
#undef HAS_RTORRENT
#define HAS_NFS
#define HAS_AFP
#define HAS_CIFS
#define HAS_BMS
#define HAS_UPNP_AV
#undef HAS_FILESYSTEM_CDDA
#undef HAS_FILESYSTEM_RTV
#undef HAS_FILESYSTEM_DAAP
#undef HAS_FILESYSTEM_SAP
#undef HAS_FILESYSTEM_VTP
#undef HAS_FILESYSTEM_HTSP
#undef HAS_CCXSTREAM
#undef HAS_FILESYSTEM_MYTH
#define HAS_FILESYSTEM_HDHOMERUN
#undef HAS_FILESYSTEM_TUXBOX
#define HAS_DVB
#define HAS_SERVER_OTA
#endif

/*****************
 * SVN revision
 *****************/

#ifndef SVN_REV
#define SVN_REV "Unknown"
#endif

#if defined(_LINUX) || defined(__APPLE__)
#include "../config.h"
#endif

/****************************************
 * Additional platform specific includes
 ****************************************/

#ifdef _WIN32
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include "mmsystem.h"
#include "DInput.h"
#include "DSound.h"
#define DSSPEAKER_USE_DEFAULT DSSPEAKER_STEREO
#define LPDIRECTSOUND8 LPDIRECTSOUND
#undef GetFreeSpace
#include "PlatformInclude.h"
#include "D3D9.h"   // On Win32, we're always using DirectX for something, whether it be the actual rendering
#include "D3DX9.h"  // or the reference video clock.
#ifdef HAS_SDL
#include "SDL\SDL.h"
#endif
#endif

#ifdef _LINUX
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include "PlatformInclude.h"
#endif

#if defined(HAS_GL) || defined(HAS_GL2)
#ifdef _WIN32
#include "GL/glew.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/wglew.h>
#elif defined(__APPLE__)
#include <GL/glew.h>
#include <OpenGL/gl.h>
#elif defined(_LINUX) && !defined(EMPOWER)
#include <GL/glew.h>
#include <GL/gl.h>
#endif
#endif

#if HAS_GLES == 2
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif


#define SAFE_DELETE(p)       { delete (p);     (p)=NULL; }
#define SAFE_DELETE_ARRAY(p) { delete[] (p);   (p)=NULL; }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

// Useful pixel colour manipulation macros
#define GET_A(color)            ((color >> 24) & 0xFF)
#define GET_R(color)            ((color >> 16) & 0xFF)
#define GET_G(color)            ((color >>  8) & 0xFF)
#define GET_B(color)            ((color >>  0) & 0xFF)

