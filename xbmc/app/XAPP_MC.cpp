
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
#include "lib/libPython/XBPython.h"
#include "AppManager.h"
#include "FileSystem/Directory.h"
#include "ItemLoader.h"
#include "VideoDatabase.h"

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
  PyObject *app = PySys_GetObject((char*)"app-id");
  if (app)
  {
    const char *strAppId = PyString_AsString(app);
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
  int iWindowId;
  PyObject *app = PySys_GetObject((char*)"app-id");
  if (app)
  {
    const char *strAppId = PyString_AsString(app);
    iWindowId = CAppManager::GetInstance().GetWindowId(strAppId, id);
  }

  CLog::Log(LOGDEBUG, "XAPP::MC::ActivateWindow, requested window id = %d, translated window id = %d (flow)", id, iWindowId);
  if (iWindowId != WINDOW_INVALID)
  {
    ThreadMessage tMsg = {TMSG_GUI_ACTIVATE_WINDOW, iWindowId, 0};
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
  if (DIRECTORY::CDirectory::GetDirectory(strPath, items))
  {
    for (int i = 0; i < items.Size(); i++)
    {
      ListItem item(items.Get(i));
      listItems.push_back(item);
    }
  }

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

std::map<std::string, int> MC::m_translatedInfo;
App MC::s_app;

}
