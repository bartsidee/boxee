/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIWindowLoginScreen.h"
#include "Application.h"
#include "GUIWindowManager.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"

#ifdef HAS_PYTHON
#include "lib/libPython/XBPython.h"
#endif

//Boxee
#include "utils/log.h"
#include "BoxeeLoginWizardManager.h"
#include "GUIDialogBoxeeLoginEditExistingUser.h"
//end Boxee

#define USERS_LIST_CONTROL_ID                                 62
#define EDIT_USER_BUTTON_CONTROL_ID                           61
#define CONTROL_ACTION_MENU_GROUP                             9000
#define ADD_USER_BUTTON_CONTROL_ID                            9011
#define NETWORK_BUTTON_CONTROL_ID                             9013
#define SHUTDOWN_BUTTON_CONTROL_ID                            9014

using namespace XFILE;
using namespace BOXEE;

CGUIWindowLoginScreen::CGUIWindowLoginScreen() : CGUIWindow(WINDOW_LOGIN_SCREEN, "boxee_login_screen.xml")
{
  m_vecItems = new CFileItemList;
}

CGUIWindowLoginScreen::~CGUIWindowLoginScreen()
{
  m_loader.StopThread();
  delete m_vecItems;
}

bool CGUIWindowLoginScreen::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
  {
    m_viewControl.Reset();
  }
  break;
  case GUI_MSG_UPDATE:
  {
    if (message.GetSenderId() == WINDOW_LOGIN_SCREEN)
    {
      Load();
    }

    return true;
  }
  break;
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();

    if (iControl == USERS_LIST_CONTROL_ID)
    {
      int iAction = message.GetParam1();
      
      if (iAction == ACTION_PREVIOUS_MENU) // oh no u don't
      {
        return false;
      }
      else if (iAction == ACTION_SELECT_ITEM ||
               iAction == ACTION_SHOW_INFO ||
               iAction == ACTION_CONTEXT_MENU ||
               iAction == ACTION_MOUSE_RIGHT_CLICK ||
               iAction == ACTION_MOUSE_LEFT_CLICK)
      {
        int userSelectedIndex = m_viewControl.GetSelectedItem() + 1; // +1 is in order to jump over Master User

        CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnMessage - RegisterUser [userSelectedIndex=%d] was selected in the list. Going to call DoLoginExistUserFromList(). (login)",userSelectedIndex);

        bool retVal = DoLoginExistUserFromList(userSelectedIndex);

        CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnMessage - Call to DoLoginExistUserFromList() returned [retVal=%d] (login)",retVal);

        return retVal;
      }
    }
    else if (iControl == EDIT_USER_BUTTON_CONTROL_ID)
    {
      bool retVal = OnEditButton();
      return retVal;
    }
    else if (iControl == ADD_USER_BUTTON_CONTROL_ID)
    {
      CBoxeeLoginWizardManager::GetInstance().RunWizard(true);
      return true;
    }
  }
  break;
  case GUI_MSG_SETFOCUS:
  {
    if (m_viewControl.HasControl(message.GetControlId()) && m_viewControl.GetCurrentControl() != message.GetControlId())
    {
      m_viewControl.SetFocused();
      return true;
    }
  }
  break;
  default:
  {
    // nothing to do
  }
  break;
  }

  return CGUIWindow::OnMessage(message);
}

bool CGUIWindowLoginScreen::OnAction(const CAction &action)
{
  if (action.id == ACTION_PREVIOUS_MENU)
  {
#ifndef HAS_EMBEDDED
    g_application.getApplicationMessenger().Quit();
#endif
    return true;
  }
  // don't allow any built in actions to act here.
  // this forces only navigation type actions to be performed.
  else if (action.id == ACTION_BUILT_IN_FUNCTION)
  {
    return true;  // pretend we handled it
  }

  return CGUIWindow::OnAction(action);
}

void CGUIWindowLoginScreen::Render()
{
  CGUIWindow::Render();
}

void CGUIWindowLoginScreen::OnInitWindow()
{
  CGUIWindow::OnInitWindow();
  Load();
}

void CGUIWindowLoginScreen::Load()
{
  // Update list/thumb control
  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);
  Update();
  m_viewControl.SetFocused();

  int numOfExistProfiles = g_settings.m_vecProfiles.size();
  if(numOfExistProfiles < 2)
  {
    CBoxeeLoginWizardManager::GetInstance().RunWizard(false);
  }
}

void CGUIWindowLoginScreen::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(USERS_LIST_CONTROL_ID));
}

