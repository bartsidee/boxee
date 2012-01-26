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

#include "GUITextLayout.h"
#include "GUIFont.h"
#include "GUIControl.h"
#include "GUIColorManager.h"
#include "GUIFontTTF.h"
#include "utils/CharsetConverter.h"
#include "StringUtils.h"

#if defined(_LINUX) && !defined(__APPLE__)
#include <glib.h>
#endif

//elis
#include "utils/log.h"

using namespace std;

#define WORK_AROUND_NEEDED_FOR_LINE_BREAKS

CGUIString::CGUIString(iString start, iString end, bool carriageReturn)
{
  m_text.assign(start, end);
  m_carriageReturn = carriageReturn;
}

CStdString CGUIString::GetAsString() const
{
  CStdString text;
  for (unsigned int i = 0; i < m_text.size(); i++)
    text += (char)(m_text[i] & 0xff);
  return text;
}

CGUITextLayout::CGUITextLayout(CGUIFont *font, bool wrap, float fHeight)
{
  m_font = font;
  m_textColor = 0;
  m_wrap = wrap;
  m_maxHeight = fHeight;
  m_textWidth = 0;
  m_textHeight = 0;
  m_bRebuildVertices = true;
  m_fAlpha = -1;
  m_eResolution = RES_INVALID;

  // we use this to track font texture changes
  m_fontTextureHeight = 0;
  // in case the font object was changed
  m_fontLineHeight = 0;
  m_faceLoadTime = 0;

}

void CGUITextLayout::SetWrap(bool bWrap)
{
  m_wrap = bWrap;
}

void CGUITextLayout::Render(float x, float y, float angle, color_t color, color_t shadowColor, uint32_t alignment, float maxWidth, bool solid, bool clip)
{
  if (!m_font)
    return;

  // our font texture might have grown
  if(m_font->GetFontTextureHeight() != (unsigned int) m_fontTextureHeight)
  {
    m_fontTextureHeight = m_font->GetFontTextureHeight();
    m_bRebuildVertices = true;
    //printf("REBUILD REASON: LINE TEXTURE HEIGHT\n");
  }

  if(fabs(m_font->GetLineHeight() - m_fontLineHeight) < 0.1)
  {
    m_fontLineHeight = m_font->GetLineHeight();
    m_bRebuildVertices = true;
    //printf("REBUILD REASON: FONT HEIGHT\n");
  }

  if(m_font->GetFont()->GetFaceLoadTime() != m_faceLoadTime)
  {
    m_faceLoadTime = m_font->GetFont()->GetFaceLoadTime();
    m_bRebuildVertices = true;
    //printf("REBUILD REASON: FACE\n");
  }

  // set the main text color
  if (m_colors.size())
  {
    if(m_colors[0] != color)
    {
      m_bRebuildVertices = true;
      //printf("REBUILD REASON: COLOR\n");
    }
    m_colors[0] = color;
  }   

  // render the text at the required location, angle, and size
  if (angle)
  {
    static const float degrees_to_radians = 0.01745329252f;
    g_graphicsContext.PushTransform(TransformMatrix::CreateZRotation(angle * degrees_to_radians, x, y, g_graphicsContext.GetScalingPixelRatio()));
  }
  // center our text vertically
  if (alignment & XBFONT_CENTER_Y)
  {
    y -= m_font->GetTextHeight(m_lines.size()) * 0.5f;;
    alignment &= ~XBFONT_CENTER_Y;
  }

  // check if we actually managed to output any text
  bool bTextOutput = false;
  bool textcoordsCreated = false;

  for (vector<CGUIString>::iterator i = m_lines.begin(); i != m_lines.end(); i++)
  {
    CGUIString &string = *i;

    uint32_t align = alignment;
    if (align & XBFONT_JUSTIFIED && string.m_carriageReturn)
      align &= ~XBFONT_JUSTIFIED;

    if(m_fAlpha != g_graphicsContext.GetFinalTransformAlpha())
    {
      m_fAlpha = g_graphicsContext.GetFinalTransformAlpha();
      m_bRebuildVertices = true;
      //printf("REBUILD REASON: ALPHA\n");
    }

    if(m_eResolution != g_graphicsContext.GetVideoResolution())
    {
      m_eResolution = g_graphicsContext.GetVideoResolution();
      m_bRebuildVertices = true;
      //printf("REBUILD REASON: RESOLUTION\n");
    }

    if(m_bRebuildVertices)
    {
      //printf("REBUILD: %s\n", string.GetAsString().c_str());
      string.m_pCharCoords.m_pCoord.clear();
      string.m_pCharCoords.m_nIndices.clear();

      if (solid)
        textcoordsCreated |= m_font->BuildTextCoords(x, y, m_colors[0], shadowColor, string.m_text, align, maxWidth, string.m_pCharCoords, clip);
      else
        textcoordsCreated |= m_font->BuildTextCoords(x, y, m_colors, shadowColor, string.m_text, align, maxWidth, string.m_pCharCoords, clip);
    }
   
    if (!string.m_pCharCoords.m_pCoord.size())
    {
      y += m_font->GetLineHeight();
      continue;
    }

    bTextOutput = true;

    TransformMatrix trans = TransformMatrix::CreateTranslation(x, y);

    g_graphicsContext.PushTransform(trans);
    m_font->RenderText(string.m_pCharCoords);
    g_graphicsContext.PopTransform();

    y += m_font->GetLineHeight();
  }

  if(bTextOutput)
  {
    m_bRebuildVertices = false;
  }
  else
  {
    m_bRebuildVertices = true;
    //printf("REBUILD REASON: NO OUTPUT\n");
  }

  if (angle)
    g_graphicsContext.PopTransform();
}


