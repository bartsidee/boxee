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

#if defined(HAS_GL) || defined(HAS_GL2)

#include "GraphicContext.h"
#include "AdvancedSettings.h"
#include "RenderSystemGL.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "TextureGL.h"
#include "GUITextureGL.h"
#include "utils/SystemInfo.h"
#include "MathUtils.h"

CRenderSystemGL::CRenderSystemGL() : CRenderSystemBase()
{
  m_enumRenderingSystem = RENDERING_SYSTEM_OPENGL;
}

CRenderSystemGL::~CRenderSystemGL()
{
  DestroyRenderSystem();
}

void CRenderSystemGL::CheckOpenGLQuirks()
// This may not be correct on all hardware, Apple Tv(Nvidia 7300) having problems with this

{
#ifdef __APPLE__	
  if (m_RenderVendor.find("NVIDIA") != CStdString::npos) // nVidia drivers.
  {             
    const char *arr[]= { "7300", "7600", "9400M", NULL };
    int j;	
    for(j=0; arr[j]; j++)
      {
      if(m_RenderRenderer.find(arr[j]) != CStdString::npos && (m_renderCaps & RENDER_CAPS_DXT_NPOT))
      {
          m_renderCaps &= ~ RENDER_CAPS_DXT_NPOT;
          break;
      }		  
  }
  }
  else if (m_RenderVendor.find("ATI") != CStdString::npos) // ATI drivers.
  {             
    const char *arr[]= { "X1600", NULL };
    int j;	
    for(j=0; arr[j]; j++)
    {
      if(m_RenderRenderer.find(arr[j]) != CStdString::npos && (m_renderCaps & RENDER_CAPS_DXT_NPOT) )
      {
        m_renderCaps &= ~ RENDER_CAPS_DXT_NPOT;
        break;
      }		  
    }
  }
#endif
}	

bool CRenderSystemGL::InitRenderSystem()
{
  m_bVSync = false;
  m_iVSyncMode = 0;
  m_iSwapStamp = 0;
  m_iSwapTime = 0;
  m_iSwapRate = 0;
  m_bVsyncInit = false;
  m_maxTextureSize = 2048;
  m_renderCaps = 0;
 
#ifdef HAS_GLEW 
  // init glew library
  GLenum err = glewInit();
  if (GLEW_OK != err)
  {
    // Problem: glewInit failed, something is seriously wrong
    return false;
  }
#endif
 
  // Get the GL version number 
  m_RenderVersionMajor = 0;
  m_RenderVersionMinor = 0;

  const char* ver = (const char*)glGetString(GL_VERSION);
  if (ver != 0)
    sscanf(ver, "%d.%d", &m_RenderVersionMajor, &m_RenderVersionMinor);
  
  // Get our driver vendor and renderer
  const char *vendor = (const char*) glGetString(GL_VENDOR);
  const char *renderer = (const char*) glGetString(GL_RENDERER);
  
  if (vendor)
    m_RenderVendor = vendor;
  if (renderer)
    m_RenderRenderer = renderer;
 
#ifdef HAS_GLEW 
  // grab our capabilities
  if (glewIsSupported("GL_EXT_texture_compression_s3tc"))
    m_renderCaps |= RENDER_CAPS_DXT;

  if (glewIsSupported("GL_ARB_texture_non_power_of_two"))
  {
    m_renderCaps |= RENDER_CAPS_NPOT;
    if (m_renderCaps & RENDER_CAPS_DXT  && !g_sysinfo.IsAppleTV())    // This may not be correct on all hardware, Apple Tv(Nvidia 7300) having problems with this 
      m_renderCaps |= RENDER_CAPS_DXT_NPOT;
  }
#endif

  //Check OpenGL quirks and revert m_renderCaps as needed
  CheckOpenGLQuirks();

  const char *ext = (const char*) glGetString(GL_EXTENSIONS);
  m_RenderExtensions  = " ";
  if (ext)
    m_RenderExtensions += ext;
  m_RenderExtensions += " ";

  LogGraphicsInfo();

  m_bRenderCreated = true;
  
  return true;
}

bool CRenderSystemGL::ResetRenderSystem(int width, int height, bool fullScreen, float refreshRate)
{
  CRenderSystemBase::ResetRenderSystem(width, height, fullScreen, refreshRate);

  glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

  CalculateMaxTexturesize();

  glViewport(0, 0, width, height);
  glScissor(0, 0, width, height);

  glEnable(GL_TEXTURE_2D); 
  glDisable(GL_SCISSOR_TEST);

  g_graphicsContext.EndPaint();

  glDisable(GL_DEPTH_TEST);  

  return true;
}

bool CRenderSystemGL::DestroyRenderSystem()
{
  m_bRenderCreated = false;

  return true;
}

bool CRenderSystemGL::BeginRender()
{
  if (!m_bRenderCreated)
    return false;

  return true;
}

bool CRenderSystemGL::EndRender()
{
  if (!m_bRenderCreated)
    return false;

  return true;
}

bool CRenderSystemGL::ClearBuffers(color_t color)
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

bool CRenderSystemGL::ClearBuffers(float r, float g, float b, float a)
{
  if (!m_bRenderCreated)
    return false;
  
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT);

  return true;
}

void CRenderSystemGL::ClearStencilBuffer(int val)
{
  glClearStencil(val);
  glClear(GL_STENCIL_BUFFER_BIT);
}

bool CRenderSystemGL::IsExtSupported(const char* extension)
{
  CStdString name;
  name  = " ";
  name += extension;
  name += " ";

  return m_RenderExtensions.find(name) != std::string::npos;;
}

