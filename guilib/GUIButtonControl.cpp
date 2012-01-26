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

#include "GUIButtonControl.h"
#include "GUIWindowManager.h"
#include "GUIDialog.h"
#include "utils/CharsetConverter.h"
#include "GUIFontManager.h"
#include "Application.h"
#include "MouseStat.h"

using namespace std;

CGUIButtonControl::CGUIButtonControl(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus, const CLabelInfo& labelInfo)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
    , m_imgFocus(posX, posY, width, height, textureFocus)
    , m_imgNoFocus(posX, posY, width, height, textureNoFocus)
    , m_textLayout(labelInfo.font, false), m_textLayout2(labelInfo.font, false)
{
  m_bSelected = false;
  m_alpha = 255;
  m_focusCounter = 0;
  m_label = labelInfo;
  ControlType = GUICONTROL_BUTTON;
  m_clickAnimationStarted = false;  
  m_imgClick = NULL;
}

CGUIButtonControl::~CGUIButtonControl(void)
{
  if (m_imgClick)
  {
    delete m_imgClick;
  }
}

CGUIButtonControl::CGUIButtonControl(const CGUIButtonControl &copy) : CGUIControl(copy),
  m_imgFocus(copy.m_imgFocus),
  m_imgNoFocus(copy.m_imgNoFocus),
  m_textLayout(copy.m_textLayout),
  m_textLayout2(copy.m_textLayout)
{
  if (copy.m_imgClick)
    m_imgClick = new CGUITexture(*copy.m_imgClick);
  else 
    m_imgClick = NULL;
  
  m_focusCounter = copy.m_focusCounter;
  m_alpha = copy.m_alpha;
  m_info = copy.m_info;
  m_info2 = copy.m_info2;
  m_label = copy.m_label;
  m_clickActions = copy.m_clickActions;
  m_focusActions = copy.m_focusActions;
  m_unfocusActions = copy.m_unfocusActions;
  m_bSelected = copy.m_bSelected;
  m_clickAnimationStarted = copy.m_clickAnimationStarted;
  m_clickAction = copy.m_clickAction;    
  m_clickVisibleDuration = copy.m_clickVisibleDuration;
  m_clickTime = copy.m_clickTime;
  m_renderTime = copy.m_renderTime;
}

CGUIButtonControl *CGUIButtonControl::Clone() const
{ 
  CGUIButtonControl *newControl = new CGUIButtonControl(*this);
  if (m_imgClick)
  {
    newControl->m_imgClick = new CGUITexture(*m_imgClick);
  }
  return newControl; 
}

void CGUIButtonControl::DoRender(unsigned int currentTime)
{
  m_renderTime = currentTime;
  CGUIControl::DoRender(currentTime);
}

void CGUIButtonControl::Render()
{
  if (m_clickAnimationStarted && m_imgClick == NULL && !IsAnimating(ANIM_TYPE_CLICK))
  {
    m_clickAnimationStarted = false;
    g_application.DeferAction(m_clickAction);
  }

  if (m_bInvalidated)
  {
    m_imgFocus.SetWidth(m_width);
    m_imgFocus.SetHeight(m_height);

    m_imgNoFocus.SetWidth(m_width);
    m_imgNoFocus.SetHeight(m_height);
    
    if (m_imgClick)
    {
      m_imgClick->SetWidth(m_width);
      m_imgClick->SetHeight(m_height);
    }
  }

  if (m_clickAnimationStarted && m_imgClick != NULL)
  {
    m_imgFocus.SetVisible(false);
    m_imgNoFocus.SetVisible(false);
    if (m_imgClick) m_imgClick->SetVisible(true);
    
    if (m_renderTime > m_clickTime + m_clickVisibleDuration)
    {
      m_clickAnimationStarted = false;
      g_application.DeferAction(m_clickAction);      
    }    
  }
  else if (HasFocus())
  {
    if (m_pulseOnSelect)
    {
      unsigned int alphaCounter = m_focusCounter + 2;
      unsigned int alphaChannel;
      if ((alphaCounter % 128) >= 64)
        alphaChannel = alphaCounter % 64;
      else
        alphaChannel = 63 - (alphaCounter % 64);

      alphaChannel += 192;
      alphaChannel = (unsigned int)((float)m_alpha * (float)alphaChannel / 255.0f);
      m_imgFocus.SetAlpha((unsigned char)alphaChannel);
    }
    m_imgFocus.SetVisible(true);
    m_imgNoFocus.SetVisible(false);
    if (m_imgClick) m_imgClick->SetVisible(false);
    m_focusCounter++;
  }
  else
  {
    m_imgFocus.SetVisible(false);
    m_imgNoFocus.SetVisible(true);
    if (m_imgClick) m_imgClick->SetVisible(false);
  }
  // render both so the visibility settings cause the frame counter to resetcorrectly
  m_imgFocus.Render();
  m_imgNoFocus.Render();
  if (m_imgClick) m_imgClick->Render();

  RenderText();
  CGUIControl::Render();
}

