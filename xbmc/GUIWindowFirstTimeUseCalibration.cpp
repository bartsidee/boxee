
#include "GUIWindowFirstTimeUseCalibration.h"

#ifdef HAS_EMBEDDED

#include "GUIWindowManager.h"
#include "LocalizeStrings.h"
#include "GUILabelControl.h"
#include "GUIEditControl.h"
#include "GUIRadioButtonControl.h"
#include "GUIDialogOK2.h"
#include "log.h"
#include "Settings.h"
#include "Application.h"

#define TOP_LEFT_MOVER 8
#define BOTTOM_RIGHT_MOVER 9

#define SETTING_TOP_LEFT 1
#define SETTING_BOTTOM_RIGHT 2
#define VALIDATE_STATE 3

#define DONE_BUTTON 250
#define TRY_AGAIN_BUTTON 251

CGUIWindowFirstTimeUseCalibration::CGUIWindowFirstTimeUseCalibration()
{
  SetID(WINDOW_FTU_CALIBRATION);
  SetXMLFile("ftu_calibration.xml");
}

CGUIWindowFirstTimeUseCalibration::~CGUIWindowFirstTimeUseCalibration()
{

}


void CGUIWindowFirstTimeUseCalibration::OnInitWindow()
{
  CGUIWindowSettingsScreenCalibration::OnInitWindow();

  SET_CONTROL_FOCUS(TOP_LEFT_MOVER,0);
  SetProperty("setting-state",SETTING_TOP_LEFT);

  CLog::Log(LOGDEBUG,"CGUIWindowFirstTimeUseCalibration::OnInitWindow - Exit function. [setting-state=%d=SETTING_TOP_LEFT] (ftu)",GetPropertyInt("setting-state"));
}

bool CGUIWindowFirstTimeUseCalibration::OnAction(const CAction &action)
{
  switch (action.id)
  {
  case ACTION_PREVIOUS_MENU:
  case ACTION_PARENT_DIR:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowFirstTimeUseCalibration::OnAction - ACTION_PREVIOUS_MENU or ACTION_PARENT_DIR is not allow (ftu)");
    return true;
  }
  break;
  case ACTION_BUILT_IN_FUNCTION:
  {
    // don't allow during FTU
    CLog::Log(LOGDEBUG,"CGUIWindowFirstTimeUseCalibration::OnAction - ACTION_BUILT_IN_FUNCTION is not allow (ftu)");
    return true;
  }
  break;
  }

  return CGUIWindowSettingsScreenCalibration::OnAction(action);
}

bool CGUIWindowFirstTimeUseCalibration::OnMessage(CGUIMessage& message)
{
  bool retVal = CGUIWindowSettingsScreenCalibration::OnMessage(message);

  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    switch(message.GetSenderId())
    {
    case TOP_LEFT_MOVER:
    {
      SetProperty("setting-state",SETTING_BOTTOM_RIGHT);
      CLog::Log(LOGDEBUG,"CGUIWindowFirstTimeUseCalibration::OnMessage - GUI_MSG_CLICKED - Finish set TOP_LEFT_MOVER. Set property [setting-state=%d=SETTING_BOTTOM_RIGHT] (ftu)",GetPropertyInt("setting-state"));
      SET_CONTROL_FOCUS(BOTTOM_RIGHT_MOVER,0);
    }
    break;
    case BOTTOM_RIGHT_MOVER:
    {
      SetProperty("setting-state",VALIDATE_STATE);
      CLog::Log(LOGDEBUG,"CGUIWindowFirstTimeUseCalibration::OnMessage - GUI_MSG_CLICKED - Finish set BOTTOM_RIGHT_MOVER. Set property [setting-state=%d=VALIDATE_STATE] (ftu)",GetPropertyInt("setting-state"));

      SET_CONTROL_VISIBLE(TRY_AGAIN_BUTTON);
      SET_CONTROL_VISIBLE(DONE_BUTTON);
      SET_CONTROL_FOCUS(DONE_BUTTON,0);
    }
    break;
    case TRY_AGAIN_BUTTON:
    {
      SET_CONTROL_FOCUS(TOP_LEFT_MOVER,0);
      SetProperty("setting-state",SETTING_TOP_LEFT);
      CLog::Log(LOGDEBUG,"CGUIWindowFirstTimeUseCalibration::OnMessage - GUI_MSG_CLICKED - click on TRY_AGAIN_BUTTON. Set property [setting-state=%d=SETTING_TOP_LEFT] (ftu)",GetPropertyInt("setting-state"));
    }
    break;
    case DONE_BUTTON:
    {
      CLog::Log(LOGDEBUG,"CGUIWindowFirstTimeUseCalibration::OnMessage - GUI_MSG_CLICKED - click on DONE_BUTTON. Call Login() (ftu)");

      g_stSettings.m_doneFTU2 = true;
      g_settings.Save();

      /////////////////////////////////////////////
      // DONE_BUTTON was clicked -> call Login() //
      /////////////////////////////////////////////

      bool loginWasDone = g_application.GetBoxeeLoginManager().Login();
      return loginWasDone;
    }
    break;
    }
  }
  break;
  }

  return retVal;
}

#endif

