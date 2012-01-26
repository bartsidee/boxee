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

#ifdef HAS_EGL

#if defined(EMPOWER) && defined(HAS_OPENKODE)
// kd.h uses st_mtime which we override (for reasons I can't remember)
#undef st_mtime
#include <KD/NV_initialize.h>
#include <KD/kd.h>
#include <KD/NV_display.h>
#endif

#include "WinSystemEGL.h"
#include "SpecialProtocol.h"
#include "Settings.h"
#include "Texture.h"
#include "log.h"

#ifdef HAS_GDL
#include "WinSystemGDL.h"
#endif

CWinSystemEGL::CWinSystemEGL() : EGL_BASE_CLASS()
{
  m_eWindowSystem = WINDOW_SYSTEM_OPENKODE;
  m_surface = EGL_NO_SURFACE;
  m_context = EGL_NO_CONTEXT;
  m_display = EGL_NO_DISPLAY;
#if defined(EMPOWER) && defined(HAS_OPENKODE)
  m_kdWindow = KD_NULL;
  m_kdDesktop = KD_NULL;
  m_kdDisplay = KD_NULL;
#endif
}

CWinSystemEGL::~CWinSystemEGL()
{
  DestroyWindowSystem();
};

bool CWinSystemEGL::InitWindowSystem()
{
  if (!EGL_BASE_CLASS::InitWindowSystem())
    return false;

#if defined(EMPOWER) && defined(HAS_OPENKODE)
  kdInitializeNV();
#endif
  
  return true;
}

bool CWinSystemEGL::DestroyWindowSystem()
{  
#if defined(EMPOWER) && defined(HAS_OPENKODE)
  kdTerminateNV();
#endif

  return true;
}

#ifdef HAS_X11
static int ApplicationErrorHandler(Display *display, XErrorEvent *theEvent)
{
  CLog::Log(LOGERROR,
       "Ignoring Xlib error: error code %d request code %d\n",
       theEvent->error_code,
       theEvent->request_code) ;

    /* No exit! - but keep lint happy */

    return 0 ;
}
#endif