void CGUIButtonControl::RenderText()
{
  m_textLayout.Update(m_info.GetLabel(m_parentID));

  float fPosX = m_posX + m_label.offsetX;
  float fPosY = m_posY + m_label.offsetY;

  if (m_label.align & XBFONT_RIGHT)
    fPosX = m_posX + m_width - m_label.offsetX;

  if (m_label.align & XBFONT_CENTER_X)
    fPosX = m_posX + m_width / 2;

  if (m_label.align & XBFONT_CENTER_Y)
    fPosY = m_posY + m_height / 2;

  if (IsDisabled())
    m_textLayout.Render( fPosX, fPosY, m_label.angle, m_label.disabledColor, m_label.shadowColor, m_label.align, m_label.width, true);
  else if (HasFocus() && m_label.focusedColor)
    m_textLayout.Render( fPosX, fPosY, m_label.angle, m_label.focusedColor, m_label.shadowColor, m_label.align, m_label.width);
  else
    m_textLayout.Render( fPosX, fPosY, m_label.angle, m_label.textColor, m_label.shadowColor, m_label.align, m_label.width);

  // render the second label if it exists
  CStdString label2(m_info2.GetLabel(m_parentID));
  if (!label2.IsEmpty())
  {
    float textWidth, textHeight;
    m_textLayout.GetTextExtent(textWidth, textHeight);
    m_textLayout2.Update(label2);

    float width = m_width - 2 * m_label.offsetX - textWidth - 5;
    if (width < 0) width = 0;
    fPosX = m_posX + m_width - m_label.offsetX;
    uint32_t dwAlign = XBFONT_RIGHT | (m_label.align & XBFONT_CENTER_Y) | XBFONT_TRUNCATED;

    if (IsDisabled() )
      m_textLayout2.Render( fPosX, fPosY, m_label.angle, m_label.disabledColor, m_label.shadowColor, dwAlign, width, true);
    else if (HasFocus() && m_label.focusedColor)
      m_textLayout2.Render( fPosX, fPosY, m_label.angle, m_label.focusedColor, m_label.shadowColor, dwAlign, width);
    else
      m_textLayout2.Render( fPosX, fPosY, m_label.angle, m_label.textColor, m_label.shadowColor, dwAlign, width);
  }
}

bool CGUIButtonControl::OnAction(const CAction &action)
{
  if (action.id == ACTION_SELECT_ITEM || action.id == ACTION_MOUSE_LEFT_CLICK)
  {
    if (action.id && !m_clickAnimationStarted)
    {
      if (m_imgClick)
      {
        m_clickAnimationStarted = true;        
        m_clickAction = action;
        m_clickAction.originalwID = m_clickAction.id;
        m_clickAction.id = ACTION_POST_ANIM_CLICK;
        m_clickTime = m_renderTime;
      }         
      else if (HasAnimation(ANIM_TYPE_CLICK))
      {
        ResetAnimation(ANIM_TYPE_CLICK);
        QueueAnimation(ANIM_TYPE_CLICK);
        m_clickAnimationStarted = true;        
        m_clickAction = action;
        m_clickAction.originalwID = m_clickAction.id;
        m_clickAction.id = ACTION_POST_ANIM_CLICK;
      }   
      else
      {
        OnClick();
      }
      return true;
    }
  }
  else if (action.id == ACTION_POST_ANIM_CLICK)
  {
    OnClick();
    return true;
  }
  
  return CGUIControl::OnAction(action);
}

bool CGUIButtonControl::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID())
  {
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      SetLabel(message.GetLabel());
      return true;
    }
    if (message.GetMessage() == GUI_MSG_LABEL2_SET)
    {
      SetLabel2(message.GetLabel());
      return true;
    }
    if (message.GetMessage() == GUI_MSG_GET_LABEL)
    {
      message.SetLabel(GetLabel());
      return true;
    }
    if (message.GetMessage() == GUI_MSG_GET_LABEL2)
    {
      message.SetLabel(GetLabel2());
      return true;
    }
    if (message.GetMessage() == GUI_MSG_SELECTED)
    {
      m_bSelected = true;
      return true;
    }
    if (message.GetMessage() == GUI_MSG_DESELECTED)
    {
      m_bSelected = false;
      return true;
    }
    if (message.GetMessage() == GUI_MSG_ITEM_SELECTED)
    {
      message.SetParam1(m_bSelected ? 1 : 0);
      return true;
  }
  }

  return CGUIControl::OnMessage(message);
}

void CGUIButtonControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_focusCounter = 0;
  m_imgFocus.AllocResources();
  m_imgNoFocus.AllocResources();
  if (m_imgClick)
    m_imgClick->AllocResources();
  if (!m_width)
    m_width = m_imgFocus.GetWidth();
  if (!m_height)
    m_height = m_imgFocus.GetHeight();
}

void CGUIButtonControl::FreeResources(bool immediately)
{
  CGUIControl::FreeResources(immediately);
  m_imgFocus.FreeResources(immediately);
  m_imgNoFocus.FreeResources(immediately);
  if (m_imgClick)
    m_imgClick->FreeResources(immediately);
}

void CGUIButtonControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_imgFocus.DynamicResourceAlloc(bOnOff);
  m_imgNoFocus.DynamicResourceAlloc(bOnOff);
  if (m_imgClick)
    m_imgClick->DynamicResourceAlloc(bOnOff);
}

