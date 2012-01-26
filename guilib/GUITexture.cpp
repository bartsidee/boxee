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
#include "GraphicContext.h"
#include "TextureManager.h"
#include "GUILargeTextureManager.h"
#include "GUIWindowManager.h"
#include "MathUtils.h"
#include "log.h"
#ifdef HAS_OPENMAX
#include "Application.h"
#endif
#include "utils/SingleLock.h"
#include "Texture.h"

using namespace std;

#define MIX_ALPHA(a,c) (((a * (c >> 24)) / 255) << 24) | (c & 0x00ffffff)

CTextureInfo::CTextureInfo()
{
  orientation = 0;
  useLarge = false;
  blendBorder = true;
  blendCenter = true;
}

CTextureInfo::CTextureInfo(const CStdString &file)
{
  orientation = 0;
  useLarge = false;
  filename = file;
  blendBorder = true;
  blendCenter = true;
}

void CTextureInfo::operator=(const CTextureInfo &right)
{
  border = right.border;
  srcBorder = right.srcBorder;
  orientation = right.orientation;
  diffuse = right.diffuse;
  filename = right.filename;
  useLarge = right.useLarge;
  blendBorder = right.blendBorder;
  blendCenter = right.blendCenter;
}

CGUITextureBase::CGUITextureBase(float posX, float posY, float width, float height, const CTextureInfo& texture, CBaseTexture* textureData)
{
  m_posX = posX;
  m_posY = posY;
  m_width = width;
  m_height = height;
  m_info = texture;

  // defaults
  m_visible = true;
  m_diffuseColor = 0xffffffff;
  m_diffuseColorBlended = 0xffffffff;
  m_alpha = 0xff;
  m_bNeedBlending = false;

  m_vertex.SetRect(m_posX, m_posY, m_posX + m_width, m_posY + m_height);

  m_frameWidth = 0;
  m_frameHeight = 0;

  m_texCoordsScaleU = 1.0f;
  m_texCoordsScaleV = 1.0f;
  m_diffuseU = 1.0f;
  m_diffuseV = 1.0f;
  m_diffuseScaleU = 1.0f;
  m_diffuseScaleV = 1.0f;

  // anim gifs
  m_currentFrame = 0;
  m_frameCounter = (unsigned int) -1;
  m_currentLoop = 0;

  m_allocateDynamically = false;
  m_isAllocated = NO;
  m_invalid = true;
  m_renderBorderOnly = false;

  if (textureData != NULL)
  {
    m_texture.Add(textureData, 0);
    m_isAllocated = NORMAL;
    m_frameWidth = width;
    m_frameHeight = height;
    CalculateSize();
  }

  m_loadingAnimation = "";

  BuildColorData();
}

CGUITextureBase::CGUITextureBase(const CGUITextureBase &right)
{
  m_posX = right.m_posX;
  m_posY = right.m_posY;
  m_width = right.m_width;
  m_height = right.m_height;
  m_info = right.m_info;

  m_visible = right.m_visible;
  m_diffuseColor = right.m_diffuseColor;
  m_alpha = right.m_alpha;
  m_aspect = right.m_aspect;
  m_renderBorderOnly = right.m_renderBorderOnly;

  m_allocateDynamically = right.m_allocateDynamically;

  // defaults
  m_vertex.SetRect(m_posX, m_posY, m_posX + m_width, m_posY + m_height);

  m_frameWidth = 0;
  m_frameHeight = 0;

  m_texCoordsScaleU = 1.0f;
  m_texCoordsScaleV = 1.0f;
  m_diffuseU = 1.0f;
  m_diffuseV = 1.0f;
  m_diffuseScaleU = 1.0f;
  m_diffuseScaleV = 1.0f;

  m_currentFrame = 0;
  m_frameCounter = (unsigned int) -1;
  m_currentLoop = 0;

  m_isAllocated = NO;
  m_invalid = true;

  BuildColorData();
}

CGUITextureBase::~CGUITextureBase(void)
{
}

void CGUITextureBase::AllocateOnDemand()
{
  if (m_visible)
  { // visible, so make sure we're allocated
    if (!IsAllocated() || (m_info.useLarge && !m_texture.size()))
      AllocResources();
  }
  else
  { // hidden, so deallocate as applicable
    if (m_allocateDynamically && IsAllocated())
      FreeResources();
    // reset animated textures (animgifs)
    m_currentLoop = 0;
    m_currentFrame = 0;
    m_frameCounter = 0;
  }
}

