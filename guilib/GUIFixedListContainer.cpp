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

#include "GUIFixedListContainer.h"
#include "GUIListItem.h"
#include "utils/GUIInfoManager.h"
#include "Key.h"
#include "utils/log.h"

CGUIFixedListContainer::CGUIFixedListContainer(int dwParentID, int dwControlId, float posX, float posY, float width, float height, ORIENTATION orientation, int scrollTime, int preloadItems, int fixedPosition, ListSelectionMode selectionMode)
: CGUIBaseContainer(dwParentID, dwControlId, posX, posY, width, height, orientation, scrollTime, preloadItems, selectionMode)
{
  ControlType = GUICONTAINER_FIXEDLIST;
  m_type = VIEW_TYPE_LIST;
  m_cursor = fixedPosition;
  PinPosition(false);
}

CGUIFixedListContainer::~CGUIFixedListContainer(void)
{
}

void CGUIFixedListContainer::PinPosition(bool bPin)
{
  m_bPinnedPosition = bPin;
}

bool CGUIFixedListContainer::OnAction(const CAction &action)
{
  switch (action.id)
  {
  case ACTION_PAGE_UP:
  {
    if (m_offset + m_cursor < m_itemsPerPage)
    { // already on the first page, so move to the first item
      Scroll(m_offset + m_cursor - m_itemsPerPage);
    }
    else
    { // scroll up to the previous page
      Scroll(-m_itemsPerPage);
    }
    return true;
  }
  break;
  case ACTION_PAGE_DOWN:
  {
    if (m_offset + m_cursor >= (int)m_items.size() || (int)m_items.size() < m_itemsPerPage)
    { // already at the last page, so move to the last item.
      Scroll(m_items.size() - 1 - m_offset + m_cursor);
    }
    else
    { // scroll down to the next page
      Scroll(m_itemsPerPage);
    }
    return true;
  }
  break;
  // smooth scrolling (for analog controls)
  case ACTION_SCROLL_UP:
  {
    m_analogScrollCount += action.amount1 * action.amount1;
    bool handled = false;
    while (m_analogScrollCount > 0.4)
    {
      handled = true;
      m_analogScrollCount -= 0.4f;
      if (m_offset > -m_cursor)
        Scroll(-1);
    }
    return handled;
  }
  break;
  case ACTION_SCROLL_DOWN:
  {
    m_analogScrollCount += action.amount1 * action.amount1;
    bool handled = false;
    while (m_analogScrollCount > 0.4)
    {
      handled = true;
      m_analogScrollCount -= 0.4f;
      if (m_offset < (int)m_items.size() - 1)
        Scroll(1);
    }
    return handled;
  }
  break;
  }
  return CGUIBaseContainer::OnAction(action);
}

bool CGUIFixedListContainer::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_ITEM_SELECT)
    {
      if (m_loading)
      {
        m_savedSelected = message.GetParam1();
      }
      else
      {
        UpdateVisibility();
        SelectItem(message.GetParam1());
      }
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_PAGE_UP)
    {
      CAction action;
      action.id = ACTION_PAGE_UP;
      return OnAction(action);
    }
    else if (message.GetMessage() == GUI_MSG_PAGE_DOWN)
    {
      CAction action;
      action.id = ACTION_PAGE_DOWN;
      return OnAction(action);
    }
  }
  return CGUIBaseContainer::OnMessage(message);
}

void CGUIFixedListContainer::Reset()
{
  CGUIBaseContainer::Reset();

  m_cursor = 0;
  m_offset = 0;
  m_scrollOffset = 0;
}

bool CGUIFixedListContainer::IsWrapAround()
{  
  bool wrapAround = m_controlUp == GetID() || !(m_controlUp || m_upActions.size());
  wrapAround |= m_controlDown == GetID() || !(m_controlDown || m_downActions.size());
  wrapAround |= m_controlLeft == GetID() || !(m_controlLeft || m_leftActions.size());
  wrapAround |= m_controlRight == GetID() || !(m_controlRight || m_rightActions.size());
  return wrapAround;
}

