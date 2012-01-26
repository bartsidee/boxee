#include "GUIDialogBoxeeLoginWizardNewUserDetails.h"
#include "GUIWindowManager.h"
#include "BoxeeLoginManager.h"
#include "GUIDialogBoxeeLoginWizardAddNewUser.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIRadioButtonControl.h"
#include "GUIEditControl.h"
#include "Application.h"
#include "GUIDialogOK2.h"
#include "BoxeeLoginWizardManager.h"
#include "RegExp.h"
#include "GUIBaseContainer.h"
#include "Util.h"
#include "lib/libBoxee/bxcredentials.h"
#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "lib/libBoxee/bxxmldocument.h"
#include "lib/libBoxee/bxutils.h"

#define BUTTONS_GROUP_CONTROL_ID        8800
#define CHOOSE_LIST_CONTROL_ID          8900

#define GENDER_BUTTON_CONTROL_ID        8801
#define MONTH_BUTTON_CONTROL_ID         8802
#define DAYS_BUTTON_CONTROL_ID          8803
#define YEAR_BUTTON_CONTROL_ID          8804

#define SUBMIT_BUTTON_CONTROL_ID        8805
#define SKIP_BUTTON_CONTROL_ID          8806

#define GENDER_LIST_ITEM_START_ID       55636
#define GENDER_LIST_ITEM_END_ID         55638

#define BIRTH_YEAR_RANGE                100
#define BIRTH_YEAR_AGE_TO_FOCUS_INIT    25

#define MONTH_LIST_ITEM_START_ID        21
#define MONTH_LIST_ITEM_END_ID          32

#define NEW_USER_MIN_AGE                13.00

#define CHOOSE_LIST_TYPE_PROPERTY_NAME  "choose-list-type"

#define SELECTED_GENDER_WINDOW_PROPERTY "chosen-gender"
#define SELECTED_YEAR_WINDOW_PROPERTY "chosen-year"
#define SELECTED_MONTH_WINDOW_PROPERTY "chosen-month"
#define SELECTED_DAY_WINDOW_PROPERTY "chosen-day"

CGUIDialogBoxeeLoginWizardNewUserDetails::CGUIDialogBoxeeLoginWizardNewUserDetails() : CGUIDialogBoxeeWizardBase(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_NEW_USER_DETAILS,"boxee_login_wizard_new_user_details.xml","CGUIDialogBoxeeLoginWizardNewUserDetails")
{
  initGenderList();
  initYearList();
  initMonthList();
  initDaysList();

  Reset();
}

CGUIDialogBoxeeLoginWizardNewUserDetails::~CGUIDialogBoxeeLoginWizardNewUserDetails()
{

}

void CGUIDialogBoxeeLoginWizardNewUserDetails::OnInitWindow()
{
  CGUIDialogBoxeeWizardBase::OnInitWindow();

  Reset();

  m_activeWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());

  m_activeWindow->SetProperty(SELECTED_GENDER_WINDOW_PROPERTY,m_selectedGender);
  m_activeWindow->SetProperty(SELECTED_YEAR_WINDOW_PROPERTY,m_selectedYear);
  m_activeWindow->SetProperty(SELECTED_MONTH_WINDOW_PROPERTY,m_selectedMonth);
  m_activeWindow->SetProperty(SELECTED_DAY_WINDOW_PROPERTY,m_selectedDay);

  m_activeListType = NONE_LIST;
  m_saveFocusedControlId = 0;

  SET_CONTROL_HIDDEN(CHOOSE_LIST_CONTROL_ID);
  SET_CONTROL_VISIBLE(BUTTONS_GROUP_CONTROL_ID);
}

bool CGUIDialogBoxeeLoginWizardNewUserDetails::OnAction(const CAction& action)
{
  if (action.id == ACTION_PREVIOUS_MENU)
  {
    const CGUIControl* pButtons = GetControl(BUTTONS_GROUP_CONTROL_ID);
    if (!pButtons || pButtons->IsVisible())
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardNewUserDetails::OnAction - ACTION_PREVIOUS_MENU - can't go back from here (blw)(digwiz)");
      return true;
    }
    else
    {
      SET_CONTROL_HIDDEN(CHOOSE_LIST_CONTROL_ID);
      SET_CONTROL_VISIBLE(BUTTONS_GROUP_CONTROL_ID);
      SET_CONTROL_FOCUS(m_saveFocusedControlId,0);
      m_saveFocusedControlId = 0;

      return true;
    }
  }

  return CGUIDialogBoxeeWizardBase::OnAction(action);
}

