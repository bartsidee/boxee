/*!
\file GUIEditControl.h
\brief 
*/

#ifndef GUILIB_GUIEditControl_H
#define GUILIB_GUIEditControl_H

#pragma once

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
#include "utils/Stopwatch.h"

/*!
 \ingroup controls
 \brief 
 */

class CGUIEditControl : public CGUIButtonControl
{
public:
  enum INPUT_TYPE {
                    INPUT_TYPE_TEXT = 0,
                    INPUT_TYPE_NUMBER,
                    INPUT_TYPE_SECONDS,
                    INPUT_TYPE_DATE,
                    INPUT_TYPE_IPADDRESS,
                    INPUT_TYPE_PASSWORD,
                    INPUT_TYPE_SEARCH,
                    INPUT_TYPE_FILTER
                  };

  CGUIEditControl(int parentID, int controlID, float posX, float posY,
                  float width, float height, const CTextureInfo &textureFocus, const CTextureInfo &textureNoFocus,
                  const CLabelInfo& labelInfo, const std::string &text, const CLabelInfo& emptylabelInfo, const std::string &emptyText, int maxTextSize = -1);
  CGUIEditControl(const CGUIButtonControl &button);
  virtual ~CGUIEditControl(void);
  virtual CGUIEditControl *Clone() const { return new CGUIEditControl(*this); };

  virtual bool OnMessage(CGUIMessage &message);
  virtual bool OnAction(const CAction &action);
  virtual void OnClick();

  virtual void SetLabel(const std::string &text);
  virtual void SetLabel2(const std::string &text);

  virtual CStdString GetLabel2() const;
  virtual CStdStringW GetUnicodeLabel() const;
  
  unsigned int GetCursorPosition() const;
  void SetCursorPosition(unsigned int iPosition);

  void SetInputType(INPUT_TYPE type, int heading);
  INPUT_TYPE GetInputType() { return m_inputType; }
  void SetTextChangeActions(const std::vector<CGUIActionDescriptor>& textChangeActions) { m_textChangeActions = textChangeActions; };
  
  bool HasTextChangeActions() { return m_textChangeActions.size() > 0; };
  
  bool SetHighlighted(bool highlighted);

  int GetMaxInputSize();

protected:
  virtual void RenderText();
  CStdStringW GetDisplayedText();
  void RecalcLabelPosition();
  void ValidateCursor();
  void UpdateText(bool sendUpdate = true);
  void OnPasteClipboard();
  void OnSMSCharacter(unsigned int key);

  bool CanAddToLabel2(int textToAddSize = 1);
  std::string GetAllowedText(const std::string& text);

  CStdStringW m_text2;
  CStdString  m_text;
  float m_textOffset;
  float m_textWidth;
  float m_textHeight;

  static const int spaceWidth = 5;

  unsigned int m_cursorPos;
  unsigned int m_cursorBlink;

  int m_inputHeading;
  INPUT_TYPE m_inputType;
  
  std::vector<CGUIActionDescriptor> m_textChangeActions;  

  
  unsigned int m_smsKeyIndex;
  unsigned int m_smsLastKey;
  CStopWatch   m_smsTimer;
  
  static const char*        smsLetters[10];
  static const unsigned int smsDelay;

  CGUITextLayout m_emptyLayout;
  CLabelInfo m_emptyLabelInfo;

  int m_maxTextSize;
  int m_addToText2Counter;

  bool m_highlighted;
};
#endif
