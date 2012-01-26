#ifndef XAPP_EDIT_H_
#define XAPP_EDIT_H_

#include <string>
#include "XAPP_Control.h"

namespace XAPP
{

/**
 * Represents an edit control in the user interface. Get the Edit object by calling GetEdit() on the Window.
 */
class Edit : public Control
{
public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS    
  Edit(int windowId, int controlId) throw (XAPP::AppException);
#endif
  
  /**
   * Sets the text of the edit control
   * 
   * @param text the text of the edit control
   */
  virtual void SetText(const std::string& text);

  /**
   * Gets the text of the edit control
   */
  virtual std::string GetText();
};

}

#endif /* EDIT_H_ */
