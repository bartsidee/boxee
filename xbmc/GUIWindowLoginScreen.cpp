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

#include "system.h"
#include "Application.h"
#include "GUIWindowLoginScreen.h"
#include "GUIWindowSettingsProfile.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogProfileSettings.h"
#include "GUIDialogBoxeeUserLogin.h"
#include "utils/GUIInfoManager.h"
#include "GUIPassword.h"
#ifdef HAS_PYTHON
#include "lib/libPython/XBPython.h"
#endif
#include "lib/libscrobbler/scrobbler.h"
#include "utils/Weather.h"
#include "utils/Network.h"
#include "SkinInfo.h"
#include "lib/libBoxee/boxee.h"
#include "Profile.h"
#include "GUIWindowManager.h"
#include "GUIDialogOK.h"
#include "Settings.h"
#include "GUISettings.h"
#include "FileSystem/File.h"
#include "FileItem.h"
#include "md5.h"
#include "utils/Base64.h"
#include "FileSystem/Directory.h"
#include "GUIDialogProgress.h"
#include "Util.h"
#include "LocalizeStrings.h"
#include "LocalizeStrings.h"
#include "SystemInfo.h"
#include "SpecialProtocol.h"
#include "GUIEditControl.h"
#include "GUIRadioButtonControl.h"
#include "GUILabelControl.h"
#include "GUIUserMessages.h"
#include "utils/RegExp.h"
#include "GUIInfoTypes.h"

//Boxee
#include "GUIDialogBoxeeLoggingIn.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "GUIDialogYesNo.h"
#include "BoxeeVersionUpdateManager.h"
#include "BoxeeLoginManager.h"
#include "GUIDialogBoxeeDropdown.h"
#include "GUIDialogOK2.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogBoxeeMessageScroll.h"
//end Boxee

#include "Util.h"
#include "BoxeeUtils.h"

using namespace XFILE;
using namespace BOXEE;

#define CONTROL_BIG_LIST                                 52
#define CONTROL_BIG_LIST_BG                              53
#define CONTROL_LABEL_HEADER                             2
#define CONTROL_LABEL_SELECTED_PROFILE                   3
#define CONTROL_PROFILE_EDIT_BUTTON                      60
#define CONTROL_USER_PASSWORD_GROUP                      8000

#define CONTROL_EU_GROUP                                 8600
#define CONTROL_USER_NAME_EDIT                           8601
#define CONTROL_USER_NAME_LABEL                          8610
#define CONTROL_USER_NAME_TITLE                          8011
#define CONTROL_PASSWORD_EDIT                            8602
#define CONTROL_REMEMBER_PASSWORD_RADIO_BUTTON           8603
#define CONTROL_LOGIN_BUTTON                             8604
#define CONTROL_CANCEL_BUTTON                            8605
#define CONTROL_REMOVE_USER_BUTTON                       8606

#define CONTROL_CU_GROUP                                 8700
#define CONTROL_CU_EMAIL_EDIT                            8701
#define CONTROL_CU_PASSWORD_EDIT                         8702
#define CONTROL_CU_RETYPE_PASSWORD_EDIT                  8703
#define CONTROL_CU_USERNAME_LIST                         8730
#define CONTROL_CU_USERNAME__LABEL                       8731
#define CONTROL_CU_USERNAME_DROPDOWN                     8740
#define CONTROL_CU_DONE_BUTTON                           8707
#define CONTROL_CU_CANCEL_BUTTON                         8708
#define CONTROL_CU_TOU_LABEL_GROUP                       8730
#define CONTROL_CU_TOU_LABEL                             8735
#define CONTROL_CU_TOU_BUTTON                            8736
#define CONTROL_CU_PRIVACY_BUTTON                        8738

#define CONTROL_USER_TYPE_GROUP                          8050
#define CONTROL_USER_TYPE_EXIST                          8051
#define CONTROL_USER_TYPE_NEW                            8052
#define CONTROL_LIST_BUTTON                              9000

#define NEW_USER_PROFILE_INDEX                           -100

#define CHECK_CREATE_USER_EDITBOX_DELAY                  15

#define EXISTS_XML_ELEMENT                               "exists"
#define SUGGESTION_XML_ELEMENT                           "suggestion"

#define BOXEE_XML_ELEMENT                                "boxee"
#define SUCCESS_XML_ELEMENT                              "success"
#define LOGIN_XML_ELEMENT                                "login"
#define OBJECT_XML_ELEMENT                               "object"
#define ERRORS_XML_ELEMENT                               "errors"

#define USERNAME_DROPDOWN_POS_X                          500.0
#define USERNAME_DROPDOWN_POS_Y                          15.0

#define EMAIL_REGEXP_PATTERN                             "^[^@]{1,64}@[\\w\\.-]{1,255}\\.[a-zA-Z]{2,}$"

CGUIWindowLoginScreen::CGUIWindowLoginScreen(void) : CGUIWindow(WINDOW_LOGIN_SCREEN, "LoginScreen.xml")
{
  watch.StartZero();
  m_vecItems = new CFileItemList;
  m_iSelectedItem = -1;

  m_editUserButtonWasHit = false;
  m_addUserButtonWasHit = false;
  m_username = "";
  m_password = "";
  m_rememberPassword = false;
  m_needToEncodePassword = false;
  m_userSelectedIndex = NEW_USER_PROFILE_INDEX;

  m_addExistUserIsSelected = false;
  m_createNewtUserIsSelected = false;

  m_isNewUserEmailValid = false;
  m_checkNewUserEmailDelay = 0;
  m_isNewUserPasswordValid = false;
  m_checkNewUserPasswordDelay = 0;

  m_isNewUserRetypePasswordMatch = false;
  m_checkNewUserRetypePasswordDelay = 0;

  m_isUsernameSuggestionListValid = false;

  m_addExistUserIsSelected = NULL;
  m_createNewtUserIsSelected = NULL;

  m_validEmail = "";
  m_validEmailPrefix = "";
  m_validPassword = "";

  m_selectedFirstName = "";
  m_selectedLastName = "";

  m_touStr = "";
  m_privacyTermStr = "";

  m_customTitle = g_localizeStrings.Get(53444);
}