// Draw all the elements of the frame including left and right parts
void CGUITextureBase::DrawRectangleElements()
{
  unsigned char alpha = GET_A(m_diffuseColorBlended);

  bool bEnableBorderBlend = m_info.blendBorder || alpha != 255;
  bool bEnableCenterBlend = m_info.blendCenter || alpha != 255;

  // first draw borders
  m_bNeedBlending = bEnableBorderBlend;
  g_Windowing.EnableBlending(m_bNeedBlending);

  if(!LoadShaders())
    return;

  if(!SelectShader())
    return;

  if(m_diffuse.size() > 0)
    m_diffuse.m_textures[0]->LoadToGPU();

  Begin();

  // left segment (0,0,u1,v3)
  if (m_info.border.x1)
  {
    if (m_info.border.y1)
      Draw(m_vertexCoords[0]);
    Draw(m_vertexCoords[1]);
    if (m_info.border.y2)
      Draw(m_vertexCoords[2]);
  }

  // middle segment (u1,0,u2,v3)
  if (m_info.border.y1)
  {
    Draw(m_vertexCoords[3]);
  }

  if (m_info.border.y2)
  {
    Draw(m_vertexCoords[5]);
  }

  // right segment
  if (m_info.border.x2)
  { // have a left border
    if (m_info.border.y1)
      Draw(m_vertexCoords[6]);
    Draw(m_vertexCoords[7]);
    if (m_info.border.y2)
      Draw(m_vertexCoords[8]);
  } 

  // now draw the center
  if(bEnableBorderBlend != bEnableCenterBlend)
  {
    m_bNeedBlending = bEnableCenterBlend;
    g_Windowing.EnableBlending(m_bNeedBlending);
    if(!SelectShader())
        return;
    Begin();
  }

  if (!m_renderBorderOnly)
  {
    Draw(m_vertexCoords[4]);
  }

  g_Windowing.EnableBlending(false);
  g_Windowing.EnableTexture(false);
 
  End();
}

void CGUITextureBase::Render()
{
  bool bClippingUsed = false;
  // check if we need to allocate our resources
  AllocateOnDemand();

  if (!m_visible || !m_texture.size())
    return;

  if (m_texture.size() > 1)
    UpdateAnimFrame();

  // see if we need to clip the image
  if (m_vertex.Width() > m_width || m_vertex.Height() > m_height)
  {
    if (!g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height))
      return;
    bClippingUsed = true;
  }

  BuildColorData();

  // if nothing changed, just render and return
  if (m_invalid)
  {
    CalculateSize();
  
    // compute the texture coordinates
    float u1, u2, u3, v1, v2, v3;
    u1 = m_info.srcBorder.x1;
    u2 = m_frameWidth - m_info.srcBorder.x2;
    u3 = m_frameWidth;
    v1 = m_info.srcBorder.y1;
    v2 = m_frameHeight - m_info.srcBorder.y2;
    v3 = m_frameHeight;

    if (!m_texture.m_texCoordsArePixels)
    {
      // for twiddled images (twiddled at texture.xbt creation time), we'll need to multiply u1,u2...v3 by
      // m_imageInitialHeight/m_imageHeight or m_imageInitialWidth/m_imageWidth to achieve the initial image size
      u1 *= m_texCoordsScaleU;
      u2 *= m_texCoordsScaleU;
      u3 *= m_texCoordsScaleU;
      v1 *= m_texCoordsScaleV;
      v2 *= m_texCoordsScaleV;
      v3 *= m_texCoordsScaleV;
    }

    // TODO: The diffuse coloring applies to all vertices, which will
    //       look weird for stuff with borders, as will the -ve height/width
    //       for flipping
    memset(m_vertexCoords, 0, sizeof(ImageCoords) * 9);

    // left segment (0,0,u1,v3)
    BuildVertexCoords(m_vertexCoords[0], m_vertex.x1, m_vertex.y1, 
      m_vertex.x1 + m_info.border.x1, m_vertex.y1 + m_info.border.y1, 0, 0, u1, v1, u3, v3);
    BuildVertexCoords(m_vertexCoords[1], m_vertex.x1, m_vertex.y1 + m_info.border.y1, 
      m_vertex.x1 + m_info.border.x1, m_vertex.y2 - m_info.border.y2, 0, v1, u1, v2, u3, v3);
    BuildVertexCoords(m_vertexCoords[2], m_vertex.x1, m_vertex.y2 - m_info.border.y2, 
      m_vertex.x1 + m_info.border.x1, m_vertex.y2, 0, v2, u1, v3, u3, v3); 
  
    // middle segment (u1,0,u2,v3)
    BuildVertexCoords(m_vertexCoords[3], m_vertex.x1 + m_info.border.x1, m_vertex.y1, 
      m_vertex.x2 - m_info.border.x2, m_vertex.y1 + m_info.border.y1, u1, 0, u2, v1, u3, v3);
    BuildVertexCoords(m_vertexCoords[4], m_vertex.x1 + m_info.border.x1, m_vertex.y1 + m_info.border.y1, 
      m_vertex.x2 - m_info.border.x2, m_vertex.y2 - m_info.border.y2, u1, v1, u2, v2, u3, v3);
    BuildVertexCoords(m_vertexCoords[5], m_vertex.x1 + m_info.border.x1, m_vertex.y2 - m_info.border.y2, m_vertex.x2 - m_info.border.x2, m_vertex.y2, u1, v2, u2, v3, u3, v3); 

    // right segment (u2,0,u3,v3)
    BuildVertexCoords(m_vertexCoords[6], m_vertex.x2 - m_info.border.x2, m_vertex.y1, m_vertex.x2, 
      m_vertex.y1 + m_info.border.y1, u2, 0, u3, v1, u3, v3);
    BuildVertexCoords(m_vertexCoords[7], m_vertex.x2 - m_info.border.x2, m_vertex.y1 + m_info.border.y1, 
      m_vertex.x2, m_vertex.y2 - m_info.border.y2, u2, v1, u3, v2, u3, v3);
    BuildVertexCoords(m_vertexCoords[8], m_vertex.x2 - m_info.border.x2, m_vertex.y2 - m_info.border.y2, 
      m_vertex.x2, m_vertex.y2, u2, v2, u3, v3, u3, v3); 
  }

  DrawRectangleElements();

  if (bClippingUsed)
    g_graphicsContext.RestoreClipRegion();

  m_invalid = false;
}