// dconti-
// A valiant effort to explain this code
//
// m_items: this is the set of items in the list
// m_items.size(): this is the total number of items in the list
// m_itemsPerPage: this is the number of items we can display at once (horizontal or vertical pages)
//
// nCurrOffset: this is the 0-based current offset within the list
// nTargetOffset: this is the 0-based offset within the list that we would like to end up on;
//                so scrolling down one at a time = nTargetOffset increases by 1
//
// CorrectOffset(): this function just adds the two items together.
//
// 

void CGUIFixedListContainer::ScrollToOffset(int offset)
{
  CLog::Log(LOGDEBUG,"CGUIFixedListContainer::ScrollToOffset - ENTER. [m_cursor=%d][offset=%d][m_offset=%d] (scroll)", m_cursor, offset, m_offset);

  bool wrapAround = IsWrapAround();
  if ( wrapAround || m_bPinnedPosition)
  {
    CGUIBaseContainer::ScrollToOffset(offset);
    return; 
  }

  int nCurrOffset = CorrectOffset(m_offset, m_cursor); 
  int nTargetOffset = CorrectOffset(offset, m_cursor); 
  
  int last_section = m_items.size() - (m_itemsPerPage / 2);
  int first_section = (m_itemsPerPage / 2);

  CLog::Log(LOGDEBUG, "CGUIFixedListContainer::ScrollToOffset - [m_cursor=%d][offset=%d][m_offset=%d][nCurrOffset=%d][nTargetOffset=%d][numOfItems=%zu][m_itemsPerPage=%d] (scroll)",
      m_cursor, offset, m_offset, nCurrOffset, nTargetOffset, m_items.size(), m_itemsPerPage);
  
  // Case 1:
  // Someone initialized the window and immediately scrolled to a cached offset, and we have more than one page of content
  // We detect this case by seeing that we are not stepping by an increment of one
  // In this situation we recalculate the cursor and the list offset from scratch and call base seek, but only if we have more than a page
  // if we have less than a page we never scroll

  if( nTargetOffset != nCurrOffset+1 && nTargetOffset != nCurrOffset-1 && nTargetOffset != nCurrOffset)
  {
    // BUGBUG: we don't handle separators properly here, but normally SelectItem will handle that for us (since that is the likely call path here)
    
    // See if any scroll is required. If we have a page or less then no
    if( m_items.size() > (size_t) m_itemsPerPage )
    {
      // see if we are in the first block
      if( nTargetOffset <= first_section )
      {
        m_cursor = nTargetOffset;
        nTargetOffset = 0;
      }
      // or in the last block
      else if( nTargetOffset >= last_section )
      {
        int off = m_items.size() - m_itemsPerPage;
        m_cursor = nTargetOffset - off;
        nTargetOffset = off;
      }
      // otherwise, just center us
      else
      {
        m_cursor = m_itemsPerPage / 2;
        nTargetOffset -= m_cursor;
      }

      //avoid scrolling and starting the scrolltimer, used when we're setting selection on specific item
      m_wasReset = true;
      CGUIBaseContainer::ScrollToOffset(nTargetOffset);
    }
    else
    {
      CGUIBaseContainer::ScrollToOffset(0);
      m_cursor = nTargetOffset;
    }
  }
  else
  // Case 2:
  // We are stepping backwards in the list. I.e. moving up or left depending on orientation (nCurrOffset > nTargetOffset)
  // We are moving in the fixed pane of the content
  // If we hit a separator then we recurse to ensure we shift appropriately.
  if (nCurrOffset >= nTargetOffset && ((size_t) nCurrOffset >= m_items.size() - (m_itemsPerPage / 2) || nCurrOffset == 0 ))
  {
    // Move the cursor by the requested delta
    m_cursor -= (nCurrOffset - nTargetOffset);
    
    // Handle the case where we land on a separator by moving us along one extra space
    if (m_items.size() > (size_t) (m_offset +m_cursor) && m_items[m_offset +m_cursor]->GetPropertyBOOL("isseparator"))
    {
      if (m_offset +m_cursor == 0)
      {
        MoveDown(false);
      }
      else
      {
        MoveUp(false);
      }
    }
  }
  // Case 3:
  // We are stepping forwards in the list. I.e. moving down or right depending on the orientation (nTargetOffset > nCurrOffset)
  // We are moving in the fixed pane of the content
  // If we hit a separator then we recurse to ensure we shift appropriately.
  else if (nCurrOffset <= nTargetOffset && (nCurrOffset < m_itemsPerPage / 2 || (m_items.size() - m_itemsPerPage) == (size_t) m_offset) )
  {
    // Move the cursor by the requested delta
    m_cursor += (nTargetOffset - nCurrOffset);
    
    // Handle the case where we land ona  separator by moving us along one extra space
    if (m_items.size() > (size_t) (m_offset +m_cursor) && m_items[m_offset +m_cursor]->GetPropertyBOOL("isseparator"))
    {
      if ((size_t) (m_offset +m_cursor) == m_items.size() - 1)
      {
        MoveUp(false);
      }
      else
      {
        MoveDown(false);
      }
    }
  }
  else if( (size_t) m_itemsPerPage < m_items.size() )
  {
    // Case 4:
    // We are stepping forwards or back in the list, not within the fixed block, and have no special conditions.
    // We actually need to scroll the contents of the list
    // BUGBUG: in a long list this means we land on separators and don't shuffle off them??
    if( offset >= 0 )
      CGUIBaseContainer::ScrollToOffset(offset);
    else
      m_cursor += offset;
  }
  else
  {
    // Case 5:
    // Don't scroll; just move the cursor
    m_cursor += offset;
  }
  
  // This centers the cursor in the list
  if (m_cursor > m_itemsPerPage)
  {
    CGUIBaseContainer::ScrollToOffset(m_cursor - m_itemsPerPage / 2);
    m_cursor = m_itemsPerPage / 2;
  }

  CLog::Log(LOGDEBUG,"CGUIFixedListContainer::ScrollToOffset - EXIT. [m_cursor=%d][offset=%d][m_offset=%d] (scroll)",m_cursor,offset,m_offset);
}

