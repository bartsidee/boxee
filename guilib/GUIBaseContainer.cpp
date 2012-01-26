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

#include "GUIBaseContainer.h"
#include "GUIControlFactory.h"
#include "GUILabelControl.h"
#include "GUIWindowManager.h"
#include "utils/CharsetConverter.h"
#include "utils/GUIInfoManager.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "XMLUtils.h"
#include "SkinInfo.h"
#include "../xbmc/FileSystem/Directory.h"
#include "../xbmc/FileSystem/File.h"
#include "StringUtils.h"
#include "GUIWindowManager.h"
#include "Application.h"
#include "Key.h"
#include "ItemLoader.h"
#include "utils/SingleLock.h"
#include "MouseStat.h"
#include "../GUISettings.h"
#include "GUIMessage.h"


using namespace std;

#define HOLD_TIME_START 100
#define HOLD_TIME_MIN_ACCELERATION 500
#define HOLD_TIME_MAX_ACCELERATION 2000
#define HOLD_TIME_DELTA_ACCELERATION 1500

#define OLD_SCROLLING

#ifdef OLD_SCROLLING
#define HOLD_TIME_END   3000
#else
#define HOLD_TIME_END   20000
#endif

CGUIBaseContainer::CGUIBaseContainer(int parentID, int controlId, float posX, float posY, float width, float height, ORIENTATION orientation, int scrollTime, int preloadItems, ListSelectionMode selectionMode) : CGUIControl(parentID, controlId, posX, posY, width, height)
{
  m_cursor = 0;
  m_offset = 0;
  m_scrollOffset = 0;
  m_scrollSpeed = 0;
  m_scrollLastTime = 0;
  m_scrollTime = scrollTime ? scrollTime : 1;
  m_scrollTimeMin = m_scrollTime;
  m_currentScrollTime = m_scrollTime;
  m_lastHoldTime = 0;
  m_itemsPerPage = 10;
  m_pageControl = 0;
  m_renderTime = 0;
  m_orientation = orientation;
  m_analogScrollCount = 0;
  m_lastItem = NULL;
  m_hasStaticContent = false;
  m_staticUpdateTime = 0;
  m_staticUpdateTime = 0;
  m_wasReset = false;
  m_layout = NULL;
  m_focusedLayout = NULL;
  m_offsetX = 0;
  m_offsetY = 0;
  m_loading = false;
  m_loaded = false;
  m_hasStaticActions = false;
  m_selectionMode = selectionMode;
  m_cacheItems = preloadItems;
  m_focusedPosX = 0;
  m_focusedPosY = 0;
  m_autoScrollTime = 0;
  m_clickAnimationStarted = false;
  m_firstRenderAfterLoad = false;
  m_savedSelected = -1;
  m_itemsPerFrame = 0.9; //TODO: calculate by scrolltime / fps or something?

  CLog::Log(LOGDEBUG,"CGUIBaseContainer::CGUIBaseContainer, %d (basecontainer)", GetID());
}

CGUIBaseContainer::~CGUIBaseContainer(void)
{
  // the destructor called from CGUIPanelContainer::OnAction when m_itemsLock is locked
  // we need to unlock it here so it will be properly destroyed
  if(OwningCriticalSection(&m_itemsLock))
  {
    LeaveCriticalSection(&m_itemsLock);
  }
}

void CGUIBaseContainer::Render()
{
#ifdef OLD_SCROLLING
  if (m_clickAnimationStarted && GetFocusedLayout()&& !GetFocusedLayout()->IsAnimating(ANIM_TYPE_CLICK))
#else
  if (!m_deferredActions.empty() && !IsScrolling())
#endif
  {
    m_clickAnimationStarted = false;
#ifdef OLD_SCROLLING
    g_application.DeferAction(m_clickAction);
#else
    {
      CSingleLock lock(m_actionsLock);
      if (!m_deferredActions.empty())
      {
        CAction action = m_deferredActions.front();
        m_deferredActions.pop();
        g_application.DeferAction(action);
      }
    }
#endif
  }

  ValidateOffset();

  if (m_autoScrollTime > 0 && m_items.size() > 1)
  {
    if (!m_autoScrollTimer.IsRunning())
    {
      m_autoScrollTimer.StartZero();
    }
    
    if (m_autoScrollTimer.GetElapsedMilliseconds() > m_autoScrollTime)
    {
      MoveDown(true);
      m_autoScrollTimer.StartZero();    
    }
  }
  
  if (m_bInvalidated)
    UpdateLayout();

  if (!m_layout || !m_focusedLayout) return;

  UpdateScrollOffset();

  int offset = (int)floorf(m_scrollOffset / m_layout->Size(m_orientation));
  int cacheBefore, cacheAfter;
  GetCacheOffsets(cacheBefore, cacheAfter);

  // Free memory not used on screen
  if ((int)m_items.size() > m_itemsPerPage + cacheBefore + cacheAfter)
    FreeMemory(CorrectOffset(offset - cacheBefore, 0), CorrectOffset(offset + m_itemsPerPage + 1 + cacheAfter, 0));

  bool clip = g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height);
  float pos = (m_orientation == VERTICAL) ? m_posY + m_offsetY : m_posX + m_offsetX;
  float end = (m_orientation == VERTICAL) ? m_posY + m_height : m_posX + m_width;

  // we offset our draw position to take into account scrolling and whether or not our focused
  // item is offscreen "above" the list.
  float drawOffset = (offset - cacheBefore) * m_layout->Size(m_orientation) - m_scrollOffset;
  if (m_offset + m_cursor < offset)
    drawOffset += m_focusedLayout->Size(m_orientation) - m_layout->Size(m_orientation);
  pos += drawOffset;
  end += cacheAfter * m_layout->Size(m_orientation);

  float focusedPos = 0;
  unsigned int focusedRow = 0;
  CGUIListItemPtr focusedItem;
  int current = offset - cacheBefore;
  unsigned int row = 0;
  while (pos < end && m_items.size())
  {
    int itemNo = CorrectOffset(current, 0);
    if (itemNo >= (int)m_items.size())
      break;
    bool focused = (current == m_offset + m_cursor);
    if (itemNo >= 0)
    {
      CGUIListItemPtr item = m_items[itemNo];
      if (item)
        item.get()->SetOffset(current);
      // render our item
      if (focused)
      {
        focusedPos = pos;
        focusedItem = item;
        focusedRow = row;
      }
      else
      {
        if (m_orientation == VERTICAL)
          RenderItem(m_posX + m_offsetX, pos, item.get(), row, 0, false);
        else
          RenderItem(pos, m_posY + m_offsetY, item.get(), row, 0, false);
      }
    }
    // increment our position
    pos += focused ? m_focusedLayout->Size(m_orientation) : m_layout->Size(m_orientation);
    current++;
    row++;
  }
  
  // render focused item last so it can overlap other items
  if (focusedItem)
  {
    if (m_orientation == VERTICAL)
      RenderItem(m_posX + m_offsetX, focusedPos , focusedItem.get(), focusedRow, 0, true);
    else
      RenderItem(focusedPos, m_posY + m_offsetY, focusedItem.get(), focusedRow, 0, true);
  }
  
  m_firstRenderAfterLoad = false;

  if(clip)
    g_graphicsContext.RestoreClipRegion();

  UpdatePageControl(offset);

  CGUIControl::Render();
}