void CGUITextureBase::BuildVertexCoords(ImageCoords& coords, float left, float top, float right, 
                                        float bottom, float u1, float v1, float u2, float v2, float u3, float v3)
{
  /*
  static int count = 0;
  CLog::Log(LOGINFO, "GUITextureBase::BuildVertexCoords %d", count++);
  */

  CRect diffuse(u1, v1, u2, v2);
  CRect texture(u1, v1, u2, v2);
  CRect vertex(left, top, right, bottom);

  if (vertex.IsEmpty())
    return; // nothing to render

  int orientation = GetOrientation();
  OrientateTexture(texture, u3, v3, orientation);

  if (m_diffuse.size())
  {
    // flip the texture as necessary.  Diffuse just gets flipped according to m_info.orientation.
    // Main texture gets flipped according to GetOrientation().
    diffuse.x1 *= m_diffuseScaleU / u3; diffuse.x2 *= m_diffuseScaleU / u3;
    diffuse.y1 *= m_diffuseScaleV / v3; diffuse.y2 *= m_diffuseScaleV / v3;
    diffuse += m_diffuseOffset;
    OrientateTexture(diffuse, m_diffuseU, m_diffuseV, m_info.orientation);
  }

  float x[4], y[4], z[4];

#define ROUND_TO_PIXEL(x) (float)(MathUtils::round_int(x))

  x[0] = vertex.x1;
  y[0] = vertex.y1;
  z[0] = 0;

  x[1] = vertex.x2;
  y[1] = vertex.y1;
  z[1] = 0;

  x[2] = vertex.x2;
  y[2] = vertex.y2;
  z[2] = 0;

  x[3] = vertex.x1;
  y[3] = vertex.y2;
  z[3] = 0;
  
  if (y[2] == y[0]) y[2] += 1.0f; if (x[2] == x[0]) x[2] += 1.0f;
  if (y[3] == y[1]) y[3] += 1.0f; if (x[3] == x[1]) x[3] += 1.0f;

  ImageVertex* v = coords.m_pCoords;

  for(int i = 0; i < 4; i++)
  {
    v[i].x = x[i];
    v[i].y = y[i];
    v[i].z = z[i];
  }

  v[0].u1 = texture.x1;
  v[0].v1 = texture.y1;
  v[0].u2 = diffuse.x1;
  v[0].v2 = diffuse.y1;

  if (orientation & 4)
  {
    v[1].u1 = texture.x1;
    v[1].v1 = texture.y2;
  }
  else
  {
    v[1].u1 = texture.x2;
    v[1].v1 = texture.y1;
  }
  if (m_info.orientation & 4)
  {
    v[1].u2 = diffuse.x1;
    v[1].v2 = diffuse.y2;
  }
  else
  {
    v[1].u2 = diffuse.x2;
    v[1].v2 = diffuse.y1;
  }
  
  v[2].u1 = texture.x2;
  v[2].v1 = texture.y2;
  v[2].u2 = diffuse.x2;
  v[2].v2 = diffuse.y2;

  if (orientation & 4)
  {
    v[3].u1 = texture.x2;
    v[3].v1 = texture.y1;
  }
  else
  {
    v[3].u1 = texture.x1;
    v[3].v1 = texture.y2;
  }
  if (m_info.orientation & 4)
  {
    v[3].u2 = diffuse.x2;
    v[3].v2 = diffuse.y1;
  }
  else
  {
    v[3].u2 = diffuse.x1;
    v[3].v2 = diffuse.y2;
  }
}

