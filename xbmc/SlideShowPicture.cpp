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
#include "SlideShowPicture.h"
#include "Texture.h"
#include "cores/ssrc.h"         // for M_PI
#include "utils/GUIInfoManager.h"
#include "AdvancedSettings.h"
#include "Settings.h"
#include "GUISettings.h"
#include "system.h"
#include "WindowingFactory.h"
#include "utils/log.h"
#include "utils/SingleLock.h"
#include "Shader.h"
#include "Application.h"

#if defined(HAS_GL) || defined(HAS_GLES)
using namespace Shaders;
#endif
using namespace std;

#define IMMEDIATE_TRANSISTION_TIME          20

#define PICTURE_MOVE_AMOUNT              0.02f
#define PICTURE_MOVE_AMOUNT_ANALOG       0.01f
#define PICTURE_VIEW_BOX_COLOR      0xffffff00 // YELLOW
#define PICTURE_VIEW_BOX_BACKGROUND 0xff000000 // BLACK

#define FPS                                 25

#if defined(HAS_GLES) || defined(HAS_GL) 

static std::string g_TextureVertexShader =
"uniform mat4 u_matModelView;\n"
"uniform mat4 u_matProjection;\n"
"// Input vertex parameters\n"
"attribute vec3 a_vertex;\n"
"attribute vec2 a_texCoord0;\n"
"varying vec2 v_texCoord0;\n"
"void main() \n"
"{\n"
"v_texCoord0 = a_texCoord0;\n"
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
#if defined (HAS_GLES) && defined(_WIN32)
"gl_FragColor.rgba = texture0Color.bgra;\n"
#else
"gl_FragColor = texture0Color;\n"
#endif
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
#if defined (HAS_GLES) && defined(_WIN32)
"gl_FragColor.rgba = texture0Color.bgra;\n"
#else
"gl_FragColor = texture0Color;\n"
#endif
"}\n";

static std::string g_NoTexturePixelShader = 
#ifndef HAS_GL2
"precision mediump float;\n"
#endif
"varying vec2 v_texCoord0;\n"
"uniform vec4 u_diffuseColor;\n"
"uniform sampler2D u_texture0;\n"
"void main()\n"
"{\n"
"vec4 texture0Color;\n"
"texture0Color = u_diffuseColor;\n"
#if defined (HAS_GLES) && defined(_WIN32)
"gl_FragColor.rgba = texture0Color.bgra;\n"
#else
"gl_FragColor = texture0Color;\n"
#endif
"}\n";

static Shaders::CGLSLShaderProgram* g_pTextureShadersProgram;
static Shaders::CGLSLShaderProgram* g_pNoTextureShadersProgram;
static Shaders::CGLSLShaderProgram* g_pTextureShadersProgramNoDiffuse;

#endif

static float zoomamount[10] = { 1.0f, 1.2f, 1.5f, 2.0f, 2.8f, 4.0f, 6.0f, 9.0f, 13.5f, 20.0f };

CSlideShowPic::CSlideShowPic()
{
  m_pImage = NULL;
  m_bIsLoaded = false;
  m_bIsFinished = false;
  m_bDrawNextImage = false;
  m_bTransistionImmediately = false;
  m_bScreenSaverMode = false;
}

CSlideShowPic::~CSlideShowPic()
{
  Close();
}

void CSlideShowPic::Close()
{
  if (!g_application.IsCurrentThread())
  {
    // must close from main thread because textures are getting deleted
    ThreadMessage tMsg(TMSG_CLOSE_SLIDESHOWPIC);
    tMsg.lpVoid = this;
    g_application.getApplicationMessenger().SendMessage(tMsg, true);      
  }
  else
  {
  CSingleLock lock(m_textureAccess);
  if (m_pImage)
  {
    delete m_pImage;
    m_pImage = NULL;
  }
  m_bIsLoaded = false;
  m_bIsFinished = false;
  m_bDrawNextImage = false;
  m_bTransistionImmediately = false;
}
}

