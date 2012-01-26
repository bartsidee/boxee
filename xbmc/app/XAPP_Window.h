#ifndef XAPP_WINDOW_H
#define XAPP_WINDOW_H


#include <map>
#include "XAPP_Label.h"
#include "XAPP_List.h"
#include "XAPP_Control.h"
#include "XAPP_Button.h"
#include "XAPP_ToggleButton.h"
#include "XAPP_Image.h"
#include "XAPP_Edit.h"
#include "GUIWindow.h"
#include "AppException.h"

namespace XAPP
{

/**
 * Represents a toggle button control in the user interface. Get the Window object by calling GetActiveWindow() 
 * or GetWindow() functions.
 */
class Window
{
public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS    
  Window(int id) throw (XAPP::AppException);
#endif
  
  /**
   * Returns a control in the window
   * 
   * @param id the id of the control
   */
  Control GetControl(int id) throw (XAPP::AppException);
  
  /**
   * Returns a label control in the window
   */
  Label GetLabel(int id) throw (XAPP::AppException);

  /**
   * Returns a list control in the window
   */  
  List GetList(int id) throw (XAPP::AppException);

  /**
   * Returns a toggle button control in the window
   */    
  ToggleButton GetToggleButton(int id) throw (XAPP::AppException);

  /**
   * Returns a button control in the window
   */      
  Button GetButton(int id) throw (XAPP::AppException);
  
  /**
   * Returns an image control in the window
   */    
  Image GetImage(int id) throw (XAPP::AppException);

  /**
   * Returns an edit control in the window
   */    
  Edit GetEdit(int id) throw (XAPP::AppException);
  
  /**
   * Saves the state of the window and pushes it to the top of the window state stack. By default, if
   * user hits ESC or Back, instead of closing the window, it will pop the state and return it.
   * The state includes contents of lists and the selected items in lists. This is useful if you want 
   * to support "drill down" navigation in a window.
   */
  void PushState();
  
  /**
   * Pops a state from the state stack (see PushState()) and sets the user interface accordingly.
   * 
   * @param restoreState true to restore the top most state in the stack
   */
  void PopState(bool restoreState = true);

  /**
   * Pops states from the state stack (see PushState()) and leaves "count" states in the stack. 
   * 
   * @param remainInStack number of items to keep remained in the stack
   */  
  void PopToState(int remainInStack);
  
  /**
   * Clears all the saved states.
   * 
   * @param restoreState restore the state of the bottom most state in the stack?
   */  
  void ClearStateStack(bool restoreState = true);  
  
private:
  int m_id;
  CGUIWindow* m_window;
};

}

#endif /* XAPP_WINDOW_H */
