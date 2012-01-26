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
#include <assert.h>
#include "system.h"
#include "GUITextureGL.h"
#include "Texture.h"
#include "GUIWindowManager.h"
#include "utils/log.h"


#if defined(HAS_GL)


static std::string g_TextureVertexShaderMulti =
"uniform mat4 u_matModelView;\n"
"uniform mat4 u_matProjection;\n"
"// Input vertex parameters\n"
"attribute vec3 a_vertex;\n"
"attribute vec2 a_texCoord0;\n"
"attribute vec2 a_texCoord1;\n"
"varying vec2 v_texCoord0;\n"
"varying vec2 v_texCoord1;\n"
"void main() \n"
"{\n"
"v_texCoord0  = a_texCoord0;\n"
"v_texCoord1  = a_texCoord1;\n"
"gl_Position = u_matProjection * u_matModelView * vec4(a_vertex,1.0);\n"
"}\n";

static std::string g_TextureVertexShader =
"uniform mat4 u_matModelView;\n"
"uniform mat4 u_matProjection;\n"
"// Input vertex parameters\n"
"attribute vec3 a_vertex;\n"
"attribute vec2 a_texCoord0;\n"
"varying vec2 v_texCoord0;\n"
"void main() \n"
"{\n"
"v_texCoord0  = a_texCoord0;\n"
"gl_Position = u_matProjection * u_matModelView * vec4(a_vertex,1.0);\n"
"}\n";

static std::string g_TexturePixelShader = 
#ifndef HAS_GL2
"precision mediump float;\n"
#endif
"varying vec2 v_texCoord0;\n"
"uniform vec4 u_diffuseColor;\n"
"uniform sampler2D u_texture0;\n"
"void main()\n"
"{\n"
"vec4 texture0Color;\n"
"texture0Color = texture2D(u_texture0, v_texCoord0) * u_diffuseColor;\n"
#if defined(_WIN32)
"gl_FragColor.rgba = texture0Color.bgra;\n"
#else
"gl_FragColor = texture0Color;\n"
#endif
"}\n";

static std::string g_TexturePixelShaderNoBlend =
#ifndef HAS_GL2
"precision mediump float;\n"
#endif
"varying vec2 v_texCoord0;\n"
"uniform vec4 u_diffuseColor;\n"
"uniform sampler2D u_texture0;\n"
"void main()\n"
"{\n"
"vec4 texture0Color;\n"
"texture0Color = texture2D(u_texture0, v_texCoord0) * u_diffuseColor;\n"
#if defined(_WIN32)
"gl_FragColor.rgba = texture0Color.bgra;\n"
#else
"gl_FragColor = texture0Color;\n"
#endif
"gl_FragColor.a = 1.0;\n"
"}\n";

static std::string g_TexturePixelShaderNoDiffuse =
#ifndef HAS_GL2
"precision mediump float;\n"
#endif
"varying vec2 v_texCoord0;\n"
"uniform sampler2D u_texture0;\n"
"void main()\n"
"{\n"
"vec4 texture0Color;\n"
"texture0Color = texture2D(u_texture0, v_texCoord0);\n"
#if defined(_WIN32)
"gl_FragColor.rgba = texture0Color.bgra;\n"
#else
"gl_FragColor = texture0Color;\n"
#endif
"}\n";

static std::string g_TexturePixelShaderNoDiffuseNoBlend =
#ifndef HAS_GL2
    "precision mediump float;\n"
#endif
    "varying vec2 v_texCoord0;\n"
    "uniform sampler2D u_texture0;\n"
    "void main()\n"
    "{\n"
    "vec4 texture0Color;\n"
    "texture0Color = texture2D(u_texture0, v_texCoord0);\n"
#if defined(_WIN32)
    "gl_FragColor.rgba = texture0Color.bgra;\n"
#else
    "gl_FragColor.rgb = texture0Color.rgb;\n"
#endif
    "gl_FragColor.a = 1.0;\n"
    "}\n";

static std::string g_TexturePixelShaderMulti =
#ifndef HAS_GL2
    "precision mediump float;\n"
