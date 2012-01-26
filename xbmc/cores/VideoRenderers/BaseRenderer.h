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

#include "Resolution.h"
#include "Geometry.h"
#include "GraphicContext.h"
#include "../../settings/VideoSettings.h"


#define MAX_PLANES 3
#define MAX_FIELDS 3

#define AUTOSOURCE -1

typedef struct YV12Image
{
  BYTE *   plane[MAX_PLANES];
  unsigned stride[MAX_PLANES];
  unsigned width;
  unsigned height;
  unsigned flags;

  unsigned cshift_x; /* this is the chroma shift used */
  unsigned cshift_y;

  void* opaque;
} YV12Image;

class CBaseTexture;

class CBaseRenderer
{
public:
  CBaseRenderer();
  virtual ~CBaseRenderer();

  void SetViewMode(int viewMode);
  RESOLUTION GetResolution() const;
  void GetVideoRect(CRect &source, CRect &dest);
  float GetAspectRatio() const;
  virtual void AutoCrop(bool bCrop) {};
  virtual int  GetImage(YV12Image *image, int source = AUTOSOURCE, bool readonly = false) { return 0; }
  virtual void ReleaseImage(int source, bool preserve = false) {}
  virtual unsigned int DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y) { return 0; }
  virtual void SetRGB32Image(char *image, int nHeight, int nWidth, int nPitch) {}
  virtual bool Supports(EINTERLACEMETHOD method) { return true; }
  virtual bool Supports(ESCALINGMETHOD method) { return true; }
  virtual void Reset(){} /* resets renderer after seek for example */
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags) { return true; }
  virtual bool IsConfigured() { return true; } 
  virtual void Update(bool bPauseDrawing) {}
  virtual void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);
  virtual unsigned int PreInit() { return 0; }
  virtual void UnInit() {}
  virtual void SetupScreenshot() {}
  virtual void CreateThumbnail(CBaseTexture *texture, unsigned int width, unsigned int height) {}
  virtual bool SupportsBrightness() { return false; }
  virtual bool SupportsContrast() { return false; }
  virtual bool SupportsGamma() { return false; }
  virtual void FlipPage(int source) {}

protected:
  void ChooseBestResolution(float fps);
  void CalcNormalDisplayRect(float offsetX, float offsetY, float screenWidth, float screenHeight, float inputFrameRatio, float zoomAmount);
  void CalculateFrameAspectRatio(unsigned int desired_width, unsigned int desired_height);
  void ManageDisplay();
  void AutoCrop(YV12Image &im, RECT& crop);

  RESOLUTION m_resolution;    // the resolution we're running in
  unsigned int m_sourceWidth;
  unsigned int m_sourceHeight;
  float m_sourceFrameRatio;

  CRect m_destRect;
  CRect m_sourceRect;
};
