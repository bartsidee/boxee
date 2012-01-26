%module mc
%feature("docstring");
%feature("autodoc", "1");
%include "mc_doc.i"

%include "exception.i"

%{
#include "XAPP_App.h"
#include "XAPP_Button.h"
#include "XAPP_Control.h"
#include "XAPP_Edit.h"
#include "XAPP_Image.h"
#include "XAPP_Label.h"
#include "XAPP_Textbox.h"
#include "XAPP_List.h"
#include "XAPP_ListItem.h"
#include "XAPP_LocalConfig.h"
#include "XAPP_ToggleButton.h"
#include "XAPP_MC.h"
#include "XAPP_Player.h"
#include "XAPP_PlayList.h"
#include "XAPP_Window.h"
#include "XAPP_Http.h"
#include "AppException.h"
%}

%include "std_map.i"
%include "std_vector.i"
%include "std_string.i"

namespace std
{
  %template(stdParameters) map<std::string, std::string>;
  %template(stdListItems) vector<XAPP::ListItem>;
}

%{
/* This function matches the prototype of the normal C callback
   function for our widget. However, we use the clientdata pointer
   for holding a reference to a Python callable object. */

static void PlayerCallBack(int event, void *clientdata)
{
   PyObject *func, *arglist;
   PyObject *result;
   
   func = (PyObject *) clientdata;               // Get Python function
   arglist = Py_BuildValue("(i)",event);         // Build argument list
   result = PyEval_CallObject(func,arglist);     // Call Python
   Py_DECREF(arglist);                           // Trash arglist
   Py_XDECREF(result);
}
%}

%include "XAPP_ListItem.h"
%include "XAPP_Control.h"
%include "XAPP_Button.h"
%include "XAPP_Edit.h"
%include "XAPP_Image.h"
%include "XAPP_Label.h"
%include "XAPP_Textbox.h"
%include "XAPP_List.h"
%include "XAPP_LocalConfig.h"
%include "XAPP_ToggleButton.h"
%include "XAPP_Window.h"
%include "XAPP_Http.h"
%include "XAPP_MC.h"
%include "XAPP_Player.h"
%include "XAPP_PlayList.h"
%include "XAPP_App.h"

/*
 * When a c++ exception occurs, this code will turn it into
 * a python exception. Cool.
 */
%exception 
{
  try 
  {
    $action
  } 
  catch(XAPP::AppException& e) {
      const char* sData = (char*)e.getMessage().c_str();
      SWIG_exception(SWIG_RuntimeError,sData);
  } 
  catch(...) {
      SWIG_exception(SWIG_RuntimeError,"Unknown exception");
  }  
}

/*
 * The function GetRxPacket returns a new object that python
 * memory management must delete when required.
 * %newobject GetRxPacket;
 */

// Grab a Python function object as a Python object.
%typemap(in) PyObject *pyfunc {
  if (!PyCallable_Check($input)) {
      PyErr_SetString(PyExc_TypeError, "Need a callable object!");
      return NULL;
  }
  $1 = $input;
}

/*
// Attach a new method to our plot widget for adding Python functions
%extend XAPP::Player {
   // Set a Python function object as a callback function
   // Note : PyObject *pyfunc is remapped with a typempap
   void SetCallback(PyObject *pyfunc) {
     self->set_method(PlayerCallBack, (void *) pyfunc);
     Py_INCREF(pyfunc);
   }
}
*/
%pythoncode %{
  
def GetLocalizedString(id):
  return MC.GetLocalizedString(id)

def GetInfoString(info):
  return MC.GetInfoString(info)

def ShowDialogNotification(msg, thumbnail=""):
  MC.ShowDialogNotification(msg, thumbnail)

def ShowDialogWait():
  MC.ShowDialogWait()

def HideDialogWait():
  MC.HideDialogWait()

def ShowDialogConfirm(heading, body, cancelButton, confirmButton):
  return MC.ShowDialogConfirm(heading, body, cancelButton, confirmButton)

def ShowDialogOk(heading, body):
  MC.ShowDialogOk(heading, body)
  
def ShowDialogKeyboard(heading, value, hiddenInput):
  return MC.ShowDialogKeyboard(heading, value, hiddenInput);
  
def ShowDialogSelect(heading, choices):
  return MC.ShowDialogSelect(heading, choices)
  
def ShowCustomDialog(id):
  return MC.ShowCustomDialog(id)

def CloseActiveDialogs():
  return MC.CloseActiveDialogs()
  
def LogDebug(msg):
  MC.LogDebug(msg)

def LogInfo(msg):
  MC.LogInfo(msg)

def LogError(msg):
  MC.LogError(msg)

def GetActiveWindow():
  return MC.GetActiveWindow()

def CloseWindow():
	MC.CloseWindow()

def GetWindow(id):
  return MC.GetWindow(id)

def ActivateWindow(id):
  MC.ActivateWindow(id)
  
def GetPlayer():
  return MC.GetPlayer()
  
def GetTempDir():
  return MC.GetTempDir()
  
def GetApp():
  return MC.GetApp()    

def GetCookieJar():
  return MC.GetCookieJar()
  
def GetDirectory(strPath):
  return MC.GetDirectory(strPath)
  
def GetFocusedItem(windowId, listId):
  return MC.GetFocusedItem(windowId, listId)    
  
def GetGeoLocation() :
  return MC.GetGeoLocation()
  
def GetDeviceId() :
  return MC.GetDeviceId()
  
def GetPlatform() :
  return MC.GetPlatform()
  
def IsEmbedded():
  return MC.IsEmbedded()
  
def GetCurrentPositionInSec(strPath):
  return MC.GetCurrentPositionInSec(strPath)

def GetTimezoneCity():
  return MC.GetTimezoneCity()

def GetTimezoneCounty():
  return MC.GetTimezoneCounty()

def GetWeatherLocation():
  return MC.GetWeatherLocation()

def SetWeatherLocation(location):
  return MC.SetWeatherLocation(location)

def SetWeatherLocation2(cityName, countryCode):
  return MC.SetWeatherLocation2(cityName, countryCode)

def GetTemperatureScale():
  return MC.GetTemperatureScale()

def SetTemperatureScale(scale):
  return MC.SetTemperatureScale(scale)

def IsConnectedToInternet():
  return MC.IsConnectedToInternet()

def GetUniqueId():
  return MC.GetUniqueId()

def GetSystemLanguage():
  return MC.GetSystemLanguage()

def GetHardwareVendor():
  return MC.GetHardwareVendor()
 
def GetHardwareModel():
  return MC.GetHardwareModel()
 
def GetHardwareRevision():
  return MC.GetHardwareRevision()
 
def GetHardwareSerialNumber():
  return MC.GetHardwareSerialNumber()

%}

