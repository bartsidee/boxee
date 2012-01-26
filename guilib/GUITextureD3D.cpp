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

#include "Texture.h"
#include "GUITextureD3D.h"
#include "WindowingFactory.h"

#ifdef HAS_DX


CGUITextureD3D::CGUITextureD3D(float posX, float posY, float width, float height, const CTextureInfo& texture, CBaseTexture* textureData)
: CGUITextureBase(posX, posY, width, height, texture, textureData)
{
}

void CGUITextureD3D::Begin()
{
  CBaseTexture* texture = m_texture.m_textures[m_currentFrame];
  LPDIRECT3DDEVICE9 p3DDevice = g_Windowing.Get3DDevice();

  texture->LoadToGPU();
  if (m_diffuse.size())
    m_diffuse.m_textures[0]->LoadToGPU();
  // Set state to render the image
  p3DDevice->SetTexture( 0, texture->GetTextureObject() );
  p3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  p3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  p3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
  p3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
  p3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
  p3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
  p3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
  p3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
  p3DDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
  p3DDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
  if (m_diffuse.size())
  {
    p3DDevice->SetTexture( 1, m_diffuse.m_textures[0]->GetTextureObject() );
    p3DDevice->SetSamplerState( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    p3DDevice->SetSamplerState( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    p3DDevice->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    p3DDevice->SetTextureStageState( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );
    p3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_MODULATE );
    p3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    p3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT );
    p3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
    p3DDevice->SetSamplerState( 1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    p3DDevice->SetSamplerState( 1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
  }

  p3DDevice->SetFVF( D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2 );

  TransformMatrix* matModelView = g_Windowing.GetHardwareTransform(MATRIX_TYPE_MODEL_VIEW);
  TransformMatrix* matProjection = g_Windowing.GetHardwareTransform(MATRIX_TYPE_PROJECTION);
  p3DDevice->SetTransform(D3DTS_VIEW, &D3DXMATRIX((float *)matModelView->m));
  p3DDevice->SetTransform(D3DTS_PROJECTION, &D3DXMATRIX((float *)matProjection->m));
}

void CGUITextureD3D::End()
{
  // unset the texture and palette or the texture caching crashes because the runtime still has a reference
  g_Windowing.Get3DDevice()->SetTexture( 0, NULL );
  if (m_diffuse.size())
    g_Windowing.Get3DDevice()->SetTexture( 1, NULL );
}

void CGUITextureD3D::Draw(ImageCoords& coords)
{
  struct CUSTOMVERTEX {
      FLOAT x, y, z;
      DWORD color;
      FLOAT tu, tv;   // Texture coordinates
      FLOAT tu2, tv2;
  };

  CUSTOMVERTEX verts[4];

  // D3D aligns to half pixel boundaries
  for (int i = 0; i < 4; i++)
  {
    verts[i].x = coords.m_pCoords[i].x;
    verts[i].y = coords.m_pCoords[i].y;
    verts[i].z = coords.m_pCoords[i].z;
    verts[i].tu = coords.m_pCoords[i].u1;
    verts[i].tv = coords.m_pCoords[i].v1;
    verts[i].tu2 = coords.m_pCoords[i].u2;
    verts[i].tv2 = coords.m_pCoords[i].v2;
    verts[i].color = m_diffuseColorBlended;
  };

  g_Windowing.Get3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(CUSTOMVERTEX));
}
/*
void CGUITextureD3D::Draw(ImageCoords& coords)
{
  struct CUSTOMVERTEX {
      FLOAT x, y, z;
      DWORD color;
      FLOAT tu, tv;   // Texture coordinates
      FLOAT tu2, tv2;
  };

  CUSTOMVERTEX verts[4];

  // D3D aligns to half pixel boundaries
  for (int i = 0; i < 4; i++)
  {
    verts[i].x = coords.m_pCoords[i].x;
    verts[i].y = coords.m_pCoords[i].y;
    verts[i].z = coords.m_pCoords[i].z;
    verts[i].tu = coords.m_pCoords[i].u1;
    verts[i].tv = coords.m_pCoords[i].v1;
    verts[i].tu2 = coords.m_pCoords[i].u2;
    verts[i].tv2 = coords.m_pCoords[i].v2;
    verts[i].color = m_diffuseColor;
  };

  g_Windowing.Get3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(CUSTOMVERTEX));
}*/

void CGUITextureD3D::DrawQuad(const CRect &rect, color_t color, bool bWireframe, CBaseTexture *texture, const CRect *texCoords)
{

  struct CUSTOMVERTEX {
      FLOAT x, y, z;
      DWORD color;
      FLOAT tu, tv;   // Texture coordinates
      FLOAT tu2, tv2;
  };

  LPDIRECT3DDEVICE9 p3DDevice = g_Windowing.Get3DDevice();

  if (texture)
  {
    texture->LoadToGPU();
    // Set state to render the image
    p3DDevice->SetTexture( 0, texture->GetTextureObject() );
    p3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    p3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    p3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    p3DDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    p3DDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
    p3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    p3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
  }

  p3DDevice->SetRenderState( D3DRS_ALPHATESTENABLE, TRUE );
  p3DDevice->SetRenderState( D3DRS_ALPHAREF, 0 );
  p3DDevice->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );
  p3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
  p3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
  p3DDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
  p3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  p3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
  p3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  p3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
  p3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
  p3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE);

  p3DDevice->SetFVF( D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2 );

  CRect coords = texCoords ? *texCoords : CRect(0.0f, 0.0f, 1.0f, 1.0f);
  CUSTOMVERTEX verts[4] = {
    { rect.x1 - 0.5f, rect.y1 - 0.5f, 0, color, coords.x1, coords.y1, 0, 0 },
    { rect.x2 - 0.5f, rect.y1 - 0.5f, 0, color, coords.x2, coords.y1, 0, 0 },
    { rect.x2 - 0.5f, rect.y2 - 0.5f, 0, color, coords.x2, coords.y2, 0, 0 },
    { rect.x1 - 0.5f, rect.y2 - 0.5f, 0, color, coords.x1, coords.y2, 0, 0 },
  };
  p3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(CUSTOMVERTEX));

  p3DDevice->SetTexture( 0, NULL );
}


#endif
