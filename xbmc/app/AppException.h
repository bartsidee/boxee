#ifndef APPEXCEPTION_H_
#define APPEXCEPTION_H_

#include <string>

namespace XAPP
{

/**
 * Exception thrown by the app API.
 */
class AppException
{
public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS    
  AppException(const std::string& msg) { m_msg = msg; }
  virtual ~AppException() {}
#endif
  
  /**
   * Get the message associated with the exception.
   */
  const std::string& getMessage() const { return m_msg; }
  
private:
  std::string m_msg;
};

}

#endif /* APPEXCEPTION_H_ */
