/*!
\file GUIListContainer.h
\brief
*/

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

#include "GUIControl.h"
#include "GUIListItemLayout.h"
#include "boost/shared_ptr.hpp"
#include "utils/Stopwatch.h"
#include "Key.h"
#include "utils/CriticalSection.h"

typedef boost::shared_ptr<CGUIListItem> CGUIListItemPtr;

/*!
 \ingroup controls
 \brief
 */

typedef enum { SELECTION_NONE = 0, SELECTION_SINGLE = 1, SELECTION_MULTIPLE = 2 } ListSelectionMode;

class CGUIBaseContainer : public CGUIControl
{
  
public:
  CGUIBaseContainer(int parentID, int controlId, float posX, float posY, float width, float height, ORIENTATION orientation, int scrollTime, int preloadItems, ListSelectionMode selectionMode);
  virtual ~CGUIBaseContainer(void);

  virtual bool OnAction(const CAction &action);
  virtual void OnDown();
  virtual void OnUp();
  virtual void OnLeft();
  virtual void OnRight();
  virtual bool OnMouseOver(const CPoint &point);
  virtual bool OnMouseClick(int button, const CPoint &point);
  virtual bool OnMouseDoubleClick(int button, const CPoint &point);
  virtual bool OnMouseWheel(char wheel, const CPoint &point);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void SetFocus(bool bOnOff);
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);
  virtual void SetVisible(bool bVisible);

  virtual unsigned int GetRows() const;

  virtual bool HasNextPage() const;
  virtual bool HasPreviousPage() const;

  void SetPageControl(int id);

  virtual CStdString GetDescription() const;
  virtual void SaveStates(std::vector<CControlState> &states);
  virtual int GetSelectedItem() const;

  virtual void DoRender(unsigned int currentTime);
  void LoadLayout(TiXmlElement *layout);
  void LoadContent(TiXmlElement *content);
  void ReloadContent();

  VIEW_TYPE GetType() const { return m_type; };
  const CStdString &GetLabel() const { return m_label; };
  void SetType(VIEW_TYPE type, const CStdString &label);
  void SetScrollTimeMin(int scrollTimeMin);

  virtual bool IsContainer() const { return true; };
  bool IsScrolling() const;
  CGUIListItemPtr GetListItem(int offset, unsigned int flag = 0) const;

  virtual bool GetCondition(int condition, int data) const;
  CStdString GetLabel(int info) const;
  
  CGUIListItemPtr GetSelectedItemPtr();
  std::vector< CGUIListItemPtr > GetMarkedItems();
  std::vector< CGUIListItemPtr > GetItems();
  std::vector< CGUIListItemPtr >& GetItemsByRef();  
  void SetItems(std::vector< CGUIListItemPtr >);

  void SetStaticContent(const std::vector<CGUIListItemPtr> &items);
  void SetOffsets(float offsetX, float offsetY);

#ifdef _DEBUG
  virtual void DumpTextureUse();
#endif
//Boxee
  virtual void SetSingleSelectedItem(int offset=0);
  CStdString GetPath() const { return m_strPath; }
  void SetPath(const CStdString& strPath) { m_strPath = strPath; }
  int GetItemsPerPage(){return m_itemsPerPage;};
//end Boxee
  virtual unsigned int GetNumItems() const { return m_items.size(); };
  
  float GetFocusedPositionX() const;
  float GetFocusedPositionY() const;
  
  void SetAutoScrollTime(int autoScrollTime);

  void Lock();
  void Unlock();
  CCriticalSection &GetLock();
  
