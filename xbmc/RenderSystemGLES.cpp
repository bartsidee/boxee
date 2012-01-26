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

#if defined(HAS_GLES)

#include "GraphicContext.h"
#include "Settings.h"
#include "RenderSystemGLES.h"
#include "AdvancedSettings.h"
#include "TimeUtils.h"
#include "log.h"
#include "Application.h"
#include "TextureGL.h"
#include "GUITextureGLES.h"

using namespace Shaders;

CRenderSystemGLES::CRenderSystemGLES() : CRenderSystemBase()
{
  m_enumRenderingSystem = RENDERING_SYSTEM_OPENGLES;
}

CRenderSystemGLES::~CRenderSystemGLES()
{
  DestroyRenderSystem();
}

bool CRenderSystemGLES::InitRenderSystem()
{
  m_bVSync = false;
  m_iVSyncMode = 0;
  m_iSwapStamp = 0;
  m_iSwapTime = 0;
  m_iSwapRate = 0;
  m_bVsyncInit = false;
  m_maxTextureSize = 2048;


  // Get the GL version number 
  m_RenderVersionMajor = 0;
  m_RenderVersionMinor = 0;

  const char* ver = (const char*)glGetString(GL_VERSION);
  if (ver != 0)
    sscanf(ver, "%d.%d", &m_RenderVersionMajor, &m_RenderVersionMinor);

  // Get our driver vendor and renderer
  m_RenderVendor = (const char*) glGetString(GL_VENDOR);
  m_RenderRenderer = (const char*) glGetString(GL_RENDERER);
  m_renderCaps = RENDER_CAPS_DXT | RENDER_CAPS_NPOT | RENDER_CAPS_DXT_NPOT;
#if defined(HAS_GLEW)
  // init glew library
  GLenum err = glewInit();
  if (GLEW_OK != err)
  {
    // Problem: glewInit failed, something is seriously wrong
    return false;
  }
#endif

  // set matrix identity
  m_matModelView.Reset();
  m_matProjection.Reset();

  LogGraphicsInfo();

  m_bRenderCreated = true;

  return true;
}

bool CRenderSystemGLES::ResetRenderSystem(int width, int height, bool fullScreen, float refreshRate)
{
  CRenderSystemBase::ResetRenderSystem(width, height, fullScreen, refreshRate);

  glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

  CalculateMaxTexturesize();

  glViewport(0, 0, width, height);
  glScissor(0, 0, width, height);

  glEnable(GL_TEXTURE_2D); 
  glDisable(GL_SCISSOR_TEST);

  glDisable(GL_DEPTH_TEST);  

  return true;
}

bool CRenderSystemGLES::DestroyRenderSystem()
{
  m_bRenderCreated = false;

  return true;
}

bool CRenderSystemGLES::BeginRender()
{
  if (!m_bRenderCreated)
    return false;

  return CRenderSystemBase::BeginRender();
}

bool CRenderSystemGLES::EndRender()
{
  if (!m_bRenderCreated)
    return false;

  return CRenderSystemBase::EndRender();
}

bool CRenderSystemGLES::ClearBuffers(color_t color)
{
  if (!m_bRenderCreated)
    return false;

  float r = (GLfloat)GET_R(color) / 255.0f;
  float g = (GLfloat)GET_G(color) / 255.0f;
  float b = (GLfloat)GET_B(color) / 255.0f;
  float a = (GLfloat)GET_A(color) / 255.0f;

  ClearBuffers(r, g, b, a);

  return true;
}

bool CRenderSystemGLES::ClearBuffers(float r, float g, float b, float a)
{
  if (!m_bRenderCreated)
    return false;

  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT);

  return true;
}

void CRenderSystemGLES::ClearStencilBuffer(int val)
{
  glClearStencil(val);
  glClear(GL_STENCIL_BUFFER_BIT);
}

bool CRenderSystemGLES::IsExtSupported(const char* extension)
{
  return true;
}

bool CRenderSystemGLES::PresentRender()
{
  if (!m_bRenderCreated)
    return false;

#ifdef HAS_FRAMELIMITER
  if (m_iVSyncMode != 0 && m_iSwapRate != 0) 
  {
    int64_t curr, diff, freq;
    curr = CurrentHostCounter();
    freq = CurrentHostFrequency();

    if(m_iSwapStamp == 0)
      m_iSwapStamp = curr;

    /* calculate our next swap timestamp */
    diff = curr - m_iSwapStamp;
    diff = m_iSwapRate - diff % m_iSwapRate;
    m_iSwapStamp = curr + diff;

    /* sleep as close as we can before, assume 1ms precision of sleep *
     * this should always awake so that we are guaranteed the given   *
     * m_iSwapTime to do our swap                                     */
    diff = (diff - m_iSwapTime) * 1000 / freq;
    if (diff > 0)
      Sleep((DWORD)diff);
  }
#endif

  bool result = PresentRenderImpl();

#ifdef HAS_FRAMELIMITER
  if (m_iVSyncMode && m_iSwapRate != 0)
  {
    int64_t curr, diff;
    curr = CurrentHostCounter();

    diff = curr - m_iSwapStamp;
    m_iSwapStamp = curr;

    /*
    if (fabs(diff - m_iSwapRate) < fabs(diff))
      CLog::Log(LOGDEBUG, "%s - missed requested swap",__FUNCTION__);
     */
  }
#endif

  return result;
}