void CGUIBaseContainer::RenderItem(float posX, float posY, CGUIListItem *item, unsigned int row, unsigned int col, bool focused)
{
  if (!m_focusedLayout || !m_layout)
    return;

  if (m_scrollSpeed == 0 && item->GetPropertyBOOL("needsloading") && item->IsFileItem())
  {
    item->ClearProperty("needsloading");
    item->SetProperty("isloaded",false);

    m_itemsToLoad.push_back(item);
  }

  // set the origin
  g_graphicsContext.PushTransform(TransformMatrix::CreateTranslation(posX, posY));
  if (m_bInvalidated)
    item->SetInvalid();
  if (focused)
  {    
    if (!item->GetFocusedLayout())
    {
      CGUIListItemLayout *layout = new CGUIListItemLayout(*m_focusedLayout);
      item->SetFocusedLayout(layout);
    }
    if (item->GetFocusedLayout())
    {
      if (item != m_lastItem || !HasFocus())
      {
        item->GetFocusedLayout()->SetFocusedItem(0);
      }
      if (item != m_lastItem && HasFocus())
      {
        item->GetFocusedLayout()->ResetAnimation(ANIM_TYPE_UNFOCUS);
        unsigned int subItem = 1;
        if (m_lastItem && m_lastItem->GetFocusedLayout())
          subItem = m_lastItem->GetFocusedLayout()->GetFocusedItem();
        item->GetFocusedLayout()->SetFocusedItem(subItem ? subItem : 1);
      }
      if (m_firstRenderAfterLoad && HasAnimation(ANIM_TYPE_LIST_LOAD))
      {
        CAnimation *loadAnimation = GetAnimation(ANIM_TYPE_LIST_LOAD, false);
        loadAnimation->UpdateDelay(row, col);
        item->GetFocusedLayout()->AddAnimation(*loadAnimation);
        item->GetFocusedLayout()->ResetAnimation(ANIM_TYPE_LIST_LOAD);
        item->GetFocusedLayout()->QueueAnimation(ANIM_TYPE_LIST_LOAD);
      }      
      m_focusedPosX = posX;
      m_focusedPosY = posY;  

      item->GetFocusedLayout()->Render(item, m_parentID, m_renderTime);
    }
    m_lastItem = item;
  }
  else
  {
    if (item->GetFocusedLayout())
      item->GetFocusedLayout()->SetFocusedItem(0); // focus is not set
    if (!item->GetLayout())
    {
      CGUIListItemLayout *layout = new CGUIListItemLayout(*m_layout);
      item->SetLayout(layout);
    }
    if (item->GetFocusedLayout() && item->GetFocusedLayout()->IsAnimating(ANIM_TYPE_UNFOCUS))
      item->GetFocusedLayout()->Render(item, m_parentID, m_renderTime);
    else if (item->GetLayout())
    {
      if (m_firstRenderAfterLoad && HasAnimation(ANIM_TYPE_LIST_LOAD))
      {
        CAnimation *loadAnimation = GetAnimation(ANIM_TYPE_LIST_LOAD, false);
        loadAnimation->UpdateDelay(row, col);        
        item->GetLayout()->AddAnimation(*loadAnimation);
        item->GetLayout()->ResetAnimation(ANIM_TYPE_LIST_LOAD);
        item->GetLayout()->QueueAnimation(ANIM_TYPE_LIST_LOAD);
      }
      
      item->GetLayout()->Render(item, m_parentID, m_renderTime);
    }
  }
  g_graphicsContext.PopTransform();
}

bool CGUIBaseContainer::OnAction(const CAction &action)
{
  CSingleLock lock(m_itemsLock);

  if (action.id >= KEY_ASCII)
  {
    OnJumpLetter((char)(action.id & 0xff));
    return true;
  }

  switch (action.id)
  {
    case ACTION_MOVE_LEFT:
    case ACTION_MOVE_RIGHT:
    case ACTION_MOVE_DOWN:
    case ACTION_MOVE_UP:
    {
#ifdef OLD_SCROLLING
      if (!HasFocus()) return false;
      if (action.holdTime > HOLD_TIME_START &&
        ((m_orientation == VERTICAL && (action.id == ACTION_MOVE_UP || action.id == ACTION_MOVE_DOWN)) ||
         (m_orientation == HORIZONTAL && (action.id == ACTION_MOVE_LEFT || action.id == ACTION_MOVE_RIGHT))))
      { // action is held down - repeat a number of times
        float speed = std::min(1.0f, (float)(action.holdTime - HOLD_TIME_START) / (HOLD_TIME_END - HOLD_TIME_START));
        unsigned int itemsPerFrame = 1;
        if (m_lastHoldTime) // number of rows/10 items/second max speed
          itemsPerFrame = std::max((unsigned int)1, (unsigned int)(speed * 0.0001f * GetRows() * (CTimeUtils::GetFrameTime() - m_lastHoldTime)));
        m_lastHoldTime = CTimeUtils::GetFrameTime();
        if (action.id == ACTION_MOVE_LEFT || action.id == ACTION_MOVE_UP)
          while (itemsPerFrame--) MoveUp(false);
        else
          while (itemsPerFrame--) MoveDown(false);
        return true;
      }
      else
      {
        m_lastHoldTime = 0;
        return CGUIControl::OnAction(action);
      }
#else
      if (!HasFocus()) return false;

      CLog::Log(LOGNONE,"got click: holdtime=%d , repeat=%f , amount1=%f , amount2=%f , buttonCode=%d (haim)",action.holdTime, action.repeat, action.amount1, action.amount2, action.buttonCode);

      if (action.holdTime > HOLD_TIME_START &&
        ((m_orientation == VERTICAL && (action.id == ACTION_MOVE_UP || action.id == ACTION_MOVE_DOWN)) ||
         (m_orientation == HORIZONTAL && (action.id == ACTION_MOVE_LEFT || action.id == ACTION_MOVE_RIGHT))))
      { // action is held down - repeat a number of times

        //m_scrollTime is the time that takes the animation to scroll ones. (ie 400ms)
        //we need to calculate the delta we add each time there's an action, which is affected by the fps
        //on each scrolltime the m_itemsPerFrame should hit 1
        float acceleration = 1;

        if (action.holdTime < HOLD_TIME_MIN_ACCELERATION)
          m_currentScrollTime = m_scrollTime;

        m_itemsPerFrame += 1 / (m_currentScrollTime / g_graphicsContext.GetFPS());

        if ((int)m_itemsPerFrame > 0)
        {
          if (action.id == ACTION_MOVE_LEFT || action.id == ACTION_MOVE_UP)
          {
            while (((int) m_itemsPerFrame--) > 0)
            {
              MoveUp(false);
            }
          }
          else
          {
            while (((int) m_itemsPerFrame--) > 0)
            {
              MoveDown(false);
            }
          }

          if (m_scrollTimeMin < m_scrollTime && action.holdTime > HOLD_TIME_MIN_ACCELERATION)
          {
            //CLog::Log(LOGNONE,"m_currentScrollTime:%d , m_scrollTimer:%d (haim) , acceleration=%f ",m_currentScrollTime, m_scrollTimer.GetElapsedMilliseconds(), acceleration);

            if (action.holdTime < HOLD_TIME_MAX_ACCELERATION)
            {
              int holdtime = (action.holdTime - HOLD_TIME_MIN_ACCELERATION);

              if (holdtime < HOLD_TIME_DELTA_ACCELERATION)
              {
                acceleration = (holdtime / (float)HOLD_TIME_DELTA_ACCELERATION); //should be a value from 0 to 1
                m_currentScrollTime = m_scrollTime - (m_scrollTime - m_scrollTimeMin) * acceleration;
              }
              else
              {
                //top speed
                m_currentScrollTime = m_scrollTimeMin;
              }
            }
            else
            {
              //top speed
              m_currentScrollTime = m_scrollTimeMin;
            }
            m_itemsPerFrame = 0.0;
          }

        }
        return true;
      }
      else
      {
        bool bDeferred = false;
        if (IsScrolling())
        {
          //defer the action, let the Render function check when we're not scrolling and pop the action when possible and rerun this function
          CSingleLock lock(m_actionsLock);

          if (!m_deferredActions.empty() && (abs(m_deferredActions.front().id - action.id) == 1))
          {
            //for example if the last command is down and we click up or last is left and clicked right
            //empty the queue
            std::queue<CAction> empty;
            std::swap(m_deferredActions,empty);
          }

          m_deferredActions.push(action);

          bDeferred = true;
        }

        m_currentScrollTime = m_scrollTime;
        m_lastHoldTime = 0;

        if (!bDeferred)
        {
          return CGUIControl::OnAction(action);
        }
        else
        {
          return true;
        }
      }
#endif
    }
    break;

    case ACTION_FIRST_PAGE:
      SelectItem(0);
      return true;

    case ACTION_LAST_PAGE:
      if (m_items.size())
        SelectItem(m_items.size() - 1);
      return true;

    case ACTION_NEXT_LETTER:
    {
      OnNextLetter();
      return true;
    }
      break;
    case ACTION_PREV_LETTER:
    {
      OnPrevLetter();
      return true;
    }
      break;
    case ACTION_JUMP_SMS2:
    case ACTION_JUMP_SMS3:
    case ACTION_JUMP_SMS4:
    case ACTION_JUMP_SMS5:
    case ACTION_JUMP_SMS6:
    case ACTION_JUMP_SMS7:
    case ACTION_JUMP_SMS8:
    case ACTION_JUMP_SMS9:
    {
      OnJumpSMS(action.id - ACTION_JUMP_SMS2 + 2);
      return true;
    }
      break;

    case ACTION_POST_ANIM_CLICK:
      return OnClick(action.originalwID);
    
    case ACTION_SELECT_ITEM:
    case ACTION_MOUSE_CLICK:
    case ACTION_ENTER:
    case ACTION_MOUSE_DOUBLE_CLICK:
    case ACTION_MOUSE_XBUTTON1_CLICK:
    case ACTION_MOUSE_XBUTTON1_DOUBLE_CLICK:
      if (action.id && !m_clickAnimationStarted)
      {
        if (GetFocusedLayout() && GetFocusedLayout()->HasAnimation(ANIM_TYPE_CLICK))
        {
          GetFocusedLayout()->ResetAnimation(ANIM_TYPE_CLICK);
          GetFocusedLayout()->QueueAnimation(ANIM_TYPE_CLICK);
          m_clickAnimationStarted = true;        
          m_clickAction = action;
          m_clickAction.originalwID = m_clickAction.id;
          m_clickAction.id = ACTION_POST_ANIM_CLICK;
          
          return true;
        }
        else
        {
          return OnClick(action.id);
        }
      }
    default:
      return false;
  }
  return false;
}