void CSlideShowPic::SetTexture(int iSlideNumber, CBaseTexture* pTexture, DISPLAY_EFFECT dispEffect, TRANSISTION_EFFECT transEffect)
{
  Close();
  CSingleLock lock(m_textureAccess);
  m_bPause = false;
  m_bNoEffect = false;
  m_bTransistionImmediately = false;
  m_iSlideNumber = iSlideNumber;

  m_pImage = pTexture;
  m_fWidth = (float)pTexture->GetWidth();
  m_fHeight = (float)pTexture->GetHeight();
  // reset our counter
  m_iCounter = 0;
#if defined(HAS_GLES) || defined(HAS_GL) 
  //transEffect = CROSSFADE;
#endif

  // initialize our transistion effect
  m_transistionStart.type = transEffect;
  m_transistionStart.start = 0;
  // the +1's make sure it actually occurs
  m_transistionStart.length = max((int)(g_infoManager.GetFPS() * g_guiSettings.GetInt("slideshow.transistiontime") * 0.001f), 1); // transition time in msec
  m_transistionEnd.type = transEffect;
  m_transistionEnd.start = m_transistionStart.length + max((int)(g_infoManager.GetFPS() * g_guiSettings.GetInt("slideshow.staytime")), 1);
  m_transistionEnd.length = max((int)(g_infoManager.GetFPS() * g_guiSettings.GetInt("slideshow.transistiontime") * 0.001f), 1); // transition time in msec
  if(m_bScreenSaverMode)
  {
    m_transistionStart.length = max((int)(g_infoManager.GetFPS() * 5), 180);
#ifdef HAS_INTEL_SMD
    m_transistionStart.length = 150;
#endif
    m_transistionEnd.start    = m_transistionStart.length;
    m_transistionEnd.length   = m_transistionStart.length;
  }
  CLog::Log(LOGDEBUG,"Duration: %i (transistion out length %i)", m_transistionEnd.start, m_transistionEnd.length);
  m_transistionTemp.type = TRANSISTION_NONE;
  m_fTransistionAngle = 0;
  m_fTransistionZoom = 0;
  m_fAngle = 0;
  if (pTexture->GetOrientation() == 7)
  { // rotate to 270 degrees
    m_fAngle = 3.0f;
  }
  if (pTexture->GetOrientation() == 5)
  { // rotate to 90 degrees
    m_fAngle = 1.0f;
  }
  m_fZoomAmount = 1;
  m_fZoomLeft = 0;
  m_fZoomTop = 0;
  m_iTotalFrames = m_transistionStart.length + m_transistionEnd.length + max(((int)g_infoManager.GetFPS() * g_guiSettings.GetInt("slideshow.staytime")), 1);
  if(m_bScreenSaverMode)
    m_iTotalFrames = m_transistionStart.length + m_transistionEnd.length + max(((int)g_infoManager.GetFPS() * 1), 1);
  // initialize our display effect
  if (dispEffect == EFFECT_RANDOM)
    m_displayEffect = (DISPLAY_EFFECT)((rand() % (EFFECT_RANDOM - 1)) + 1);
  else
    m_displayEffect = dispEffect;
  m_fPosX = m_fPosY = 0.0f;
  m_fPosZ = 1.0f;
  m_fVelocityX = m_fVelocityY = m_fVelocityZ = 0.0f;
  if (m_displayEffect == EFFECT_FLOAT)
  {
    // Calculate start and end positions
    // choose a random direction
    float angle = (rand() % 1000) / 1000.0f * 2 * (float)M_PI;
    m_fPosX = cos(angle) * g_advancedSettings.m_slideshowPanAmount * m_iTotalFrames * 0.00005f;
    m_fPosY = sin(angle) * g_advancedSettings.m_slideshowPanAmount * m_iTotalFrames * 0.00005f;
    m_fVelocityX = -m_fPosX * 2.0f / m_iTotalFrames;
    m_fVelocityY = -m_fPosY * 2.0f / m_iTotalFrames;
  }
  else if (m_displayEffect == EFFECT_ZOOM)
  {
    m_fPosZ = 1.0f;
    m_fVelocityZ = 0.0001f * g_advancedSettings.m_slideshowZoomAmount;
  }

  m_bIsFinished = false;
  m_bDrawNextImage = false;
  m_bIsLoaded = true;
  return ;
}

void CSlideShowPic::SetOriginalSize(int iOriginalWidth, int iOriginalHeight, bool bFullSize)
{
  m_iOriginalWidth = iOriginalWidth;
  m_iOriginalHeight = iOriginalHeight;
  m_bFullSize = bFullSize;
}

int CSlideShowPic::GetOriginalWidth()
{
  int iAngle = (int)(m_fAngle + 0.4f);
  if (iAngle % 2)
    return m_iOriginalHeight;
  else
    return m_iOriginalWidth;
}

int CSlideShowPic::GetOriginalHeight()
{
  int iAngle = (int)(m_fAngle + 0.4f);
  if (iAngle % 2)
    return m_iOriginalWidth;
  else
    return m_iOriginalHeight;
}

void CSlideShowPic::UpdateTexture(CBaseTexture* pTexture)
{
  CSingleLock lock(m_textureAccess);
  if (m_pImage)
  {
    delete m_pImage;
    m_pImage = NULL;
  }
  m_pImage = pTexture;
  m_fWidth = (float)pTexture->GetWidth();
  m_fHeight = (float)pTexture->GetHeight();
}

