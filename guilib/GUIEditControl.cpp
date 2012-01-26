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

#include "GUIEditControl.h"
#include "GUIWindowManager.h"
#include "utils/CharsetConverter.h"
#include "utils/Clipboard.h"
#include "StdString.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogNumeric.h"
#include "LocalizeStrings.h"
#include "DateTime.h"
#include "utils/AnnouncementManager.h"
#ifdef __APPLE__
#include "CocoaInterface.h"
#endif
#ifdef HAS_GLES
#include "GUITextureGLES.h"
#endif

const char* CGUIEditControl::smsLetters[10] = { " !@#$%^&*()[]{}<>/\\|0", ".,;:\'\"-+_=?`~1", "abc2", "def3", "ghi4", "jkl5", "mno6", "pqrs7", "tuv8", "wxyz9" };
const unsigned int CGUIEditControl::smsDelay = 1000;

using namespace std;

#ifdef WIN32
extern HWND g_hWnd;
#endif

#define EXPOSE_PASSWORD_LETTER_IN_MS 800

CGUIEditControl::CGUIEditControl(int parentID, int controlID, float posX, float posY,
                                 float width, float height, const CTextureInfo &textureFocus, const CTextureInfo &textureNoFocus,
                                 const CLabelInfo& labelInfo, const std::string &text, const CLabelInfo& emptylabelInfo, const std::string &emptyText, int maxTextSize)
    : CGUIButtonControl(parentID, controlID, posX, posY, width, height, textureFocus, textureNoFocus, labelInfo), m_emptyLayout(emptylabelInfo.font, false)
{
  ControlType = GUICONTROL_EDIT;
  m_textOffset = 0;
  m_textWidth = width;
  m_cursorPos = 0;
  m_cursorBlink = 0;
  m_inputHeading = 0;
  m_inputType = INPUT_TYPE_TEXT;
  m_smsLastKey = 0;
  m_smsKeyIndex = 0;
  m_highlighted = false;

  m_emptyLabelInfo = emptylabelInfo;
  CGUIInfoLabel info;
  info.SetLabel(emptyText,"");
  CStdStringW emptyTextW;
  g_charsetConverter.utf8ToW(info.GetLabel(GetParentID(), false), emptyTextW, false);
  m_emptyLayout.SetText(emptyTextW);

  m_maxTextSize = maxTextSize;
  std::string allowedText = GetAllowedText(text);
  SetLabel(allowedText);

  m_addToText2Counter = -1;
}

CGUIEditControl::CGUIEditControl(const CGUIButtonControl &button)
    : CGUIButtonControl(button), m_emptyLayout(m_textLayout)
{
  ControlType = GUICONTROL_EDIT;
  SetLabel(m_info.GetLabel(GetParentID()));
  m_textOffset = 0;
  m_textWidth = GetWidth();
  m_cursorPos = 0;
  m_cursorBlink = 0;
  m_smsLastKey = 0;
  m_smsKeyIndex = 0;
  m_maxTextSize = -1;
  m_highlighted = false;

  m_addToText2Counter = -1;
}

CGUIEditControl::~CGUIEditControl(void)
{
}

bool CGUIEditControl::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_SET_TYPE)
  {
    SetInputType((INPUT_TYPE)message.GetParam1(), (int)message.GetParam2());
    return true;
  }
  else if (message.GetMessage() == GUI_MSG_ITEM_SELECTED)
  {
    message.SetLabel(GetLabel2());
    return true;
  }
  else if (message.GetMessage() == GUI_MSG_SETFOCUS)
  {
    m_smsTimer.Stop();
    ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AF_GUI, "xbmc", "KeyboardDisplay");
  }
  else if (message.GetMessage() == GUI_MSG_LOSTFOCUS)
  {
    m_smsTimer.Stop();
    ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AF_GUI, "xbmc", "KeyboardHide");
  }

  return CGUIButtonControl::OnMessage(message);
}

