#include "../linux/PlatformDefs.h"
#include "../utils/log.h"
#include "WidgetEngine.h"
#include "WidgetParser.h"
#include "Widget.h"

#include "MonkeyEngine.h"

WidgetEngine::WidgetEngine()
{
}

WidgetEngine::~WidgetEngine()
{
}

bool WidgetEngine::LoadWidget(const string& widgetFilePath) {
  WidgetParser parser;
  //  
  m_pWidget = parser.createWidget(widgetFilePath);
  
  if (m_pWidget == NULL) {
    cerr << "ERROR: Could not create widget" << endl;
    return false;
  }
  
  return true;
}

bool WidgetEngine::Init() {
  bool initOK = true;
  
  initOK &= InitializeSpiderMonkey(); 
  
  return initOK;
}

bool WidgetEngine::Run() {
  return true;
}

bool WidgetEngine::InitializeSpiderMonkey() 
{
  CLog::Log(LOGDEBUG, "WidgetEngine: Initializing the JavaScript Engine");
  m_pJSEngine = new MonkeyEngine();
  m_pJSEngine->init();
}