void CGUITextureBase::BuildColorData()
{
  m_diffuseColorBlended = MIX_ALPHA(m_alpha, m_diffuseColor);
  m_diffuseColorBlended = g_graphicsContext.MergeAlpha(m_diffuseColorBlended);
}

void CGUITextureBase::AllocResources()
{
  if (m_info.filename.IsEmpty())
    return;

  if (m_texture.size() && IsAllocated())
    return; // already have our texture

  // reset our animstate
  m_frameCounter = 0;
  m_currentFrame = 0;
  m_currentLoop = 0;

  if (m_info.useLarge)
  { // we want to use the large image loader, but we first check for bundled textures
    if (!IsAllocated())
    {
      int images = g_TextureManager.Load(m_info.filename, true, m_loadingAnimation);
      if (images)
      {
        CSingleLock lock(g_TextureManager.GetLock());
        m_texture = g_TextureManager.GetTexture(m_info.filename);
        m_isAllocated = m_texture.m_bPlaceHolder?NO:NORMAL; // real image is not loaded yet (only placeholder)
      }
    }
    if (m_isAllocated != NORMAL && !m_texture.m_bPlaceHolder)
    { // use our large image background loader
      CTextureArray texture;
      if (g_largeTextureManager.GetImage(m_info.filename, texture, !IsAllocated()))
      {
      m_isAllocated = LARGE;

      if (!texture.size()) // not ready as yet
        return;

      m_texture = texture;
      }
      else
        m_isAllocated = LARGE_FAILED;
    }
  } 
  else if (!IsAllocated())
  {
    int images = g_TextureManager.Load(m_info.filename, false, m_loadingAnimation);

    // set allocated to true even if we couldn't load the image to save
    // us hitting the disk every frame
    m_isAllocated = images ? NORMAL : NORMAL_FAILED;
    if (!images)
      return;

    CSingleLock lock(g_TextureManager.GetLock());
    m_texture = g_TextureManager.GetTexture(m_info.filename);
    if (m_texture.m_bPlaceHolder)
      m_isAllocated = NO; // real image is not loaded yet (only placeholder)
  }
  m_frameWidth = (float)m_texture.m_width;
  m_frameHeight = (float)m_texture.m_height;

  // load the diffuse texture (if necessary)
  if (!m_info.diffuse.IsEmpty())
  {
    g_TextureManager.Load(m_info.diffuse, false, m_loadingAnimation);
    
    CSingleLock lock(g_TextureManager.GetLock());
    m_diffuse = g_TextureManager.GetTexture(m_info.diffuse);
  }

  CalculateSize();

  m_invalid = true;

  // call our implementation
  Allocate();
}

