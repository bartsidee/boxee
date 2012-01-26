#ifndef XAPP_NATIVE_H_
#define XAPP_NATIVE_H_

#include <string>
#include <map>

namespace XAPP
{

class NativeApp
{
public:
  NativeApp(std::string name);
  virtual ~NativeApp();
  virtual bool Start(int argc, char** argv) = 0;
  virtual void Process() = 0;
  virtual bool Call(std::string function, std::map<std::string, std::string> args) = 0;
};

}

#endif