void CGUIButtonControl::SetLabel(const string &label)
{ // NOTE: No fallback for buttons at this point
  m_info.SetLabel(label, "");
}

void CGUIButtonControl::SetLabel2(const string &label2)
{ // NOTE: No fallback for buttons at this point
  m_info2.SetLabel(label2, "");
}

void CGUIButtonControl::SetPosition(float posX, float posY)
{
  CGUIControl::SetPosition(posX, posY);

  m_imgFocus.SetPosition(posX, posY);
  m_imgNoFocus.SetPosition(posX, posY);
  if (m_imgClick)
    m_imgClick->SetPosition(posX, posY);
}

void CGUIButtonControl::SetWidth(float width)
{
  CGUIControl::SetWidth(width);

  m_imgFocus.SetWidth(width);
  m_imgNoFocus.SetWidth(width);
  if (m_imgClick)
    m_imgClick->SetWidth(width);
}

void CGUIButtonControl::SetAlpha(unsigned char alpha)
{
  m_alpha = alpha;
  m_imgFocus.SetAlpha(alpha);
  m_imgNoFocus.SetAlpha(alpha);
  if (m_imgClick)
    m_imgClick->SetAlpha(alpha);
}

void CGUIButtonControl::UpdateColors()
{
  m_label.UpdateColors();
  CGUIControl::UpdateColors();
  m_imgFocus.SetDiffuseColor(m_diffuseColor);
  m_imgNoFocus.SetDiffuseColor(m_diffuseColor);
  if (m_imgClick)
    m_imgClick->SetDiffuseColor(m_diffuseColor);
}

bool CGUIButtonControl::OnMouseClick(int button, const CPoint &point)
{
  if (button == MOUSE_LEFT_BUTTON)
  {
    g_Mouse.SetState(MOUSE_STATE_CLICK);
    CAction action;
    action.id = ACTION_SELECT_ITEM;
    return OnAction(action);
    //return true;
  }
  return false;
}

CStdString CGUIButtonControl::GetDescription() const
{
  CStdString strLabel(m_info.GetLabel(m_parentID));
  return strLabel;
}

CStdString CGUIButtonControl::GetLabel2() const
{
  CStdString strLabel(m_info2.GetLabel(m_parentID));
  return strLabel;
}

void CGUIButtonControl::PythonSetLabel(const CStdString &strFont, const string &strText, color_t textColor, color_t shadowColor, color_t focusedColor)
{
  m_label.font = g_fontManager.GetFont(strFont);
  m_label.textColor = textColor;
  m_label.focusedColor = focusedColor;
  m_label.shadowColor = shadowColor;
  SetLabel(strText);
}

void CGUIButtonControl::PythonSetDisabledColor(color_t disabledColor)
{
  m_label.disabledColor = disabledColor;
}

void CGUIButtonControl::RAMSetTextColor(color_t textColor)
{
  m_label.textColor = textColor;
}

void CGUIButtonControl::SettingsCategorySetTextAlign(uint32_t align)
{
  m_label.align = align;
}

void CGUIButtonControl::OnClick()
{
  // Save values, as the click message may deactivate the window
  int controlID = GetID();
  int parentID = GetParentID();
  vector<CGUIActionDescriptor> clickActions = m_clickActions;

  // button selected, send a message
  CGUIMessage msg(GUI_MSG_CLICKED, controlID, parentID, 0);
  SendWindowMessage(msg);

  // and execute our actions
  for (unsigned int i = 0; i < clickActions.size(); i++)
  {
    CGUIMessage message(GUI_MSG_EXECUTE, controlID, parentID);
    message.SetAction(clickActions[i]);
    g_windowManager.SendMessage(message);
  }
}

void CGUIButtonControl::OnFocus()
{
  for (unsigned int i = 0; i < m_focusActions.size(); i++)
  {
    CGUIMessage message(GUI_MSG_EXECUTE, m_controlID, m_parentID);
    message.SetAction(m_focusActions[i]);
    g_windowManager.SendThreadMessage(message);
  }
}

void CGUIButtonControl::OnUnFocus()
{
  for (unsigned int i = 0; i < m_unfocusActions.size(); i++)
  {
    CGUIMessage message(GUI_MSG_EXECUTE, m_controlID, m_parentID);
    message.SetAction(m_unfocusActions[i]);
    g_windowManager.SendThreadMessage(message);
  }
}

void CGUIButtonControl::SetSelected(bool bSelected)
{
  if (m_bSelected != bSelected)
  {
    m_bSelected = bSelected;
    SetInvalid();
  }
}

bool CGUIButtonControl::IsSelected()
{
  return m_bSelected;
}

void CGUIButtonControl::SetClickTexture(const CTextureInfo& textureClick, int visibleDuration)
{
  if (m_imgClick)
  {
    delete m_imgClick;
  }
  
  m_imgClick = new CGUITexture(GetXPosition(), GetYPosition(), GetWidth(), GetHeight(), textureClick);
  m_clickVisibleDuration = visibleDuration;
}
