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

#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#pragma once

#include "Geometry.h"
#include "TransformMatrix.h"
#include "StdString.h"
#include "FrameBufferObject.h"
#include "Resolution.h"
#include <stdint.h>



typedef enum _RenderingSystemType
{
  RENDERING_SYSTEM_OPENGL,
  RENDERING_SYSTEM_DIRECTX,
  RENDERING_SYSTEM_OPENGLES
} RenderingSystemType;

typedef enum _TransformMatrixType
{
  MATRIX_TYPE_MODEL_VIEW,
  MATRIX_TYPE_PROJECTION
} TransformMatrixType;


/*
*   CRenderSystemBase interface allows us to create the rendering engine we use.
*   We currently have two engines: OpenGL and DirectX
*   This interface is very basic since a lot of the actual details will go in to the derived classes
*/

typedef uint32_t color_t;

enum
{
  RENDER_CAPS_DXT      = (1 << 0),
  RENDER_CAPS_NPOT     = (1 << 1),
  RENDER_CAPS_DXT_NPOT = (1 << 2),
  RENDER_CAPS_RT_NPOT  = (1 << 3)
};

typedef enum _StencilFunc 
{
  STENCIL_FUNC_NEVER  = 0,
  STENCIL_FUNC_LESS = 1,
  STENCIL_FUNC_EQUAL = 2,
  STENCIL_FUNC_LEQUAL = 3,
  STENCIL_FUNC_GREATER = 4,
  STENCIL_FUNC_NOTEQUAL = 5,
  STENCIL_FUNC_GEQUAL = 6,
  STENCIL_FUNC_ALWAYS = 7
} StencilFunc;

typedef enum _StencilOp
{
  STENCIL_OP_KEEP = 0,
  STENCIL_OP_REPLACE = 1,
  STENCIL_OP_INCR = 2,
  STENCIL_OP_DECR = 3,
  STENCIL_OP_INVERT = 4,
  STENCIL_OP_INCR_WRAP = 5,
  STENCIL_OP_DECR_WRAP = 6,
} StencilOp;

typedef enum _Mode3D
{
  MODE_3D_NONE = 0,
  MODE_3D_SBS = 1,
  MODE_3D_OU = 2
} Mode3D;

class CRenderSystemBase
{
public:
  CRenderSystemBase();
  virtual ~CRenderSystemBase();

  // Retrieve 
  RenderingSystemType GetRenderingSystemType() { return m_enumRenderingSystem; }

  virtual bool InitRenderSystem() = 0;
  virtual bool DestroyRenderSystem() = 0;
  virtual bool ResetRenderSystem(int width, int height, bool fullScreen, float refreshRate);

  virtual bool BeginRender();
  virtual bool EndRender();
  virtual bool PresentRender() = 0;
  virtual bool ClearBuffers(color_t color) { return false; }
  virtual bool ClearBuffers(float r, float g, float b, float a) { return false; }
  virtual void ClearStencilBuffer(int val) {}
  virtual bool IsExtSupported(const char* extension) = 0;

  virtual void SetVSync(bool vsync) = 0;
  bool GetVSync() { return m_bVSync; }

  virtual void SetViewPort(CRect& viewPort) = 0;
  virtual void GetViewPort(CRect& viewPort) = 0;

  virtual void EnableClipping(bool bEnable) {}
  virtual void CaptureStateBlock() = 0;
  virtual void ApplyStateBlock() = 0;

  virtual void ApplyHardwareTransform(TransformMatrixType type, const TransformMatrix &matrix);
  virtual TransformMatrix* GetHardwareTransform(TransformMatrixType type);
 
  virtual void ApplyClippingRect(CRect& clipRect) = 0;
  virtual void GetClipRect(CRect& clipRect) = 0;

  virtual bool TestRender() = 0;

  void GetRenderVersion(unsigned int& major, unsigned int& minor) const;
  void GetShaderVersion(unsigned int& major, unsigned int& minor) const;
  const CStdString& GetRenderVendor() const { return m_RenderVendor; }
  const CStdString& GetRenderRenderer() const { return m_RenderRenderer; }
  virtual void* GetRenderContext() { return NULL; }
  bool SupportsDXT() const;
  bool SupportsNPOT(bool dxt) const;
  bool SupportsRT_NPOT() const;
  unsigned int GetMaxTextureSize() const { return m_maxTextureSize; }
  unsigned int GetTextureMemorySize() const { return m_textureMemorySize; } // in MB

  virtual void EnableTexture(bool bEnable) {}
  virtual void EnableBlending(bool bEnableRGB, bool bEnableAlpha = false) = 0;

  virtual void EnableStencil(bool bEnable) {}
  virtual void EnableDepthTest(bool bEnable) {}
  virtual void SetStencilFunc(StencilFunc func, int ref, unsigned int mask) {}
  virtual void SetStencilOp (StencilOp fail_op, StencilOp fail, StencilOp pass) {}
  virtual void SetColorMask(bool r, bool g, bool b, bool a) {}
  
  virtual void InitializeFBO();

  Mode3D GetMode3D();
  void SetMode3D(Mode3D mode);

  CStdString GetRenderSystemErrorStatus() { return m_strErrorMessage; }

protected:
  bool                m_bRenderCreated;
  RenderingSystemType m_enumRenderingSystem;
  bool                m_bVSync;
  unsigned int        m_maxTextureSize;
  unsigned int        m_textureMemorySize; // in MB

  CStdString   m_RenderRenderer;
  CStdString   m_RenderVendor;
  int          m_RenderVersionMinor;
  int          m_RenderVersionMajor;
  unsigned int m_nShaderModelVersionMajor;
  unsigned int m_nShaderModelVersionMinor;

  unsigned int m_renderCaps;

  TransformMatrix m_matModelView;
  TransformMatrix m_matProjection;

  int        m_width;
  int        m_height;

  unsigned int        m_minDXTPitch;

  CStdString  m_strErrorMessage;

  CFrameBufferObject m_FBO;

  OVERSCAN m_lastOverScan;
};

#endif // RENDER_SYSTEM_H