bool CGUIEditControl::OnAction(const CAction &action)
{
  ValidateCursor();

  if (action.id == ACTION_BACKSPACE)
  {
    // backspace
    if (m_highlighted)
    {
      m_highlighted = false;
      m_text2.clear();
      m_cursorPos = 0;
      UpdateText();
    }
    else if (m_cursorPos)
    {
      m_text2.erase(--m_cursorPos, 1);
      UpdateText();
    }
    return true;
  }
  else if (action.id == ACTION_MOVE_LEFT)
  {
    if (m_highlighted)
    {
      m_cursorPos = 0;
      m_highlighted = false;
      UpdateText(false);
      return true;
    }

    if (m_cursorPos > 0)
    {
      m_cursorPos--;
      UpdateText(false);
      return true;
    }
  }
  else if (action.id == ACTION_MOVE_RIGHT)
  {
    if (m_highlighted)
    {
      m_highlighted = false;
      m_cursorPos = m_text2.size();
      UpdateText(false);
      return true;
    }

    if ((unsigned int) m_cursorPos < m_text2.size())
    { 
      m_cursorPos++;
      UpdateText(false);
      return true;
    }
  }
  else if (action.id == ACTION_PASTE)
  {
    m_highlighted = false;

    OnPasteClipboard();
  }
  else if (action.id >= KEY_VKEY && action.id < KEY_ASCII)
  {
    // input from the keyboard (vkey, not ascii)
    BYTE b = action.id & 0xFF;
    if (b == 0x25 && m_cursorPos > 0)
    { // left
      m_highlighted = false;
      m_cursorPos--;
      UpdateText(false);
      return true;
    }
    if (b == 0x27 && m_cursorPos < m_text2.length())
    { // right
      m_highlighted = false;
      m_cursorPos++;
      UpdateText(false);
      return true;
    }
    if (b == 0x2e)
    {
      if (m_cursorPos < m_text2.length())
      { // delete
        m_highlighted = false;
        m_text2.erase(m_cursorPos, 1);
        UpdateText();
        return true;
      }
    }
    if (b == 0x8)
    {
      if (m_highlighted)
      {
        m_text2.clear();
        m_cursorPos = 0;
        UpdateText();
        m_highlighted = false;
      }
      else if (m_cursorPos > 0)
      { // backspace
        m_text2.erase(--m_cursorPos, 1);
        UpdateText();      
      }    
      return true;
    }
  }
  else if (action.id >= KEY_ASCII)
  {
    // input from the keyboard
    switch (action.unicode) 
    {
    case '\t':
      break;
    case 10:
    case 13:
      {
        // enter - send click message, but otherwise ignore
        SEND_CLICK_MESSAGE(GetID(), GetParentID(), 1);
        return true;
      }
    case 27:
      { // escape - fallthrough to default action
        return CGUIButtonControl::OnAction(action);
      }
    case 8:
      {
        // backspace
        if (m_highlighted)
        {
          m_text2.clear();
          m_cursorPos = 0;
        }
        else if (m_cursorPos)
        {
          m_text2.erase(--m_cursorPos, 1);
        }
        break;
      }
    default:
      {
        if (CanAddToLabel2())
        {
          if (m_highlighted)
          {
            m_text2.clear();
            m_cursorPos = 0;
          }

          m_text2.insert(m_text2.begin() + m_cursorPos++, (WCHAR)action.unicode);
          m_addToText2Counter = 0;
        }
        break;
      }
    }
    m_highlighted = false;
    UpdateText();
    return true;
  }
  else if (action.id >= REMOTE_0 && action.id <= REMOTE_9)
  { // input from the remote
    if (m_highlighted)
    {
      m_text2.clear();
    }
    m_highlighted = false;
    if (m_inputType == INPUT_TYPE_FILTER)
    {
      if ((int)m_text2.size() < m_maxTextSize)
      {
        // filtering - use single number presses

        if (CanAddToLabel2())
        {
          m_text2.insert(m_text2.begin() + m_cursorPos++, L'0' + (action.id - REMOTE_0));
          m_addToText2Counter = 0;
          UpdateText();
        }
      }
    }
    else
      OnSMSCharacter(action.id - REMOTE_0);
    return true;
  }

  return CGUIButtonControl::OnAction(action);
}

