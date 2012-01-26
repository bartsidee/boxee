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
#include "GraphicContext.h"
#include "GUIFontManager.h"
#include "../xbmc/Application.h"
#include "LocalizeStrings.h"
#include "../xbmc/GUISettings.h"
#include "../xbmc/Settings.h"
#include "../xbmc/AdvancedSettings.h"
#include "utils/SingleLock.h"
#include "Application.h"
#include "GUISettings.h"
#include "Settings.h"
#include "AdvancedSettings.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "WindowingFactory.h"
#include "SkinInfo.h"
#include "TextureManager.h"
#include "MouseStat.h"
#include "GUIWindowManager.h"
#include "MathUtils.h"
#include "log.h"

using namespace std;

CGraphicContext g_graphicsContext;
extern bool g_fullScreen;

/* quick access to a skin setting, fine unless we starts clearing video settings */
static CSettingInt* g_guiSkinzoom = NULL;

CGraphicContext::CGraphicContext(void)
{
	CSingleLock lock(*this);
  m_iScreenWidth = 0;
  m_iScreenHeight = 0;
  m_iScreenId = 0;
  m_strMediaDir = "";
  m_bCalibrating = false;
  m_Resolution = RES_INVALID;
  m_guiScaleX = m_guiScaleY = 1.0f;
  m_skinResolution = RES_HDTV_720p;
  m_bFullScreenRoot = false;
  m_GraphicsResolution = RES_HDTV_720p;
  m_bRenderLowGraphics = false;

  m_TransformStack.push(TransformMatrix());
}

CGraphicContext::~CGraphicContext(void)
{
}

void CGraphicContext::PushTransform(const TransformMatrix &matrix, bool premult)
{
	CSingleLock lock(*this);
  ASSERT(m_TransformStack.size() < 32);

  TransformMatrix newMat = matrix;

  if(newMat.IsColumnMajor())
    newMat.Transpose();

  if(!premult && m_TransformStack.size() > 0)
    m_TransformStack.push(m_TransformStack.top() * newMat);
  else
    m_TransformStack.push(newMat); 

  g_Windowing.ApplyHardwareTransform(MATRIX_TYPE_MODEL_VIEW, m_TransformStack.top());
}

void CGraphicContext::PopTransform()
{
	CSingleLock lock(*this);
  ASSERT(m_TransformStack.size() > 0);

  if(m_TransformStack.size())
    m_TransformStack.pop();

  if(m_TransformStack.size() > 0)
    g_Windowing.ApplyHardwareTransform(MATRIX_TYPE_MODEL_VIEW, m_TransformStack.top());
  else
    g_Windowing.ApplyHardwareTransform(MATRIX_TYPE_MODEL_VIEW, TransformMatrix());
}

// add a new clip region, intersecting with the previous clip region.
bool CGraphicContext::SetClipRegion(float x, float y, float w, float h, bool bUseSkinRes)
{ 
	CSingleLock lock(*this);
  float z = 0;

  ASSERT(m_clipStack.size() < 32);

  // use defaults if needed
  if(x == 0 && y == 0 && w == 0 && h == 0)
  {
    w = m_iScreenWidth;
    h = m_iScreenHeight;
  }

  CRect finalRect;

  float x1 = x;
  float x2 = x + w;
  float y1 = y;
  float y2 = y + h;

  if(bUseSkinRes)
  {
    m_TransformStack.top().TransformPosition(x1, y1, z);
    m_TransformStack.top().TransformPosition(x2, y2, z);
  } 
  bool bRenderLowRes = GetRenderLowresGraphics();

  if(bRenderLowRes)
  {
    m_GraphicsToScreen.TransformPosition(x1, y1, z);
    m_GraphicsToScreen.TransformPosition(x2, y2, z);
  }

  finalRect.x1 = x1;
  finalRect.y1 = y1;
  finalRect.x2 = x2;
  finalRect.y2 = y2;

  if (m_clipStack.size())
  { 
    // intersect with original clip region
    finalRect.Intersect(m_clipStack.top());
  }

  if (finalRect.IsEmpty())
  {
    return false;
  }

  finalRect.x1 = (int)(finalRect.x1 + 0.5);
  finalRect.x2 = (int)(finalRect.x2 + 0.5);
  finalRect.y1 = (int)(finalRect.y1 + 0.5);
  finalRect.y2 = (int)(finalRect.y2 + 0.5);

  g_Windowing.ApplyClippingRect(finalRect);
  m_clipStack.push(finalRect);

  return true;
}

