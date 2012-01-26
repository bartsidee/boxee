/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *      Initial code sponsored by: Voddler Inc (voddler.com)
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

#if defined(HAS_GL) || defined(HAS_GLES)

#include "OverlayRenderer.h"
#include "OverlayRendererUtil.h"
#include "OverlayRendererGL.h"
#include "LinuxRendererGL.h"
#include "RenderManager.h"
#include "cores/dvdplayer/DVDCodecs/Overlay/DVDOverlayImage.h"
#include "cores/dvdplayer/DVDCodecs/Overlay/DVDOverlaySpu.h"
#include "cores/dvdplayer/DVDCodecs/Overlay/DVDOverlaySSA.h"
#include "WindowingFactory.h" 
#include "Shader.h"

#define USE_PREMULTIPLIED_ALPHA 1

using namespace OVERLAY;

static std::string g_OverlayVertexShader = 
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
"v_colorVar = a_color;\n"
"gl_Position = u_matProjection * u_matModelView * vec4(a_vertex,1.0);\n"
"}\n";


static std::string g_OverlayPixelShader =
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

static std::string g_OverlayTexturePixelShader =
#ifndef HAS_GL2
"precision mediump float;\n"
#endif
"varying vec2 v_texCoord;\n"
"uniform vec4 v_colorVar;\n"
"uniform sampler2D u_texture\n;"
"void main()\n"
"{\n"
"gl_FragColor.rgb = texture2D(u_texture, v_texCoord).rgb * v_colorVar.rgb;\n"
"gl_FragColor.a = texture2D(u_texture, v_texCoord).a;\n"
"}\n";

static Shaders::CGLSLShaderProgram* g_pOverlayShadersProgram = NULL;
static Shaders::CGLSLShaderProgram* g_pOverlayTextureShadersProgram = NULL;

static void LoadTexture(GLenum target
                      , GLsizei width, GLsizei height, GLsizei stride
                      , GLfloat* u, GLfloat* v
                      , GLenum format, const GLvoid* pixels)
{
  int width2  = NP2(width);
  int height2 = NP2(height);

#ifdef HAS_EMBEDDED
  width2 = width;
  height2 = height;
#endif

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);

#ifdef HAS_GLES

  glTexImage2D( GL_TEXTURE_2D, 0, format, width2, height2, 0, format, GL_UNSIGNED_BYTE, NULL );

  for( int y = 0; y < height; y++ )
  {
      char *row = (char *)pixels + y * stride;
      glTexSubImage2D( GL_TEXTURE_2D, 0, 0, y, width, 1, format, GL_UNSIGNED_BYTE, row );
  }

#else

  if(format == GL_RGBA)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / 4);
  else if(format == GL_RGB)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / 3);
  else
    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);

  glTexImage2D   (target, 0, format
                , width2, height2, 0
                , format, GL_UNSIGNED_BYTE, NULL);

  glTexSubImage2D(target, 0
                , 0, 0, width, height
                , format, GL_UNSIGNED_BYTE
                , pixels);

  if(height < height2)
    glTexSubImage2D( target, 0
                   , 0, height, width, 1
                   , format, GL_UNSIGNED_BYTE
                   , (unsigned char*)pixels + stride * (height-1));

  if(width  < width2)
    glTexSubImage2D( target, 0
                   , width, 0, 1, height
                   , format, GL_UNSIGNED_BYTE
                   , (unsigned char*)pixels + stride - 1);

  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

#endif

  *u = (GLfloat)width  / width2;
  *v = (GLfloat)height / height2;
}

COverlayTextureGL::COverlayTextureGL(CDVDOverlayImage* o)
{
  m_texture = 0;

  uint32_t* rgba = convert_rgba(o, USE_PREMULTIPLIED_ALPHA);

  if(!rgba)
  {
    CLog::Log(LOGERROR, "COverlayTextureGL::COverlayTextureGL - failed to convert overlay to rgb");
    return;
  }

  glGenTextures(1, &m_texture);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  LoadTexture(GL_TEXTURE_2D
            , o->width
            , o->height
            , o->width * 4
            , &m_u, &m_v
            , GL_RGBA
            , rgba);
  free(rgba);

  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);

  if(o->source_width && o->source_height)
  {
    float center_x = (float)(0.5f * o->width  + o->x) / o->source_width;
    float center_y = (float)(0.5f * o->height + o->y) / o->source_height;

    m_width  = (float)o->width  / o->source_width;
    m_height = (float)o->height / o->source_height;
    m_pos    = POSITION_RELATIVE;

#if 0
    if(center_x > 0.4 && center_x < 0.6
    && center_y > 0.8 && center_y < 1.0)
    {
     /* render bottom aligned to subtitle line */
      m_align  = ALIGN_SUBTITLE;
      m_x      = 0.0f;
      m_y      = - 0.5 * m_height;
    }
    else
#endif
    {
      /* render aligned to screen to avoid cropping problems */
      m_align  = ALIGN_SCREEN;
      m_x      = center_x;
      m_y      = center_y;
    }
  }
  else
  {
    m_align  = ALIGN_VIDEO;
    m_pos    = POSITION_ABSOLUTE;
    m_x      = (float)o->x;
    m_y      = (float)o->y;
    m_width  = (float)o->width;
    m_height = (float)o->height;
  }
}