bool CRenderSystemGL::PresentRender()
{
  if (!m_bRenderCreated)
    return false;

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
  
  bool result = PresentRenderImpl();
  
  if (m_iVSyncMode && m_iSwapRate != 0)
  {
    int64_t curr, diff;
    curr = CurrentHostCounter();

    diff = curr - m_iSwapStamp;
    m_iSwapStamp = curr;

    if (MathUtils::abs(diff - m_iSwapRate) < abs(diff))
      CLog::Log(LOGDEBUG, "%s - missed requested swap",__FUNCTION__);
  }
  
  return result;
}

void CRenderSystemGL::SetVSync(bool enable)
{
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
  
  if (!enable)
    return;

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
      m_iSwapRate   = (int64_t)((double)freq / rate);
      m_iSwapTime   = (int64_t)(0.001 * g_advancedSettings.m_ForcedSwapTime * freq);
      m_iSwapStamp  = 0;
      CLog::Log(LOGDEBUG, "GL: Using artificial vsync sleep with rate %f", rate);
      if(!m_iVSyncMode)
        m_iVSyncMode = 1;
    }
  }
    
  if (!m_iVSyncMode)
    CLog::Log(LOGERROR, "GL: Vertical Blank Syncing unsupported");
  else
    CLog::Log(LOGDEBUG, "GL: Selected vsync mode %d", m_iVSyncMode);
}

void CRenderSystemGL::CaptureStateBlock()
{
  return;
}

void CRenderSystemGL::ApplyStateBlock()
{
  return; 
}

bool CRenderSystemGL::TestRender()
{
  static float theta = 0.0;

  //RESOLUTION_INFO resInfo = g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution];
  //glViewport(0, 0, resInfo.iWidth, resInfo.iHeight);

  glPushMatrix();
  glRotatef( theta, 0.0f, 0.0f, 1.0f );
  glBegin( GL_TRIANGLES );
  glColor3f( 1.0f, 0.0f, 0.0f ); glVertex2f( 0.0f, 1.0f );
  glColor3f( 0.0f, 1.0f, 0.0f ); glVertex2f( 0.87f, -0.5f );
  glColor3f( 0.0f, 0.0f, 1.0f ); glVertex2f( -0.87f, -0.5f );
  glEnd();
  glPopMatrix();

  theta += 1.0f;

  return true;
}

void CRenderSystemGL::ApplyClippingRect(CRect& clipRect)
{
  glScissor((GLint) clipRect.x1, (GLint) (m_height - clipRect.y1 - clipRect.Height()), (GLsizei) clipRect.Width(), (GLsizei) clipRect.Height());
  glEnable(GL_SCISSOR_TEST);
}

void CRenderSystemGL::GetClipRect(CRect& clipRect)
{
  GLint box[4];
  glGetIntegerv(GL_SCISSOR_BOX, box);

  clipRect.x1 = (float)box[0];
  clipRect.y1 = (float)box[1];
  clipRect.x2 = (float)(box[0] + box[2]);
  clipRect.y2 = (float)(box[1] + box[3]);
}

void CRenderSystemGL::CalculateMaxTexturesize()
{
  GLint width = 256;
  glGetError(); // reset any previous GL errors

  // max out at 2^(8+8)
  for (int i = 0 ; i<8 ; i++) 
  {
    glTexImage2D(GL_PROXY_TEXTURE_2D, 0, 4, width, width, 0, GL_BGRA,
                 GL_UNSIGNED_BYTE, NULL);
    glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,
                             &width);

    // GMA950 on OS X sets error instead
    if (width == 0 || (glGetError() != GL_NO_ERROR) )
      break;
    
    m_maxTextureSize = width;
    width *= 2;
    if (width > 65536) // have an upper limit in case driver acts stupid
    {
      CLog::Log(LOGERROR, "GL: Could not determine maximum texture width, falling back to 2048");
      m_maxTextureSize = 2048;
      break;
    }
  }

  CLog::Log(LOGDEBUG, "GL: Maximum texture width: %u", m_maxTextureSize);
}

void CRenderSystemGL::GetViewPort(CRect& viewPort)
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

void CRenderSystemGL::SetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;
  
  glViewport((GLint) viewPort.x1, (GLint) (m_height - viewPort.y1 - viewPort.Height()), (GLsizei) viewPort.Width(), (GLsizei) viewPort.Height());
}

void CRenderSystemGL::EnableTexture(bool bEnable)
{
  if(bEnable)
    glEnable(GL_TEXTURE_2D);
  else
    glDisable(GL_TEXTURE_2D);
}

void CRenderSystemGL::EnableBlending(bool bEnableRGB, bool bEnableAlpha)
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


void CRenderSystemGL::EnableStencil(bool bEnable)
{
  if(bEnable)
    glEnable(GL_STENCIL_TEST);
  else
    glDisable(GL_STENCIL_TEST);
}

void CRenderSystemGL::SetColorMask(bool r, bool g, bool b, bool a)
{
  glColorMask(r, g, b, a);
}

void CRenderSystemGL::SetStencilFunc(StencilFunc func, int ref, unsigned int mask)
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

void CRenderSystemGL::SetStencilOp (StencilOp fail_op, StencilOp fail, StencilOp pass)
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

void CRenderSystemGL::EnableClipping(bool bEnable)
{
  if(bEnable)
    glEnable(GL_SCISSOR_TEST);
  else
    glDisable(GL_SCISSOR_TEST);
}

void CRenderSystemGL::EnableDepthTest(bool bEnable)
{
  if(bEnable)
    glEnable(GL_DEPTH_TEST);
  else
    glDisable(GL_DEPTH_TEST);
}

#endif
