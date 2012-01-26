#pragma once

#include "GUIDialogBoxeeWizardBase.h"
#include "FileItem.h"

class CGUIDialogBoxeeLoginWizardNewUserDetails : public CGUIDialogBoxeeWizardBase
{
public:

  CGUIDialogBoxeeLoginWizardNewUserDetails();
  virtual ~CGUIDialogBoxeeLoginWizardNewUserDetails();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);

  CStdString GetGender();
  CStdString GetYear();
  CStdString GetMonth();
  CStdString GetDay();

protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

private:

  enum CHOOSE_LIST_TYPE {NONE_LIST=0,GENDER_LIST=1,YEAR_LIST=2,MONTH_LIST=3,DAYS_LIST=4};

  bool HandleClick(CGUIMessage& message);

  bool HandleClickOnButton(CFileItemList& list);
  bool HandleClickOnChooseListItem();
  bool HandleClickOnSubmitButton();
  bool HandleClickOnSkipButton();

  int GetFocusItemIndex();

  bool IsAllFieldsWereChosen();
  bool GetIsAgeValid();
  bool CanSubmit();

  void Reset();

  bool CloseDialog(CActionChoose::ActionChooseEnums actionChoseEnum);

  void initGenderList();
  void initYearList();
  void initMonthList();
  void initDaysList();

  CFileItemList m_genderList;
  CFileItemList m_yearList;
  CFileItemList m_monthList;
  CFileItemList m_daysList;

  CStdString m_selectedGender;
  CStdString m_selectedYear;
  CStdString m_selectedMonth;
  CStdString m_selectedDay;

  CHOOSE_LIST_TYPE m_activeListType;

  int m_saveFocusedControlId;
  CGUIWindow* m_activeWindow;
};