void CGUIEditControl::OnClick()
{  
  // we received a click - it's not from the keyboard, so pop up the virtual keyboard, unless
  // that is where we reside!
  if (GetParentID() == WINDOW_DIALOG_KEYBOARD)
    return;

  CStdString utf8;
  g_charsetConverter.wToUTF8(m_text2, utf8);
  bool textChanged = false;
  CStdString heading = g_localizeStrings.Get(m_inputHeading ? m_inputHeading : 16028);
  switch (m_inputType)
  {
    case INPUT_TYPE_NUMBER:
      textChanged = CGUIDialogNumeric::ShowAndGetNumber(utf8, heading);
      break;
    case INPUT_TYPE_SECONDS:
      textChanged = CGUIDialogNumeric::ShowAndGetSeconds(utf8, g_localizeStrings.Get(21420));
      break;
    case INPUT_TYPE_DATE:
    {
      CDateTime dateTime;
      dateTime.SetFromDBDate(utf8);
      if (dateTime < CDateTime(2000,1, 1, 0, 0, 0))
        dateTime = CDateTime(2000, 1, 1, 0, 0, 0);
      SYSTEMTIME date;
      dateTime.GetAsSystemTime(date);
      if (CGUIDialogNumeric::ShowAndGetDate(date, g_localizeStrings.Get(21420)))
      {
        dateTime = CDateTime(date);
        utf8 = dateTime.GetAsDBDate();
        textChanged = true;
      }
      break;
    }
    case INPUT_TYPE_IPADDRESS:
      textChanged = CGUIDialogNumeric::ShowAndGetIPAddress(utf8, heading);
      break;
    case INPUT_TYPE_SEARCH:
      textChanged = CGUIDialogKeyboard::ShowAndGetFilter(utf8, true);
      break;
    case INPUT_TYPE_FILTER:
      textChanged = CGUIDialogKeyboard::ShowAndGetFilter(utf8, false);
      break;
    case INPUT_TYPE_TEXT:
    default:
      textChanged = CGUIDialogKeyboard::ShowAndGetInput(utf8, heading, true, m_inputType == INPUT_TYPE_PASSWORD);
      break;
  }
  if (textChanged)
  {
    g_charsetConverter.utf8ToW(utf8, m_text2, false);
    m_cursorPos = m_text2.size();
    UpdateText();
    m_cursorPos = m_text2.size();
  }
}

void CGUIEditControl::UpdateText(bool sendUpdate)
{
  m_smsTimer.Stop();
  if (sendUpdate)
  {
    SEND_CLICK_MESSAGE(GetID(), GetParentID(), 0);
    
    vector<CGUIActionDescriptor> textChangeActions = m_textChangeActions;
    for (unsigned int i = 0; i < textChangeActions.size(); i++)
    {
      CGUIMessage message(GUI_MSG_EXECUTE, GetID(), GetParentID());
      message.SetAction(textChangeActions[i]);
      g_windowManager.SendMessage(message);
    }
  }
  SetInvalid();
}

void CGUIEditControl::SetInputType(CGUIEditControl::INPUT_TYPE type, int heading)
{
  m_inputType = type;
  m_inputHeading = heading;
  SetInvalid();
  // TODO: Verify the current input string?
}

void CGUIEditControl::RecalcLabelPosition()
{
  if (!m_label.font) return;

  // ensure that our cursor is within our width
  ValidateCursor();

  CStdStringW text = GetDisplayedText();
  m_textWidth = m_textLayout2.GetTextWidth(text + L'|');
  m_textHeight = m_textLayout2.GetTextHeight(1);
  float beforeCursorWidth = m_textLayout2.GetTextWidth(text.Left(m_cursorPos));
  float afterCursorWidth = m_textLayout2.GetTextWidth(text.Left(m_cursorPos) + L'|');
  float leftTextWidth = m_textLayout.GetTextWidth();
  float maxTextWidth = m_width - m_label.offsetX*2;
  if (leftTextWidth > 0)
    maxTextWidth -= leftTextWidth + spaceWidth;

  // if skinner forgot to set height :p
  if (m_height == 0)
    m_height = 2*m_label.font->GetTextHeight(1);

  if (m_textWidth > maxTextWidth)
  { // we render taking up the full width, so make sure our cursor position is
    // within the render window
    if (m_textOffset + afterCursorWidth > maxTextWidth)
    { 
      // move the position to the left (outside of the viewport)
      m_textOffset = maxTextWidth - afterCursorWidth;
    }
    else if (m_textOffset + beforeCursorWidth < 0) // offscreen to the left
    {
      // otherwise use original position
      m_textOffset = -beforeCursorWidth;
    }
    else if (m_textOffset + m_textWidth < maxTextWidth)
    { // we have more text than we're allowed, but we aren't filling all the space
      m_textOffset = maxTextWidth - m_textWidth;
    }
  }
  else
    m_textOffset = 0;
}