#endif
    "varying vec2 v_texCoord0;\n"
    "varying vec2 v_texCoord1;\n"
    "uniform vec4 u_diffuseColor;\n"
    "uniform sampler2D u_texture0;\n"
    "uniform sampler2D u_texture1;\n"
    "void main()\n"
    "{\n"
    "vec4 texture0Color;\n"
    "vec4 texture1Color;\n"
    "texture0Color = texture2D(u_texture0, v_texCoord0) * u_diffuseColor;\n"
    "texture1Color = texture2D(u_texture1, v_texCoord1) * texture0Color;\n"
#if defined(_WIN32)
    "gl_FragColor.rgba = texture1Color.bgra;\n"
#else
    "gl_FragColor = texture1Color;\n"
#endif
    "}\n";

static std::string g_TexturePixelShaderMultiNoDiffuse =
#ifndef HAS_GL2
    "precision mediump float;\n"
#endif
    "varying vec2 v_texCoord0;\n"
    "varying vec2 v_texCoord1;\n"
    "uniform sampler2D u_texture0;\n"
    "uniform sampler2D u_texture1;\n"
    "void main()\n"
    "{\n"
    "vec4 texture0Color;\n"
    "vec4 texture1Color;\n"
    "texture0Color = texture2D(u_texture0, v_texCoord0);\n"
    "texture1Color = texture2D(u_texture1, v_texCoord1) * texture0Color;\n"
#if defined(_WIN32)
    "gl_FragColor.rgba = texture1Color.bgra;\n"
#else
    "gl_FragColor = texture1Color;\n"
#endif
    "}\n";

static std::string g_ColorPixelShader =
#ifndef HAS_GL2
"precision mediump float;\n"
#endif
"uniform vec4 u_diffuseColor;\n"
"void main()\n"
"{\n"
#if defined(_WIN32)
"gl_FragColor.rgba = u_diffuseColor.bgra;\n"
#else
"gl_FragColor = u_diffuseColor;\n"
#endif
"}\n";

static std::string g_PixelShaderWireFrame =
#ifndef HAS_GL2
    "precision mediump float;\n"
#endif
    "uniform vec4 u_diffuseColor;\n"
    "void main()\n"
    "{\n"
    "gl_FragColor = u_diffuseColor;\n"
    "}\n";

static std::string g_PixelShaderEmpty =
    "void main()\n"
    "{\n"
    "}\n";

static Shaders::CGLSLShaderProgram* g_pTextureShadersProgram = NULL;
static Shaders::CGLSLShaderProgram* g_pTextureShadersProgramNoBlend = NULL;
static Shaders::CGLSLShaderProgram* g_pTextureShadersProgramNoDiffuse = NULL;
static Shaders::CGLSLShaderProgram* g_pTextureShadersProgramNoDiffuseNoBlend = NULL;

static Shaders::CGLSLShaderProgram* g_pTextureShadersProgramMulti = NULL;
static Shaders::CGLSLShaderProgram* g_pTextureShadersProgramMultiNoDiffuse = NULL;

static Shaders::CGLSLShaderProgram* g_pWireFrameShader = NULL;
static Shaders::CGLSLShaderProgram* g_pEmptyShader = NULL;

CGUITextureGL::CGUITextureGL(float posX, float posY, float width, float height, const CTextureInfo &texture, CBaseTexture* textureData)
: CGUITextureBase(posX, posY, width, height, texture, textureData)
{
  m_pCurrentPixelShader = NULL;
  m_progObj = (GLuint) -1;
}