CGUIWindowLoginScreen::~CGUIWindowLoginScreen(void)
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

    if (iControl == CONTROL_BIG_LIST)
    {
      int iAction = message.GetParam1();
      
      if (iAction == ACTION_PREVIOUS_MENU) // oh no u don't
      {
        return false;
      }
      else if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_SHOW_INFO || iAction == ACTION_CONTEXT_MENU ||
               iAction == ACTION_MOUSE_RIGHT_CLICK || iAction == ACTION_MOUSE_LEFT_CLICK)
      {
        m_userSelectedIndex = m_viewControl.GetSelectedItem() + 1; // +1 is in order to jump over Master User

        CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnMessage - RegisterUser [m_userSelectedIndex=%d] was selected in the list. Going to call DoLoginExistUserFromList(). [m_editUserButton=%d][m_addUserButtonWasHit=%d] (login)",m_userSelectedIndex,m_editUserButtonWasHit,m_addUserButtonWasHit);

        bool retVal = DoLoginExistUserFromList();

        CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnMessage - Call to DoLoginExistUserFromList() returned [retVal=%d] (login)",retVal);

        return retVal;
      }
    }
    else if (iControl == CONTROL_PROFILE_EDIT_BUTTON)
    {
      bool retVal = OnEditButton();

      return retVal;
    }
    else if (iControl == CONTROL_PASSWORD_EDIT)
    {
      // A change was made in CONTROL_PASSWORD_EDIT -> Need to encode pasword before login
      m_needToEncodePassword = true;
      return true;
    }
    else if (iControl == CONTROL_REMEMBER_PASSWORD_RADIO_BUTTON)
    {
      const CGUIControl* control = GetControl(iControl);
      m_rememberPassword = ((CGUIRadioButtonControl*)control)->IsSelected();

      return true;
    }
    else if (iControl == CONTROL_LOGIN_BUTTON)
    {
      bool retVal = OnLoginButton();

      return retVal;
    }
    else if (iControl == CONTROL_CANCEL_BUTTON)
    {
      bool retVal = OnCancelButton();

      return retVal;
    }
    else if (iControl == CONTROL_REMOVE_USER_BUTTON)
    {
      bool retVal = OnRemoveUserButton();

      return retVal;
    }
    else if (iControl == CONTROL_CU_CANCEL_BUTTON)
    {
      bool retVal = OnCancelButton();

      return retVal;
    }
    else if (iControl == CONTROL_CU_DONE_BUTTON)
    {
      bool isLogin = false;
      int profileId = -1;

      bool succeeded = OnCreateUserDoneButton(profileId, isLogin);

      if (succeeded)
      {
        if (isLogin)
        {
          g_application.SetOfflineMode(false);
        }

        bool isFirstUser = true;

        if (profileId > 1)
        {
          isFirstUser = false;
        }

        g_application.GetBoxeeLoginManager().FinishSuccessfulLogin(isFirstUser);
      }

      return succeeded;
    }
    else if (iControl == CONTROL_LIST_BUTTON)
    {
      CGUIBaseContainer* pControl = (CGUIBaseContainer*) GetControl(iControl);

      if (pControl)
      {
        CGUIListItemPtr selectedButton = pControl->GetSelectedItemPtr();
        if (!selectedButton.get())
        {
          CLog::Log(LOGERROR,"CGUIWindowLoginScreen::OnMessage - FAILED to get the SelectedItem from container [%d=CONTROL_LIST_BUTTON] container (bma)",iControl);
          return false;
        }

        CStdString selectedButtonLabel = selectedButton->GetLabel();

        if(selectedButtonLabel == g_localizeStrings.Get(53410))
        {
          bool retVal = false;

          if(g_settings.m_vecProfiles.size() < 2)
          {
            retVal = OnAddUserButton(true);
          }
          else
          {
            retVal = OnAddUserButton(false);
          }

          g_application.UpdateLibraries();

          return retVal;
        }
      }
      else
      {
        CLog::Log(LOGWARNING,"CGUIWindowLoginScreen::OnMessage - FAILED to get control in GUI_MSG_CLICKED of [CONTROL_LIST_BUTTON=%d] (login)",CONTROL_LIST_BUTTON);
      }
    }
    else if (iControl == CONTROL_USER_TYPE_EXIST)
    {
      bool retVal = false;

      if(g_settings.m_vecProfiles.size() < 2)
      {
        retVal = OnAddExistUser(true);
      }
      else
      {
        retVal = OnAddExistUser(false);
      }

      SET_CONTROL_FOCUS(CONTROL_USER_NAME_EDIT,0);

      return retVal;
    }
    else if (iControl == CONTROL_USER_TYPE_NEW)
    {
      bool retVal = OnCreateNewUser();
      return retVal;
    }
    else if (iControl == CONTROL_CU_USERNAME_DROPDOWN)
    {
      if (usernameSuggestionList.Size() > 0)
      {
        CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnMessage - CONTROL_CU_USERNAME_DROPDOWN - Going to open DropDown dialog. [username=%s] (cnu)",m_selectedUsername.c_str());

        if (CGUIDialogBoxeeDropdown::Show(usernameSuggestionList, g_localizeStrings.Get(53564), m_selectedUsername,USERNAME_DROPDOWN_POS_X,USERNAME_DROPDOWN_POS_Y,1,this))
        {
          CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnMessage - CONTROL_CU_USERNAME_DROPDOWN - Choose [username=%s]. Going to enable DONE button (cnu)",m_selectedUsername.c_str());
          SetProperty("choosen-username",m_selectedUsername);
        }

        return true;
      }
      else
      {
        CLog::Log(LOGWARNING,"CGUIWindowLoginScreen::OnMessage - CONTROL_CU_USERNAME_DROPDOWN - Can't open username dropdown because [SuggestionListSize=%d] (cnu)",usernameSuggestionList.Size());
      }
    }
    else if (iControl == CONTROL_CU_TOU_BUTTON)
    {
      return ShowTOU();
    }
    else if (iControl == CONTROL_CU_PRIVACY_BUTTON)
    {
      return ShowPrivacyTerm();
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
  case GUI_MSG_LABEL_BIND:
  {
    SetProperty("loading", false);

    if (message.GetPointer() && message.GetControlId() == 0)
    {
      CFileItemList* items = (CFileItemList*)message.GetPointer();
      if (items)
      {
        usernameSuggestionList.Clear();
        usernameSuggestionList.Append(*items);

        if (usernameSuggestionList.Size() > 0)
        {
          SET_CONTROL_VISIBLE(CONTROL_CU_USERNAME__LABEL);
          SET_CONTROL_VISIBLE(CONTROL_CU_USERNAME_DROPDOWN);

          m_selectedUsername = (usernameSuggestionList[0])->GetLabel();
          SetProperty("choosen-username",m_selectedUsername);

          m_isUsernameSuggestionListValid = true;

          CONTROL_ENABLE(CONTROL_CU_PASSWORD_EDIT);
          CONTROL_ENABLE(CONTROL_CU_RETYPE_PASSWORD_EDIT);
        }

        delete items;
      }
    }
  }
  break;
  default:
    break;
  }

  return CGUIWindow::OnMessage(message);
}

bool CGUIWindowLoginScreen::OnAction(const CAction &action)
{
  if (action.id == ACTION_PREVIOUS_MENU)
  {
    if (m_createNewtUserIsSelected)
    {
      m_createNewtUserIsSelected = false;
    }

    CGUIBaseContainer* pControl = (CGUIBaseContainer*)GetControl(CONTROL_CANCEL_BUTTON);
    if (pControl)
    {
      if (!pControl->IsVisible())
      {
        g_application.getApplicationMessenger().Quit();
        return true;
      }
      else
      {
        OnCancelButton();
        return true;
      }
    }
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
  if (m_createNewtUserIsSelected)
  {
    processCreateNewUser();
  }

  if (GetFocusedControlID() == CONTROL_BIG_LIST && g_windowManager.GetTopMostModalDialogID() == WINDOW_INVALID)
    if (m_viewControl.HasControl(CONTROL_BIG_LIST))
      m_iSelectedItem = m_viewControl.GetSelectedItem();

  CStdString strLabel;
  //strLabel.Format(g_localizeStrings.Get(20114),m_iSelectedItem+1,g_settings.m_vecProfiles.size());
  strLabel.Format(g_localizeStrings.Get(20114),m_iSelectedItem+1,g_settings.m_vecProfiles.size());
  SET_CONTROL_LABEL(CONTROL_LABEL_SELECTED_PROFILE,strLabel);
  CGUIWindow::Render();
}

void CGUIWindowLoginScreen::OnInitWindow()
{
  m_isNewUserEmailValid = false;
  m_checkNewUserEmailDelay = 0;
  m_isNewUserPasswordValid = false;
  m_checkNewUserPasswordDelay = 0;
  m_checkNewUserRetypePasswordDelay = 0;

  m_isNewUserRetypePasswordMatch = false;
  m_isUsernameSuggestionListValid = false;

  m_validEmail = "";
  m_validEmailPrefix = "";
  m_validPassword = "";
  m_selectedUsername = "";
  m_selectedFirstName = "";
  m_selectedLastName = "";

  CGUIWindow::OnInitWindow();
  Load();
}

void CGUIWindowLoginScreen::Load()
{
  m_editUserButtonWasHit = false;
  m_addUserButtonWasHit = false;
  m_username = "";
  m_password = "";
  m_rememberPassword = false;
  m_needToEncodePassword = false;
  m_userSelectedIndex = NEW_USER_PROFILE_INDEX;

  // Update list/thumb control
  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);
  Update();
  m_viewControl.SetFocused();
  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER,g_localizeStrings.Get(20115));

  int numOfExistProfiles = g_settings.m_vecProfiles.size();

  if(numOfExistProfiles < 2)
  {
    // Only MasterUser exist -> No UserProfiles -> Show Login

    OnAddUserButton(true);
  }
  else
  {
    // UserProfiles exist -> Show List

    SET_CONTROL_VISIBLE(CONTROL_BIG_LIST);
    SET_CONTROL_VISIBLE(CONTROL_PROFILE_EDIT_BUTTON);
    SET_CONTROL_HIDDEN(CONTROL_USER_PASSWORD_GROUP);
    SET_CONTROL_HIDDEN(CONTROL_CANCEL_BUTTON);
  }
}

void CGUIWindowLoginScreen::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(CONTROL_BIG_LIST));
}

void CGUIWindowLoginScreen::Update()
{
  m_loader.StopThread();
  m_vecItems->Clear();
  for (unsigned int i=0;i<g_settings.m_vecProfiles.size(); ++i)
  {
    if (g_settings.m_vecProfiles[i].getName() == "Master user")
      continue;
    
    CFileItemPtr item(new CFileItem(g_settings.m_vecProfiles[i].getName()));
    CStdString strLabel;

    if (g_settings.m_vecProfiles[i].getDate().IsEmpty())
      strLabel = g_localizeStrings.Get(20113);
    else
      strLabel.Format(g_localizeStrings.Get(20112),g_settings.m_vecProfiles[i].getDate());
    item->SetLabel2(strLabel);
    item->SetThumbnailImage(g_settings.m_vecProfiles[i].getThumb());
    item->SetCachedPictureThumb();
    if (g_settings.m_vecProfiles[i].getThumb().IsEmpty() || g_settings.m_vecProfiles[i].getThumb().Equals("-"))
      item->SetThumbnailImage("unknown-user.png");
    item->SetLabelPreformated(true);
    m_vecItems->Add(item);
  }

  m_viewControl.SetItems(*m_vecItems);
  m_viewControl.SetSelectedItem(0);

  m_loader.Load(*m_vecItems);
}

