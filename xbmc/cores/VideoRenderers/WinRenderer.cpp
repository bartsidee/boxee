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

#ifdef HAS_DX

#include "WinRenderer.h"
#include "Util.h"
#include "Settings.h"
#include "Texture.h"
#include "WindowingFactory.h"
#include "AdvancedSettings.h"
#include "SingleLock.h"
#include "utils/log.h"
#include "FileSystem/File.h"
#include "MathUtils.h"
#include "SpecialProtocol.h"
#include "XBTF.h"

// http://www.martinreddy.net/gfx/faqs/colorconv.faq

YUVRANGE yuv_range_lim =  { 16, 235, 16, 240, 16, 240 };
YUVRANGE yuv_range_full = {  0, 255,  0, 255,  0, 255 };

static float yuv_coef_bt601[4][4] = 
{
    { 1.0f,      1.0f,     1.0f,     0.0f },
    { 0.0f,     -0.344f,   1.773f,   0.0f },
    { 1.403f,   -0.714f,   0.0f,     0.0f },
    { 0.0f,      0.0f,     0.0f,     0.0f } 
};

static float yuv_coef_bt709[4][4] =
{
    { 1.0f,      1.0f,     1.0f,     0.0f },
    { 0.0f,     -0.1870f,  1.8556f,  0.0f },
    { 1.5701f,  -0.4664f,  0.0f,     0.0f },
    { 0.0f,      0.0f,     0.0f,     0.0f }
};

static float yuv_coef_ebu[4][4] = 
{
    { 1.0f,      1.0f,     1.0f,     0.0f },
    { 0.0f,     -0.3960f,  2.029f,   0.0f },
    { 1.140f,   -0.581f,   0.0f,     0.0f },
    { 0.0f,      0.0f,     0.0f,     0.0f }
};

static float yuv_coef_smtp240m[4][4] =
{
    { 1.0f,      1.0f,     1.0f,     0.0f },
    { 0.0f,     -0.2253f,  1.8270f,  0.0f },
    { 1.5756f,  -0.5000f,  0.0f,     0.0f },
    { 0.0f,      0.0f,     0.0f,     0.0f }
};

const D3DFORMAT D3DFMT_VIDPROC = (D3DFORMAT)MAKEFOURCC('N','V','1','2');

static const GUID DXVA_ModeMPEG2_VLD  = {0xee27417f, 0x5e28, 0x4e65,0xbe,0xea,0x1d,0x26,0xb5,0x08,0xad,0xc9};
static const GUID DXVA_ModeMPEG2_IDCT = {0xbf22ad00, 0x03ea, 0x4690,0x80,0x77,0x47,0x33,0x46,0x20,0x9b,0x7e};
static const GUID DXVA_DeinterlaceAdvanced = {0x6cb69578, 0x7617, 0x4637, 0x91, 0xe5, 0x1c, 0x2, 0xdb, 0x81, 0x2, 0x85};

typedef HRESULT (__stdcall *PFNDXVA2CREATEVIDEOSERVICE)(IDirect3DDevice9* pDD, REFIID riid, void** ppService);

// Intel ClearVideo VC1 bitstream decoder
DEFINE_GUID(DXVA_Intel_VC1_ClearVideo, 0xBCC5DB6D, 0xA2B6,0x4AF0,0xAC,0xE4,0xAD,0xB1,0xF7,0x87,0xBC,0x89);

// Intel ClearVideo H264 bitstream decoder
DEFINE_GUID(DXVA_Intel_H264_ClearVideo, 0x604F8E68, 0x4951,0x4C54,0x88,0xFE,0xAB,0xD2,0x5C,0x15,0xB3,0xD6);

unsigned int Pict16Rounded(unsigned int t)
{
  // Picture dimension should be rounded to 16 for DXVA
  return ((t + 15) / 16) * 16;
}

inline unsigned int NP2( unsigned x ) 
{
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;

    return ++x;
}


IDirect3DSurface9* g_pD3DRT = NULL;

CWinRenderer::CWinRenderer()
{
  memset(m_YUVMemoryTexture, 0, sizeof(m_YUVMemoryTexture));

  m_iYV12RenderBuffer = 0;
  m_NumYV12Buffers = 0;

  m_rgbBuffer = NULL;
  m_rgbBufferSize = 0;
  m_bRGBImageSet = false;
  m_pRGBTexture = NULL;

  m_bUseDXVA = false;
  m_DXVATextureTargetWidth = 0;
  m_DXVATextureTargetHeight = 0;
  m_pLastSurface = NULL;

  m_pDXVA2VideoProcessor = NULL;
  m_pTargetTexture = NULL;
  
  m_pDXVA2VideoProcessServices = NULL;
  m_bDXVAResourcesAllocated = false;

  m_hDXVA2Library = NULL;

  m_bUseSoftwareRendering = false;
}

CWinRenderer::~CWinRenderer()
{
  UnInit();
}


void CWinRenderer::ManageTextures()
{
  int neededbuffers = 2;

  if( m_NumYV12Buffers < neededbuffers )
  {
    for(int i = m_NumYV12Buffers; i<neededbuffers;i++)
      CreateYV12Texture(i);

    m_NumYV12Buffers = neededbuffers;
  }
  else if( m_NumYV12Buffers > neededbuffers )
  {
    m_NumYV12Buffers = neededbuffers;
    m_iYV12RenderBuffer = m_iYV12RenderBuffer % m_NumYV12Buffers;    

    for(int i = m_NumYV12Buffers-1; i>=neededbuffers;i--)
      DeleteYV12Texture(i);
  }
}