bool CGUITextureGL::LoadShaders()
{
  if(g_pTextureShadersProgram == NULL)
  {
    g_pTextureShadersProgram = new Shaders::CGLSLShaderProgram();
    g_pTextureShadersProgram->VertexShader()->SetSource(g_TextureVertexShader);
    g_pTextureShadersProgram->PixelShader()->SetSource(g_TexturePixelShader);
    g_pTextureShadersProgram->CompileAndLink();
  }

  if(g_pTextureShadersProgramNoDiffuse == NULL)
  {
    g_pTextureShadersProgramNoDiffuse = new Shaders::CGLSLShaderProgram();
    g_pTextureShadersProgramNoDiffuse->VertexShader()->SetSource(g_TextureVertexShader);
    g_pTextureShadersProgramNoDiffuse->PixelShader()->SetSource(g_TexturePixelShaderNoDiffuse);
    g_pTextureShadersProgramNoDiffuse->CompileAndLink();
  }

  if(g_pTextureShadersProgramNoBlend == NULL)
  {
    g_pTextureShadersProgramNoBlend = new Shaders::CGLSLShaderProgram();
    g_pTextureShadersProgramNoBlend->VertexShader()->SetSource(g_TextureVertexShader);
    g_pTextureShadersProgramNoBlend->PixelShader()->SetSource(g_TexturePixelShaderNoBlend);
    g_pTextureShadersProgramNoBlend->CompileAndLink();
  }

  if(g_pTextureShadersProgramNoDiffuseNoBlend == NULL)
  {
    g_pTextureShadersProgramNoDiffuseNoBlend = new Shaders::CGLSLShaderProgram();
    g_pTextureShadersProgramNoDiffuseNoBlend->VertexShader()->SetSource(g_TextureVertexShader);
    g_pTextureShadersProgramNoDiffuseNoBlend->PixelShader()->SetSource(g_TexturePixelShaderNoDiffuseNoBlend);
    g_pTextureShadersProgramNoDiffuseNoBlend->CompileAndLink();
  }

  if(g_pTextureShadersProgramMulti == NULL)
  {
    g_pTextureShadersProgramMulti = new Shaders::CGLSLShaderProgram();
    g_pTextureShadersProgramMulti->VertexShader()->SetSource(g_TextureVertexShaderMulti);
    g_pTextureShadersProgramMulti->PixelShader()->SetSource(g_TexturePixelShaderMulti);
    g_pTextureShadersProgramMulti->CompileAndLink();
  }

  if(g_pTextureShadersProgramMultiNoDiffuse == NULL)
  {
    g_pTextureShadersProgramMultiNoDiffuse = new Shaders::CGLSLShaderProgram();
    g_pTextureShadersProgramMultiNoDiffuse->VertexShader()->SetSource(g_TextureVertexShaderMulti);
    g_pTextureShadersProgramMultiNoDiffuse->PixelShader()->SetSource(g_TexturePixelShaderMultiNoDiffuse);
    g_pTextureShadersProgramMultiNoDiffuse->CompileAndLink();
  }

  if(g_pWireFrameShader == NULL)
  {
    g_pWireFrameShader = new Shaders::CGLSLShaderProgram();
    g_pWireFrameShader->VertexShader()->SetSource(g_TextureVertexShader);
    g_pWireFrameShader->PixelShader()->SetSource(g_PixelShaderWireFrame);
    g_pWireFrameShader->CompileAndLink();
  }

  if(g_pEmptyShader == NULL)
  {
    g_pEmptyShader = new Shaders::CGLSLShaderProgram();
    g_pEmptyShader->VertexShader()->SetSource(g_TextureVertexShader);
    g_pEmptyShader->PixelShader()->SetSource(g_PixelShaderEmpty);
    g_pEmptyShader->CompileAndLink();
  }

  if(g_pTextureShadersProgram == NULL 
      || g_pTextureShadersProgramNoDiffuse == NULL
      || g_pTextureShadersProgramNoBlend == NULL
      || g_pTextureShadersProgramNoDiffuseNoBlend == NULL
      || g_pTextureShadersProgramMulti == NULL
      || g_pTextureShadersProgramMultiNoDiffuse == NULL
      || g_pWireFrameShader == NULL
      || g_pEmptyShader == NULL)
    return false;

  return true;
}

