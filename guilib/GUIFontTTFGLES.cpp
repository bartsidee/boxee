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
#include "WindowingFactory.h"
#include "AdvancedSettings.h"
#include "GUIFont.h"
#include "GUIFontTTFGL.h"
#include "GUIFontManager.h"
#include "Texture.h"
#include "GraphicContext.h"
#include "GUIWindowManager.h"
#include "gui3d.h"
#include "utils/log.h"
#include "GUIFontTTFGLES.h"

// stuff for freetype
#ifndef _LINUX
#include "ft2build.h"
#else
#include <ft2build.h>
#endif
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

using namespace std;

#ifdef HAS_GLES


static std::string g_FontsVertexShader = 
"uniform mat4 u_matModelView;\n"
"uniform mat4 u_matProjection;\n"
"// Input vertex parameters\n"
"attribute vec3 a_vertex;\n"
"attribute vec4 a_color;\n"
"attribute vec2 a_texCoord;\n"
"varying vec4 v_colorVar;\n"
"varying vec2 v_texCoord;\n"
"void main() \n"
"{\n"
"v_texCoord = a_texCoord;\n"
"v_colorVar	= a_color;\n"
"gl_Position = u_matProjection * u_matModelView * vec4(a_vertex,1.0);\n"
"}\n";


static std::string g_FontsPixelShader = 
#ifndef HAS_GL2
"precision mediump float;\n"
#endif
"varying vec2 v_texCoord;\n"
"varying vec4 v_colorVar;\n"
"uniform sampler2D u_texture\n;"
"void main()\n"
"{\n"
"gl_FragColor = v_colorVar;\n"
"gl_FragColor.a = texture2D(u_texture, v_texCoord).a * v_colorVar.a;\n"
"}\n";

static std::string g_FontsEmptyPixelShader = 
"void main()\n"
"{\n"
"}\n";

static Shaders::CGLSLShaderProgram* g_pFontsShadersProgram = NULL;
static Shaders::CGLSLShaderProgram* g_pEmptyFontsShadersProgram = NULL;
static Shaders::CGLSLShaderProgram* g_pCurrentFontsShadersProgram = NULL;

#ifdef _DEBUG
static unsigned long long pixelCount = 0;
static int texturesCount = 0;
#define RATIO 2.25f
#endif

CGUIFontTTFGLES::CGUIFontTTFGLES(const CStdString& strFileName)
: CGUIFontTTFBase(strFileName)
{
  m_uniMatProjection  = -1;
  m_uniMatModelView  = -1;
  m_uniTexture = -1;

  m_attribVertex = -1;
  m_attribTextureCoord = -1;
  m_attribColor = -1;
}

CGUIFontTTFGLES::~CGUIFontTTFGLES(void)
{
 
}

void CGUIFontTTFGLES::Begin()
{
  m_uniMatModelView = -1;
  m_uniMatProjection = -1;
  m_uniTexture = -1;
  m_attribVertex = -1;
  m_attribTextureCoord = -1;
  m_attribColor = -1;

  if(g_pFontsShadersProgram == NULL || g_pEmptyFontsShadersProgram == NULL)
    return;

  g_pCurrentFontsShadersProgram = g_pFontsShadersProgram;

  if (!m_bTextureLoaded)
  {
    // Have OpenGL generate a texture object handle for us
    glGenTextures(1, (GLuint*) &m_nTexture);

    // Bind the texture object
    glBindTexture(GL_TEXTURE_2D, m_nTexture);

    // Set the texture's stretching properties
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Set the texture image -- THIS WORKS, so the pixels must be wrong.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, m_texture->GetWidth(), m_texture->GetHeight(), 0,
        GL_ALPHA, GL_UNSIGNED_BYTE, m_texture->GetPixels());

    m_bTextureLoaded = true;
  }

  g_pCurrentFontsShadersProgram->Enable();

  GLuint progObj = g_pCurrentFontsShadersProgram->ProgramHandle();
  m_uniMatProjection  = glGetUniformLocation(progObj, "u_matProjection");
  m_uniMatModelView  = glGetUniformLocation(progObj, "u_matModelView");
  m_uniTexture = glGetUniformLocation(progObj, "u_texture");
  m_attribVertex = glGetAttribLocation(progObj, "a_vertex");
  m_attribTextureCoord = glGetAttribLocation(progObj, "a_texCoord");
  m_attribColor = glGetAttribLocation(progObj, "a_color");

  glActiveTexture(GL_TEXTURE0);
  if(m_attribVertex != -1)
    glEnableVertexAttribArray(m_attribVertex); 
  if(m_attribTextureCoord != -1)
    glEnableVertexAttribArray(m_attribTextureCoord);
  if(m_attribColor != -1)
    glEnableVertexAttribArray(m_attribColor);

  TransformMatrix* matModelView = g_Windowing.GetHardwareTransform(MATRIX_TYPE_MODEL_VIEW);
  TransformMatrix* matProjection = g_Windowing.GetHardwareTransform(MATRIX_TYPE_PROJECTION);

  if(m_uniMatModelView != -1)
    glUniformMatrix4fv(m_uniMatModelView, 1, GL_FALSE, (GLfloat *)matModelView->m);
  if(m_uniMatProjection != -1)
    glUniformMatrix4fv(m_uniMatProjection, 1, GL_FALSE, (GLfloat *)matProjection->m); 

  if(m_uniTexture != -1)
    glUniform1i(m_uniTexture, 0);

  glBindTexture(GL_TEXTURE_2D, m_nTexture);
}

