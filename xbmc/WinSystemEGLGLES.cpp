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

#if defined(HAS_EGL) && defined(HAS_GLES)

#if defined(_WIN32)
#pragma comment (lib,"libEGL.lib")
#pragma comment (lib,"libGLESv2.lib")
#endif

#include "WinSystemEGLGLES.h"

CWinSystemEGLGLES g_Windowing;

CWinSystemEGLGLES::CWinSystemEGLGLES()
{
}

CWinSystemEGLGLES::~CWinSystemEGLGLES()
{
}

bool CWinSystemEGLGLES::PresentRenderImpl()
{    
  eglSwapBuffers(m_display, m_surface);
  return true;
}

void CWinSystemEGLGLES::SetVSyncImpl(bool enable)
{
  eglSwapInterval(m_display, 0);
}

bool CWinSystemEGLGLES::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CWinSystemEGL::ResizeWindow(newWidth, newHeight, newLeft, newTop);
  CRenderSystemGLES::ResetRenderSystem(newWidth, newHeight, true, 0);
  
  return true;
}

bool CWinSystemEGLGLES::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  CWinSystemEGL::SetFullScreen(fullScreen, res, blankOtherDisplays);
  CRenderSystemGLES::ResetRenderSystem(res.iWidth, res.iHeight, true, 0);
  
  return true;
}

#endif
