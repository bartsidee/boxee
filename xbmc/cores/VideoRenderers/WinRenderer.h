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

#if !defined(_LINUX) && !defined(HAS_GL)

#include "GraphicContext.h"
#include "RenderFlags.h"
#include "BaseRenderer.h"
#include "D3DResource.h"
#include "settings/VideoSettings.h"
#include "dxva.h"
#include "dxva2api.h"
#include "../ffmpeg/DllSwScale.h"

//#define MP_DIRECTRENDERING

#ifdef MP_DIRECTRENDERING
#define NUM_BUFFERS 3
#else
#define NUM_BUFFERS 2
#endif


#define MAX_DXVA2_SURFACES  32
#define MAX_VP_SURFACES     16

#define ALIGN(value, alignment) (((value)+((alignment)-1))&~((alignment)-1))
#define CLAMP(a, min, max) ((a) > (max) ? (max) : ( (a) < (min) ? (min) : a ))

#define AUTOSOURCE -1

#define IMAGE_FLAG_WRITING   0x01 /* image is in use after a call to GetImage, caller may be reading or writing */
#define IMAGE_FLAG_READING   0x02 /* image is in use after a call to GetImage, caller is only reading */
#define IMAGE_FLAG_DYNAMIC   0x04 /* image was allocated due to a call to GetImage */
#define IMAGE_FLAG_RESERVED  0x08 /* image is reserved, must be asked for specifically used to preserve images */

#define IMAGE_FLAG_INUSE (IMAGE_FLAG_WRITING | IMAGE_FLAG_READING | IMAGE_FLAG_RESERVED)


#define RENDER_FLAG_EVEN        0x01
#define RENDER_FLAG_ODD         0x02
#define RENDER_FLAG_BOTH (RENDER_FLAG_EVEN | RENDER_FLAG_ODD)
#define RENDER_FLAG_FIELDMASK   0x03

#define RENDER_FLAG_NOOSD       0x04 /* don't draw any osd */

/* these two flags will be used if we need to render same image twice (bob deinterlacing) */
#define RENDER_FLAG_NOLOCK      0x10   /* don't attempt to lock texture before rendering */
#define RENDER_FLAG_NOUNLOCK    0x20   /* don't unlock texture after rendering */

/* this defines what color translation coefficients */
#define CONF_FLAGS_YUVCOEF_MASK(a) ((a) & 0x07)
#define CONF_FLAGS_YUVCOEF_BT709 0x01
#define CONF_FLAGS_YUVCOEF_BT601 0x02
#define CONF_FLAGS_YUVCOEF_240M  0x03
#define CONF_FLAGS_YUVCOEF_EBU   0x04

#define CONF_FLAGS_YUV_FULLRANGE 0x08
#define CONF_FLAGS_FULLSCREEN    0x10

class CBaseTexture;

struct DRAWRECT
{
  float left;
  float top;
  float right;
  float bottom;
};

enum EFIELDSYNC
{
  FS_NONE,
  FS_ODD,
  FS_EVEN,
  FS_BOTH,
};


struct YUVRANGE
{
  int y_min, y_max;
  int u_min, u_max;
  int v_min, v_max;
};

extern YUVRANGE yuv_range_lim;
extern YUVRANGE yuv_range_full;

typedef enum VideoCodec_enum {
  VideoCodec_MPEG1=0,
  VideoCodec_MPEG2,
  VideoCodec_MPEG4,
  VideoCodec_VC1,
  VideoCodec_H264,
  VideoCodec_NumCodecs,
  // Uncompressed YUV
  VideoCodec_YUV420 = (('I'<<24)|('Y'<<16)|('U'<<8)|('V')),
  VideoCodec_YV12   = (('Y'<<24)|('V'<<16)|('1'<<8)|('2')),
  VideoCodec_NV12   = (('N'<<24)|('V'<<16)|('1'<<8)|('2')),
}VideoCodec;

class CWinRenderer : public CBaseRenderer, public ID3DResource
{
public:
  CWinRenderer();
  ~CWinRenderer();

  virtual void Update(bool bPauseDrawing);
  virtual void SetupScreenshot() {};
  void CreateThumbnail(CBaseTexture *texture, unsigned int width, unsigned int height);

