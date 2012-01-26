
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIWindowManager.h"
#include "GUIDialogProgress.h"
#include "GUIDialogYesNo2.h"
#include "GUIDialogOK2.h"
#include "GUIDialogSelect.h"
#include "GUIInfoManager.h"
#include "Application.h"
#include "XAPP_MC.h"
#include "GUIDialogKeyboard.h"
#include "bxcurl.h"
#include "SpecialProtocol.h"
#include "StringUtils.h"
#include "AppManager.h"
#include "FileSystem/Directory.h"
#include "ItemLoader.h"
#include "VideoDatabase.h"
#include "HalServices.h"
#include "Util.h"
#include "GUISettings.h"
#include "utils/Weather.h"
#include "BoxeeUtils.h"
#include "ThreadPolicy.h"
#include <openssl/md5.h>
#ifdef HAS_BOXEE_HAL
#include "../BoxeeHalServices.h"
#endif

namespace XAPP
{

std::string MC::GetLocalizedString(int id)
{
  return g_localizeStrings.Get(id);
}

std::string MC::GetInfoString(const std::string& info)
{
  int translated;
  
  std::map<std::string, int>::iterator it = m_translatedInfo.find(info);
  if (it != m_translatedInfo.end())
  {
    translated = it->second;   
  }
  else
  {
    translated = g_infoManager.TranslateString(info);
  }
  
  return g_infoManager.GetLabel(translated, g_windowManager.GetActiveWindow());
}

void MC::ShowDialogWait()
{
  CGUIDialogProgress* progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (progress) 
  {
    g_application.getApplicationMessenger().Show(progress);
  }
}

void MC::HideDialogWait()
{
  CGUIDialogProgress* progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (progress) 
  {
    g_application.getApplicationMessenger().CloseDialog(progress, true);
  }
}

bool MC::ShowDialogConfirm(const std::string& heading, const std::string& body, const std::string& cancelButtom, const std::string& confirmButton)
{
  return CGUIDialogYesNo2::ShowAndGetInput(heading, body, cancelButtom, confirmButton);
}

void MC::ShowDialogOk(const std::string& heading, const std::string& body)
{
  CGUIDialogOK2::ShowAndGetInput(heading, body);  
}

std::string MC::ShowDialogKeyboard(const std::string& heading, const std::string& defaultValue, bool hiddenInput)
{
  CStdString boxeeValue = defaultValue;
  
  bool result = CGUIDialogKeyboard::ShowAndGetInput(boxeeValue, heading, true, hiddenInput);
  if (result)
  {
    return boxeeValue;
  }
  
  return StringUtils::EmptyString;
}

int MC::ShowDialogSelect(const std::string& heading, const std::vector<std::string>& choices)
{
  CGUIDialogSelect *pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);

  pDlgSelect->SetHeading(heading);
  pDlgSelect->Reset();

  for (int i = 0; i < (int)choices.size(); i++)
  {
    pDlgSelect->Add(choices[i]);
  }

  pDlgSelect->EnableButton(true);
  pDlgSelect->SetButtonLabel(222); //'Cancel' button
  pDlgSelect->DoModal();

  return pDlgSelect->GetSelectedLabel();
}

void MC::LogDebug(const std::string& msg)
{
  CLog::Log(LOGDEBUG, "%s", msg.c_str());
}

void MC::LogInfo(const std::string& msg)
{
  CLog::Log(LOGINFO, "%s", msg.c_str());
}

void MC::LogError(const std::string& msg)
{
  CLog::Log(LOGERROR, "%s", msg.c_str());  
}

Window MC::GetWindow(int id) throw (AppException)
{
  int iWindowId = id;

  const char *strAppId = CAppManager::GetInstance().GetCurrentContextAppId();
  if (strAppId)
  {
    iWindowId = CAppManager::GetInstance().GetWindowId(strAppId, id);
  }

  CLog::Log(LOGDEBUG, "XAPP::MC::GetWindow, requested window id = %d, translated window id = %d (applaunch)", id, iWindowId);
  return Window(iWindowId);
}

Window MC::GetActiveWindow()
{
  return Window(g_windowManager.GetActiveWindow());
}

void MC::ActivateWindow(int id)
{
  int iWindowId = WINDOW_INVALID;

  const char *strAppId = CAppManager::GetInstance().GetCurrentContextAppId();
  if (strAppId)
  {
    iWindowId = CAppManager::GetInstance().GetWindowId(strAppId, id);
  }

  CLog::Log(LOGDEBUG, "XAPP::MC::ActivateWindow, requested window id = %d, translated window id = %d (flow)", id, iWindowId);
  if (iWindowId != WINDOW_INVALID)
  {
    ThreadMessage tMsg (TMSG_GUI_ACTIVATE_WINDOW, iWindowId, 0);
    g_application.getApplicationMessenger().SendMessage(tMsg, false);

  }
}