void CGraphicContext::RestoreClipRegion()
{
	CSingleLock lock(*this);
  if (m_clipStack.size())
    m_clipStack.pop();

  if (m_clipStack.size())
    g_Windowing.ApplyClippingRect(m_clipStack.top());
  else
  {
    CRect emptyRect(0, 0, 0, 0);
    g_Windowing.ApplyClippingRect(emptyRect);
  }
}

void CGraphicContext::PushViewPort(float fx, float fy , float fwidth, float fheight, bool bUseSkinRes)
{
	CSingleLock lock(*this);
  float z = 0;

  ASSERT(m_viewportStack.size() < 32);

  CRect finalRect;

  // use defaults if needed
  if(fx == 0 && fy == 0 && fwidth == 0 && fheight == 0)
  {
    fwidth = m_iScreenWidth;
    fheight = m_iScreenHeight;
  }

  float x1 = fx;
  float x2 = fx + fwidth;
  float y1 = fy;
  float y2 = fy + fheight;
 
  if(bUseSkinRes)
  {
    m_guiTransform.TransformPosition(x1, y1, z);
    m_guiTransform.TransformPosition(x2, y2, z);
  }

  bool bRenderLowRes = GetRenderLowresGraphics();

  if(bRenderLowRes)
  {
    m_GraphicsToScreen.TransformPosition(x1, y1, z);
    m_GraphicsToScreen.TransformPosition(x2, y2, z);
  }

  finalRect.x1 = x1;
  finalRect.y1 = y1;
  finalRect.x2 = x2;
  finalRect.y2 = y2;

  g_Windowing.SetViewPort(finalRect);

  m_viewportStack.push(finalRect);
}

void CGraphicContext::PopViewPort()
{
	CSingleLock lock(*this);
  if(m_viewportStack.size())
    m_viewportStack.pop();

  if(m_viewportStack.size())
    g_Windowing.SetViewPort(m_viewportStack.top());
  else
  {
    CRect emptyRect(0, 0, 0, 0);
    g_Windowing.SetViewPort(emptyRect);
  }
}

const CRect& CGraphicContext::GetViewWindow() const
{
  return m_videoRect;
}

void CGraphicContext::SetViewWindow(float left, float top, float right, float bottom)
{
	CSingleLock lock(*this);
  float z = 0;

  if (m_bCalibrating)
  {
    SetFullScreenViewWindow(m_Resolution);
  }
  else
  {
    m_TransformStack.top().TransformPosition(left, top, z);
    m_TransformStack.top().TransformPosition(right, bottom, z);

    m_videoRect.x1 = left;
    m_videoRect.y1 = top;
    m_videoRect.x2 = right;
    m_videoRect.y2 = bottom;
  }
}

void CGraphicContext::SetFullScreenViewWindow(RESOLUTION &res)
{
  m_videoRect.x1 = (float)g_settings.m_ResInfo[res].Overscan.left;
  m_videoRect.y1 = (float)g_settings.m_ResInfo[res].Overscan.top;
  m_videoRect.x2 = (float)g_settings.m_ResInfo[res].Overscan.right;
  m_videoRect.y2 = (float)g_settings.m_ResInfo[res].Overscan.bottom;
}

void CGraphicContext::SetFullScreenVideo(bool bOnOff)
{
  Lock();
  m_bFullScreenVideo = bOnOff;

#if defined(HAS_VIDEO_PLAYBACK)
  if(m_bFullScreenRoot)
  {
    if(m_bFullScreenVideo)
      g_graphicsContext.SetVideoResolution(g_renderManager.GetResolution());
    else if(g_guiSettings.m_LookAndFeelResolution > RES_DESKTOP)
      g_graphicsContext.SetVideoResolution(g_guiSettings.m_LookAndFeelResolution);
    else
      g_graphicsContext.SetVideoResolution(RES_DESKTOP);    
  }
  else
    g_graphicsContext.SetVideoResolution(RES_WINDOW);
#endif

  SetFullScreenViewWindow(m_Resolution);
  Unlock();
}

bool CGraphicContext::IsFullScreenVideo() const
{
  return m_bFullScreenVideo;
}

bool CGraphicContext::IsCalibrating() const
{
  return m_bCalibrating;
}

void CGraphicContext::SetCalibrating(bool bOnOff)
{
  m_bCalibrating = bOnOff;
}