COverlayTextureGL::COverlayTextureGL(CDVDOverlaySpu* o)
{
  m_texture = 0;

  int min_x, max_x, min_y, max_y;
  uint32_t* rgba = convert_rgba(o, USE_PREMULTIPLIED_ALPHA
                              , min_x, max_x, min_y, max_y);

  if(!rgba)
  {
    CLog::Log(LOGERROR, "COverlayTextureGL::COverlayTextureGL - failed to convert overlay to rgb");
    return;
  }

  glGenTextures(1, &m_texture);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  LoadTexture(GL_TEXTURE_2D
            , max_x - min_x
            , max_y - min_y
            , o->width * 4
            , &m_u, &m_v
            , GL_RGBA
            , rgba + min_x + min_y * o->width);

  free(rgba);

  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);

  m_align  = ALIGN_VIDEO;
  m_pos    = POSITION_ABSOLUTE;
  m_x      = (float)(min_x + o->x);
  m_y      = (float)(min_y + o->y);
  m_width  = (float)(max_x - min_x);
  m_height = (float)(max_y - min_y);
}

COverlayGlyphGL::COverlayGlyphGL(CDVDOverlaySSA* o, double pts)
{
  m_vertex = NULL;

  m_width  = (float)g_graphicsContext.GetWidth();
  m_height = (float)g_graphicsContext.GetHeight();
  m_align  = ALIGN_SCREEN;
  m_pos    = POSITION_ABSOLUTE;
  m_x      = (float)0.0f;
  m_y      = (float)0.0f;

  m_texture = ~(GLuint)0;

  SQuads quads;
  if(!convert_quad(o, pts, (int)m_width, (int)m_height, quads))
    return;
    
  glGenTextures(1, &m_texture);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  LoadTexture(GL_TEXTURE_2D
            , quads.size_x
            , quads.size_y
            , quads.size_x
            , &m_u, &m_v
            , GL_ALPHA
            , quads.data);


  float scale_u = m_u / quads.size_x;
  float scale_v = m_v / quads.size_y;

  float scale_x = 1.0f / m_width;
  float scale_y = 1.0f / m_height;

  m_count  = quads.count;
  m_vertex = (VERTEX*)calloc(m_count * 6, sizeof(VERTEX));

  VERTEX* vt = m_vertex;
  SQuad*  vs = quads.quad;


  for(int i = 0; i < quads.count; i++)
  {
    for(int s = 0; s < 6; s++)
    {
      vt[s].a = vs->a;
      vt[s].r = vs->r;
      vt[s].g = vs->g;
      vt[s].b = vs->b;

      vt[s].z = 0.0f;
      vt[s].x = scale_x;
      vt[s].y = scale_y;
      vt[s].u = scale_u;
      vt[s].v = scale_v;
    }


    vt[0].x *= vs->x;
    vt[0].u *= vs->u;
    vt[0].y *= vs->y;
    vt[0].v *= vs->v;

    vt[1].x *= vs->x + vs->w;
    vt[1].u *= vs->u + vs->w;
    vt[1].y *= vs->y;
    vt[1].v *= vs->v;

    vt[2].x *= vs->x;
    vt[2].u *= vs->u;
    vt[2].y *= vs->y + vs->h;
    vt[2].v *= vs->v + vs->h;

    vt[3] = vt[1];

    vt[4].x *= vs->x + vs->w;
    vt[4].u *= vs->u + vs->w;
    vt[4].y *= vs->y + vs->h;
    vt[4].v *= vs->v + vs->h;

    vt[5] = vt[2];

    vs += 1;
    vt += 6;
  }

  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);
}