bool CGUIFixedListContainer::MoveUp(bool wrapAround)
{
  if (m_offset > -m_cursor)
    ScrollToOffset(m_offset - 1);
  else if (wrapAround)
  {
    if (m_items.size() > 0)
    { // move 2 last item in list
      int offset = m_items.size() - m_cursor - 1;
      if (offset < -m_cursor) offset = -m_cursor;
      ScrollToOffset(offset);
      g_infoManager.SetContainerMoving(GetID(), -1);
    }
  }
  else
    return false;

  if (m_items.size() > (size_t) (m_offset +m_cursor) && m_items[m_offset +m_cursor]->GetPropertyBOOL("isseparator"))
  {
    if (m_offset +m_cursor == 0)
    {
      MoveDown(wrapAround);
      if (!wrapAround)
        return false;
    }
    else
    {
      MoveUp(wrapAround);
    }
  }

  return true;
}

bool CGUIFixedListContainer::MoveDown(bool wrapAround)
{
  if (m_offset + m_cursor + 1 < (int)m_items.size())
  {
    ScrollToOffset(m_offset + 1);
  }
  else if (wrapAround)
  { // move first item in list
    ScrollToOffset(-m_cursor);
    g_infoManager.SetContainerMoving(GetID(), 1);
  }
  else
    return false;

  if (m_items.size() > (size_t) (m_offset +m_cursor) && m_items[m_offset +m_cursor]->GetPropertyBOOL("isseparator"))
  {
    if ((size_t) (m_offset +m_cursor) == m_items.size() - 1)
    {
      MoveUp(wrapAround);
      if (!wrapAround)
        return false;
    }
    else
    {
      MoveDown(wrapAround);
    }
  }

  return true;
}

