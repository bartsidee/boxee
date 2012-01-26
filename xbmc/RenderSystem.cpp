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

#include "RenderSystem.h"
#include "GraphicContext.h"
#include "Texture.h"
#include "GUITexture.h"
#include "GUISettings.h"
#include "Settings.h"

CRenderSystemBase::CRenderSystemBase()
{
  m_bRenderCreated = false;
  m_bVSync = true;
  m_maxTextureSize = 2048;
  m_RenderVersionMajor = 0;
  m_RenderVersionMinor = 0;
  m_nShaderModelVersionMajor = 0;
  m_nShaderModelVersionMinor = 0;
  m_renderCaps = 0;
  m_strErrorMessage = "";
  m_textureMemorySize = 0;
  m_minDXTPitch = 0;
  memset(&m_lastOverScan, 0, sizeof(OVERSCAN));
}

CRenderSystemBase::~CRenderSystemBase()
{

}

bool CRenderSystemBase::InitRenderSystem()
{
  return true;
}

bool CRenderSystemBase::BeginRender()
{
  int mode3d = MODE_3D_NONE;
#ifdef HAS_EMBEDDED
  mode3d = g_guiSettings.GetInt("videoscreen.mode3d");
#endif

  if (mode3d != MODE_3D_NONE)
  {
    unsigned int width = g_graphicsContext.GetWidth();
    unsigned int height = g_graphicsContext.GetHeight();

    if (width != m_FBO.GetWidth() || height != m_FBO.GetHeight())
    {
      InitializeFBO();
    }

    if (!m_FBO.IsValid())
    {
      InitializeFBO();
      if (!m_FBO.IsValid())
        return true;
    }

    m_FBO.BeginRender();
    g_graphicsContext.Clear();
  }
  else
    m_FBO.Cleanup();

  return true;
}

bool CRenderSystemBase::EndRender()
{
  int mode3d = MODE_3D_NONE;
#ifdef HAS_EMBEDDED
  mode3d = g_guiSettings.GetInt("videoscreen.mode3d");
#endif

  if (mode3d != MODE_3D_NONE)
  {
    unsigned int width = g_graphicsContext.GetWidth();
    unsigned int height = g_graphicsContext.GetHeight();

    if (!m_FBO.IsValid())
      return true;

    m_FBO.EndRender();

    g_graphicsContext.Clear();

    g_graphicsContext.PushTransform(TransformMatrix(), true);
    g_graphicsContext.PushViewPort(0, 0, 0, 0, false);

    bool clip = g_graphicsContext.SetClipRegion(0, 0, 0, 0, false);

    uint32_t diffuseColor = 0xffffffff;

    CTexture texture;
    texture.LoadFromTexture((int) width, (int) height, XB_FMT_R8G8B8A8,
        m_FBO.Texture());

    CTextureInfo textureInfo("");
    textureInfo.blendCenter = true;
    textureInfo.orientation = 3; // flipy

    unsigned int x1, y1, newWidth, newHeight;
    x1 = y1 = newWidth = newHeight = 0;

    if(mode3d == MODE_3D_SBS)
    {
      x1 = width / 2;
      y1 = 0;
      newWidth = g_graphicsContext.GetWidth() / 2;
      newHeight = g_graphicsContext.GetHeight();
    }
    else if (mode3d == MODE_3D_OU)
    {
      x1 = 0;
      y1 = height / 2;
      newWidth = g_graphicsContext.GetWidth();
      newHeight = g_graphicsContext.GetHeight() / 2;
    }

    CGUITexture guiTexture1(0, 0, (float) width, (float) height, textureInfo, &texture);
    CGUITexture guiTexture2(x1, y1, (float) width, (float) height, textureInfo, &texture);

    guiTexture1.SetWidth(newWidth);
    guiTexture2.SetWidth(newWidth);

    guiTexture1.SetHeight(newHeight);
    guiTexture2.SetHeight(newHeight);

    guiTexture1.SetDiffuseColor(diffuseColor);
    guiTexture2.SetDiffuseColor(diffuseColor);

    guiTexture1.Render();
    guiTexture2.Render();

    g_graphicsContext.PopTransform();
    g_graphicsContext.PopViewPort();
    if (clip)
      g_graphicsContext.RestoreClipRegion();
  }

  return true;
}