protected:
  bool OnClick(int actionID);
  virtual bool SelectItemFromPoint(const CPoint &point);
  virtual void Render();
  virtual void RenderItem(float posX, float posY, CGUIListItem *item, unsigned int row, unsigned int col, bool focused);
  virtual void Scroll(int amount);
  virtual bool MoveDown(bool wrapAround);
  virtual bool MoveUp(bool wrapAround);
  virtual void MoveToItem(int item);
  virtual void ValidateOffset();
  virtual int  CorrectOffset(int offset, int cursor) const;
  virtual void UpdateLayout(bool refreshAllItems = false);
  virtual void SetPageControlRange();
  virtual void UpdatePageControl(int offset);
  virtual void CalculateLayout();
  virtual void SelectItem(int item) {};
  virtual void Reset();
  virtual int GetCurrentPage() const;
  bool InsideLayout(const CGUIListItemLayout *layout, const CPoint &point);
  CStdString GetCorrectLabel(const CGUIListItemPtr item);

  inline float Size() const;
  void MoveToRow(int row);
  void FreeMemory(int keepStart, int keepEnd);
  void GetCurrentLayouts();
  CGUIListItemLayout *GetFocusedLayout() const;

  float m_offsetX; // render offsets for starting items
  float m_offsetY;
    
  int m_offset;
  int m_cursor;
  float m_analogScrollCount;
  unsigned int m_lastHoldTime;
  float m_itemsPerFrame;
  CCriticalSection m_actionsLock;

  ORIENTATION m_orientation;
  int m_itemsPerPage;

  std::vector< CGUIListItemPtr > m_items;
  std::vector< CGUIListItem*   > m_itemsToLoad;
  std::map< CStdString, CGUIListItemPtr > m_itemsIndex;
  
  typedef std::vector<CGUIListItemPtr> ::iterator iItems;
  CGUIListItem *m_lastItem;

  int m_pageControl;
  bool m_updatePageControl;

  unsigned int m_renderTime;

  std::vector<CGUIListItemLayout> m_layouts;
  std::vector<CGUIListItemLayout> m_focusedLayouts;

  CGUIListItemLayout *m_layout;
  CGUIListItemLayout *m_focusedLayout;

  virtual void ScrollToOffset(int offset);
  void UpdateScrollOffset();

  unsigned int m_scrollLastTime;
  int   m_scrollTime;
  int   m_scrollTimeMin;
  int   m_currentScrollTime;
  float m_scrollOffset;

  VIEW_TYPE m_type;
  CStdString m_label;

  //Boxee
  CStdString m_strPath;
  bool m_loading;
  bool m_loaded;
  int m_savedSelected;
  //end Boxee

  bool m_hasStaticContent;
  bool m_hasStaticActions;
  std::vector<CGUIActionDescriptor> m_staticActions;
  std::queue<CAction>  m_deferredActions;
  unsigned int m_staticUpdateTime;
  std::vector<CGUIListItemPtr> m_staticItems;
  bool m_wasReset;  // true if we've received a Reset message until we've rendered once.  Allows
                    // us to make sure we don't tell the infomanager that we've been moving when
                    // the "movement" was simply due to the list being repopulated (thus cursor position
                    // changing around)

  void UpdateScrollByLetter();
  void GetCacheOffsets(int &cacheBefore, int &cacheAfter);
  bool ScrollingDown() const { return m_scrollSpeed > 0; };
  bool ScrollingUp() const { return m_scrollSpeed < 0; };
  void OnNextLetter();
  void OnPrevLetter();
  void OnJumpLetter(char letter);
  void OnJumpSMS(int letter);
  std::vector< std::pair<int, CStdString> > m_letterOffsets;
  
  ListSelectionMode m_selectionMode;
  
  bool m_firstRenderAfterLoad;
  
private:
  int m_cacheItems;
  float m_scrollSpeed;
  CStopWatch m_scrollTimer;
  CStopWatch m_pageChangeTimer;

  // letter match searching
  CStopWatch m_matchTimer;
  CStdString m_match;

  static const int letter_match_timeout = 1000;
  
  float m_focusedPosX;
  float m_focusedPosY;
  
  int m_autoScrollTime;
  CStopWatch m_autoScrollTimer;
  
  bool m_clickAnimationStarted;
  CAction m_clickAction;  
  
  CCriticalSection m_itemsLock;
};