void CGUITextLayout::RenderScrolling(float x, float y, float angle, color_t color, color_t shadowColor, uint32_t alignment, float maxWidth, CScrollInfo &scrollInfo)
{
  if (!m_font)
    return;

  // set the main text color
  if (m_colors.size())
    m_colors[0] = color;

  // render the text at the required location, angle, and size
  if (angle)
  {
    static const float degrees_to_radians = 0.01745329252f;
    g_graphicsContext.PushTransform(TransformMatrix::CreateZRotation(angle * degrees_to_radians, x, y, g_graphicsContext.GetScalingPixelRatio()));
  }
  // center our text vertically
  if (alignment & XBFONT_CENTER_Y)
  {
    y -= m_font->GetTextHeight(m_lines.size()) * 0.5f;;
    alignment &= ~XBFONT_CENTER_Y;
  }
 
  // NOTE: This workaround is needed as otherwise multi-line text that scrolls
  //       will scroll in proportion to the number of lines.  Ideally we should
  //       do the DrawScrollingText calculation here.  This probably won't make
  //       any difference to the smoothness of scrolling though which will be
  //       jumpy with this sort of thing.  It's not exactly a well used situation
  //       though, so this hack is probably OK.
  float speed = scrollInfo.pixelSpeed;
  for (vector<CGUIString>::iterator i = m_lines.begin(); i != m_lines.end(); i++)
  {
    CGUIString &string = *i;

    string.m_pCharCoords.m_pCoord.clear();
    string.m_pCharCoords.m_nIndices.clear();

    m_font->BuildScrollingTextCoords(x, y, m_colors, shadowColor, string.m_text, alignment, maxWidth, scrollInfo, string.m_pCharCoords);
   
    TransformMatrix trans = TransformMatrix::CreateTranslation(x, y);

    g_graphicsContext.PushTransform(trans);
    m_font->RenderText(string.m_pCharCoords);
    g_graphicsContext.PopTransform();
 
    y += m_font->GetLineHeight();
    scrollInfo.pixelSpeed = 0;
    g_graphicsContext.RestoreClipRegion();
  }
  scrollInfo.pixelSpeed = speed;
 
  if (angle)
    g_graphicsContext.PopTransform();
}