bool CWinRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{
  CSingleLock lock(g_graphicsContext);

  if (m_pRGBTexture)
    SAFE_RELEASE(m_pRGBTexture);
  
  m_rgbBuffer = NULL;
  m_bUseSoftwareRendering = false;

  m_sourceWidth = width;
  m_sourceHeight = height;
  m_flags = flags;

  // need to recreate textures
  m_NumYV12Buffers = 0;
  m_iYV12RenderBuffer = 0;

  if(flags & CONF_FLAGS_DXVA)
  {
    m_bUseDXVA = true;
    g_Windowing.Register(this);
  }

  // check for the shader level support
  // we need at least ps2.0 to render with shaders.
  // If we dont have any, then use swscale to do the convertion
  unsigned int shaderMajor, shaderMinor;
  g_Windowing.GetShaderVersion(shaderMajor, shaderMinor);
  if(shaderMajor < 2.0)
  {
    if(!(m_flags & CONF_FLAGS_EXTERN_IMAGE))
    {
      m_bUseSoftwareRendering = true;
#if (! defined USE_EXTERNAL_FFMPEG)
      m_dllSwScale.sws_rgb2rgb_init(SWS_CPU_CAPS_MMX2);
#elif (defined HAVE_LIBSWSCALE_RGB2RGB_H) || (defined HAVE_FFMPEG_RGB2RGB_H)
      m_dllSwScale.sws_rgb2rgb_init(SWS_CPU_CAPS_MMX2);
#endif
      if(m_rgbBuffer)
        SAFE_DELETE(m_rgbBuffer);
      m_rgbBufferSize = m_sourceWidth * m_sourceHeight * 4;
      m_rgbBuffer = new BYTE[m_rgbBufferSize];
    }
  }

  // calculate the input frame aspect ratio
  CalculateFrameAspectRatio(d_width, d_height);
  ChooseBestResolution(fps);
  SetViewMode(g_stSettings.m_currentVideoSettings.m_ViewMode);

  ManageDisplay();

  return true;
}

int CWinRenderer::NextYV12Texture()
{
  if(m_NumYV12Buffers)
    return (m_iYV12RenderBuffer + 1) % m_NumYV12Buffers;
  else
    return -1;
}

int CWinRenderer::GetImage(YV12Image *image, int source, bool readonly)
{
  /* take next available buffer */
  if( source == AUTOSOURCE )
    source = NextYV12Texture();

  if(m_bUseDXVA)
  {
    m_pLastSurface = (LPDIRECT3DSURFACE9 *)image->opaque;
    return 0;
  }

  if( source < 0 )
    return -1;

  YUVMEMORYPLANES &planes = m_YUVMemoryTexture[source];

  image->cshift_x = 1;
  image->cshift_y = 1;
  image->height = m_sourceHeight;
  image->width = m_sourceWidth;
  image->flags = 0;

  D3DLOCKED_RECT rect;
  for(int i=0;i<3;i++)
  {
    rect.pBits = planes[i];
    if(i == 0)
      rect.Pitch = m_sourceWidth;
    else
      rect.Pitch = m_sourceWidth / 2;
    image->stride[i] = rect.Pitch;
    image->plane[i] = (BYTE*)rect.pBits;
  }

  return source;
}

void CWinRenderer::ReleaseImage(int source, bool preserve)
{
  // no need to release anything here since we're using system memory
}

void CWinRenderer::Reset()
{
  CSingleLock lock(m_resourceSection);
  m_pLastSurface = NULL;
}

void CWinRenderer::Update(bool bPauseDrawing)
{
  if (!m_bConfigured) return;
  ManageDisplay();
}

void CWinRenderer::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  if (!m_bConfigured) 
    return;

  if(!m_bUseDXVA)
  {
    ManageTextures();
    if (!m_YUVMemoryTexture[m_iYV12RenderBuffer][0]) 
      return ;
  }
  
  CSingleLock lock(g_graphicsContext);

  ManageDisplay();
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();
  if (clear)
    pD3DDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, m_clearColour, 1.0f, 0L );

  if(alpha < 255)
    pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  else
    pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );

  Render(flags);
}

void CWinRenderer::FlipPage(int source)
{  
  if( source >= 0 && source < m_NumYV12Buffers )
    m_iYV12RenderBuffer = source;
  else
    m_iYV12RenderBuffer = NextYV12Texture();

#ifdef MP_DIRECTRENDERING
  __asm wbinvd
#endif

  return;
}


unsigned int CWinRenderer::DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y)
{
  /*
  BYTE *s;
  BYTE *d;
  int i, p;
  
  int index = NextYV12Texture();
  if( index < 0 )
    return -1;
  
  D3DLOCKED_RECT rect;
  RECT target;

  target.left = x;
  target.right = x+w;
  target.top = y;
  target.bottom = y+h;

  YUVPLANES &planes = m_YUVTexture[index];

  // copy Y
  p = 0;
  planes[p]->LockRect(0, &rect, &target, D3DLOCK_DISCARD);
  d = (BYTE*)rect.pBits;
  s = src[p];
  for (i = 0;i < h;i++)
  {
    memcpy(d, s, w);
    s += stride[p];
    d += rect.Pitch;
  }
  planes[p]->UnlockRect(0);

  w >>= 1; h >>= 1;
  x >>= 1; y >>= 1;
  target.top>>=1;
  target.bottom>>=1;
  target.left>>=1;
  target.right>>=1; 

  // copy U
  p = 1;
  planes[p]->LockRect(0, &rect, &target, D3DLOCK_DISCARD);
  d = (BYTE*)rect.pBits;
  s = src[p];
  for (i = 0;i < h;i++)
  {
    memcpy(d, s, w);
    s += stride[p];
    d += rect.Pitch;
  }
  planes[p]->UnlockRect(0);

  // copy V
  p = 2;
  planes[p]->LockRect(0, &rect, &target, D3DLOCK_DISCARD);
  d = (BYTE*)rect.pBits;
  s = src[p];
  for (i = 0;i < h;i++)
  {
    memcpy(d, s, w);
    s += stride[p];
    d += rect.Pitch;
  }
  planes[p]->UnlockRect(0);
  */

  return 0;
}

unsigned int CWinRenderer::PreInit()
{
  CSingleLock lock(g_graphicsContext);
  m_bConfigured = false;
  UnInit();
  m_resolution = RES_PAL_4x3;
  m_crop.x1 = m_crop.x2 = 0.0f;
  m_crop.y1 = m_crop.y2 = 0.0f;

  // setup the background colour
  m_clearColour = (g_advancedSettings.m_videoBlackBarColour & 0xff) * 0x010101;

  if(m_pRGBTexture)
  {
    SAFE_RELEASE(m_pRGBTexture);
  }

  m_rgbBuffer=0;
  m_rgbBufferSize = 0;
  m_bRGBImageSet = false;

  return 0;
}


void CWinRenderer::UnInit()
{
  CSingleLock lock(g_graphicsContext);

  m_YUV2RGBEffect.Release();
  m_bConfigured = false;
  for(int i = 0; i < NUM_BUFFERS; i++)
    DeleteYV12Texture(i);
  m_NumYV12Buffers = 0;

  if(m_bUseDXVA)
  {
    ReleaseDXVAResources();
    g_Windowing.Unregister(this);
  }

  if(!(m_flags & CONF_FLAGS_EXTERN_IMAGE))
  {
    SAFE_DELETE(m_rgbBuffer);
  }

  m_rgbBufferSize = 0;
  m_bRGBImageSet = false;
  m_bUseDXVA = false;
}

