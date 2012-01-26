#ifndef XAPP_CONTROL_H_
#define XAPP_CONTROL_H_

#include "AppException.h"

namespace XAPP
{

class ActionEvent
{
public:
  int windowId;
  int controlId;
};

class ActionListener
{
public:
  virtual ~ActionListener();
  virtual bool ActionPerformed(ActionEvent e) = 0;
};

/**
 * Represents a control in the user interface. Get the Control object by calling GetControl() on the Window.
 */

class Control
{
public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS    
  Control(int windowId, int controlId) throw (XAPP::AppException);
  virtual ~Control() {}
#endif
  
  /**
   * Requests a focus on the control
   */
  virtual void SetFocus();
  
  /**
   * Returns true if the control holds the focus
   */
  virtual bool HasFocus();
  
  /**
   * Changes the visibility of the control.
   * 
   * @param visible true to make the control visible. false otherwise.
   */
  virtual void SetVisible(bool visible);
  
  /**
   * Returns true if the control is visible.
   */
  virtual bool IsVisible();
  
  /**
   * Sets whether the control is enabled. Controls which are not enabled cannot be focused.
   * 
   * @param enabled true to make the control enabled. false otherwise.
   */
  virtual void SetEnabled(bool enabled);
  
  /**
   * Returns true if the control is enabled.
   */
  virtual bool IsEnabled();
  
  /**
   * Get window id
   */
  int GetWindowId();

  /**
   * Get window id
   */
  int GetControlId();

#ifndef DOXYGEN_SHOULD_SKIP_THIS  
  void AddActionListener(ActionListener* listener);
  void RemoveActionListener(ActionListener* listener);

protected:
  int m_windowId;
  int m_controlId;
#endif
};

}

#endif /* CONTROL_H_ */
