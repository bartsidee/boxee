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

#include "RenderSystem.h"

CRenderSystemBase::CRenderSystemBase()
{
  m_bRenderCreated = false;
  m_bVSync = true;
  m_maxTextureSize = 2048;
  m_RenderVersionMajor = 0;
  m_RenderVersionMinor = 0;
  m_nShaderModelVersionMajor = 0;
  m_nShaderModelVersionMinor = 0;
  m_renderCaps = 0;
  m_strErrorMessage = "";
  m_textureMemorySize = 0;
}

CRenderSystemBase::~CRenderSystemBase()
{

}

void CRenderSystemBase::GetRenderVersion(unsigned int& major, unsigned int& minor) const
{
  major = m_RenderVersionMajor;
  minor = m_RenderVersionMinor;
}

void CRenderSystemBase::GetShaderVersion(unsigned int& major, unsigned int& minor) const
{
  major = m_nShaderModelVersionMajor;
  minor = m_nShaderModelVersionMinor;
}

bool CRenderSystemBase::SupportsNPOT(bool dxt) const
{
  if (dxt)
    return (m_renderCaps & RENDER_CAPS_DXT_NPOT);
  return m_renderCaps & RENDER_CAPS_NPOT;
}

bool CRenderSystemBase::SupportsRT_NPOT() const
{
  return m_renderCaps & RENDER_CAPS_RT_NPOT;
}

bool CRenderSystemBase::SupportsDXT() const
{
  return m_renderCaps & RENDER_CAPS_DXT;
}