void CRenderSystemGLES::SetVSync(bool enable)
{
#ifdef HAS_EMBEDDED
  return;
#endif

  if (m_bVSync==enable && m_bVsyncInit == true)
    return;

  if (!m_bRenderCreated)
    return;

  if (enable)
    CLog::Log(LOGDEBUG, "GL: Enabling VSYNC");
  else
    CLog::Log(LOGDEBUG, "GL: Disabling VSYNC");

  m_iVSyncMode   = 0;
  m_iVSyncErrors = 0;
  m_iSwapRate    = 0;
  m_bVSync       = enable;
  m_bVsyncInit   = true;

  SetVSyncImpl(enable);

#ifdef HAS_FRAMELIMITER
  if (g_advancedSettings.m_ForcedSwapTime != 0.0)
  {
    /* some hardware busy wait on swap/glfinish, so we must manually sleep to avoid 100% cpu */
    double rate = g_graphicsContext.GetFPS();
    if (rate <= 0.0 || rate > 1000.0)
    {
      CLog::Log(LOGWARNING, "Unable to determine a valid horizontal refresh rate, vsync workaround disabled %.2g", rate);
      m_iSwapRate = 0;
    }
    else
    {
      int64_t freq;
      freq = CurrentHostFrequency();
      m_iSwapRate   = (__int64)((double)freq / rate);
      m_iSwapTime   = (__int64)(0.001 * g_advancedSettings.m_ForcedSwapTime * freq);
      m_iSwapStamp  = 0;
      CLog::Log(LOGDEBUG, "GL: Using artificial vsync sleep with rate %f", rate);
      if(!m_iVSyncMode)
        m_iVSyncMode = 1;
    }
  }
#endif

  if (!m_iVSyncMode)
    CLog::Log(LOGERROR, "GL: Vertical Blank Syncing unsupported");
  else
    CLog::Log(LOGDEBUG, "GL: Selected vsync mode %d", m_iVSyncMode);
}

void CRenderSystemGLES::CaptureStateBlock()
{
  return;
}

void CRenderSystemGLES::ApplyStateBlock()
{
  return;
}

bool CRenderSystemGLES::TestRender()
{
  GLfloat vVertices[] = {0.0f, 0.5f, 0.0f,
      -0.5f, -0.5f, 0.0f,
      0.5f, -0.5f, 0.0f};
  // Set the viewport
  glViewport(0, 0, m_width, m_height);
  // Clear the color buffer
  glClear(GL_COLOR_BUFFER_BIT);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
  glEnableVertexAttribArray(0);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  return true;
}

void CRenderSystemGLES::CalculateMaxTexturesize()
{
  CLog::Log(LOGDEBUG, "GL: Maximum texture width: %u", m_maxTextureSize);
}

void CRenderSystemGLES::GetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;

  GLint glvp[4];
  glGetIntegerv(GL_VIEWPORT, glvp);

  viewPort.x1 = (float)glvp[0];
  viewPort.y1 = (float)(m_height - glvp[1] - glvp[3]);
  viewPort.x2 = (float)(glvp[0] + glvp[2]);
  viewPort.y2 = (float)(viewPort.y1 + glvp[3]);
}

void CRenderSystemGLES::SetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;

  glScissor((GLint) viewPort.x1, (GLint) (m_height - viewPort.y1 - viewPort.Height()), (GLsizei) viewPort.Width(), (GLsizei) viewPort.Height());
  glViewport((GLint) viewPort.x1, (GLint) (m_height - viewPort.y1 - viewPort.Height()), (GLsizei) viewPort.Width(), (GLsizei) viewPort.Height());
}

void CRenderSystemGLES::EnableClipping(bool bEnable)
{
  if(bEnable)
    glEnable(GL_SCISSOR_TEST);
  else
    glDisable(GL_SCISSOR_TEST);
}

void CRenderSystemGLES::ApplyClippingRect(CRect& clipRect)
{
  glScissor((GLint) clipRect.x1, (GLint) (m_height - clipRect.y1 - clipRect.Height()), (GLsizei) clipRect.Width(), (GLsizei) clipRect.Height());
  glEnable(GL_SCISSOR_TEST);
}

void CRenderSystemGLES::GetClipRect(CRect& clipRect)
{
  GLint box[4];
  glGetIntegerv(GL_SCISSOR_BOX, box);

  clipRect.x1 = box[0];
  clipRect.y1 = box[1];
  clipRect.x2 = box[0] + box[2];
  clipRect.y2 = box[1] + box[3];
}

void CRenderSystemGLES::EnableTexture(bool bEnable)
{
  if(bEnable)
    glEnable(GL_TEXTURE_2D);
  else
    glDisable(GL_TEXTURE_2D);
}