CFileItemPtr CGUIWindowLoginScreen::GetCurrentListItem(int offset)
{
  int item = m_viewControl.GetSelectedItem();
  if (item < 0 || !m_vecItems->Size()) return CFileItemPtr();

  item = (item + offset) % m_vecItems->Size();
  if (item < 0) item += m_vecItems->Size();
  return m_vecItems->Get(item);
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

bool CGUIWindowLoginScreen::DoLoginNewUser()
{
  //////////////////////////////////////////////
  // Check that this user isn't already exist //
  //////////////////////////////////////////////

  CStdString username = ((CGUIEditControl*)GetControl(CONTROL_USER_NAME_EDIT))->GetLabel2();

  if(DoesThisUserAlreadyExist(username))
  {
    CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLogin - User [%s] already exist on this machine (login)",username.c_str());
    CGUIDialogOK::ShowAndGetInput(20068,51603,20022,20022);
    return false;
  }

  m_username = username;

  CStdString password = ((CGUIEditControl*)GetControl(CONTROL_PASSWORD_EDIT))->GetLabel2();
  m_password = EncodePassword(password);

  return DoLogin(NEW_USER_PROFILE_INDEX);
}

bool CGUIWindowLoginScreen::DoLoginExistUserFromLoginButton()
{
  // Case of RegisterUser from EditMode-> Need to set the UserName and Password from editbox

  m_username = ((CGUIEditControl*)GetControl(CONTROL_USER_NAME_EDIT))->GetLabel2();

  CStdString password = ((CGUIEditControl*)GetControl(CONTROL_PASSWORD_EDIT))->GetLabel2();
  if(m_needToEncodePassword)
  {
    password = EncodePassword(password);
    CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLoginExistUserFromLoginButton - Because [m_needToEncodePassword=%d] password was encoded before set (login)",m_needToEncodePassword);
  }
  m_password = password;

  CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLoginExistUserFromLoginButton - After set [m_username=%s][m_password=%s] (login)",m_username.c_str(),m_password.c_str());

  return DoLogin(m_userSelectedIndex);
}

bool CGUIWindowLoginScreen::DoLoginExistUserFromList()
{
  // Case of RegisterUser from list-> Need to set the UserName and Password from Profiles file

  CStdString password = g_settings.m_vecProfiles[m_userSelectedIndex].getLockCode();

  if(password.Equals("-"))
  {
    // Password is not saved -> Same as hit on Edit button scenario

    CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLoginExistUserFromList - Password is not saved. Going to call OnEditButton() (login)");

    return OnEditButton();
  }
  else
  {
    m_username = g_settings.m_vecProfiles[m_userSelectedIndex].getID();
    m_password = g_settings.m_vecProfiles[m_userSelectedIndex].getLockCode();
    m_rememberPassword = true;

    CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLoginExistUserFromList - After set [m_username=%s][m_password=%s] (login)",m_username.c_str(),m_password.c_str());

    bool loginSucceeded = DoLogin(m_userSelectedIndex);

    if (loginSucceeded)
    {
      CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLoginExistUserFromList - Login Succeeded [loginSucceeded=%d] -> Going to call FinishSuccessfulLogin() (login)",loginSucceeded);

      g_application.GetBoxeeLoginManager().FinishSuccessfulLogin(false);
    }

    return loginSucceeded;
  }
}

bool CGUIWindowLoginScreen::DoLogin(int profileId)
{
  CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLogin -Enter function with [profileId=%d] (login)",profileId);

  // Set the login credentials
  BOXEE::BXCredentials creds;
  creds.SetUserName(m_username);
  creds.SetPassword(m_password);

  CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLogin - Credentials was set [UserName=%s][Password=%s] (login)",creds.GetUserName().c_str(),creds.GetPassword().c_str());

  // Prepare the logging-in screen
  CGUIDialogBoxeeLoggingIn* pDlgLoggingIn = (CGUIDialogBoxeeLoggingIn*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_LOGGING_IN);
  pDlgLoggingIn->Reset();
  pDlgLoggingIn->SetName(m_username);

  if (profileId != NEW_USER_PROFILE_INDEX)
  {
    pDlgLoggingIn->SetProfileToLoad(profileId);
  }

  CBoxeeLoginManager::SetProxyCreds(creds);

  CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLogin - Going to call Boxee::Login_bg() with credentials [UserName=%s][Password=%s] and run CGUIDialogBoxeeLoggingIn (login)",creds.GetUserName().c_str(),creds.GetPassword().c_str());

  // Do the actual login and show the modal dialog
  BOXEE::Boxee::GetInstance().SetCredentials(creds);

  XFILE::CFileCurl::SetCookieJar(BOXEE::BXCurl::GetCookieJar());

  BOXEE::Boxee::GetInstance().Login_bg(creds);
  pDlgLoggingIn->DoModal();

  // Get the login result and act accordingly
  BOXEE::BXLoginStatus boxeeLoginStatus = pDlgLoggingIn->IsLoginSuccessful();

  bool result = false;

  switch (boxeeLoginStatus)
  {
  case BOXEE::LOGIN_SUCCESS:
  {
    g_application.SetOfflineMode(false);
    result = true;
    CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLogin - After call to Boxee::Login_bg() [boxeeLoginStatus=%d=LOGIN_SUCCESS] and result was set to [%d] (login)",boxeeLoginStatus,result);
  }
  break;
  case LOGIN_GENERAL_ERROR:
  {
    CGUIDialogOK::ShowAndGetInput(20068,51075,20022,20022);

    result = false;
    CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLogin - Case of LOGIN_GENERAL_ERROR -> result was set to [%d=FALSE] (login)",result);
  }
  break;
  case BOXEE::LOGIN_ERROR_ON_NETWORK:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLogin - After call to Boxee::Login_bg() [boxeeLoginStatus=%d=LOGIN_ERROR_ON_NETWORK] (login)",boxeeLoginStatus);

    if (profileId == NEW_USER_PROFILE_INDEX)
    {
      // Case of new user and no Internet connection -> Don't enter Boxee
      result = false;
      CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLogin - Case of NewUser with LOGIN_ERROR_ON_NETWORK -> result was set to [%d=FALSE] (login)",result);
      CGUIDialogOK2::ShowAndGetInput(53743, 53744);
    }
    else
    {
      // If the profile exist, prompt the user to decide whether to work OffLine or not

      CProfile& p = g_settings.m_vecProfiles[profileId];
      if (p.getLastLockCode() != m_password)
      {
        result = false;
        CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLogin - Case of RegisterUser with wrong password and LOGIN_ERROR_ON_NETWORK -> result was set to [%d=FALSE] (login)",result);

        CGUIDialogOK::ShowAndGetInput(20068,20117,20022,20022);

        m_password = "";
        ((CGUIEditControl*)GetControl(CONTROL_PASSWORD_EDIT))->SetLabel2(m_password);
        SET_CONTROL_FOCUS(CONTROL_PASSWORD_EDIT,0);
      }
      else if (CGUIDialogYesNo::ShowAndGetInput(20068,51051,51052,51053))
      {
        g_application.SetOfflineMode(true);
        g_settings.LoadProfile(profileId);
        result = true;
        CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLogin - Case of RegisterUser and LOGIN_ERROR_ON_NETWORK -> The user choose to login in offline mode -> SetOfflineMode was called with [TRUE] and result was set to [%d=TRUE] (login)",result);
      }
      else
      {
        result = false;
        CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLogin - Case of RegisterUser and LOGIN_ERROR_ON_NETWORK -> The user choose NOT to login in offline mode -> result was set to [%d=FALSE] (login)",result);
      }
    }
  }
  break;
  case BOXEE::LOGIN_ERROR_ON_CREDENTIALS:
  {
    CGUIDialogOK::ShowAndGetInput(20068,20117,20022,20022);

    m_password = "";
    ((CGUIEditControl*)GetControl(CONTROL_PASSWORD_EDIT))->SetLabel2(m_password);
    SET_CONTROL_FOCUS(CONTROL_PASSWORD_EDIT,0);

    result = false;
    CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLogin - After call to Boxee::Login_bg() [boxeeLoginStatus=%d=LOGIN_ERROR_ON_CREDENTIALS] and result was set to [%d] (login)",boxeeLoginStatus,result);
  }
  break;
  default:
    break;
  }

  // If login failed, nothing more to do here
  if (!result)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLogin - Login FAILED. Exit function and return FALSE (login)");
    return false;
  }

  CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLogin - Login SUCCEEDED (login)");

  // Now we need to update the profile or create a new profile
  if (profileId == NEW_USER_PROFILE_INDEX)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLogin - For NEW user going to call CreateProfile() (login)");

    profileId = g_application.GetBoxeeLoginManager().CreateProfile(m_username, pDlgLoggingIn->GetUserObj());

    CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLogin - Going to call LoadProfile() for [profileId=%d] (login)",profileId);

    g_settings.LoadProfile(profileId);
  }

  CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLogin - Going to call UpdateProfile() with [profileId=%d][password=%s][RememberPassword=%d] (login)",profileId,m_password.c_str(),m_rememberPassword);

  // Update the password (both OffLine of online if the user asked for it) of the user
  g_application.GetBoxeeLoginManager().UpdateProfile(profileId,m_password, m_rememberPassword);

  CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::DoLogin - Exit function and return TRUE (login)");

  return true;
}

