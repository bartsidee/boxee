#ifndef WINDOW_SYSTEM_GDL_H
#define WINDOW_SYSTEM_GDL_H

#pragma once

/*
 *      Copyright (C) 2008-2010 Boxee
 *      http://www.boxee.tv
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
#include "WinBindingEGL.h"
#include "Geometry.h"

#include <vector>
#include <libgdl.h>

#define GDL_VIDEO_PLANE           GDL_PLANE_ID_UPP_B  // bottom
#define GDL_GRAPHICS_PLANE        GDL_PLANE_ID_UPP_D  // on top for overlays
#define GDL_FLASH_GRAPHICS_PLANE  GDL_PLANE_ID_UPP_C  // middle for pages with embedded video

class CWinSystemGDL : public CWinSystemBase
{
public:
  CWinSystemGDL();
  virtual ~CWinSystemGDL();

  // CWinSystemBase
  virtual bool InitWindowSystem();
  virtual bool DestroyWindowSystem();
  virtual bool CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction);
  virtual bool DestroyWindow();
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays);
  virtual void UpdateResolutions();
  
  virtual void NotifyAppActiveChange(bool bActivated);

  virtual bool Minimize();
  virtual bool Restore() ;
  virtual bool Hide();
  virtual bool Show(bool raise = true);
  virtual void* GetRenderContext();
  
  bool EnableHardwareScaler(bool bEnable, CRect src, CRect dst, gdl_plane_id_t plane = GDL_GRAPHICS_PLANE);
  bool IsHardwareScalerEnabled(gdl_plane_id_t plane = GDL_GRAPHICS_PLANE);
  bool GetPlaneRect(gdl_plane_id_t plane, CRect& rect, gdl_plane_attr_t attrib);
  bool EnableHDMIClamp(bool enable);
  bool EnableHDCP(bool enable);
  bool ConfigureHDMIOutput();

  EGLDisplay GetEGLDisplay();

protected:  
  void GetNativeDisplayResolution(std::vector<RESOLUTION_INFO>& resolutions);
  bool ReadEDID(unsigned char** edid_data, int* len);
  int GetEDIDBlockType(unsigned char* block);
  bool GetNativeResolutionFromEDID(unsigned char* edid_data, int edid_len, std::vector<RESOLUTION_INFO>& resolutions);
  gdl_gamma_t GetGDLGamma(GAMMA gamma);

  CWinBindingEGL m_eglBinding;
  gdl_plane_id_t m_gdlPlane;
};

#endif // WINDOW_SYSTEM_GDL_H

