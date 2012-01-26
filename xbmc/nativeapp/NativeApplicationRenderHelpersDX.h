#ifndef __NATIVE_APP_RENDER_HELPERS__H__
#define __NATIVE_APP_RENDER_HELPERS__H__

/* native applications - framework for boxee native applications
* Copyright (C) 2010 Boxee.tv.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "Application.h"
#include "FrameBufferObject.h"
#include "ApplicationMessenger.h"
#include "WindowingFactory.h"
#include "GUITexture.h"
#include "D3DResource.h"

static void LoadSurfaceToMem(BX_Surface* surface);
static void LoadSurfaceToGPU(BX_Surface* surface);

static D3DFORMAT BXFormatToD3D(BX_PixelFormat format)
{
  switch(format)
  {
  case BX_PF_BGRA8888:
    return D3DFMT_A8R8G8B8;
  case BX_PF_A8:
    return D3DFMT_A8;
  }

  return D3DFMT_UNKNOWN;
}

static DWORD RandomColor()
{
  float r = (float)rand() / RAND_MAX;
  float g = (float)rand() / RAND_MAX;
  float b = (float)rand() / RAND_MAX;

  return D3DXCOLOR(r, g, b, 1.0);
}

static RECT BXRect2D3D(BX_Rect &inRect, RECT &outRect)
{
  outRect.left = inRect.x;
  outRect.top = inRect.y;
  outRect.right = inRect.x + inRect.w;
  outRect.bottom = inRect.y + inRect.h;

  return outRect;
}

typedef struct sBXSurfacePrivData : public ID3DResource
{
  IDirect3DTexture9*        pTexture;
  bool                      initialized;
  bool                      bLoadedToGPU;
  BX_Surface*               pParent;
  BOXEE::NativeApplication  *app;

  sBXSurfacePrivData() : app(NULL), initialized(false), pTexture(NULL), bLoadedToGPU(false), pParent(NULL)
  { 

  }

  ~sBXSurfacePrivData() 
  { 
    g_Windowing.Unregister(this);
    Release();
  }

  void Initialize(BX_Surface *s = NULL)
  {
    HRESULT hr;
    LPDIRECT3DDEVICE9 pDevice;

    if (initialized)
      return;

    if(s == NULL)
    {
      if(pParent != NULL)
        s = pParent;
      else
        return;
    }
    else
      g_Windowing.Register(this);

    pDevice = g_Windowing.Get3DDevice();

    pParent = s;
    sBXSurfacePrivData* pPrivate = ((sBXSurfacePrivData *)s->priv);
    IDirect3DTexture9** pTexture = &(pPrivate->pTexture);

    if(*pTexture == NULL)
      hr = pDevice->CreateTexture(s->w, s->h, 1, D3DUSAGE_RENDERTARGET, 
      BXFormatToD3D(BX_PF_BGRA8888), D3DPOOL_DEFAULT, pTexture, NULL);

    initialized = true;
    bLoadedToGPU = false;
  }

  void Release()
  {
    if(pTexture)
      pTexture->Release();
    pTexture = NULL;
    initialized = false;
    bLoadedToGPU = false;
  }

  void OnLostDevice()
  {
    LoadSurfaceToMem(pParent);
    Release();
  }

  void OnDestroyDevice()
  {
  }

  void OnCreateDevice()
  {
  }

  void OnResetDevice()
  {
    Initialize();
    LoadSurfaceToGPU(pParent);
  }

} BXSurfacePrivData;


struct CUSTOMVERTEX {
  FLOAT x, y, z;
  DWORD color;
  FLOAT tu, tv;
};

struct CUSTOMVERTEX2 {
  FLOAT x, y, z;
  DWORD color;
};

static void BXRectToDXRect(BX_Rect& src, RECT& dst)
{
  dst.left = src.x;
  dst.top = src.y;
  dst.right = src.x + src.w;
  dst.bottom = src.y + src.h;
}

// Useful pixel color manipulation macros
#define GET_A(color)            ((color >> 24) & 0xFF)
#define GET_R(color)            ((color >> 16) & 0xFF)
#define GET_G(color)            ((color >>  8) & 0xFF)
#define GET_B(color)            ((color >>  0) & 0xFF)


static void AToRGB(unsigned char* dst, unsigned char* src, unsigned int len)
{
  for(unsigned int i = 0; i < len; i++)
  {
    dst[4 * i + 0] = 0;
    dst[4 * i + 1] = 0;
    dst[4 * i + 2] = 0;
    dst[4 * i + 3] = src[i];
  }
}

static void RGBToA(unsigned char* dst, unsigned char* src, unsigned int len)
{
  for(unsigned int i = 0; i < len; i++)
  {
    dst[i] = src[4 * i + 3];
  }
}

static DWORD BX2DXcolor(DWORD color)
{
  return D3DCOLOR_RGBA(GET_R(color), GET_G(color), GET_B(color), GET_A(color));
}

static DWORD BX2DXcolor(DWORD color, unsigned char alpha)
{
  return D3DCOLOR_RGBA(GET_R(color), GET_G(color), GET_B(color), alpha);
}

static void LoadSurfaceToGPU(BX_Surface* pSrc)
{
  HRESULT hr;

  if(!pSrc || !pSrc->pixels )
    return;

  LPDIRECT3DDEVICE9 pDevice = g_Windowing.Get3DDevice();
  sBXSurfacePrivData* pPrivate = ((sBXSurfacePrivData *)pSrc->priv);
  IDirect3DTexture9** pTexture = &(pPrivate->pTexture);

  IDirect3DSurface9* pSurface;
  hr = pDevice->CreateOffscreenPlainSurface(pSrc->w, pSrc->h, 
    BXFormatToD3D(BX_PF_BGRA8888), D3DPOOL_DEFAULT, &pSurface, NULL);

  D3DLOCKED_RECT lr;
  hr = pSurface->LockRect(&lr, NULL, 0); 
  unsigned char *dst = (unsigned char *)lr.pBits;
  unsigned char *src = (unsigned char *)pSrc->pixels;
  unsigned int dstPitch = lr.Pitch;
  unsigned int srcPitch = pSrc->pitch;
  unsigned int rows = pSrc->h;

  for (unsigned int y = 0; y < rows; y++)
  {
    if(pSrc->bpp == 4)
      memcpy(dst, src, std::min(srcPitch, dstPitch));
    else
      AToRGB(dst, src, srcPitch);
    src += srcPitch;
    dst += dstPitch;
  }

  pSurface->UnlockRect();

  IDirect3DSurface9* pDestSurface = NULL;
  hr = (*pTexture)->GetSurfaceLevel(0, &pDestSurface);
  hr = pDevice->StretchRect(pSurface, NULL, pDestSurface, NULL, D3DTEXF_LINEAR);
  pSurface->Release();
  pDestSurface->Release();

  pPrivate->bLoadedToGPU = true;
}

static void LoadSurfaceToMem(BX_Surface* surface)
{
  HRESULT hr;

  if(!surface || !surface->pixels)
    return;

  BXSurfacePrivData *srcPriv = (BXSurfacePrivData *)surface->priv;

  IDirect3DTexture9* pSrcTexture = NULL;
  IDirect3DSurface9* pSrcSurface = NULL;

  LPDIRECT3DDEVICE9 pDevice = g_Windowing.Get3DDevice();

  pSrcTexture = srcPriv->pTexture;
  
  if(!pSrcTexture)
    return;

  pSrcTexture->GetSurfaceLevel(0, &pSrcSurface);

  // copy surface memory into system memory
  // we might reset the device and delete the texture
  // so we make a copy
  IDirect3DSurface9* pDestSurface;
  hr = pDevice->CreateOffscreenPlainSurface(surface->w, surface->h, 
    BXFormatToD3D(BX_PF_BGRA8888), D3DPOOL_SYSTEMMEM, &pDestSurface, NULL);
  hr = pDevice->GetRenderTargetData(pSrcSurface, pDestSurface);

  D3DLOCKED_RECT lr;
  hr = pDestSurface->LockRect(&lr, NULL, 0); 
  unsigned char *dst = (unsigned char *)surface->pixels;
  unsigned char *src = (unsigned char *)lr.pBits;
  unsigned int dstPitch = surface->pitch;
  unsigned int srcPitch =  lr.Pitch;
  unsigned int rows = surface->h;

  for (unsigned int y = 0; y < rows; y++)
  {
    if(surface->bpp == 4)
      memcpy(dst, src, std::min(srcPitch, dstPitch));
    else
      RGBToA(dst, src, dstPitch);
    src += srcPitch;
    dst += dstPitch;
  }

  pSrcSurface->Release();
  pDestSurface->Release();
}

class FillRectJob : public IGUIThreadTask
{
public: 
  virtual void DoWork()
  {
    HRESULT hr;

    if (!surface || !surface->priv)
      return;

    BXSurfacePrivData *p = (BXSurfacePrivData *)surface->priv;
    if (!p->initialized)
      p->Initialize(surface);

    IDirect3DTexture9* pSrcTexture = NULL;
    IDirect3DSurface9* pSrcSurface = NULL;

    LPDIRECT3DDEVICE9 pDevice = g_Windowing.Get3DDevice();

    if (!surface || !surface->priv)
      return;

    pSrcTexture = p->pTexture;
    pSrcTexture->GetSurfaceLevel(0, &pSrcSurface);

    hr = pDevice->SetTexture( 0, NULL );

    IDirect3DSurface9* pOldRT;
    hr = pDevice->GetRenderTarget(0, &pOldRT);
    hr = pDevice->SetRenderTarget(0, pSrcSurface);
    hr = pDevice->SetDepthStencilSurface(NULL);

    if(blend == BX_BLEND_NONE)
      pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    else
      pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );

    pDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
    pDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    pDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
    pDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );

    hr = pDevice->SetFVF( D3DFVF_XYZ | D3DFVF_DIFFUSE );

    DWORD dxColor = BX2DXcolor(c);

    D3DXMATRIX Ortho2D;	
    D3DXMATRIX Identity;

    D3DXMatrixOrthoOffCenterLH(&Ortho2D, 0, surface->w, 0, surface->h, 0.0f, 1.0f);
    D3DXMatrixIdentity(&Identity);
    pDevice->SetTransform(D3DTS_PROJECTION, &Ortho2D);
    pDevice->SetTransform(D3DTS_WORLD, &Identity);
    pDevice->SetTransform(D3DTS_VIEW, &Identity);

    CUSTOMVERTEX2 verts[4] = 
    {
      { rect.x - 0.5f, 720 - rect.y - 0.5f, 0, dxColor},
      { rect.x - 0.5f, 720 - (rect.y + rect.h) - 0.5f , 0, dxColor},
      { rect.x + rect.w - 0.5f, 720 - rect.y - 0.5f, 0, dxColor},
      { rect.x + rect.w - 0.5f, 720 - (rect.y + rect.h) - 0.5f, 0, dxColor},
    };
    pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, verts, sizeof(CUSTOMVERTEX2));
    pDevice->SetRenderTarget(0, pOldRT); 
    pOldRT->Release();
    pSrcSurface->Release();
  }  
  BX_Surface* surface; 
  BX_Color c; 
  BX_Rect rect; 
  BX_BlendMethod blend;
};

class BlitJob : public IGUIThreadTask
{
public: 
  virtual void DoWork()
  {
    HRESULT hr;

    IDirect3DTexture9* pSrcTexture = NULL;
    IDirect3DTexture9* pDestTexture = NULL;

    IDirect3DSurface9* pSrcSurface = NULL;
    IDirect3DSurface9* pDestSurface = NULL;

    LPDIRECT3DDEVICE9 pDevice = g_Windowing.Get3DDevice();

    if (!destSurface || !destSurface->priv || !sourceSurface || !sourceSurface->priv)
      return;

    BXSurfacePrivData *srcPriv = (BXSurfacePrivData *)sourceSurface->priv;
    if (!srcPriv->initialized)
      srcPriv->Initialize(sourceSurface);

    BXSurfacePrivData *dstPriv = (BXSurfacePrivData *)destSurface->priv;
    if (!dstPriv->initialized)
      dstPriv->Initialize(destSurface);

    if(!srcPriv->bLoadedToGPU)
      LoadSurfaceToGPU(sourceSurface);

    pSrcTexture = srcPriv->pTexture;
    pSrcTexture->GetSurfaceLevel(0, &pSrcSurface);

    pDestTexture = dstPriv->pTexture;
    pDestTexture->GetSurfaceLevel(0, &pDestSurface); 

    hr = pDevice->SetTexture( 0, NULL );

    IDirect3DSurface9* pOldRT;
    hr = pDevice->GetRenderTarget(0, &pOldRT);
    hr = pDevice->SetRenderTarget(0, pDestSurface);
    hr = pDevice->SetDepthStencilSurface(NULL);
    hr = pDevice->SetTexture( 0, pSrcTexture);

    if(blend == BX_BLEND_NONE)
      pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    else
      pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );

    if(sourceSurface->bpp == 1 && destSurface->bpp == 4)
    {
      pDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
      pDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
      pDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
      pDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
      alpha = 255;
    }
    else
    {
      pDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
      pDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
      pDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
      pDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
      color = 0xffffffff;
      
    }

    if(alpha == 0)
      alpha = 255;

    pDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    pDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    hr = pDevice->SetFVF( D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 );

    DWORD dxColor = BX2DXcolor(color, alpha);

    float w = sourceSurface->w;
    float h = sourceSurface->h;

    float x1, y1, x2, y2;

    x1 = sourceRect.x;
    y1 = sourceRect.y;
    x2 = sourceRect.x + sourceRect.w;
    y2 = sourceRect.y + sourceRect.h;

    D3DXMATRIX Ortho2D;	
    D3DXMATRIX Identity;

    D3DXMatrixOrthoOffCenterLH(&Ortho2D, 0, destSurface->w, 0, destSurface->h, 0.0f, 1.0f);
    D3DXMatrixIdentity(&Identity);
    pDevice->SetTransform(D3DTS_PROJECTION, &Ortho2D);
    pDevice->SetTransform(D3DTS_WORLD, &Identity);
    pDevice->SetTransform(D3DTS_VIEW, &Identity);

    CUSTOMVERTEX verts[4] = 
    {
      { destRect.x - 0.5f, 720 - destRect.y - 0.5f, 0, dxColor, x1/w, y1/h},
      { destRect.x - 0.5f, 720 - (destRect.y + destRect.h) - 0.5f , 0, dxColor, x1/w, y2/h},
      { destRect.x + destRect.w - 0.5f, 720 - destRect.y - 0.5f, 0, dxColor, x2/w, y1/h},
      { destRect.x + destRect.w - 0.5f, 720 - (destRect.y + destRect.h) - 0.5f, 0, dxColor, x2/w, y2/h},
    };
    pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, verts, sizeof(CUSTOMVERTEX));
    hr = pDevice->SetRenderTarget(0, pOldRT);
    pOldRT->Release();
    pSrcSurface->Release();
    pDestSurface->Release();
  }  

  BX_Surface* sourceSurface;
  BX_Surface* destSurface;
  BX_Rect sourceRect;
  BX_Rect destRect;
  BX_BlendMethod blend;
  BX_Color color;
  unsigned char alpha;
};

class FlipJob : public IGUIThreadTask
{
public:
  FlipJob(BX_Surface *s) : surface(s) {}
  virtual void DoWork()
  {
    HRESULT hr;

    // we need to allocate our own memory here
    if(!surface->pixels)
    {
      surface->pixels = new unsigned char[surface->h * surface->pitch];
    }
  }

  BX_Surface* surface;
};

class CreateSurfaceJob : public IGUIThreadTask
{
public: 
  CreateSurfaceJob(BX_Handle app, BX_PixelFormat pFormat, unsigned int w, unsigned int h) : 
      hApp(app), pixelFormat(pFormat), nWidth(w), nHeight(h), surface(NULL) { }

      virtual void DoWork()
      {
        BX_Surface *s = new BX_Surface;
        s->hApp = hApp;
        s->pixelFormat = pixelFormat;
        s->w = nWidth;
        s->h = nHeight;

        if (pixelFormat == BX_PF_BGRA8888)
        {
          s->bpp = 4;
        }
        else
        {
          s->bpp = 1;
        }

        s->pitch = s->w * s->bpp ;
        s->pixels = NULL;

        BXSurfacePrivData *p = new BXSurfacePrivData;
        p->app = (BOXEE::NativeApplication*)hApp->boxeeData;

        s->priv = p;    
        surface = s;
      }  

      BX_Handle      hApp; 
      BX_PixelFormat pixelFormat;
      unsigned int   nWidth; 
      unsigned int   nHeight;
      BX_Surface     *surface;
};

class FreeSurfaceJob : public IGUIThreadTask
{
public: 
  FreeSurfaceJob(BX_Surface* s) : surface(s) { }
  virtual void DoWork()
  {
    if (surface)
    {
      if (surface->priv)
      {
        BXSurfacePrivData *priv = (BXSurfacePrivData *)surface->priv;
        delete priv;
      }

      if (surface->pixels)
        delete [] surface->pixels;

      delete surface;
    }
  }  
  BX_Surface* surface;
};

class LockSurfaceJob : public IGUIThreadTask
{
public: 
  LockSurfaceJob(BX_Surface* s) : surface(s) { }
  virtual void DoWork()
  {
    if (surface)
    {
      //((BOXEE::NativeApplication *)(surface->hApp->boxeeData))->ExecuteRenderOperations();

      BXSurfacePrivData *p = (BXSurfacePrivData *)surface->priv;
      if (!p->initialized)
        p->Initialize(surface);

      if (!surface->pixels)
      {
        surface->pixels = new unsigned char[surface->h * surface->pitch];
        memset(surface->pixels, 128, surface->h * surface->pitch);
      }
      p->bLoadedToGPU = false;
    }
  }  
  BX_Surface* surface;
};

class UnlockSurfaceJob : public IGUIThreadTask
{
public: 
  UnlockSurfaceJob(BX_Surface* s) : surface(s) { }
  virtual void DoWork()
  {
    if (surface && surface->pixels)
    {
      BXSurfacePrivData *p = (BXSurfacePrivData *)surface->priv;
      if (!p->initialized)
        p->Initialize(surface);

      if(!p->bLoadedToGPU)
        LoadSurfaceToGPU(surface);
    }
  }  
  BX_Surface* surface;
};

class ClearSurfaceJob : public IGUIThreadTask
{
public:
  ClearSurfaceJob(BX_Surface* s, float a=1.0) : surface(s), alpha(a) { }
  virtual void DoWork()
  {
    HRESULT hr;	

    BXSurfacePrivData *srcPriv = (BXSurfacePrivData *)surface->priv;
    if (!srcPriv->initialized)
      srcPriv->Initialize(surface);

    IDirect3DTexture9* pSrcTexture = NULL;
    IDirect3DSurface9* pSrcSurface = NULL;

    LPDIRECT3DDEVICE9 pDevice = g_Windowing.Get3DDevice();

    pSrcTexture = srcPriv->pTexture;
    pSrcTexture->GetSurfaceLevel(0, &pSrcSurface);

    D3DCOLOR color = D3DXCOLOR(0, 0, 0, alpha);

    hr = pDevice->SetTexture( 0, NULL );

    IDirect3DSurface9* pOldRT;
    hr = pDevice->GetRenderTarget(0, &pOldRT);
    hr = pDevice->SetRenderTarget(0, pSrcSurface);
    hr = pDevice->SetDepthStencilSurface(NULL);
    hr = pDevice->Clear(0, NULL, D3DCLEAR_TARGET, color, 1.0, 0);
    hr = pDevice->SetRenderTarget(0, pOldRT);
    pOldRT->Release();
    pSrcSurface->Release();
  }
protected:
  BX_Surface* surface;  
  float       alpha;
};

class NativeAppRenderToScreenHelper
{
public:
  NativeAppRenderToScreenHelper(BX_Surface* s) : surface(s)
  {

  }

  void Render()
  {
    HRESULT hr;

    BXSurfacePrivData *srcPriv = (BXSurfacePrivData *)surface->priv;
    if (!srcPriv->initialized)
      srcPriv->Initialize(surface);

    IDirect3DTexture9* pSrcTexture = NULL;

    LPDIRECT3DDEVICE9 pDevice = g_Windowing.Get3DDevice();

    hr = pDevice->SetTexture( 0, NULL);

    pSrcTexture = srcPriv->pTexture;
    hr = pDevice->SetTexture( 0, pSrcTexture);

    pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    pDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
    pDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    pDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );

    hr = pDevice->SetFVF( D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 );

    DWORD color = 0xffffffff;
    
    RESOLUTION iRes = g_graphicsContext.GetVideoResolution();
    float offsetX = g_settings.m_ResInfo[iRes].Overscan.left;
    float offsetY = g_settings.m_ResInfo[iRes].Overscan.top;
    float width   = g_settings.m_ResInfo[iRes].Overscan.right - g_settings.m_ResInfo[iRes].Overscan.left;
    float height  = g_settings.m_ResInfo[iRes].Overscan.bottom - g_settings.m_ResInfo[iRes].Overscan.top; 

    D3DXMATRIX Ortho2D;	
    D3DXMATRIX Identity;

    D3DXMatrixOrthoOffCenterLH(&Ortho2D, 0, width, 0, height, 0.0f, 1.0f);
    D3DXMatrixIdentity(&Identity);

    pDevice->SetTransform(D3DTS_PROJECTION, &Ortho2D);
    pDevice->SetTransform(D3DTS_WORLD, &Identity);
    pDevice->SetTransform(D3DTS_VIEW, &Identity);

    D3DVIEWPORT9 vp;
    vp.X = offsetX;
    vp.Y = offsetY;
    vp.Width = width;
    vp.Height = height;
    vp.MinZ = 0;
    vp.MaxZ = 0;
    pDevice->SetViewport(&vp);

    CUSTOMVERTEX verts[4] = 
    {
      { 0-0.5, 0-0.5, 0, color, 0, 1.0,},
      { 0-0.5, height-0.5 , 0, color, 0, 0},
      { width-0.5, 0-0.5, 0, color, 1.0, 1.0},
      { width-0.5, height-0.5, 0, color, 1.0, 0},
    };

    if (g_application.IsPlayingVideo())
    {
      pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    }	

    pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, verts, sizeof(CUSTOMVERTEX));
  }
protected:
  BX_Surface* surface;
};

#endif