  // Player functions
  virtual bool         Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags);
  virtual int          GetImage(YV12Image *image, int source = AUTOSOURCE, bool readonly = false);
  virtual void         ReleaseImage(int source, bool preserve = false);
  virtual unsigned int DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y);
  virtual void         FlipPage(int source);
  virtual unsigned int PreInit();
  virtual void         UnInit();
  virtual void         Reset(); /* resets renderer after seek for example */
  virtual bool         IsConfigured() { return m_bConfigured; }

  // TODO:DIRECTX - implement these
  virtual bool         SupportsBrightness() { return false; }
  virtual bool         SupportsContrast() { return false; }
  virtual bool         SupportsGamma() { return false; }
  virtual bool         Supports(EINTERLACEMETHOD method);
  virtual bool         Supports(ESCALINGMETHOD method);
 
  virtual void SetRGB32Image(char *image, unsigned int nHeight, unsigned int nWidth, unsigned int nPitch);

  virtual void AutoCrop(bool bCrop);
  void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);

  // DXVA
  void EnableDXVA(bool bEnable) { m_bUseDXVA = bEnable; }
  HRESULT ConfigureDXVA();
  bool AllocateDXVAResources();
  void ReleaseDXVAResources();

protected:
  virtual void Render(DWORD flags);
  void CopyAlpha(int w, int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dst, unsigned char* dsta, int dststride);
  virtual void ManageTextures();
  void DeleteYV12Texture(int index);
  void ClearYV12Texture(int index);
  bool CreateYV12Texture(int index);
  void CopyYV12Texture(int dest);
  int  NextYV12Texture();

  bool LoadEffect(CStdString strName, bool bFromFile);

  // low memory renderer (default PixelShaderRenderer)
  void RenderLowMem(DWORD flags);
  void RenderRGBImage();
  void RenderSoftware();
  void RenderDXVA();


  // === ID3DResource
  virtual void OnDestroyDevice();
  virtual void OnCreateDevice();
  virtual void OnLostDevice();
  virtual void OnResetDevice();

  int m_iYV12RenderBuffer;
  int m_NumYV12Buffers;

  bool m_bConfigured;
  LPDIRECT3DTEXTURE9 m_pRGBTexture;

  CCriticalSection m_resourceSection;
  bool m_bDXVAResourcesAllocated;

  typedef CD3DTexture             YUVVIDEOPLANES[MAX_PLANES];
  typedef BYTE*                   YUVMEMORYPLANES[MAX_PLANES];
  typedef YUVVIDEOPLANES          YUVVIDEOBUFFERS[NUM_BUFFERS];
  typedef YUVMEMORYPLANES         YUVMEMORYBUFFERS[NUM_BUFFERS];   

#define PLANE_Y 0
#define PLANE_U 1
#define PLANE_V 2

#define FIELD_FULL 0
#define FIELD_ODD 1
#define FIELD_EVEN 2

  // YV12 decoder textures
  // field index 0 is full image, 1 is odd scanlines, 2 is even scanlines
  // Since DX is single threaded, we will render all video into system memory
  // We will them copy in into the device when rendering from main thread
  YUVVIDEOBUFFERS  m_YUVVideoTexture;
  YUVMEMORYBUFFERS m_YUVMemoryTexture;

  CD3DEffect  m_YUV2RGBEffect;

  BYTE*         m_rgbBuffer;  // if software scale is used, this will hold the result image
  unsigned int  m_rgbBufferSize;
  bool          m_bRGBImageSet;

  bool          m_bUseSoftwareRendering;
  DllSwScale    m_dllSwScale;

  // clear colour for "black" bars
  DWORD m_clearColour;
  
  unsigned int m_flags;
  CRect m_crop;


  // DXVA
  HMODULE m_hDXVA2Library;
  bool m_bUseDXVA;
  unsigned int m_DXVATextureTargetWidth;
  unsigned int m_DXVATextureTargetHeight;
  VideoCodec m_CodecType; 
  IDirectXVideoProcessorService* m_pDXVA2VideoProcessServices;
  IDirectXVideoProcessor*   m_pDXVA2VideoProcessor;
  DXVA2_VideoDesc           m_VideoDesc;
  DXVA2_VideoProcessorCaps  m_VPCaps;
  LPDIRECT3DSURFACE9* m_pLastSurface;
  IDirect3DTexture9* m_pTargetTexture;
  GUID m_DecodeGuid, m_VPGuid;
  DXVA2_Fixed32 m_Brightness, m_Contrast, m_Hue, m_Saturation;
  BOOL m_bFirstFrame;
  REFERENCE_TIME m_rtBegin;
  REFERENCE_TIME m_rtEnd;
};


class CPixelShaderRenderer : public CWinRenderer
{
public:
  CPixelShaderRenderer();
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags);

protected:
  virtual void Render(DWORD flags);
};

#else
#include "LinuxRenderer.h"
#endif