bool CWinRenderer::LoadEffect(CStdString strName, bool bFromFile)
{
  CStdString pStrEffect;

  if(!bFromFile)
  {
    XFILE::CFileStream file;
    if(!file.Open(strName))
    {
      CLog::Log(LOGERROR, "CWinRenderer::LoadEffect - failed to open file %s", strName.c_str());
      return false;
    }
    getline(file, pStrEffect, '\0');
  }
  else
    pStrEffect = _P(strName);

  if (!m_YUV2RGBEffect.Create(pStrEffect, bFromFile))
  {
    CLog::Log(LOGERROR, "D3DXCreateEffectFromFile %s failed", pStrEffect.c_str());
    return false;
  }

  return true;
}

void CWinRenderer::Render(DWORD flags)
{
  if( flags & RENDER_FLAG_NOOSD ) return;
}

void CWinRenderer::AutoCrop(bool bCrop)
{
  if(!m_bUseDXVA)
    if (!m_YUVMemoryTexture[0][PLANE_Y]) 
      return ;

  RECT crop;

  if (bCrop && !m_bUseDXVA)
  {
    YV12Image im;
    if(GetImage(&im, m_iYV12RenderBuffer, true) < 0)
      return;

    CBaseRenderer::AutoCrop(im, crop);

    ReleaseImage(m_iYV12RenderBuffer, false);
  }
  else
  { // reset to defaults
    crop.left   = 0;
    crop.right  = 0;
    crop.top    = 0;
    crop.bottom = 0;
  }

  m_crop.x1 += ((float)crop.left   - m_crop.x1) * 0.1f;
  m_crop.x2 += ((float)crop.right  - m_crop.x2) * 0.1f;
  m_crop.y1 += ((float)crop.top    - m_crop.y1) * 0.1f;
  m_crop.y2 += ((float)crop.bottom - m_crop.y2) * 0.1f;

  crop.left   = MathUtils::round_int(m_crop.x1);
  crop.right  = MathUtils::round_int(m_crop.x2);
  crop.top    = MathUtils::round_int(m_crop.y1);
  crop.bottom = MathUtils::round_int(m_crop.y2);

  //compare with hysteresis
# define HYST(n, o) ((n) > (o) || (n) + 1 < (o))
  if(HYST(g_stSettings.m_currentVideoSettings.m_CropLeft  , crop.left)
  || HYST(g_stSettings.m_currentVideoSettings.m_CropRight , crop.right)
  || HYST(g_stSettings.m_currentVideoSettings.m_CropTop   , crop.top)
  || HYST(g_stSettings.m_currentVideoSettings.m_CropBottom, crop.bottom))
  {
    g_stSettings.m_currentVideoSettings.m_CropLeft   = crop.left;
    g_stSettings.m_currentVideoSettings.m_CropRight  = crop.right;
    g_stSettings.m_currentVideoSettings.m_CropTop    = crop.top;
    g_stSettings.m_currentVideoSettings.m_CropBottom = crop.bottom;
    SetViewMode(g_stSettings.m_currentVideoSettings.m_ViewMode);
  }
# undef HYST

}

void CWinRenderer::RenderSoftware()
{
  CSingleLock lock(g_graphicsContext);

  int index = m_iYV12RenderBuffer;
  // set scissors if we are not in fullscreen video
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
  {
    g_graphicsContext.ClipToViewWindow();
  }

  struct SwsContext *context = m_dllSwScale.sws_getContext(m_sourceWidth, m_sourceHeight, PIX_FMT_YUV420P,
    m_sourceWidth, m_sourceHeight, PIX_FMT_BGRA,
    SWS_FAST_BILINEAR, NULL, NULL, NULL);
  uint8_t *src[] = { m_YUVMemoryTexture[index][0], m_YUVMemoryTexture[index][1], m_YUVMemoryTexture[index][2] };
  int     srcStride[] = { m_sourceWidth, m_sourceWidth/2.0, m_sourceWidth/2.0 };
  uint8_t *dst[] = { m_rgbBuffer, 0, 0 };
  int     dstStride[] = { m_sourceWidth*4, 0, 0 };
  m_dllSwScale.sws_scale(context, src, srcStride, 0, m_sourceHeight, dst, dstStride);
  m_dllSwScale.sws_freeContext(context);

  RenderRGBImage();
}

void CWinRenderer::RenderRGBImage()
{
  // copy memory textures to video textures
  D3DLOCKED_RECT rect;
  LPDIRECT3DSURFACE9 videoSurface;
  D3DSURFACE_DESC desc;
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();
  
  if (!m_pRGBTexture)
    pD3DDevice->CreateTexture(m_sourceWidth, m_sourceHeight, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &m_pRGBTexture, NULL);

  if (m_rgbBuffer)
  {
    BYTE* src = m_rgbBuffer;
    m_pRGBTexture->GetSurfaceLevel(0, &videoSurface);
    videoSurface->GetDesc(&desc);
    if(videoSurface->LockRect(&rect, NULL, 0) == D3D_OK)
    {
      for(unsigned int j = 0; j < std::min(m_sourceHeight,desc.Height); j++)
      {
        memcpy((BYTE *)rect.pBits + (j * rect.Pitch), src + (j * m_sourceWidth * 4), m_sourceWidth * 4);
      }
      videoSurface->UnlockRect();
      videoSurface->Release();
    }
  }

  pD3DDevice->SetTexture(0, m_pRGBTexture);
  pD3DDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
  pD3DDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
  pD3DDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
  pD3DDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );

  pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
  pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
  pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
  pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE  );
  pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
  pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );

  pD3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE );  // was m_pD3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE ); ???
  pD3DDevice->SetFVF( D3DFVF_XYZRHW | D3DFVF_TEX1 );

  // Render the image
  struct CUSTOMVERTEX {
    FLOAT x, y, z;
    FLOAT rhw;
    FLOAT tu, tv;   // Texture coordinates
  };

  CUSTOMVERTEX verts[4] = 
  {
    {
      m_destRect.x1                                                      ,  m_destRect.y1, 0.0f, 1.0f,
        (m_sourceRect.x1) / m_sourceWidth                                  , (m_sourceRect.y1) / m_sourceHeight,
    },
    {
      m_destRect.x2                                                      ,  m_destRect.y1, 0.0f, 1.0f,
        (m_sourceRect.x2) / m_sourceWidth                                  , (m_sourceRect.y1) / m_sourceHeight,
      },
      {
        m_destRect.x2                                                      ,  m_destRect.y2, 0.0f, 1.0f,
          (m_sourceRect.x2) / m_sourceWidth                                  , (m_sourceRect.y2) / m_sourceHeight,
      },
      {
        m_destRect.x1                                                       ,  m_destRect.y2, 0.0f, 1.0f,
          (m_sourceRect.x1) / m_sourceWidth                                   , (m_sourceRect.y2) / m_sourceHeight,
        }
  };

  pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(CUSTOMVERTEX));
  pD3DDevice->SetTexture(0, NULL);
}