void CGUITextureBase::CalculateSize()
{
  if (m_currentFrame >= m_texture.size())
    return;

  m_texCoordsScaleU = 1.0f / m_texture.m_texWidth;
  m_texCoordsScaleV = 1.0f / m_texture.m_texHeight;

  if (m_width == 0)
    m_width = m_frameWidth;
  if (m_height == 0)
    m_height = m_frameHeight;

  float newPosX = m_posX;
  float newPosY = m_posY;
  float newWidth = m_width;
  float newHeight = m_height;

  if (m_aspect.ratio != CAspectRatio::AR_STRETCH && m_frameWidth && m_frameHeight)
  {
    // to get the pixel ratio, we must use the SCALED output sizes
    float pixelRatio = g_graphicsContext.GetScalingPixelRatio();

    float fSourceFrameRatio = m_frameWidth / m_frameHeight;
    if (GetOrientation() & 4)
      fSourceFrameRatio = m_frameHeight / m_frameWidth;
    float fOutputFrameRatio = fSourceFrameRatio / pixelRatio;

    // maximize the width
    newHeight = m_width / fOutputFrameRatio;

    if ((m_aspect.ratio == CAspectRatio::AR_SCALE && newHeight < m_height) ||
        (m_aspect.ratio == CAspectRatio::AR_KEEP && newHeight > m_height))
    {
      newHeight = m_height;
      newWidth = newHeight * fOutputFrameRatio;
    }
    if (m_aspect.ratio == CAspectRatio::AR_CENTER)
    { // keep original size + center
      newWidth = m_frameWidth;
      newHeight = m_frameHeight;
    }

    if (m_aspect.align & ASPECT_ALIGN_LEFT)
      newPosX = m_posX;
    else if (m_aspect.align & ASPECT_ALIGN_RIGHT)
      newPosX = m_posX + m_width - newWidth;
    else
      newPosX = m_posX + (m_width - newWidth) * 0.5f;
    if (m_aspect.align & ASPECT_ALIGNY_TOP)
      newPosY = m_posY;
    else if (m_aspect.align & ASPECT_ALIGNY_BOTTOM)
      newPosY = m_posY + m_height - newHeight;
    else
      newPosY = m_posY + (m_height - newHeight) * 0.5f;
  }
  m_vertex.SetRect(newPosX, newPosY, newPosX + newWidth, newPosY + newHeight);

  // scale the diffuse coords as well
  if (m_diffuse.size())
  { // calculate scaling for the texcoords
    if (m_diffuse.m_texCoordsArePixels)
    {
      m_diffuseU = float(m_diffuse.m_width);
      m_diffuseV = float(m_diffuse.m_height);
    }
    else
    {
      m_diffuseU = float(m_diffuse.m_width) / float(m_diffuse.m_texWidth);
      m_diffuseV = float(m_diffuse.m_height) / float(m_diffuse.m_texHeight);
    }

    if (m_aspect.scaleDiffuse)
    {
      m_diffuseScaleU = m_diffuseU;
      m_diffuseScaleV = m_diffuseV;
      m_diffuseOffset = CPoint(0,0);
    }
    else // stretch'ing diffuse
    { // scale diffuse up or down to match output rect size, rather than image size
      //(m_fX, mfY) -> (m_fX + m_fNW, m_fY + m_fNH)
      //(0,0) -> (m_fU*m_diffuseScaleU, m_fV*m_diffuseScaleV)
      // x = u/(m_fU*m_diffuseScaleU)*m_fNW + m_fX
      // -> u = (m_posX - m_fX) * m_fU * m_diffuseScaleU / m_fNW
      m_diffuseScaleU = m_diffuseU * m_vertex.Width() / m_width;
      m_diffuseScaleV = m_diffuseV * m_vertex.Height() / m_height;
      m_diffuseOffset = CPoint((m_vertex.x1 - m_posX) / m_vertex.Width() * m_diffuseScaleU, (m_vertex.y1 - m_posY) / m_vertex.Height() * m_diffuseScaleV);
    }
  }
}

void CGUITextureBase::FreeResources(bool immediately /* = false */)
{
  if (m_info.filename.size() > 0)
  {
    if (m_isAllocated == LARGE || m_isAllocated == LARGE_FAILED)
      g_largeTextureManager.ReleaseImage(m_info.filename, immediately);
    else if (m_isAllocated == NORMAL && m_texture.size())
      g_TextureManager.ReleaseTexture(m_info.filename);
  }

  if (m_diffuse.size())
    g_TextureManager.ReleaseTexture(m_info.diffuse);
  m_diffuse.Reset();

  m_texture.Reset();

  m_currentFrame = 0;
  m_currentLoop = 0;
  m_texCoordsScaleU = 1.0f;
  m_texCoordsScaleV = 1.0f;

  // call our implementation
  Free();

  m_isAllocated = NO;
}

void CGUITextureBase::DynamicResourceAlloc(bool allocateDynamically)
{
  m_allocateDynamically = allocateDynamically;
}