bool CGUIDialogBoxeeLoginWizardNewUserDetails::OnMessage(CGUIMessage& message)
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

bool CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClick(CGUIMessage& message)
{
  int iControl = message.GetSenderId();

  switch(iControl)
  {
  case GENDER_BUTTON_CONTROL_ID:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClick - handling click on [Control=%d=GENDER_BUTTON_CONTROL_ID] (blw)(digwiz)",iControl);
    return HandleClickOnButton(m_genderList);
  }
  break;
  case YEAR_BUTTON_CONTROL_ID:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClick - handling click on [Control=%d=YEAR_BUTTON_CONTROL_ID] (blw)(digwiz)",iControl);
    return HandleClickOnButton(m_yearList);
  }
  break;
  case MONTH_BUTTON_CONTROL_ID:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClick - handling click on [Control=%d=MONTH_BUTTON_CONTROL_ID] (blw)(digwiz)",iControl);
    return HandleClickOnButton(m_monthList);
  }
  break;
  case DAYS_BUTTON_CONTROL_ID:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClick - handling click on [Control=%d=DAY_BUTTON_CONTROL_ID] (blw)(digwiz)",iControl);
    return HandleClickOnButton(m_daysList);
  }
  break;
  case CHOOSE_LIST_CONTROL_ID:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClick - handling click on [Control=%d=CHOOSE_LIST_CONTROL_ID] (blw)(digwiz)",iControl);
    return HandleClickOnChooseListItem();
  }
  break;
  case SUBMIT_BUTTON_CONTROL_ID:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClick - handling click on [Control=%d=SUBMIT_BUTTON_CONTROL_ID] (blw)(digwiz)",iControl);
    return HandleClickOnSubmitButton();
  }
  break;
  case SKIP_BUTTON_CONTROL_ID:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClick - handling click on [Control=%d=SKIP_BUTTON_CONTROL_ID] (blw)(digwiz)",iControl);
    return HandleClickOnSkipButton();
  }
  break;


  }

  return false;
}

bool CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClickNext()
{
  return true;
}

bool CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClickBack()
{
  return true;
}

int CGUIDialogBoxeeLoginWizardNewUserDetails::GetFocusItemIndex()
{
  int focusedItemIndex = 0;

  switch(m_activeListType)
  {
  case GENDER_LIST:
  {
    if(m_selectedGender == g_localizeStrings.Get(55637)) //female
    {
      focusedItemIndex = 1;
    }
  }
  break;
  case YEAR_LIST:
  {
    if(m_selectedYear != "")
    {
      time_t now = time(NULL);
      struct tm *tmNow = localtime(&now);
      int currYear = tmNow->tm_year + 1900;
      int selectedYear =  BOXEE::BXUtils::StringToInt(m_selectedYear);
      focusedItemIndex = BIRTH_YEAR_RANGE - (currYear - selectedYear);
    }
    else
    {
      focusedItemIndex = BIRTH_YEAR_RANGE - BIRTH_YEAR_AGE_TO_FOCUS_INIT - 1;
    }
  }
  break;
  case MONTH_LIST:
  {
    if(m_selectedMonth != "")
    {
      for (int i=MONTH_LIST_ITEM_START_ID; i<=MONTH_LIST_ITEM_END_ID; i++)
      {
        if (m_selectedMonth == g_localizeStrings.Get(i))
        {
          focusedItemIndex = i - MONTH_LIST_ITEM_START_ID;
          break;
        }
      }
    }
  }
  break;
  case DAYS_LIST:
  {
    if(m_selectedDay != "")
    {
      focusedItemIndex = BOXEE::BXUtils::StringToInt(m_selectedDay) - 1;
    }
  }
  break;
  default:
  {
    focusedItemIndex = 0;
  }
  break;
  }

  return focusedItemIndex;
}

bool CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClickOnButton(CFileItemList& list)
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CHOOSE_LIST_CONTROL_ID);
  OnMessage(msg);

  CGUIMessage msg2(GUI_MSG_LABEL_BIND, GetID(), CHOOSE_LIST_CONTROL_ID, 0, 0, &list);
  OnMessage(msg2);

  m_activeListType = (CHOOSE_LIST_TYPE)list.GetPropertyInt(CHOOSE_LIST_TYPE_PROPERTY_NAME);

  m_saveFocusedControlId = GetFocusedControlID();

  SET_CONTROL_HIDDEN(BUTTONS_GROUP_CONTROL_ID);
  SET_CONTROL_VISIBLE(CHOOSE_LIST_CONTROL_ID);

  int focusedItemIndex = GetFocusItemIndex();
  SET_CONTROL_FOCUS(CHOOSE_LIST_CONTROL_ID,focusedItemIndex);

  return true;
}

