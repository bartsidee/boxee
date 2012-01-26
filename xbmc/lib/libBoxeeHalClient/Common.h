#ifndef BOXEEHALCONSTANTS_H
#define BOXEEHALCONSTANTS_H

#include <stdint.h>
#include <map>
#include <string>
#include <vector>

static const char NOTIFICATION_REGISTRATION_RESPONSE[] = {
  "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\nServer: BOXEE HAL/0.4\r\n\r\n"
};

#define BOXEE_HAL_PORT  5700

class StringMap : public std::map< std::string, std::string >
{
public:
  StringMap() : std::map< std::string, std::string >() { }
  void AddIntParameter( const char* name, int value);
  void AddBoolParameter( const char* name, bool value);
};

typedef std::vector< StringMap > StringMapVector;
  
enum ERROR_CODE
{
  NO_ERROR,
  MALFORMED_REQUEST,
  GENERAL_ERROR
};

#endif