bool CGUIBaseContainer::OnMessage(CGUIMessage& message)
{
  CSingleLock lock(m_itemsLock);

  if (message.GetControlId() == GetID())
  {
    if (!m_hasStaticContent)
    {
      if (message.GetMessage() == GUI_MSG_LOADING)
      {
        m_loading = true;
        m_loaded = false;
        CLog::Log(LOGDEBUG,"CGUIBaseContainer::CGUIBaseContainer,  GUI_MSG_LOADING, %d (basecontainer)", GetID());
        return true;
      }
      if (message.GetMessage() == GUI_MSG_LOAD_FAILED)
      {
        m_loading = false;
        m_loaded = true;
        CLog::Log(LOGDEBUG,"CGUIBaseContainer::CGUIBaseContainer,  GUI_MSG_LOAD_FAILED, %d (basecontainer)", GetID());
        return true;
      }
      if (message.GetMessage() == GUI_MSG_LABEL_BIND && message.GetPointer())
      { // bind our items
        m_loading = false;
        m_loaded = true;
        bool bAppend = message.GetParam2();

        CLog::Log(LOGDEBUG,"CGUIBaseContainer::CGUIBaseContainer,  GUI_MSG_LABEL_BIND, %d , append=%d (basecontainer)", GetID(), bAppend);

        CFileItemList *items = (CFileItemList *)message.GetPointer();

        if (!bAppend)
        {
          //if we're not appending something new, reset everything we had before
          Reset();
        }
        else
        {
          if (m_items.size() > 0)
          {
            //set the last item properties that it won't be last or first (we're going to append new items)
            m_items[m_items.size()-1]->SetProperty("is-first-in-container", false);
            m_items[m_items.size()-1]->SetProperty("is-last-in-container", false);
          }
        }

        for (int i = 0; i < items->Size(); i++)
        {
          m_items.push_back(items->Get(i));

          items->Get(i)->SetProperty("index", (int)(m_items.size()+1));

          if (items->Get(i) && items->Get(i)->HasProperty("itemid"))
            m_itemsIndex[items->Get(i)->GetProperty("itemid")] = m_items[m_items.size()-1];

          m_items[m_items.size()-1]->SetProperty("is-first-in-container", (m_items.size() == 1));
          m_items[m_items.size()-1]->SetProperty("is-last-in-container", (i == items->Size()-1));
        }

        UpdateLayout(true); // true to refresh all items
        UpdateScrollByLetter();

        int iSelecteItem = message.GetParam1();
        if (items->Size() > 0 && items->Size() > iSelecteItem)
          SelectItem(iSelecteItem>0?iSelecteItem:0);

        if (m_savedSelected != -1)
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetParentID(), GetID(), m_savedSelected);
          OnMessage(msg);

          m_savedSelected = -1;
        }

        m_firstRenderAfterLoad = true;
        return true;
      }
      if (message.GetMessage() == GUI_MSG_LABEL_ADD && message.GetItem())
      {
        CGUIListItemPtr item = message.GetItem();
        m_items.push_back(item);

        m_items[m_items.size()-1]->SetProperty("is-first-in-container", (m_items.size() == 1));
        m_items[m_items.size()-1]->SetProperty("is-last-in-container", true);

        if (m_items.size() > 1)
        {
          m_items[m_items.size()-2]->SetProperty("is-last-in-container", false);
        }

        if (item->HasProperty("itemid"))
          m_itemsIndex[item->GetProperty("itemid")] = m_items[m_items.size()-1];
        UpdateScrollByLetter();
        SetPageControlRange();
        return true;
      }
      else if (message.GetMessage() == GUI_MSG_LABEL_SET && message.GetItem())
      {
        CGUIListItemPtr item = message.GetItem();
        int itemIndex = message.GetParam1();
        m_items[itemIndex] = item;
        if (item->HasProperty("itemid"))
          m_itemsIndex[item->GetProperty("itemid")] = m_items[itemIndex];
        UpdateScrollByLetter();
        SetPageControlRange();
        return true;
      }
      else if (message.GetMessage() == GUI_MSG_ITEM_LOADED)
      {
        CFileItem *pItem = (CFileItem *) message.GetPointer();
        message.SetPointer(NULL);

        if (!pItem)
          return true;

        bool bFound = false;
        CGUIListItemPtr pListItem = m_itemsIndex[pItem->GetProperty("itemid")];
        if (pListItem && pListItem->GetProperty("itemid") == pItem->GetProperty("itemid"))
          bFound = true;
        
        for (size_t i = 0; !bFound && i < m_items.size(); i++)
        {
          if (m_items[i]->IsFileItem())
          {
            pListItem = m_items[i];
            // Compare the two items by item id
            if (pListItem && pListItem->GetProperty("itemid") == pItem->GetProperty("itemid"))
              bFound = true;
          }
        }
        
        if (bFound)
        {
          CFileItem *pTarget = (CFileItem *) pListItem.get();
          int indexValue = pTarget->GetPropertyInt("index");
          pItem->SetProperty("index",indexValue);
          *pTarget = *pItem;
        }

        delete pItem;
        return true;
      }
      else if (message.GetMessage() == GUI_MSG_LABEL_RESET)
      {
        m_loaded = false;
        if (message.GetParam1() == 1)
        {
          // Loading phase is over
          m_loading = false;
        }
        Reset();
        SetPageControlRange();
        return true;
      }
    }
    if (message.GetMessage() == GUI_MSG_ITEM_SELECTED)
    {
      message.SetParam1(GetSelectedItem());
      return true;
    }
    if (message.GetMessage() == GUI_MSG_JUMPTOLETTER)
    {
      if (!message.GetLabel().empty())
        OnJumpLetter(message.GetLabel()[0]);
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_PAGE_CHANGE)
    {
      if (message.GetSenderId() == m_pageControl && IsVisible())
      { // update our page if we're visible - not much point otherwise
        if ((int) message.GetParam1() != m_offset)
          m_pageChangeTimer.StartZero();
        ScrollToOffset(message.GetParam1());
        return true;
      }
    }
    else if (message.GetMessage() == GUI_MSG_REFRESH_LIST)
    { // update our list contents
      for (unsigned int i = 0; i < m_items.size(); ++i)
        m_items[i]->SetInvalid();
    }
    else if (message.GetMessage() == GUI_MSG_MOVE_OFFSET)
    {
      int count = (int) message.GetParam1();
      while (count < 0)
      {
        MoveUp(true);
        count++;
      }
      while (count > 0)
      {
        MoveDown(true);
        count--;
      }
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_MARK_ITEM)
    {
      int selected = message.GetParam1();      
      if (selected >= 0 && selected < (int) m_items.size())
      {
        m_items[selected]->Select(true);
      }
    }
    else if (message.GetMessage() == GUI_MSG_UNMARK_ITEM)
    {
      int selected = message.GetParam1();      
      if (selected >= 0 && selected < (int) m_items.size())
      {
        m_items[selected]->Select(false);
      }
    }
    else if (message.GetMessage() == GUI_MSG_UNMARK_ALL_ITEMS)
    {
      for (size_t i = 0; i < m_items.size(); i++)
      {
        m_items[i]->Select(false);
      }
    }
    else if (message.GetMessage() == GUI_MSG_MARK_ALL_ITEMS)
    {
      for (size_t i = 0; i < m_items.size(); i++)
      {
        m_items[i]->Select(true);
      }
    }
    else if (message.GetMessage() == GUI_MSG_RELOAD_CONTENT)
    {
      ReloadContent();
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_GET_ITEM)
    {
      int item = message.GetParam1();
      if (item < (int)m_items.size() && item >= 0)
      {
        message.SetItem(m_items[item]);
        return true;
      }
    }
    else if (message.GetMessage() == GUI_MSG_GET_ITEMS)
    {
      std::vector<CGUIListItemPtr>* vecRes = new std::vector<CGUIListItemPtr>();
      *vecRes = GetItems();
      message.SetPointer(vecRes);
    }
    else if (message.GetMessage() == GUI_MSG_GET_MARKED_ITEMS)
    {
      std::vector<CGUIListItemPtr>* vecRes = new std::vector<CGUIListItemPtr>();
      *vecRes = GetMarkedItems();
      message.SetPointer(vecRes);
    }
    else if (message.GetMessage() == GUI_MSG_SET_PATH)
    {
      SetPath(message.GetLabel());
    }
  }
  return CGUIControl::OnMessage(message);
}