void CWinRenderer::RenderLowMem(DWORD flags)
{
  CStdString filename = "special://xbmc/system/shaders/yuv2rgb_d3d.fx";

  if (!m_YUV2RGBEffect.Get())
    LoadEffect(filename, false);

  CSingleLock lock(g_graphicsContext);

  int index = m_iYV12RenderBuffer;
  // set scissors if we are not in fullscreen video
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
  {
    g_graphicsContext.ClipToViewWindow();
  }

  // copy memory textures to video textures
  D3DLOCKED_RECT rect;
  LPDIRECT3DSURFACE9 videoSurface;
  D3DSURFACE_DESC desc;
  for(unsigned int i = 0; i < 3; i++)
  {
    BYTE* src = (BYTE *)m_YUVMemoryTexture[index][i];
    m_YUVVideoTexture[index][i].GetSurfaceLevel(0, &videoSurface);
    videoSurface->GetDesc(&desc);
    if(videoSurface->LockRect(&rect, NULL, 0) == D3D_OK)
    {
      if (rect.Pitch == desc.Width)
      {
        memcpy((BYTE *)rect.pBits, src, desc.Height * desc.Width);
      }
      else for(unsigned int j = 0; j < desc.Height; j++)
      {
        memcpy((BYTE *)rect.pBits + (j * rect.Pitch), src + (j * desc.Width), rect.Pitch);
      }
      videoSurface->UnlockRect();
    }
    SAFE_RELEASE(videoSurface);
  }

  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();
  pD3DDevice->SetFVF( D3DFVF_XYZRHW | D3DFVF_TEX3 );

  //See RGB renderer for comment on this
  #define CHROMAOFFSET_HORIZ 0.25f

  // Render the image
  struct CUSTOMVERTEX {
      FLOAT x, y, z;
      FLOAT rhw;
      FLOAT tu, tv;   // Texture coordinates
      FLOAT tu2, tv2;
      FLOAT tu3, tv3;
  };

  CUSTOMVERTEX verts[4] = 
  {
    {
      m_destRect.x1                                                      ,  m_destRect.y1, 0.0f, 1.0f,
      (m_sourceRect.x1) / m_sourceWidth                                  , (m_sourceRect.y1) / m_sourceHeight,
      (m_sourceRect.x1 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceWidth>>1) , (m_sourceRect.y1 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceHeight>>1),
      (m_sourceRect.x1 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceWidth>>1) , (m_sourceRect.y1 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceHeight>>1)
    },
    {
      m_destRect.x2                                                      ,  m_destRect.y1, 0.0f, 1.0f,
      (m_sourceRect.x2) / m_sourceWidth                                  , (m_sourceRect.y1) / m_sourceHeight,
      (m_sourceRect.x2 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceWidth>>1) , (m_sourceRect.y1 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceHeight>>1),
      (m_sourceRect.x2 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceWidth>>1) , (m_sourceRect.y1 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceHeight>>1)
    },
    {
      m_destRect.x2                                                      ,  m_destRect.y2, 0.0f, 1.0f,
      (m_sourceRect.x2) / m_sourceWidth                                  , (m_sourceRect.y2) / m_sourceHeight,
      (m_sourceRect.x2 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceWidth>>1) , (m_sourceRect.y2 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceHeight>>1),
      (m_sourceRect.x2 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceWidth>>1) , (m_sourceRect.y2 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceHeight>>1)
    },
    {
      m_destRect.x1                                                       ,  m_destRect.y2, 0.0f, 1.0f,
      (m_sourceRect.x1) / m_sourceWidth                                   , (m_sourceRect.y2) / m_sourceHeight,
      (m_sourceRect.x1 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceWidth>>1)  , (m_sourceRect.y2 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceHeight>>1),
      (m_sourceRect.x1 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceWidth>>1)  , (m_sourceRect.y2 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceHeight>>1)
    }
  };

  for(int i = 0; i < 4; i++)
  {
    verts[i].x -= 0.5;
    verts[i].y -= 0.5;
  }

  float contrast   = g_stSettings.m_currentVideoSettings.m_Contrast * 0.02f;
  float blacklevel = g_stSettings.m_currentVideoSettings.m_Brightness * 0.01f - 0.5f;

  D3DXMATRIX temp, mat;
  D3DXMatrixIdentity(&mat);

  if (!(m_flags & CONF_FLAGS_YUV_FULLRANGE))
  {
    D3DXMatrixTranslation(&temp, - 16.0f / 255
                               , - 16.0f / 255
                               , - 16.0f / 255);
    D3DXMatrixMultiply(&mat, &mat, &temp);

    D3DXMatrixScaling(&temp, 255.0f / (235 - 16)
                           , 255.0f / (240 - 16)
                           , 255.0f / (240 - 16));
    D3DXMatrixMultiply(&mat, &mat, &temp);
  }

  D3DXMatrixTranslation(&temp, 0.0f, - 0.5f, - 0.5f);
  D3DXMatrixMultiply(&mat, &mat, &temp);

  switch(CONF_FLAGS_YUVCOEF_MASK(m_flags))
  {
   case CONF_FLAGS_YUVCOEF_240M:
     memcpy(temp.m, yuv_coef_smtp240m, 4*4*sizeof(float)); break;
   case CONF_FLAGS_YUVCOEF_BT709:
     memcpy(temp.m, yuv_coef_bt709   , 4*4*sizeof(float)); break;
   case CONF_FLAGS_YUVCOEF_BT601:    
     memcpy(temp.m, yuv_coef_bt601   , 4*4*sizeof(float)); break;
   case CONF_FLAGS_YUVCOEF_EBU:
     memcpy(temp.m, yuv_coef_ebu     , 4*4*sizeof(float)); break;
   default:
     memcpy(temp.m, yuv_coef_bt601   , 4*4*sizeof(float)); break;
  }
  temp.m[3][3] = 1.0f;
  D3DXMatrixMultiply(&mat, &mat, &temp);

  D3DXMatrixTranslation(&temp, blacklevel, blacklevel, blacklevel);
  D3DXMatrixMultiply(&mat, &mat, &temp);

  D3DXMatrixScaling(&temp, contrast, contrast, contrast);
  D3DXMatrixMultiply(&mat, &mat, &temp);

  m_YUV2RGBEffect.SetMatrix( "g_ColorMatrix", &mat);
  m_YUV2RGBEffect.SetTechnique( "YUV2RGB_T" );
  m_YUV2RGBEffect.SetTexture( "g_YTexture",  m_YUVVideoTexture[index][0] ) ;
  m_YUV2RGBEffect.SetTexture( "g_UTexture",  m_YUVVideoTexture[index][1] ) ;
  m_YUV2RGBEffect.SetTexture( "g_VTexture",  m_YUVVideoTexture[index][2] ) ;

  UINT cPasses, iPass;
  if (!m_YUV2RGBEffect.Begin( &cPasses, 0 ))
    return;

  for( iPass = 0; iPass < cPasses; iPass++ )
  {
    m_YUV2RGBEffect.BeginPass( iPass );

    pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(CUSTOMVERTEX));
    pD3DDevice->SetTexture(0, NULL);
    pD3DDevice->SetTexture(1, NULL);
    pD3DDevice->SetTexture(2, NULL);
  
    m_YUV2RGBEffect.EndPass() ;
  }

  m_YUV2RGBEffect.End() ;
  pD3DDevice->SetPixelShader( NULL );
}