COverlayGlyphGL::~COverlayGlyphGL()
{
  glDeleteTextures(1, &m_texture);
  free(m_vertex);
}

void COverlayGlyphGL::Render(SRenderState& state)
{
  if (m_texture == ~GLuint(0))
    return;

  glEnable(GL_TEXTURE_2D);
  g_Windowing.EnableBlending(true);

  m_uniMatModelView = -1;
  m_uniMatProjection = -1;
  m_uniTexture = -1;
  m_attribVertex = -1;
  m_attribTextureCoord = -1;
  m_attribColor = -1;

  if(g_pOverlayShadersProgram == NULL)
  {
    g_pOverlayShadersProgram = new Shaders::CGLSLShaderProgram();
    g_pOverlayShadersProgram->VertexShader()->SetSource(g_OverlayVertexShader);
    g_pOverlayShadersProgram->PixelShader()->SetSource(g_OverlayPixelShader);
    g_pOverlayShadersProgram->CompileAndLink();
  }

  g_pOverlayShadersProgram->Enable();

  GLuint progObj = g_pOverlayShadersProgram->ProgramHandle();
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

  TransformMatrix* matProjection = g_Windowing.GetHardwareTransform(MATRIX_TYPE_PROJECTION);

  g_graphicsContext.PushTransform(TransformMatrix(), true);

  TransformMatrix transMat = TransformMatrix::CreateTranslation(state.x, state.y);
  g_graphicsContext.PushTransform(transMat);
  TransformMatrix scaleMat = TransformMatrix::CreateScaler(state.width, state.height);
  g_graphicsContext.PushTransform(scaleMat);

  TransformMatrix* matModelView = g_Windowing.GetHardwareTransform(MATRIX_TYPE_MODEL_VIEW);

  if(m_uniMatModelView != -1)
    glUniformMatrix4fv(m_uniMatModelView, 1, GL_FALSE, (GLfloat *)matModelView->m);
  if(m_uniMatProjection != -1)
    glUniformMatrix4fv(m_uniMatProjection, 1, GL_FALSE, (GLfloat *)matProjection->m); 

  glBindTexture(GL_TEXTURE_2D, m_texture);

  if(m_uniTexture != -1)
    glUniform1i(m_uniTexture, 0);

  if(m_attribVertex != -1)
    glVertexAttribPointer(m_attribVertex, 3, GL_FLOAT, GL_FALSE, sizeof(VERTEX), (char*)m_vertex + offsetof(VERTEX, x));
  if(m_attribTextureCoord != -1)
    glVertexAttribPointer(m_attribTextureCoord, 2, GL_FLOAT, GL_FALSE, sizeof(VERTEX), (char*)m_vertex + offsetof(VERTEX, u)); 
  if(m_attribColor != -1)
    glVertexAttribPointer(m_attribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VERTEX), (char*)m_vertex + offsetof(VERTEX, r)); 

  glDrawArrays(GL_TRIANGLES, 0, 6 * m_count);

  glBindTexture(GL_TEXTURE_2D, 0);

  if(m_attribVertex != -1)
    glDisableVertexAttribArray(m_attribVertex);
  if(m_attribTextureCoord != -1)
    glDisableVertexAttribArray(m_attribTextureCoord);
  if(m_attribColor != -1)
    glDisableVertexAttribArray(m_attribColor);

  g_Windowing.EnableBlending(false);

  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);

  g_graphicsContext.PopTransform();
  g_graphicsContext.PopTransform();
  g_graphicsContext.PopTransform();
}


COverlayTextureGL::~COverlayTextureGL()
{
  glDeleteTextures(1, &m_texture);
}