void CGUIBaseContainer::OnUp()
{
  bool wrapAround = m_controlUp == GetID() || !(m_controlUp || m_upActions.size());
  if (m_orientation == VERTICAL && MoveUp(wrapAround))
    return;
  // with horizontal lists it doesn't make much sense to have multiselect labels
  CGUIControl::OnUp();
}

void CGUIBaseContainer::OnDown()
{
  bool wrapAround = m_controlDown == GetID() || !(m_controlDown || m_downActions.size());
  if (m_orientation == VERTICAL && MoveDown(wrapAround))
    return;
  // with horizontal lists it doesn't make much sense to have multiselect labels
  CGUIControl::OnDown();
}

void CGUIBaseContainer::OnLeft()
{
  bool wrapAround = m_controlLeft == GetID() || !(m_controlLeft || m_leftActions.size());
  if (m_orientation == HORIZONTAL && MoveUp(wrapAround))
    return;
  else if (m_orientation == VERTICAL)
  {
    CGUIListItemLayout *focusedLayout = GetFocusedLayout();
    if (focusedLayout && focusedLayout->MoveLeft())
      return;
  }
  CGUIControl::OnLeft();
}

void CGUIBaseContainer::OnRight()
{
  bool wrapAround = m_controlRight == GetID() || !(m_controlRight || m_rightActions.size());
  if (m_orientation == HORIZONTAL && MoveDown(wrapAround))
    return;
  else if (m_orientation == VERTICAL)
  {
    CGUIListItemLayout *focusedLayout = GetFocusedLayout();
    if (focusedLayout && focusedLayout->MoveRight())
      return;
  }
  CGUIControl::OnRight();
}

void CGUIBaseContainer::OnNextLetter()
{
  int offset = CorrectOffset(m_offset, m_cursor);
  for (unsigned int i = 0; i < m_letterOffsets.size(); i++)
  {
    if (m_letterOffsets[i].first > offset)
    {
      SelectItem(m_letterOffsets[i].first);
      return;
    }
  }
}

void
CGUIBaseContainer::OnPrevLetter()
{
  int offset = CorrectOffset(m_offset, m_cursor);
  if (!m_letterOffsets.size())
    return;
  for (int i = (int) m_letterOffsets.size() - 1; i >= 0; i--)
  {
    if (m_letterOffsets[i].first < offset)
    {
      SelectItem(m_letterOffsets[i].first);
      return;
    }
  }
}

void CGUIBaseContainer::OnJumpLetter(char letter)
{
  if (m_items.size() == 0)
    return;

  if (m_matchTimer.GetElapsedMilliseconds() < letter_match_timeout)
    m_match.push_back(letter);
  else
    m_match.Format("%c", letter);

  m_matchTimer.StartZero();

  // we can't jump through letters if we have none
  if (0 == m_letterOffsets.size())
    return;

  // find the current letter we're focused on
  unsigned int offset = CorrectOffset(m_offset, m_cursor);
  for (unsigned int i = (offset + 1) % m_items.size(); i != offset; i = (i+1) % m_items.size())
  {
    CGUIListItemPtr item = m_items[i];

    CStdString strItemLabel = GetCorrectLabel(item); //get the correct label according to the prefix setting
    if (0 == strnicmp(strItemLabel.c_str(), m_match.c_str(), m_match.size()))
    {
      SelectItem(i);
      return;
    }
  }
  // no match found - repeat with a single letter
  if (m_match.size() > 1)
  {
    m_match.clear();
    OnJumpLetter(letter);
  }
}

void CGUIBaseContainer::OnJumpSMS(int letter)
{
  static const char letterMap[8][6] = { "ABC2", "DEF3", "GHI4", "JKL5", "MNO6", "PQRS7", "TUV8", "WXYZ9" };

  // only 2..9 supported
  if (letter < 2 || letter > 9 || !m_letterOffsets.size())
    return;

  const CStdString letters = letterMap[letter - 2];
  // find where we currently are
  int offset = CorrectOffset(m_offset, m_cursor);
  unsigned int currentLetter = 0;
  while (currentLetter + 1 < m_letterOffsets.size() && m_letterOffsets[currentLetter + 1].first <= offset)
    currentLetter++;

  // now switch to the next letter
  CStdString current = m_letterOffsets[currentLetter].second;
  int startPos = (letters.Find(current) + 1) % letters.size();
  // now jump to letters[startPos], or another one in the same range if possible
  int pos = startPos;
  while (true)
  {
    // check if we can jump to this letter
    for (unsigned int i = 0; i < m_letterOffsets.size(); i++)
    {
      if (m_letterOffsets[i].second == letters.Mid(pos, 1))
      {
        SelectItem(m_letterOffsets[i].first);
        return;
      }
    }
    pos = (pos + 1) % letters.size();
    if (pos == startPos)
      return;
  }
}

bool CGUIBaseContainer::MoveUp(bool wrapAround)
{
  return true;
}

bool CGUIBaseContainer::MoveDown(bool wrapAround)
{
  return true;
}

// scrolls the said amount
void CGUIBaseContainer::Scroll(int amount)
{
  ScrollToOffset(m_offset + amount);
}

int CGUIBaseContainer::GetSelectedItem() const
{
  return CorrectOffset(m_offset, m_cursor);
}

CGUIListItemPtr CGUIBaseContainer::GetListItem(int offset, unsigned int flag) const
{
  if (!m_items.size())
    return CGUIListItemPtr();
  int item;
  if (flag & INFOFLAG_LISTITEM_SELECTED)
  {
    bool found = false;
    for (size_t i = 0; i < m_items.size(); i++)
    {
      if (m_items[i].get()->IsSelected())
      {
        item = i;
        found = true;
        break;
      }
    }
    
    if (!found)
    {
      return CGUIListItemPtr();
    }
  }
  else
  {
    item = GetSelectedItem() + offset;
  }

  if (flag & INFOFLAG_LISTITEM_POSITION) // use offset from the first item displayed, taking into account scrolling
    item = CorrectOffset((int) (m_scrollOffset / m_layout->Size(m_orientation)), offset);

  if (flag & INFOFLAG_LISTITEM_WRAP)
  {
    item %= ((int) m_items.size());
    if (item < 0)
      item += m_items.size();
    return m_items[item];
  }
  else
  {
    if (item >= 0 && item < (int) m_items.size())
      return m_items[item];
  }
  return CGUIListItemPtr();
}

CGUIListItemLayout *CGUIBaseContainer::GetFocusedLayout() const
{
  CGUIListItemPtr item = GetListItem(0);
  if (item.get())
    return item->GetFocusedLayout();
  return NULL;
}