void CGUITextLayout::RenderOutline(float x, float y, color_t color, color_t outlineColor, uint32_t outlineWidth, uint32_t alignment, float maxWidth)
{
  if (!m_font)
    return;

  // set the main text color
  if (m_colors.size())
    m_colors[0] = color;

  // center our text vertically
  if (alignment & XBFONT_CENTER_Y)
  {
    y -= m_font->GetTextHeight(m_lines.size()) * 0.5f;;
    alignment &= ~XBFONT_CENTER_Y;
  }

  for (vector<CGUIString>::iterator i = m_lines.begin(); i != m_lines.end(); i++)
  {
    const CGUIString &string = *i;
    uint32_t align = alignment;
    if (align & XBFONT_JUSTIFIED && string.m_carriageReturn)
      align &= ~XBFONT_JUSTIFIED;

    DrawOutlineText(m_font, x, y, m_colors, outlineColor, outlineWidth, string.m_text, align, maxWidth);
    y += m_font->GetLineHeight() * (1 / g_graphicsContext.GetGUIScaleY());
  }
}

bool CGUITextLayout::Update(const CStdString &text, float maxWidth, bool forceUpdate /*= false*/, bool forceLTRReadingOrder /*= false*/)
{
  if (text == m_lastText && !forceUpdate)
    return false;

  // convert to utf16
  CStdStringW utf16;
  utf8ToW(text, utf16);

  // update
  SetText(utf16, maxWidth, forceLTRReadingOrder);

  // and set our parameters to indicate no further update is required
  m_lastText = text;
  return true;
}

void CGUITextLayout::SetText(const CStdStringW &text, float maxWidth, bool forceLTRReadingOrder /*= false*/)
{
  vecText parsedText;

  // empty out our previous string
  m_lines.clear();
  m_colors.clear();
  m_colors.push_back(m_textColor);

  // parse the text into our string objects
  ParseText(text, parsedText);

  // add \n to the end of the string
  parsedText.push_back(L'\n');

  // if we need to wrap the text, then do so
  if (m_wrap && maxWidth > 0)
    WrapText(parsedText, maxWidth);
  else
    LineBreakText(parsedText, m_lines);

  // remove any trailing blank lines
  while (!m_lines.empty() && m_lines.back().m_text.empty())
    m_lines.pop_back();
  
  BidiTransform(m_lines, forceLTRReadingOrder);
  
  // and cache the width and height for later reading
  CalcTextExtent();

  m_bRebuildVertices = true;
}

// BidiTransform is used to handle RTL text flipping in the string
void CGUITextLayout::BidiTransform(vector<CGUIString> &lines, bool forceLTRReadingOrder)
{
  for (unsigned int i=0; i<lines.size(); i++)
  {
    CGUIString &line = lines[i];

    // reserve enough space in the flipped text
    vecText flippedText; 
    flippedText.reserve(line.m_text.size());

    character_t sectionStyle = 0xffff0000; // impossible to achieve
    CStdStringW sectionText;
    for (vecText::iterator it = line.m_text.begin(); it != line.m_text.end(); ++it)
    { 
      character_t style = *it & 0xffff0000;
      if (style != sectionStyle) 
      {
        if (!sectionText.IsEmpty())
        { // style has changed, bidi flip text 
          CStdStringW sectionFlipped = BidiFlip(sectionText, forceLTRReadingOrder);
          for (unsigned int j = 0; j < sectionFlipped.size(); j++)
            flippedText.push_back(sectionStyle | sectionFlipped[j]);
        }
        sectionStyle = style;
        sectionText.clear();
      }
      sectionText.push_back( (wchar_t)(*it & 0xffff) );
    }

    // handle the last section
    if (!sectionText.IsEmpty())
    {
      CStdStringW sectionFlipped = BidiFlip(sectionText, forceLTRReadingOrder);
      for (unsigned int j = 0; j < sectionFlipped.size(); j++)
        flippedText.push_back(sectionStyle | sectionFlipped[j]);
    }

    // replace the original line with the proccessed one
    lines[i] = CGUIString(flippedText.begin(), flippedText.end(), line.m_carriageReturn);
  }
}

CStdStringW CGUITextLayout::BidiFlip(const CStdStringW &text, bool forceLTRReadingOrder)
{
  CStdStringA utf8text;
  CStdStringW visualText;

  // convert to utf8, and back to utf16 with bidi flipping
  g_charsetConverter.wToUTF8(text, utf8text);
  g_charsetConverter.utf8ToW(utf8text, visualText, true, forceLTRReadingOrder);

  return visualText;
}