void CGUITextureBase::UpdateAnimFrame()
{
  m_frameCounter++;
  unsigned int delay = m_texture.m_delays[m_currentFrame];
  if (!delay) delay = 100;
  if (m_frameCounter * 40 >= delay)
  {
    m_frameCounter = 0;
    if (m_currentFrame + 1 >= m_texture.size())
    {
      if (m_texture.m_loops > 0)
      {
        if (m_currentLoop + 1 < m_texture.m_loops)
        {
          m_currentLoop++;
          m_currentFrame = 0;
        }
      }
      else
      {
        // 0 == loop forever
        m_currentFrame = 0;
      }
    }
    else
    {
      m_currentFrame++;
    }
  }
}

void CGUITextureBase::SetVisible(bool visible)
{
  m_visible = visible;
}

void CGUITextureBase::SetAlpha(unsigned char alpha)
{
  m_alpha = alpha;

  BuildColorData();
}

void CGUITextureBase::SetDiffuseColor(color_t color)
{
  m_diffuseColor = color;

  BuildColorData();
}

bool CGUITextureBase::ReadyToRender() const
{
  return m_texture.size() > 0;
}

void CGUITextureBase::OrientateTexture(CRect &rect, float width, float height, int orientation)
{
  switch (orientation & 3)
  {
  case 0:
    // default
    break;
  case 1:
    // flip in X direction
    rect.x1 = width - rect.x1;
    rect.x2 = width - rect.x2;
    break;
  case 2:
    // rotate 180 degrees
    rect.x1 = width - rect.x1;
    rect.x2 = width - rect.x2;
    rect.y1 = height - rect.y1;
    rect.y2 = height - rect.y2;
    break;
  case 3:
    // flip in Y direction
    rect.y1 = height - rect.y1;
    rect.y2 = height - rect.y2;
    break;
  }
  if (orientation & 4)
  {
    // we need to swap x and y coordinates but only within the width,height block
    float temp = rect.x1;
    rect.x1 = rect.y1 * width/height;
    rect.y1 = temp * height/width;
    temp = rect.x2;
    rect.x2 = rect.y2 * width/height;
    rect.y2 = temp * height/width;
  }
}

void CGUITextureBase::SetWidth(float width)
{
  if (width < m_info.border.x1 + m_info.border.x2)
    width = m_info.border.x1 + m_info.border.x2;
  if (m_width != width)
  {
    m_width = width;
    m_invalid = true;
  }
}

void CGUITextureBase::SetHeight(float height)
{
  if (height < m_info.border.y1 + m_info.border.y2)
    height = m_info.border.y1 + m_info.border.y2;
  if (m_height != height)
  {
    m_height = height;
    m_invalid = true;
  }
}

void CGUITextureBase::SetPosition(float posX, float posY)
{
  if (m_posX != posX || m_posY != posY)
  {
    m_posX = posX;
    m_posY = posY;
    m_invalid = true;
  }
}

void CGUITextureBase::SetAspectRatio(const CAspectRatio &aspect)
{
  if (m_aspect != aspect)
  {
    m_aspect = aspect;
    m_invalid = true;
  }
}

void CGUITextureBase::SetFileName(const CStdString& filename)
{
  if (m_info.filename.Equals(filename)) return;
  // Don't completely free resources here - we may be just changing
  // filenames mid-animation
  FreeResources();
  m_info.filename = filename;
  // Don't allocate resources here as this is done at render time
}

int CGUITextureBase::GetOrientation() const
{
  if (m_isAllocated == LARGE)
    return m_info.orientation;
  // otherwise multiply our orientations
  static char orient_table[] = { 0, 1, 2, 3, 4, 5, 6, 7,
                                 1, 0, 3, 2, 5, 4, 7, 6,
                                 2, 3, 0, 1, 6, 7, 4, 5,
                                 3, 2, 1, 0, 7, 6, 5, 4,
                                 4, 7, 6, 5, 0, 3, 2, 1,
                                 5, 6, 7, 4, 1, 2, 3, 0,
                                 6, 5, 4, 7, 2, 1, 0, 3,
                                 7, 4, 5, 6, 3, 0, 1, 2 };
  return (int)orient_table[8 * m_info.orientation + m_texture.m_orientation];
}

void CGUITextureBase::SetRenderBorderOnly(bool renderBorderOnly)
{
  m_renderBorderOnly = renderBorderOnly;
}