bool CGUIBaseContainer::SelectItemFromPoint(const CPoint &point)
{
  if (!m_focusedLayout || !m_layout)
    return false;

  int row = 0;
  float pos = (m_orientation == VERTICAL) ? point.y : point.x;
  while (row < m_itemsPerPage + 1) // 1 more to ensure we get the (possible) half item at the end.
  {
    const CGUIListItemLayout *layout = (row == m_cursor) ? m_focusedLayout : m_layout;
    if (pos < layout->Size(m_orientation) && row + m_offset < (int) m_items.size())
    { // found correct "row" -> check horizontal
      if (!InsideLayout(layout, point))
        return false;

      MoveToItem(row);
      CGUIListItemLayout *focusedLayout = GetFocusedLayout();
      if (focusedLayout)
      {
        CPoint pt(point);
        if (m_orientation == VERTICAL)
          pt.y = pos;
        else
          pt.x = pos;
        focusedLayout->SelectItemFromPoint(pt);
      }
      return true;
    }
    row++;
    pos -= layout->Size(m_orientation);
  }
  return false;
}

bool CGUIBaseContainer::OnMouseOver(const CPoint &point)
{
  // select the item under the pointer
  SelectItemFromPoint(point - CPoint(m_posX, m_posY));
  return CGUIControl::OnMouseOver(point);
}

bool CGUIBaseContainer::OnMouseClick(int button, const CPoint &point)
{
  if (button == MOUSE_RIGHT_BUTTON)
    return false;
  
  if (SelectItemFromPoint(point - CPoint(m_posX, m_posY)))
  { // send click message to window
    OnClick(ACTION_MOUSE_CLICK + button);
    return true;
  }
  return false;
}

bool CGUIBaseContainer::OnMouseDoubleClick(int button, const CPoint &point)
{
  if (SelectItemFromPoint(point - CPoint(m_posX, m_posY)))
  { // send double click message to window
    OnClick(ACTION_MOUSE_DOUBLE_CLICK + button);
    return true;
  }
  return false;
}

bool CGUIBaseContainer::OnClick(int actionID)
{
  bool clickHandled = true;

  int subItem = 0;
  if (actionID == ACTION_SELECT_ITEM || actionID == ACTION_MOUSE_LEFT_CLICK)
  {
    if (m_selectionMode == SELECTION_SINGLE)
    {
      for (size_t i = 0; i < m_items.size(); i++)
      {
        m_items[i]->Select(false);
      }      
    }
    
    if (m_selectionMode != SELECTION_NONE)
    {
      int selected = GetSelectedItem();
      if (selected >= 0 && selected < (int) m_items.size())
      {
        m_items[selected]->Select(true);
      }
    }
    
    if (m_hasStaticContent || m_hasStaticActions)
    { // "select" action
      int selected = GetSelectedItem();
      if (selected >= 0 && selected < (int) m_items.size())
      {
        CFileItemPtr item = boost::static_pointer_cast<CFileItem>(m_items[selected]);

        CStdString actionsString;
        
        // In case both m_hasStaticActions and m_hasStaticContent are true (as in the case of action container)
        // the actions should have priority over content
        if (m_hasStaticActions)
        {
          if (m_staticActions.size() > 0)
          {
            int controlID = GetID();
            int parentID = GetParentID();
            vector<CGUIActionDescriptor> tempActions = m_staticActions;
            for (unsigned int i = 0; i < tempActions.size(); i++)
            {
              CGUIMessage message(GUI_MSG_EXECUTE, controlID, parentID);
              message.SetAction(tempActions[i]);
              g_windowManager.SendMessage(message);
            }          
          }
          else
          {
            clickHandled = false;
          }
        }
        else if (m_hasStaticContent)
        {
          actionsString = item->m_strPath;

          if(actionsString != "-")
          {
            int controlID = GetID();
            int parentID = GetParentID();
            vector<CStdString> actions;

            StringUtils::SplitString(actionsString, " , ", actions);
            for (unsigned int i = 0; i < actions.size(); i++)
            {
              CStdString action = actions[i];
              action.Replace(",,", ",");
              CGUIMessage message(GUI_MSG_EXECUTE, controlID, parentID);
              message.SetStringParam(action);
              g_windowManager.SendMessage(message);
            }
          }
          else
          {
            // in case the action is empty -> need to handle it
            clickHandled = false;
          }

          if (item->HasProperty("controlId"))
          {
            CGUIMessage msg(GUI_MSG_CLICKED, item->GetPropertyInt("controlId"), GetParentID(), actionID);
            g_application.getApplicationMessenger().SendGUIMessageToWindow(msg,GetParentID(),false);
            return true;
          }
        }
      }

      if (clickHandled)
      {
        return true;
      }
    }

    // grab the currently focused subitem (if applicable)
    CGUIListItemLayout *focusedLayout = GetFocusedLayout();
    if (focusedLayout)
      subItem = focusedLayout->GetFocusedItem();
  }
  // Don't know what to do, so send to our parent window.
  CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID(), actionID, subItem);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(msg,GetParentID(),false);
  return true;
  //return SendWindowMessage(msg);
}

bool CGUIBaseContainer::OnMouseWheel(char wheel, const CPoint &point)
{
  Scroll(-wheel);
  return true;
}

CStdString CGUIBaseContainer::GetDescription() const
{
  CStdString strLabel;
  int item = GetSelectedItem();
  if (item >= 0 && item < (int) m_items.size())
  {
    CGUIListItemPtr pItem = m_items[item];
    if (pItem->m_bIsFolder)
      strLabel.Format("[%s]", pItem->GetLabel().c_str());
    else
      strLabel = pItem->GetLabel();
  }
  return strLabel;
}

void CGUIBaseContainer::SetFocus(bool bOnOff)
{
  if (bOnOff != HasFocus())
  {
    SetInvalid();
    m_lastItem = NULL;
  }
  CGUIControl::SetFocus(bOnOff);
}

void CGUIBaseContainer::SaveStates(vector<CControlState> &states)
{
  CSingleLock lock(m_itemsLock);
  states.push_back(CControlState(GetID(), GetSelectedItem()));
}

void CGUIBaseContainer::SetPageControl(int id)
{
  m_pageControl = id;
}

void CGUIBaseContainer::ValidateOffset()
{
}

void CGUIBaseContainer::DoRender(unsigned int currentTime)
{
  m_renderTime = currentTime;

  m_itemsToLoad.clear();
  CGUIControl::DoRender(currentTime);
  for (int i=(int)m_itemsToLoad.size()-1; i>=0 ; i--)
  {
    if (m_itemsToLoad[i]->GetPropertyBOOL("needsloading") || m_itemsToLoad[i]->GetThumbnailImage() == PLACEHOLDER_IMAGE)
    {
      m_itemsToLoad[i]->SetThumbnailImage(m_itemsToLoad[i]->GetProperty("OriginalThumb"));
    }

    //we can't process anything too intensive on the UI thread
    if (!ScrollingDown() && !ScrollingUp())
    {
      //CLog::Log(LOGNONE,"Scrolling stopped. Requesting item loading...");
      g_application.GetItemLoader().LoadItem((CFileItem*)m_itemsToLoad[i] , true);
    }
  }
  m_itemsToLoad.clear();
    
  if (m_pageChangeTimer.GetElapsedMilliseconds() > 200)
    m_pageChangeTimer.Stop();
  m_wasReset = false;
}

void CGUIBaseContainer::AllocResources()
{
  CalculateLayout();
}

void CGUIBaseContainer::FreeResources(bool immediately)
{
  CGUIControl::FreeResources(immediately);

  CSingleLock lock(m_itemsLock);

  if (m_hasStaticContent)
  { // free any static content
    Reset();
    m_staticItems.clear();
  }
  m_scrollSpeed = 0;
}

void CGUIBaseContainer::UpdateLayout(bool updateAllItems)
{
  if (updateAllItems)
  { // free memory of items
    for (iItems it = m_items.begin(); it != m_items.end(); it++)
      (*it)->FreeMemory();
  }
  // and recalculate the layout
  CalculateLayout();
  SetPageControlRange();
}

void CGUIBaseContainer::SetPageControlRange()
{
  if (m_pageControl)
  {
    CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, GetRows());
    SendWindowMessage(msg);
  }
}

