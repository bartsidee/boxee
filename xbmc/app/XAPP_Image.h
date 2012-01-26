#ifndef XAPP_IMAGE_H_
#define XAPP_IMAGE_H_

#include "XAPP_Control.h"

namespace XAPP
{

/**
 * Represents an image control in the user interface. Get the Image object by calling GetImage() on the Window.
 */
class Image : public Control
{
public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS    
  Image(int windowId, int controlId) throw (XAPP::AppException);
#endif
  
  /**
   * Set the texture of the image.
   * 
   * @param imagePath the full path where the image resides. Should be a local file only. Supported formats: PNG, GIF, JPG.
   */
  void SetTexture(const std::string& imagePath);
};

}

#endif /* IMAGE_H_ */