void CSlideShowPic::Process()
{
  if (!m_pImage || !m_bIsLoaded || m_bIsFinished) return ;
  if (m_iCounter <= m_transistionStart.length)
  { // do start transistion
    if (m_transistionStart.type == CROSSFADE)
    { // fade in at 1x speed
      m_alpha = (color_t)((float)m_iCounter / (float)m_transistionStart.length * 255.0f);
    }
    else if (m_transistionStart.type == FADEIN_FADEOUT)
    { // fade in at 2x speed, then keep solid
      m_alpha = (color_t)((float)m_iCounter / (float)m_transistionStart.length * 255.0f * 2);
      if (m_alpha > 255) m_alpha = 255;
    }
    else // m_transistionEffect == TRANSISTION_NONE
    {
      m_alpha = 0xFF; // opaque
    }
  }
  bool bPaused = m_bPause | (m_fZoomAmount != 1.0f);
  // check if we're doing a temporary effect (such as rotate + zoom)
  if (m_transistionTemp.type != TRANSISTION_NONE)
  {
    bPaused = true;
    if (m_iCounter >= m_transistionTemp.start)
    {
      if (m_iCounter >= m_transistionTemp.start + m_transistionTemp.length)
      { // we're finished this transistion
        if (m_transistionTemp.type == TRANSISTION_ZOOM)
        { // correct for any introduced inaccuracies.
          int i;
          for (i = 0; i < 10; i++)
            if (fabs(m_fZoomAmount - zoomamount[i]) < 0.01*zoomamount[i])
              break;
          m_fZoomAmount = zoomamount[i];
          m_bNoEffect = (m_fZoomAmount != 1.0f); // turn effect rendering back on.
        }
        if (m_transistionTemp.type == TRANSISTION_ROTATE)
        { // round to nearest integer for accuracy purposes
          m_fAngle = floor(m_fAngle + 0.4f);
        }
        m_transistionTemp.type = TRANSISTION_NONE;
      }
      else
      {
        if (m_transistionTemp.type == TRANSISTION_ROTATE)
        {
          m_fAngle += m_fTransistionAngle;
        }
        if (m_transistionTemp.type == TRANSISTION_ZOOM)
        {
          m_fZoomAmount += m_fTransistionZoom;
        }
      }
    }
  }
  // now just display
  if (!m_bNoEffect && !bPaused)
  {
    if (m_displayEffect == EFFECT_FLOAT)
    {
      m_fPosX += m_fVelocityX;
      m_fPosY += m_fVelocityY;
      float fMoveAmount = g_advancedSettings.m_slideshowPanAmount * m_iTotalFrames * 0.0001f;
      if (m_fPosX > fMoveAmount)
      {
        m_fPosX = fMoveAmount;
        m_fVelocityX = -m_fVelocityX;
      }
      if (m_fPosX < -fMoveAmount)
      {
        m_fPosX = -fMoveAmount;
        m_fVelocityX = -m_fVelocityX;
      }
      if (m_fPosY > fMoveAmount)
      {
        m_fPosY = fMoveAmount;
        m_fVelocityY = -m_fVelocityY;
      }
      if (m_fPosY < -fMoveAmount)
      {
        m_fPosY = -fMoveAmount;
        m_fVelocityY = -m_fVelocityY;
      }
    }
    else if (m_displayEffect == EFFECT_ZOOM)
    {
      m_fPosZ += m_fVelocityZ;
/*      if (m_fPosZ > 1.0f + 0.01f*g_guiSettings.GetInt("Slideshow.ZoomAmount"))
      {
        m_fPosZ = 1.0f + 0.01f * g_guiSettings.GetInt("Slideshow.ZoomAmount");
        m_fVelocityZ = -m_fVelocityZ;
      }
      if (m_fPosZ < 1.0f)
      {
        m_fPosZ = 1.0f;
        m_fVelocityZ = -m_fVelocityZ;
      }*/
    }
  }
  if (m_displayEffect != EFFECT_NO_TIMEOUT && bPaused && !m_bTransistionImmediately)
  { // paused - increment the last transistion start time
    m_transistionEnd.start++;
  }
  if (m_iCounter >= m_transistionEnd.start)
  { // do end transistion
//    CLog::Log(LOGDEBUG,"Transistioning");
    m_bDrawNextImage = true;
    if (m_transistionEnd.type == CROSSFADE)
    { // fade out at 1x speed
      m_alpha = 255 - (color_t)((float)(m_iCounter - m_transistionEnd.start) / (float)m_transistionEnd.length * 255.0f);
    }
    else if (m_transistionEnd.type == FADEIN_FADEOUT)
    { // keep solid, then fade out at 2x speed
      m_alpha = (color_t)((float)(m_transistionEnd.length - m_iCounter + m_transistionEnd.start) / (float)m_transistionEnd.length * 255.0f * 2);
      if (m_alpha > 255) m_alpha = 255;
    }
    else // m_transistionEffect == TRANSISTION_NONE
    {
      m_alpha = 0xFF; // opaque
    }
  }
  if (m_displayEffect != EFFECT_NO_TIMEOUT || m_iCounter < m_transistionStart.length || m_iCounter >= m_transistionEnd.start || (m_iCounter >= m_transistionTemp.start && m_iCounter < m_transistionTemp.start + m_transistionTemp.length))
  {
    /* this really annoying.  there's non-stop logging when viewing a pic outside of the slideshow
    if (m_displayEffect == EFFECT_NO_TIMEOUT)
      CLog::Log(LOGDEBUG, "Incrementing counter (%i) while not in slideshow (startlength=%i,endstart=%i,endlength=%i)", m_iCounter, m_transistionStart.length, m_transistionEnd.start, m_transistionEnd.length);
    */
    m_iCounter++;
  }
  if (m_iCounter > m_transistionEnd.start + m_transistionEnd.length)
    m_bIsFinished = true;
}