void MC::CloseWindow()
{
  g_application.getApplicationMessenger().PreviousWindow();
}

void MC::ShowDialogNotification(const std::string& msg, const std::string& thumbnailUrl)
{
  g_application.m_guiDialogKaiToast.QueueNotification(thumbnailUrl, "", msg, 5000);
}

Player MC::GetPlayer()
{
  return Player();
}

std::string MC::GetTempDir()
{
  return CSpecialProtocol::TranslatePath("special://temp/");
}

XAPP::App& MC::GetApp()
{
  return s_app;
}

std::string MC::GetCookieJar()
{
  return BOXEE::BXCurl::GetCookieJar();
}

ListItems MC::GetDirectory(const std::string& strPath)
{
  CFileItemList items;
  ListItems listItems;
#if defined(HAS_EMBEDDED) && !defined(__APPLE__) 
  int tid = gettid();
  FileSystemItem item = {0,0};
  bool safeToOpen = true;

  item.fileName = strPath.c_str();
  item.accessMode = "rb";

  if(TPApplyPolicy(tid, FILE_SYSTEM, &item, &safeToOpen) && !safeToOpen)
    return listItems;
  
  TPDisablePolicy(tid, false);
#endif
  if (DIRECTORY::CDirectory::GetDirectory(strPath, items))
  {
    for (int i = 0; i < items.Size(); i++)
    {
      ListItem item(items.Get(i));
      listItems.push_back(item);
    }
  }
#if defined(HAS_EMBEDDED) && !defined(__APPLE__)
  TPDisablePolicy(tid, true);
#endif
  return listItems;
}

void MC::SetItems(int windowId, int controlId, ListItems& items, int selectedItem)
{
  CFileItemList fileItems;

  for (size_t i = 0; i < items.size(); i++)
  {
    CFileItem* pItem =  items[i].GetFileItem().get();
    CFileItemPtr fileItem(new CFileItem(*pItem));
    fileItems.Add(fileItem);
  }

  if (fileItems.Size() > 0)
  {
    g_application.GetItemLoader().AddControl(windowId, controlId, fileItems, SORT_METHOD_NONE, SORT_ORDER_NONE, selectedItem);
  }
  else
  {
    CGUIMessage clearmsg(GUI_MSG_LABEL_RESET, windowId, controlId, 0, 0);
    g_windowManager.SendThreadMessage(clearmsg);
  }
}

ListItem MC::GetFocusedItem(int windowId, int listId)
{
  Window window = GetWindow(windowId);
  List list = window.GetList(listId);

  int iSelectedItem = list.GetFocusedItem();
  return list.GetItem(iSelectedItem);
}

std::string MC::GetGeoLocation()
{
  return g_application.GetCountryCode();
}

std::string MC::GetDeviceId()
{
#ifdef HAS_EMBEDDED
  CStdString strMac = "UNKNOWN";
  CNetworkInterfacePtr net = g_application.getNetwork().GetInterfaceByName("eth0");
  if (net.get())
  {
    strMac = net->GetMacAddress();
    strMac.Replace(":","");
  }
 
  return strMac; 
#else
  CNetworkInterfacePtr pInterface = g_application.getNetwork().GetFirstConnectedInterface();

  if( pInterface.get() )
  {
    CLog::Log(LOGDEBUG, "XAPP::MC::GetDeviceId, interface IP = %s, MAC = %s (flow)", pInterface->GetCurrentIPAddress().c_str(), pInterface->GetMacAddress().c_str());
    return pInterface->GetMacAddress();
  }

  return "";
#endif
}

bool MC::IsEmbedded()
{
   return CUtil::IsEmbedded();
}

std::string MC::GetPlatform()
{
   return CUtil::GetPlatform();
}

int MC::GetCurrentPositionInSec(const std::string& strPath)
{
  int timeInSec = 0;
  CVideoDatabase db;
  if (db.Open())
  {
    CBookmark bookmark;

    if (db.GetResumeBookMark(strPath, bookmark) )
    {
      timeInSec = lrint(bookmark.timeInSeconds);
    }
    db.Close();
  }
  return timeInSec;
}

std::string MC::GetWeatherLocation()
{
  CStdString areacode = g_guiSettings.GetString("weather.areacode1");
  std::string result = areacode.c_str();
  return result; 
}

void MC::SetWeatherLocation(std::string location)
{
  g_guiSettings.SetString("weather.areacode1", location.c_str());
  CWeather::GetInstance().Refresh();
}

bool MC::SetWeatherLocation2(std::string cityName, std::string countryCode)
{
  return BoxeeUtils::SetWeatherLocation(cityName,countryCode);
}

std::string MC::GetTimezoneCountry()
{
  CStdString s = g_guiSettings.GetString("timezone.country");
  std::string result = s.c_str();
  return result;
}