void CGUIBaseContainer::UpdatePageControl(int offset)
{
  if (m_pageControl && m_updatePageControl)
  { // tell our pagecontrol (scrollbar or whatever) to update (offset it by our cursor position)
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), m_pageControl, offset);
    SendWindowMessage(msg);

    m_updatePageControl = false;
  }
}

void CGUIBaseContainer::UpdateVisibility(const CGUIListItem *item)
{
  CGUIControl::UpdateVisibility(item);

  if (!IsVisible())
    return; // no need to update the content if we're not visible

  // check whether we need to update our layouts
  if ((m_layout && m_layout->GetCondition() && !g_infoManager.GetBool(m_layout->GetCondition(), GetParentID()))
      || (m_focusedLayout && m_focusedLayout->GetCondition() && !g_infoManager.GetBool(m_focusedLayout->GetCondition(),
          GetParentID())))
  {
    // and do it
    int item = GetSelectedItem();
    UpdateLayout(true); // true to refresh all items
    SelectItem(item);
  }

  if (m_hasStaticContent)
  { // update our item list with our new content, but only add those items that should
    // be visible.  Save the previous item and keep it if we are adding that one.
    CGUIListItem *lastItem = m_lastItem;
    Reset();
    bool updateItems = false;
    if (!m_staticUpdateTime)
      m_staticUpdateTime = CTimeUtils::GetFrameTime();
    if (CTimeUtils::GetFrameTime() - m_staticUpdateTime > 1000)
    {
      m_staticUpdateTime = CTimeUtils::GetFrameTime();
      updateItems = true;
    }
    for (unsigned int i = 0; i < m_staticItems.size(); ++i)
    {
      CFileItemPtr item = boost::static_pointer_cast<CFileItem>(m_staticItems[i]);
      // m_idepth is used to store the visibility condition
      if (!item->m_idepth || g_infoManager.GetBool(item->m_idepth, GetParentID()))
      {
        m_items.push_back(item);
        if (item->HasProperty("itemid"))
          m_itemsIndex[item->GetProperty("itemid")] = m_items[m_items.size()-1];

        if (item.get() == lastItem)
          m_lastItem = lastItem;
      }
      if (updateItems && item->HasProperties()) 
      { // has info, so update it
        CStdString info = item->GetProperty("label");
        if (!info.IsEmpty()) item->SetLabel(CGUIInfoLabel::GetLabel(info));
        info = item->GetProperty("label2");
        if (!info.IsEmpty()) item->SetLabel2(CGUIInfoLabel::GetLabel(info));
        info = item->GetProperty("icon");
        if (!info.IsEmpty()) item->SetIconImage(CGUIInfoLabel::GetLabel(info, true));
        info = item->GetProperty("thumb");
        if (!info.IsEmpty()) item->SetThumbnailImage(CGUIInfoLabel::GetLabel(info, true));
    }
    }
    UpdateScrollByLetter();
  }
}

void CGUIBaseContainer::CalculateLayout()
{
  CGUIListItemLayout *oldFocusedLayout = m_focusedLayout;
  CGUIListItemLayout *oldLayout = m_layout;
  GetCurrentLayouts();

  // calculate the number of items to display

  if (!m_focusedLayout || !m_layout)
  {
    CLog::Log(LOGERROR,"CGUIBaseContainer::CalculateLayout, either item or focused layout is missing from control = %d, parent = %d (control)", m_controlID, m_parentID);
    return;
  }

  assert(m_focusedLayout && m_layout);

  if (oldLayout == m_layout && oldFocusedLayout == m_focusedLayout)
    return; // nothing has changed, so don't update stuff

  m_itemsPerPage = (int) ((Size() - m_focusedLayout->Size(m_orientation)) / m_layout->Size(m_orientation) + 1);

  // ensure that the scroll offset is a multiple of our size
  m_scrollOffset = m_offset * m_layout->Size(m_orientation);
}

void CGUIBaseContainer::UpdateScrollByLetter()
{
  m_letterOffsets.clear();

  // for scrolling by letter we have an offset table into our vector.
  CStdString currentMatch;
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CGUIListItemPtr item = m_items[i];
    // The letter offset jumping is only for ASCII characters at present, and
    // our checks are all done in uppercase
    CStdString nextLetter = item->GetSortLabel().Left(1);
    nextLetter.ToUpper();
    if (currentMatch != nextLetter)
    {
      currentMatch = nextLetter;
      m_letterOffsets.push_back(make_pair((int) i, currentMatch));
    }
  }
}

unsigned int CGUIBaseContainer::GetRows() const
{
  return m_items.size();
}

inline float CGUIBaseContainer::Size() const
{
  return (m_orientation == HORIZONTAL) ? m_width : m_height;
}

#define MAX_SCROLL_AMOUNT 0.4f

void CGUIBaseContainer::ScrollToOffset(int offset)
{
  float size = (m_layout) ? m_layout->Size(m_orientation) : 10.0f;
  int range = m_itemsPerPage / 4;
  if (range <= 0) range = 1;
  if (offset * size < m_scrollOffset && m_scrollOffset - offset * size > size * range)
  { // scrolling up, and we're jumping more than 0.5 of a screen
    m_scrollOffset = (offset + range) * size;
  }
  if (offset * size > m_scrollOffset && offset * size - m_scrollOffset > size * range)
  { // scrolling down, and we're jumping more than 0.5 of a screen
    m_scrollOffset = (offset - range) * size;
  }
  m_scrollSpeed = (offset * size - m_scrollOffset) / m_currentScrollTime;
  if (!m_wasReset)
  {
    g_infoManager.SetContainerMoving(GetID(), offset - m_offset);
    if (m_scrollSpeed)
      m_scrollTimer.Start();
    else
      m_scrollTimer.Stop();
  }
  m_offset = offset;  

  m_updatePageControl = true;
}

void CGUIBaseContainer::UpdateScrollOffset()
{
  m_scrollOffset += m_scrollSpeed * (m_renderTime - m_scrollLastTime);
  if ((m_scrollSpeed < 0 && m_scrollOffset < m_offset * m_layout->Size(m_orientation)) ||
      (m_scrollSpeed > 0 && m_scrollOffset > m_offset * m_layout->Size(m_orientation)))
  {
    m_scrollOffset = m_offset * m_layout->Size(m_orientation);
    m_scrollSpeed = 0;
    m_scrollTimer.Stop();
  }
  m_scrollLastTime = m_renderTime;

  m_updatePageControl = true;
}

int CGUIBaseContainer::CorrectOffset(int offset, int cursor) const
{
  return offset + cursor;
}

void CGUIBaseContainer::Reset()
{
  CSingleLock lock(m_itemsLock);

  m_wasReset = true;
  m_items.clear();
  m_itemsIndex.clear();
  m_lastItem = NULL;
}

void CGUIBaseContainer::LoadLayout(TiXmlElement *layout)
{
  TiXmlElement* itemElement = layout->FirstChildElement("itemlayout");
  while (itemElement)
  { // we have a new item layout
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, false);
    m_layouts.push_back(itemLayout);
    itemElement = itemElement->NextSiblingElement("itemlayout");
  }

  itemElement = layout->FirstChildElement("focusedlayout");
  while (itemElement)
  { // we have a new item layout
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, true);
    m_focusedLayouts.push_back(itemLayout);
    itemElement = itemElement->NextSiblingElement("focusedlayout");
  }
}

void CGUIBaseContainer::ReloadContent()
{
  if (m_strPath == "")
  {
    CLog::Log(LOGDEBUG, "FEED, CGUIBaseContainer, path is empty");
    return;
  }

  CSingleLock lock(m_itemsLock);

  CLog::Log(LOGDEBUG, "FEED, CGUIBaseContainer, reload content from path = %s", m_strPath.c_str());
  Reset();
  CFileItemList items;
  DIRECTORY::CDirectory::GetDirectory(m_strPath, items);
  for (int i = 0; i < items.Size(); i++)
  {
    m_items.push_back(items[i]);
    if (items[i] && items[i]->HasProperty("itemid"))
      m_itemsIndex[items[i]->GetProperty("itemid")] = m_items[m_items.size()-1];
  }

}

