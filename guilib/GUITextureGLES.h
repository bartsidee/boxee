/*!
\file GUITextureGL.h
\brief 
*/

#ifndef GUILIB_GUITEXTUREGLES_H
#define GUILIB_GUITEXTUREGLES_H

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

#include "GUITexture.h"
#include "Shader.h"

class CGUITextureGLES : public CGUITextureBase
{
public:
  CGUITextureGLES(float posX, float posY, float width, float height, const CTextureInfo& texture, CBaseTexture* textureData = NULL);
  static void DrawQuad(const CRect &coords, DWORD color, bool bWireframe = false, CBaseTexture *texture = NULL, const CRect *texCoords = NULL);
  static void DrawLine(const CRect &coords, DWORD color);
  static void PrintPixelCount(){}

protected:
  void Begin();
  void Draw(ImageCoords &coords);
  void DrawWireFrame(ImageCoords &coords);
  void End();

  virtual bool LoadShaders();
  virtual bool SelectShader();
  virtual void SetShader(Shaders::CGLSLShaderProgram* shader);

  GLint m_uniMatModelView;
  GLint m_uniMatProjection;

  Shaders::CGLSLShaderProgram* m_pCurrentPixelShader;

  GLuint m_progObj;
  GLint m_uniTexture0;
  GLint m_uniTexture1;
  GLint m_attribVertex;
  GLint m_attribTextureCoord0;
  GLint m_attribTextureCoord1;
  GLint m_uniColor;
};

#endif