void CWinRenderer::CreateThumbnail(CBaseTexture *texture, unsigned int width, unsigned int height)
{
  CSingleLock lock(g_graphicsContext);

  // create a new render surface to copy out of - note, this may be slow on some hardware
  // due to the TRUE parameter - you're supposed to use GetRenderTargetData.
  LPDIRECT3DSURFACE9 surface = NULL;
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();
  if (D3D_OK == pD3DDevice->CreateRenderTarget(width, height, D3DFMT_LIN_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &surface, NULL))
  {
    LPDIRECT3DSURFACE9 oldRT;
    CRect saveSize = m_destRect;
    m_destRect.SetRect(0, 0, (float)width, (float)height);
    pD3DDevice->GetRenderTarget(0, &oldRT);
    pD3DDevice->SetRenderTarget(0, surface);
    pD3DDevice->BeginScene();
    RenderLowMem(0);
    pD3DDevice->EndScene();
    m_destRect = saveSize;
    pD3DDevice->SetRenderTarget(0, oldRT);
    oldRT->Release();

    D3DLOCKED_RECT lockedRect;
    if (D3D_OK == surface->LockRect(&lockedRect, NULL, NULL))
    {
      texture->LoadFromMemory(width, height, lockedRect.Pitch, XB_FMT_B8G8R8A8, (unsigned char *)lockedRect.pBits);
      surface->UnlockRect();
    }
    surface->Release();
  }
}

//********************************************************************************************************
// YV12 Texture creation, deletion, copying + clearing
//********************************************************************************************************
void CWinRenderer::DeleteYV12Texture(int index)
{
  CSingleLock lock(g_graphicsContext);
  YUVVIDEOPLANES &videoPlanes = m_YUVVideoTexture[index];
  YUVMEMORYPLANES &memoryPlanes = m_YUVMemoryTexture[index];

  videoPlanes[0].Release();
  videoPlanes[1].Release();
  videoPlanes[2].Release();

  SAFE_DELETE_ARRAY(memoryPlanes[0]);
  SAFE_DELETE_ARRAY(memoryPlanes[1]);
  SAFE_DELETE_ARRAY(memoryPlanes[2]);

  m_NumYV12Buffers = 0;
}

void CWinRenderer::ClearYV12Texture(int index)
{  
  YUVMEMORYPLANES &planes = m_YUVMemoryTexture[index];
  D3DLOCKED_RECT rect;
  
  rect.pBits = planes[0];
  rect.Pitch = m_sourceWidth;
  memset(rect.pBits, 0,   rect.Pitch * m_sourceHeight);

  rect.pBits = planes[1];
  rect.Pitch = m_sourceWidth / 2;
  memset(rect.pBits, 128, rect.Pitch * m_sourceHeight>>1);

  rect.pBits = planes[2];
  rect.Pitch = m_sourceWidth / 2;
  memset(rect.pBits, 128, rect.Pitch * m_sourceHeight>>1);
}

void CWinRenderer::SetRGB32Image(char *image, unsigned int nHeight, unsigned int nWidth, unsigned int nPitch)
{
  CSingleLock lock(g_graphicsContext);

  m_rgbBufferSize = nPitch*nHeight;
  m_rgbBuffer = (BYTE*)image;

  m_bRGBImageSet = true;
}

bool CWinRenderer::CreateYV12Texture(int index)
{

  CSingleLock lock(g_graphicsContext);
  DeleteYV12Texture(index);
  if (
    !m_YUVVideoTexture[index][0].Create(m_sourceWidth, m_sourceHeight, 1, 0, D3DFMT_L8, D3DPOOL_MANAGED) ||
    !m_YUVVideoTexture[index][1].Create(m_sourceWidth / 2, m_sourceHeight / 2, 1, 0, D3DFMT_L8, D3DPOOL_MANAGED) ||
    !m_YUVVideoTexture[index][2].Create(m_sourceWidth / 2, m_sourceHeight / 2, 1, 0, D3DFMT_L8, D3DPOOL_MANAGED))
  {
    CLog::Log(LOGERROR, "Unable to create YV12 video texture %i", index);
    return false;
  }

  if (
    NULL == (m_YUVMemoryTexture[index][0] = new BYTE[m_sourceWidth * m_sourceHeight]) ||
    NULL == (m_YUVMemoryTexture[index][1] = new BYTE[m_sourceWidth / 2 * m_sourceHeight / 2]) ||
    NULL == (m_YUVMemoryTexture[index][2] = new BYTE[m_sourceWidth / 2* m_sourceHeight / 2]))
  {
    CLog::Log(LOGERROR, "Unable to create YV12 memory texture %i", index);
    return false;
  }

  ClearYV12Texture(index);
  CLog::Log(LOGDEBUG, "created yv12 texture %i", index);
  return true;
}