void CGUIBaseContainer::LoadContent(TiXmlElement *content)
{
  CSingleLock lock(m_itemsLock);

  TiXmlElement *root = content->FirstChildElement("content");
  if (!root)
    return;

  g_SkinInfo.ResolveIncludes(root);

  CStdString contentType = root->Attribute("type");

  if (contentType == "directory")
  {
    if (root->GetText())
    {
      m_strPath = root->GetText();
      m_strPath.Trim();
      const CStdString constStrPath = m_strPath;
      CGUIInfoLabel info(constStrPath);
      m_strPath = info.GetLabel(g_windowManager.GetActiveWindow(), false);
    }
    //m_hasStaticContent = false;
    m_hasStaticActions = false;
  }
  else if (contentType == "url")
  {
    m_strPath = root->Attribute("url");
    m_strPath.Trim();
    const CStdString constStrPath = m_strPath;
    CGUIInfoLabel info(constStrPath);
    m_strPath = info.GetLabel(g_windowManager.GetActiveWindow(), false);
    //m_hasStaticContent = false;
    m_hasStaticActions = true;
  }
  else if (contentType == "action")
  {
    //m_hasStaticContent = true;
    m_hasStaticActions = true;
  }
  else
  {
    //m_hasStaticContent = true;
    m_hasStaticActions = false;
  }

  if (m_hasStaticActions)
  {
    CGUIControlFactory::GetMultipleString(root, "onclick", m_staticActions);    
  }

  vector<CGUIListItemPtr> items;

  TiXmlElement *item = root->FirstChildElement("item");
  
  // In case there are items in the content section, we use them as static content  
  if (item) 
  {
    m_hasStaticContent = true;
  }
  else
  {
    m_hasStaticContent = false;
  }
  
  while (item)
  {
    // format:
    // <item controlid="511" label="Cool Video" label2="" thumb="mythumb.png">PlayMedia(c:\videos\cool_video.avi)</item>
    // <item controlid="511" label="My Album" label2="" thumb="whatever.jpg">ActivateWindow(MyMusic,c:\music\my album)</item>
    // <item controlid="511" label="Apple Movie Trailers" label2="Bob" thumb="foo.tbn">RunScript(special://xbmc/scripts/apple movie trailers/default.py)</item>

    // OR the more verbose, but includes-friendly:
    // <item>
    //   <controlid>511</controlid>
    //   <label>blah</label>
    //   <label2>foo</label2>
    //   <thumb>bar.png</thumb>
    //   <icon>foo.jpg</icon>
    //   <onclick>ActivateWindow(Home)</onclick>
    // </item>
    g_SkinInfo.ResolveIncludes(item);
    if (item->FirstChild())
    {
      CFileItemPtr newItem;
      // check whether we're using the more verbose method...
      TiXmlNode *click = item->FirstChild("onclick");
      if (click && click->FirstChild())
      {
        CStdString controlId, label, label2, thumb, icon;
        XMLUtils::GetString(item, "controlid", controlId);
        XMLUtils::GetString(item, "label", label);   label  = CGUIControlFactory::FilterLabel(label);
        XMLUtils::GetString(item, "label2", label2); label2 = CGUIControlFactory::FilterLabel(label2);
        XMLUtils::GetString(item, "thumb", thumb);   thumb  = CGUIControlFactory::FilterLabel(thumb);
        XMLUtils::GetString(item, "icon", icon);     icon   = CGUIControlFactory::FilterLabel(icon);

        const char *id = item->Attribute("id");
        int visibleCondition = 0;
        CGUIControlFactory::GetConditionalVisibility(item, visibleCondition);
        newItem.reset(new CFileItem(CGUIInfoLabel::GetLabel(label)));
        // multiple action strings are concat'd together, separated with " , "
        vector<CGUIActionDescriptor> actions;
        CGUIControlFactory::GetMultipleString(item, "onclick", actions);
        newItem->m_strPath = "";
        for (vector<CGUIActionDescriptor>::iterator it = actions.begin(); it != actions.end(); ++it)
        {
          (*it).m_action.Replace(",", ",,");
          if (newItem->m_strPath.length() > 0)
          {
            newItem->m_strPath   += " , ";
          }
          newItem->m_strPath += (*it).m_action;          
        }
        newItem->SetLabel2(CGUIInfoLabel::GetLabel(label2));
        newItem->SetThumbnailImage(CGUIInfoLabel::GetLabel(thumb, true));
        newItem->SetIconImage(CGUIInfoLabel::GetLabel(icon, true));

        TiXmlElement* propElem = item->FirstChildElement("property");
        while (propElem && propElem->FirstChild() && propElem->FirstChild()->Value() && propElem->Attribute("name"))
        {
          newItem->SetProperty(propElem->Attribute("name"), propElem->FirstChild()->Value());
          propElem = propElem->NextSiblingElement("property");
        }

        if (label.Find("$INFO") >= 0) newItem->SetProperty("label", label);
        if (label2.Find("$INFO") >= 0) newItem->SetProperty("label2", label2);
        if (icon.Find("$INFO") >= 0) newItem->SetProperty("icon", icon);
        if (thumb.Find("$INFO") >= 0) newItem->SetProperty("thumb", thumb);
        if (id)
          newItem->m_iprogramCount = atoi(id);
        newItem->m_idepth = visibleCondition;
        if (!controlId.IsEmpty())
          newItem->SetProperty("controlId", atoi(controlId.c_str()));
      }
      else
      {
        CStdString label, label2, thumb, icon;
        label  = item->Attribute("label");  label  = CGUIControlFactory::FilterLabel(label);
        label2 = item->Attribute("label2"); label2 = CGUIControlFactory::FilterLabel(label2);
        thumb  = item->Attribute("thumb");  thumb  = CGUIControlFactory::FilterLabel(thumb);
        icon   = item->Attribute("icon");   icon   = CGUIControlFactory::FilterLabel(icon);

        const char *id = item->Attribute("id");
        newItem.reset(new CFileItem(CGUIInfoLabel::GetLabel(label)));
        newItem->m_strPath = item->FirstChild()->Value();
        newItem->SetLabel2(CGUIInfoLabel::GetLabel(label2));
        newItem->SetThumbnailImage(CGUIInfoLabel::GetLabel(thumb, true));
        newItem->SetIconImage(CGUIInfoLabel::GetLabel(icon, true));
        TiXmlElement* propElem = item->FirstChildElement("property");
        while (propElem && propElem->FirstChild() && propElem->FirstChild()->Value() && propElem->Attribute("name"))
        {
          newItem->SetProperty(propElem->Attribute("name"), propElem->FirstChild()->Value());
          propElem = propElem->NextSiblingElement("property");
        }
        if (id) newItem->m_iprogramCount = atoi(id);
        newItem->m_idepth = 0; // no visibility condition
      }
      m_staticItems.push_back(newItem);
    }
    item = item->NextSiblingElement("item");
  }

  if (m_staticItems.size() > 0)
  {
    m_staticItems[0]->SetProperty("is-first-in-container", true);
    m_staticItems[m_staticItems.size()-1]->SetProperty("is-last-in-container", true);
  }
}

void CGUIBaseContainer::SetStaticContent(const vector<CGUIListItemPtr> &items)
{
  CSingleLock lock(m_itemsLock);

  m_hasStaticContent = true;
  m_staticUpdateTime = 0;
  m_staticUpdateTime = 0;
  m_staticItems.clear();
  m_staticItems.assign(items.begin(), items.end());
  UpdateVisibility();
}

void CGUIBaseContainer::SetOffsets(float offsetX, float offsetY)
{
  m_offsetX = offsetX;
  m_offsetY = offsetY;
}

void CGUIBaseContainer::SetType(VIEW_TYPE type, const CStdString &label)
{
  m_type = type;
  m_label = label;
}

void CGUIBaseContainer::SetScrollTimeMin(int scrollTimeMin)
{
  m_scrollTimeMin = scrollTimeMin;
}

void CGUIBaseContainer::MoveToItem(int item)
{
  g_infoManager.SetContainerMoving(GetID(), item - m_cursor);
  m_cursor = item;
}

