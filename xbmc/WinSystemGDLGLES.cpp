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

#if defined(HAS_GDL) && defined(HAS_GLES)

#include "WinSystemGDLGLES.h"

CWinSystemGDLGLES g_Windowing;

CWinSystemGDLGLES::CWinSystemGDLGLES()
{
}

CWinSystemGDLGLES::~CWinSystemGDLGLES()
{
}

bool CWinSystemGDLGLES::PresentRenderImpl()
{    
  eglSwapBuffers(m_eglBinding.GetDisplay(), m_eglBinding.GetSurface());
  return true;
}

void CWinSystemGDLGLES::SetVSyncImpl(bool enable)
{
  eglSwapInterval(m_eglBinding.GetDisplay(), 0);
}

bool CWinSystemGDLGLES::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CWinSystemGDL::ResizeWindow(newWidth, newHeight, newLeft, newTop);
  CRenderSystemGLES::ResetRenderSystem(newWidth, newHeight, true, 0);
  
  return true;
}

bool CWinSystemGDLGLES::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  CWinSystemGDL::SetFullScreen(fullScreen, res, blankOtherDisplays);
  CRenderSystemGLES::ResetRenderSystem(res.iWidth, res.iHeight, true, 0);
  
  return true;
}

void* CWinSystemGDLGLES::GetRenderContext()
{
  return CWinSystemGDL::GetRenderContext();
}

#endif