void CSlideShowPic::Keep()
{
  // this is called if we need to keep the current pic on screen
  // to wait for the next pic to load
  if (!m_bDrawNextImage) return ; // don't need to keep pic
  // hold off the start of the next frame
  m_transistionEnd.start = m_iCounter;
}

bool CSlideShowPic::StartTransistion()
{
  // this is called if we need to start transistioning immediately to the new picture
  if (m_bDrawNextImage) return false; // don't need to do anything as we are already transistioning
  // decrease the number of display frame
  m_transistionEnd.start = m_iCounter;
  m_bTransistionImmediately = true;
  return true;
}

void CSlideShowPic::Pause(bool bPause)
{
  if (!m_bDrawNextImage)
    m_bPause = bPause;
}

void CSlideShowPic::SetInSlideshow(bool slideshow)
{
  if (slideshow && m_displayEffect == EFFECT_NO_TIMEOUT)
    m_displayEffect = EFFECT_NONE;
}

int CSlideShowPic::GetTransistionTime(int iType) const
{
  if (iType == 0) // start transistion
    return m_transistionStart.length;
  else // iType == 1 // end transistion
    return m_transistionEnd.length;
}

void CSlideShowPic::SetTransistionTime(int iType, int iTime)
{
  if (iType == 0) // start transistion
    m_transistionStart.length = iTime;
  else // iType == 1 // end transistion
    m_transistionEnd.length = iTime;
}

void CSlideShowPic::Rotate(int iRotate)
{
  if (m_bDrawNextImage) return ;
  if (m_transistionTemp.type == TRANSISTION_ZOOM) return ;
  m_transistionTemp.type = TRANSISTION_ROTATE;
  m_transistionTemp.start = m_iCounter;
  m_transistionTemp.length = IMMEDIATE_TRANSISTION_TIME;
  m_fTransistionAngle = (float)(iRotate - m_fAngle) / (float)m_transistionTemp.length;
  // reset the timer
  m_transistionEnd.start = m_iCounter + m_transistionStart.length + ((int)g_infoManager.GetFPS() * g_guiSettings.GetInt("slideshow.staytime"));
}

void CSlideShowPic::Zoom(int iZoom, bool immediate /*= false*/)
{
  if (m_bDrawNextImage) return ;
  if (m_transistionTemp.type == TRANSISTION_ROTATE) return ;
  if (immediate)
  {
    m_fZoomAmount = zoomamount[iZoom - 1];
    return;
  }
  m_transistionTemp.type = TRANSISTION_ZOOM;
  m_transistionTemp.start = m_iCounter;
  m_transistionTemp.length = IMMEDIATE_TRANSISTION_TIME;
  m_fTransistionZoom = (float)(zoomamount[iZoom - 1] - m_fZoomAmount) / (float)m_transistionTemp.length;
  // reset the timer
  m_transistionEnd.start = m_iCounter + m_transistionStart.length + ((int)g_infoManager.GetFPS() * g_guiSettings.GetInt("slideshow.staytime"));
  // turn off the render effects until we're back down to normal zoom
  m_bNoEffect = true;
}

void CSlideShowPic::Move(float fDeltaX, float fDeltaY)
{
  m_fZoomLeft += fDeltaX;
  m_fZoomTop += fDeltaY;
  // reset the timer
 // m_transistionEnd.start = m_iCounter + m_transistionStart.length + ((int)g_infoManager.GetFPS() * g_guiSettings.GetInt("slideshow.staytime"));
}

