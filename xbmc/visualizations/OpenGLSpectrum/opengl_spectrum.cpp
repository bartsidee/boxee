/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 *  Wed May 24 10:49:37 CDT 2000
 *  Fixes to threading/context creation for the nVidia X4 drivers by 
 *  Christian Zander <phoenix@minion.de>
 */

/*
 *  Ported to XBMC by d4rk
 *  Also added 'hSpeed' to animate transition between bar heights
 */

#ifdef HAS_GLES
#include <GLES2/gl2.h>
#endif

#include "../../../visualisations/xbmc_vis.h"
#include <math.h>
#include "TransformMatrix.h"

#ifdef HAS_GL 
#include <GL/glew.h>
#endif

#ifdef _WIN32
#pragma comment (lib,"glew32.lib")
#endif


#if (HAS_GLES)
#if defined(_WIN32)
#pragma comment (lib,"libEGL.lib")
#pragma comment (lib,"libGLESv2.lib")
#endif
#define GL_FILL 1
#define GL_POINT 2
#define GL_LINE 3
#endif

#define NUM_BANDS 16

#if defined(HAS_GL) && (HAS_GL<20)
#define HAS_GL_1
#endif

#if defined(HAS_GL) && (HAS_GL>=20)
#define HAS_GL_2
#endif


#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif
#define DEG_TO_RAD(x) (M_PI * x) / 180

GLfloat y_angle = 45.0, y_speed = 0.5;
GLfloat x_angle = 20.0, x_speed = 0.0;
GLfloat z_angle = 0.0, z_speed = 0.0;
GLfloat heights[16][16], cHeights[16][16], scale;
GLfloat hSpeed = 0.05f;
GLenum  g_mode = GL_FILL;
vector<VisSetting> g_vecSettings;

GLuint g_indices[6] = 
{
  0, 1, 2,
  3, 4, 5,
};

GLuint g_programObject;
GLfloat g_vertex[3 * 6];
GLfloat g_color[4] = {1.0, 1.0, 1.0, 1.0} ;
GLfloat g_colors[4 * 6];

GLint  g_uniMatProjection = -1;
GLint  g_uniMatModelView  = -1;

GLint g_attribVertex = -1;
GLint g_attribColor = -1;

GLboolean g_bShaderLoaded = false;

char g_vShaderStr[] =
  "attribute vec3 a_vertex; \n"
  "attribute vec4 a_color; \n"
  "varying vec4 v_colorVar; \n"
  "uniform mat4 u_matModelView; \n"
  "uniform mat4 u_matProjection; \n"
  "void main() \n"
  "{ \n"
  " v_colorVar	= a_color; \n"
  " gl_Position	= u_matProjection * u_matModelView * vec4(a_vertex,1.0); \n"
  "} \n";

char g_fShaderStr[] =
  "precision mediump float; \n"
  "varying vec4 v_colorVar; \n"
  "void main() \n"
  "{ \n"
  "  gl_FragColor = v_colorVar; \n"
  "} \n";

extern "C" void Create(void* pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName,
                       float fPixelRatio, const char *szSubModuleName)
{
  g_vecSettings.clear();
  m_uiVisElements = 0;
  VisSetting scale(VisSetting::SPIN, "Bar Height");
  scale.AddEntry("Default");
  scale.AddEntry("Big");
  scale.AddEntry("Very Big");
  scale.AddEntry("Small");

  VisSetting mode(VisSetting::SPIN, "Mode");
  mode.AddEntry("Default");
  mode.AddEntry("Wireframe");
  mode.AddEntry("Points");

  VisSetting speed(VisSetting::SPIN, "Speed");
  speed.AddEntry("Default");
  speed.AddEntry("Slow");
  speed.AddEntry("Very Slow");
  speed.AddEntry("Fast");
  speed.AddEntry("Very Fast");

  g_vecSettings.push_back( scale );
  g_vecSettings.push_back( mode );
  g_vecSettings.push_back( speed );
}