bool CGUIWindowLoginScreen::DoesThisUserAlreadyExist(const CStdString& username)
{
  for (unsigned int i=0;i<g_settings.m_vecProfiles.size(); ++i)
  {
    CStdString id = (g_settings.m_vecProfiles[i]).getID();

    if (id.Equals(username))
    {
      return true;
    }
  }

  return false;
}

bool CGUIWindowLoginScreen::OnAddUserButton(bool firstUser)
{
  m_editUserButtonWasHit = false;
  m_addUserButtonWasHit = true;

  bool succeeded = OnAddExistUser(firstUser);

  return succeeded;
}

bool CGUIWindowLoginScreen::OnCreateNewUser()
{
  m_addExistUserIsSelected = false;
  m_createNewtUserIsSelected = true;

  m_validEmail = "";
  m_validEmailPrefix = "";
  m_isNewUserEmailValid = false;
  m_validPassword = "";
  m_isNewUserPasswordValid = false;
  m_needToGetUsernameFromServer = false;

  m_isNewUserRetypePasswordMatch = false;

  m_isUsernameSuggestionListValid = false;

  usernameSuggestionList.Clear();
  m_selectedUsername = "";

  SetProperty("email-is-valid", false);
  SetProperty("password-is-valid", false);
  SetProperty("retype-password-is-valid", false);

  SetProperty("choosen-username","");

  m_unsGetterProcessor.Start(1);

  ((CGUILabelControl*)GetControl(CONTROL_USER_NAME_TITLE))->SetLabel(g_localizeStrings.Get(53443));

  CStdString rawTouLabelStr = g_localizeStrings.Get(53435);
  CStdString buttonLabel = g_localizeStrings.Get(53449);
  CStdString touLabelStr;
  touLabelStr.Format(rawTouLabelStr.c_str(), buttonLabel);
  ((CGUILabelControl*)GetControl(CONTROL_CU_TOU_LABEL))->SetLabel(touLabelStr);

  SET_CONTROL_SELECTED(GetID(), CONTROL_USER_TYPE_EXIST, false);
  SET_CONTROL_SELECTED(GetID(), CONTROL_USER_TYPE_NEW, true);

  SET_CONTROL_VISIBLE(CONTROL_CU_GROUP);

  SET_CONTROL_HIDDEN(CONTROL_EU_GROUP);

  CONTROL_DISABLE(CONTROL_CU_DONE_BUTTON);
  CONTROL_DISABLE(CONTROL_CU_PASSWORD_EDIT);
  CONTROL_DISABLE(CONTROL_CU_RETYPE_PASSWORD_EDIT);
  SET_CONTROL_HIDDEN(CONTROL_CU_USERNAME__LABEL);
  SET_CONTROL_HIDDEN(CONTROL_CU_USERNAME_DROPDOWN);

  int numOfExistProfiles = g_settings.m_vecProfiles.size();
  if(numOfExistProfiles < 2)
  {
    SET_CONTROL_HIDDEN(CONTROL_CU_CANCEL_BUTTON);
  }
  else
  {
    SET_CONTROL_VISIBLE(CONTROL_CU_CANCEL_BUTTON);
  }

  SET_CONTROL_FOCUS(CONTROL_CU_EMAIL_EDIT,0);

  return true;
}

bool CGUIWindowLoginScreen::OnAddExistUser(bool firstUser)
{
  m_addExistUserIsSelected = true;
  m_createNewtUserIsSelected = false;

  CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnAddExistUser - Enter function. [m_editUserButtonWasHit=%d][m_addUserButtonWasHit=%d] (login)",m_editUserButtonWasHit,m_addUserButtonWasHit);

  m_unsGetterProcessor.Stop();

  // Set m_username, m_password and m_rememberPassword from profile
  // in order to show then in the edit fields.
  m_username = "";
  m_password = "";
  m_rememberPassword = true;
  m_needToEncodePassword = true;
  m_userSelectedIndex = NEW_USER_PROFILE_INDEX;

  if (firstUser)
  {
    ((CGUILabelControl*)GetControl(CONTROL_USER_NAME_TITLE))->SetLabel(g_localizeStrings.Get(53403));
  }
  else
  {
    ((CGUILabelControl*)GetControl(CONTROL_USER_NAME_TITLE))->SetLabel(g_localizeStrings.Get(53404));
  }
  SET_CONTROL_VISIBLE(CONTROL_USER_NAME_TITLE);

  ((CGUILabelControl*)GetControl(CONTROL_USER_NAME_LABEL))->SetLabel(m_username);
  ((CGUIEditControl*)GetControl(CONTROL_USER_NAME_EDIT))->SetLabel2(m_username);
  SET_CONTROL_VISIBLE(CONTROL_USER_NAME_EDIT);
  SET_CONTROL_HIDDEN(CONTROL_USER_NAME_LABEL);
  ((CGUIEditControl*)GetControl(CONTROL_PASSWORD_EDIT))->SetLabel2(m_password);
  ((CGUIRadioButtonControl*) GetControl(CONTROL_REMEMBER_PASSWORD_RADIO_BUTTON))->SetSelected(m_rememberPassword);

  SET_CONTROL_VISIBLE(CONTROL_EU_GROUP);
  SET_CONTROL_HIDDEN(CONTROL_CU_GROUP);

  SET_CONTROL_HIDDEN(CONTROL_BIG_LIST);
  SET_CONTROL_HIDDEN(CONTROL_PROFILE_EDIT_BUTTON);

  SET_CONTROL_VISIBLE(CONTROL_USER_PASSWORD_GROUP);

  SET_CONTROL_HIDDEN(CONTROL_REMOVE_USER_BUTTON);

  int numOfExistProfiles = g_settings.m_vecProfiles.size();
  if(numOfExistProfiles < 2)
  {
    SET_CONTROL_HIDDEN(CONTROL_CANCEL_BUTTON);
  }
  else
  {
    SET_CONTROL_VISIBLE(CONTROL_CANCEL_BUTTON);
  }

  CONTROL_ENABLE(CONTROL_USER_TYPE_GROUP);
  SET_CONTROL_SELECTED(GetID(), CONTROL_USER_TYPE_EXIST, true);
  SET_CONTROL_SELECTED(GetID(), CONTROL_USER_TYPE_NEW, false);

  SET_CONTROL_FOCUS(CONTROL_USER_TYPE_EXIST,0);

  m_validEmail = "";
  m_validEmailPrefix = "";
  m_isNewUserEmailValid = false;
  m_validPassword = "";
  m_isNewUserPasswordValid = false;
  m_needToGetUsernameFromServer = false;

  m_isNewUserRetypePasswordMatch = false;

  m_isUsernameSuggestionListValid = false;

  SetProperty("email-is-valid", false);
  SetProperty("password-is-valid", false);
  SetProperty("retype-password-is-valid", false);

  usernameSuggestionList.Clear();
  m_selectedUsername = "";

  CGUIEditControl* newUserEmailEditControl = (CGUIEditControl*)GetControl(CONTROL_CU_EMAIL_EDIT);
  if (newUserEmailEditControl)
  {
    newUserEmailEditControl->SetLabel2("");
  }

  CGUIEditControl* newUserPasswordEditControl = (CGUIEditControl*)GetControl(CONTROL_CU_PASSWORD_EDIT);
  if (newUserPasswordEditControl)
  {
    newUserPasswordEditControl->SetLabel2("");
  }

  CGUIEditControl* newUserRetypePasswordEditControl = (CGUIEditControl*)GetControl(CONTROL_CU_RETYPE_PASSWORD_EDIT);
  if (newUserRetypePasswordEditControl)
  {
    newUserRetypePasswordEditControl->SetLabel2("");
  }

  return true;
}

