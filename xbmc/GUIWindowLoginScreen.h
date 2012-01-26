#pragma once

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

#include "GUIDialog.h"
#include "GUIViewControl.h"
#include "utils/Stopwatch.h"
#include "PictureThumbLoader.h"
#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxcredentials.h"
#include "FileCurl.h"

#include "GUIDialogBoxeeDropdown.h"

#define LOGIN_STATUS_OK                 0
#define LOGIN_STATUS_ERROR             -1
#define LOGIN_STATUS_CREDENTIALS_ERROR -2
#define LOGIN_STATUS_NETWORK_ERROR     -3

class CFileItemList;
class CGUIEditControl;

class CGUIWindowLoginScreen : public CGUIWindow, public ICustomDropdownLabelCallback
{
public:

  class BXUsernameSuggestionJob : public BOXEE::BXBGJob
  {
  public:
    BXUsernameSuggestionJob(const CStdString& strUsername, CGUIWindowLoginScreen* callback);
    virtual ~BXUsernameSuggestionJob();
    virtual void DoWork();

  private:

    bool ParseUsernameSuggestionXml(const CStdString& strHtml, CFileItemList& items);

    CStdString m_emailPrefix;
    CGUIWindowLoginScreen* m_callback;
    XFILE::CFileCurl m_http;
  };

  CGUIWindowLoginScreen(void);
  virtual ~CGUIWindowLoginScreen(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();
  virtual bool HasListItems() const { return true; };
  virtual CFileItemPtr GetCurrentListItem(int offset = 0);
  int GetViewContainerID() const { return m_viewControl.GetCurrentControl(); };

  static void Show();

  virtual void OnGetDropdownLabel(CStdString& label);

protected:
  virtual void OnInitWindow();
  virtual void OnWindowLoaded();
  void Update();
  void SetLabel(int iControl, const CStdString& strLabel);

  bool DoesThisUserAlreadyExist(const CStdString& Username);
  
  bool DoLoginNewUser();
  bool DoLoginExistUserFromList();
  bool DoLoginExistUserFromLoginButton();
  bool DoLogin(int profileId);

  bool IsThereEmptyfieldsForLogin();

  CStdString EncodePassword(CStdString password);

  bool OnAddUserButton(bool firstUser);
  bool OnAddExistUser(bool firstUser);
  bool OnCreateNewUser();
  bool OnEditButton();
  bool OnLoginButton();
  bool OnCancelButton();
  bool OnRemoveUserButton();
  bool OnCreateUserDoneButton(int& profileId, bool& isLogin);

  bool IsEmailValid(const CStdString& email);
  bool IsCreateNewUserPasswordValid(const CStdString& password);

  void processCreateNewUser();
  void GetUsernameSuggestionListFromServer(CStdString email);

  void BuildCreateNewUserOnLoginPostData(CStdString& postData);

  bool ParseServerCreatedNewUserResponse(BOXEE::BXXMLDocument& doc, bool& isLogin, BOXEE::BXObject& newUserObj, CStdString& errorMessage);
  bool CreateNewUserProfile(const BOXEE::BXObject& newUserObj, int& profileId);

  bool ShowTOU();
  bool ShowPrivacyTerm();

  bool m_editUserButtonWasHit;
  bool m_addUserButtonWasHit;
  CStdString m_username;
  CStdString m_password;
  bool m_rememberPassword;
  bool m_needToEncodePassword;
  int m_userSelectedIndex;

  CGUIViewControl m_viewControl;
  CFileItemList* m_vecItems;

  int m_iSelectedItem;
  CStopWatch watch;

  CPictureThumbLoader m_loader;

  bool m_addExistUserIsSelected;
  bool m_createNewtUserIsSelected;

  CStdString m_validEmail;
  CStdString m_validEmailPrefix;
  bool m_isNewUserEmailValid;
  int m_checkNewUserEmailDelay;
  CStdString m_validPassword;
  bool m_isNewUserPasswordValid;
  int m_checkNewUserPasswordDelay;

  bool m_isNewUserRetypePasswordMatch;
  int m_checkNewUserRetypePasswordDelay;

  CFileItemList usernameSuggestionList;
  CStdString m_selectedUsername;
  bool m_isUsernameSuggestionListValid;

  CStdString m_selectedFirstName;
  CStdString m_selectedLastName;

  bool m_needToGetUsernameFromServer;
  BOXEE::BXBGProcess m_unsGetterProcessor;

  CStdString m_touStr;
  CStdString m_privacyTermStr;

private:
  
  void Load();

};
