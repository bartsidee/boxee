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
#include "TextureGL.h"
#include "WindowingFactory.h"
#include "utils/log.h"

#if defined(HAS_GL) || defined(HAS_GLES)

//
// Adding missing macros, to allow GLES to compile in Windows for OpenGL testing
//

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

using namespace std;

/************************************************************************/
/*    CGLTexture                                                       */
/************************************************************************/
CGLTexture::CGLTexture(unsigned int width, unsigned int height, unsigned int format)
: CBaseTexture(width, height, format)
{
}

CGLTexture::~CGLTexture()
{
  DestroyTextureObject();
}

void CGLTexture::CreateTextureObject()
{
  if (m_textureOwner)
  {
    glGenTextures(1, (GLuint*) &m_texture);
  }
}

void CGLTexture::DestroyTextureObject()
{
  if (m_texture && m_textureOwner)
  {
    glDeleteTextures(1, (GLuint*) &m_texture);
    m_texture = NULL;
  }
}

void CGLTexture::LoadToGPU()
{
  if (!m_textureOwner)
  {
    return;
  }

  if (!m_pixels)
  {
    // nothing to load - probably same image (no change)
    return;
  }

  if (m_texture == 0)
  {
    // Have OpenGL generate a texture object handle for us
    // this happens only one time - the first time the texture is loaded
    CreateTextureObject();
  }

  // Bind the texture object
  glBindTexture(GL_TEXTURE_2D, m_texture);

  // Set the texture's stretching properties
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  unsigned int maxSize = g_Windowing.GetMaxTextureSize();
  if (m_textureHeight > maxSize)
    {
    CLog::Log(LOGERROR, "GL: Image height %d too big to fit into single texture unit, truncating to %u", m_textureHeight, maxSize);
    m_textureHeight = maxSize;
    }
  if (m_textureWidth > maxSize)
    {
    CLog::Log(LOGERROR, "GL: Image width %d too big to fit into single texture unit, truncating to %u", m_textureWidth, maxSize);
#ifndef HAS_GLES
    glPixelStorei(GL_UNPACK_ROW_LENGTH, m_textureWidth);
#else
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
#endif
    m_textureWidth = maxSize;
    }

  GLenum format, internalFormat;
  
#ifdef HAS_GL
  internalFormat = 4; 
#elif defined(HAS_GLES)
  internalFormat = format = GL_RGBA;
#endif
  
  switch (m_format)
  {
  case XB_FMT_DXT1: 
    format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    break; 
  case XB_FMT_DXT3: 
    format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    break;     
  case XB_FMT_DXT5: 
  case XB_FMT_DXT5_YCoCg:
    format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    break; 
  case XB_FMT_R8G8B8A8:
#ifdef HAS_GL
    format = GL_RGBA;
#elif defined(HAS_GLES)
    internalFormat = format = GL_RGBA;
#endif
    break;
  case XB_FMT_B8G8R8A8:
  default:
#ifdef HAS_GL
    format = GL_BGRA;
#elif HAS_GLES && !defined(_WIN32)
    internalFormat = format = GL_BGRA_EXT;
#elif HAS_GLES && defined(_WIN32)
    internalFormat = format = GL_RGBA;
#endif
    break;
  }
  
  if ((m_format & XB_FMT_DXT_MASK) == 0)
  {
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_textureWidth, m_textureHeight, 0,
      format, GL_UNSIGNED_BYTE, m_pixels);
  }
  else
  {
#ifdef HAS_GLES
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, format, 
      m_textureWidth, m_textureHeight, 0, GetPitch() * GetRows(), m_pixels);
#else
    glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, format, 
      m_textureWidth, m_textureHeight, 0, GetPitch() * GetRows(), m_pixels);
#endif
  }
  
#ifndef HAS_GLES
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
  VerifyGLState();

  delete [] m_pixels;
  m_pixels = NULL;

  m_loadedToGPU = true;   
}
#endif // HAS_GL