bool CGUIWindowLoginScreen::OnEditButton()
{
  m_editUserButtonWasHit = true;
  m_addUserButtonWasHit = false;

  m_userSelectedIndex = (m_viewControl.GetSelectedItem() + 1); // +1 is in order to jump over Master User

  if(m_userSelectedIndex >= (int)g_settings.m_vecProfiles.size())
  {
    CLog::Log(LOGERROR,"CGUIWindowLoginScreen::OnEditButton - Could not find profile for [m_userSelectedIndex=%d]. [VecProfilesSize=%d] (login)",m_userSelectedIndex,(int)g_settings.m_vecProfiles.size());
    m_userSelectedIndex = NEW_USER_PROFILE_INDEX;
    return false;
  }

  CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnEditButton - [m_userSelectedIndex=%d][m_editUserButtonWasHit=%d][m_addUserButtonWasHit=%d] (login)",m_userSelectedIndex,m_editUserButtonWasHit,m_addUserButtonWasHit);

  // Set m_username, m_password and m_rememberPassword from profile
  // in order to show then in the edit fields.
  m_username = g_settings.m_vecProfiles[m_userSelectedIndex].getID();
  m_password = g_settings.m_vecProfiles[m_userSelectedIndex].getLockCode();
  if(m_password.Equals("-"))
  {
    m_password = "";
    m_needToEncodePassword = true;
    m_rememberPassword = false;
  }
  else
  {
    m_rememberPassword = true;
  }

  CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnEditButton - After set [m_username=%s][m_password=%s][m_rememberPassword=%d]. [m_userSelectedIndex=%d] (login)",m_username.c_str(),m_password.c_str(),m_rememberPassword,m_userSelectedIndex);

  ((CGUILabelControl*)GetControl(CONTROL_USER_NAME_TITLE))->SetLabel(g_settings.m_vecProfiles[m_userSelectedIndex].getName());
  SET_CONTROL_VISIBLE(CONTROL_USER_NAME_TITLE);

  ((CGUILabelControl*)GetControl(CONTROL_USER_NAME_LABEL))->SetLabel(m_username);
  ((CGUIEditControl*)GetControl(CONTROL_USER_NAME_EDIT))->SetLabel2(m_username);
  SET_CONTROL_VISIBLE(CONTROL_USER_NAME_LABEL);
  SET_CONTROL_HIDDEN(CONTROL_USER_NAME_EDIT);
  ((CGUIEditControl*)GetControl(CONTROL_PASSWORD_EDIT))->SetLabel2(m_password);
  ((CGUIRadioButtonControl*) GetControl(CONTROL_REMEMBER_PASSWORD_RADIO_BUTTON))->SetSelected(m_rememberPassword);

  CGUIBaseContainer* pControl = (CGUIBaseContainer*)GetControl(CONTROL_USER_PASSWORD_GROUP);
  if(pControl)
  {
    pControl->AllocResources();
    pControl->SetPosition(0,0);
  }

  SET_CONTROL_HIDDEN(CONTROL_BIG_LIST);
  SET_CONTROL_HIDDEN(CONTROL_PROFILE_EDIT_BUTTON);
  SET_CONTROL_HIDDEN(CONTROL_CU_GROUP);
  SET_CONTROL_VISIBLE(CONTROL_EU_GROUP);

  SET_CONTROL_SELECTED(GetID(), CONTROL_USER_TYPE_EXIST, true);
  SET_CONTROL_SELECTED(GetID(), CONTROL_USER_TYPE_NEW, false);
  CONTROL_DISABLE(CONTROL_USER_TYPE_GROUP);

  SET_CONTROL_FOCUS(CONTROL_PASSWORD_EDIT,0);
  SET_CONTROL_VISIBLE(CONTROL_USER_PASSWORD_GROUP);
  SET_CONTROL_VISIBLE(CONTROL_CANCEL_BUTTON);

  SET_CONTROL_VISIBLE(CONTROL_REMOVE_USER_BUTTON);

  return true;
}

bool CGUIWindowLoginScreen::OnLoginButton()
{
  bool loginSucceeded = false;

  CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnLoginButton - Enter function. [m_editUserButtonWasHit=%d][m_addUserButtonWasHit=%d] (login)",m_editUserButtonWasHit,m_addUserButtonWasHit);

  if(IsThereEmptyfieldsForLogin())
  {
    // Error logs was written in IsThereEmptyfieldsForLogin()
    return false;
  }

  bool addingFirstUser = false;

  if(m_addUserButtonWasHit)
  {
    if((int)g_settings.m_vecProfiles.size() == 1)
    {
      // Case that we are adding the first user
      addingFirstUser = true;

      if (!g_application.IsConnectedToInternet())
      {
        CLog::Log(LOGERROR,"CGUIWindowLoginScreen::OnLoginButton - Can't login user because [IsConnectedToInternet=%d] (login)",g_application.IsConnectedToInternet());
        CGUIDialogOK2::ShowAndGetInput(53743, 53744);
        return false;
      }
    }

    // Case of login button for add new user

    CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnLoginButton - Going to call DoLoginNewUser() (login)");

    loginSucceeded = DoLoginNewUser();

    CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnLoginButton - Call DoLoginNewUser() returned [loginSucceeded=%d] (login)",loginSucceeded);
  }
  else if (m_editUserButtonWasHit)
  {
    // Case of login button for register user

    CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnLoginButton - Going to call DoLoginExistUserFromLoginButton(). [m_userSelectedIndex=%d] (login)",m_userSelectedIndex);

    loginSucceeded = DoLoginExistUserFromLoginButton();

    CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnLoginButton - Call DoLoginExistUserFromLoginButton() returned [loginSucceeded=%d]. [m_userSelectedIndex=%d] (login)",loginSucceeded,m_userSelectedIndex);
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIWindowLoginScreen::OnLoginButton - No case for [m_editUserButtonWasHit=%d][m_addUserButtonWasHit=%d]. Going to exit and return [loginSucceeded=%d] (login)",m_editUserButtonWasHit,m_addUserButtonWasHit,loginSucceeded);
  }

  if(loginSucceeded)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnLoginButton - Login Succeeded [loginSucceeded=%d] -> Going to call FinishSuccessfulLogin() (login)",loginSucceeded);

    g_application.GetBoxeeLoginManager().FinishSuccessfulLogin(addingFirstUser);
  }

  return loginSucceeded;
}

bool CGUIWindowLoginScreen::OnCancelButton()
{
  m_username = "";
  m_password = "";
  m_rememberPassword = false;
  m_needToEncodePassword = false;
  m_editUserButtonWasHit = false;
  m_addUserButtonWasHit = false;

  m_validEmail = "";
  m_validEmailPrefix = "";
  m_isNewUserEmailValid = false;
  m_validPassword = "";
  m_isNewUserPasswordValid = false;
  m_needToGetUsernameFromServer = false;

  m_isNewUserRetypePasswordMatch = false;

  m_isUsernameSuggestionListValid = false;

  SetProperty("email-is-valid", false);
  SetProperty("password-is-valid", false);
  SetProperty("retype-password-is-valid", false);

  usernameSuggestionList.Clear();
  m_selectedUsername = "";

  CGUIEditControl* newUserEmailEditControl = (CGUIEditControl*)GetControl(CONTROL_CU_EMAIL_EDIT);
  if (newUserEmailEditControl)
  {
    newUserEmailEditControl->SetLabel2("");
  }

  CGUIEditControl* newUserPasswordEditControl = (CGUIEditControl*)GetControl(CONTROL_CU_PASSWORD_EDIT);
  if (newUserPasswordEditControl)
  {
    newUserPasswordEditControl->SetLabel2("");
  }

  CGUIEditControl* newUserRetypePasswordEditControl = (CGUIEditControl*)GetControl(CONTROL_CU_RETYPE_PASSWORD_EDIT);
  if (newUserRetypePasswordEditControl)
  {
    newUserRetypePasswordEditControl->SetLabel2("");
  }

  SET_CONTROL_VISIBLE(CONTROL_BIG_LIST);
  SET_CONTROL_VISIBLE(CONTROL_PROFILE_EDIT_BUTTON);
  SET_CONTROL_HIDDEN(CONTROL_USER_PASSWORD_GROUP);
  SET_CONTROL_HIDDEN(CONTROL_CANCEL_BUTTON);

  m_viewControl.SetFocused();

  return true;
}

