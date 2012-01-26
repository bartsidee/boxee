#ifndef WIDGETENGINE_H_
#define WIDGETENGINE_H_

#include <string>

using namespace std;


// Forward declarations
class Widget;
class MonkeyEngine;

/**
 * The key widget engine class
 */
class WidgetEngine
{
  
private :
  
  Widget * m_pWidget;
  
  MonkeyEngine* m_pJSEngine;
  
public:
  WidgetEngine();
  virtual ~WidgetEngine();
  
  /**
   * Loads and creates the widget from the *.kon file
   * Widget resources are assumed to be at the same path under
   * the Resources directory
   * 
   */
  bool LoadWidget(const string& widgetFilePath);
  
  /**
   * Initializes the required engines: the JSEngine and the VideoEngine
   */
  bool Init();
  bool InitializeSpiderMonkey();  
  /**
   * Runs the widget including opening a graphical window and 
   * rendering the widget
   */
  bool Run();
};

#endif /*WIDGETENGINE_H_*/