bool CGraphicContext::IsValidResolution(RESOLUTION res)
{
  if (res >= RES_WINDOW && (size_t) res <= g_settings.m_ResInfo.size())
  {
    return true;
  }

  return false;
}

void CGraphicContext::SetVideoResolution(RESOLUTION res, bool forceUpdate)
{
  // sanity. in case we disconnected the screen that boxee ran on - we fallback to full screen 
  if ((size_t) res >= g_settings.m_ResInfo.size())
    res = g_guiSettings.m_LookAndFeelResolution;

  if(res == RES_INVALID)
  {
    printf("Warning: CGraphicContext::SetVideoResolution with RES_INVALID");
    return;
  }

  RESOLUTION lastRes = m_Resolution;
  
  // If the user asked us to guess, go with desktop
  if (res == RES_AUTORES || !IsValidResolution(res))
  {
    res = RES_DESKTOP;
  }

  // If we are switching to the same resolution and same window/full-screen, no need to do anything
  if (!forceUpdate && res == lastRes && m_bFullScreenRoot == g_advancedSettings.m_fullScreen)
  {
    BuildGUITransform();
    return ;
  }

  if (res >= RES_DESKTOP)
  {
    g_advancedSettings.m_fullScreen = true;
    m_bFullScreenRoot = true;
  }
  else
  {
    g_advancedSettings.m_fullScreen = false;
    m_bFullScreenRoot = false;
  }
  
  Lock();
  
  m_iScreenWidth  = g_settings.m_ResInfo[res].iWidth;
  m_iScreenHeight = g_settings.m_ResInfo[res].iHeight;
  m_iScreenId     = g_settings.m_ResInfo[res].iScreen;
  m_Resolution = res;

  CLog::Log(LOGINFO, "CGraphicContext::SetVideoResolution res = %d width = %d  height = %d screen id = %d",
      res, m_iScreenWidth, m_iScreenHeight, m_iScreenId);
    
  if (g_advancedSettings.m_fullScreen)
  {
#if defined (__APPLE__) || defined (_WIN32)
    bool blankOtherDisplays = g_guiSettings.GetInt("videoscreen.displayblanking")  == BLANKING_ALL_DISPLAYS;
    g_Windowing.SetFullScreen(true,  g_settings.m_ResInfo[res], blankOtherDisplays);
#else
    // special case for true24p
    RESOLUTION_INFO res_info = g_settings.m_ResInfo[res];
    g_Windowing.SetFullScreen(true,  res_info, BLANKING_DISABLED);
#endif
  }
  else if (lastRes >= RES_DESKTOP )
    g_Windowing.SetFullScreen(false, g_settings.m_ResInfo[res], false);
  else
    g_Windowing.ResizeWindow(m_iScreenWidth, m_iScreenHeight, -1, -1);

  BuildGUITransform();

  ResetAllStacks();
    
  // set the mouse resolution
  g_renderManager.Recover();
  g_Mouse.SetResolution(m_iScreenWidth, m_iScreenHeight, 1, 1);
  g_windowManager.SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_WINDOW_RESIZE);
   
  SetFullScreenViewWindow(res);

  Unlock();  
}

void CGraphicContext::SetSkinResolution(RESOLUTION res)
{
  m_skinResolution = res;
  BuildGUITransform();
}

RESOLUTION CGraphicContext::GetVideoResolution() const
{
  return m_Resolution;
}

RESOLUTION CGraphicContext::MatchResolution(int width, int height, float refresh, bool interlaced)
{
  for (size_t r = (size_t) RES_DESKTOP; r < g_settings.m_ResInfo.size(); r++)
  {
    /*
    printf("Testing %d %d %d %f %d against %d %d %f %d\n",
        r,
        g_settings.m_ResInfo[(RESOLUTION)r].iWidth,
        g_settings.m_ResInfo[(RESOLUTION)r].iHeight,
        g_settings.m_ResInfo[(RESOLUTION)r].fRefreshRate,
        g_settings.m_ResInfo[(RESOLUTION)r].dwFlags & D3DPRESENTFLAG_INTERLACED,
        width, height, refresh, interlaced);
        */

    if(width == g_settings.m_ResInfo[(RESOLUTION)r].iWidth &&
        height == g_settings.m_ResInfo[(RESOLUTION)r].iHeight &&
        refresh == g_settings.m_ResInfo[(RESOLUTION)r].fRefreshRate &&
        interlaced == (g_settings.m_ResInfo[(RESOLUTION)r].dwFlags & D3DPRESENTFLAG_INTERLACED))
      return (RESOLUTION)r;

    else
    {
      /*
      if(width != g_settings.m_ResInfo[(RESOLUTION)r].iWidth)
        printf("width is different\n");
      if(height != g_settings.m_ResInfo[(RESOLUTION)r].iHeight)
        printf("height is different\n");
      if(refresh != g_settings.m_ResInfo[(RESOLUTION)r].fRefreshRate)
        printf("refresh is different\n");
      if(interlaced != (g_settings.m_ResInfo[(RESOLUTION)r].dwFlags & D3DPRESENTFLAG_INTERLACED))
        printf("interlace is different\n");
        */
    }
  }

  return RES_INVALID;
}