bool CGUIWindowLoginScreen::OnRemoveUserButton()
{
  CStdString username = ((CGUIEditControl*)GetControl(CONTROL_USER_NAME_EDIT))->GetLabel2();

  CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnRemoveUserButton - Enter function. [m_userSelectedIndex=%d][username=%s] (login)",m_userSelectedIndex,username.c_str());

  bool succeeded = g_settings.DeleteProfile(m_userSelectedIndex);

  if (succeeded)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnRemoveUserButton - User [username=%s] was successfully removed (login)",username.c_str());
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIWindowLoginScreen::OnRemoveUserButton - FAILED to remove user [username=%s] (login)",username.c_str());
  }

  CGUIMessage msg(GUI_MSG_UPDATE, GetID(), 0);
  OnMessage(msg);

  return succeeded;
}

bool CGUIWindowLoginScreen::OnCreateUserDoneButton(int& profileId, bool& isLogin)
{
  if (!g_application.IsConnectedToInternet())
  {
    CLog::Log(LOGERROR,"CGUIWindowLoginScreen::OnCreateUserDoneButton - Can't create new user [username=%s] because [IsConnectedToInternet=%d] (cnu)",m_selectedUsername.c_str(),g_application.IsConnectedToInternet());
    CGUIDialogOK2::ShowAndGetInput(53743, 53744);
    return false;
  }

  CStdString strUrl = BXConfiguration::GetInstance().GetStringParam("Boxee.Create.User","https://secure.boxee.tv/api/register");

  ListHttpHeaders headers;
  headers.push_back("Content-Type: text/xml");

  BXXMLDocument doc;
  doc.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());

  CStdString postData;
  BuildCreateNewUserOnLoginPostData(postData);

  CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnCreateUserDoneButton - Going to create new user. [postData=%s] (cnu)",postData.c_str());

  bool retPost = false;
  retPost = doc.LoadFromURL(strUrl,headers,postData);

  if (!retPost)
  {
    CLog::Log(LOGERROR,"CGUIWindowLoginScreen::OnCreateUserDoneButton - FAILED to create new user [username=%s]. Request from server returned FALSE (cnu)",m_selectedUsername.c_str());
    CGUIDialogOK2::ShowAndGetInput(53701, 53455);
    return false;
  }

  BXObject newUserObj(false);
  CStdString errorMessage = "";

  if (!ParseServerCreatedNewUserResponse(doc, isLogin, newUserObj, errorMessage))
  {
    if (errorMessage.IsEmpty())
    {
      CGUIDialogOK2::ShowAndGetInput(53701, 53456);
    }
    else
    {
      CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701), errorMessage);
    }

    return false;
  }

  CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnCreateUserDoneButton - New user [username=%s] was created (cnu)",m_selectedUsername.c_str());

  if (!CreateNewUserProfile(newUserObj, profileId))
  {
    CLog::Log(LOGERROR,"CGUIWindowLoginScreen::OnCreateUserDoneButton - FAILED to create profile for new user [username=%s] (cnu)",m_selectedUsername.c_str());
    CGUIDialogOK2::ShowAndGetInput(53701, 53456);
    return false;
  }

  CProfile& p = g_settings.m_vecProfiles[profileId];

  BXCredentials credentials;
  credentials.SetUserName(p.getName());
  credentials.SetPassword(p.getLastLockCode());
  BOXEE::Boxee::GetInstance().PostLoginInitializations(credentials,newUserObj);

  CGUIDialogOK2::ShowAndGetInput(53453, 53454);

  return true;
}

CStdString CGUIWindowLoginScreen::EncodePassword(CStdString password)
{
  XBMC::MD5 md5;
  unsigned char md5hash[16];
  md5.append((unsigned char *)password.c_str(), (int)password.size());
  md5.getDigest(md5hash);

  return CBase64::Encode(md5hash, 16);
}

bool CGUIWindowLoginScreen::IsThereEmptyfieldsForLogin()
{
  CStdString username = ((CGUIEditControl*)GetControl(CONTROL_USER_NAME_EDIT))->GetLabel2();

  if(username.IsEmpty())
  {
    // Case of empty UserName field

    CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::IsThereEmptyfieldsForLogin - [username=%s] is empty (login)",username.c_str());
    CGUIDialogOK::ShowAndGetInput(20068,51058,20022,20022);
    SET_CONTROL_FOCUS(CONTROL_USER_NAME_EDIT,0);
    return true;
  }

  CStdString password = ((CGUIEditControl*)GetControl(CONTROL_PASSWORD_EDIT))->GetLabel2();

  if(password.IsEmpty())
  {
    // Case of empty Password field

    CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::IsThereEmptyfieldsForLogin - [password=%s] is empty (login)",password.c_str());
    CGUIDialogOK::ShowAndGetInput(20068,51059,20022,20022);
    SET_CONTROL_FOCUS(CONTROL_PASSWORD_EDIT,0);
    return true;
  }

  return false;
}

