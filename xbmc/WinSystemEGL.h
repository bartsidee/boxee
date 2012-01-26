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

#ifndef WINDOW_SYSTEM_EGL_H
#define WINDOW_SYSTEM_EGL_H

#include <EGL/egl.h>
#ifndef _WIN32
#include <EGL/eglext.h>
#include "WinSystem.h"
#define EGL_BASE_CLASS CWinSystemBase
#else
#include "WinSystemWin32.h"
#define EGL_BASE_CLASS CWinSystemWin32
#endif

#if defined(EMPOWER) && defined(HAS_OPENKODE)
struct KDWindow;
struct KDDesktopNV;
struct KDDisplayNV;
#endif

#ifdef HAS_X11
#include <X11/Xlib.h>
#endif

#if defined(HAS_GDL)
#include <libgdl.h>
#endif

class CWinSystemEGL : public EGL_BASE_CLASS
{
public:
  CWinSystemEGL();
  virtual ~CWinSystemEGL();

  // CWinSystemBase
  virtual bool InitWindowSystem();
  virtual bool DestroyWindowSystem();
  virtual bool CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction);
  virtual bool DestroyWindow();
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays);
  virtual void UpdateResolutions();

#ifdef HAS_X11
  Display* GetDisplay() { return m_x11Display; }
#endif
  EGLDisplay GetEGLDisplay() { return m_display; }

protected:
    NativeWindowType  m_nativeWindow;
    EGLDisplay        m_display;
    EGLSurface        m_surface;
    EGLConfig         m_config;
    EGLContext        m_context;
    EGLint            buffering;
#if defined(EMPOWER) && defined(HAS_OPENKODE)
    KDWindow*         m_kdWindow;
    KDDesktopNV*      m_kdDesktop;
    KDDisplayNV*      m_kdDisplay;
#endif
#if defined(HAS_X11)
    Display*          m_x11Display;
#endif
};

#endif // WINDOW_SYSTEM_H