std::string MC::GetTimezoneCity()
{
  CStdString s = g_guiSettings.GetString("timezone.city");
  std::string result = s.c_str();
  return result;
}

std::string MC::GetTemperatureScale()
{
  CStdString s = g_guiSettings.GetString("locale.tempscale");
  std::string result = s.c_str();
  return result;
}

void MC::SetTemperatureScale(std::string scale)
{
  if (scale != "C" && scale != "F")
  {
    return;
  }

  CStdString value = scale.c_str();
  g_guiSettings.SetString("locale.tempscale", value);
}

bool MC::IsConnectedToInternet()
{
  return g_application.IsConnectedToInternet();
}

std::string MC::GetSystemLanguage()
{
  return g_guiSettings.GetString("locale.language");
}

/**
 * Returns unique box id
 */
std::string MC::GetUniqueId()
{
#ifndef HAS_EMBEDDED
  // Get the mac address
  CStdString strMac;
  CNetworkInterfacePtr net = g_application.getNetwork().GetInterfaceByName("eth0");
  if (!net.get())
    net = g_application.getNetwork().GetInterfaceByName("en0");

  if (net.get())
  {
    strMac = net->GetMacAddress();
    strMac.Replace(":","");
  }

  if (strMac.IsEmpty())
  {
    std::vector<CNetworkInterfacePtr> interfaces = g_application.getNetwork().GetInterfaceList();
    for (size_t i=0; i<interfaces.size(); i++)
    {
      const CStdString &name = interfaces[i]->GetName();
      strMac = interfaces[i]->GetMacAddress();
      strMac.Replace(":","");

      if (!name.IsEmpty() && !strMac.IsEmpty() && name.size() >= 2 && name.Left(2) != "lo")
        break;
    }
  }

  // Get the app id
  CStdString strId = CAppManager::GetInstance().GetLastLaunchedId();

  // Concatenate all this
  char data[2048];
  sprintf(data, "%s&%s", strMac.c_str(), strId.c_str());

  // MD5
  MD5_CTX md5Hash;
  unsigned char digest[MD5_DIGEST_LENGTH] = {0};

  memset(&md5Hash, 0, sizeof(md5Hash));
  MD5_Init(&md5Hash);
  MD5_Update(&md5Hash, data, strlen(data));
  MD5_Final(digest, &md5Hash);

  // Return string
  std::string output;
  char ch[3];
  for(int i = 0; i < MD5_DIGEST_LENGTH; ++i)
  {
    sprintf(ch, "%02x", digest[i]);
    output += ch;
  }

  return output;
#else
  FILE   *p = NULL;
  CStdString uuidCmdPath = "/opt/local/bin/bxuuid";
  CStdString uuidCmdArgs = CAppManager::GetInstance().GetLastLaunchedId();
  CStdString popenCmd = uuidCmdPath + " " + uuidCmdArgs;

  char uuid[256];
  p = popen (popenCmd.c_str(), "r");
  if (!p)
    return "";

  fgets(uuid, sizeof(uuid), p);
  uuid[strlen(uuid)-1] = '\0';
  fclose(p);

  return uuid;
#endif
}

/**
 * Returns box vendor name
 */
std::string MC::GetHardwareVendor()
{
#ifdef HAS_BOXEE_HAL
  CHalHardwareInfo info;
  if (CHalServicesFactory::GetInstance().GetHardwareInfo(info))
  {
    return info.vendor;
  }
  else
  {
    return "UNKNOWN";
  }
#else
  return "UNKNOWN";
#endif
}

/**
 * Returns box model name
 */
std::string MC::GetHardwareModel()
{
#ifdef HAS_BOXEE_HAL
  CHalHardwareInfo info;
  if (CHalServicesFactory::GetInstance().GetHardwareInfo(info))
  {
    return info.model;
  }
  else
  {
    return "UNKNOWN";
  }
#else
  return "UNKNOWN";
#endif
}

/**
 * Returns box revision number
 */
std::string MC::GetHardwareRevision()
{
#ifdef HAS_BOXEE_HAL
  CHalHardwareInfo info;
  if (CHalServicesFactory::GetInstance().GetHardwareInfo(info))
  {
    return info.revision;
  }
  else
  {
    return "UNKNOWN";
  }
#else
  return "UNKNOWN";
#endif
}

/**
 * Returns box revision number
 */
std::string MC::GetHardwareSerialNumber()
{
#ifdef HAS_BOXEE_HAL
  CHalHardwareInfo info;
  if (CHalServicesFactory::GetInstance().GetHardwareInfo(info))
  {
    return info.serialNumber;
  }
  else
  {
    return "UNKNOWN";
  }
#else
  return "UNKNOWN";
#endif
}

std::map<std::string, int> MC::m_translatedInfo;
App MC::s_app;

}