bool CGraphicContext::IsUsingOverscan()
{
  int left = g_settings.m_ResInfo[m_Resolution].Overscan.left;
  int right = g_settings.m_ResInfo[m_Resolution].Overscan.right;
  int top = g_settings.m_ResInfo[m_Resolution].Overscan.top;
  int bottom = g_settings.m_ResInfo[m_Resolution].Overscan.bottom;      

  if(left != 0 || top != 0 || right != m_iScreenWidth || bottom != m_iScreenHeight)
  {
    return true;
  }

  return false;
}

void CGraphicContext::ResetOverscan(RESOLUTION_INFO &res)
{
  res.Overscan.left = 0;
  res.Overscan.top = 0;
  res.Overscan.right = res.iWidth;
  res.Overscan.bottom = res.iHeight;

  BuildGUITransform();
}

void CGraphicContext::ResetOverscan(RESOLUTION res, OVERSCAN &overscan)
{
  overscan.left = 0;
  overscan.top = 0;
  switch (res)
  {
  case RES_HDTV_1080i:
    overscan.right = 1920;
    overscan.bottom = 1080;
    break;
  case RES_HDTV_720p:
    overscan.right = 1280;
    overscan.bottom = 720;
    break;
  case RES_HDTV_480p_16x9:
  case RES_HDTV_480p_4x3:
  case RES_NTSC_16x9:
  case RES_NTSC_4x3:
  case RES_PAL60_16x9:
  case RES_PAL60_4x3:
    overscan.right = 720;
    overscan.bottom = 480;
    break;
  case RES_PAL_16x9:
  case RES_PAL_4x3:
    overscan.right = 720;
    overscan.bottom = 576;
    break;
  default:
    overscan.right = g_settings.m_ResInfo[res].iWidth;
    overscan.bottom = g_settings.m_ResInfo[res].iHeight;
    break;
  }

  BuildGUITransform();
}