void CGUITextLayout::Filter(CStdString &text)
{
  CStdStringW utf16;
  utf8ToW(text, utf16);
  vecColors colors;
  vecText parsedText;
  ParseText(utf16, 0, colors, parsedText);
  utf16.Empty();
  for (unsigned int i = 0; i < parsedText.size(); i++)
    utf16 += (wchar_t)(0xffff & parsedText[i]);
  g_charsetConverter.wToUTF8(utf16, text);
}

void CGUITextLayout::ParseText(const CStdStringW &text, vecText &parsedText)
{
  if (!m_font)
    return;
  ParseText(text, m_font->GetStyle(), m_colors, parsedText);
}

void CGUITextLayout::ParseText(const CStdStringW &text, uint32_t defaultStyle, vecColors &colors, vecText &parsedText)
{
  // run through the string, searching for:
  // [B] or [/B] -> toggle bold on and off
  // [I] or [/I] -> toggle italics on and off
  // [COLOR ffab007f] or [/COLOR] -> toggle color on and off
  // [CAPS <option>] or [/CAPS] -> toggle capatilization on and off

//  uint32_t currentStyle = defaultStyle; // start with the default font's style
//  color_t currentColor = 0;

  stack<color_t> colorStack;
  colorStack.push(0);

  // these aren't independent, but that's probably not too much of an issue
  // eg [UPPERCASE]Glah[LOWERCASE]FReD[/LOWERCASE]Georeg[/UPPERCASE] will work (lower case >> upper case)
  // but [LOWERCASE]Glah[UPPERCASE]FReD[/UPPERCASE]Georeg[/LOWERCASE] won't
#define FONT_STYLE_UPPERCASE 4
#define FONT_STYLE_LOWERCASE 8

  int boldCounter = 0;
  int italicsCoutner = 0;
  int upperCounter = 0;
  int lowerCounter = 0;
  color_t color = 0;

  int startPos = 0;
  size_t pos = text.Find(L'[');
  while (pos != CStdString::npos && pos + 1 < text.size())
  {
    int style = 0;

    if (pos - startPos > 0)
    {
      if (boldCounter)
        style |= FONT_STYLE_BOLD;

      if (italicsCoutner)
        style |= FONT_STYLE_ITALICS;

      CStdStringW subText = text.Mid(startPos, pos - startPos);

      if (upperCounter)
      {
#if defined(_LINUX) && !defined(__APPLE__)
        std::transform(subText.begin(), subText.end(), subText.begin(),
                       (gunichar(*)(gunichar)) g_unichar_toupper);
#else
        subText.ToUpper();
#endif
      }

      if (lowerCounter)
      {
#if defined(_LINUX) && !defined(__APPLE__)
        std::transform(subText.begin(), subText.end(), subText.begin(),
                       (gunichar(*)(gunichar)) g_unichar_tolower);
#else
        subText.ToLower();
#endif
      }

      AppendToUTF32(subText, ((style & 3) << 24) | (color << 16), parsedText);

      startPos = pos;
    }

    // have a [ - check if it's an ON or OFF switch
    bool ignoreTag = false;
    ++pos;

    bool on = true;
    if (text[pos] == L'/')
    {
      on = false;
      pos++;
    }

    // check for each type
    if (text.Mid(pos,2) == L"B]")
    { // bold - finish the current text block and assign the bold state
      pos += 2;
      on ? ++boldCounter : --boldCounter;
    }
    else if (text.Mid(pos,2) == L"I]")
    { // italics
      pos += 2;
      on ? ++italicsCoutner : --italicsCoutner;
    }
    else if (text.Mid(pos,10) == L"UPPERCASE]")
    {
      pos += 10;
      on ? ++upperCounter : --upperCounter;
    }
    else if (text.Mid(pos,10) == L"LOWERCASE]")
    {
      pos += 10;
      on ? ++lowerCounter : --lowerCounter;
    }
    else if (text.Mid(pos,3) == L"CR]" && on)
    {
      pos += 3;
      parsedText.push_back(L'\n');
    }
    else if (text.Mid(pos,5) == L"COLOR")
    { // color
      size_t finish = text.Find(L']', pos + 5);
      if (on && finish != CStdString::npos && (size_t) text.Find(L"[/COLOR]",finish) != CStdString::npos)
      { // create new color
        color = colors.size();
        colors.push_back(g_colorManager.GetColor(text.Mid(pos + 5, finish - pos - 5)));
        colorStack.push(color);
      }
      else if (!on && finish == pos + 5 && colorStack.size() > 1)
      { // revert to previous color
        colorStack.pop();
        color = colorStack.top();
      }
      pos = finish + 1;
    }
    else
    {
      ignoreTag = true;
    }

    if (!ignoreTag)
    {
      startPos = pos;
    }

    pos = text.Find(L'[',pos);
  }

  // now grab the remainder of the string
  CStdStringW subText = text.Mid(startPos, text.GetLength() - startPos);
  int style = 0;
  if (upperCounter)
  {
#if defined(_LINUX) && !defined(__APPLE__)
        std::transform(subText.begin(), subText.end(), subText.begin(),
                       (gunichar(*)(gunichar)) g_unichar_toupper);
#else
        subText.ToUpper();
#endif
  }

  if (lowerCounter)
  {
#if defined(_LINUX) && !defined(__APPLE__)
    std::transform(subText.begin(), subText.end(), subText.begin(),
                   (gunichar(*)(gunichar)) g_unichar_tolower);
#else
    subText.ToLower();
#endif
  }

  if (boldCounter)
    style |= FONT_STYLE_BOLD;
  if (italicsCoutner)
    style |= FONT_STYLE_ITALICS;
  AppendToUTF32(subText, ((style & 3) << 24) | (color << 16), parsedText);
}