bool CWinSystemEGL::CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{
#if defined(EMPOWER) && defined(HAS_OPENKODE)
  KDboolean b;
  KDDisplayModeNV  mode;
  KDint desktopSize[2] = { res.iWidth, res.iHeight };
#endif
#ifdef _WIN32
  EGL_BASE_CLASS::CreateNewWindow(name, fullScreen, res, userFunction);
#endif

  m_nWidth = res.iWidth;
  m_nHeight = res.iHeight;
  m_bFullScreen = fullScreen;

  EGLBoolean eglStatus;
  EGLint     configCount;
  EGLConfig* configList = NULL;  

#if defined(EMPOWER) && defined(HAS_OPENKODE)
  //CLog::Log(LOGDEBUG, "NV: GetDisplay");
  m_kdDisplay = kdGetDisplayNV(KD_DEFAULT_DISPLAY_NV, KD_NULL);
  if (!m_kdDisplay)
  {
    CLog::Log(LOGERROR, "Could not obtain KDDisplayNV pointer");
    return false;
  }
  b = KD_FALSE;
  //CLog::Log(LOGDEBUG, "NV: SetDisplayProperty");
  kdSetDisplayPropertybvNV(m_kdDisplay, KD_DISPLAYPROPERTY_ENABLED_NV, &b);
  kdReleaseDisplayNV(m_kdDisplay);

  // MZL: enable HDMI display
  //CLog::Log(LOGDEBUG, "NV: SetDisplayPropertybyNV");
  kdSetDisplayPropertybvNV(m_kdDisplay, KD_DISPLAYPROPERTY_ENABLED_NV, &b);
  m_kdDisplay = kdGetDisplayNV("Tegra:HDMI0", KD_NULL);
  if (!m_kdDisplay)
  {
    CLog::Log(LOGERROR, "Could not obtain KDDisplayNV pointer");
    return false;
  }
  b = KD_TRUE;
  kdSetDisplayPropertybvNV(m_kdDisplay, KD_DISPLAYPROPERTY_ENABLED_NV, &b);
  kdSetDisplayPropertycvNV(m_kdDisplay, KD_DISPLAYPROPERTY_DESKTOP_NAME_NV, KD_DEFAULT_DESKTOP_NV);

  mode.width   = res.iWidth;
  mode.height  = res.iHeight;
  mode.refresh = 60;
  //CLog::Log(LOGDEBUG, "NV: SetDisplayPropertyNV");
  if (kdSetDisplayModeNV(m_kdDisplay, &mode, KD_DISPLAY_PROTOCOL_AUTOMATIC_NV))
  {
    CLog::Log(LOGERROR, "Could not set display mode\n");
    return false;
  }

  //CLog::Log(LOGDEBUG, "NV: GetDesktopNV");
  m_kdDesktop = kdGetDesktopNV(KD_DEFAULT_DESKTOP_NV, KD_NULL);
  if (!m_kdDesktop)
  {
    CLog::Log(LOGERROR, "Could not obtain KDDesktopNV pointer");
    return false;
  } 

  //CLog::Log(LOGDEBUG, "NV: SetDesktopivNV");
  if (kdSetDesktopPropertyivNV(m_kdDesktop, KD_DESKTOPPROPERTY_SIZE_NV, desktopSize))
  {
    CLog::Log(LOGERROR, "Could not set desktop size");
    return false;
  }
#endif

#ifdef HAS_X11
  m_x11Display = XOpenDisplay(NULL);
  if (!m_x11Display)
  {
    CLog::Log(LOGERROR, "Could not open X11");
    return false;
  }

  XSetErrorHandler(ApplicationErrorHandler) ;
#endif

  //CLog::Log(LOGDEBUG, "eglGetDisplay");
#if defined(HAS_OPENKODE) || defined(HAS_GDL)
  m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
#elif defined(_WIN32)
  m_display = eglGetDisplay(m_hDC);
#elif defined(HAS_X11)
  m_display = eglGetDisplay((EGLNativeDisplayType) m_x11Display);
#endif
  if (m_display == EGL_NO_DISPLAY) 
  {
    CLog::Log(LOGERROR, "EGL failed to obtain display");
    return false;
  }
   
  //CLog::Log(LOGDEBUG, "eglInitialize");
  if (!eglInitialize(m_display, 0, 0)) 
  {
    CLog::Log(LOGERROR, "EGL failed to initialize");
    return false;
  } 
  
  EGLint configAttrs[] = {
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_DEPTH_SIZE,     16,
        EGL_STENCIL_SIZE,    8,
        EGL_SAMPLE_BUFFERS,  0,
        EGL_SAMPLES,         0,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
  };

  // Find out how many configurations suit our needs  
  //CLog::Log(LOGDEBUG, "eglChooseConfig");
  eglStatus = eglChooseConfig(m_display, configAttrs, NULL, 0, &configCount);
  if (!eglStatus || !configCount) 
  {
    CLog::Log(LOGERROR, "EGL failed to return any matching configurations");
    return false;
  }
    
  // Allocate room for the list of matching configurations
  configList = (EGLConfig*)malloc(configCount * sizeof(EGLConfig));
  if (!configList) 
  {
    CLog::Log(LOGERROR, "kdMalloc failure obtaining configuration list");
    return false;
  }

  // Obtain the configuration list from EGL
  eglStatus = eglChooseConfig(m_display, configAttrs,
                                configList, configCount, &configCount);
  if (!eglStatus || !configCount) 
  {
    CLog::Log(LOGERROR, "EGL failed to populate configuration list");
    return false;
  }
  
  // Select an EGL configuration that matches the native window
  m_config = configList[0];

  EGLint* attribList = NULL;

#ifdef EMPOWER
  EGLint windowAttrs[3];
  int windowIndex = 0;
  windowAttrs[windowIndex++] = EGL_RENDER_BUFFER;
  windowAttrs[windowIndex++] = EGL_BACK_BUFFER;
  windowAttrs[windowIndex++] = EGL_NONE;
  attribList = windowAttrs;
#endif

#ifdef HAS_OPENKODE
  //CLog::Log(LOGDEBUG, "KD: kdCreateWindow");
  m_kdWindow = kdCreateWindow(m_display, m_config, KD_NULL);
  if (!m_kdWindow)
  {
    CLog::Log(LOGERROR, "Error creating native window");
    return false;
  }

  //CLog::Log(LOGDEBUG, "KD: kdRealizeWindow");
  if (kdRealizeWindow(m_kdWindow, &m_nativeWindow))
  {
    CLog::Log(LOGERROR, "Could not realize native window");
    return false;
  }

#elif defined HAS_X11
  int screen = DefaultScreen(m_x11Display);

  XSetWindowAttributes windowAttributes;
  windowAttributes.colormap     = DefaultColormap(m_x11Display, screen);
  windowAttributes.border_pixel = 0;
  windowAttributes.event_mask   = ExposureMask           |
                                  VisibilityChangeMask   |
                                  KeyPressMask           |
                                  KeyReleaseMask         |
                                  ButtonPressMask        |
                                  ButtonReleaseMask      |
                                  PointerMotionMask      |
                                  StructureNotifyMask    |
                                  SubstructureNotifyMask |
                                  FocusChangeMask;

  m_nativeWindow = (NativeWindowType) XCreateWindow( m_x11Display,
                              RootWindow(m_x11Display, screen),
                              0, 0,     // x/y position of top-left outside corner of the window
                              res.iWidth, res.iHeight, // Width and height of window
                              0,        // Border width
                              DefaultDepth(m_x11Display, screen),
                              InputOutput,
                              DefaultVisual(m_x11Display, screen),
                              CWBorderPixel | CWColormap | CWEventMask,
                              &windowAttributes );

  XSetStandardProperties(m_x11Display, (Window) m_nativeWindow, name, name, None, NULL, 0, NULL);

  Atom wmDeleteMessage = XInternAtom(m_x11Display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(m_x11Display, (Window) m_nativeWindow, &wmDeleteMessage, 1);

  if (fullScreen && !SetFullScreen(fullScreen, res, false))
  {
    return false;
  }

   XMapRaised(m_x11Display, (Window) m_nativeWindow);
#endif

#ifdef _WIN32
  m_nativeWindow = m_hWnd;
 
  DEVMODE dm;
  ZeroMemory(&dm, sizeof(dm));
  dm.dmSize = sizeof(dm);
  EnumDisplaySettingsEx(NULL, ENUM_CURRENT_SETTINGS, &dm, 0);

  m_nLeft = (dm.dmPelsWidth / 2) - (m_nWidth / 2);
  m_nTop = (dm.dmPelsHeight / 2) - (m_nHeight / 2);

  RECT rc;
  rc.left = m_nLeft;
  rc.top = m_nTop;
  rc.right = rc.left + m_nWidth;
  rc.bottom = rc.top + m_nHeight;
  AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, false );
  SetWindowPos(m_hWnd, 0, rc.left, rc.top, 0, 0, SWP_NOSIZE);
#endif

#if defined(HAS_GDL)
  m_nativeWindow = (NativeWindowType)GDL_GRAPHICS_PLANE;
#endif

  m_surface = eglCreateWindowSurface(m_display, m_config, m_nativeWindow, attribList);
  if (!m_surface)
  { 
    CLog::Log(LOGERROR, "EGL couldn't create window");
    return false;
  }

#ifdef CANMORE
  eglStatus = eglBindAPI(EGL_OPENGL_ES_API);
  if (!eglStatus) 
  {
    CLog::Log(LOGERROR, "EGL failed to bind API");
    return false;
  }
#endif

  EGLint contextAttrs[] = 
  {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };

  // Create an EGL context
  //CLog::Log(LOGDEBUG, "eglCreateContext");
  m_context = eglCreateContext(m_display, m_config, NULL, contextAttrs);
  if (!m_context) 
  {
    CLog::Log(LOGERROR, "EGL couldn't create context");
    return false;
  }

  // Make the context and surface current for rendering
  eglStatus = eglMakeCurrent(m_display, m_surface, m_surface, m_context);
  if (!eglStatus) 
  {
    CLog::Log(LOGERROR, "EGL couldn't make context/surface current");
    return false;
  }
 
  free(configList);

  eglSwapInterval(m_display, 0);

  m_bWindowCreated = true;

  CLog::Log(LOGINFO, "Window creation complete");
  return true;
}

