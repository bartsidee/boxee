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

#ifndef GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
#endif

#ifndef GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 0x8C03
#endif

#ifdef USE_EGL_IMAGE
// Function pointers for the extension functions we need
PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = NULL;
PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR = NULL;
PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES = NULL;
#endif

using namespace std;

/************************************************************************/
/*    CGLTexture                                                       */
/************************************************************************/
CGLTexture::CGLTexture(unsigned int width, unsigned int height, unsigned int format)
: CBaseTexture(width, height, format)
{
#ifdef USE_EGL_IMAGE
  if (!eglCreateImageKHR)
  {
    eglCreateImageKHR =
        (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
    if (NULL == eglCreateImageKHR)
    {
        printf("eglGetProcAddress didn't find eglCreateImageKHR\n");
    }
  }

  if (!eglDestroyImageKHR)
  {
    eglDestroyImageKHR =
        (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
    if (NULL == eglDestroyImageKHR)
    {
        printf("eglGetProcAddress didn't find eglDestroyImageKHR\n");
    }
  }

  if (!glEGLImageTargetTexture2DOES)
  {
    glEGLImageTargetTexture2DOES =
        (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress(
                                 "glEGLImageTargetTexture2DOES");
    if (NULL == glEGLImageTargetTexture2DOES)
    {
        printf("eglGetProcAddress didn't find glEGLImageTargetTexture2DOES\n");
    }
  }
#endif
}

CGLTexture::~CGLTexture()
{
  DestroyTextureObject();
}

#if defined(HAS_GDL) && defined(USE_EGL_IMAGE)
extern "C"
{
  static gma_ret_t destroy_gdl_pixmap(gma_pixmap_info_t *pixmap_info);
}
#endif

void CGLTexture::CreateTextureObject()
{
  if (m_textureOwner)
  {
    glGenTextures(1, (GLuint*) &m_texture);
  }
}

void CGLTexture::DeletePixels()
{
  if (m_pixels)
  {
    delete [] m_pixels;
    m_pixels = NULL;
  }
}

#if defined(HAS_GDL) && defined(USE_EGL_IMAGE)
static gma_ret_t destroy_gdl_pixmap(gma_pixmap_info_t *pixmap_info)
{
  //CGLTexture* texture = (CGLTexture*) pixmap_info->user_data;
  //texture->DeletePixels();
  // I Assume it's deleted in the destructor
  return GMA_SUCCESS;
}
#endif

void CGLTexture::DestroyTextureObject()
{
  if (m_texture && m_textureOwner)
  {
    glDeleteTextures(1, (GLuint*) &m_texture);
    m_texture = 0;
#ifdef USE_EGL_IMAGE
    eglDestroyImageKHR(g_Windowing.GetEGLDisplay(), m_textureImage);
#ifdef HAS_GDL
    gma_pixmap_release(&m_pixmap);
#endif
#endif
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

  if (m_loadedToGPU)
  {
    return;
  }

#ifdef USE_EGL_IMAGE

#ifdef HAS_GDL
  static gma_pixmap_funcs_t gdl_pixmap_funcs;
  gdl_pixmap_funcs.destroy = destroy_gdl_pixmap;

  m_pixmap = NULL;

  gma_pixmap_info_t pixmap_info;
  pixmap_info.type = GMA_PIXMAP_TYPE_VIRTUAL;
  pixmap_info.virt_addr = m_pixels;
  pixmap_info.width = m_textureWidth;
  pixmap_info.height = m_textureHeight;
  if (m_format == XB_FMT_A8)
  {
    pixmap_info.format = GMA_PF_A8;
    pixmap_info.pitch = m_textureWidth;
  }
  else
  {
    pixmap_info.format = GMA_PF_ARGB_32;
    pixmap_info.pitch = m_textureWidth * 4;
  }
  pixmap_info.user_data = this;

  gma_ret_t gma_rc = gma_pixmap_alloc(&pixmap_info, &gdl_pixmap_funcs, &m_pixmap);
  if (gma_rc != GMA_SUCCESS)
  {
    CLog::Log(LOGERROR, "Pixmal allocation failed");
    m_pixmap = NULL;
    return;
  }
#endif

  // Bind the texture object
  glBindTexture(GL_TEXTURE_2D, m_texture);

  m_textureImage =
      eglCreateImageKHR(g_Windowing.GetEGLDisplay(),
                        EGL_NO_CONTEXT,
                        EGL_NATIVE_PIXMAP_KHR,
                        (EGLClientBuffer)m_pixmap,
                        NULL);
  if (EGL_NO_IMAGE_KHR == m_textureImage)
  {
    CLog::Log(LOGERROR, "Creation of image failed! EGL error=%X",
             eglGetError());
    return;
  }

  glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_textureImage);

#else

  // Bind the texture object
  glBindTexture(GL_TEXTURE_2D, m_texture);

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
  
#if defined(HAS_GL) || defined(HAS_GL2)
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
   case XB_FMT_PVR2:
     format = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
     break;
   case XB_FMT_PVR4:
     format = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
     break;
  case XB_FMT_A8:
    internalFormat = format = GL_ALPHA;
    break;
  case XB_FMT_R8G8B8A8:
#if defined(HAS_GL) || defined(HAS_GL2)
    format = GL_RGBA;
#elif defined(HAS_GLES)
    internalFormat = format = GL_RGBA;
#endif
    break;
  case XB_FMT_B8G8R8A8:
  default:
#if defined(HAS_GL) || defined(HAS_GL2)
    format = GL_BGRA;
#elif HAS_GLES && !defined(_WIN32)
#ifdef GL_BGRA
    internalFormat = format = GL_BGRA;
#else
    internalFormat = format = GL_BGRA_EXT;
#endif
#elif HAS_GLES && defined(_WIN32)
    internalFormat = format = GL_RGBA;
#endif
    break;
  }
  
  if ( ((m_format & XB_FMT_DXT_MASK) == 0)  && ((m_format & XB_FMT_PVR4) == 0) )
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
#endif

  m_loadedToGPU = true;   
}
#endif // HAS_GL