void CGUITextLayout::SetMaxHeight(float fHeight)
{
  m_maxHeight = fHeight;
}

void CGUITextLayout::WrapText(const vecText &text, float maxWidth)
{
  if (!m_font)
    return;

  int nMaxLines = (m_maxHeight > 0 && m_font->GetLineHeight() > 0)?(int)(m_maxHeight / m_font->GetLineHeight()):-1;

  //clear the output
  m_lines.clear();

  vector<CGUIString> lines;

  //breaks the text according to \n
  LineBreakText(text, lines);

  for (unsigned int i = 0; i < lines.size(); i++)
  {
    CGUIString &line = lines[i];
    vecText::const_iterator lastSpace = line.m_text.begin();
    vecText::const_iterator pos = line.m_text.begin();
    unsigned int lastSpaceInLine = 0;
    vecText curLine;
    while (pos != line.m_text.end() && (nMaxLines <= 0 || m_lines.size() < (size_t)nMaxLines))
    {
      // Get the current letter in the string
      character_t letter = *pos;
      // check for a space

      float width = m_font->GetTextWidth(curLine);

      if (CanWrapAtLetter(letter) || width > maxWidth)
      {
        //exceeded the width, push what we have into m_lines and continue with the next line
        if (width > maxWidth)
        {
          if (lastSpace != line.m_text.begin() && lastSpaceInLine > 0)
          {
            //push the current line, until the last space
            CGUIString string(curLine.begin(), curLine.begin() + lastSpaceInLine, false);
            m_lines.push_back(string);
            if (IsSpace(letter))
              lastSpace++;  // ignore the space
            pos = lastSpace;
            curLine.clear();
            lastSpaceInLine = 0;
            lastSpace = line.m_text.begin();
            continue;
          }
        }
        // only add spaces if we're not empty
        if (!IsSpace(letter) || curLine.size())
        {
          lastSpace = pos;
          lastSpaceInLine = curLine.size();
          curLine.push_back(letter);
        }
      }
      else
      {
        curLine.push_back(letter);
      }
      pos++;
    }
    // now add whatever we have left to the string
    float width = m_font->GetTextWidth(curLine);

    if (width > maxWidth)
    {
      // too long - put up to the last space on if we can + remove it from what's left.
      if (lastSpace != line.m_text.begin() && lastSpaceInLine > 0)
      {
        //take the begining of the string until the space and push it in the lines
        CGUIString string(curLine.begin(), curLine.begin() + lastSpaceInLine, false);
        m_lines.push_back(string);

        //remove everything before the last space, we will push it later if possible
        curLine.erase(curLine.begin(), curLine.begin() + lastSpaceInLine);

        while (curLine.size() && IsSpace(curLine.at(0)))
          curLine.erase(curLine.begin());
      }
    }

    // if at this point we already have all the lines we need do not add another one
    if (m_lines.size() < (size_t) nMaxLines)
    {
      CGUIString string(curLine.begin(), curLine.end(), true);
      m_lines.push_back(string);
    }
    else if (m_lines.size() && !m_wrap)
    {
      CGUIString &str=m_lines[m_lines.size()-1];
      for (int i=0; i<3; i++)
        str.m_text.push_back(L'.'); // add ellipsis in case there are hidden lines
    }
  }

  m_bRebuildVertices = true;
}

