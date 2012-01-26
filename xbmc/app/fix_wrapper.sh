#/bin/sh
cat $1 | sed '
/#include <Python.h>/  a\
#if (defined HAVE_CONFIG_H) && (!defined WIN32)\
  #include "config.h"\
#endif\
#if (defined USE_EXTERNAL_PYTHON)\
  #if (defined HAVE_LIBPYTHON2_6)\
    #include <python2.6/Python.h>\
    #include <python2.6/osdefs.h>\
  #elif (defined HAVE_LIBPYTHON2_5)\
    #include <python2.5/Python.h>\
    #include <python2.5/osdefs.h>\
  #elif (defined HAVE_LIBPYTHON2_4)\
    #include <python2.4/Python.h>\
    #include <python2.4/osdefs.h>\
  #else\
    #error "Could not determine version of Python to use."\
  #endif\
#else\
#include "lib/libPython/Python/Include/Python.h"\
#include "lib/libPython/Python/Include/osdefs.h"\
#endif\
#include "lib/libPython/XBPythonDll.h"
' | sed '/return swig_this;/ {
c\
return _SWIG_This();
}' | sed '/#define SWIGPYTHON/ {
a\
#include "system.h"
}' | sed 's/#include <Python.h>//' > $2