bool CWinRenderer::Supports(EINTERLACEMETHOD method)
{
  if(method == VS_INTERLACEMETHOD_NONE
  || method == VS_INTERLACEMETHOD_AUTO
  || method == VS_INTERLACEMETHOD_DEINTERLACE)
    return true;

  return false;
}

bool CWinRenderer::Supports(ESCALINGMETHOD method)
{
  if(method == VS_SCALINGMETHOD_NEAREST
  || method == VS_SCALINGMETHOD_LINEAR)
    return true;

  return false;
}

CPixelShaderRenderer::CPixelShaderRenderer()
    : CWinRenderer()
{
}

bool CPixelShaderRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{
  if(!CWinRenderer::Configure(width, height, d_width, d_height, fps, flags))
    return false;

  m_bConfigured = true;
  return true;
}


void CPixelShaderRenderer::Render(DWORD flags)
{
  if(m_bRGBImageSet)
    CWinRenderer::RenderRGBImage();
  else if(m_bUseSoftwareRendering)
    CWinRenderer::RenderSoftware();
  else if(m_bUseDXVA)
    CWinRenderer::RenderDXVA();
  else
    CWinRenderer::RenderLowMem(flags);
  CWinRenderer::Render(flags);
}

HRESULT CWinRenderer::ConfigureDXVA()
{
  IDirect3DDevice9* pDevice;
  IDirectXVideoDecoderService* pDXVA2VideoDecoderServices;
  UINT uGuidCount, uFormatCount;
  GUID *pGuids;
  GUID DecodeGuid;
  D3DFORMAT *pFormats;
  D3DFORMAT DecodeRTFormat;
  DXVA2_ValueRange rng;
  HRESULT hr;
  PFNDXVA2CREATEVIDEOSERVICE pfnDXVA2CreateVideoService;

  m_CodecType = VideoCodec_H264; // for now

  pDevice = g_Windowing.Get3DDevice();

  if(m_hDXVA2Library == NULL)
    return S_FALSE;


  pfnDXVA2CreateVideoService = (PFNDXVA2CREATEVIDEOSERVICE)GetProcAddress(m_hDXVA2Library, "DXVA2CreateVideoService");
  if (pfnDXVA2CreateVideoService == NULL)
  {
    CLog::Log(LOGERROR, "CWinRenderer::ConfigureDXVA GetProcAddress failed");
    return S_FALSE;
  }

  hr = pfnDXVA2CreateVideoService(pDevice,
    IID_IDirectXVideoDecoderService,
    (VOID**)&pDXVA2VideoDecoderServices);

  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "%s m_pDeviceManager->GetVideoService failed", __FUNCTION__);
    return S_FALSE;
  }

  // Find the appropriate DXVA GUID
  uGuidCount = 0;
  pGuids = NULL;
  hr = pDXVA2VideoDecoderServices->GetDecoderDeviceGuids(&uGuidCount, &pGuids);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "%s Failed to get decoder device Guids", __FUNCTION__);
    return S_FALSE;
  }

  LONG lGuidNdx = -1;
  
  DecodeGuid = GUID_NULL;
  for (UINT iGuid=0; iGuid<uGuidCount; iGuid++)
  {
    switch(m_CodecType)
    {
    case VideoCodec_H264:
      if (pGuids[iGuid] == DXVA_ModeH264_VLD_NoFGT || pGuids[iGuid] == DXVA_Intel_H264_ClearVideo)
        lGuidNdx = iGuid;
      break;
    case VideoCodec_MPEG1:
      if (pGuids[iGuid] == DXVA_ModeMPEG2_IDCT)
        lGuidNdx = iGuid;
      break;
    case VideoCodec_MPEG2:
      if ((pGuids[iGuid] == DXVA_ModeMPEG2_VLD)
        || ((m_DecodeGuid == GUID_NULL) && (pGuids[iGuid] == DXVA_ModeMPEG2_IDCT)))
        lGuidNdx = iGuid;
      break;
    case VideoCodec_VC1:
      if ((pGuids[iGuid] == DXVA_ModeVC1_VLD || pGuids[iGuid] == DXVA_Intel_VC1_ClearVideo)
        || ((m_DecodeGuid == GUID_NULL) && (pGuids[iGuid] == DXVA_ModeVC1_IDCT)))
        lGuidNdx = iGuid;
      break;
    }
    if (lGuidNdx >= 0)
    {
      m_DecodeGuid = pGuids[lGuidNdx];
    }
  }

  CLog::Log(LOGDEBUG, "%s Found GUID for rendering: %d", __FUNCTION__, m_DecodeGuid);

  if(g_Windowing.SupportsRT_NPOT())
  {
    m_DXVATextureTargetWidth = Pict16Rounded(m_sourceWidth);
    m_DXVATextureTargetHeight = Pict16Rounded(m_sourceHeight);
  }
  else
  {
    m_DXVATextureTargetWidth = NP2(m_sourceWidth);
    m_DXVATextureTargetHeight = NP2(m_sourceHeight);
  }

  CLog::Log(LOGDEBUG, "%s Using RenderTarget %d x %d", __FUNCTION__, m_DXVATextureTargetWidth, m_DXVATextureTargetHeight);

  CoTaskMemFree(pGuids);
  if (m_DecodeGuid == GUID_NULL)
  {
    CLog::Log(LOGERROR, "%s Codec type not supported (%d guids found)", __FUNCTION__, uGuidCount);
    return S_FALSE;
  }
  // Use the default render target format
  hr = pDXVA2VideoDecoderServices->GetDecoderRenderTargets(m_DecodeGuid, &uFormatCount, &pFormats);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "%s Failed to get decoder render target format", __FUNCTION__);
    return S_FALSE;
  }

  CLog::Log(LOGDEBUG, "%s Render Targets: %d total", __FUNCTION__, uFormatCount);
  
  DecodeRTFormat = pFormats[0];

  //
  // Query video processor capabilities.
  //
  memset(&m_VideoDesc, 0, sizeof(DXVA2_VideoDesc));
  memset(&m_VPCaps, 0, sizeof(DXVA2_VideoProcessorCaps));
  
  CoTaskMemFree(pFormats);
  
  // Setup DXVA config
  m_VideoDesc.SampleWidth = Pict16Rounded(m_sourceWidth);
  m_VideoDesc.SampleHeight = Pict16Rounded(m_sourceHeight);
  m_VideoDesc.Format = DecodeRTFormat;
  m_VideoDesc.InputSampleFreq.Numerator = 25000;
  m_VideoDesc.InputSampleFreq.Denominator = 1000;
  m_VideoDesc.OutputFrameFreq = m_VideoDesc.InputSampleFreq;
  m_VideoDesc.SampleFormat.SampleFormat = DXVA2_SampleProgressiveFrame;
  m_VideoDesc.SampleFormat.VideoChromaSubsampling = DXVA2_VideoChromaSubsampling_MPEG2;
  m_VideoDesc.SampleFormat.NominalRange = DXVA2_NominalRange_0_255;
  m_VideoDesc.SampleFormat.VideoTransferMatrix = DXVA2_VideoTransferMatrix_BT709;

  hr = m_pDXVA2VideoProcessServices->GetVideoProcessorDeviceGuids(&m_VideoDesc, &uGuidCount, &pGuids);
  if (hr != S_OK)
  {
    CLog::Log(LOGERROR, "CWinRenderer::ConfigureDXVA: Failed to enumerate video process guids (0x%X)", hr);
    return S_FALSE;
  }

  CLog::Log(LOGDEBUG, "CWinRenderer::ConfigureDXVA: Video Processor Device Guid: %d total", uGuidCount);

  m_VPGuid = GUID_NULL;

  unsigned int i;
  for(i = 0; i < uGuidCount; i++)
  {
    if(pGuids[i] == DXVA2_VideoProcProgressiveDevice)
    {
      m_VPGuid = DXVA2_VideoProcProgressiveDevice;
      break;
    }
  }

  if(m_VPGuid == GUID_NULL)
    m_VPGuid = DXVA2_VideoProcBobDevice;

  CoTaskMemFree(pGuids);

  hr = m_pDXVA2VideoProcessServices->GetProcAmpRange(m_VPGuid, &m_VideoDesc, D3DFMT_VIDPROC, DXVA2_ProcAmp_Brightness, &rng); 
  m_Brightness = rng.DefaultValue;
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "CWinRenderer::ConfigureDXVA: GetProcAmpRange failed (0x%X)", hr);
    return S_FALSE;
  }
  hr = m_pDXVA2VideoProcessServices->GetProcAmpRange(m_VPGuid, &m_VideoDesc, D3DFMT_VIDPROC, DXVA2_ProcAmp_Contrast, &rng); 
  m_Contrast = rng.DefaultValue;
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "CWinRenderer::ConfigureDXVA: GetProcAmpRange failed (0x%X)", hr);
    return S_FALSE;
  }
  hr = m_pDXVA2VideoProcessServices->GetProcAmpRange(m_VPGuid, &m_VideoDesc, D3DFMT_VIDPROC, DXVA2_ProcAmp_Hue, &rng); 
  m_Hue = rng.DefaultValue;
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "CWinRenderer::ConfigureDXVA: GetProcAmpRange failed (0x%X)", hr);
    return S_FALSE;
  }
  hr = m_pDXVA2VideoProcessServices->GetProcAmpRange(m_VPGuid, &m_VideoDesc, D3DFMT_VIDPROC, DXVA2_ProcAmp_Saturation, &rng); 
  m_Saturation = rng.DefaultValue;
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "CWinRenderer::ConfigureDXVA: GetProcAmpRange failed (0x%X)", hr);
    return S_FALSE;
  }
  hr = m_pDXVA2VideoProcessServices->CreateVideoProcessor(m_VPGuid, &m_VideoDesc,
    D3DFMT_VIDPROC, 0, &m_pDXVA2VideoProcessor);
  if (FAILED(hr))
  {
    hr = m_pDXVA2VideoProcessServices->CreateVideoProcessor(m_VPGuid, &m_VideoDesc,
      D3DFMT_X8R8G8B8, 0, &m_pDXVA2VideoProcessor);
    if(FAILED(hr))
    {
      CLog::Log(LOGERROR, "CWinRenderer::ConfigureDXVA: Failed to create Video Processor (0x%08X)!", hr);
      return S_FALSE;
    }
  }

  pDXVA2VideoDecoderServices->Release();

  return S_OK;
}