void CGraphicContext::ResetScreenParameters(RESOLUTION res)
{
  // For now these are all on the first screen.
  g_settings.m_ResInfo[res].iScreen = 0;
  
  // 1080i
  switch (res)
  {
  case RES_HDTV_1080i:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 1080);
    g_settings.m_ResInfo[res].iWidth = 1920;
    g_settings.m_ResInfo[res].iHeight = 1080;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    g_settings.m_ResInfo[res].fPixelRatio = 1.0f;
    g_settings.m_ResInfo[res].strMode ="1080i 16:9";
    g_settings.m_ResInfo[res].fRefreshRate = 59.94;
    break;
  case RES_HDTV_720p:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 720);
    g_settings.m_ResInfo[res].iWidth = 1280;
    g_settings.m_ResInfo[res].iHeight = 720;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
    g_settings.m_ResInfo[res].fPixelRatio = 1.0f;
    g_settings.m_ResInfo[res].strMode = "720p 16:9";
    g_settings.m_ResInfo[res].fRefreshRate = 59.94;
    break;
  case RES_HDTV_480p_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
    g_settings.m_ResInfo[res].fPixelRatio = 4320.0f / 4739.0f;
    g_settings.m_ResInfo[res].strMode = "480p 4:3";
    g_settings.m_ResInfo[res].fRefreshRate = 59.94;
    break;
  case RES_HDTV_480p_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 480);
    g_settings.m_ResInfo[res].iWidth = 854;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
    g_settings.m_ResInfo[res].fPixelRatio = 4320.0f / 4739.0f*4.0f / 3.0f;
    g_settings.m_ResInfo[res].strMode = "480p 16:9";
    g_settings.m_ResInfo[res].fRefreshRate = 59.94;
    break;
  case RES_NTSC_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED;
    g_settings.m_ResInfo[res].fPixelRatio = 4320.0f / 4739.0f;
    g_settings.m_ResInfo[res].strMode = "NTSC 4:3";
    g_settings.m_ResInfo[res].fRefreshRate = 59.94;
    break;
  case RES_NTSC_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    g_settings.m_ResInfo[res].fPixelRatio = 4320.0f / 4739.0f*4.0f / 3.0f;
    g_settings.m_ResInfo[res].strMode = "NTSC 16:9";
    g_settings.m_ResInfo[res].fRefreshRate = 59.94;
    break;
  case RES_PAL_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 576);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 576;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED;
    g_settings.m_ResInfo[res].fPixelRatio = 128.0f / 117.0f;
    g_settings.m_ResInfo[res].strMode = "PAL 4:3";
    g_settings.m_ResInfo[res].fRefreshRate = 50;
    break;
  case RES_PAL_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 576);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 576;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    g_settings.m_ResInfo[res].fPixelRatio = 128.0f / 117.0f*4.0f / 3.0f;
    g_settings.m_ResInfo[res].strMode = "PAL 16:9";
    g_settings.m_ResInfo[res].fRefreshRate = 50;
    break;
  case RES_PAL60_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED;
    g_settings.m_ResInfo[res].fPixelRatio = 4320.0f / 4739.0f;
    g_settings.m_ResInfo[res].strMode = "PAL60 4:3";
    g_settings.m_ResInfo[res].fRefreshRate = 59.94;
    break;
  case RES_PAL60_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    g_settings.m_ResInfo[res].fPixelRatio = 4320.0f / 4739.0f*4.0f / 3.0f;
    g_settings.m_ResInfo[res].strMode = "PAL60 16:9";
    g_settings.m_ResInfo[res].fRefreshRate = 59.94;
    break;
  default:
    break;
  }
  ResetOverscan(res, g_settings.m_ResInfo[res].Overscan);
}

float CGraphicContext::GetPixelRatio(RESOLUTION iRes) const
{
  if (iRes >= 0 && iRes < (int)g_settings.m_ResInfo.size())
  return g_settings.m_ResInfo[iRes].fPixelRatio;
  return 0.0f;
}

void CGraphicContext::Clear()
{
  g_Windowing.ClearBuffers(0, 0, 0, 0);
}

void CGraphicContext::CaptureStateBlock()
{
  g_Windowing.CaptureStateBlock();
  }

void CGraphicContext::ApplyStateBlock()
{
  g_Windowing.ApplyStateBlock();
}

void CGraphicContext::BuildGUITransform()
{
  float fFromWidth = 0;
  float fFromHeight = 0;
  float fToPosX = 0;
  float fToPosY = 0;
  float fToWidth = 0;
  float fToHeight = 0;

  Lock();

  if(m_Resolution != RES_INVALID && m_skinResolution != RES_INVALID)
  {
    fFromWidth = (float)g_settings.m_ResInfo[m_skinResolution].iWidth;
    fFromHeight = (float)g_settings.m_ResInfo[m_skinResolution].iHeight;
    fToPosX = (float)g_settings.m_ResInfo[m_Resolution].Overscan.left;
    fToPosY = (float)g_settings.m_ResInfo[m_Resolution].Overscan.top;
    fToWidth = (float)g_settings.m_ResInfo[m_Resolution].Overscan.right - fToPosX;
    fToHeight = (float)g_settings.m_ResInfo[m_Resolution].Overscan.bottom - fToPosY;      
  
    // add additional zoom to compensate for any overscan built in skin
    float fZoom = g_SkinInfo.GetSkinZoom();

    if(!g_guiSkinzoom) // lookup gui setting if we didn't have it already
      g_guiSkinzoom = (CSettingInt*)g_guiSettings.GetSetting("lookandfeel.skinzoom");

    if(g_guiSkinzoom)
      fZoom *= (100 + g_guiSkinzoom->GetData()) * 0.01f;

    fZoom -= 1.0f;
    fToPosX -= fToWidth * fZoom * 0.5f;
    fToWidth *= fZoom + 1.0f;

    // adjust for aspect ratio as zoom is given in the vertical direction and we don't 
    // do aspect ratio corrections in the gui code 
    fZoom = fZoom / g_settings.m_ResInfo[m_Resolution].fPixelRatio;
    fToPosY -= fToHeight * fZoom * 0.5f;
    fToHeight *= fZoom + 1.0f;
    
    m_guiScaleX = fFromWidth / fToWidth;
    m_guiScaleY = fFromHeight / fToHeight;
    TransformMatrix guiScaler = TransformMatrix::CreateScaler(fToWidth / fFromWidth, fToHeight / fFromHeight, fToHeight / fFromHeight);
    TransformMatrix guiOffset = TransformMatrix::CreateTranslation(fToPosX, fToPosY);
    m_guiTransform = guiOffset * guiScaler;

    int graphicsWidth = g_settings.m_ResInfo[m_GraphicsResolution].iWidth;
    int graphicsHeight= g_settings.m_ResInfo[m_GraphicsResolution].iHeight;
    int screenWidth = g_settings.m_ResInfo[m_Resolution].iWidth;
    int screenHeight = g_settings.m_ResInfo[m_Resolution].iHeight;

    guiScaler = TransformMatrix::CreateScaler((float)graphicsWidth / screenWidth, (float)graphicsHeight / screenHeight);
    //guiOffset = TransformMatrix::CreateTranslation(0, screenHeight - graphicsHeight);
    m_GraphicsToScreen = guiScaler;
  }
  else
  {
    m_GraphicsToScreen.Reset();
    m_guiTransform.Reset();
    m_guiScaleX = 1.0f;
    m_guiScaleY = 1.0f;
  }

  Unlock();
}

