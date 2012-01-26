#ifndef XAPP_LABEL_H_
#define XAPP_LABEL_H_

#include "XAPP_Control.h"

namespace XAPP
{

/**
 * Represents a label control in the user interface. Get the Label object by calling GetLabel() on the Window.
 */
class Label : public Control
{
public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS    
  Label(int windowId, int controlId) throw (XAPP::AppException);
#endif
  
  /**
   * Set the label text
   * 
   * @param label the text of the label
   */  
  void SetLabel(const std::string& label);

  /**
   * Returns the value of the label
   */
  virtual std::string GetLabel();
};

}

#endif /* LABEL_H_ */