void CRenderSystemBase::GetRenderVersion(unsigned int& major, unsigned int& minor) const
{
  major = m_RenderVersionMajor;
  minor = m_RenderVersionMinor;
}

void CRenderSystemBase::GetShaderVersion(unsigned int& major, unsigned int& minor) const
{
  major = m_nShaderModelVersionMajor;
  minor = m_nShaderModelVersionMinor;
}

bool CRenderSystemBase::ResetRenderSystem(int width, int height, bool fullScreen, float refreshRate)
{
  m_width = width;
  m_height = height;

  TransformMatrix matMV, matProj;
  ApplyHardwareTransform(MATRIX_TYPE_MODEL_VIEW, matMV);
  matProj.MatrixOrtho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
  ApplyHardwareTransform(MATRIX_TYPE_PROJECTION, matProj);

  EnableBlending(false, false);
  EnableStencil(false);

  return true;
}

bool CRenderSystemBase::SupportsNPOT(bool dxt) const
{
  if (dxt)
    return (m_renderCaps & RENDER_CAPS_DXT_NPOT);
  return m_renderCaps & RENDER_CAPS_NPOT;
}

bool CRenderSystemBase::SupportsRT_NPOT() const
{
  return m_renderCaps & RENDER_CAPS_RT_NPOT;
}

bool CRenderSystemBase::SupportsDXT() const
{
  return m_renderCaps & RENDER_CAPS_DXT;
}

void CRenderSystemBase::ApplyHardwareTransform(TransformMatrixType type, const TransformMatrix &matrix)
{ 
  if(type == MATRIX_TYPE_MODEL_VIEW)
  {
    m_matModelView = matrix;
    if(!m_matModelView.IsColumnMajor())
      m_matModelView.Transpose();
  }
  else if(type == MATRIX_TYPE_PROJECTION)
  {
    m_matProjection = matrix;
  }
}

TransformMatrix* CRenderSystemBase::GetHardwareTransform(TransformMatrixType type)
{
  if(type == MATRIX_TYPE_MODEL_VIEW)
    return &m_matModelView;
  else if(type ==MATRIX_TYPE_PROJECTION)
    return &m_matProjection;
  else
    return NULL;
}

void CRenderSystemBase::InitializeFBO()
{
  unsigned int width;
  unsigned int height;

  width = g_graphicsContext.GetWidth();
  height = g_graphicsContext.GetHeight();

#if defined(HAS_GL) || defined(HAS_GLES)
  m_FBO.Cleanup();
  m_FBO.Initialize();
  m_FBO.CreateAndBindToTexture(GL_TEXTURE_2D, (int)width, (int)height);
#endif
}

Mode3D CRenderSystemBase::GetMode3D()
{
  if (g_guiSettings.GetBool("videoscreen.tv3dfc") == false)
  {
    return MODE_3D_NONE;
  }

  return (Mode3D) g_guiSettings.GetInt("videoscreen.mode3d");
}

void CRenderSystemBase::SetMode3D(Mode3D mode)
{
  if (g_guiSettings.GetBool("videoscreen.tv3dfc") == false)
  {
    g_guiSettings.SetInt("videoscreen.mode3d", (int) MODE_3D_NONE);
  }
  else
  {
    g_guiSettings.SetInt("videoscreen.mode3d", (int) mode);
  }

  // disable overscan when using 3D mode
  if (mode != MODE_3D_NONE)
  {
    m_lastOverScan = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan;
    g_graphicsContext.ResetOverscan(g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()]);
  }
  else
  {
    if(m_lastOverScan.bottom != 0 && m_lastOverScan.right != 0)
      g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan = m_lastOverScan;
  }
}
