#include "GUIDialogBoxeeLoginWizardTOU.h"
#include "utils/log.h"
#include "GUIDialogProgress.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "lib/libBoxee/bxcurl.h"
#include "GUIDialogOK2.h"
#include "LocalizeStrings.h"
#include "GUITextBox.h"
#include "GUIWindowManager.h"
#include "FileItem.h"

#define HIDDEN_CONTAINER                5000

#define BUTTONS_GROUP_CONTROL_ID        8700
#define TEXTBOX_GROUP_CONTROL_ID        8800

#define TOU_BUTTON_CONTROL_ID           8701
#define PP_BUTTON_CONTROL_ID            8702

#define SCROLL_CONTROL_ID               8801
#define TEXTBOX_CONTROL_ID              8802
#define TEXTBOX_TITLE_CONTROL_ID        8803

CGUIDialogBoxeeLoginWizardTOU::CGUIDialogBoxeeLoginWizardTOU() : CGUIDialogBoxeeWizardBase(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_TOU,"boxee_login_wizard_tou.xml","CGUIDialogBoxeeLoginWizardTOU")
{

}

CGUIDialogBoxeeLoginWizardTOU::~CGUIDialogBoxeeLoginWizardTOU()
{

}

void CGUIDialogBoxeeLoginWizardTOU::OnInitWindow()
{
  CGUIDialogBoxeeWizardBase::OnInitWindow();

  SET_CONTROL_HIDDEN(TEXTBOX_GROUP_CONTROL_ID);
  SET_CONTROL_VISIBLE(BUTTONS_GROUP_CONTROL_ID);
}

bool CGUIDialogBoxeeLoginWizardTOU::OnAction(const CAction& action)
{
  if (action.id == ACTION_PREVIOUS_MENU)
  {
    int focusedControlId = GetFocusedControlID();
    if (focusedControlId == SCROLL_CONTROL_ID)
    {
      SET_CONTROL_HIDDEN(TEXTBOX_GROUP_CONTROL_ID);
      SET_CONTROL_VISIBLE(BUTTONS_GROUP_CONTROL_ID);
      SET_CONTROL_FOCUS(BUTTONS_GROUP_CONTROL_ID,0);
      return true;
    }
  }

  return CGUIDialogBoxeeWizardBase::OnAction(action);
}

bool CGUIDialogBoxeeLoginWizardTOU::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    if (HandleClick(message))
    {
      // continue to stay in this screen
      return true;
    }
  }
  break;
  }

  return CGUIDialogBoxeeWizardBase::OnMessage(message);
}

bool CGUIDialogBoxeeLoginWizardTOU::HandleClick(CGUIMessage& message)
{
  int iControl = message.GetSenderId();

  switch(iControl)
  {
  case TOU_BUTTON_CONTROL_ID:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardTOU::HandleClick - handling click on [Control=%d=TOU_BUTTON_CONTROL_ID] (blw)(digwiz)",iControl);
    return HandleClickOnTouButton();
  }
  break;
  case PP_BUTTON_CONTROL_ID:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardTOU::HandleClick - handling click on [Control=%d=PP_BUTTON_CONTROL_ID] (blw)(digwiz)",iControl);
    return HandleClickOnPpButton();
  }
  break;
  }

  return false;
}

bool CGUIDialogBoxeeLoginWizardTOU::HandleClickNext()
{
  return true;
}

bool CGUIDialogBoxeeLoginWizardTOU::HandleClickBack()
{
  return true;
}

bool CGUIDialogBoxeeLoginWizardTOU::HandleClickOnTouButton()
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
    CStdString strUrl = BOXEE::BXConfiguration::GetInstance().GetStringParam("Boxee.Term.Of.Use","http://app.boxee.tv/api/getlegal?page=tou");
    BOXEE::BXCurl curl;

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

  if (!InitContainerText(m_touStr))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardTOU::HandleClickOnTouButton - FAILED to get TextBox container. [id=%d] (blw)(digwiz)",TEXTBOX_CONTROL_ID);
    return true;
  }

  SET_CONTROL_HIDDEN(BUTTONS_GROUP_CONTROL_ID);
  SET_CONTROL_VISIBLE(TEXTBOX_GROUP_CONTROL_ID);
  SET_CONTROL_LABEL(TEXTBOX_TITLE_CONTROL_ID,g_localizeStrings.Get(53436));
  SET_CONTROL_FOCUS(SCROLL_CONTROL_ID,0);

  return true;
}

bool CGUIDialogBoxeeLoginWizardTOU::HandleClickOnPpButton()
{
  if (m_ppStr.IsEmpty())
  {
    CGUIDialogProgress* progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      progress->StartModal();
      progress->Progress();
    }

    CStdString strUrl = BOXEE::BXConfiguration::GetInstance().GetStringParam("Boxee.Privacy.Policy","http://app.boxee.tv/api/getlegal?page=privacy");
    BOXEE::BXCurl curl;

    m_ppStr = curl.HttpGetString(strUrl, false);

    if (progress)
    {
      progress->Close();
    }

    if (m_ppStr.IsEmpty())
    {
      CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),g_localizeStrings.Get(53433));
      return true;
    }
  }

  if (!InitContainerText(m_ppStr))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardTOU::HandleClickOnPpButton - FAILED to get TextBox container. [id=%d] (blw)(digwiz)",TEXTBOX_CONTROL_ID);
    return true;
  }

  SET_CONTROL_HIDDEN(BUTTONS_GROUP_CONTROL_ID);
  SET_CONTROL_VISIBLE(TEXTBOX_GROUP_CONTROL_ID);
  SET_CONTROL_LABEL(TEXTBOX_TITLE_CONTROL_ID,g_localizeStrings.Get(53438));
  SET_CONTROL_FOCUS(SCROLL_CONTROL_ID,0);

  return true;
}

bool CGUIDialogBoxeeLoginWizardTOU::InitContainerText(const CStdString& text)
{
  CGUIMessage winmsg1(GUI_MSG_LABEL_RESET, GetID(), HIDDEN_CONTAINER);
  OnMessage(winmsg1);

  CFileItemPtr itemPtr(new CFileItem(text));
  CGUIMessage winmsg2(GUI_MSG_LABEL_ADD, GetID(), HIDDEN_CONTAINER, 0, 0, itemPtr);
  OnMessage(winmsg2);

  return true;
}
