#ifndef XAPP_TOGGLE_BUTTON_H
#define XAPP_TOGGLE_BUTTON_H

#include <string>
#include "XAPP_Control.h"

namespace XAPP
{

/**
 * Represents a toggle button control in the user interface. Get the ToggleButton object by calling GetToggleButton() on the Window.
 */
class ToggleButton : public Control
{
public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS    
  ToggleButton(int windowId, int controlId) throw (XAPP::AppException);
#endif
  
  /**
   * Set the label of the button
   * 
   * @param label the label of the button
   */  
  void SetLabel(const std::string& label);
  
  /**
   * Mark the toggle button as selected/unselected
   * 
   * @param selected true to select the toggle button, false for unselected
   */
  void SetSelected(bool selected);
  
  /**
   * Returns true if the toggle button is selected, false otherwise.
   */
  bool IsSelected();
};

}

#endif /* TOGGLE_BUTTON_H */
