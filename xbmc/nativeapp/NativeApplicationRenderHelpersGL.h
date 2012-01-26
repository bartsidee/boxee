#ifndef __NATIVE_APP_RENDER_HELPERS__H__
#define __NATIVE_APP_RENDER_HELPERS__H__

/* native applications - framework for boxee native applications
 * Copyright (C) 2010 Boxee.tv.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "Application.h"
#include "FrameBufferObject.h"
#include "ApplicationMessenger.h"
#include "Shader.h"
#include "WindowingFactory.h"
#include "GUITexture.h"

extern CFrameBufferObject g_fbo;

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
"uniform sampler2D u_texture0;\n"
"uniform vec4 u_diffuseColor;\n"
"void main()\n"
"{\n"
"vec4 texture0Color;\n"
"texture0Color = texture2D(u_texture0, v_texCoord0) * u_diffuseColor;\n"
#ifdef HAS_EMBEDDED
"gl_FragColor.rgba = texture0Color.bgra;\n"
#else
"gl_FragColor.rgba = texture0Color.rgba;\n"
#endif
"}\n";

static std::string g_AlphaPixelShader =
#ifndef HAS_GL2
"precision mediump float;\n"
#endif
"varying vec2 v_texCoord0;\n"
"uniform sampler2D u_texture0;\n"
"uniform vec4 u_diffuseColor;\n"
"void main()\n"
"{\n"
"vec4 texture0Color;\n"
"gl_FragColor.a = texture2D(u_texture0, v_texCoord0).a;\n"
#ifdef HAS_EMBEDDED
"gl_FragColor.rgb = u_diffuseColor.bgr;\n"
#else
"gl_FragColor.rgb = u_diffuseColor.rgb;\n"
#endif
"}\n";

static std::string g_ColorPixelShader =
#ifndef HAS_GL2
"precision mediump float;\n"
#endif
"uniform vec4 u_diffuseColor;\n"
"void main()\n"
"{\n"
#ifdef HAS_EMBEDDED
"gl_FragColor.rgba = u_diffuseColor.bgra;\n"
#else
"gl_FragColor.rgba = u_diffuseColor.rgba;\n"
#endif
"}\n";

extern Shaders::CGLSLShaderProgram* g_TextureShaderProgram;
extern Shaders::CGLSLShaderProgram* g_ColorShaderProgram;
extern Shaders::CGLSLShaderProgram* g_AlphaShaderProgram;

typedef struct sBXSurfacePrivData
{
  CCriticalSection          lock;
  BOXEE::NativeApplication *app;
  GLuint                    texid;
  bool                      initialized;
  bool                      bLoadedToGPU;

  sBXSurfacePrivData() : app(NULL), texid(0), initialized(false)  { }

  ~sBXSurfacePrivData()
  {
    if (initialized)
    {
      glDeleteTextures(1, &texid);
      texid = 0;
    }
  }

  void Initialize(BX_Surface *s)
  {
    if (initialized)
      return;

#ifdef HAS_GLES
    if(s->w > 2048 || s->h > 2048)
    {
      CLog::Log(LOGERROR,"sBXSurfacePrivData::Initialize size is too big width = %d height = %d",
          s->w, s->h);
      return ;
    }
#endif

    initialized = true;

    //printf("Create RGBA FBO: %dx%d\n", (int) (s->w), (int)s->h);
    glGetError();
    glGenTextures(1, &texid);
    glBindTexture(GL_TEXTURE_2D, texid);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  (int) s->w, (int)s->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    GLenum errCode;
    if ((errCode = glGetError()) != GL_NO_ERROR)
    {
      CLog::Log(LOGERROR, "sBXSurfacePrivData::Initialize glTexImage2D failed %x", errCode);
      return;
    }

    bLoadedToGPU = false;
  }
} BXSurfacePrivData;

class FillRectJob : public IGUIThreadTask
{
public: 
  virtual void DoWork()
  {
    if (!surface || !surface->priv)
      return;

    glDisable(GL_SCISSOR_TEST);

#if defined(HAS_GL) || defined(HAS_GLES)
    bool bBlend = glIsEnabled(GL_BLEND);
    glViewport(0, 0, surface->w, surface->h);

#ifdef HAS_GL
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, surface->w, surface->h, 0.0, -1.0, 1.0);
#endif

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glDisable(GL_BLEND);

    BXSurfacePrivData *p = (BXSurfacePrivData *)surface->priv;
    if (!p->initialized)
      p->Initialize(surface);

    if (!p->initialized)
      return;

    GLenum errCode = glGetError();

    if (!g_fbo.BindToTexture(GL_TEXTURE_2D, p->texid))
    {
      CLog::Log(LOGERROR, "FillRectJob::BindToTexture failed");
    }

    g_fbo.BeginRender();
    
    if (blend == BX_BLEND_SOURCE_ALPHA)
    {
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_BLEND);
    }     

#ifdef HAS_GL
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
    glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
    glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
    glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
    
    glDisable(GL_TEXTURE_2D);
    
    glColor4f( ((float) ((c >> 16) & 0xff)) / 255.0, ((float) ((c >> 8) & 0xff)) / 255.0,((float) (c & 0xff)) / 255.0,
              ((float) ((c >> 24) & 0xff)) / 255.0);
    
    glRecti(rect.x, rect.y, rect.x + rect.w, (rect.y + rect.h));
#else
    g_ColorShaderProgram->Enable();
    GLuint progObj = g_ColorShaderProgram->ProgramHandle();

    // Set transformation matrixes
    TransformMatrix matProjection;
    matProjection.MatrixOrtho(0, surface->w, surface->h, 0.0, -1.0, 1.0);
    GLint uniMatProjection  = glGetUniformLocation(progObj, "u_matProjection");
    glUniformMatrix4fv(uniMatProjection, 1, GL_FALSE, (GLfloat *)matProjection.m);

    if(uniMatProjection == -1)
    {
      CLog::Log(LOGERROR, "Error uniMatProjection == -1");
      return;
    }

    TransformMatrix matModelView;
    GLint uniMatModelView  = glGetUniformLocation(progObj, "u_matModelView");
    glUniformMatrix4fv(uniMatModelView, 1, GL_FALSE, (GLfloat *)matModelView.m);

    if(uniMatModelView == -1)
    {
      CLog::Log(LOGERROR, "Error uniMatModelView == -1");
      return;
    }

    GLfloat diffuseColor[4];
    diffuseColor[0] = (GLfloat)GET_B(c) / 255.0f;
    diffuseColor[1] = (GLfloat)GET_G(c) / 255.0f;
    diffuseColor[2] = (GLfloat)GET_R(c) / 255.0f;
    diffuseColor[3] = ((GLfloat)GET_A(c) / 255.0f);

    GLint m_uniColor = glGetUniformLocation(progObj, "u_diffuseColor");
    glUniform4fv(m_uniColor, 1, diffuseColor);

    // Set vertices
    GLint attribVertex = glGetAttribLocation(progObj, "a_vertex");
    glEnableVertexAttribArray(attribVertex);

    ImageVertex v[4];
    v[0].x = rect.x;
    v[0].y = rect.y;
    v[0].z = 0;
    v[0].u1 = 0;
    v[0].v1 = 0;

    v[3].x = rect.x;
    v[3].y = rect.y + rect.h;
    v[3].z = 0;
    v[3].u1 = 0;
    v[3].v1 = 1;

    v[2].x = rect.x + rect.w;
    v[2].y = rect.y + rect.h;
    v[2].z = 0;
    v[2].u1 = 1;
    v[2].v1 = 1;

    v[1].x = rect.x + rect.w;
    v[1].y = rect.y;
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
    glDrawElements(GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_INT, indices);

    glDisableVertexAttribArray(attribVertex);
    g_ColorShaderProgram->Disable();
#endif
    
    g_fbo.EndRender();

    bBlend?glEnable(GL_BLEND):glDisable(GL_BLEND);
    
    glViewport(0, 0, g_graphicsContext.GetWidth(), g_graphicsContext.GetHeight());
#ifdef HAS_GL
    glOrtho(0, g_graphicsContext.GetWidth()-1, g_graphicsContext.GetHeight()-1, 0.0, -1.0, 1.0);
#endif
    
    if ((errCode = glGetError()) != GL_NO_ERROR)
    {
      CLog::Log(LOGERROR, "FillRectJob failed %x", errCode);
      return;
    }

#endif
  }  
  BX_Surface* surface; 
  BX_Color c; 
  BX_Rect rect; 
  BX_BlendMethod blend;
};

class BlitJob : public IGUIThreadTask
{
public: 
  BlitJob() : color(0xffffffff), alpha(255) {}

  virtual void DoWork()
  {
    GLenum errCode = glGetError();

    if (!destSurface || !destSurface->priv || !sourceSurface)
      return;
#if defined(HAS_GL) || defined(HAS_GLES)
    bool bBlend = glIsEnabled(GL_BLEND);

    if(alpha == 0)
      alpha = 255;

    glDisable(GL_SCISSOR_TEST);

    glViewport(0, 0, destSurface->w, destSurface->h);
#ifdef HAS_GL
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, destSurface->w, destSurface->h, 0.0, -1.0, 1.0);
#else
    glActiveTexture(GL_TEXTURE0);
#endif

#ifdef HAS_GL
    glEnable(GL_TEXTURE_2D);
#endif

    BXSurfacePrivData *srcPriv = (BXSurfacePrivData *)sourceSurface->priv;
    
    if (!srcPriv->initialized)
      srcPriv->Initialize(sourceSurface);

    if (!srcPriv->initialized)
      return;

    glBindTexture(GL_TEXTURE_2D, srcPriv->texid);
    if (sourceSurface->pixels && !srcPriv->bLoadedToGPU)
    {
      if (sourceSurface->bpp == 4)
      {
#ifdef __APPLE__
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, sourceSurface->w, sourceSurface->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, sourceSurface->pixels);
#else
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, sourceSurface->w, sourceSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, sourceSurface->pixels);
#endif
      }
      else
      {
        glTexImage2D (GL_TEXTURE_2D, 0, GL_ALPHA, sourceSurface->w, sourceSurface->h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, sourceSurface->pixels);
      }
      srcPriv->bLoadedToGPU = true;
    }
    
    BXSurfacePrivData *p = (BXSurfacePrivData *)destSurface->priv;
    if (!p->initialized)
      p->Initialize(destSurface);

    if (!p->initialized)
      return;

    if (!g_fbo.BindToTexture(GL_TEXTURE_2D, p->texid))
    {
      CLog::Log(LOGERROR, "BlitJob::BindToTexture failed");
    }

    g_fbo.BeginRender();
    
    glBindTexture(GL_TEXTURE_2D, srcPriv->texid);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glDisable(GL_BLEND);
    if (blend == BX_BLEND_SOURCE_ALPHA)
    {
      glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
      glEnable(GL_BLEND);
    }     
    
    float w = sourceSurface->w;
    float h = sourceSurface->h;
    float x1 = sourceRect.x;
    float y1 = sourceRect.y;
    float x2 = sourceRect.x + sourceRect.w;
    float y2 = sourceRect.y + sourceRect.h;

#ifdef HAS_GL
    if (sourceSurface->bpp == 1 && destSurface->bpp == 4)
    {  
      // Alpha texture rendering, need to take color from glColor
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
      glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
      glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PRIMARY_COLOR);
      glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
      glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
      glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE0);
      glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
      glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PRIMARY_COLOR);
      glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
      glColor3f(((float) ((color >> 16) & 0xff)) / 255.0, 
                ((float) ((color >> 8) & 0xff)) / 255.0, 
                ((float) (color & 0xff)) / 255.0);
    }
    else
    {
      // Regular texture rendering
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
      glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
      glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
      glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
      glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
      glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

      glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
      glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE0);
      glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
      glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PRIMARY_COLOR);
      glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);

      glColor4f(1.0f, 1.0f, 1.0f, alpha / 255.0f);
    }
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBegin(GL_QUADS);
    glTexCoord2f(x1/w, y1/h); glVertex3i(destRect.x, destRect.y, 0);
    glTexCoord2f(x2/w, y1/h); glVertex3i(destRect.x + destRect.w, destRect.y, 0);
    glTexCoord2f(x2/w, y2/h); glVertex3i(destRect.x + destRect.w,  (destRect.y + destRect.h), 0);
    glTexCoord2f(x1/w, y2/h); glVertex3i(destRect.x,  (destRect.y + destRect.h), 0);
    glEnd();

#else
    GLuint progObj;
    if (sourceSurface->bpp == 1 && destSurface->bpp == 4)
    {
      g_AlphaShaderProgram->Enable();
      progObj = g_AlphaShaderProgram->ProgramHandle();
    }
    else
    {
      g_TextureShaderProgram->Enable();
      progObj = g_TextureShaderProgram->ProgramHandle();
    }

    // Set transformation matrixes
    TransformMatrix matProjection;
    matProjection.MatrixOrtho(0, destSurface->w, destSurface->h, 0.0, -1.0, 1.0);
    GLint uniMatProjection  = glGetUniformLocation(progObj, "u_matProjection");
    glUniformMatrix4fv(uniMatProjection, 1, GL_FALSE, (GLfloat *)matProjection.m);

    if(uniMatProjection == -1)
    {
      CLog::Log(LOGERROR, "Error uniMatProjection == -1");
      return;
    }

    TransformMatrix matModelView;
    GLint uniMatModelView  = glGetUniformLocation(progObj, "u_matModelView");
    glUniformMatrix4fv(uniMatModelView, 1, GL_FALSE, (GLfloat *)matModelView.m);

    if(uniMatModelView == -1)
    {
      CLog::Log(LOGERROR, "Error uniMatModelView == -1");
      return;
    }

    GLfloat diffuseColor[4];

    if (sourceSurface->bpp == 1 && destSurface->bpp == 4)
    {
      diffuseColor[0] = (GLfloat)GET_B(color) / 255.0f;
      diffuseColor[1] = (GLfloat)GET_G(color) / 255.0f;
      diffuseColor[2] = (GLfloat)GET_R(color) / 255.0f;
      diffuseColor[3] = 1.0;
    }
    else
    {
      diffuseColor[0] = 1.0;
      diffuseColor[1] = 1.0;
      diffuseColor[2] = 1.0;
      diffuseColor[3] = alpha / 255.0f;
    }

    GLint m_uniColor = glGetUniformLocation(progObj, "u_diffuseColor");
    glUniform4fv(m_uniColor, 1, diffuseColor);

    GLint uniTexture0 = glGetUniformLocation(progObj, "u_texture0");
    GLint attribTextureCoord0 = glGetAttribLocation(progObj, "a_texCoord0");
    glEnableVertexAttribArray(attribTextureCoord0);
    glUniform1i(uniTexture0, 0);

    // Set vertices
    GLint attribVertex = glGetAttribLocation(progObj, "a_vertex");
    glEnableVertexAttribArray(attribVertex);

    ImageVertex v[4];
    v[0].x = destRect.x;
    v[0].y = destRect.y;
    v[0].z = 0;
    v[0].u1 = x1/w;
    v[0].v1 = y1/h;

    v[3].x = destRect.x;
    v[3].y = destRect.y + destRect.h;
    v[3].z = 0;
    v[3].u1 = x1/w;
    v[3].v1 = y2/h;

    v[2].x = destRect.x + destRect.w;
    v[2].y = destRect.y + destRect.h;
    v[2].z = 0;
    v[2].u1 = x2/w;
    v[2].v1 = y2/h;

    v[1].x = destRect.x + destRect.w;
    v[1].y = destRect.y;
    v[1].z = 0;
    v[1].u1 = x2/w;
    v[1].v1 = y1/h;

    glVertexAttribPointer(attribVertex, 3, GL_FLOAT, GL_FALSE, sizeof(ImageVertex), (char*)v + offsetof(ImageVertex, x));
    glVertexAttribPointer(attribTextureCoord0, 2, GL_FLOAT, GL_FALSE, sizeof(ImageVertex), (char*)v + offsetof(ImageVertex, u1));

    // Draw
    GLuint indices[6] =
    {
      0, 1, 2,
      2, 3, 0,
    };
    glDrawElements(GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_INT, indices);

    glDisableVertexAttribArray(attribVertex);
    glDisableVertexAttribArray(attribTextureCoord0);
    if (sourceSurface->bpp == 1 && destSurface->bpp == 4)
      g_AlphaShaderProgram->Disable();
    else
      g_TextureShaderProgram->Disable();
#endif
    
    g_fbo.EndRender();
    
    bBlend ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);
#ifdef HAS_GL
    glDisable(GL_TEXTURE_2D);
#endif

    glViewport(0, 0, g_graphicsContext.GetWidth(), g_graphicsContext.GetHeight());
#ifdef HAS_GL
    glOrtho(0, g_graphicsContext.GetWidth()-1, g_graphicsContext.GetHeight()-1, 0.0, -1.0, 1.0);
#endif
    
    if ((errCode = glGetError()) != GL_NO_ERROR)
    {
      CLog::Log(LOGERROR, "BlitJob failed %x", errCode);
      return;
    }

#endif
  }  

  BX_Surface* sourceSurface;
  BX_Surface* destSurface;
  BX_Rect sourceRect;
  BX_Rect destRect;
  BX_BlendMethod blend;
  BX_Color color;
  unsigned char alpha;
};

class FlipJob : public IGUIThreadTask
{
public:
  FlipJob(BX_Surface *s) : surface(s) {}
  virtual void DoWork()
  {

  }

  BX_Surface* surface;
};

class CreateSurfaceJob : public IGUIThreadTask
{
public: 
  CreateSurfaceJob(BX_Handle app, BX_PixelFormat pFormat, unsigned int w, unsigned int h) : 
      hApp(app), pixelFormat(pFormat), nWidth(w), nHeight(h), surface(NULL) { }
  
  virtual void DoWork()
  {
    BX_Surface *s = new BX_Surface;
    s->hApp = hApp;
    s->pixelFormat = pixelFormat;
    s->w = nWidth;
    s->h = nHeight;
    
    if (pixelFormat == BX_PF_BGRA8888)
    {
      s->bpp = 4;
    }
    else
    {
      s->bpp = 1;
    }
    
    s->pitch = s->w * s->bpp ;
    s->pixels = NULL;
    
    BXSurfacePrivData *p = new BXSurfacePrivData;
    p->app = (BOXEE::NativeApplication*)hApp->boxeeData;
    
    s->priv = p;    
    surface = s;
  }  
  
  BX_Handle      hApp; 
  BX_PixelFormat pixelFormat;
  unsigned int   nWidth; 
  unsigned int   nHeight;
  BX_Surface     *surface;
};

class FreeSurfaceJob : public IGUIThreadTask
{
public: 
  FreeSurfaceJob(BX_Surface* s) : surface(s) { }
  virtual void DoWork()
  {

    if (surface)
    {
      if (surface->priv)
      {
        BXSurfacePrivData *priv = (BXSurfacePrivData *)surface->priv;
        delete priv;
      }
      
      if (surface->pixels)
        delete [] surface->pixels;
      
      delete surface;
    }
  }  
  BX_Surface* surface;
};

class LockSurfaceJob : public IGUIThreadTask
{
public: 
  LockSurfaceJob(BX_Surface* s, bool bReadPixels=false) : surface(s), readPixels(bReadPixels) { }
  virtual void DoWork()
  {
    if (surface)
    {
      ((BOXEE::NativeApplication *)(surface->hApp->boxeeData))->ExecuteRenderOperations();

      BXSurfacePrivData *p = (BXSurfacePrivData *)surface->priv;
      if (!p->initialized)
        p->Initialize(surface);
      
      if (!p->initialized)
        return;

      if (!surface->pixels)
      {
        surface->pixels = new unsigned char[surface->h * surface->pitch];
      }

#ifdef HAS_GL
      if (readPixels)
      {
        glReadBuffer((GLenum)GL_COLOR_ATTACHMENT0);
        glReadPixels(0, 0, surface->w, surface->h, (surface->pixelFormat == BX_PF_BGRA8888)?GL_RGBA:GL_ALPHA, GL_UNSIGNED_BYTE, surface->pixels);
      }
#endif
    }
  }  
  BX_Surface* surface;
  bool        readPixels;
};

class UnlockSurfaceJob : public IGUIThreadTask
{
public: 
  UnlockSurfaceJob(BX_Surface* s) : surface(s) { }
  virtual void DoWork()
  {
    if (surface && surface->pixels)
    {
      BXSurfacePrivData *p = (BXSurfacePrivData *)surface->priv;
      if (!p->initialized)
        p->Initialize(surface);

      if (!p->initialized)
        return;

#ifdef HAS_GL
      glEnable(GL_TEXTURE_2D);
#endif
      glBindTexture(GL_TEXTURE_2D, p->texid);

      if (surface->bpp == 4)
      {
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
      }
      else
      {
        glTexImage2D (GL_TEXTURE_2D, 0, GL_ALPHA, surface->w, surface->h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, surface->pixels);
      }    
      
      glBindTexture(GL_TEXTURE_2D, 0);
    }
  }  
  BX_Surface* surface;
};

class ClearSurfaceJob : public IGUIThreadTask
{
public:
  ClearSurfaceJob(BX_Surface* s, float a=1.0) : surface(s), alpha(a) { }
  virtual void DoWork()
  {
    BXSurfacePrivData *srcPriv = (BXSurfacePrivData *)surface->priv;
    if (!srcPriv->initialized)
      srcPriv->Initialize(surface);
    
    if (!srcPriv->initialized)
      return;

    if (!g_fbo.BindToTexture(GL_TEXTURE_2D, srcPriv->texid))
    {
      CLog::Log(LOGERROR, "ClearSurfaceJob::BindToTexture failed");
    }

    g_fbo.BeginRender();
    glClearColor(0.0, 0.0, 0.0, alpha);
    glClear(GL_COLOR_BUFFER_BIT);
    g_fbo.EndRender();
  }
protected:
  BX_Surface* surface;  
  float       alpha;
};

class NativeAppRenderToScreenHelper
{
public:
  NativeAppRenderToScreenHelper(BX_Surface* s) : surface(s)
  {
  }

  static void LoadShaders()
  {
    if (g_TextureShaderProgram != NULL && g_ColorShaderProgram != NULL && g_AlphaShaderProgram != NULL)
      return;

    g_fbo.Initialize();

    g_TextureShaderProgram = new Shaders::CGLSLShaderProgram();
    g_TextureShaderProgram->VertexShader()->SetSource(g_TextureVertexShader);
    g_TextureShaderProgram->PixelShader()->SetSource(g_TexturePixelShader);
    g_TextureShaderProgram->CompileAndLink();

    g_ColorShaderProgram = new Shaders::CGLSLShaderProgram();
    g_ColorShaderProgram->VertexShader()->SetSource(g_TextureVertexShader);
    g_ColorShaderProgram->PixelShader()->SetSource(g_ColorPixelShader);
    g_ColorShaderProgram->CompileAndLink();

    g_AlphaShaderProgram = new Shaders::CGLSLShaderProgram();
    g_AlphaShaderProgram->VertexShader()->SetSource(g_TextureVertexShader);
    g_AlphaShaderProgram->PixelShader()->SetSource(g_AlphaPixelShader);
    g_AlphaShaderProgram->CompileAndLink();
  }

  void Render()
  {
    GLenum errCode = glGetError();

    BXSurfacePrivData *srcPriv = (BXSurfacePrivData *)surface->priv;
    if (!srcPriv->initialized)
      srcPriv->Initialize(surface);

    RESOLUTION iRes = g_graphicsContext.GetVideoResolution();
    float offsetX = g_settings.m_ResInfo[iRes].Overscan.left;
    float offsetY = g_settings.m_ResInfo[iRes].Overscan.top;
    float width   = g_settings.m_ResInfo[iRes].Overscan.right - g_settings.m_ResInfo[iRes].Overscan.left;
    float height  = g_settings.m_ResInfo[iRes].Overscan.bottom - g_settings.m_ResInfo[iRes].Overscan.top;
    glViewport((GLint) offsetX, (GLint) offsetY, (GLint) width, (GLint) height);
    
#ifdef HAS_GL
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0.0, -1.0, 1.0);
#else
    glActiveTexture(GL_TEXTURE0);
#endif

    glDisable(GL_SCISSOR_TEST);

#ifdef HAS_GL
    glEnable(GL_TEXTURE_2D);
#endif
    glBindTexture(GL_TEXTURE_2D, srcPriv->texid);
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    glDisable(GL_BLEND);
    if (g_application.IsPlayingVideo())
    {
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_BLEND);
    }
    
    float w = width;
    float h = height;
    float x2 = w;
    float y2 = h;

#ifdef HAS_GL
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
    glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
    glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
    glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

    glColor4f(1.0, 1.0, 1.0, 1.0);

    glBegin(GL_QUADS);
    glTexCoord2f(x2/w, y2/h); glVertex3i(0, 0, 0);  // TODO: Is x1<->x2 OK here? Is y1<->y2 OK here?
    glTexCoord2f(x2/w, y2/h); glVertex3i(x2, 0, 0);   // TODO: Is y1<->y2 OK here?
    glTexCoord2f(x2/w, y2/h); glVertex3i(x2, y2, 0);
    glTexCoord2f(x2/w, y2/h); glVertex3i(0, y2, 0); // TODO: Is x1<->x2 OK here?
    glEnd();
    
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#else
    g_TextureShaderProgram->Enable();
    GLuint progObj = g_TextureShaderProgram->ProgramHandle();


    // Set transformation matrixes
    TransformMatrix matProjection;
    matProjection.MatrixOrtho(0, width, height, 0.0, -1.0, 1.0);
    GLint uniMatProjection  = glGetUniformLocation(progObj, "u_matProjection");
    glUniformMatrix4fv(uniMatProjection, 1, GL_FALSE, (GLfloat *)matProjection.m);

    TransformMatrix matModelView;
    GLint uniMatModelView  = glGetUniformLocation(progObj, "u_matModelView");
    glUniformMatrix4fv(uniMatModelView, 1, GL_FALSE, (GLfloat *)matModelView.m);

    GLfloat diffuseColor[4];
    diffuseColor[0] = 1.0;
    diffuseColor[1] = 1.0;
    diffuseColor[2] = 1.0;
    diffuseColor[3] = 1.0;

    GLint m_uniColor = glGetUniformLocation(progObj, "u_diffuseColor");
    glUniform4fv(m_uniColor, 1, diffuseColor);

    GLint uniTexture0 = glGetUniformLocation(progObj, "u_texture0");
    GLint attribTextureCoord0 = glGetAttribLocation(progObj, "a_texCoord0");
    glEnableVertexAttribArray(attribTextureCoord0);
    glUniform1i(uniTexture0, 0);

    // Set vertices
    GLint attribVertex = glGetAttribLocation(progObj, "a_vertex");
    glEnableVertexAttribArray(attribVertex);

    ImageVertex v[4];
    v[0].x = 0;
    v[0].y = 0;
    v[0].z = 0;
    v[0].u1 = 0;
    v[0].v1 = 0;

    v[3].x = 0;
    v[3].y = y2;
    v[3].z = 0;
    v[3].u1 = 0;
    v[3].v1 = 1;

    v[2].x = x2;
    v[2].y = y2;
    v[2].z = 0;
    v[2].u1 = 1;
    v[2].v1 = 1;

    v[1].x = x2;
    v[1].y = 0;
    v[1].z = 0;
    v[1].u1 = 1;
    v[1].v1 = 0;

    glVertexAttribPointer(attribVertex, 3, GL_FLOAT, GL_FALSE, sizeof(ImageVertex), (char*)v + offsetof(ImageVertex, x));
    glVertexAttribPointer(attribTextureCoord0, 2, GL_FLOAT, GL_FALSE, sizeof(ImageVertex), (char*)v + offsetof(ImageVertex, u1));

    // Draw
    GLuint indices[6] =
    {
      0, 1, 2,
      2, 3, 0,
    };
    glDrawElements(GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_INT, indices);

    glDisableVertexAttribArray(attribVertex);
    glDisableVertexAttribArray(attribTextureCoord0);
    g_TextureShaderProgram->Disable();
#endif

    glBindTexture(GL_TEXTURE_2D, 0);
    
#ifdef HAS_GL
    glDisable(GL_TEXTURE_2D);
#endif

    if ((errCode = glGetError()) != GL_NO_ERROR)
    {
      CLog::Log(LOGERROR, "NativeAppRenderToScreenHelper failed %x", errCode);
      return;
    }
  }

protected:
  BX_Surface* surface;  
};

#endif

