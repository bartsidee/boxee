#ifndef XAPP_APP_H_
#define XAPP_APP_H_

#include <string>
#include <map>
#include "XAPP_LocalConfig.h"
#include "XAPP_ListItem.h"

namespace XAPP
{

/**
 * This class represents parameters that are passed to application launch. Should be used as a dictionary.
 */
class Parameters : public std::map<std::string, std::string>
{
public:
  /**
   * Converts the parameters in a URL query to a string: key=value&key2=value.
   */
  const std::string toQueryString() const;
};

/**
 * Main class for working with an application. Includes launching of applications, accessing the application
 * configuration parameters storage, etc. Get the App object by calling GetApp() function.
 */
class App
{
public:
  /**
   * Returns the local configuration storage for an application.
   */
  static LocalConfig& GetLocalConfig();
  
  /**
   * Activate a window of the application with parameters.
   * 
   * @param windowId window id
   * @param parameters parameters that will be passed to the application 
   */
  static void ActivateWindow(int windowId, const Parameters& parameters);
  
  /**
   * Closes the application and stops application thread
   */
  static void Close();

  /**
   * Returns the parameters of the activated application window using ActivateWindow.
   */
  static XAPP::Parameters GetLaunchedWindowParameters();
  
  /**
   * Returns the list item that the application was launched with. this is mainly useful to get an item from the history/recommendation/rating.
   */
  static XAPP::ListItem GetLaunchedListItem();  
  
  /**
   * Run a python script of an application.
   * 
   * @param scriptName script name
   * @param parameters parameters that will be passed to the application script
   */
  static void RunScript(const std::string& scriptName, const Parameters& parameters);

  /**
   * Send a message to an application
   * The mesage will be passed to a global handler
   *
   * @param handler allows to specify which handler should eventually handle the message
   * @param parameter additional string parameter
   */
  static void SendMessage(const std::string& handler, const std::string& parameter);

  /**
   * Returns the parameters of the executed script using RunScript. You can also access
   * those parameters with sys.arv[1].
   */
  static XAPP::Parameters GetLaunchedScriptParameters();  
  
  /**
   * Returns the id of the currently running application.
   */
  static std::string GetId();
  
  /**
   * Returns the full path where the application is installed.
   */
  static std::string GetAppDir();
  
  /**
   * Returns the full path where the media files of the application are stored.
   */
  static std::string GetAppMediaDir();
  
  /**
   * Returns the full path where the skin files of the application are stored.
   */
  static std::string GetAppSkinDir();  
  
  /**
   * Returns an authentication token for the application. For boxee internal use.
   */
  static std::string GetAuthenticationToken();  
  
private:
  static LocalConfig s_localConfig;
};

}

#endif /* APP_H_ */