bool CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClickOnChooseListItem()
{
  CGUIBaseContainer* pList = (CGUIBaseContainer*)GetControl(CHOOSE_LIST_CONTROL_ID);

  if (!pList)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClickOnChooseListItem - FAILED to get container [%d=CHOOSE_LIST_CONTROL_ID] container (blw)",CHOOSE_LIST_CONTROL_ID);
    return false;
  }

  CGUIListItemPtr selectedButton = pList->GetSelectedItemPtr();

  if (!selectedButton.get())
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClickOnChooseListItem - FAILED to get selected FileItem from [%d=CHOOSE_LIST_CONTROL_ID] container (blw)",CHOOSE_LIST_CONTROL_ID);
    return false;
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClickOnChooseListItem - selected item is [label=%s][choose-property=%d] (blw)",selectedButton->GetLabel().c_str(),selectedButton->GetPropertyInt(CHOOSE_LIST_TYPE_PROPERTY_NAME));

  CStdString selectedLabel = selectedButton->GetLabel();

  switch (m_activeListType)
  {
  case GENDER_LIST:
  {
    m_selectedGender = selectedLabel;
    m_activeWindow->SetProperty(SELECTED_GENDER_WINDOW_PROPERTY,m_selectedGender);
  }
  break;
  case YEAR_LIST:
  {
    m_selectedYear = selectedLabel;
    m_activeWindow->SetProperty(SELECTED_YEAR_WINDOW_PROPERTY,m_selectedYear);
  }
  break;
  case MONTH_LIST:
  {
    m_selectedMonth = selectedLabel;
    m_activeWindow->SetProperty(SELECTED_MONTH_WINDOW_PROPERTY,m_selectedMonth);
  }
  break;
  case DAYS_LIST:
  {
    m_selectedDay = selectedLabel;
    m_activeWindow->SetProperty(SELECTED_DAY_WINDOW_PROPERTY,m_selectedDay);
  }
  break;
  case NONE_LIST:
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClickOnChooseListItem - FAILED to handle NONE_LIST type (blw)");
    return false;
  }
  break;
  }

  SET_CONTROL_HIDDEN(CHOOSE_LIST_CONTROL_ID);
  SET_CONTROL_VISIBLE(BUTTONS_GROUP_CONTROL_ID);
  SET_CONTROL_FOCUS(m_saveFocusedControlId,0);

  m_saveFocusedControlId = 0;

  return true;
}

bool CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClickOnSubmitButton()
{
  if(!CanSubmit())
  {
    return false;
  }

  CStdString strUrl = BOXEE::BXConfiguration::GetInstance().GetStringParam("Boxee.Update.User.Details","http://app.boxee.tv/accountapi/updatedetails");

  BOXEE::ListHttpHeaders headers;
  //headers.push_back("Content-Type: text/xml");

  BOXEE::BXXMLDocument doc;
  //doc.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());

  CStdString postData = "dob=";

  CStdString monthAsNumberStr;
  for (int i=MONTH_LIST_ITEM_START_ID; i<=MONTH_LIST_ITEM_END_ID; i++)
  {
    if (m_selectedMonth == g_localizeStrings.Get(i))
    {
      monthAsNumberStr = BOXEE::BXUtils::IntToString(i - MONTH_LIST_ITEM_START_ID + 1);
      if (i < (MONTH_LIST_ITEM_END_ID - 2))
      {
        monthAsNumberStr = "0" + monthAsNumberStr;
      }

      break;
    }
  }

  postData += m_selectedYear + "-" + monthAsNumberStr + "-" + m_selectedDay;

  CGUIDialogBoxeeLoginWizardAddNewUser* pDialog = (CGUIDialogBoxeeLoginWizardAddNewUser*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_LOGIN_WIZARD_ADD_NEW_USER);
  if (!pDialog)
  {
    CLog::Log(LOGWARNING,"CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClickOnSubmitButton - FAILED to get LOGIN_WIZARD_ADD_NEW_USER window. continue as skip (blw)");
    //return CloseDialog(CActionChoose::NEXT);
  }
  else
  {
    CStdString email = pDialog->GetEmail();
    if (email.IsEmpty())
    {
      CLog::Log(LOGWARNING,"CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClickOnSubmitButton - FAILED to get email. continue as NEXT (blw)");
      //return CloseDialog(CActionChoose::NEXT);
    }
    else
    {
      postData += "&email=";
      postData += email;
    }

    CStdString username = pDialog->GetUserName();
    if (username.IsEmpty())
    {
      CLog::Log(LOGWARNING,"CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClickOnSubmitButton - FAILED to get username. continue as NEXT (blw)");
      //return CloseDialog(CActionChoose::NEXT);
    }
    else
    {
      CStdString firstName = username;
      CStdString lastName = "";

      size_t spacePos = username.find(" ");
      if (spacePos != std::string::npos)
      {
        firstName = username.substr(0,spacePos);
        lastName = username.substr(spacePos+1);
      }

      if (!firstName.IsEmpty())
      {
        postData += "&fname=";
        postData += firstName;
      }

      if (!lastName.IsEmpty())
      {
        postData += "&lname=";
        postData += lastName;
      }
    }
  }

  CStdString gender = "";
  if (m_selectedGender == g_localizeStrings.Get(55636))
  {
    gender = "1";
  }
  else if (m_selectedGender == g_localizeStrings.Get(55637))
  {
    gender = "2";
  }

  if (!gender.IsEmpty())
  {
    postData += "&gender=";
    postData += gender;
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClickOnSubmitButton - going to update user details. postData is [%s] (blw)(cnu)",postData.c_str());

  bool isSucceeded = false;
  isSucceeded = doc.LoadFromURL(strUrl,headers,postData);

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClickOnSubmitButton - update user details returned [isSucceeded=%d][RetCode=%ld] (blw)(cnu)",isSucceeded,doc.GetLastRetCode());

  if (!isSucceeded)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClickOnSubmitButton - FAILED to update user details. continue as NEXT (blw)");
    CGUIDialogOK2::ShowAndGetInput(53701, 53473);
  }

  return CloseDialog(CActionChoose::NEXT);
}