void CGUITextLayout::LineBreakText(const vecText &text, vector<CGUIString> &lines)
{
  bool bUseHeight = (m_maxHeight > 0 && m_font && m_font->GetLineHeight() > 0);
  int nMaxLines = bUseHeight?(int)(m_maxHeight / m_font->GetLineHeight()):-1;
  if (bUseHeight && nMaxLines == 0) // hack. if height is not enough - force 1 line of text
    nMaxLines = 1;
  
  vecText::const_iterator lineStart = text.begin();
  vecText::const_iterator pos = text.begin();
  while (pos != text.end() && (nMaxLines <= 0 || lines.size() < (size_t)nMaxLines))
  {
    // Get the current letter in the string
    character_t letter = *pos;

    // Handle the newline character
    if ((letter & 0xffff) == L'\n' )
    { // push back everything up till now
      CGUIString string(lineStart, pos, true);
      lines.push_back(string);
      lineStart = pos + 1;
    }
    pos++;
  }
}

void CGUITextLayout::GetTextExtent(float &width, float &height) const
{
  width = m_textWidth;
  height = m_textHeight;
}

void CGUITextLayout::CalcTextExtent()
{
  m_textWidth = 0;
  m_textHeight = 0;
  if (!m_font) return;

  for (vector<CGUIString>::iterator i = m_lines.begin(); i != m_lines.end(); i++)
  {
    const CGUIString &string = *i;
    float w = m_font->GetTextWidth(string.m_text);
    if (w > m_textWidth)
      m_textWidth = w;
  }
  m_textHeight = m_font->GetTextHeight(m_lines.size());
}

unsigned int CGUITextLayout::GetTextLength() const
{
  unsigned int length = 0;
  for (vector<CGUIString>::const_iterator i = m_lines.begin(); i != m_lines.end(); i++)
    length += i->m_text.size();
  return length;
}

void CGUITextLayout::GetFirstText(vecText &text) const
{
  text.clear();
  if (m_lines.size())
    text = m_lines[0].m_text;
}

float CGUITextLayout::GetTextWidth(const CStdStringW &text) const
{
  // NOTE: Assumes a single line of text
  if (!m_font) return 0;
  vecText utf32;
  AppendToUTF32(text, (m_font->GetStyle() & 3) << 24, utf32);
  return m_font->GetTextWidth(utf32);
}

float CGUITextLayout::GetTextHeight(int numLines) const
{
  // NOTE: Assumes a single line of text
  if (!m_font) return 0;
  if (numLines == 1)
    return m_font->GetLineHeight();
  else
    return m_font->GetTextHeight(numLines);
}

void CGUITextLayout::DrawText(CGUIFont *font, float x, float y, color_t color, color_t shadowColor, const CStdString &text, uint32_t align)
{
  /* elis
  if (!font) return;
  vecText utf32;
  AppendToUTF32(text, 0, utf32);
  font->DrawText(x, y, color, shadowColor, utf32, align, 0);
  */
}

void CGUITextLayout::DrawOutlineText(CGUIFont *font, float x, float y, color_t color, color_t outlineColor, uint32_t outlineWidth, const CStdString &text)
{
  if (!font) return;
  vecText utf32;
  AppendToUTF32(text, 0, utf32);
  vecColors colors;
  colors.push_back(color);
  DrawOutlineText(font, x, y, colors, outlineColor, outlineWidth, utf32, 0, 0);
}