bool CWinRenderer::AllocateDXVAResources()
{
  HRESULT hr;
  IDirect3DDevice9* pDevice;
  PFNDXVA2CREATEVIDEOSERVICE pfnDXVA2CreateVideoService;

  CSingleLock lock(m_resourceSection);

  ReleaseDXVAResources();

  m_hDXVA2Library = LoadLibraryW(L"dxva2.dll");
  if(m_hDXVA2Library == NULL)
  {
    CLog::Log(LOGERROR, "CWinRenderer::AllocateDXVAResources: LoadLibrary dxva2.dll failed");
    return FALSE;
  }

  pfnDXVA2CreateVideoService = (PFNDXVA2CREATEVIDEOSERVICE)GetProcAddress(m_hDXVA2Library, "DXVA2CreateVideoService");
  if (pfnDXVA2CreateVideoService == NULL)
  {
    CLog::Log(LOGERROR, "CWinRenderer::AllocateDXVAResources GetProcAddress failed");
    return FALSE;
  }

  pDevice = g_Windowing.Get3DDevice();

  hr = pfnDXVA2CreateVideoService(pDevice,
    IID_IDirectXVideoProcessorService,
    (VOID**)&m_pDXVA2VideoProcessServices);

  if (hr != S_OK)
  {
    CLog::Log(LOGERROR, "CWinRenderer::AllocateDXVAResources: Failed to create VideoProcessor Service (0x%08X)", hr);
    return FALSE;
  }

  if(FAILED(ConfigureDXVA()))
    return false;

  m_bFirstFrame = TRUE;
  m_rtBegin = 0;
  m_rtEnd = 0;

  if(FAILED(g_Windowing.Get3DDevice()->CreateTexture(
	  m_DXVATextureTargetWidth, m_DXVATextureTargetHeight, 
	  0, D3DUSAGE_RENDERTARGET, 
    D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &m_pTargetTexture, NULL)))
    return false;

  m_bDXVAResourcesAllocated = true;

  return true;
}

void CWinRenderer::ReleaseDXVAResources()
{
  CSingleLock lock(m_resourceSection);

  if(m_pTargetTexture)
  {
    m_pTargetTexture->Release();
    m_pTargetTexture = NULL;
  }

  if (m_pDXVA2VideoProcessor)
  {
    m_pDXVA2VideoProcessor->Release();
    m_pDXVA2VideoProcessor = NULL;
  }

  if (m_pDXVA2VideoProcessServices)
  {
    m_pDXVA2VideoProcessServices->Release();
    m_pDXVA2VideoProcessServices = NULL;
  }

  if (m_hDXVA2Library != NULL)
  {
    FreeLibrary(m_hDXVA2Library);
    m_hDXVA2Library = NULL;
  }


  m_bDXVAResourcesAllocated = false;
}