// scrolls the said amount
void CGUIFixedListContainer::Scroll(int amount)
{
  // increase or decrease the offset
  int offset = m_offset + amount;
  if (offset >= (int)m_items.size() - m_cursor)
  {
    offset = m_items.size() - m_cursor - 1;
  }
  if (offset < -m_cursor) offset = -m_cursor;
  ScrollToOffset(offset);

  if (amount > 0)
  {
    if (m_items.size() > (size_t) (m_offset +m_cursor) && m_items[m_offset +m_cursor]->GetPropertyBOOL("isseparator"))
    {
      if ((size_t) (m_offset +m_cursor) == m_items.size() - 1)
      {
        Scroll(-1);
        return;
}
      else
      {
        Scroll(1);
        return;
      }
    }
  }

  if (amount < 0)
  {
    if (m_items.size() > (size_t) (m_offset +m_cursor) && m_items[m_offset +m_cursor]->GetPropertyBOOL("isseparator"))
    {
      if (m_offset +m_cursor == 0)
      {
        Scroll(1);
        return;
      }
      else
      {
        Scroll(-1);
        return;
      }
    }
  }
}

void CGUIFixedListContainer::ValidateOffset()
{ // first thing is we check the range of m_offset
  if (!m_layout) return;
  if (m_items.size() == 0) return;
  if (m_offset > (int)m_items.size() - m_cursor)
  {
    m_offset = m_items.size() - m_cursor;
    m_scrollOffset = m_offset * m_layout->Size(m_orientation);
  }
  if (m_offset < -m_cursor)
  {
    m_offset = -m_cursor;
    m_scrollOffset = m_offset * m_layout->Size(m_orientation);
  }
}

bool CGUIFixedListContainer::SelectItemFromPoint(const CPoint &point)
{
  if (!m_focusedLayout || !m_layout)
    return false;

  const float mouse_scroll_speed = 0.05f;
  const float mouse_max_amount = 1.5f;   // max speed: 1 item per frame
  float sizeOfItem = m_layout->Size(m_orientation);
  // see if the point is either side of our focused item
  float start = m_cursor * sizeOfItem;
  float end = start + m_focusedLayout->Size(m_orientation);
  float pos = (m_orientation == VERTICAL) ? point.y : point.x;
  if (pos < start - 0.5f * sizeOfItem)
  { // scroll backward
    if (!InsideLayout(m_layout, point))
      return false;
    float amount = std::min((start - pos) / sizeOfItem, mouse_max_amount);
    m_analogScrollCount += amount * amount * mouse_scroll_speed;
    if (m_analogScrollCount > 1)
    {
      Scroll(-1);
      m_analogScrollCount-=1.0f;
    }
    return true;
  }
  else if (pos > end + 0.5f * sizeOfItem)
  {
    if (!InsideLayout(m_layout, point))
      return false;
    // scroll forward
    float amount = std::min((pos - end) / sizeOfItem, mouse_max_amount);
    m_analogScrollCount += amount * amount * mouse_scroll_speed;
    if (m_analogScrollCount > 1)
    {
      Scroll(1);
      m_analogScrollCount-=1.0f;
    }
    return true;
  }
  return InsideLayout(m_focusedLayout, point);
}

void CGUIFixedListContainer::SelectItem(int item)
{
  // Check that m_offset is valid
  ValidateOffset();
  // only select an item if it's in a valid range
  if (item >= 0 && item < (int)m_items.size())
  {
    if (m_items[item]->GetPropertyBOOL("isseparator"))
    {
      SelectItem(item + 1);
      return;
    }

    ScrollToOffset(item - m_cursor);
  }
}

bool CGUIFixedListContainer::HasPreviousPage() const
{
  return (m_offset > 0);
}

bool CGUIFixedListContainer::HasNextPage() const
{
  return (m_offset != (int)m_items.size() - m_itemsPerPage && (int)m_items.size() >= m_itemsPerPage);
}

int CGUIFixedListContainer::GetCurrentPage() const
{
  int offset = CorrectOffset(m_offset, m_cursor);
  if (offset + m_itemsPerPage - m_cursor >= (int)GetRows())  // last page
    return (GetRows() + m_itemsPerPage - 1) / m_itemsPerPage;
  return offset / m_itemsPerPage + 1;
}