bool CGUITextureGL::SelectShader()
{
  m_progObj = 1;
  m_uniTexture0 = -1;
  m_uniTexture1 = -1;
  m_attribVertex = -1;
  m_attribTextureCoord0 = -1;
  m_attribTextureCoord1 = -1;
  m_uniColor = -1;

  if(m_diffuse.size() && m_diffuseColorBlended == 0xffffffff)
  {
    SetShader(g_pTextureShadersProgramMultiNoDiffuse);
    return true;
  }

  if(m_diffuse.size())
  {
    SetShader(g_pTextureShadersProgramMulti);
    return true;
  }

  if(m_bNeedBlending && m_diffuseColorBlended != 0xffffffff)
  {
    SetShader(g_pTextureShadersProgram);
    return true;
  }

  if(!m_bNeedBlending && m_diffuseColorBlended != 0xffffffff)
  {
    SetShader(g_pTextureShadersProgramNoBlend);
    return true;
  }

  if(m_bNeedBlending && m_diffuseColorBlended == 0xffffffff)
  {
    SetShader(g_pTextureShadersProgramNoDiffuse);
    return true;
  }

  if(!m_bNeedBlending && m_diffuseColorBlended == 0xffffffff)
  {
    SetShader(g_pTextureShadersProgramNoDiffuseNoBlend);
    return true;
  }

  return true;
}

void CGUITextureGL::SetShader(Shaders::CGLSLShaderProgram* shader)
{
  shader->Enable();
  m_pCurrentPixelShader = shader;
  m_progObj = shader->ProgramHandle();
}

