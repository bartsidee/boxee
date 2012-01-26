/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *      Test patterns designed by Ofer LaOr - hometheater.co.il
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
#include "GUIWindowTestPatternGLES.h"
#include "GUITextureGLES.h"

#ifdef HAS_GLES

CGUIWindowTestPatternGLES::CGUIWindowTestPatternGLES(void) : CGUIWindowTestPattern()
{
}

CGUIWindowTestPatternGLES::~CGUIWindowTestPatternGLES(void)
{
}

void CGUIWindowTestPatternGLES::DrawVerticalLines(int top, int left, int bottom, int right)
{
  for (int i = left; i <= right; i += 2)
  {
    CGUITextureGLES::DrawLine(CRect(i, top, i, bottom), 0xffffffff);
  }
}

void CGUIWindowTestPatternGLES::DrawHorizontalLines(int top, int left, int bottom, int right)
{
  for (int i = top; i <= bottom; i += 2)
  {
    CGUITextureGLES::DrawLine(CRect(left, i, right, i), 0xffffffff);
  }
}

void CGUIWindowTestPatternGLES::DrawCheckers(int top, int left, int bottom, int right)
{
}

void CGUIWindowTestPatternGLES::DrawBouncingRectangle(int top, int left, int bottom, int right)
{
  m_bounceX += m_bounceDirectionX;
  m_bounceY += m_bounceDirectionY;

  if ((m_bounceDirectionX == 1 && m_bounceX + TEST_PATTERNS_BOUNCE_SQUARE_SIZE == right) || (m_bounceDirectionX == -1 && m_bounceX == left))
    m_bounceDirectionX = -m_bounceDirectionX;

  if ((m_bounceDirectionY == 1 && m_bounceY + TEST_PATTERNS_BOUNCE_SQUARE_SIZE == bottom) || (m_bounceDirectionY == -1 && m_bounceY == top))
    m_bounceDirectionY = -m_bounceDirectionY;

  CGUITextureGLES::DrawQuad(CRect(m_bounceX, m_bounceY, m_bounceX + TEST_PATTERNS_BOUNCE_SQUARE_SIZE, m_bounceY + TEST_PATTERNS_BOUNCE_SQUARE_SIZE), 0xffffffff);
}

static DWORD circleColor = 0;

void CGUIWindowTestPatternGLES::DrawContrastBrightnessPattern(int top, int left, int bottom, int right)
{
  int x5p = (int) (left + (0.05f * (right - left)));
  int y5p = (int) (top + (0.05f * (bottom - top)));
  int x12p = (int) (left + (0.125f * (right - left)));
  int y12p = (int) (top + (0.125f * (bottom - top)));
  int x25p = (int) (left + (0.25f * (right - left)));
  int y25p = (int) (top + (0.25f * (bottom - top)));
  int x37p = (int) (left + (0.375f * (right - left)));
  int y37p = (int) (top + (0.375f * (bottom - top)));
  int x50p = left + (right - left) / 2;
  int y50p = top + (bottom - top) / 2;
  int x62p = (int) (left + (0.625f * (right - left)));
  int y62p = (int) (top + (0.625f * (bottom - top)));
  int x75p = (int) (left + (0.75f * (right - left)));
  int y75p = (int) (top + (0.75f * (bottom - top)));
  int x87p = (int) (left + (0.875f * (right - left)));
  int y87p = (int) (top + (0.875f * (bottom - top)));
  int x95p = (int) (left + (0.95f * (right - left)));
  int y95p = (int) (top + (0.95f * (bottom - top)));

  m_blinkFrame = (m_blinkFrame + 1) % TEST_PATTERNS_BLINK_CYCLE;

  // draw main quadrants
  CGUITextureGLES::DrawQuad(CRect(x50p, top, right, y50p), 0xffffffff);
  CGUITextureGLES::DrawQuad(CRect(left, y50p, x50p, bottom), 0xffffffff);

  // draw border lines
  CGUITextureGLES::DrawLine(CRect(left, y5p, x50p, y5p), 0xffffffff);
  CGUITextureGLES::DrawLine(CRect(x5p, top,x5p, y50p), 0xffffffff);
  CGUITextureGLES::DrawLine(CRect(x50p, y95p, right, y95p), 0xffffffff);
  CGUITextureGLES::DrawLine(CRect(x95p, y50p, x95p, bottom), 0xffffffff);

  CGUITextureGLES::DrawLine(CRect(x50p, y5p, right, y5p), 0xff000000);
  CGUITextureGLES::DrawLine(CRect(x5p, y50p, x5p, bottom), 0xff000000);
  CGUITextureGLES::DrawLine(CRect(left, y95p, x50p, y95p), 0xff000000);
  CGUITextureGLES::DrawLine(CRect(x95p, top, x95p, y50p), 0xff000000);

  // draw inner rectangles
  CGUITextureGLES::DrawQuad(CRect(x12p, y12p, x37p, y37p), 0xffffffff);
  CGUITextureGLES::DrawQuad(CRect(x62p, y62p, x87p, y87p), 0xffffffff);

  CGUITextureGLES::DrawQuad(CRect(x62p, y12p, x87p, y37p), 0xff000000);
  CGUITextureGLES::DrawQuad(CRect(x12p, y62p, x37p, y87p), 0xff000000);

  // draw inner circles
  if (m_blinkFrame < TEST_PATTERNS_BLINK_CYCLE / 2)
    circleColor = 0xff050505;
  else
    circleColor = 0xff000000;

  int radius = (y37p - y12p) / 3;
  CGUITextureGLES::DrawQuad(CRect(x25p - radius, y75p - radius, x25p + radius, y75p + radius), circleColor);
  CGUITextureGLES::DrawQuad(CRect(x75p - radius, y25p - radius, x75p + radius, y25p + radius), circleColor);

  if (m_blinkFrame < TEST_PATTERNS_BLINK_CYCLE / 2)
    circleColor = 0xfffafafa;
  else
    circleColor = 0xffffffff;
  CGUITextureGLES::DrawQuad(CRect(x25p - radius, y25p - radius, x25p + radius, y25p + radius), circleColor);
  CGUITextureGLES::DrawQuad(CRect(x75p - radius, y75p - radius, x75p + radius, y75p + radius), circleColor);
}

void CGUIWindowTestPatternGLES::BeginRender()
{
  glClearColor(0, 0, 0, 255);
  glClear(GL_COLOR_BUFFER_BIT);

  g_graphicsContext.PushTransform(TransformMatrix(), true);
}

void CGUIWindowTestPatternGLES::EndRender()
{
  g_graphicsContext.PopTransform();
}

#endif