void CSlideShowPic::Render()
{
  CSingleLock lock(m_textureAccess);
  if (!m_pImage || !m_bIsLoaded || m_bIsFinished) return ;
  // update the image
  Process();
  // calculate where we should render (and how large it should be)
  // calculate aspect ratio correction factor
  RESOLUTION iRes = g_graphicsContext.GetVideoResolution();
  float fOffsetX = (float)g_settings.m_ResInfo[iRes].Overscan.left;
  float fOffsetY = (float)g_settings.m_ResInfo[iRes].Overscan.top;
  float fScreenWidth = (float)g_settings.m_ResInfo[iRes].Overscan.right - g_settings.m_ResInfo[iRes].Overscan.left;
  float fScreenHeight = (float)g_settings.m_ResInfo[iRes].Overscan.bottom - g_settings.m_ResInfo[iRes].Overscan.top;

  float fPixelRatio = g_settings.m_ResInfo[iRes].fPixelRatio;

  // Rotate the image as needed
  float x[4];
  float y[4];
  float si = (float)sin(m_fAngle * M_PI * 0.5);
  float co = (float)cos(m_fAngle * M_PI * 0.5);
  x[0] = -m_fWidth * co + m_fHeight * si;
  y[0] = -m_fWidth * si - m_fHeight * co;
  x[1] = m_fWidth * co + m_fHeight * si;
  y[1] = m_fWidth * si - m_fHeight * co;
  x[2] = m_fWidth * co - m_fHeight * si;
  y[2] = m_fWidth * si + m_fHeight * co;
  x[3] = -m_fWidth * co - m_fHeight * si;
  y[3] = -m_fWidth * si + m_fHeight * co;

  // calculate our scale amounts
  float fSourceAR = m_fWidth / m_fHeight;
  float fSourceInvAR = 1 / fSourceAR;
  float fAR = si * si * (fSourceInvAR - fSourceAR) + fSourceAR;

  //float fOutputFrameAR = fAR / fPixelRatio;

  float fScaleNorm = fScreenWidth / m_fWidth;
  float fScaleInv = fScreenWidth / m_fHeight;

  bool bFillScreen = false;
  float fComp = 1.0f + 0.01f * g_advancedSettings.m_slideshowBlackBarCompensation;
  float fScreenRatio = fScreenWidth / fScreenHeight * fPixelRatio;
  // work out if we should be compensating the zoom to minimize blackbars
  // we should compute this based on the % of black bars on screen perhaps??
  // TODO: change m_displayEffect != EFFECT_NO_TIMEOUT to whether we're running the slideshow
  if (m_displayEffect != EFFECT_NO_TIMEOUT && fScreenRatio < fSourceAR * fComp && fSourceAR < fScreenRatio * fComp)
    bFillScreen = true;
  if ((!bFillScreen && fScreenWidth*fPixelRatio > fScreenHeight*fSourceAR) || (bFillScreen && fScreenWidth*fPixelRatio < fScreenHeight*fSourceAR))
    fScaleNorm = fScreenHeight / (m_fHeight * fPixelRatio);
  bFillScreen = false;
  if (m_displayEffect != EFFECT_NO_TIMEOUT && fScreenRatio < fSourceInvAR * fComp && fSourceInvAR < fScreenRatio * fComp)
    bFillScreen = true;
  if ((!bFillScreen && fScreenWidth*fPixelRatio > fScreenHeight*fSourceInvAR) || (bFillScreen && fScreenWidth*fPixelRatio < fScreenHeight*fSourceInvAR))
    fScaleInv = fScreenHeight / (m_fWidth * fPixelRatio);

  float fScale = si * si * (fScaleInv - fScaleNorm) + fScaleNorm;
  // scale if we need to due to the effect we're using
  if (m_displayEffect == EFFECT_FLOAT)
    fScale *= (1.0f + g_advancedSettings.m_slideshowPanAmount * m_iTotalFrames * 0.0001f);
  if (m_displayEffect == EFFECT_ZOOM)
    fScale *= m_fPosZ;
  // zoom image
  fScale *= m_fZoomAmount;

#ifdef HAS_EMBEDDED
  if(m_bScreenSaverMode)
    fScale = 1.25;
#endif

  // calculate the resultant coordinates
  for (int i = 0; i < 4; i++)
  {
    x[i] *= fScale * 0.5f; // as the offsets x[] and y[] are from center
    y[i] *= fScale * fPixelRatio * 0.5f;
    // center it
    x[i] += 0.5f * fScreenWidth + fOffsetX;
    y[i] += 0.5f * fScreenHeight + fOffsetY;
  }
  // shift if we're zooming
  if (m_fZoomAmount > 1)
  {
    float minx = x[0];
    float maxx = x[0];
    float miny = y[0];
    float maxy = y[0];
    for (int i = 1; i < 4; i++)
    {
      if (x[i] < minx) minx = x[i];
      if (x[i] > maxx) maxx = x[i];
      if (y[i] < miny) miny = y[i];
      if (y[i] > maxy) maxy = y[i];
    }
    float w = maxx - minx;
    float h = maxy - miny;
    if (w >= fScreenWidth)
    { // must have no black bars
      if (minx + m_fZoomLeft*w > fOffsetX)
        m_fZoomLeft = (fOffsetX - minx) / w;
      if (maxx + m_fZoomLeft*w < fOffsetX + fScreenWidth)
        m_fZoomLeft = (fScreenWidth + fOffsetX - maxx) / w;
      for (int i = 0; i < 4; i++)
        x[i] += w * m_fZoomLeft;
    }
    if (h >= fScreenHeight)
    { // must have no black bars
      if (miny + m_fZoomTop*h > fOffsetY)
        m_fZoomTop = (fOffsetY - miny) / h;
      if (maxy + m_fZoomTop*h < fOffsetY + fScreenHeight)
        m_fZoomTop = (fScreenHeight + fOffsetY - maxy) / h;
      for (int i = 0; i < 4; i++)
        y[i] += m_fZoomTop * h;
    }
  }
  // add offset from display effects
  for (int i = 0; i < 4; i++)
  {
    x[i] += m_fPosX * m_fWidth * fScale;
    y[i] += m_fPosY * m_fHeight * fScale;
  }
  // and render
  Render(x, y, m_pImage, (m_alpha << 24) | 0xFFFFFF);

  // now render the image in the top right corner if we're zooming
  if (m_fZoomAmount == 1 || m_bIsComic) return ;

  float sx[4], sy[4];
  sx[0] = -m_fWidth * co + m_fHeight * si;
  sy[0] = -m_fWidth * si - m_fHeight * co;
  sx[1] = m_fWidth * co + m_fHeight * si;
  sy[1] = m_fWidth * si - m_fHeight * co;
  sx[2] = m_fWidth * co - m_fHeight * si;
  sy[2] = m_fWidth * si + m_fHeight * co;
  sx[3] = -m_fWidth * co - m_fHeight * si;
  sy[3] = -m_fWidth * si + m_fHeight * co;
  // convert to the appropriate scale
  float fSmallArea = fScreenWidth * fScreenHeight / 50;
  float fSmallWidth = sqrt(fSmallArea * fAR / fPixelRatio); // fAR*height = width, so total area*far = width*width
  float fSmallHeight = fSmallArea / fSmallWidth;
  float fSmallX = fOffsetX + fScreenWidth * 0.95f - fSmallWidth * 0.5f;
  float fSmallY = fOffsetY + fScreenHeight * 0.05f + fSmallHeight * 0.5f;
  fScaleNorm = fSmallWidth / m_fWidth;
  fScaleInv = fSmallWidth / m_fHeight;
  fScale = si * si * (fScaleInv - fScaleNorm) + fScaleNorm;
  for (int i = 0; i < 4; i++)
  {
    sx[i] *= fScale * 0.5f;
    sy[i] *= fScale * fPixelRatio * 0.5f;
  }
  // calculate a black border
  float bx[4];
  float by[4];
  for (int i = 0; i < 4; i++)
  {
    if (sx[i] > 0)
      bx[i] = sx[i] + 1;
    else
      bx[i] = sx[i] - 1;
    if (sy[i] > 0)
      by[i] = sy[i] + 1;
    else
      by[i] = sy[i] - 1;
    sx[i] += fSmallX;
    sy[i] += fSmallY;
    bx[i] += fSmallX;
    by[i] += fSmallY;
  }

  fSmallX -= fSmallWidth * 0.5f;
  fSmallY -= fSmallHeight * 0.5f;

  Render(bx, by, NULL, PICTURE_VIEW_BOX_BACKGROUND);
  Render(sx, sy, m_pImage, 0xFFFFFFFF);

  // now we must render the wireframe image of the view window
  // work out the direction of the top of pic vector
  float scale;
  if (fabs(x[1] - x[0]) > fabs(x[3] - x[0]))
    scale = (sx[1] - sx[0]) / (x[1] - x[0]);
  else
    scale = (sx[3] - sx[0]) / (x[3] - x[0]);
  float ox[4];
  float oy[4];
  ox[0] = (fOffsetX - x[0]) * scale + sx[0];
  oy[0] = (fOffsetY - y[0]) * scale + sy[0];
  ox[1] = (fOffsetX + fScreenWidth - x[0]) * scale + sx[0];
  oy[1] = (fOffsetY - y[0]) * scale + sy[0];
  ox[2] = (fOffsetX + fScreenWidth - x[0]) * scale + sx[0];
  oy[2] = (fOffsetY + fScreenHeight - y[0]) * scale + sy[0];
  ox[3] = (fOffsetX - x[0]) * scale + sx[0];
  oy[3] = (fOffsetY + fScreenHeight - y[0]) * scale + sy[0];
  // crop to within the range of our piccy
  for (int i = 0; i < 4; i++)
  {
    if (ox[i] < fSmallX) ox[i] = fSmallX;
    if (ox[i] > fSmallX + fSmallWidth) ox[i] = fSmallX + fSmallWidth;
    if (oy[i] < fSmallY) oy[i] = fSmallY;
    if (oy[i] > fSmallY + fSmallHeight) oy[i] = fSmallY + fSmallHeight;
  }

  Render(ox, oy, NULL, PICTURE_VIEW_BOX_COLOR);
}