void CGUITextureGL::Begin()
{
  CBaseTexture* texture = NULL;

  if(m_texture.size())
  {
    texture = m_texture.m_textures[m_currentFrame];
    glActiveTexture(GL_TEXTURE0);
    if(texture)
      texture->LoadToGPU();
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  if(m_progObj == (GLuint) -1)
    return;

  m_uniMatProjection  = glGetUniformLocation(m_progObj, "u_matProjection");
  m_uniMatModelView  = glGetUniformLocation(m_progObj, "u_matModelView");
  m_uniTexture0 = glGetUniformLocation(m_progObj, "u_texture0");
  m_attribTextureCoord0 = glGetAttribLocation(m_progObj, "a_texCoord0");
  m_attribVertex = glGetAttribLocation(m_progObj, "a_vertex");
  m_uniColor = glGetUniformLocation(m_progObj, "u_diffuseColor");
  m_uniTexture1 = glGetUniformLocation(m_progObj, "u_texture1");
  m_attribTextureCoord1 = glGetAttribLocation(m_progObj, "a_texCoord1");

  glEnableVertexAttribArray(m_attribVertex); 

  if(m_attribTextureCoord0 != -1)
    glEnableVertexAttribArray(m_attribTextureCoord0);
  if(texture)
    glBindTexture(GL_TEXTURE_2D, texture->GetTextureObject());


  TransformMatrix* matModelView = g_Windowing.GetHardwareTransform(MATRIX_TYPE_MODEL_VIEW);
  TransformMatrix* matProjection = g_Windowing.GetHardwareTransform(MATRIX_TYPE_PROJECTION);

  glUniformMatrix4fv(m_uniMatModelView, 1, GL_FALSE, (GLfloat *)matModelView->m);
  glUniformMatrix4fv(m_uniMatProjection, 1, GL_FALSE, (GLfloat *)matProjection->m);

  if(m_uniTexture0 != -1)
    glUniform1i(m_uniTexture0, 0);

  if (m_uniColor != -1)
  {
    GLfloat diffuseColor[4];
    diffuseColor[0] = (GLfloat)GET_R(m_diffuseColorBlended) / 255.0f;
    diffuseColor[1] = (GLfloat)GET_G(m_diffuseColorBlended) / 255.0f;
    diffuseColor[2] = (GLfloat)GET_B(m_diffuseColorBlended) / 255.0f;
    diffuseColor[3] = ((GLfloat)GET_A(m_diffuseColorBlended) / 255.0f);

    glUniform4fv(m_uniColor, 1, diffuseColor);
  }

  if (m_diffuse.size())
  {
    glActiveTexture(GL_TEXTURE1);
    if(m_uniTexture1 != -1)
      glUniform1i(m_uniTexture1, 1);
    texture = m_diffuse.m_textures[0];
    if(texture)
    { 
      texture->LoadToGPU();
      glBindTexture(GL_TEXTURE_2D, texture->GetTextureObject());
    }
    if(m_attribTextureCoord1 != -1)
      glEnableVertexAttribArray(m_attribTextureCoord1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
}

void CGUITextureGL::End()
{
  if (m_diffuse.size())
  {
    glActiveTexture(GL_TEXTURE0);
    if(m_attribTextureCoord1 != -1)
      glDisableVertexAttribArray(m_attribTextureCoord1);
  }
  if(m_attribVertex != -1)
    glDisableVertexAttribArray(m_attribVertex);
  if(m_attribTextureCoord0 != -1)
    glDisableVertexAttribArray(m_attribTextureCoord0);
  if(m_pCurrentPixelShader)
    m_pCurrentPixelShader->Disable();
  m_pCurrentPixelShader = NULL;
}

void CGUITextureGL::Draw(ImageCoords &coords)
{
  ImageVertex* v = coords.m_pCoords;

  GLuint indices[6] = 
  {
      0, 1, 2,
      2, 3, 0,
  };

  if(m_attribVertex != -1)
    glVertexAttribPointer(m_attribVertex, 3, GL_FLOAT, GL_FALSE, sizeof(ImageVertex), (char*)v + offsetof(ImageVertex, x));
  if(m_attribTextureCoord0 != -1)
    glVertexAttribPointer(m_attribTextureCoord0, 2, GL_FLOAT, GL_FALSE, sizeof(ImageVertex), (char*)v + offsetof(ImageVertex, u1));
  if (m_diffuse.size())
  {
    if(m_attribTextureCoord1 != -1)
      glVertexAttribPointer(m_attribTextureCoord1, 2, GL_FLOAT, GL_FALSE, sizeof(ImageVertex), (char*)v + offsetof(ImageVertex, u2));
  }

#ifdef _DEBUG
  if(g_advancedSettings.m_bWireFrameMode)
  {
    DrawWireFrame(coords);
  }
  else
#endif
    glDrawElements( GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_INT, indices);
}

void CGUITextureGL::DrawWireFrame(ImageCoords &coords)
{
  GLuint progObj;

  ImageVertex* v = coords.m_pCoords;

  GLuint indices[6] = 
  {
      0, 1, 2,
      2, 3, 0,
  };

  g_pWireFrameShader->Enable();
  progObj = g_pWireFrameShader->ProgramHandle();
  m_uniColor = glGetUniformLocation(progObj, "u_diffuseColor");

  TransformMatrix* matModelView = g_Windowing.GetHardwareTransform(MATRIX_TYPE_MODEL_VIEW);
  TransformMatrix* matProjection = g_Windowing.GetHardwareTransform(MATRIX_TYPE_PROJECTION);

  m_uniMatProjection  = glGetUniformLocation(progObj, "u_matProjection");
  m_uniMatModelView  = glGetUniformLocation(progObj, "u_matModelView");
  m_attribVertex = glGetAttribLocation(progObj, "a_vertex");

  glEnableVertexAttribArray(m_attribVertex); 

  glUniformMatrix4fv(m_uniMatModelView, 1, GL_FALSE, (GLfloat *)matModelView->m);
  glUniformMatrix4fv(m_uniMatProjection, 1, GL_FALSE, (GLfloat *)matProjection->m);

  GLfloat diffuseColor[4];
  diffuseColor[0] = (GLfloat)GET_R(m_diffuseColor) / 255.0f;
  diffuseColor[1] = (GLfloat)GET_G(m_diffuseColor) / 255.0f;
  diffuseColor[2] = (GLfloat)GET_B(m_diffuseColor) / 255.0f;
  diffuseColor[3] = 1.0;
  glUniform4fv(m_uniColor, 1, diffuseColor);

  glVertexAttribPointer(m_attribVertex, 3, GL_FLOAT, GL_FALSE, sizeof(ImageVertex), (char*)v + offsetof(ImageVertex, x));

  glDisable(GL_BLEND);
  glDisable(GL_SCISSOR_TEST);
  glDrawElements( GL_LINE_LOOP, 6, GL_UNSIGNED_INT, indices);
}

static Shaders::CGLSLShaderProgram* g_pDrawQuadShadersProgram = NULL;

void CGUITextureGL::DrawQuad(const CRect &coords, DWORD color, bool bWireFrame, CBaseTexture *texture, const CRect *texCoords)
{
  if (g_pDrawQuadShadersProgram == NULL)
  {
    g_pDrawQuadShadersProgram = new Shaders::CGLSLShaderProgram();
    g_pDrawQuadShadersProgram->VertexShader()->SetSource(g_TextureVertexShader);
    g_pDrawQuadShadersProgram->PixelShader()->SetSource(g_ColorPixelShader);
    g_pDrawQuadShadersProgram->CompileAndLink();
  }

  if(bWireFrame)
  {
    if(g_pWireFrameShader == NULL)
    {
      g_pWireFrameShader = new Shaders::CGLSLShaderProgram();
      g_pWireFrameShader->VertexShader()->SetSource(g_TextureVertexShader);
      g_pWireFrameShader->PixelShader()->SetSource(g_PixelShaderWireFrame);
      g_pWireFrameShader->CompileAndLink();
    }
  }

  if (texture)
  {
    glActiveTexture(GL_TEXTURE0);
    texture->LoadToGPU();
    glBindTexture(GL_TEXTURE_2D, texture->GetTextureObject());
    glEnable(GL_TEXTURE_2D);
  }
  else
    glDisable(GL_TEXTURE_2D);

  g_Windowing.EnableBlending(true, true);

  GLuint progObj = -1;

  if(bWireFrame)
  {
    g_pWireFrameShader->Enable();
    progObj = g_pWireFrameShader->ProgramHandle();
  }
  else
  {
    g_pDrawQuadShadersProgram->Enable();
    progObj = g_pDrawQuadShadersProgram->ProgramHandle();
  }

  // Set diffuse color
  GLint uniColor = glGetUniformLocation(progObj, "u_diffuseColor");
  GLfloat diffuseColor[4];
  diffuseColor[0] = (GLfloat)GET_R(color) / 255.0f;
  diffuseColor[1] = (GLfloat)GET_G(color) / 255.0f;
  diffuseColor[2] = (GLfloat)GET_B(color) / 255.0f;
  diffuseColor[3] = (GLfloat)GET_A(color) / 255.0f;
  glUniform4fv(uniColor, 1, diffuseColor);

  g_graphicsContext.PushViewPort(0, 0, g_graphicsContext.GetWidth(), g_graphicsContext.GetHeight(), false);
  bool clip = g_graphicsContext.SetClipRegion(0, 0, g_graphicsContext.GetWidth(), g_graphicsContext.GetHeight(), false);

  // Set transformation matrixes
  TransformMatrix* matProjection = g_Windowing.GetHardwareTransform(MATRIX_TYPE_PROJECTION);
  GLint uniMatProjection  = glGetUniformLocation(progObj, "u_matProjection");
  glUniformMatrix4fv(uniMatProjection, 1, GL_FALSE, (GLfloat *)matProjection->m);

  TransformMatrix* matModelView = g_Windowing.GetHardwareTransform(MATRIX_TYPE_MODEL_VIEW);
  GLint uniMatModelView  = glGetUniformLocation(progObj, "u_matModelView");
  glUniformMatrix4fv(uniMatModelView, 1, GL_FALSE, (GLfloat *)matModelView->m);

  // Set vertices
  GLint attribVertex = glGetAttribLocation(progObj, "a_vertex");
  glEnableVertexAttribArray(attribVertex);

  ImageVertex v[4];
  v[0].x = coords.x1;
  v[0].y = coords.y1;
  v[0].z = 0;
  v[0].u1 = 0;
  v[0].v1 = 0;

  v[3].x = coords.x1;
  v[3].y = coords.y2;
  v[3].z = 0;
  v[3].u1 = 0;
  v[3].v1 = 1;

  v[2].x = coords.x2;
  v[2].y = coords.y2;
  v[2].z = 0;
  v[2].u1 = 1;
  v[2].v1 = 1;

  v[1].x = coords.x2;
  v[1].y = coords.y1;
  v[1].z = 0;
  v[1].u1 = 1;
  v[1].v1 = 0;

  glVertexAttribPointer(attribVertex, 3, GL_FLOAT, GL_FALSE, sizeof(ImageVertex), (char*)v + offsetof(ImageVertex, x));

  // Draw
  GLuint indices[6] =
  {
    0, 1, 2,
    2, 3, 0,
  };

  if(bWireFrame)
    glDrawElements( GL_LINE_LOOP, 6, GL_UNSIGNED_INT, indices);
  else
    glDrawElements(GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_INT, indices);

  if(clip)
    g_graphicsContext.RestoreClipRegion();
  g_graphicsContext.PopViewPort();

  glDisableVertexAttribArray(attribVertex);

  if(bWireFrame)
    g_pWireFrameShader->Disable();
  else
    g_pDrawQuadShadersProgram->Disable();

  g_Windowing.EnableBlending(false);

  if (texture)
    glDisable(GL_TEXTURE_2D);
}

void CGUITextureGL::DrawLine(const CRect &coords, DWORD color)
{
  if (g_pDrawQuadShadersProgram == NULL)
  {
    g_pDrawQuadShadersProgram = new Shaders::CGLSLShaderProgram();
    g_pDrawQuadShadersProgram->VertexShader()->SetSource(g_TextureVertexShader);
    g_pDrawQuadShadersProgram->PixelShader()->SetSource(g_ColorPixelShader);
    g_pDrawQuadShadersProgram->CompileAndLink();
  }

  glDisable(GL_TEXTURE_2D);

  g_pDrawQuadShadersProgram->Enable();
  GLuint progObj = g_pDrawQuadShadersProgram->ProgramHandle();

  // Set diffuse color
  GLint uniColor = glGetUniformLocation(progObj, "u_diffuseColor");
  GLfloat diffuseColor[4];
  diffuseColor[0] = (GLfloat)GET_R(color) / 255.0f;
  diffuseColor[1] = (GLfloat)GET_G(color) / 255.0f;
  diffuseColor[2] = (GLfloat)GET_B(color) / 255.0f;
  diffuseColor[3] = (GLfloat)GET_A(color) / 255.0f;
  glUniform4fv(uniColor, 1, diffuseColor);

  g_graphicsContext.PushViewPort(0, 0, g_graphicsContext.GetWidth(), g_graphicsContext.GetHeight(), false);
  bool clip = g_graphicsContext.SetClipRegion(0, 0, g_graphicsContext.GetWidth(), g_graphicsContext.GetHeight(), false);

  // Set transformation matrixes
  TransformMatrix* matProjection = g_Windowing.GetHardwareTransform(MATRIX_TYPE_PROJECTION);
  GLint uniMatProjection  = glGetUniformLocation(progObj, "u_matProjection");
  glUniformMatrix4fv(uniMatProjection, 1, GL_FALSE, (GLfloat *)matProjection->m);

  TransformMatrix* matModelView = g_Windowing.GetHardwareTransform(MATRIX_TYPE_MODEL_VIEW);
  GLint uniMatModelView  = glGetUniformLocation(progObj, "u_matModelView");
  glUniformMatrix4fv(uniMatModelView, 1, GL_FALSE, (GLfloat *)matModelView->m);

  // Set vertices
  GLint attribVertex = glGetAttribLocation(progObj, "a_vertex");
  glEnableVertexAttribArray(attribVertex);

  ImageVertex v[2];
  v[0].x = coords.x1;
  v[0].y = coords.y1;
  v[0].z = 0;

  v[1].x = coords.x2;
  v[1].y = coords.y2;
  v[1].z = 0;

  glVertexAttribPointer(attribVertex, 3, GL_FLOAT, GL_FALSE, sizeof(ImageVertex), (char*)v + offsetof(ImageVertex, x));

  // Draw
  glDrawArrays(GL_LINES, 0, 2);

  if(clip)
    g_graphicsContext.RestoreClipRegion();
  g_graphicsContext.PopViewPort();

  glDisableVertexAttribArray(attribVertex);
  g_pDrawQuadShadersProgram->Disable();
}

#endif
