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

/*!
\file GraphicContext.h
\brief 
*/

#ifndef GUILIB_GRAPHICCONTEXT_H
#define GUILIB_GRAPHICCONTEXT_H

#pragma once

#undef XBMC_FORCE_INLINE
#ifdef __GNUC__
// under gcc, inline will only take place if optimizations are applied (-O). this will force inline even whith optimizations.
#define XBMC_FORCE_INLINE __attribute__((always_inline))
#else
#define XBMC_FORCE_INLINE
#endif

#include <vector>
#include <stack>
#include <map>
#include "utils/CriticalSection.h"  // base class
#include "TransformMatrix.h"        // for the members m_guiTransform etc.
#include "Geometry.h"               // for CRect/CPoint
#include "gui3d.h"
#include "StdString.h"
#include "Resolution.h"
#include "../xbmc/utils/SingleLock.h"

enum VIEW_TYPE { VIEW_TYPE_NONE = 0,
                 VIEW_TYPE_LIST,
                 VIEW_TYPE_ICON,
                 VIEW_TYPE_BIG_LIST,
                 VIEW_TYPE_BIG_ICON,
                 VIEW_TYPE_WIDE,
                 VIEW_TYPE_BIG_WIDE,
                 VIEW_TYPE_WRAP,
                 VIEW_TYPE_BIG_WRAP,
                 VIEW_TYPE_AUTO,
                 VIEW_TYPE_MAX };


class CGraphicContext : public CCriticalSection
{
public:
  CGraphicContext(void);
  virtual ~CGraphicContext(void);

  // the following two functions should wrap any
  // GL calls to maintain thread safety
  void BeginPaint(bool lock=false);
  void EndPaint(bool lock=false);

  int GetWidth() const { return m_iScreenWidth; }
  int GetHeight() const { return m_iScreenHeight; }
  float GetFPS() const;
  const CStdString& GetMediaDir() const { return m_strMediaDir; }
  void SetMediaDir(const CStdString& strMediaDir);
  bool IsWidescreen() const { return m_bWidescreen; }
  const CRect& GetViewWindow() const;
  void SetViewWindow(float left, float top, float right, float bottom);
  bool IsFullScreenRoot() const;
  bool ToggleFullScreenRoot();
  void SetFullScreenVideo(bool bOnOff);
  bool IsFullScreenVideo() const;
  bool IsCalibrating() const;
  void SetCalibrating(bool bOnOff);
  bool IsValidResolution(RESOLUTION res);
  void SetVideoResolution(RESOLUTION res, bool forceUpdate = false);
  void SetSkinResolution(RESOLUTION res);
  RESOLUTION GetVideoResolution() const;
  RESOLUTION GetGraphicsResolution() const { return m_GraphicsResolution; }
  RESOLUTION GetSkinResolution() const { return m_skinResolution; }
  RESOLUTION MatchResolution(int width, int height, float refresh, bool intelaced);
  void ResetOverscan(RESOLUTION res, OVERSCAN &overscan);
  void ResetOverscan(RESOLUTION_INFO &resinfo);
  bool IsUsingOverscan();
  void ResetScreenParameters(RESOLUTION res);
  void Lock() { EnterCriticalSection(*this);  }
  void Unlock() { LeaveCriticalSection(*this); }
  float GetPixelRatio(RESOLUTION iRes) const;
  void CaptureStateBlock();
  void ApplyStateBlock();
  void Clear();
  void GetAllowedResolutions(std::vector<RESOLUTION> &res);

  // output scaling
  float GetScalingPixelRatio() const;
  void Flip();
  void MapScreenToWorld(float &x, float &y) const;
  inline TransformMatrix GetGuiTransform() { return m_guiTransform; }
  inline TransformMatrix GetGraphicsTransform() { return m_GraphicsToScreen; }
  
  inline void ScaleFinalCoords(float &x, float &y, float &z) const XBMC_FORCE_INLINE { m_guiTransform.TransformPosition(x, y, z); }

  inline float GetGUIScaleX() const XBMC_FORCE_INLINE { return m_guiScaleX; }
  inline float GetGUIScaleY() const XBMC_FORCE_INLINE { return m_guiScaleY; }
  inline color_t MergeAlpha(color_t color) const XBMC_FORCE_INLINE
  {
	  CSingleLock lock(*this);
    color_t alpha = m_TransformStack.top().TransformAlpha((color >> 24) & 0xff);
    if (alpha > 255) alpha = 255;
    return ((alpha << 24) & 0xff000000) | (color & 0xffffff);
  }

  inline float GetFinalTransformAlpha() { CSingleLock lock(*this); return m_TransformStack.top().alpha; }

  bool GetRenderLowresGraphics();
  void SetRenderLowresGraphics(bool bEnable);
  bool UpdateGraphicsRects();

  void ResetAllTransforms();

  void ResetTransformStack();
  void ResetClipingStack();
  void ResetViewportStack();
  void ResetAllStacks();

  void PushTransform(const TransformMatrix &matrix, bool premult = false);
  void PopTransform();

  void PushViewPort(float fx, float fy , float fwidth, float fheight, bool useSkinRes = true);
  void PopViewPort();
  CRect GetViewPort() { CSingleLock lock(*this); return m_viewportStack.top(); }

  bool SetClipRegion(float x, float y, float w, float h, bool useSkinRes = true);
  void RestoreClipRegion();

  void ApplyGuiTransform();
  void RestoreGuiTransform();

protected:
  void SetFullScreenViewWindow(RESOLUTION &res);
  void BuildGUITransform();

  int m_iScreenHeight;
  int m_iScreenWidth;
  int m_iScreenId;
  int m_iBackBufferCount;
  bool m_bWidescreen;
  CStdString m_strMediaDir;
  CRect m_videoRect;
  bool m_bFullScreenRoot;
  bool m_bFullScreenVideo;
  bool m_bCalibrating;
  RESOLUTION m_Resolution;
  
private:
  RESOLUTION m_skinResolution;
  RESOLUTION m_GraphicsResolution;
  float m_guiScaleX;
  float m_guiScaleY;

  // Skin Res->Screen Res (including overscan)
  TransformMatrix m_guiTransform;

  // Use this setting to determine the graphics real resolution
  TransformMatrix m_GraphicsToScreen;
  bool m_bRenderLowGraphics;
 
  std::stack<CRect>    m_viewportStack;
  std::stack<CRect>    m_clipStack;
  std::stack<TransformMatrix> m_TransformStack;
};

/*!
 \ingroup graphics
 \brief 
 */
extern CGraphicContext g_graphicsContext;
#endif


