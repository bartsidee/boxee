/*
 * XAPP_Textbox.h
 *
 *  Created on: Aug 4, 2011
 *      Author: shay
 */

#ifndef XAPP_TEXTBOX_H_
#define XAPP_TEXTBOX_H_
#include "XAPP_Control.h"

namespace XAPP
{

class Textbox: public Control
{
public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  Textbox(int windowId, int controlId) throw (XAPP::AppException);
#endif

  void SetText(const std::string& text);
};

}
#endif /* XAPP_TEXTBOX_H_ */
