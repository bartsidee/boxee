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

#include "../xbmc/Settings.h"
#include "WindowingFactory.h"
#include "FrameBufferObject.h"
#include "utils/log.h"

//////////////////////////////////////////////////////////////////////
// CFrameBufferObject
//////////////////////////////////////////////////////////////////////

CFrameBufferObjectBase::CFrameBufferObjectBase()
{
  m_fbo = 0;
  m_valid = false;
  m_supported = false;
  m_bound = false;
  m_texid = 0;
  m_width = 0;
  m_height = 0;
  m_isRendering = false;
}

CFrameBufferObjectBase::~CFrameBufferObjectBase()
{

}

#if defined(HAS_GL) || HAS_GLES == 2

CFrameBufferObjectGL::CFrameBufferObjectGL()
{
  CFrameBufferObjectBase();
}

CFrameBufferObjectGL::~CFrameBufferObjectGL()
{
  Cleanup();
}

bool CFrameBufferObjectGL::IsSupported()
{
#if defined(HAS_GL) || defined(HAS_GL2)
  if(g_Windowing.IsExtSupported("GL_EXT_framebuffer_object"))
    m_supported = true;
  else
    m_supported = false;
  return m_supported;
#elif defined(HAS_GLES)
  return true;
#else
  return false;
#endif
}

bool CFrameBufferObjectGL::Initialize()
{
  if (!IsSupported())
    return false;

  Cleanup();

#if defined(HAS_GL) || defined(HAS_GL2)
  glGenFramebuffersEXT(1, &m_fbo);
#else
  glGenFramebuffers(1, &m_fbo);
#endif
  VerifyGLState();

  if (!m_fbo)
    return false;

  m_valid = true;
  return true;
}

void CFrameBufferObjectGL::Cleanup()
{
  if (!IsValid())
    return;

  if (m_fbo)
  {
#ifdef HAS_GLEW
    glDeleteFramebuffersEXT(1, &m_fbo);
#else
    glDeleteFramebuffers(1, &m_fbo);
#endif
  }

  if (m_texid)
  {
    glDeleteTextures(1, &m_texid);
  }

  m_texid = 0;
  m_fbo = 0;
  m_valid = false;
  m_bound = false;
  m_isRendering = false;
}

bool CFrameBufferObjectGL::CreateAndBindToTexture(GLenum target, int width, int height, GLenum format,
                                                GLenum filter, GLenum clampmode)
{
  unsigned char* buf = NULL;
  GLenum errCode;

#ifdef HAS_GLES
  if(width > 2048 || height > 2048)
  {
    CLog::Log(LOGERROR,"CFrameBufferObjectGL::CreateAndBindToTexture size is too big width = %d height = %d",
        width, height);
    return false;
  }
#endif

  if (!IsValid())
    return false;

  if (m_texid)
    glDeleteTextures(1, &m_texid);

  m_width = width;
  m_height = height;

  // color
  glGenTextures(1, &m_texid);

  if(m_texid == 0)
  {
    printf("CreateAndBindToTexture glGenTextures failed\n");
  }

  glBindTexture(target, m_texid);
  glGetError();
  glTexImage2D(target, 0, format,  width, height, 0, format, GL_UNSIGNED_BYTE, NULL);

  if ((errCode = glGetError()) != GL_NO_ERROR)
  {
    CLog::Log(LOGERROR, "CFrameBufferObjectGL::CreateAndBindToTexture glTexImage2D failed %x", errCode);
    glBindTexture(target, 0);
    Cleanup();
    return false;
  }


  glTexParameteri(target, GL_TEXTURE_WRAP_S, clampmode);
  glTexParameteri(target, GL_TEXTURE_WRAP_T, clampmode);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);

#ifdef _DEBUG
  buf = (unsigned char *)malloc(width * height * 4);
  memset(buf, 128, width * height * 4);
  glTexSubImage2D(target, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buf);
  free(buf);
#endif
  glBindTexture(target, 0);

  if(!BindToTexture(target, m_texid))
    return false;
  else
    return true;
}

void CFrameBufferObjectGL::SetFiltering(GLenum target, GLenum mode)
{
  glBindTexture(target, m_texid);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, mode);
  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, mode);
}

bool CFrameBufferObjectGL::BindToTexture(GLenum target, GLuint texid)
{
  if (!IsValid())
    return false;

  m_bound = false;
#ifdef HAS_GLEW
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo);
  glBindTexture(target, texid);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, target, texid, 0);
  GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
#else
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  glBindTexture(target, texid);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, texid, 0);
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
  glBindTexture (target, 0);
  if (status != GL_FRAMEBUFFER_COMPLETE)
  {
    CLog::Log(LOGERROR,"**** FAILED to create fbo. status: %x", status);
    return false;
  }
  m_bound = true;
  return true;
}

bool CFrameBufferObjectGL::BeginRender()
{
  if (IsValid() && IsBound())
  {
#if defined(HAS_GL) || defined(HAS_GL2)
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo);
#else
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
#endif
    m_isRendering = true;
    return true;
  }
  return false;
}

void CFrameBufferObjectGL::EndRender()
{
  if (IsValid())
#if defined(HAS_GL) || defined(HAS_GL2)
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
#else
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
  m_isRendering = false;
}

#endif

#ifdef HAS_DX
CFrameBufferObjectDX::CFrameBufferObjectDX()
{


}
CFrameBufferObjectDX::~CFrameBufferObjectDX()
{

}
#endif