void CWinRenderer::RenderDXVA()
{
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();
  DXVA2_VideoProcessBltParams vpblt;
  DXVA2_VideoSample vs;
  HRESULT hr;
  int num_fields;

  int nOutputPicIdx = 0;
  int nDecodePicIdx = 0;

  CSingleLock lock(m_resourceSection);

  // Check if we have anything to render
  if(m_pLastSurface == NULL || *m_pLastSurface == NULL)
    return;

  UINT nVPTargets = MAX_VP_SURFACES;
  // Input
  RECT rcSource, rcTarget;

  int width16 = Pict16Rounded(m_sourceWidth);
  int height16 = Pict16Rounded(m_sourceHeight);

  if(!m_bDXVAResourcesAllocated)
    if(!AllocateDXVAResources())
      return;

  rcSource.left = (LONG)0;
  rcSource.top = (LONG)0;
  rcSource.right = (LONG)width16;
  rcSource.bottom = (LONG)height16;
  rcTarget.left = (LONG)0;
  rcTarget.top = (LONG)0;
  rcTarget.right = m_DXVATextureTargetWidth;
  rcTarget.bottom = m_DXVATextureTargetHeight;

  memset(&vs, 0, sizeof(vs));
  vs.Start = m_rtBegin;
  vs.End   = m_rtEnd;
  vs.PlanarAlpha.ll = 0x10000;
  vs.SrcSurface = *m_pLastSurface;
  vs.SrcRect = rcSource;
  vs.DstRect = rcTarget;
  vs.SampleData = 0x02;

  vs.SampleFormat.SampleFormat = DXVA2_SampleProgressiveFrame;
  vs.SampleFormat.VideoChromaSubsampling = DXVA2_VideoChromaSubsampling_MPEG2;
  vs.SampleFormat.NominalRange = DXVA2_NominalRange_Unknown;
  vs.SampleFormat.VideoTransferMatrix = DXVA2_VideoTransferMatrix_BT601;

  // Output
  memset(&vpblt, 0, sizeof(vpblt));
  vpblt.TargetRect = rcTarget;
  vpblt.ConstrictionSize.cx = vpblt.TargetRect.right - vpblt.TargetRect.left;
  vpblt.ConstrictionSize.cy = vpblt.TargetRect.bottom - vpblt.TargetRect.top;
  vpblt.DestFormat = vs.SampleFormat;
  vpblt.DestFormat.SampleFormat = DXVA2_SampleProgressiveFrame;
  vpblt.Alpha.ll = 0x10000;
  vpblt.TargetFrame = vs.Start;
  vpblt.ProcAmpValues.Brightness = m_Brightness;
  vpblt.ProcAmpValues.Contrast = m_Contrast;
  vpblt.ProcAmpValues.Hue = m_Hue;
  vpblt.ProcAmpValues.Saturation = m_Saturation;
  vpblt.BackgroundColor.Y = 0x1000;
  vpblt.BackgroundColor.Cb = 0x8000;
  vpblt.BackgroundColor.Cr = 0x8000;
  vpblt.BackgroundColor.Alpha = 0xffff;

  num_fields = 1;
  nOutputPicIdx = 1;

  IDirect3DSurface9* pDstSurface;
  m_pTargetTexture->GetSurfaceLevel(0, &pDstSurface);

  hr = m_pDXVA2VideoProcessor->VideoProcessBlt(pDstSurface, &vpblt, &vs, 1, NULL);
  if (FAILED(hr))
  {
    CLog::Log(LOGDEBUG, "%s VideoProcessBlt failed", __FUNCTION__);
    return;
  }

  pD3DDevice->SetTexture(0, m_pTargetTexture);

  pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
  pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
  pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
  pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE  );
  pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
  pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
  pD3DDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
  pD3DDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
  pD3DDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
  pD3DDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );

  pD3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
  pD3DDevice->SetFVF( D3DFVF_XYZRHW | D3DFVF_TEX1 );

  // Render the image
  struct CUSTOMVERTEX {
    FLOAT x, y, z;
    FLOAT rhw;
    FLOAT tu, tv;   // Texture coordinates
  };

  // DXVA requires width and height to be rounded to 16
  // so we need to consider that when calculating the texture coordinates
  // on NPOT systems this is allready taken care
  float ratioX = 1.0;
  float ratioY = 1.0;

  if(g_Windowing.SupportsRT_NPOT())
  {
	ratioX = (float)m_sourceWidth / m_DXVATextureTargetWidth;
	ratioY = (float)m_sourceHeight / m_DXVATextureTargetHeight;
  }

  CUSTOMVERTEX verts[4] = 
  {
    {
      m_destRect.x1                                                      ,  m_destRect.y1, 0.0f, 1.0f,
        (m_sourceRect.x1) / m_sourceWidth * ratioX                , (m_sourceRect.y1) / m_sourceHeight * ratioY,
    },
    {
      m_destRect.x2                                                      ,  m_destRect.y1, 0.0f, 1.0f,
        (m_sourceRect.x2) / m_sourceWidth * ratioX               , (m_sourceRect.y1) / m_sourceHeight * ratioY,
      },
      {
        m_destRect.x2                                                      ,  m_destRect.y2, 0.0f, 1.0f,
          (m_sourceRect.x2) / m_sourceWidth * ratioX               , (m_sourceRect.y2) / m_sourceHeight * ratioY,
      },
      {
        m_destRect.x1                                                       ,  m_destRect.y2, 0.0f, 1.0f,
          (m_sourceRect.x1) / m_sourceWidth * ratioX                 , (m_sourceRect.y2) / m_sourceHeight * ratioY,
        }
  };

  pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(CUSTOMVERTEX));
  pD3DDevice->SetTexture(0, NULL);
   
  pDstSurface->Release();

  return;
}


// === ID3DResource
void CWinRenderer::OnDestroyDevice()
{
  ReleaseDXVAResources();
}

void CWinRenderer::OnCreateDevice()
{

}

void CWinRenderer::OnLostDevice()
{
  ReleaseDXVAResources();
}

void CWinRenderer::OnResetDevice()
{
  AllocateDXVAResources();
}

#endif
