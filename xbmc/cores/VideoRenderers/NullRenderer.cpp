/*
 * XBMC Media Center
 * Linux OpenGL Renderer
 * Copyright (c) 2007 Frodo/jcmarshall/vulkanr/d4rk
 *
 * Based on XBoxRenderer by Frodo/jcmarshall
 * Portions Copyright (c) by the authors of ffmpeg / xvid /mplayer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "system.h"

#ifdef HAS_NULL_RENDERER

#include "NullRenderer.h"

CNullRenderer::CNullRenderer()
{
}

CNullRenderer::~CNullRenderer()
{
}

unsigned int CNullRenderer::PreInit()
{
  return true;
}

void CNullRenderer::UnInit()
{
}

void CNullRenderer::SetSpeed(int speed)
{
}

void CNullRenderer::Flush()
{
}

bool CNullRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, CRect &rect)
{
  return true;
}

int CNullRenderer::GetImage(YV12Image *image, double pts, int source, bool readonly)
{
  return 0;
}

void CNullRenderer::ReleaseImage(int source, bool preserve)
{
}

void CNullRenderer::Reset()
{
}

void CNullRenderer::Update(bool bPauseDrawing)
{
}

void CNullRenderer::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
}

unsigned int CNullRenderer::DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y)
{
  return 0;
}

bool CNullRenderer::SupportsBrightness()
{
  return false;
}

bool CNullRenderer::SupportsContrast()
{
  return false;
}

bool CNullRenderer::SupportsGamma()
{
  return false;
}

#endif

