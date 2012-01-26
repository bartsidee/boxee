#include "GUIDialogBoxeeLoginWizardMenuCust.h"
#include "utils/log.h"
#include "GUIDialogProgress.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "lib/libBoxee/bxcurl.h"
#include "GUIDialogOK2.h"
#include "LocalizeStrings.h"
#include "GUITextBox.h"
#include "GUIWindowManager.h"
#include "GUIWindowStateDatabase.h"

#define WEB_BUTTON_CONTROL_ID           8700
#define OWN_BUTTON_CONTROL_ID           8701
#define DONT_KNOW_BUTTON_CONTROL_ID     8702

#define CATEGORY_LOCAL                  "local"
#define CATEGORY_FAVORITES              "favorite"
#define CATEGORY_ALL                    "all"

CGUIDialogBoxeeLoginWizardMenuCust::CGUIDialogBoxeeLoginWizardMenuCust() : CGUIDialogBoxeeWizardBase(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_MENU_CUST,"boxee_login_wizard_menu_cust.xml","CGUIDialogBoxeeLoginWizardMenuCust")
{

}

CGUIDialogBoxeeLoginWizardMenuCust::~CGUIDialogBoxeeLoginWizardMenuCust()
{

}

void CGUIDialogBoxeeLoginWizardMenuCust::OnInitWindow()
{
  CGUIDialogBoxeeWizardBase::OnInitWindow();

  m_tvShowMenuCategory = CATEGORY_ALL;
  m_movieMenuCategory = CATEGORY_ALL;
  m_appMenuCategory = CATEGORY_ALL;
}

bool CGUIDialogBoxeeLoginWizardMenuCust::OnAction(const CAction& action)
{
  if (action.id == ACTION_PREVIOUS_MENU)
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardMenuCust::OnAction - ACTION_PREVIOUS_MENU - can't go back from here (blw)(digwiz)");
    return true;
  }

  return CGUIDialogBoxeeWizardBase::OnAction(action);
}

bool CGUIDialogBoxeeLoginWizardMenuCust::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    HandleClick(message);

    m_actionChoseEnum = CActionChoose::NEXT;
    Close();
    return true;
  }
  break;
  }

  return CGUIDialogBoxeeWizardBase::OnMessage(message);
}

bool CGUIDialogBoxeeLoginWizardMenuCust::HandleClick(CGUIMessage& message)
{
  int iControl = message.GetSenderId();

  switch(iControl)
  {
  case OWN_BUTTON_CONTROL_ID:
  {
    m_tvShowMenuCategory = CATEGORY_LOCAL;
    m_movieMenuCategory = CATEGORY_LOCAL;
    m_appMenuCategory = CATEGORY_FAVORITES;
  }
  break;
  case WEB_BUTTON_CONTROL_ID:
  case DONT_KNOW_BUTTON_CONTROL_ID:
  {
    m_tvShowMenuCategory = CATEGORY_ALL;
    m_movieMenuCategory = CATEGORY_ALL;
    m_appMenuCategory = CATEGORY_ALL;
  }
  break;
  }

  SetUserMenuCust();

  return true;
}

bool CGUIDialogBoxeeLoginWizardMenuCust::HandleClickNext()
{
  return true;
}

bool CGUIDialogBoxeeLoginWizardMenuCust::HandleClickBack()
{
  return true;
}

void CGUIDialogBoxeeLoginWizardMenuCust::SetUserMenuCust()
{
  CGUIWindowStateDatabase stateDB;
  stateDB.SetDefaultCategory(WINDOW_BOXEE_BROWSE_TVSHOWS, m_tvShowMenuCategory);
  stateDB.SetDefaultCategory(WINDOW_BOXEE_BROWSE_MOVIES, m_movieMenuCategory);
  stateDB.SetDefaultCategory(WINDOW_BOXEE_BROWSE_APPS, m_appMenuCategory);
}
