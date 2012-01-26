#ifndef XBJAVASCRIPT_H
#define XBJAVASCRIPT_H

#include "js/jsapi.h"
#include "js/jsstr.h"

class XBJavaScript
{
public:
  XBJavaScript();
  virtual ~XBJavaScript();
  bool Init();
  bool DeInit();
  bool evalString(std::string script);
  
protected:
  static void JSReportError(JSContext *cx, const char *message, JSErrorReport *report);

  JSRuntime *m_jsRuntime;
  JSContext *m_jsMainJSCtx;  
  JSObject  *m_jsGlobalObj;  
};


#endif /* XBJAVASCRIPT_H */