void CRenderSystemGLES::EnableBlending(bool bEnableRGB, bool bEnableAlpha)
{
  GLenum srcRGB = GL_SRC_ALPHA;
  GLenum dstRGB = bEnableRGB ? GL_ONE_MINUS_SRC_ALPHA : GL_DST_ALPHA;
  GLenum srcAlpha = GL_SRC_ALPHA;
  GLenum dstAlpha = bEnableAlpha ? GL_SRC_ALPHA : GL_ONE;

#ifdef HAS_GLEW
  if (GLEW_EXT_blend_func_separate)
    glBlendFuncSeparateEXT(srcRGB, dstRGB, srcAlpha, dstAlpha);
  else
    glBlendFunc(srcRGB, dstRGB);
#else
  glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
#endif

  if(bEnableRGB || bEnableAlpha)
    glEnable(GL_BLEND);
  else
    glDisable(GL_BLEND);
}

void CRenderSystemGLES::EnableStencil(bool bEnable)
{
  if(bEnable)
    glEnable(GL_STENCIL_TEST);
  else
    glDisable(GL_STENCIL_TEST);
}

void CRenderSystemGLES::EnableDepthTest(bool bEnable)
{
  if(bEnable)
    glEnable(GL_DEPTH_TEST);
  else
    glDisable(GL_DEPTH_TEST);
}

void CRenderSystemGLES::SetStencilFunc(StencilFunc func, int ref, unsigned int mask)
{
  GLenum glFunc = 0;

  switch(func)
  {
  case STENCIL_FUNC_NEVER:
    glFunc = GL_NEVER;
    break;
  case STENCIL_FUNC_LESS:
    glFunc = GL_LESS;
    break;
  case STENCIL_FUNC_EQUAL:
    glFunc = GL_EQUAL;
    break;
  case STENCIL_FUNC_LEQUAL:
    glFunc = GL_LEQUAL;
    break;
  case STENCIL_FUNC_GREATER:
    glFunc = GL_GREATER;
    break;
  case STENCIL_FUNC_NOTEQUAL:
    glFunc = GL_NOTEQUAL;
    break;
  case STENCIL_FUNC_GEQUAL:
    glFunc = GL_GEQUAL;
    break;
  case STENCIL_FUNC_ALWAYS:
    glFunc = GL_ALWAYS;
    break;
  }

  glStencilFunc(glFunc, ref, mask);
};

void CRenderSystemGLES::SetStencilOp (StencilOp fail_op, StencilOp fail, StencilOp pass)
{
  GLenum glFail_op = 0;
  GLenum glFail = 0;
  GLenum glPass = 0;

  switch(fail_op)
  {
  case STENCIL_OP_KEEP:
    glFail_op = GL_KEEP;
    break;
  case STENCIL_OP_REPLACE:
    glFail_op = GL_REPLACE;
    break;
  case STENCIL_OP_INCR:
    glFail_op = GL_INCR;
    break;
  case STENCIL_OP_DECR:
    glFail_op = GL_DECR;
    break;
  case STENCIL_OP_INVERT:
    glFail_op = GL_INVERT;
    break;
  case STENCIL_OP_INCR_WRAP:
    glFail_op = GL_INCR_WRAP;
    break;
  case STENCIL_OP_DECR_WRAP:
    glFail_op = GL_DECR_WRAP;
    break;
  }

  switch(fail)
  {
  case STENCIL_OP_KEEP:
    glFail = GL_KEEP;
    break;
  case STENCIL_OP_REPLACE:
    glFail = GL_REPLACE;
    break;
  case STENCIL_OP_INCR:
    glFail = GL_INCR;
    break;
  case STENCIL_OP_DECR:
    glFail = GL_DECR;
    break;
  case STENCIL_OP_INVERT:
    glFail = GL_INVERT;
    break;
  case STENCIL_OP_INCR_WRAP:
    glFail = GL_INCR_WRAP;
    break;
  case STENCIL_OP_DECR_WRAP:
    glFail = GL_DECR_WRAP;
    break;
  }

  switch(pass)
  {
  case STENCIL_OP_KEEP:
    glPass = GL_KEEP;
    break;
  case STENCIL_OP_REPLACE:
    glPass = GL_REPLACE;
    break;
  case STENCIL_OP_INCR:
    glPass = GL_INCR;
    break;
  case STENCIL_OP_DECR:
    glPass = GL_DECR;
    break;
  case STENCIL_OP_INVERT:
    glPass = GL_INVERT;
    break;
  case STENCIL_OP_INCR_WRAP:
    glPass = GL_INCR_WRAP;
    break;
  case STENCIL_OP_DECR_WRAP:
    glPass = GL_DECR_WRAP;
    break;
  }


  glStencilOp(glFail_op, glFail, glPass);
}

void CRenderSystemGLES::SetColorMask(bool r, bool g, bool b, bool a)
{
  glColorMask(r, g, b, a);
}

#endif