bool CGUIDialogBoxeeLoginWizardNewUserDetails::IsAllFieldsWereChosen()
{
  if (m_selectedGender == g_localizeStrings.Get(55630) ||
      m_selectedYear == g_localizeStrings.Get(55651) ||
      m_selectedMonth == g_localizeStrings.Get(55652) ||
      m_selectedDay == g_localizeStrings.Get(55653))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeLoginWizardNewUserDetails::IsAllFieldsWereChosen - NOT all field was chosen (blw)");
    return false;
  }

  return true;
}

bool CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClickOnSkipButton()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeLoginWizardNewUserDetails::HandleClickOnSkipButton - enter HandleClickOnSkipButton function (blw)");

  return CloseDialog(CActionChoose::NEXT);
}

bool CGUIDialogBoxeeLoginWizardNewUserDetails::CloseDialog(CActionChoose::ActionChooseEnums actionChoseEnum)
{
  m_actionChoseEnum = actionChoseEnum;
  Close();
  return true;
}

void CGUIDialogBoxeeLoginWizardNewUserDetails::initGenderList()
{
  m_genderList.Clear();

  m_genderList.SetProperty(CHOOSE_LIST_TYPE_PROPERTY_NAME,GENDER_LIST);
  for (int i=GENDER_LIST_ITEM_START_ID; i<=GENDER_LIST_ITEM_END_ID; i++)
  {
    CFileItemPtr item(new CFileItem(g_localizeStrings.Get(i)));
    m_genderList.Add(item);
  }
}

void CGUIDialogBoxeeLoginWizardNewUserDetails::initYearList()
{
  m_yearList.Clear();

  m_yearList.SetProperty(CHOOSE_LIST_TYPE_PROPERTY_NAME,YEAR_LIST);

  time_t now = time(NULL);
  struct tm *tmNow = localtime(&now);
  int currYear = tmNow->tm_year + 1900;
  int startYear = currYear - BIRTH_YEAR_RANGE;

  for (int i=startYear; i <= currYear; i++)
  {
    char year[5];
    itoa(i,year,10);
    CFileItemPtr item(new CFileItem(year));
    m_yearList.Add(item);
  }
}

void CGUIDialogBoxeeLoginWizardNewUserDetails::initMonthList()
{
  m_monthList.Clear();

  m_monthList.SetProperty(CHOOSE_LIST_TYPE_PROPERTY_NAME,MONTH_LIST);

  for (int i=MONTH_LIST_ITEM_START_ID; i<=MONTH_LIST_ITEM_END_ID; i++)
  {
    CFileItemPtr item(new CFileItem(g_localizeStrings.Get(i)));
    m_monthList.Add(item);
  }
}