void CGraphicContext::MapScreenToWorld(float &x, float &y) const
{
  m_guiTransform.InverseTransformPosition(x, y);
}

float CGraphicContext::GetScalingPixelRatio() const
{
  if (m_Resolution == m_skinResolution)
    return GetPixelRatio(m_skinResolution);

  RESOLUTION checkRes = m_skinResolution;
  if (checkRes == RES_INVALID)
    checkRes = m_Resolution;
  // resolutions are different - we want to return the aspect ratio of the video resolution
  // but only once it's been corrected for the skin -> screen coordinates scaling
  float winWidth = (float)g_settings.m_ResInfo[checkRes].iWidth;
  float winHeight = (float)g_settings.m_ResInfo[checkRes].iHeight;
  float outWidth = (float)g_settings.m_ResInfo[m_Resolution].iWidth;
  float outHeight = (float)g_settings.m_ResInfo[m_Resolution].iHeight;
  float outPR = GetPixelRatio(m_Resolution);

  return outPR * (outWidth / outHeight) / (winWidth / winHeight);
}

float CGraphicContext::GetFPS() const
{
  if (m_Resolution != RES_INVALID)
  {
    if (g_settings.m_ResInfo[m_Resolution].fRefreshRate > 0)
      return g_settings.m_ResInfo[m_Resolution].fRefreshRate;
    if (m_Resolution == RES_PAL_4x3 || m_Resolution == RES_PAL_16x9)
      return 50.0f;
    if (m_Resolution == RES_HDTV_1080i)
      return 30.0f;
  }
  return 60.0f;
}

void CGraphicContext::BeginPaint(bool lock)
{
  if (lock) Lock();
}

void CGraphicContext::EndPaint(bool lock)
{
  if (lock) Unlock();
}

bool CGraphicContext::IsFullScreenRoot () const
{
  return m_bFullScreenRoot;
}

bool CGraphicContext::ToggleFullScreenRoot ()
{
#ifdef HAS_EMBEDDED
  return true;
#endif

  RESOLUTION newRes;
  
  if (m_bFullScreenRoot)
  {
    newRes = RES_WINDOW;

    CStdString caption = g_localizeStrings.Get(51507); 
    CStdString description;
#ifdef __APPLE__
    //char cmd[] = { 0xe2, 0x8c, 0x98, 0x2b, 0x46, 0x00 }; // utf-8 for "cmd+F" (cmd as in apple-key symbol)
    //description.Format(g_localizeStrings.Get(51508), cmd);
    description.Format(g_localizeStrings.Get(51508), "cmd+F"); // temp fix until we use a font that supports the "apple" key character
#elif defined (_WIN32)
    description.Format(g_localizeStrings.Get(51508), "\\");
#else
    description.Format(g_localizeStrings.Get(51508), "\\");
#endif
    g_application.m_guiDialogKaiToast.QueueNotification(caption, description);

  }
  else
  {
    if (g_guiSettings.m_LookAndFeelResolution > RES_DESKTOP)
      newRes = g_guiSettings.m_LookAndFeelResolution;
    else
      newRes = RES_DESKTOP;      

#if defined(HAS_VIDEO_PLAYBACK)
    if (g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating())
    {
      /* we need to trick renderer that we are fullscreen already so it gives us a valid value */
      m_bFullScreenRoot = true;
      newRes = g_renderManager.GetResolution();
      m_bFullScreenRoot = false;
    }
#endif

  }

  SetVideoResolution(newRes);
  
  return  m_bFullScreenRoot;
}