void CGUIEditControl::RenderText()
{
  if (m_smsTimer.GetElapsedMilliseconds() > smsDelay)
    UpdateText();
  
  if (m_bInvalidated)
    RecalcLabelPosition();

  float leftTextWidth = m_textLayout.GetTextWidth();
  float maxTextWidth = m_width - m_label.offsetX * 2;

  // start by rendering the normal text
  float posX = m_posX + m_label.offsetX;
  float posY = m_posY;
  uint32_t align = m_label.align & XBFONT_CENTER_Y;

  if (m_label.align & XBFONT_CENTER_Y)
    posY += m_height*0.5f;

  if (leftTextWidth > 0)
  {
    // render the text on the left
    if (IsDisabled())
      m_textLayout.Render(posX, posY, m_label.angle, m_label.disabledColor, m_label.shadowColor, align, leftTextWidth, true);
    else if (HasFocus() && m_label.focusedColor)
      m_textLayout.Render(posX, posY, m_label.angle, m_label.focusedColor, m_label.shadowColor, align, leftTextWidth);
    else
      m_textLayout.Render(posX, posY, m_label.angle, m_label.textColor, m_label.shadowColor, align, leftTextWidth);

    posX += leftTextWidth + spaceWidth;
    maxTextWidth -= leftTextWidth + spaceWidth;
  }

  if (g_graphicsContext.SetClipRegion(posX, m_posY, maxTextWidth, m_height))
  {
    if (m_textWidth < maxTextWidth)
    { // align text as our text fits
      if (leftTextWidth > 0)
      { // right align as we have 2 labels
        posX = m_posX + m_width - m_label.offsetX;
        align |= XBFONT_RIGHT;
      }
      else
      { // align by whatever the skinner requests
        if (m_label.align & XBFONT_CENTER_X)
          posX += 0.5f*maxTextWidth;
        if (m_label.align & XBFONT_RIGHT)
          posX += maxTextWidth;
        align |= (m_label.align & 3);
      }
    }
    CStdStringW text = GetDisplayedText();
    // let's render it ourselves
    if (HasFocus() && !m_highlighted)
    { // cursor location assumes utf16 text, so deal with that (inefficient, but it's not as if it's a high-use area
      // virtual keyboard only)
      CStdStringW col;
      if ((SDL_GetTicks() % 800) <= 400)
        col.Format(L"|");
      else
        col.Format(L" ");
      text.Insert(m_cursorPos, col);
    }

    m_textLayout2.SetText(text);

    if (m_highlighted && m_text2.size() > 0)
    {
      float highlightY = posY;
      if (m_label.align & XBFONT_CENTER_Y)
        highlightY -=  m_textHeight * 0.5f;

      CRect rect(posX, highlightY, posX + m_textLayout2.GetTextWidth(), highlightY + m_textHeight);
#if defined(HAS_GLES)
      CGUITextureGLES::DrawQuad(rect, (color_t) m_label.selectedBackColor);
#elif defined(HAS_GL)
      CGUITextureGL::DrawQuad(rect, (color_t) m_label.selectedBackColor);
#elif defined(HAS_DX)
      CGUITextureD3D::DrawQuad(rect, (color_t) m_label.selectedBackColor);
#endif
    }

    if (IsDisabled())
      m_textLayout2.Render(posX + m_textOffset, posY, m_label.angle, m_label.disabledColor, m_label.shadowColor, align, m_textWidth, true);
    else if (m_highlighted)
      m_textLayout2.Render(posX + m_textOffset, posY, m_label.angle, m_label.selectedColor, m_label.shadowColor, align, m_textWidth);
    else if (HasFocus() && m_label.focusedColor)
      m_textLayout2.Render(posX + m_textOffset, posY, m_label.angle, m_label.focusedColor, m_label.shadowColor, align, m_textWidth);
    else
      m_textLayout2.Render(posX + m_textOffset, posY, m_label.angle, m_label.textColor, m_label.shadowColor, align, m_textWidth);

    if (m_text2.size() == 0)
    {
      m_emptyLayout.Render(posX + 5, posY, m_emptyLabelInfo.angle, m_emptyLabelInfo.textColor, m_emptyLabelInfo.shadowColor, align, m_emptyLayout.GetTextWidth(), true);
    }

    g_graphicsContext.RestoreClipRegion();
  }
}

CStdStringW CGUIEditControl::GetDisplayedText()
{
  if (m_inputType == INPUT_TYPE_PASSWORD)
  {
    CStdStringW text;

    if (!m_text2.empty() > 0 && m_addToText2Counter >= 0 && (m_addToText2Counter*50 < EXPOSE_PASSWORD_LETTER_IN_MS))
    {
      m_addToText2Counter++;
      text.append(m_text2.size()-1, L'\u2022');
      text += m_text2[m_text2.size()-1];
    }
    else
    {
      m_addToText2Counter = -1;
      text.append(m_text2.size(), L'\u2022');
    }
    return text;
  }
  return m_text2;
}