void COverlayTextureGL::Render(SRenderState& state)
{
  VERTEX m_vertex[4];

  DRAWRECT rd;
  if(m_pos == POSITION_RELATIVE)
  {
    rd.top     = state.y - state.height * 0.5;
    rd.bottom  = state.y + state.height * 0.5;
    rd.left    = state.x - state.width  * 0.5;
    rd.right   = state.x + state.width  * 0.5;
  }
  else
  {
    rd.top     = state.y;
    rd.bottom  = state.y + state.height;
    rd.left    = state.x;
    rd.right   = state.x + state.width;
  }

  if(rd.bottom - rd.top < 0 || rd.right - rd.left < 0)
  {
    return;
  }

  m_vertex[0].x = rd.left;
  m_vertex[0].y = rd.top;
  m_vertex[0].z = 0;
  m_vertex[0].u = 0.0;
  m_vertex[0].v = 0.0;

  m_vertex[1].x = rd.right;
  m_vertex[1].y = rd.top;
  m_vertex[1].z = 0;
  m_vertex[1].u = m_u;
  m_vertex[1].v = 0.0;

  m_vertex[2].x = rd.right;
  m_vertex[2].y = rd.bottom;
  m_vertex[2].z = 0;
  m_vertex[2].u = m_u;
  m_vertex[2].v = m_v;

  m_vertex[3].x = rd.left;
  m_vertex[3].y = rd.bottom;
  m_vertex[3].z = 0;
  m_vertex[3].u = 0;
  m_vertex[3].v = m_v;

  if (m_texture == ~GLuint(0))
    return;

  glEnable(GL_TEXTURE_2D);

#if USE_PREMULTIPLIED_ALPHA
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
#else
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
#endif

  m_uniMatModelView = -1;
  m_uniMatProjection = -1;
  m_uniTexture = -1;
  m_attribVertex = -1;
  m_attribTextureCoord = -1;

  if(g_pOverlayTextureShadersProgram == NULL)
  {
    g_pOverlayTextureShadersProgram = new Shaders::CGLSLShaderProgram();
    g_pOverlayTextureShadersProgram->VertexShader()->SetSource(g_OverlayVertexShader);
    g_pOverlayTextureShadersProgram->PixelShader()->SetSource(g_OverlayTexturePixelShader);
    g_pOverlayTextureShadersProgram->CompileAndLink();
  }

  g_pOverlayTextureShadersProgram->Enable();

  GLuint progObj = g_pOverlayTextureShadersProgram->ProgramHandle();
  m_uniMatProjection  = glGetUniformLocation(progObj, "u_matProjection");
  m_uniMatModelView  = glGetUniformLocation(progObj, "u_matModelView");
  m_uniTexture = glGetUniformLocation(progObj, "u_texture");
  m_attribVertex = glGetAttribLocation(progObj, "a_vertex");
  m_attribTextureCoord = glGetAttribLocation(progObj, "a_texCoord");

  // Set diffuse color
  GLint uniColor = glGetUniformLocation(progObj, "v_colorVar");
  GLfloat diffuseColor[4];
  diffuseColor[0] = 1.0;
  diffuseColor[1] = 1.0;
  diffuseColor[2] = 1.0;
  diffuseColor[3] = 1.0;
  glUniform4fv(uniColor, 1, diffuseColor);

  glActiveTexture(GL_TEXTURE0);
  if(m_attribVertex != -1)
    glEnableVertexAttribArray(m_attribVertex);
  if(m_attribTextureCoord != -1)
    glEnableVertexAttribArray(m_attribTextureCoord);

  TransformMatrix* matProjection = g_Windowing.GetHardwareTransform(MATRIX_TYPE_PROJECTION);
  TransformMatrix finalMat;

  if(m_uniMatModelView != -1)
    glUniformMatrix4fv(m_uniMatModelView, 1, GL_FALSE, (GLfloat *)finalMat.m);
  if(m_uniMatProjection != -1)
    glUniformMatrix4fv(m_uniMatProjection, 1, GL_FALSE, (GLfloat *)matProjection->m);

  glBindTexture(GL_TEXTURE_2D, m_texture);

  if(m_uniTexture != -1)
    glUniform1i(m_uniTexture, 0);

  if(m_attribVertex != -1)
    glVertexAttribPointer(m_attribVertex, 3, GL_FLOAT, GL_FALSE, sizeof(VERTEX), (char*)m_vertex + offsetof(VERTEX, x));
  if(m_attribTextureCoord != -1)
    glVertexAttribPointer(m_attribTextureCoord, 2, GL_FLOAT, GL_FALSE, sizeof(VERTEX), (char*)m_vertex + offsetof(VERTEX, u));

  // Draw
  GLuint indices[6] =
  {
    0, 1, 2,
    2, 3, 0,
  };

  glDrawElements(GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_INT, indices);

  glBindTexture(GL_TEXTURE_2D, 0);

  if(m_attribVertex != -1)
    glDisableVertexAttribArray(m_attribVertex);
  if(m_attribTextureCoord != -1)
    glDisableVertexAttribArray(m_attribTextureCoord);

  g_Windowing.EnableBlending(false);

  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);

  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, 0);
}

#endif // HAS_GL
