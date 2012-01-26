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

/*!
\file TextureManager.h
\brief 
*/

#ifndef GUILIB_TEXTUREGL_H
#define GUILIB_TEXTUREGL_H

#include "Texture.h"

#pragma once

#if defined(HAS_GL2) || defined(HAS_GLES)

#ifdef USE_EGL_IMAGE
#include <EGL/egl.h>
#include <EGL/eglext.h>
#ifdef HAS_GDL
#include <libgdl.h>
#include <libgma.h>
#endif
#endif

/************************************************************************/
/*    CGLTexture                                                       */
/************************************************************************/
class CGLTexture : public CBaseTexture
{
public:
  CGLTexture(unsigned int width = 0, unsigned int height = 0, unsigned int format = XB_FMT_B8G8R8A8);
  virtual ~CGLTexture();
  
  void CreateTextureObject();
  virtual void DestroyTextureObject();
  void LoadToGPU();

  void DeletePixels();

private:
#ifdef USE_EGL_IMAGE
#ifdef HAS_GDL
  gma_pixmap_t m_pixmap;
#endif
  EGLImageKHR m_textureImage;
#endif
};

#endif

#endif