void CSlideShowPic::Render(float *x, float *y, CBaseTexture* pTexture, color_t color)
{
  if(GET_A(color) == 255)
    g_Windowing.EnableBlending(false);
  else if(m_bBlend)
    g_Windowing.EnableBlending(true);

#ifdef HAS_DX
  struct VERTEX
  {
    D3DXVECTOR4 p;
    D3DCOLOR col;
    FLOAT tu, tv;
  };
  static const DWORD FVF_VERTEX = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;

  VERTEX vertex[5];

  for (int i = 0; i < 4; i++)
  {
#ifdef HAS_XBOX_D3D
    vertex[i].p = D3DXVECTOR4( x[i], y[i], 0, 0 );
#else
    vertex[i].p = D3DXVECTOR4( x[i], y[i], 0, 1.0f);
#endif
    vertex[i].tu = 0;
    vertex[i].tv = 0;
    vertex[i].col = color;
  }
#ifdef HAS_XBOX_D3D
  vertex[1].tu = m_fWidth;
  vertex[2].tu = m_fWidth;
  vertex[2].tv = m_fHeight;
  vertex[3].tv = m_fHeight;
#else
  vertex[1].tu = 1.0f;
  vertex[2].tu = 1.0f;
  vertex[2].tv = 1.0f;
  vertex[3].tv = 1.0f;
#endif

  // close the loop incase we want to do wireframe
  memcpy(&vertex[4], &vertex[0], sizeof(VERTEX));

  // Set state to render the image
  if (pTexture)
  {
    pTexture->LoadToGPU();
    g_Windowing.Get3DDevice()->SetTexture( 0, pTexture->GetTextureObject() );
  }
  g_Windowing.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
  g_Windowing.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
  g_Windowing.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
  g_Windowing.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
  g_Windowing.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
  g_Windowing.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
  g_Windowing.Get3DDevice()->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
  g_Windowing.Get3DDevice()->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
  g_Windowing.Get3DDevice()->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
  g_Windowing.Get3DDevice()->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
  g_Windowing.Get3DDevice()->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR /*g_stSettings.m_minFilter*/ );
  g_Windowing.Get3DDevice()->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR /*g_stSettings.m_maxFilter*/ );
  g_Windowing.Get3DDevice()->SetRenderState( D3DRS_ZENABLE, FALSE );
  g_Windowing.Get3DDevice()->SetRenderState( D3DRS_FOGENABLE, FALSE );
  g_Windowing.Get3DDevice()->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
  g_Windowing.Get3DDevice()->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  g_Windowing.Get3DDevice()->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
  g_Windowing.Get3DDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  g_Windowing.Get3DDevice()->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
  g_Windowing.Get3DDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
  g_Windowing.Get3DDevice()->SetRenderState(D3DRS_LIGHTING, FALSE);

  g_Windowing.Get3DDevice()->SetFVF( FVF_VERTEX );
  // Render the image
  g_Windowing.Get3DDevice()->DrawPrimitiveUP( pTexture ? D3DPT_TRIANGLEFAN : D3DPT_LINESTRIP, pTexture ? 2 : 4, vertex, sizeof(VERTEX) );
  if (pTexture) g_Windowing.Get3DDevice()->SetTexture(0, NULL);

