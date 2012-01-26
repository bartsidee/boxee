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

#include "system.h"
#include "gui3d.h"

//
// CFrameBufferObjectGL
// A class that abstracts FBOs to facilitate Render To Texture
//
// Requires OpenGL 1.5+ or the GL_EXT_framebuffer_object extension.
//
// Usage:
//
//     CFrameBufferObjectGL *fbo = new CFrameBufferObjectGL();
//     fbo->Initialize();
//     fbo->CreateAndBindToTexture(GL_TEXTURE_2D, 256, 256, GL_RGBA);
//  OR fbo->BindToTexture(GL_TEXTURE_2D, <existing texture ID>);
//     fbo->BeginRender();
//     <normal GL rendering calls>
//     fbo->EndRender();
//     bind and use texture anywhere
//     glBindTexture(GL_TEXTURE_2D, fbo->Texture());
//

class CFrameBufferObjectBase
{
public:
  CFrameBufferObjectBase();
  virtual ~CFrameBufferObjectBase();

  // returns true if FBO support is detected
  virtual bool IsSupported() { return false; }

  // returns true if FBO has been initialized
  virtual bool IsValid() { return m_valid; }

  // returns true if FBO has a texture bound to it
  virtual bool IsBound() { return m_bound; }

  // initialize the FBO
  virtual bool Initialize() { return false; }

  // Cleanup
  virtual void Cleanup() {}

  virtual bool BeginRender() { return false; }

  virtual void EndRender() {}

  unsigned int GetWidth() { return m_width; }
  unsigned int GetHeight() { return m_height; }

  XBMC::TexturePtr Texture() { return m_texid; }

protected:
  unsigned int      m_width;
  unsigned int      m_height;
  unsigned int      m_fbo;
  bool              m_valid;
  bool              m_bound;
  bool              m_supported;
  XBMC::TexturePtr  m_texid;
  bool              m_isRendering;
};

#if defined(HAS_GL2) || HAS_GLES == 2

class CFrameBufferObjectGL : public CFrameBufferObjectBase
{
public:
  // Constructor
  CFrameBufferObjectGL();
  virtual ~CFrameBufferObjectGL();

  // returns true if FBO support is detected
  virtual bool IsSupported();

  // returns true if FBO has been initialized
  virtual bool IsValid() { return m_valid; }

  // returns true if FBO has a texture bound to it
  virtual bool IsBound() { return m_bound; }

  // initialize the FBO
  virtual bool Initialize();

  // Cleanup
  virtual void Cleanup();

  // Bind to an exiting texture
  bool BindToTexture(GLenum target, GLuint texid);

  // Set texture filtering
  void SetFiltering(GLenum target, GLenum mode);

  // Create a new texture and bind to it
  bool CreateAndBindToTexture(GLenum target, int width, int height, GLenum format = GL_RGBA,
                              GLenum filter=GL_LINEAR, GLenum clamp=GL_CLAMP_TO_EDGE);

  GLuint FBO() { return m_fbo; }

  // Begin rendering to FBO
  virtual bool BeginRender();

  // Finish rendering to FBO
  virtual void EndRender();

  bool IsRendering() { return m_isRendering; }
};

#define  CFrameBufferObject CFrameBufferObjectGL
#endif

#if defined(HAS_DX)

class CFrameBufferObjectDX : public CFrameBufferObjectBase
{
public:
  CFrameBufferObjectDX();
  virtual ~CFrameBufferObjectDX();
};

#define CFrameBufferObject CFrameBufferObjectDX
#endif
