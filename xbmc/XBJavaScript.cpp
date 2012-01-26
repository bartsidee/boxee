/*
 * AppJS.cpp
 *
 *  Created on: Jan 26, 2009
 *      Author: yuvalt
 */
#include <string>
#include "XBJavaScript.h"
#include "stdafx.h"
#include "utils/log.h"
#include "app/App.h"
#include "app/Button.h"
#include "app/Control.h"
#include "app/Image.h"
#include "app/Label.h"
#include "app/List.h"
#include "app/LocalConfig.h"
#include "app/PlayList.h"
#include "app/ServerConfig.h"
#include "app/ToggleButton.h"
#include "app/Window.h"

JSClass js_global_object_class = {
  "System",
  0,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_EnumerateStub,
  JS_ResolveStub,
  JS_ConvertStub,
  JS_FinalizeStub,
  JSCLASS_NO_OPTIONAL_MEMBERS
};

XBJavaScript::XBJavaScript()
{
  Init();
}

bool XBJavaScript::Init()
{
  m_jsRuntime = NULL;
  m_jsMainJSCtx = NULL;  
  m_jsGlobalObj = NULL;  
  
  /* Create a JS runtime. */
  m_jsRuntime = JS_NewRuntime(10L * 1024L * 1024L);
  if (m_jsRuntime == NULL)
  {
    printf("failed to created JS runtime");
    DeInit();
    return false;
  }  
    
  /* Create a context. */
  m_jsMainJSCtx = JS_NewContext(m_jsRuntime, 8192);
  if (m_jsMainJSCtx == NULL)
  {
    printf("failed to created JS context");
    DeInit();
    return false;
  }  
  
  JS_SetOptions(m_jsMainJSCtx, JSOPTION_VAROBJFIX);
  JS_SetVersion(m_jsMainJSCtx, JSVERSION_1_7);
  
  JS_SetErrorReporter(m_jsMainJSCtx, XBJavaScript::JSReportError);
  
  /* Create the global object. */
  m_jsGlobalObj = JS_NewObject(m_jsMainJSCtx, &js_global_object_class, NULL, NULL);
  if (m_jsGlobalObj == NULL)
  {
    printf("failed to created JS object");
    DeInit();
    return false;
  }  
  
  /* Populate the global object with the standard globals,
   like Object and Array. */
  if (!JS_InitStandardClasses(m_jsMainJSCtx, m_jsGlobalObj))
  {
    printf("failed to Init JS object");
    DeInit();
    return false;
  }  
  
  XAPP::App::AppJSInit(m_jsMainJSCtx, m_jsGlobalObj);
  XAPP::Window::WindowJSInit(m_jsMainJSCtx, m_jsGlobalObj);
  XAPP::Control::ControlJSInit(m_jsMainJSCtx, m_jsGlobalObj);
  XAPP::Button::ButtonJSInit(m_jsMainJSCtx, m_jsGlobalObj);
  XAPP::Image::ImageJSInit(m_jsMainJSCtx, m_jsGlobalObj);
  XAPP::Label::LabelJSInit(m_jsMainJSCtx, m_jsGlobalObj);
  XAPP::List::ListJSInit(m_jsMainJSCtx, m_jsGlobalObj);
  XAPP::LocalConfig::LocalConfigJSInit(m_jsMainJSCtx, m_jsGlobalObj);
  XAPP::PlayList::PlayListJSInit(m_jsMainJSCtx, m_jsGlobalObj);
  XAPP::ServerConfig::ServerConfigJSInit(m_jsMainJSCtx, m_jsGlobalObj);
  XAPP::ToggleButton::ToggleButtonJSInit(m_jsMainJSCtx, m_jsGlobalObj);
  
  return true;
}

bool XBJavaScript::DeInit()
{
  if (m_jsMainJSCtx)
  {
#if !defined(_WINDOWS) 
    JS_ClearContextThread(m_jsMainJSCtx);
#endif
    JS_DestroyContext(m_jsMainJSCtx);
  }
  m_jsMainJSCtx = NULL;
  
  if (m_jsRuntime)
  {
    JS_DestroyRuntime(m_jsRuntime);
  }

  m_jsRuntime = NULL;
  
  return true;
}

XBJavaScript::~XBJavaScript()
{
  DeInit();
}

bool XBJavaScript::evalString(std::string script)
{
  jsval rval; 
  uintN lineno = 0; 
  JSBool ok = JS_EvaluateScript(m_jsMainJSCtx, m_jsGlobalObj, script.c_str(), script.length(), "script", lineno, &rval);
  if (ok == JS_FALSE)
  {
    CLog::Log(LOGERROR, "Error running JavaScript code. Offending line: %d", lineno); 
    printf("Error JS:: %s\n", lineno);     
    return false;
  }
  
  return true;
}

void XBJavaScript::JSReportError(JSContext *cx, const char *message, JSErrorReport *report)
{
  CLog::Log(LOGERROR, "JS: %d: %s", report->lineno, message); 
  printf("JS: %d: %s\n", report->lineno, message); 
}

/*
int main()
{
  XBJavaScript appjs;
  appjs.evalString("App.GetButton(15).SetLabel(\"hello\");");
  //appjs.evalString("App.GetLocalizedString(15);");
}
*/