#elif defined(HAS_GL_1) 
  g_graphicsContext.BeginPaint();
  if (pTexture)
  {
    pTexture->LoadToGPU();
    glBindTexture(GL_TEXTURE_2D, pTexture->GetTextureObject());
    glEnable(GL_TEXTURE_2D);
         
    // diffuse coloring
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
    glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
    glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
    glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
  }
  else
    glDisable(GL_TEXTURE_2D);
  glPolygonMode(GL_FRONT_AND_BACK, pTexture ? GL_FILL : GL_LINE);
  
  glBegin(GL_QUADS);
  float u1 = 0, u2 = 1, v1 = 0, v2 = 1;
  if (pTexture)
  {
    u2 = (float)pTexture->GetWidth() / pTexture->GetTextureWidth();
    v2 = (float)pTexture->GetHeight() / pTexture->GetTextureHeight();
  }
  
  glColor4ub((GLubyte)GET_R(color), (GLubyte)GET_G(color), (GLubyte)GET_B(color), (GLubyte)GET_A(color));
  glTexCoord2f(u1, v1);
  glVertex3f(x[0], y[0], 0);
  
  // Bottom-left vertex (corner)
  glColor4ub((GLubyte)GET_R(color), (GLubyte)GET_G(color), (GLubyte)GET_B(color), (GLubyte)GET_A(color));
  glTexCoord2f(u2, v1);
  glVertex3f(x[1], y[1], 0);
  
  // Bottom-right vertex (corner)
  glColor4ub((GLubyte)GET_R(color), (GLubyte)GET_G(color), (GLubyte)GET_B(color), (GLubyte)GET_A(color));
  glTexCoord2f(u2, v2);
  glVertex3f(x[2], y[2], 0);
  
  // Top-right vertex (corner)
  glColor4ub((GLubyte)GET_R(color), (GLubyte)GET_G(color), (GLubyte)GET_B(color), (GLubyte)GET_A(color));
  glTexCoord2f(u1, v2);
  glVertex3f(x[3], y[3], 0);
    
  glEnd();
  g_graphicsContext.EndPaint();