bool CWinSystemEGL::DestroyWindow()
{
  EGLBoolean eglStatus;
  if (m_context != EGL_NO_CONTEXT)
  {
    eglStatus = eglDestroyContext(m_display, m_context);
    if (!eglStatus)
      CLog::Log(LOGERROR, "Error destroying EGL context");
    m_context = EGL_NO_CONTEXT;
  }

  if (m_surface != EGL_NO_SURFACE)
  {
    eglStatus = eglDestroySurface(m_display, m_surface);
    if (!eglStatus)
      CLog::Log(LOGERROR, "Error destroying EGL surface");
    m_surface = EGL_NO_SURFACE;
  }

  if (m_display != EGL_NO_DISPLAY)
  {
    eglStatus = eglTerminate(m_display);
    if (!eglStatus)
      CLog::Log(LOGERROR, "Error terminating EGL");
    m_display = EGL_NO_DISPLAY;
  }

#ifdef HAS_OPENKODE
  if (m_kdWindow)
  {
    if (kdDestroyWindow(m_kdWindow))
      CLog::Log(LOGERROR, "Error destroying native window");

    m_kdWindow = KD_NULL;
    m_nativeWindow = (NativeWindowType)0;
  }

  // Release display
  if (m_kdDisplay)
  {
    if (kdReleaseDisplayNV(m_kdDisplay))
      CLog::Log(LOGERROR, "Error destroying native display");
    m_kdDisplay = KD_NULL;
  }
#endif

  return true;
}
    
