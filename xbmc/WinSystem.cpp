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

#include "WinSystem.h"
#include "GraphicContext.h"
#include "Settings.h"

CWinSystemBase::CWinSystemBase()
{
  m_nWidth = 0;
  m_nHeight = 0;
  m_nTop = 0;
  m_nLeft = 0;
  m_bWindowCreated = false;
  m_bFullScreen = false;
  m_nScreen = 0;
  m_bBlankOtherDisplay = false;
  m_bSupportTrue24p = false;
}

CWinSystemBase::~CWinSystemBase()
{

}

bool CWinSystemBase::InitWindowSystem()
{
  UpdateResolutions();

  return true;
}

void CWinSystemBase::UpdateDesktopResolution(RESOLUTION_INFO& newRes, int screen, int width, int height, float refreshRate, bool interlaced)
{
  CStdString interlacedStr;

  newRes.Overscan.left = 0;
  newRes.Overscan.top = 0;
  newRes.Overscan.right = width;
  newRes.Overscan.bottom = height;
  newRes.iScreen = screen;
  newRes.bFullScreen = true;
  newRes.iSubtitles = (int)(0.965 * height);
  newRes.fRefreshRate = refreshRate;
  newRes.fPixelRatio = 1.0f;  
  newRes.iWidth = width;
  newRes.iHeight = height;
  
  newRes.dwFlags = 0;
  if (interlaced)
  {
    newRes.dwFlags |= D3DPRESENTFLAG_INTERLACED;
    interlacedStr = "i";
  }
  else
  {
    newRes.dwFlags |= D3DPRESENTFLAG_PROGRESSIVE;
    interlacedStr = "p";
  }

  if (width == 1280 && height == 720 && refreshRate < 59.95f && refreshRate > 59.93f)
  {
    newRes.strMode = CStdString("720") + interlacedStr;
  }
  else if (width == 1280 && height == 720 && refreshRate < 50.1f && refreshRate > 49.99f)
  {
    newRes.strMode = CStdString("720") + interlacedStr + CStdString(" 50Hz");
  }
  else if (width == 720 && height == 480 && interlacedStr == "p")
  {
    newRes.strMode = CStdString("480p");
    newRes.fPixelRatio = 1.18f;
  }
  else if (width == 720 && height == 480 && interlacedStr == "i")
  {
    newRes.strMode = CStdString("NTSC");
    newRes.fPixelRatio = 1.18f;
  }
  else if (width == 720 && height == 576 && interlacedStr == "i")
  {
    newRes.strMode = CStdString("PAL");
    newRes.fPixelRatio = 1.416f;
  }
  else if (width == 720 && height == 576 && interlacedStr == "p")
  {
    newRes.strMode = CStdString("576p");
    newRes.fPixelRatio = 1.416f;
  }
  else if (width == 1920 && height == 1080 && refreshRate < 59.95f && refreshRate > 59.93f)
  {
    newRes.strMode = CStdString("1080") + interlacedStr;
  }
  else if (width == 1920 && height == 1080 && refreshRate < 50.1f && refreshRate > 49.99f)
  {
    newRes.strMode = CStdString("1080") + interlacedStr + CStdString(" 50Hz");
  }
  else if (width == 1920 && height == 1080 && refreshRate < 24.1f && refreshRate > 23.8f)
  {
    newRes.strMode = CStdString("1080") + interlacedStr + CStdString(" 24Hz");
  }
  else
  {
    newRes.strMode.Format("%dx%d", width, height);
    if (refreshRate > 1)
    {
      if (refreshRate - floorf(refreshRate) < 0.001f)
      {
        newRes.strMode.Format("%s (%.0fHz%s)",  newRes.strMode, refreshRate, interlaced ? ", interlaced" : "");
      }
      else
      {
        newRes.strMode.Format("%s (%.2fHz%s)",  newRes.strMode, refreshRate, interlaced ? ", interlaced" : "");
      }
    }
  }
  
  if (screen > 0)
  {
    newRes.strMode.Format("%s #%d", newRes.strMode, screen + 1);    
  }
}

void CWinSystemBase::UpdateResolutions()
{
  // add the window res - defaults are fine.
  RESOLUTION_INFO& window = g_settings.m_ResInfo[RES_WINDOW];
  window.iSubtitles = (int)(0.965 * 480);
  window.iWidth = 854;
  window.iHeight = 480;
  window.iScreen = 0;
  window.fPixelRatio = 1.0f;
  window.strMode = "Windowed";
}

void CWinSystemBase::SetWindowResolution(int width, int height)
{
  RESOLUTION_INFO& window = g_settings.m_ResInfo[RES_WINDOW];
  window.iWidth = width;
  window.iHeight = height;
  window.iSubtitles = (int)(0.965 * window.iHeight);
  window.strMode = "Windowed";
  g_graphicsContext.ResetOverscan(window);
}
