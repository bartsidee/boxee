#ifndef MC_H_
#define MC_H_

#include <string>
#include <map>
#include <vector>
#include "XAPP_Window.h"
#include "XAPP_Player.h"
#include "XAPP_App.h"

namespace XAPP
{

/**
 * Main class for interfacing with BOXEE.
 */
class MC
{
public:
  /**
   * Returns a localized string based on an id of the string.
   * 
   * @param id id of the string. could be either a system wide id or an application specific id.
   */
  static std::string GetLocalizedString(int id);
  
  /**
   * Returns information about the user interface. See separate documentation regarding information 
   * that can be retrieved. 
   * 
   * @param info a string representing the information to be retrieved.
   */
  static std::string GetInfoString(const std::string& info);
  
  /**
   * Displays a notification at the upper right corner of the screen for 5 seconds.
   * 
   * @param msg mesage to be notified to the user.
   * @param thumbnail file name that contains the image to be displayed. Optional.
   */
  static void ShowDialogNotification(const std::string& msg, const std::string& thumbnail = "");
  
  /**
   * Displays a wait dialog. This should be displayed during long operations.
   */
  static void ShowDialogWait();
  
  /**
   * Hides the wait dialog.
   */
  static void HideDialogWait();
  
  /**
   * Displays a confirmation dialog, such as Ok/Canel or Yes/No. Returns true if the confirm button was clicked 
   * or false if the cancel button was pressed or if the dialog was closed.
   * 
   * @param heading heading for the dialog
   * @param body contents of the dialog. use [CR] for line breaks.
   * @param cancelButton text to appear in the cancel button. Default is "Cancel". Optional.
   * @param confirmButton text to appear in the confirm button. Default is "Ok". Optional.
   */
  static bool ShowDialogConfirm(const std::string& heading, const std::string& body, const std::string& cancelButton, const std::string& confirmButton);
  
  /**
   * Displays an "Ok" dialog for displaying information to the user.
   * 
   * @param heading heading of the dialog
   * @param body contents of the dialog. use [CR] for line breaks. 
   */
  static void ShowDialogOk(const std::string& heading, const std::string& body);
  
  /**
   * Displays a keyboard dialog for text input. Returns true if a value was entered or false if the dialog was cancelled.
   * 
   * @param heading heading of the dialog
   * @param defaultValue value to be pre-populated in the dialog when displayed. if the dialog was closed with "Ok" it also 
   *              contains the value that was types.
   * @param hiddenInput false - the typed value is displayed, true - the typed value is hidden and * are shown instead.
   */
  static std::string ShowDialogKeyboard(const std::string& heading, const std::string& defaultValue, bool hiddenInput);
  
  /**
   * Displays a selection dialog between several string options
   * @param heading heading of the dialog
   * @param choices choices to select from
   */
  static int ShowDialogSelect(const std::string& heading, const std::vector<std::string>& choices);

  /**
   * Log debug message into the Boxee log file.
   * 
   * @param msg message to be written to the log file
   */
  static void LogDebug(const std::string& msg);

  /**
   * Log information message into the Boxee log file.
   * 
   * @param msg message to be written to the log file
   */  
  static void LogInfo(const std::string& msg);

  /**
   * Log error message into the Boxee log file.
   * 
   * @param msg message to be written to the log file
   */    
  static void LogError(const std::string& msg);
  
  /**
   * Get a reference to the currently active window.   
   */
  static Window GetActiveWindow();
  
  /**
   * Get a reference to the window by its id.
   *    
   * @param id the id of the window
   */  
  static Window GetWindow(int id) throw (XAPP::AppException);
  
  /**
   * Activate a specific window by its id.
   *    
   * @param id the id of the window
   */   
  static void ActivateWindow(int id);
  
  /**
   * Closes the currently active window and go to the previously open window.
   */   
  static void CloseWindow();
  
  /**
   * Returns a reference to a media player that can be used for playing media.
   */
  static XAPP::Player GetPlayer();
  
  /**
   * Returns the full path of a directory where temporary files can be placed.
   */
  static std::string GetTempDir();
  
  /**
   * Returns a reference to the App object, that should be used for application specific operations.
   */
  static XAPP::App& GetApp();
  
  /**
   * Returns the cookie jar used by boxee
   */
  static std::string GetCookieJar();

  /**
   * Returns contents of specified path
   * @param strPath - path
   */
  static ListItems GetDirectory(const std::string& strPath);

  /**
   * Returns focused item from the list
   * @param windowId - window id
   * @param listId - id of the list
   */
  static ListItem GetFocusedItem(int windowId, int listId);

  /**
   * Set the list of item to specified list
   * @param windowId - window id
   * @param controlId - control id
   * @param items - item list
   * @param selectedItem - selected item
   */
  static void SetItems(int windowId, int controlId, ListItems& items, int selectedItem);

  /**
   * Returns geo location of the current user
   */
  static std::string GetGeoLocation();

  /**
    * Returns device id
    */
   static std::string GetDeviceId();

   /**
    * Returns platform id
    */
   static std::string GetPlatform();

   /**
    * Returns true if running on embedded platform and false otherwise
    */
   static bool IsEmbedded();

  /**
   * Returns current position of the played video in seconds
   * @param strPath - path of the video
   */
  static int GetCurrentPositionInSec(const std::string& strPath);

   /**
    * Return the current weather setting location. For example: "USNY0996 - New York, NY"
    */
   static std::string GetWeatherLocation();

   /**
    * Return the current weather setting location. For example: "USNY0996 - New York, NY"
    */
   static void SetWeatherLocation(std::string location);

   /**
    * Set the current weather setting location.
    * @param cityName - the city name
    * @param countryCode - the city country code
    * Returns true on success and false on failure
    */
   static bool SetWeatherLocation2(std::string cityName, std::string countryCode);

   /**
    * Return the current timezone country setting location
    */
   static std::string GetTimezoneCountry();
  
   /**
    * Return the current timezone city setting location
    */
   static std::string GetTimezoneCity();

   /**
    * Return the current temperature scale. Either 'C' or 'F'.
    */
   static std::string GetTemperatureScale();

   /**
    * Set the temperature scale. Input should be either 'C' or 'F'.
    */
   static void SetTemperatureScale(std::string scale);

   /**
    * Returns true if has internet connection
    */
   static bool IsConnectedToInternet();

   /**
    * Returns unique box id
    */
   static std::string GetUniqueId();

   /**
    * Returns the system language
    */
   static std::string GetSystemLanguage();

   /**
    * Returns box vendor name
    */
   static std::string GetHardwareVendor();

   /**
    * Returns box model name
    */
   static std::string GetHardwareModel();

   /**
    * Returns box revision number
    */
   static std::string GetHardwareRevision();

   /**
    * Returns box revision number
    */
   static std::string GetHardwareSerialNumber();

private:
  static std::map<std::string, int> m_translatedInfo;  
  static App s_app;
};

}

#endif
