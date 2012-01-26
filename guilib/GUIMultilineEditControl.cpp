#include "GUIMultilineEditControl.h"

#define CHARS_IN_LINE_WIDTH 38

CGUIMultilineEditControl::CGUIMultilineEditControl(int parentID, int controlID, float posX, float posY,
                                 float width, float height, const CTextureInfo &textureFocus, const CTextureInfo &textureNoFocus,
                                 const CLabelInfo& labelInfo, const std::string &text, const CLabelInfo& emptylabelInfo, const std::string &emptyText, int maxTextSize)
:CGUIEditControl(parentID, controlID , posX, posY, width, height, textureFocus, textureNoFocus, labelInfo, text , emptylabelInfo, emptyText, maxTextSize)
{

  m_textLayout.SetWrap(true);
  m_textLayout2.SetWrap(true);
}

unsigned int CGUIMultilineEditControl::GlobalTextCursorToLocal(unsigned int& cursor)
{
  int cursorLine=0;

  for (size_t i = 0 ; i < m_textLayout2.GetCountLines() ; i++)
  {
    size_t currentLineSize = m_textLayout2.GetLineSize(i);
    cursorLine = i;

    if (cursor < currentLineSize)
    {
      //we found the line where the cursor is
      break;
    }
    else
    {
      cursor -= currentLineSize;
    }
  }

  return cursorLine;
}

bool CGUIMultilineEditControl::OnAction(const CAction &action)
{
  ValidateCursor();

  if (action.id == ACTION_MOVE_UP)
  {
    unsigned int cursor = m_cursorPos;
    unsigned int cursorLine;

    cursorLine = GlobalTextCursorToLocal(cursor);

    if (cursorLine > 0)
    {
      unsigned int lineBeforeLength = m_textLayout2.GetLineSize(cursorLine-1);

      m_cursorPos -= cursor;

      int cutoffPreviousLine = (lineBeforeLength-cursor);

      if (cutoffPreviousLine <= 0)
      {
        m_cursorPos --;
      }
      else
      {
        m_cursorPos -= cutoffPreviousLine;
      }

      if ((int)m_cursorPos <= 0)
      {
        m_cursorPos = 0;
      }

      UpdateText(false);
      return true;
    }
  }
  else if (action.id == ACTION_MOVE_DOWN)
  {
    if ((unsigned int) m_cursorPos < m_text2.size())
    {
      unsigned int cursor = m_cursorPos;
      unsigned int cursorLine;

      cursorLine = GlobalTextCursorToLocal(cursor);

      unsigned int lineAfterLength = m_textLayout2.GetLineSize(cursorLine);
      m_cursorPos += cursor;
      m_cursorPos += (lineAfterLength-cursor);

      if ((unsigned int) m_cursorPos > m_text2.size())
        m_cursorPos = (unsigned int)m_text2.size();

      UpdateText(false);
      return true;
    }
  }

  return CGUIEditControl::OnAction(action);
}

void CGUIMultilineEditControl::RecalcLabelPosition()
{
  //unlike the GUIEditControl, here we calculate how to shit the text in the Y axis up.
  if (!m_label.font) return;

  // ensure that our cursor is within our width
  ValidateCursor();

  float lineHeight = m_label.font->GetLineHeight();

  if (lineHeight <= 0)
    return;

  unsigned int cursorLine = 0;
  unsigned int cursorLinePosition = m_cursorPos; // set it to global text position

  cursorLine = GlobalTextCursorToLocal(cursorLinePosition);
  cursorLine++; //if its 0 we use it as 1

  int totalLines = (int) (m_height / lineHeight); //possible lines

  float position = (float)cursorLine / totalLines;

  if (position < 1)
  {
    position = 0;
  }
  else
  {
    position = cursorLine - totalLines + 0.5;
  }

  m_textOffset = -position*lineHeight;

}

void CGUIMultilineEditControl::RenderText()
{
  if (m_smsTimer.GetElapsedMilliseconds() > smsDelay)
    UpdateText();

  float leftTextWidth = m_textLayout.GetTextWidth();
  float maxTextWidth = m_width - m_label.offsetX * 2;

  // start by rendering the normal text
  float posX = m_posX + m_label.offsetX;
  float posY = m_posY + m_label.offsetY;
  uint32_t align = m_label.align & XBFONT_CENTER_Y;

  if (m_label.align & XBFONT_CENTER_Y)
    posY += m_height*0.5f;

  if (leftTextWidth > 0)
  {
    // render the text on the left
    if (IsDisabled())
      m_textLayout.Render(posX, posY, m_label.angle, m_label.disabledColor, m_label.shadowColor, align, leftTextWidth);
    else if (HasFocus() && m_label.focusedColor)
      m_textLayout.Render(posX, posY, m_label.angle, m_label.focusedColor, m_label.shadowColor, align, leftTextWidth);
    else
      m_textLayout.Render(posX, posY, m_label.angle, m_label.textColor, m_label.shadowColor, align, leftTextWidth);

    posX += leftTextWidth + spaceWidth;
    maxTextWidth -= leftTextWidth + spaceWidth;
  }

  if (g_graphicsContext.SetClipRegion(posX, m_posY+m_label.offsetY, maxTextWidth, m_height-m_label.offsetY*2))
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
    if (HasFocus())
    { // cursor location assumes utf16 text, so deal with that (inefficient, but it's not as if it's a high-use area
      // virtual keyboard only)
      CStdStringW col;
      if ((SDL_GetTicks() % 800) <= 400)
        col.Format(L"|");
      else
        col.Format(L"");
      text.Insert(m_cursorPos, col);
    }

    m_textLayout2.SetText(text, maxTextWidth-m_label.offsetX);

    if (m_bInvalidated)
      RecalcLabelPosition();

    if (IsDisabled())
      m_textLayout2.Render(posX, posY + m_textOffset, m_label.angle, m_label.disabledColor, m_label.shadowColor, align, maxTextWidth);
    else if (HasFocus() && m_label.focusedColor)
      m_textLayout2.Render(posX, posY + m_textOffset, m_label.angle, m_label.focusedColor, m_label.shadowColor, align, maxTextWidth);
    else
      m_textLayout2.Render(posX, posY + m_textOffset, m_label.angle, m_label.textColor, m_label.shadowColor, align, maxTextWidth);

   /* if (m_text2.size() == 0)
    {
      m_emptyLayout.Render(posX + 5, posY, m_emptyLabelInfo.angle, m_emptyLabelInfo.textColor, m_emptyLabelInfo.shadowColor, align, m_emptyLayout.GetTextWidth(), true);
    }
*/
    g_graphicsContext.RestoreClipRegion();
  }
}