///
// Create a shader object, load the shader source, and
// compile the shader.
//
#if defined(HAS_GLES) || defined(HAS_GL_2)

GLuint LoadShader(const char *shaderSrc, GLenum type)
{
  GLuint shader;
  GLint compiled;
  // Create the shader object
  shader = glCreateShader(type);
  if(shader == 0)
    return 0;
  // Load the shader source
  glShaderSource(shader, 1, &shaderSrc, NULL);
  // Compile the shader
  glCompileShader(shader);
  // Check the compile status
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

  if(!compiled)
  {
    GLint infoLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
    if(infoLen > 1)
    {
      char* infoLog = (char *)malloc(sizeof(char) * infoLen);
      glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
      free(infoLog);
    }
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

int InitShaders()
{
  if(g_bShaderLoaded)
    return true;

  GLuint vertexShader;
  GLuint fragmentShader;
  GLint linked;

  // Load the vertex/fragment shaders
  vertexShader = LoadShader((char *)g_vShaderStr, GL_VERTEX_SHADER);
  fragmentShader = LoadShader((char *)g_fShaderStr, GL_FRAGMENT_SHADER);
  // Create the program object
  g_programObject = glCreateProgram();
  if(g_programObject == 0)
    return 0;
  glAttachShader(g_programObject, vertexShader);
  glAttachShader(g_programObject, fragmentShader);
  // Link the program
  glLinkProgram(g_programObject);
  // Check the link status
 
#ifdef _WIN32
  glGetProgramiv(g_programObject, GL_LINK_STATUS, &linked);
  if(!linked)
  {
    GLint infoLen = 0;
    glGetProgramiv(g_programObject, GL_INFO_LOG_LENGTH, &infoLen);
    if(infoLen > 1)
    {
      char* infoLog = (char *)malloc(sizeof(char) * infoLen);
      glGetProgramInfoLog(g_programObject, infoLen, NULL, infoLog);
      free(infoLog);
    }
    glDeleteProgram(g_programObject);
    return FALSE;
  }
#endif
 
  g_bShaderLoaded = true;  

  return TRUE;
}

#endif // GLES | GL20

void draw_rectangle(GLfloat x1, GLfloat y1, GLfloat z1, GLfloat x2, GLfloat y2, GLfloat z2)
{
#if defined(HAS_GL_1)
  if(y1 == y2)
  {
    glVertex3f(x1, y1, z1);
    glVertex3f(x2, y1, z1);
    glVertex3f(x2, y2, z2);

    glVertex3f(x2, y2, z2);
    glVertex3f(x1, y2, z2);
    glVertex3f(x1, y1, z1);
  }
  else
  {
    glVertex3f(x1, y1, z1);
    glVertex3f(x2, y1, z2);
    glVertex3f(x2, y2, z2);

    glVertex3f(x2, y2, z2);
    glVertex3f(x1, y2, z1);
    glVertex3f(x1, y1, z1);
  }
#elif( defined(HAS_GLES) || defined(HAS_GL_2))

  if(y1 == y2)
  {
    g_vertex[0] = x1; g_vertex[1] = y1; g_vertex[2] = z1;
    g_vertex[3] = x2; g_vertex[4] = y1; g_vertex[5] = z1;
    g_vertex[6] = x2; g_vertex[7] = y2; g_vertex[8] = z2;

    g_vertex[9] = x2; g_vertex[10] = y2; g_vertex[11] = z2;
    g_vertex[12] = x1; g_vertex[13] = y2; g_vertex[14] = z2;
    g_vertex[15] = x1; g_vertex[16] = y1; g_vertex[17] = z1;

  }
  else
  {
    g_vertex[0] = x1; g_vertex[1] = y1; g_vertex[2] = z1;
    g_vertex[3] = x2; g_vertex[4] = y1; g_vertex[5] = z2;
    g_vertex[6] = x2; g_vertex[7] = y2; g_vertex[8] = z2;

    g_vertex[9] = x2; g_vertex[10] = y2; g_vertex[11] = z2;
    g_vertex[12] = x1; g_vertex[13] = y2; g_vertex[14] = z1;
    g_vertex[15] = x1; g_vertex[16] = y1; g_vertex[17] = z1;
  }

  for(int i = 0; i < 6; i++)
    for(int j = 0; j < 4; j++)
      g_colors[i * 4 + j] = g_color[j];

  glVertexAttribPointer(g_attribVertex, 3, GL_FLOAT, GL_FALSE, 0, g_vertex);
  glVertexAttribPointer(g_attribColor, 4, GL_FLOAT, GL_FALSE, 0, g_colors);

  glDrawElements( GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_INT, g_indices); 
#endif
}

void draw_bar(GLfloat x_offset, GLfloat z_offset, GLfloat height, GLfloat red, GLfloat green, GLfloat blue )
{
  GLfloat width = 0.1f;

#if defined(HAS_GL_1)
  if (g_mode == GL_POINT)
    glColor3f(0.2, 1.0, 0.2);

  if (g_mode != GL_POINT)
  {
    glColor3f(red,green,blue);
    draw_rectangle(x_offset, height, z_offset, x_offset + width, height, z_offset + 0.1);
  }
  draw_rectangle(x_offset, 0, z_offset, x_offset + width, 0, z_offset + 0.1);

  if (g_mode != GL_POINT)
  {
    glColor3f(0.5 * red, 0.5 * green, 0.5 * blue);
    draw_rectangle(x_offset, 0.0, z_offset + 0.1, x_offset + width, height, z_offset + 0.1);
  }
  draw_rectangle(x_offset, 0.0, z_offset, x_offset + width, height, z_offset );

  if (g_mode != GL_POINT)
  {
    glColor3f(0.25 * red, 0.25 * green, 0.25 * blue);
    draw_rectangle(x_offset, 0.0, z_offset , x_offset, height, z_offset + 0.1);
  }
  draw_rectangle(x_offset + width, 0.0, z_offset , x_offset + width, height, z_offset + 0.1);
#elif defined(HAS_GLES) || defined(HAS_GL_2)
  if (g_mode == GL_POINT)
    g_color[0] = 0.2f; g_color[1] = 1.0f; g_color[2] = 0.2f;

  if (g_mode != GL_POINT)
  {
    g_color[0] = red; g_color[1] = green; g_color[2] = blue;
    draw_rectangle(x_offset, height, z_offset, x_offset + width, height, z_offset + 0.1f);
  }
  draw_rectangle(x_offset, 0.0f, z_offset, x_offset + width, 0, z_offset + 0.1f);

  if (g_mode != GL_POINT)
  {
    g_color[0] = 0.5f * red; g_color[1] = 0.5f * green; g_color[2] = 0.5f * blue;
    draw_rectangle(x_offset, 0.0f, z_offset + 0.1f, x_offset + width, height, z_offset + 0.1f);
  }
  draw_rectangle(x_offset, 0.0, z_offset, x_offset + width, height, z_offset );

  if (g_mode != GL_POINT)
  {
    g_color[0] = 0.25f * red; g_color[1] = 0.25f * green; g_color[2] = 0.2f * blue;
    draw_rectangle(x_offset, 0.0, z_offset , x_offset, height, z_offset + 0.1f);
  }
  draw_rectangle(x_offset + width, 0.0, z_offset , x_offset + width, height, z_offset + 0.1f);
  

#endif
}

void draw_bars(void)
{
  int x,y;
  GLfloat x_offset, z_offset, r_base, b_base;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#if defined(HAS_GL_1)
  glPushMatrix();
  glTranslatef(0.0,-0.5,-5.0);
  glRotatef(x_angle,1.0,0.0,0.0);
  glRotatef(y_angle,0.0,1.0,0.0);
  glRotatef(z_angle,0.0,0.0,1.0);
  glPolygonMode(GL_FRONT_AND_BACK, g_mode);
  glBegin(GL_TRIANGLES);
#endif

#if defined(HAS_GLES) || defined(HAS_GL_2)
  TransformMatrix matModel, matTrans, matRotX, matRotY, matRotZ;
  matTrans.SetTranslation(0.0,-0.5,-5.0);
  matRotX.SetXRotation(DEG_TO_RAD(x_angle), 0.0f, 0.0f);
  matRotY.SetYRotation(DEG_TO_RAD(y_angle), 0.0f, 0.0f);
  matRotZ.SetZRotation(DEG_TO_RAD(z_angle), 0.0f, 0.0f);
  matModel = matTrans * matRotX * matRotY * matRotZ;
  matModel.Transpose(); // a bug in the GLES emu in win32, transpose doesn't work
  glUniformMatrix4fv(g_uniMatModelView, 1, GL_FALSE, (float *)matModel.m);
#endif

  for(y = 0; y < 16; y++)
  {
    z_offset = -1.6 + ((15 - y) * 0.2);

    b_base = y * (1.0 / 15);
    r_base = 1.0 - b_base;

    for(x = 0; x < 16; x++)
    {
      x_offset = -1.6 + (x * 0.2);
      if (::fabs(cHeights[y][x]-heights[y][x])>hSpeed)
      {
        if (cHeights[y][x]<heights[y][x])
          cHeights[y][x] += hSpeed;
        else
          cHeights[y][x] -= hSpeed;
      }
      draw_bar(x_offset, z_offset,
               cHeights[y][x], r_base - (x * (r_base / 15.0)),
               x * (1.0 / 15), b_base);
    }
  }
#if defined(HAS_GL_1)
  glEnd();
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glPopMatrix();
#endif
}

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
extern "C" void Render()
{
  bool configured = true; //FALSE;

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glDisable(GL_BLEND);

#if defined(HAS_GL_1)
  glDisable(GL_BLEND);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glFrustum(-1, 1, -1, 1, 1.5, 10);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glPolygonMode(GL_FRONT, GL_FILL);
#endif

#if defined(HAS_GLES) || defined(HAS_GL_2)
  if(!InitShaders())
    return;

  glUseProgram(g_programObject);

  g_uniMatProjection  = glGetUniformLocation(g_programObject, "u_matProjection");
  g_uniMatModelView  = glGetUniformLocation(g_programObject, "u_matModelView");

  g_attribVertex = glGetAttribLocation(g_programObject, "a_vertex");
  g_attribColor = glGetAttribLocation(g_programObject, "a_color");

  glEnableVertexAttribArray(g_attribVertex); 
  glEnableVertexAttribArray(g_attribColor);

  TransformMatrix matProj, matModelView;
  matProj.MatrixFrustum(-1, 1, -1, 1, 1.5, 10);
  glUniformMatrix4fv(g_uniMatProjection, 1, GL_FALSE, (float *)matProj.m);
  glUniformMatrix4fv(g_uniMatModelView, 1, GL_FALSE, (float *)matModelView.m);
#endif

  if(configured)
  {
    x_angle += x_speed;
    if(x_angle >= 360.0)
      x_angle -= 360.0;

    y_angle += y_speed;
    if(y_angle >= 360.0)
      y_angle -= 360.0;

    z_angle += z_speed;
    if(z_angle >= 360.0)
      z_angle -= 360.0;

    draw_bars();
  }

#if defined(HAS_GL_1)
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
#endif
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
}

extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{
  int x, y;

  for(x = 0; x < 16; x++)
  {
    for(y = 0; y < 16; y++)
    {
      cHeights[y][x] = 0.0;
    }
  }

  scale = 1.0f / log(256.0f);

  x_speed = 0.0f;
  y_speed = 0.5f;
  z_speed = 0.0f;
  x_angle = 20.0f;
  y_angle = 45.0f;
  z_angle = 0.0f;
}

extern "C" void Stop()
{

}

extern "C" void AudioData(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  int i,c;
  int y=0;
  GLfloat val;

  int xscale[] = {0, 1, 2, 3, 5, 7, 10, 14, 20, 28, 40, 54, 74, 101, 137, 187, 255};

  for(y = 15; y > 0; y--)
  {
    for(i = 0; i < 16; i++)
    {
      heights[y][i] = heights[y - 1][i];
    }
  }

  for(i = 0; i < NUM_BANDS; i++)
  {
    for(c = xscale[i], y = 0; c < xscale[i + 1]; c++)
    {
      if (c<iAudioDataLength)
      {
        if(pAudioData[c] > y)
          y = (int)pAudioData[c];
      }
      else
        continue;
    }
    y >>= 7;
    if(y > 0)
      val = (logf(y) * scale);
    else
      val = 0;
    heights[0][i] = val;
  }
}


//-- GetInfo ------------------------------------------------------------------
// Tell XBMC our requirements
//-----------------------------------------------------------------------------
extern "C" void GetInfo(VIS_INFO* pInfo)
{
  pInfo->bWantsFreq = false;
  pInfo->iSyncDelay = 0;
}


//-- OnAction -----------------------------------------------------------------
// Handle XBMC actions such as next preset, lock preset, album art changed etc
//-----------------------------------------------------------------------------
extern "C" bool OnAction(long flags, void *param)
{
  bool ret = false;
  return ret;
}

//-- GetPresets ---------------------------------------------------------------
// Return a list of presets to XBMC for display
//-----------------------------------------------------------------------------
extern "C" void GetPresets(char ***pPresets, int *currentPreset, int *numPresets, bool *locked)
{

}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
//-----------------------------------------------------------------------------
extern "C" unsigned int GetSettings(StructSetting*** sSet)
{ 
  m_uiVisElements = VisUtils::VecToStruct(g_vecSettings, &m_structSettings);
  *sSet = m_structSettings;
  return m_uiVisElements;
}

extern "C" void FreeSettings()
{
  VisUtils::FreeStruct(m_uiVisElements, &m_structSettings);
}

//-- UpdateSetting ------------------------------------------------------------
// Handle setting change request from XBMC
//-----------------------------------------------------------------------------
extern "C" void UpdateSetting(int num, StructSetting*** sSet)
{
  VisUtils::StructToVec(m_uiVisElements, sSet, &g_vecSettings);

  if ( (int)g_vecSettings.size() <= num || num < 0 )
    return;

  if (strcmp(g_vecSettings[num].name, "Size")==0)
  {
    switch (g_vecSettings[num].current)
    {
    case 0:
      scale = 1.0f / log(256.0f);
      break;

    case 1:
      scale = 2.0f / log(256.0f);
      break;

    case 2:
      scale = 3.0f / log(256.0f);
      break;

    case 3:
      scale = 0.5f / log(256.0f);
      break;

    case 4:
      scale = 0.33f / log(256.0f);
      break;
    }
  }

  if (strcmp(g_vecSettings[num].name, "Speed")==0)
  {
    switch (g_vecSettings[num].current)
    {
    case 0:
      hSpeed = 0.05f;
      break;

    case 1:
      hSpeed = 0.025f;
      break;

    case 2:
      hSpeed = 0.0125f;
      break;

    case 3:
      hSpeed = 0.10f;
      break;

    case 4:
      hSpeed = 0.20f;
      break;
    }
  }

  if (strcmp(g_vecSettings[num].name, "Mode")==0)
  {
    switch (g_vecSettings[num].current)
    {
    case 0:
      g_mode = GL_FILL;
      break;

    case 1:
      g_mode = GL_LINE;
      break;

    case 2:
      g_mode = GL_POINT;
      break;
    }
  }
}

//-- GetSubModules ------------------------------------------------------------
// Return any sub modules supported by this vis
//-----------------------------------------------------------------------------
extern "C" int GetSubModules(char ***names, char ***paths)
{
  return 0; // this vis supports 0 sub modules
}