#elif defined(HAS_GLES) || defined(HAS_GL)
  struct ImageVertex
  {
    float x, y, z;
    unsigned char r, g, b, a;    
    float u1, v1;
  };

  ImageVertex v[6];

  if(g_pTextureShadersProgram == NULL)
  {
    g_pTextureShadersProgram = new CGLSLShaderProgram();
    g_pTextureShadersProgram->VertexShader()->SetSource(g_TextureVertexShader);
    g_pTextureShadersProgram->PixelShader()->SetSource(g_TexturePixelShader);
    g_pTextureShadersProgram->CompileAndLink();
  }

  if(g_pNoTextureShadersProgram == NULL)
  {
    g_pNoTextureShadersProgram = new CGLSLShaderProgram();
    g_pNoTextureShadersProgram->VertexShader()->SetSource(g_TextureVertexShader);
    g_pNoTextureShadersProgram->PixelShader()->SetSource(g_NoTexturePixelShader);
    g_pNoTextureShadersProgram->CompileAndLink();
  }

  if(g_pTextureShadersProgramNoDiffuse == NULL)
  {
    g_pTextureShadersProgramNoDiffuse = new CGLSLShaderProgram();
    g_pTextureShadersProgramNoDiffuse->VertexShader()->SetSource(g_TextureVertexShader);
    g_pTextureShadersProgramNoDiffuse->PixelShader()->SetSource(g_TexturePixelShaderNoDiffuse);
    g_pTextureShadersProgramNoDiffuse->CompileAndLink();
  }

  GLuint progObj = -1;

  if(pTexture)
  {
    g_pTextureShadersProgram->Enable();
    progObj = g_pTextureShadersProgram->ProgramHandle();
  }
  else
  {
    g_pNoTextureShadersProgram->Enable();
    progObj = g_pNoTextureShadersProgram->ProgramHandle();
  }

  if(GET_A(color) == 255)
  {
    g_pTextureShadersProgramNoDiffuse->Enable();
    progObj = g_pTextureShadersProgramNoDiffuse->ProgramHandle();
  }

  if(progObj == (GLuint) -1)
    return;
  
  m_uniMatProjection  = glGetUniformLocation(progObj, "u_matProjection");
  m_uniMatModelView  = glGetUniformLocation(progObj, "u_matModelView");
  m_uniTexture0 = glGetUniformLocation(progObj, "u_texture0");
  m_attribTextureCoord0 = glGetAttribLocation(progObj, "a_texCoord0");
  m_attribVertex = glGetAttribLocation(progObj, "a_vertex");
  m_uniColor = glGetUniformLocation(progObj, "u_diffuseColor");

  glEnableVertexAttribArray(m_attribVertex); 
  glEnableVertexAttribArray(m_attribTextureCoord0);
  
  g_graphicsContext.BeginPaint();

  if (pTexture)
  {
    pTexture->LoadToGPU();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pTexture->GetTextureObject());
    glEnable(GL_TEXTURE_2D);
  } 
  else
  {
    glDisable(GL_TEXTURE_2D);
  }

  if (m_uniColor != -1)
  {
    GLfloat diffuseColor[4];
    diffuseColor[0] = (GLfloat)GET_R(color) / 255.0f;
    diffuseColor[1] = (GLfloat)GET_G(color) / 255.0f;
    diffuseColor[2] = (GLfloat)GET_B(color) / 255.0f;
    diffuseColor[3] = (GLfloat)GET_A(color) / 255.0f;
    glUniform4fv(m_uniColor, 1, diffuseColor);

    if(diffuseColor[3] == 0.0f)
    {
      g_graphicsContext.EndPaint();
      return;
    }
  }

  g_graphicsContext.PushTransform(TransformMatrix(), true);

  TransformMatrix* matModelView = g_Windowing.GetHardwareTransform(MATRIX_TYPE_MODEL_VIEW);
  TransformMatrix* matProjection = g_Windowing.GetHardwareTransform(MATRIX_TYPE_PROJECTION);

  glUniformMatrix4fv(m_uniMatModelView, 1, GL_FALSE, (GLfloat *)matModelView->m);
  glUniformMatrix4fv(m_uniMatProjection, 1, GL_FALSE, (GLfloat *)matProjection->m);

  glUniform1i(m_uniTexture0, 0);

  // Set the texture's stretching properties
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  float u1 = 0, u2 = 1, v1 = 0, v2 = 1;
  if (pTexture)
  {
    u2 = (float)pTexture->GetWidth() / pTexture->GetTextureWidth();
    v2 = (float)pTexture->GetHeight() / pTexture->GetTextureHeight();
  }

  v[0].x = x[0];
  v[0].y = y[0];
  v[0].z = 0.0f;
  v[0].u1 = u1;
  v[0].v1 = v1;

  v[1].x = x[1];
  v[1].y = y[1];
  v[1].z = 0.0f;
  v[1].u1 = u2;
  v[1].v1 = v1;

  v[2].x = x[2];
  v[2].y = y[2];
  v[2].z = 0.0f;
  v[2].u1 = u2;
  v[2].v1 = v2;

  v[3].x = x[2];
  v[3].y = y[2];
  v[3].z = 0.0f;
  v[3].u1 = u2;
  v[3].v1 = v2;

  v[4].x = x[3];
  v[4].y = y[3];
  v[4].z = 0.0f;
  v[4].u1 = u1;
  v[4].v1 = v2;

  v[5].x = x[0];
  v[5].y = y[0];
  v[5].z = 0.0f;
  v[5].u1 = u1;
  v[5].v1 = v1;

  glVertexAttribPointer(m_attribVertex, 3, GL_FLOAT, GL_FALSE, sizeof(ImageVertex), (char*)v + offsetof(ImageVertex, x));
  glVertexAttribPointer(m_attribTextureCoord0, 2, GL_FLOAT, GL_FALSE, sizeof(ImageVertex), (char*)v + offsetof(ImageVertex, u1));
   
if(pTexture)
  glDrawArrays(GL_TRIANGLES, 0, 6);
else
  glDrawArrays(GL_LINE_LOOP, 0, 6);

  g_graphicsContext.PopTransform();
  g_graphicsContext.EndPaint();
#else
// SDL render
  g_Windowing.BlitToScreen(m_pImage, NULL, NULL);
#endif
}

void CSlideShowPic::CopyLocationAndZoom(const CSlideShowPic& pic)
{
  m_fPosX = pic.m_fPosX;
  m_fPosY = pic.m_fPosY;
  m_fZoomLeft = pic.m_fZoomLeft;
  m_fZoomTop = pic.m_fZoomTop;
  m_fZoomAmount = pic.m_fZoomAmount;
}