void CGraphicContext::SetMediaDir(const CStdString &strMediaDir)
{
  g_TextureManager.SetTexturePath(strMediaDir);
  m_strMediaDir = strMediaDir;
}

bool CGraphicContext::GetRenderLowresGraphics()
{
  return m_bRenderLowGraphics && m_iScreenHeight > 720;
}

void CGraphicContext::SetRenderLowresGraphics(bool bEnable)
{
  CLog::Log(LOGDEBUG, "CGraphicContext::SetRenderLowresGraphics %d", bEnable);

  //m_bRenderLowGraphics = bEnable;
  m_bRenderLowGraphics = false;

  UpdateGraphicsRects();
}

bool CGraphicContext::UpdateGraphicsRects()
{
  CRect srcRect;
  CRect dstRect;
  int renderingWidth, renderingHeight;
  int screenWidth, screenHeight;

  screenWidth = renderingWidth = GetWidth();
  screenHeight = renderingHeight = GetHeight();

  if(screenWidth == 0 || screenHeight == 0)
    return false;

  if(GetRenderLowresGraphics())
  {
    renderingWidth = g_settings.m_ResInfo[GetGraphicsResolution()].iWidth;
    renderingHeight= g_settings.m_ResInfo[GetGraphicsResolution()].iHeight;
  }

  CLog::Log(LOGDEBUG, "CGraphicContext::UpdateGraphicsRects %d %d -> %d %d",
      renderingWidth, renderingHeight, screenWidth, screenHeight);

  if(renderingWidth > screenWidth || renderingHeight > screenHeight)
    return true;

  srcRect.x1 = 0;
  srcRect.y1 = 0;
  srcRect.x2 = renderingWidth;
  srcRect.y2 = renderingHeight;

  dstRect.x1 = 0;
  dstRect.y1 = 0;
  dstRect.x2 = screenWidth;
  dstRect.y2 = screenHeight;

  return true;
}

void CGraphicContext::Flip()
{
  g_Windowing.PresentRender();
}

void CGraphicContext::GetAllowedResolutions(vector<RESOLUTION> &res)
{
  res.clear();  

  res.push_back(RES_WINDOW);
  res.push_back(RES_DESKTOP);
  for (size_t r = (size_t) RES_CUSTOM; r < g_settings.m_ResInfo.size(); r++)
  {
    res.push_back((RESOLUTION) r);
  }
}

void CGraphicContext::ResetAllTransforms()
{
  ResetAllStacks();
  BuildGUITransform();
}

void CGraphicContext::ResetTransformStack()
{
	CSingleLock lock(*this);
  while (m_TransformStack.size() > 1)
    PopTransform();
  PushTransform(TransformMatrix());
}

void CGraphicContext::ResetClipingStack()
{
	CSingleLock lock(*this);
  while (m_clipStack.size())
    RestoreClipRegion();
  SetClipRegion(0, 0, (float)m_iScreenWidth, (float)m_iScreenHeight, false);
}

void CGraphicContext::ResetViewportStack()
{
	CSingleLock lock(*this);
  while (m_viewportStack.size())
    PopViewPort();
  PushViewPort(0, 0, (float)m_iScreenWidth, (float)m_iScreenHeight, false);
}

void CGraphicContext::ResetAllStacks()
{
  ResetTransformStack();
  ResetClipingStack();
  ResetViewportStack();
}

void CGraphicContext::ApplyGuiTransform()
{
  PushTransform(m_guiTransform, true);
  PushViewPort(0, 0, m_iScreenWidth, m_iScreenHeight, false);
  SetClipRegion(0, 0, m_iScreenWidth, m_iScreenHeight, false);
}

void CGraphicContext::RestoreGuiTransform()
{
  PopTransform();
  PopViewPort();
  RestoreClipRegion();
}

