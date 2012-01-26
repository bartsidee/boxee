#ifndef XAPP_BUTTON_H_
#define XAPP_BUTTON_H_

#include <string>
#include "XAPP_Control.h"

namespace XAPP
{

/**
 * Represents a button control in the user interface. Get the Button object by calling GetButton() on the Window.
 */
class Button : public Control
{
public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS    
  Button(int windowId, int controlId) throw (XAPP::AppException);
#endif
  
  /**
   * Set the label of the button
   * 
   * @param label the label of the button
   */
  virtual void SetLabel(const std::string& label);

  /**
   * Get button label
   *
   * @return the label of the button
   */
  std::string GetLabel();
};

}

#endif /* BUTTON_H_ */