void CGUIBaseContainer::FreeMemory(int keepStart, int keepEnd)
{
  CSingleLock lock(m_itemsLock);
  if (keepStart < keepEnd)
  { // remove before keepStart and after keepEnd
    for (int i = 0; i < keepStart && i < (int) m_items.size(); ++i)
    {
      m_items[i]->FreeMemory();
    }
    for (int i = keepEnd + 1; i > 0 && i < (int) m_items.size(); ++i)
    {
      m_items[i]->FreeMemory();
    }
  }
  else
  { // wrapping
    for (int i = keepEnd + 1; i > 0 && i < keepStart && i < (int) m_items.size(); ++i)
      m_items[i]->FreeMemory();
  }
}

bool CGUIBaseContainer::InsideLayout(const CGUIListItemLayout *layout, const CPoint &point)
{
  if (!layout)
    return false;
  if ((m_orientation == VERTICAL && layout->Size(HORIZONTAL) && point.x > layout->Size(HORIZONTAL)) || (m_orientation
      == HORIZONTAL && layout->Size(VERTICAL) && point.y > layout->Size(VERTICAL)))
    return false;
  return true;
}

#ifdef _DEBUG
void CGUIBaseContainer::DumpTextureUse()
{
  CLog::Log(LOGDEBUG, "%s for container %u", __FUNCTION__, GetID());
  for (unsigned int i = 0; i < m_items.size(); ++i)
  {
    CGUIListItemPtr item = m_items[i];
    if (item->GetFocusedLayout()) item->GetFocusedLayout()->DumpTextureUse();
    if (item->GetLayout()) item->GetLayout()->DumpTextureUse();
  }
}
#endif

bool CGUIBaseContainer::GetCondition(int condition, int data) const
{
  switch (condition)
  {
    case CONTAINER_ROW:
      return (m_orientation == VERTICAL) ? (m_cursor == data) : true;
    case CONTAINER_COLUMN:
      return (m_orientation == HORIZONTAL) ? (m_cursor == data) : true;
    case CONTAINER_POSITION:
      return (m_cursor == data);
    case CONTAINER_HAS_NEXT:
      return (HasNextPage());
    case CONTAINER_HAS_PREVIOUS:
      return (HasPreviousPage());
    case CONTAINER_SUBITEM:
    {
      CGUIListItemLayout *layout = GetFocusedLayout();
      return layout ? (layout->GetFocusedItem() == (unsigned int) data) : false;
    }
    case CONTAINER_IS_LOADING:
      return m_loading;
    case CONTAINER_IS_LOADED:
          return m_loaded;
    case CONTAINER_IS_EMPTY:
      return m_items.empty();
    case CONTAINER_SCROLLING:
      return IsScrolling();
    default:
      return false;
  }
}

bool CGUIBaseContainer::IsScrolling() const
{
  return (m_scrollTimer.GetElapsedMilliseconds() > m_currentScrollTime || m_pageChangeTimer.IsRunning());
}

void CGUIBaseContainer::GetCurrentLayouts()
{
  m_layout = NULL;
  for (unsigned int i = 0; i < m_layouts.size(); i++)
  {
    int condition = m_layouts[i].GetCondition();
    if (!condition || g_infoManager.GetBool(condition, GetParentID()))
    {
      m_layout = &m_layouts[i];
      break;
    }
  }
  if (!m_layout && m_layouts.size())
    m_layout = &m_layouts[0]; // failsafe

  m_focusedLayout = NULL;
  for (unsigned int i = 0; i < m_focusedLayouts.size(); i++)
  {
    int condition = m_focusedLayouts[i].GetCondition();
    if (!condition || g_infoManager.GetBool(condition, GetParentID()))
    {
      m_focusedLayout = &m_focusedLayouts[i];
      break;
    }
  }
  if (!m_focusedLayout && m_focusedLayouts.size())
    m_focusedLayout = &m_focusedLayouts[0]; // failsafe
}

bool CGUIBaseContainer::HasNextPage() const
{
  return false;
}

bool CGUIBaseContainer::HasPreviousPage() const
{
  return false;
}

CStdString CGUIBaseContainer::GetLabel(int info) const
{
  CStdString label;
  switch (info)
  {
    case CONTAINER_NUM_PAGES:
      label.Format("%u", (((GetRows() + m_itemsPerPage - 1) / m_itemsPerPage) > 0 ? ((GetRows() + m_itemsPerPage - 1)
          / m_itemsPerPage) : 1));
      break;
    case CONTAINER_CURRENT_PAGE:
      label.Format("%u", GetCurrentPage());
      break;
    case CONTAINER_POSITION:
      label.Format("%i", m_cursor);
      break;
    case CONTAINER_NUM_ITEMS:
    {
      unsigned int numItems = GetNumItems();
      if (numItems && m_items[0]->IsFileItem() && (boost::static_pointer_cast<CFileItem>(m_items[0]))->IsParentFolder())
        label.Format("%u", numItems - 1);
      else
        label.Format("%u", numItems);
    }
      break;
    default:
      break;
  }
  return label;
}

int CGUIBaseContainer::GetCurrentPage() const
{
  if (m_offset + m_itemsPerPage >= (int) GetRows()) // last page
    return (GetRows() + m_itemsPerPage - 1) / m_itemsPerPage;
  return m_offset / m_itemsPerPage + 1;
}

//Boxee
void CGUIBaseContainer::SetSingleSelectedItem(int offset)
{
  for (size_t i = 0; i < m_items.size(); i++)
    m_items[i]->Select(false);
  m_items[GetSelectedItem()]->Select(true);
}

CGUIListItemPtr CGUIBaseContainer::GetSelectedItemPtr()
{
  int offset = GetSelectedItem();

  if (offset >= 0 && offset < (int)m_items.size())
    return m_items[offset];
  else
    return CGUIListItemPtr();
}

std::vector< CGUIListItemPtr > CGUIBaseContainer::GetItems()
{
  return m_items;
}

std::vector< CGUIListItemPtr >& CGUIBaseContainer::GetItemsByRef()
{
  return m_items;
}

std::vector< CGUIListItemPtr > CGUIBaseContainer::GetMarkedItems()
{
  std::vector< CGUIListItemPtr > result;
  for (size_t i = 0; i < m_items.size(); i++)
  {
    if (m_items[i]->IsSelected())
    {
      result.push_back(m_items[i]);
    }
  }
  
  return result;
}

void CGUIBaseContainer::SetItems(std::vector< CGUIListItemPtr > items)
{
  CSingleLock lock(m_itemsLock);

  Reset();
  for (size_t i = 0; i < items.size(); i++)
  {
    m_items.push_back(items[i]);
    if (items[i] && items[i]->HasProperty("itemid"))
      m_itemsIndex[items[i]->GetProperty("itemid")] = m_items[m_items.size()-1];
  }
}

//end Boxee

void CGUIBaseContainer::GetCacheOffsets(int &cacheBefore, int &cacheAfter)
{
  if (m_scrollSpeed > 0)
  {
    cacheBefore = 0;
    cacheAfter = m_cacheItems;
  }
  else if (m_scrollSpeed < 0)
  {
    cacheBefore = m_cacheItems;
    cacheAfter = 0;
  }
  else
  {
    cacheBefore = m_cacheItems / 2;
    cacheAfter = m_cacheItems / 2;
  }
}

float CGUIBaseContainer::GetFocusedPositionX() const
{
  return m_focusedPosX;
}

float CGUIBaseContainer::GetFocusedPositionY() const
{
  return m_focusedPosY;
}

void CGUIBaseContainer::SetAutoScrollTime(int autoScrollTime)
{
  m_autoScrollTime = autoScrollTime;
}

void CGUIBaseContainer::SetVisible(bool bVisible)
{
  if (bVisible && m_autoScrollTime > 0)  
  {
    m_autoScrollTimer.StartZero();
  }
  CGUIControl::SetVisible(bVisible);
}

void CGUIBaseContainer::Lock()
{
  ::EnterCriticalSection(&m_itemsLock);
}

void CGUIBaseContainer::Unlock()
{
  ::LeaveCriticalSection(&m_itemsLock);
}

CCriticalSection &CGUIBaseContainer::GetLock()
{
  return m_itemsLock;
}

// boxee
CStdString CGUIBaseContainer::GetCorrectLabel(const CGUIListItemPtr item)
{
  bool IgnorePrefix = g_guiSettings.GetBool("sort.showstarter");

  if (IgnorePrefix)
    return item->GetSortLabel();
  else
    return item->GetLabel();
}
// boxee end