void CGUITextLayout::DrawOutlineText(CGUIFont *font, float x, float y, const vecColors &colors, color_t outlineColor, uint32_t outlineWidth, const vecText &text, uint32_t align, float maxWidth)
{
  for (unsigned int i = 1; i < outlineWidth; i++)
  {
    unsigned int ymax = (unsigned int)(sqrt((float)outlineWidth*outlineWidth - i*i) + 0.5f);
    for (unsigned int j = 1; j < ymax; j++)
    {
      BuildAndRenderScreenText(font, x - i, y + j, outlineColor, 0, outlineWidth, text, align, maxWidth);
      BuildAndRenderScreenText(font, x - i, y - j, outlineColor, 0, outlineWidth, text, align, maxWidth);
      BuildAndRenderScreenText(font, x + i, y + j, outlineColor, 0, outlineWidth, text, align, maxWidth);
      BuildAndRenderScreenText(font, x + i, y - j, outlineColor, 0, outlineWidth, text, align, maxWidth);
    }
  }

  BuildAndRenderScreenText(font, x, y, colors[0], 0, outlineWidth, text, align, maxWidth);
}

void CGUITextLayout::BuildAndRenderScreenText(CGUIFont *font, float x, float y, const color_t &color, color_t outlineColor, 
                                          uint32_t outlineWidth, const vecText &text, uint32_t align, float maxWidth)
{
  FontCoordsIndiced fontCoords;
  TransformMatrix trans, scale;

  float scalex = g_graphicsContext.GetGUIScaleX();
  float scaley = g_graphicsContext.GetGUIScaleY();

  ASSERT(scalex != 0);
  ASSERT(scaley != 0);

  font->BuildTextCoords(x, y, color, 0, text, align, maxWidth, fontCoords, false);

  // Our x,y location is in screen coordinate
  // so load an identity matrix.
  g_graphicsContext.PushTransform(TransformMatrix(), true);

  trans = TransformMatrix::CreateTranslation(x, y);
  scale = TransformMatrix::CreateScaler(1/scalex, 1/scaley);
  g_graphicsContext.PushTransform(trans);
  g_graphicsContext.PushTransform(scale);

  font->RenderText(fontCoords);

  // scale
  g_graphicsContext.PopTransform();
  // trans
  g_graphicsContext.PopTransform();
  // pop identity
  g_graphicsContext.PopTransform();
}

void CGUITextLayout::AppendToUTF32(const CStdStringW &utf16, character_t colStyle, vecText &utf32)
{
  // NOTE: Assumes a single line of text
  utf32.reserve(utf32.size() + utf16.size());
  for (unsigned int i = 0; i < utf16.size(); i++)
    utf32.push_back(utf16[i] | colStyle);
}

void CGUITextLayout::utf8ToW(const CStdString &utf8, CStdStringW &utf16)
{
#ifdef WORK_AROUND_NEEDED_FOR_LINE_BREAKS
  // NOTE: This appears to strip \n characters from text.  This may be a consequence of incorrect
  //       expression of the \n in utf8 (we just use character code 10) or it might be something
  //       more sinister.  For now, we use the workaround below.
  CStdStringArray multiLines;
  StringUtils::SplitString(utf8, "\n", multiLines);
  for (unsigned int i = 0; i < multiLines.size(); i++)
  {
    CStdStringW line;
    // no need to bidiflip here - it's done in BidiTransform above
    g_charsetConverter.utf8ToW(multiLines[i], line, false);
    utf16 += line;
    if (i < multiLines.size() - 1)
      utf16.push_back(L'\n');
  }
#else
  // no need to bidiflip here - it's done in BidiTransform above
  g_charsetConverter.utf8ToW(utf8, utf16, false);
#endif
}

void CGUITextLayout::AppendToUTF32(const CStdString &utf8, character_t colStyle, vecText &utf32)
{
  CStdStringW utf16;
  utf8ToW(utf8, utf16);
  AppendToUTF32(utf16, colStyle, utf32);
}

void CGUITextLayout::Reset()
{
  m_lines.clear();
  m_lastText.Empty();
  m_textWidth = m_textHeight = 0;

  m_bRebuildVertices = true;
}


