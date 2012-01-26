#ifndef XAPP_LIST_H_
#define XAPP_LIST_H_

#include "XAPP_Control.h"
#include "XAPP_ListItem.h"

namespace XAPP
{

/**
 * Represents a list control in the user interface. Get the List object by calling GetList() on the Window.
 */
class List : public Control
{
public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS  
  List(int windowId, int controlId) throw (XAPP::AppException);
#endif
  
  /**
   * Scrolls the list one page up for vertical lists or left for horizontal lists.
   */
  void ScrollPageUp();
  
  /**
   * Scrolls the list one page down for vertical lists or right for horizontal lists.
   */
  void ScrollPageDown();
  
  /**
   * Refreshes the contents of the list.
   */
  void Refresh();
  
  /**
   * Sets the list with items from an RSS feed. Please see the RSS specification for supported RSS formats.
   * 
   * @param url url of the RSS. Should have and rss:// scheme.
   */
  void SetContentURL(const std::string& url);
  
  /**
   * Focuses a specific item in the list. Only one item can be focused in the list.
   * 
   * @param item index of item in the list that should be focused
   */
  void SetFocusedItem(int item);
  
  /**
   * Selects/unselects an item in the list. Single/multiple selection is defined in the skin.
   * 
   * @param item index of item in the list that should be selected/unselected
   * @param selected true to select the item, false to unselect
   */
  void SetSelected(int item, bool selected);
  
  /**
   * Selects all the items in the list.
   */
  void SelectAll();
  
  /**
   * Unselect all the items in the list.
   */
  void UnselectAll();
  
  /**
   * Returns all the selected items in the list.
   */
  ListItems GetSelected();
  
  /**
   * Returns the index of the focused item in the list. Throws an exception if no item is focused.
   */
  int GetFocusedItem() throw (XAPP::AppException);
  
  /**
   * Returns true if the item with specified index
   * @param itemIndex - index of the item
   */
  bool IsSelected(int itemIndex);

  /**
   * Returns a specific item in the list. Throws an exception if the item does not exist.
   */
  ListItem GetItem(int item) throw (XAPP::AppException);
  
  /**
   * Returns all the items in the list.
   */
  ListItems GetItems();
  
  /**
   * Loads the list with items.
   * 
   * @param list items to be set in the list. 
   */
  void SetItems(ListItems list, int iSelectedItem = -1);
  
  /**
   * Moves the focus in the list to the first item with the specified letter.
   * 
   * @param letter the letter to jump to. 
   */
  void JumpToLetter(char letter);

  /**
   * Updates an existing item.
   *
   * @param item to be set in the list.
   */
  void UpdateItem(int index, ListItem item);
};

}

#endif /* LIST_H_ */