void CGUIEditControl::ValidateCursor()
{
  if (m_cursorPos > m_text2.size())
    m_cursorPos = m_text2.size();
}

void CGUIEditControl::SetLabel(const std::string &text)
{
  std::string textToSet;
  if ((int)text.size() > m_maxTextSize)
  {
    textToSet = text.substr(0,m_maxTextSize);
  }
  else
  {
    textToSet = text;
  }

  m_textLayout.Update(textToSet);
  SetInvalid();
}

void CGUIEditControl::SetLabel2(const std::string &text)
{
  std::string textToSet;
  if ((int)text.size() > m_maxTextSize)
  {
    textToSet = text.substr(0,m_maxTextSize);
  }
  else
  {
    textToSet = text;
  }

  CStdStringW newText;
  g_charsetConverter.utf8ToW(textToSet, newText, false);
  if (newText != m_text2)
  {
    m_text2 = newText;
    m_cursorPos = m_text2.size();
    SetInvalid();
  }
}

CStdString CGUIEditControl::GetLabel2() const
{
  CStdString text;
  g_charsetConverter.wToUTF8(m_text2, text);
  return text;
}

CStdStringW CGUIEditControl::GetUnicodeLabel() const
{
  return m_text2;
}

unsigned int CGUIEditControl::GetCursorPosition() const
{
  return m_cursorPos;
}

void CGUIEditControl::SetCursorPosition(unsigned int iPosition)
{
  m_cursorPos = iPosition;
}

void CGUIEditControl::OnSMSCharacter(unsigned int key)
{
  assert(key < 10);
  bool sendUpdate = false;
  if (m_smsTimer.IsRunning())
  { 
    // we're already entering an SMS character
    if (key != m_smsLastKey || m_smsTimer.GetElapsedMilliseconds() > smsDelay)
    { // a different key was clicked than last time, or we have timed out
      m_smsLastKey = key;
      m_smsKeyIndex = 0;
      sendUpdate = true;
    }
    else
    { // same key as last time within the appropriate time period
      m_smsKeyIndex++;
      if (m_cursorPos)
        m_text2.erase(--m_cursorPos, 1);
    }
  }
  else
  { // key is pressed for the first time
    m_smsLastKey = key;
    m_smsKeyIndex = 0;
  }
  
  m_smsKeyIndex = m_smsKeyIndex % strlen(smsLetters[key]);
  
  if (CanAddToLabel2())
  {
    m_text2.insert(m_text2.begin() + m_cursorPos++, smsLetters[key][m_smsKeyIndex]);
    UpdateText(sendUpdate);
    m_smsTimer.StartZero();
  }
}

void CGUIEditControl::OnPasteClipboard()
{
#ifdef __APPLE__
  const char *szStr = Cocoa_Paste();
  if (szStr)
  {
    m_text2 += GetAllowedText(std::string(szStr));
    m_cursorPos+=strlen(szStr);
    UpdateText();
  }
#elif defined _WIN32
  HGLOBAL   hglb;
  LPTSTR    lptstr;
  if (OpenClipboard(g_hWnd))
  {
    hglb = GetClipboardData(CF_TEXT);
    if (hglb != NULL)
    { 
      lptstr = (LPTSTR)GlobalLock(hglb);
      if (lptstr != NULL)
      { 
        m_text2 = GetAllowedText(std::string((char*)lptstr));
        GlobalUnlock(hglb);
      } 
    }
    CloseClipboard();
    UpdateText();
  }
#elif defined _LINUX
  CStdString result;
  
  CClipboard::Paste( result );
  if( !result.IsEmpty() )
  {
    m_text2 += GetAllowedText(result);
    m_cursorPos += result.GetLength();
    UpdateText();
  }
#endif
}

bool CGUIEditControl::CanAddToLabel2(int textToAddSize)
{
  if (m_maxTextSize == -1)
  {
    return true;
  }

  if ((m_maxTextSize > -1) && (((int)m_text2.size() + textToAddSize) <= m_maxTextSize))
  {
    return true;
  }

  return false;
}

std::string CGUIEditControl::GetAllowedText(const std::string& text)
{
  std::string allowedText;
  if ((m_maxTextSize > -1) && ((int)text.size() > m_maxTextSize))
  {
    allowedText = text.substr(0,m_maxTextSize);
  }
  else
  {
    allowedText = text;
  }

  return allowedText;
}

int CGUIEditControl::GetMaxInputSize()
{
  return m_maxTextSize;
}

bool CGUIEditControl::SetHighlighted(bool highlighted)
{
  m_highlighted = highlighted;
  return true;
}