void CGUIWindowLoginScreen::Update()
{
  m_loader.StopThread();
  m_vecItems->Clear();
  for (unsigned int i=0;i<g_settings.m_vecProfiles.size(); ++i)
  {
    if (g_settings.m_vecProfiles[i].getName() == "Master user")
    {
      continue;
    }
    
    CFileItemPtr item(new CFileItem(g_settings.m_vecProfiles[i].getName()));
    CStdString strLabel;

    if (g_settings.m_vecProfiles[i].getDate().IsEmpty())
    {
      strLabel = g_localizeStrings.Get(20113);
    }
    else
    {
      strLabel.Format(g_localizeStrings.Get(20112),g_settings.m_vecProfiles[i].getDate());
    }

    item->SetLabel2(strLabel);
    item->SetThumbnailImage(g_settings.m_vecProfiles[i].getThumb());
    item->SetCachedPictureThumb();

    if (g_settings.m_vecProfiles[i].getThumb().IsEmpty() || g_settings.m_vecProfiles[i].getThumb().Equals("-"))
    {
      item->SetThumbnailImage("unknown-user.png");
    }

    item->SetLabelPreformated(true);
    m_vecItems->Add(item);
  }

  m_viewControl.SetItems(*m_vecItems);
  m_viewControl.SetSelectedItem(0);

  m_loader.Load(*m_vecItems);
}

void CGUIWindowLoginScreen::Show()
{
  CGUIWindowLoginScreen* pWindow = (CGUIWindowLoginScreen*)g_windowManager.GetWindow(WINDOW_LOGIN_SCREEN);

  if(pWindow)
  {
    g_windowManager.ActivateWindow(WINDOW_LOGIN_SCREEN);
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIWindowLoginScreen::Show - FAILED to get CGUIWindowLoginScreen from WindowManager (login)");
  }
}

bool CGUIWindowLoginScreen::DoLoginExistUserFromList(int userSelectedIndex)
{
  // Case of RegisterUser from list-> Need to set the UserName and Password from Profiles file

  CStdString password = g_settings.m_vecProfiles[userSelectedIndex].getLockCode();

  if(password.Equals("-"))
  {
    // Password is not saved -> Same as hit on Edit button scenario

    CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLoginExistUserFromList - Password is not saved. Going to call OnEditButton() (login)");

    return OnEditButton();
  }
  else
  {
    CStdString username = g_settings.m_vecProfiles[userSelectedIndex].getID();

    CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLoginExistUserFromList - After set [username=%s][password=%s] (login)",username.c_str(),password.c_str());

    BOXEE::BXLoginStatus loginStatus = CBoxeeLoginManager::DoLogin(username,password,true,userSelectedIndex);

    if (loginStatus == LOGIN_SUCCESS)
    {
      CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLoginExistUserFromList - Login Succeeded [loginStatus=%d=LOGIN_SUCCESS] -> Going to call FinishSuccessfulLogin(false) (login)",loginStatus);
      g_application.GetBoxeeLoginManager().FinishSuccessfulLogin();

      CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLoginExistUserFromList - Going to WINDOW_HOME (login)");
      g_windowManager.ChangeActiveWindow(WINDOW_HOME);
    }
    else
    {
      SET_CONTROL_FOCUS(USERS_LIST_CONTROL_ID,0);
    }

    return (loginStatus == LOGIN_SUCCESS);
  }
}

bool CGUIWindowLoginScreen::OnEditButton()
{
  int userSelectedIndex = (m_viewControl.GetSelectedItem() + 1); // +1 is in order to jump over Master User
  CStdString username = g_settings.m_vecProfiles[userSelectedIndex].getName();

  CStdString userId = g_settings.m_vecProfiles[userSelectedIndex].getID();
  CStdString lockCode = g_settings.m_vecProfiles[userSelectedIndex].getLockCode();
  bool rememberPassword = (lockCode != "-");

  CStdString password = "";
  if (rememberPassword)
  {
    password = lockCode;
  }

  CStdString thumb = g_settings.m_vecProfiles[userSelectedIndex].getThumb();

  CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnEditButton - call DialogBoxeeLoginEditExistingUser with  [userSelectedIndex=%d][username=%s][password=%s][rememberPassword=%d][thumb=%s] (login)",userSelectedIndex,username.c_str(),password.c_str(),rememberPassword,thumb.c_str());

  bool retVal = CGUIDialogBoxeeLoginEditExistingUser::Show(userSelectedIndex, username, userId, password, rememberPassword, thumb);

  CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnEditButton - call to DialogBoxeeLoginEditExistingUser returned [%d] with [password=%s][rememberPassword=%d] (login)",retVal,password.c_str(),rememberPassword);

  return retVal;
}