bool CGUIWindowLoginScreen::IsEmailValid(const CStdString& email)
{
  if (email.length() > 45)
  {
    return false;
  }

  //CStdString strToCheck = "*" + email + "*";
  CStdString strToCheck = email;
  strToCheck.ToLower();

  CStdString strRegExpr = EMAIL_REGEXP_PATTERN;

  CRegExp reg;
  reg.RegComp(strRegExpr);
  int pos = reg.RegFind(strToCheck);

  if (pos != -1)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool CGUIWindowLoginScreen::IsCreateNewUserPasswordValid(const CStdString& password)
{
  if (password.length() < 6 || password.length() > 45)
  {
    return false;
  }

  return true;
}

void CGUIWindowLoginScreen::processCreateNewUser()
{
  m_checkNewUserEmailDelay++;

  if (m_checkNewUserEmailDelay > CHECK_CREATE_USER_EDITBOX_DELAY)
  {
    m_checkNewUserEmailDelay = 0;

    CGUIEditControl* newUserEmailEditControl = (CGUIEditControl*)GetControl(CONTROL_CU_EMAIL_EDIT);
    if (newUserEmailEditControl)
    {
      CStdString email = newUserEmailEditControl->GetLabel2();

      if (!m_validEmail.Equals(email))
      {
        if (!email.IsEmpty() && IsEmailValid(email))
        {
            m_validEmail = email;
            m_isNewUserEmailValid = true;
            m_needToGetUsernameFromServer = true;
            SetProperty("email-is-valid", true);

            m_needToGetUsernameFromServer = false;

            int pos = m_validEmail.Find("@");
            if (pos != (-1))
            {
              CStdString emailPrefix = m_validEmail.substr(0,pos);

              if (!m_validEmailPrefix.Equals(emailPrefix))
              {
                m_validEmailPrefix = emailPrefix;

                // new prefix was found -> need to get new username Suggestions from server
                GetUsernameSuggestionListFromServer(m_validEmailPrefix);
              }
              else
              {
                if (usernameSuggestionList.Size() > 0)
                {
                  SET_CONTROL_VISIBLE(CONTROL_CU_USERNAME__LABEL);
                  SET_CONTROL_VISIBLE(CONTROL_CU_USERNAME_DROPDOWN);

                  m_selectedUsername = (usernameSuggestionList[0])->GetLabel();
                  SetProperty("choosen-username",m_selectedUsername);

                  m_isUsernameSuggestionListValid = true;

                  CONTROL_ENABLE(CONTROL_CU_PASSWORD_EDIT);
                  CONTROL_ENABLE(CONTROL_CU_RETYPE_PASSWORD_EDIT);
                }
              }
            }

            if (m_isNewUserPasswordValid && m_isNewUserRetypePasswordMatch && m_isUsernameSuggestionListValid)
            {
              CONTROL_ENABLE(CONTROL_CU_DONE_BUTTON);
            }
        }
        else
        {
          if (m_isNewUserEmailValid)
          {
            // email valid status was change

            m_validEmail = "";
            m_validEmailPrefix = "";
            m_isNewUserEmailValid = false;
            SetProperty("email-is-valid", false);
            SetProperty("choosen-username","");

            CONTROL_DISABLE(CONTROL_CU_DONE_BUTTON);
            CONTROL_DISABLE(CONTROL_CU_PASSWORD_EDIT);
            CONTROL_DISABLE(CONTROL_CU_RETYPE_PASSWORD_EDIT);
            SET_CONTROL_HIDDEN(CONTROL_CU_USERNAME__LABEL);
            SET_CONTROL_HIDDEN(CONTROL_CU_USERNAME_DROPDOWN);
            m_isUsernameSuggestionListValid = false;
          }
        }
      }
    }
  }

  if (m_isNewUserEmailValid)
  {
    m_checkNewUserPasswordDelay++;

    if (m_checkNewUserPasswordDelay > CHECK_CREATE_USER_EDITBOX_DELAY)
    {
      m_checkNewUserPasswordDelay = 0;

      CGUIEditControl* newUserPasswordEditControl = (CGUIEditControl*)GetControl(CONTROL_CU_PASSWORD_EDIT);
      if (newUserPasswordEditControl)
      {
        CStdString password = newUserPasswordEditControl->GetLabel2();

        if (!m_validPassword.Equals(password))
        {
          if (!password.IsEmpty() && IsCreateNewUserPasswordValid(password))
          {
            m_validPassword = password;

            m_isNewUserPasswordValid = true;
            SetProperty("password-is-valid", true);

            CONTROL_ENABLE(CONTROL_CU_RETYPE_PASSWORD_EDIT);
          }
          else
          {
            if (m_isNewUserPasswordValid)
            {
              m_validPassword = "";

              m_isNewUserPasswordValid = false;
              SetProperty("password-is-valid", false);
              SetProperty("retype-password-is-valid", false);

              CONTROL_DISABLE(CONTROL_CU_RETYPE_PASSWORD_EDIT);
              CONTROL_DISABLE(CONTROL_CU_DONE_BUTTON);
            }
          }
        }
      }
    }
  }

  if (m_isNewUserEmailValid && m_isNewUserPasswordValid)
  {
    m_checkNewUserRetypePasswordDelay++;

    if (m_checkNewUserRetypePasswordDelay > CHECK_CREATE_USER_EDITBOX_DELAY)
    {
      m_checkNewUserRetypePasswordDelay = 0;

      CGUIEditControl* newUserRetypePasswordEditControl = (CGUIEditControl*)GetControl(CONTROL_CU_RETYPE_PASSWORD_EDIT);
      if (newUserRetypePasswordEditControl)
      {
        CStdString retypePassword = newUserRetypePasswordEditControl->GetLabel2();

        if (retypePassword.Equals(m_validPassword))
        {
          m_isNewUserRetypePasswordMatch = true;
          SetProperty("retype-password-is-valid", true);

          if (m_isUsernameSuggestionListValid)
          {
            CONTROL_ENABLE(CONTROL_CU_DONE_BUTTON);
          }
        }
        else
        {
          m_isNewUserRetypePasswordMatch = false;
          SetProperty("retype-password-is-valid", false);
          CONTROL_DISABLE(CONTROL_CU_DONE_BUTTON);
        }
      }
    }
  }
}

void CGUIWindowLoginScreen::GetUsernameSuggestionListFromServer(CStdString emailPrefix)
{
  SET_CONTROL_HIDDEN(CONTROL_CU_USERNAME__LABEL);
  SET_CONTROL_HIDDEN(CONTROL_CU_USERNAME_DROPDOWN);

  SetProperty("loading", true);
  m_unsGetterProcessor.QueueJob(new BXUsernameSuggestionJob(emailPrefix, this));
}

////////////////////////////////
// BXUsernameSuggestionJob //
////////////////////////////////

CGUIWindowLoginScreen::BXUsernameSuggestionJob::BXUsernameSuggestionJob(const CStdString& emailPrefix, CGUIWindowLoginScreen* callback) : BXBGJob("BXUsernameSuggestionJob")
{
  m_emailPrefix = emailPrefix;
  m_callback = callback;
}

CGUIWindowLoginScreen::BXUsernameSuggestionJob::~BXUsernameSuggestionJob()
{

}

void CGUIWindowLoginScreen::BXUsernameSuggestionJob::DoWork()
{
  CLog::Log(LOGDEBUG,"BXUsernameSuggestionJob::DoWork - Enter function. [m_emailPrefix=%s] (cnu)",m_emailPrefix.c_str());

  if (m_emailPrefix.IsEmpty())
  {
    m_callback->SetProperty("loading", false);
    CLog::Log(LOGERROR,"BXUsernameSuggestionJob::DoWork - Email prefix is empty. [%s] (cnu)",m_emailPrefix.c_str());
    return;
  }

  CStdString strLink = BXConfiguration::GetInstance().GetStringParam("Boxee.Username.Suggestions","http://app.boxee.tv/api/checkid?userid=");
  strLink += m_emailPrefix;
  strLink += "&suggest";

  CStdString strHtml;
  if (m_http.Get(strLink,strHtml,false))
  {
    CLog::Log(LOGDEBUG,"BXUsernameSuggestionJob::DoWork - Going to parse server response for [%s] (cnu)", strLink.c_str());

    CFileItemList usernameList;
    if (!ParseUsernameSuggestionXml(strHtml, usernameList))
    {
      // ERROR log will be written from ParseUsernameSuggestionXml()
      m_callback->SetProperty("loading", false);
      return;
    }

    CFileItemList* pUsernameList = new CFileItemList();
    pUsernameList->Copy(usernameList);

    CGUIMessage msg(GUI_MSG_LABEL_BIND, m_callback->GetID(), 0, 0, 0, pUsernameList);
    g_windowManager.SendThreadMessage(msg, m_callback->GetID());
  }
  else
  {
    CLog::Log(LOGERROR,"BXUsernameSuggestionJob::DoWork - FAILED to get response from server for [%s] (cnu)", strLink.c_str());
    m_callback->SetProperty("loading", false);
  }
}

bool CGUIWindowLoginScreen::BXUsernameSuggestionJob::ParseUsernameSuggestionXml(const CStdString& strHtml, CFileItemList& items)
{
  TiXmlDocument xmlDoc;
  xmlDoc.Parse(strHtml);

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (!pRootElement)
  {
    CLog::Log(LOGERROR,"BXUsernameSuggestionJob::ParseUsernameSuggestionXml - FAILED to get root element (cnu)");
    return false;
  }

  if (strcmp(pRootElement->Value(),"boxee") != 0)
  {
    CLog::Log(LOGERROR,"BXUsernameSuggestionJob::ParseUsernameSuggestionXml - Root element isn't <boxee>. <%s> (cnu)",pRootElement->Value());
    return false;
  }

  CStdString username;
  int counter = 0;

  const TiXmlNode* pTag = NULL;
  while ((pTag = pRootElement->IterateChildren(pTag)))
  {
    counter++;

    if (pTag->ValueStr() == EXISTS_XML_ELEMENT)
    {
      const TiXmlNode* pValue = pTag->FirstChild();

      if (pValue)
      {
        CStdString existValue = pValue->ValueStr();
        CLog::Log(LOGDEBUG,"BXUsernameSuggestionJob::ParseUsernameSuggestionXml - [%d] - <exist=%s> for [m_emailPrefix=%s] (cnu)",counter,existValue.c_str(),m_emailPrefix.c_str());

        if (existValue.Equals("0"))
        {
          username = m_emailPrefix;
        }
      }
    }
    else if (pTag->ValueStr() == SUGGESTION_XML_ELEMENT)
    {
      const TiXmlNode* pValue = pTag->FirstChild();

      if (pValue && !pValue->ValueStr().empty())
      {
        username = pValue->ValueStr();
        CLog::Log(LOGDEBUG,"BXUsernameSuggestionJob::ParseUsernameSuggestionXml - [%d] - Got suggestion [%s] for [emailPrefix=%s] (cnu)",counter,username.c_str(),m_emailPrefix.c_str());
      }
    }

    if (!username.IsEmpty())
    {
      CLog::Log(LOGDEBUG,"BXUsernameSuggestionJob::ParseUsernameSuggestionXml - [%d] - Going to add [%s] for [emailPrefix=%s] (cnu)",counter,username.c_str(),m_emailPrefix.c_str());

      CFileItemPtr usernameItem(new CFileItem(username));
      usernameItem->SetProperty("value",username);
      items.Add(usernameItem);

      username = "";
    }
  }

  return true;
}

bool CGUIWindowLoginScreen::CreateNewUserProfile(const BXObject& newUserObj, int& profileId)
{
  if (!newUserObj.IsValid())
  {
    CLog::Log(LOGERROR,"BXUsernameSuggestionJob::CreateNewUserProfile - Enter function with a INVALID user object. [username=%s] (cnu)",m_selectedUsername.c_str());
    return false;
  }

  profileId = g_application.GetBoxeeLoginManager().CreateProfile(newUserObj.GetID(), newUserObj);

  if (profileId > 0)
  {
    g_settings.LoadProfile(profileId);

    CStdString password = m_validPassword;
    password = EncodePassword(password);
    g_application.GetBoxeeLoginManager().UpdateProfile(profileId,password,true);

    CLog::Log(LOGDEBUG,"BXUsernameSuggestionJob::CreateNewUserProfile - After create profile for [username=%s]. [profileId=%d] (cnu)",m_selectedUsername.c_str(),profileId);

    return true;
  }
  else
  {
    CLog::Log(LOGERROR,"BXUsernameSuggestionJob::CreateNewUserProfile - FAILED to create profile for [username=%s]. [profileId=%d] (cnu)",m_selectedUsername.c_str(),profileId);
    return false;
  }
}

void CGUIWindowLoginScreen::BuildCreateNewUserOnLoginPostData(CStdString& postData)
{
  postData = "<boxee><register>";
  postData += "<email>";
  postData += m_validEmail;
  postData += "</email>";

  postData += "<password>";
  postData += m_validPassword;
  postData += "</password>";

  postData += "<username>";
  postData += m_selectedUsername;
  postData += "</username>";

  postData += "<first_name>";
  postData += m_selectedFirstName;
  postData += "</first_name>";

  postData += "<last_name>";
  postData += m_selectedFirstName;
  postData += "</last_name>";

  postData += "</register></boxee>";
}

bool CGUIWindowLoginScreen::ParseServerCreatedNewUserResponse(BXXMLDocument& doc, bool& isLogin, BXObject& newUserObj, CStdString& errorMessage)
{
  const TiXmlElement* pRootElement = doc.GetDocument().RootElement();
  if (!pRootElement)
  {
    CLog::Log(LOGERROR,"CGUIWindowLoginScreen::ParseServerCreatedNewUserResponse - FAILED to get root element (cnu)");
    return false;
  }

  if (strcmp(pRootElement->Value(),BOXEE_XML_ELEMENT) != 0)
  {
    CLog::Log(LOGERROR,"CGUIWindowLoginScreen::ParseServerCreatedNewUserResponse - Root element isn't <boxee>. <%s> (cnu)",pRootElement->Value());
    return false;
  }

  bool foundSucceessElement = false;
  bool isSucceess = false;

  const TiXmlNode* pTag = NULL;
  while ((pTag = pRootElement->IterateChildren(pTag)))
  {
    CStdString elementName = pTag->ValueStr();

    if (elementName == SUCCESS_XML_ELEMENT)
    {
      ///////////////////////
      // case of <success> //
      ///////////////////////

      foundSucceessElement = true;

      const TiXmlNode* pValue = pTag->FirstChild();

      if (pValue)
      {
        CStdString succeessValue = pValue->ValueStr();

        if (succeessValue.Equals("1"))
        {
          CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::ParseServerCreatedNewUserResponse - Server returned <success=%s=1> for [SelectedUsername=%s] (cnu)",succeessValue.c_str(),m_selectedUsername.c_str());
          isSucceess = true;
        }
        else
        {
          CLog::Log(LOGERROR,"CGUIWindowLoginScreen::ParseServerCreatedNewUserResponse - Server returned <success=%s> for [SelectedUsername=%s] (cnu)",succeessValue.c_str(),m_selectedUsername.c_str());
        }
      }
      else
      {
        CLog::Log(LOGERROR,"CGUIWindowLoginScreen::ParseServerCreatedNewUserResponse - FAILED to get <success> element value. [SelectedUsername=%s] (cnu)",m_selectedUsername.c_str());
      }
    }
    else if (elementName == LOGIN_XML_ELEMENT)
    {
      /////////////////////
      // case of <login> //
      /////////////////////

      if (!foundSucceessElement)
      {
        CLog::Log(LOGERROR,"CGUIWindowLoginScreen::ParseServerCreatedNewUserResponse - There wasn't <success> before <login> for [SelectedUsername=%s] (cnu)",m_selectedUsername.c_str());
        return false;
      }

      const TiXmlNode* pValue = pTag->FirstChild();

      if (pValue)
      {
        CStdString loginValue = pValue->ValueStr();

        if (loginValue.Equals("1"))
        {
          CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::ParseServerCreatedNewUserResponse - Server returned <login=%s=1> for [SelectedUsername=%s] (cnu)",loginValue.c_str(),m_selectedUsername.c_str());
          isLogin = true;
        }
        else
        {
          CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::ParseServerCreatedNewUserResponse - Server returned <login=%s> for [SelectedUsername=%s] (cnu)",loginValue.c_str(),m_selectedUsername.c_str());
        }
      }
      else
      {
        CLog::Log(LOGERROR,"CGUIWindowLoginScreen::ParseServerCreatedNewUserResponse - FAILED to get <login> element value. [SelectedUsername=%s] (cnu)",m_selectedUsername.c_str());
        return false;
      }
    }
    else if (elementName == OBJECT_XML_ELEMENT)
    {
      ///////////////////////
      // case of <object> //
      ///////////////////////

      if (!isSucceess)
      {
        CLog::Log(LOGERROR,"CGUIWindowLoginScreen::ParseServerCreatedNewUserResponse - <object> element won't be parse because server FAILED to create new user. [SelectedUsername=%s] (cnu)",m_selectedUsername.c_str());
        return false;
      }

      newUserObj.FromXML(pTag);
      newUserObj.SetValid(true);
    }
    else if (elementName == ERRORS_XML_ELEMENT)
    {
      ///////////////////////
      // case of <errors> //
      ///////////////////////

      if (isSucceess)
      {
        CLog::Log(LOGERROR,"CGUIWindowLoginScreen::ParseServerCreatedNewUserResponse - Received <errors> element while <success=%d>. [SelectedUsername=%s] (cnu)",isSucceess,m_selectedUsername.c_str());
        return false;
      }

      const TiXmlNode* pErrorTag = NULL;
      while ((pErrorTag = pTag->IterateChildren(pErrorTag)))
      {
        const TiXmlNode* pErrorValue = pErrorTag->FirstChild();

        if (pErrorValue)
        {
          errorMessage = pErrorValue->ValueStr();
          CLog::Log(LOGERROR,"CGUIWindowLoginScreen::ParseServerCreatedNewUserResponse - Parse error message [%s]. [SelectedUsername=%s] (cnu)",errorMessage.c_str(),m_selectedUsername.c_str());
          return false;
        }
      }

      CLog::Log(LOGERROR,"CGUIWindowLoginScreen::ParseServerCreatedNewUserResponse - FAILED to parse any error message [%s]. [SelectedUsername=%s] (cnu)",errorMessage.c_str(),m_selectedUsername.c_str());
      return false;
    }
  }

  return true;
}

void CGUIWindowLoginScreen::OnGetDropdownLabel(CStdString& label)
{
  CStdString customUsername;
  if (CGUIDialogKeyboard::ShowAndGetNewUsername(customUsername))
  {
    if (!customUsername.IsEmpty())
    {
      label = customUsername;
      CLog::Log(LOGDEBUG,"CGUIWindowLoginScreen::OnGetDropdownLabel - Castom username [%s] was set (cnu)",label.c_str());
    }
    else
    {
      CLog::Log(LOGERROR,"CGUIWindowLoginScreen::OnGetDropdownLabel - An empty username was chose (cnu)");
    }
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIWindowLoginScreen::OnGetDropdownLabel - ERROR returned from Keyboard (cnu)");
  }
}

bool CGUIWindowLoginScreen::ShowTOU()
{
  if (m_touStr.IsEmpty())
  {
    CGUIDialogProgress* progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      progress->StartModal();
      progress->Progress();
    }

    // verify the username is available
    CStdString strUrl = BXConfiguration::GetInstance().GetStringParam("Boxee.Term.Of.Use","http://app.boxee.tv/api/getlegal?page=tou");
    BXCurl curl;

    m_touStr = curl.HttpGetString(strUrl, false);

    if (progress)
    {
      progress->Close();
    }

    if (m_touStr.IsEmpty())
    {
      CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(53432));
      return true;
    }
  }

  CGUIDialogBoxeeMessageScroll::ShowAndGetInput(g_localizeStrings.Get(53436),m_touStr);
  return true;
}

bool CGUIWindowLoginScreen::ShowPrivacyTerm()
{
  if (m_privacyTermStr.IsEmpty())
  {
    CGUIDialogProgress* progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      progress->StartModal();
      progress->Progress();
    }

    CStdString strUrl = BXConfiguration::GetInstance().GetStringParam("Boxee.Privacy.Policy","http://app.boxee.tv/api/getlegal?page=privacy");
    BXCurl curl;

    m_privacyTermStr = curl.HttpGetString(strUrl, false);

    if (progress)
    {
      progress->Close();
    }

    if (m_privacyTermStr.IsEmpty())
    {
      CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(53433));
      return true;
    }
  }

  CGUIDialogBoxeeMessageScroll::ShowAndGetInput(g_localizeStrings.Get(53438),m_privacyTermStr);
  return true;
}