void CGUIDialogBoxeeLoginWizardNewUserDetails::initDaysList()
{
  m_daysList.Clear();

  m_daysList.SetProperty(CHOOSE_LIST_TYPE_PROPERTY_NAME,DAYS_LIST);
  for (int i=1; i<=31; i++)
  {
    char day[5];
    itoa(i,day,10);
    CFileItemPtr item(new CFileItem());

    CStdString dayLabel(day);
    if (i < 10)
    {
      dayLabel.Insert(0,'0');
    }

    item->SetLabel(dayLabel);
    m_daysList.Add(item);
  }
}

bool CGUIDialogBoxeeLoginWizardNewUserDetails::CanSubmit()
{
  if (!IsAllFieldsWereChosen())
  {
    CGUIDialogOK2::ShowAndGetInput(20068,55672);
    return false;
  }

  if(!GetIsAgeValid())
  {
    CGUIDialogOK2::ShowAndGetInput(20068,55673);
   return false;
  }

  return true;
}

bool CGUIDialogBoxeeLoginWizardNewUserDetails::GetIsAgeValid()
{
  bool isAgeValid = false;
  // get current time
  time_t tmNow = time(NULL);

  //init current time
  struct tm currTime;
  localtime_r(&tmNow, &currTime);

  // init birthday time
  int birthdayYear = BOXEE::BXUtils::StringToInt(m_selectedYear);
  int birthdayMonth = 0;
  for (int i=MONTH_LIST_ITEM_START_ID; i<=MONTH_LIST_ITEM_END_ID; i++)
  {
     if (m_selectedMonth == g_localizeStrings.Get(i))
     {
        birthdayMonth = i - MONTH_LIST_ITEM_START_ID + 1;
        break;
     }
  }
  int birthdayDay = BOXEE::BXUtils::StringToInt(m_selectedDay);

  struct tm birthdayTime;
  localtime_r(&tmNow, &birthdayTime);

  birthdayTime.tm_mday = birthdayDay;
  birthdayTime.tm_mon = birthdayMonth - 1;
  birthdayTime.tm_year = birthdayYear - 1900;

  // calculate the diff
  double diff = difftime(mktime(&currTime),mktime(&birthdayTime));

  // compare the diff to the number of second of age 13
  if (diff > 410227200.0)
  {
    isAgeValid = true;
  }

  return isAgeValid;
}

CStdString CGUIDialogBoxeeLoginWizardNewUserDetails::GetGender()
{
  return m_selectedGender;
}

CStdString CGUIDialogBoxeeLoginWizardNewUserDetails::GetYear()
{
  return m_selectedYear;
}

CStdString CGUIDialogBoxeeLoginWizardNewUserDetails::GetMonth()
{
  return m_selectedMonth;
}

CStdString CGUIDialogBoxeeLoginWizardNewUserDetails::GetDay()
{
  return m_selectedDay;
}

void CGUIDialogBoxeeLoginWizardNewUserDetails::Reset()
{
  m_selectedGender = g_localizeStrings.Get(55630);
  m_selectedYear = g_localizeStrings.Get(55651);
  m_selectedMonth = g_localizeStrings.Get(55652);
  m_selectedDay = g_localizeStrings.Get(55653);

  if (CBoxeeLoginWizardManager::GetInstance().GetServerUserInfoByRef().IsInitialize())
  {
    CServerUserInfo serverUserInfo = CBoxeeLoginWizardManager::GetInstance().GetServerUserInfo();

    CStdString gender = serverUserInfo.m_gender;
    if (!gender.IsEmpty())
    {
      gender.ToLower();
      m_selectedGender = (gender == "male") ? g_localizeStrings.Get(55636) : g_localizeStrings.Get(55637);
    }

    if (!serverUserInfo.m_birthday.IsEmpty())
    {
      std::vector<CStdString> birthdayTokens;
      CUtil::Tokenize(serverUserInfo.m_birthday,birthdayTokens,"/");

      if (birthdayTokens.size() > 2)
      {
        if (StringUtils::IsNaturalNumber(birthdayTokens[0]))
        {
          int month = BOXEE::BXUtils::StringToInt(birthdayTokens[0]);
          m_selectedMonth = g_localizeStrings.Get(MONTH_LIST_ITEM_START_ID + month - 1);
        }

        if (StringUtils::IsNaturalNumber(birthdayTokens[1]))
        {
          m_selectedDay = birthdayTokens[1];
        }

        if (StringUtils::IsNaturalNumber(birthdayTokens[2]))
        {
          m_selectedYear = birthdayTokens[2];
        }
      }
    }
  }

  m_activeListType = NONE_LIST;
}