void CGUIFontTTFGLES::Render(FontCoordsIndiced& coords, bool useShadow)
{
  FontVertex* v = (FontVertex *)&coords.m_pCoord[0];
  //assert(v != 0);
  GLvoid* indices = &coords.m_nIndices[0];
  unsigned int size = coords.m_pCoord.size();

#ifdef _DEBUG
  
  if (g_advancedSettings.m_bCountPixels)
  {
    texturesCount += size;
    for (unsigned int i = 0; i < size; i++)
    {
      FontCoords& fontCoords = coords.m_pCoord[i];
      pixelCount += (long) ((fontCoords.m_pCoords[2].x - fontCoords.m_pCoords[0].x) * (fontCoords.m_pCoords[2].y - fontCoords.m_pCoords[0].y) * RATIO);
    }
  }
#endif

  if(m_attribVertex != -1)
    glVertexAttribPointer(m_attribVertex, 3, GL_FLOAT, GL_FALSE, sizeof(FontVertex), (char*)v + offsetof(FontVertex, x));
  if(m_attribTextureCoord != -1)
    glVertexAttribPointer(m_attribTextureCoord, 2, GL_FLOAT, GL_FALSE, sizeof(FontVertex), (char*)v + offsetof(FontVertex, u1)); 

  if(useShadow)
  {
    TransformMatrix mat = TransformMatrix::CreateTranslation(1.0f, 1.0f, 0.0f);
    g_graphicsContext.PushTransform(mat);
    if(m_attribColor != -1)
      glVertexAttribPointer(m_attribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(FontVertex), (char*)v + offsetof(FontVertex, rs)); 
    glDrawElements( GL_TRIANGLES, 6 * size, GL_UNSIGNED_SHORT, indices);
    g_graphicsContext.PopTransform();
  }
  if(m_attribColor != -1)
    glVertexAttribPointer(m_attribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(FontVertex), (char*)v + offsetof(FontVertex, r)); 

#ifdef _DEBUG
  if(g_advancedSettings.m_bWireFrameMode)
  {
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glDrawElements( GL_LINE_LOOP, 6 * size, GL_UNSIGNED_SHORT, indices);
  }
  else
#endif
    glDrawElements( GL_TRIANGLES, 6 * size, GL_UNSIGNED_SHORT, indices);
}

void CGUIFontTTFGLES::End()
{
  if(m_attribVertex != -1)
    glDisableVertexAttribArray(m_attribVertex);
  if(m_attribTextureCoord != -1)
    glDisableVertexAttribArray(m_attribTextureCoord);
  if(m_attribColor != -1)
    glDisableVertexAttribArray(m_attribColor);
}

CBaseTexture* CGUIFontTTFGLES::ReallocTexture(unsigned int& newHeight)
{
  newHeight = CBaseTexture::PadPow2(newHeight);

  CBaseTexture* newTexture = new CTexture(m_textureWidth, newHeight, XB_FMT_A8);

  if (!newTexture || newTexture->GetPixels() == NULL)
  {
    CLog::Log(LOGERROR, "GUIFontTTFGL::CacheCharacter: Error creating new cache texture for size %f", m_height);
    return NULL;
  }
  m_textureHeight = newTexture->GetHeight();
  m_textureWidth = newTexture->GetWidth();

  if (m_texture)
  {
    unsigned char* src = (unsigned char*) m_texture->GetPixels();
    unsigned char* dst = (unsigned char*) newTexture->GetPixels();
    for (unsigned int y = 0; y < m_texture->GetHeight(); y++)
    {
      memcpy(dst, src, m_texture->GetPitch());
      src += m_texture->GetPitch();
      dst += newTexture->GetPitch();
    }
    delete m_texture;
  }

  return newTexture;
}

bool CGUIFontTTFGLES::CopyCharToTexture(FT_BitmapGlyph bitGlyph, Character* ch)
{
  FT_Bitmap bitmap = bitGlyph->bitmap;

  unsigned char* source = (unsigned char*) bitmap.buffer;
  unsigned char* target = (unsigned char*) m_texture->GetPixels() + (m_posY + ch->offsetY) * m_texture->GetPitch() + m_posX + bitGlyph->left;

  for (int y = 0; y < bitmap.rows; y++)
  {
    memcpy(target, source, bitmap.width);
    source += bitmap.width;
    target += m_texture->GetPitch();
  }
  // THE SOURCE VALUES ARE THE SAME IN BOTH SITUATIONS.

  // Since we have a new texture, we need to delete the old one
  // the Begin(); End(); stuff is handled by whoever called us
  if (m_bTextureLoaded)
  {
    g_graphicsContext.BeginPaint();  //FIXME
    DeleteHardwareTexture();
    g_graphicsContext.EndPaint();
    m_bTextureLoaded = false;
  }

  return TRUE;
}

void CGUIFontTTFGLES::DeleteHardwareTexture()
{
  if (m_bTextureLoaded)
  {
    if (glIsTexture(m_nTexture))
      glDeleteTextures(1, (GLuint*) &m_nTexture);
    m_bTextureLoaded = false;
  }
}

bool CGUIFontTTFGLES::LoadShaders()
{
  if(g_pFontsShadersProgram != NULL)
    return true;

  g_pFontsShadersProgram = new Shaders::CGLSLShaderProgram();
  g_pFontsShadersProgram->VertexShader()->SetSource(g_FontsVertexShader);
  g_pFontsShadersProgram->PixelShader()->SetSource(g_FontsPixelShader);
  g_pFontsShadersProgram->CompileAndLink();

  g_pEmptyFontsShadersProgram = new Shaders::CGLSLShaderProgram();
  g_pEmptyFontsShadersProgram->VertexShader()->SetSource(g_FontsVertexShader);
  g_pEmptyFontsShadersProgram->PixelShader()->SetSource(g_FontsEmptyPixelShader);
  g_pEmptyFontsShadersProgram->CompileAndLink();

  return true;
}

#endif
