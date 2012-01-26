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

#include "GUIWebControl.h"
#ifdef HAS_EMBEDDED
#include "GUITextureGLES.h"
#endif
#include "utils/log.h"

CGUIWebControl::CGUIWebControl(int parentID, int controlID, float posX, float posY, float width, float height, const CStdString& url)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
    , m_url(url), m_rect(posX, posY, posX + width, posY + height), m_ended(false)
{
  ControlType = GUICONTROL_WEB;
  m_browser = new CFlashVideoPlayer(*this);
  float z1 = 0.0, z2 = 0.0;
  g_graphicsContext.GetGuiTransform().TransformPosition(m_rect.x1, m_rect.y1, z1);
  g_graphicsContext.GetGuiTransform().TransformPosition(m_rect.x2, m_rect.y2, z2);
}

CGUIWebControl::~CGUIWebControl(void)
{
  if (m_browser)
  {
    delete m_browser;
    m_browser = NULL;
  }
}

void CGUIWebControl::Render()
{
  if (!IsVisible()) return;

  if (!m_ended)
  {
  // Draw transparent area
#ifdef HAS_INTEL_SMD
    g_graphicsContext.PushTransform(TransformMatrix(), true);
    CGUITextureGLES::DrawQuad(m_rect, 0xff);
    g_graphicsContext.PopTransform();
#endif
  }
  CGUIControl::Render();
}

bool CGUIWebControl::OnAction(const CAction &action)
{
  switch (action.id)
  {
    case ACTION_MOVE_LEFT:
    {
      CAction a(ACTION_STEP_BACK);
      return m_browser->OnAction(a);
    }
    case ACTION_MOVE_RIGHT:
    {
      CAction a(ACTION_STEP_FORWARD);
      return m_browser->OnAction(a);
    }
  }

  if (m_browser->OnAction(action))
    return true;

  return CGUIControl::OnAction(action);
}

bool CGUIWebControl::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_SET_FILENAME)
  {
    SetURL(message.GetStringParam());
    return true;
  }

  return CGUIControl::OnMessage(message);
}

const CStdString &CGUIWebControl::GetURL() const
{
  return m_url;
}

void CGUIWebControl::SetURL(const CStdString& url)
{
  m_url = url;
}

void CGUIWebControl::LoadUrl(const CStdString& url/* = ""*/)
{
  if (!url.IsEmpty())
    SetURL(url);

  m_browser->LoadURL(m_url, m_rect);
}

CStdString CGUIWebControl::GetDescription(void) const
{
  return GetURL();
}

bool CGUIWebControl::OnMouseOver(const CPoint &point)
{
  return true;
}

void CGUIWebControl::OnPlayBackEnded(bool bError/* = false*/, const CStdString& error/* = ""*/)
{
  m_ended = true;
  CGUIMessage playbackEnded(GUI_MSG_WINDOW_DEINIT,GetID(),0,bError,0);
  playbackEnded.SetStringParam(error);
  SendWindowMessage(playbackEnded);
}