bool CWinSystemEGL::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  if (m_nWidth == newWidth && m_nHeight == newHeight) return true;

  m_nWidth  = newWidth;
  m_nHeight = newHeight;

#ifdef HAS_X11
  XResizeWindow(m_x11Display, (Window) m_nativeWindow, m_nWidth, m_nHeight);
#endif

  return true;
}

bool CWinSystemEGL::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{  
  m_nWidth      = res.iWidth;
  m_nHeight     = res.iHeight;
  m_bFullScreen = fullScreen;

#if defined(HAS_XRANDR)
  if(m_bFullScreen)
  {
    XOutput out;
    XMode mode;
    out.name = res.strOutput;
    mode.w   = res.iWidth;
    mode.h   = res.iHeight;
    mode.hz  = res.fRefreshRate;
    mode.id  = res.strId;
    g_xrandr.SetMode(out, mode);
  }
  else
    g_xrandr.RestoreState();
#endif

#ifdef HAS_X11
  XClientMessageEvent xMessage;
  xMessage.type = ClientMessage;
  xMessage.serial = 0;
  xMessage.send_event = True;
  xMessage.window = (Window) m_nativeWindow;
  xMessage.message_type = XInternAtom(m_x11Display, "_NET_WM_STATE", True);
  xMessage.format = 32;
  xMessage.data.l[0] = (fullScreen ? 1 : 0);
  xMessage.data.l[1] = XInternAtom(m_x11Display, "_NET_WM_STATE_FULLSCREEN", True);
  xMessage.data.l[2] = 0;

  XSendEvent(m_x11Display, DefaultRootWindow(m_x11Display), False, SubstructureRedirectMask | SubstructureNotifyMask, (XEvent*)&xMessage);
#endif
  return true;
}

// TODO: Fix me
void CWinSystemEGL::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();
  
  // Add desktop resolution
#ifdef _WIN32
  DEVMODE dm;
  ZeroMemory(&dm, sizeof(dm));
  dm.dmSize = sizeof(dm);
  EnumDisplaySettingsEx(NULL, ENUM_CURRENT_SETTINGS, &dm, 0);

  UpdateDesktopResolution(g_settings.m_ResInfo[RES_DESKTOP], 0, dm.dmPelsWidth, dm.dmPelsHeight, 60);
#else
  UpdateDesktopResolution(g_settings.m_ResInfo[RES_DESKTOP], 0, 1920, 1080, 60);
#endif

}

#endif
