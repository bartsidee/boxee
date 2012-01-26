/*
 * GUIWindowSettingsScreenSimpleCalibration.cpp
 *
 *  Created on: Jul 20, 2009
 *      Author: yuvalt
 */


#include "GUIButtonControl.h"
#include "GUIWindowSettingsScreenSimpleCalibration.h"
#include "GUIWindowManager.h"
#include "Settings.h"
#include "GUIToggleButtonControl.h"
#include "GUIInfoManager.h"
#include "Application.h"

#define BUTTON_SCREEN_FORMAT_4_3    101
#define BUTTON_SCREEN_FORMAT_16_9   102
#define BUTTON_SCREEN_FORMAT_16_10  103
#define BUTTON_SCREEN_FORMAT_21_9   104
#define BUTTON_SCREEN_FORMAT_CUSTOM 105

#define BUTTON_OVERSCAN_NONE   201
#define BUTTON_OVERSCAN_3      202
#define BUTTON_OVERSCAN_4      203
#define BUTTON_OVERSCAN_5      204
#define BUTTON_OVERSCAN_6      205
#define BUTTON_OVERSCAN_CUSTOM 206

#define BUTTON_MANUAL          301
#define BUTTON_RESET           302

#define BUTTON_NEXT            10

CGUIWindowSettingsScreenSimpleCalibration::CGUIWindowSettingsScreenSimpleCalibration() : CGUIWindow(WINDOW_SCREEN_SIMPLE_CALIBRATION, "SettingsScreenSimpleCalibration.xml")
{
  m_launchFromLogin = false;;
}

CGUIWindowSettingsScreenSimpleCalibration::~CGUIWindowSettingsScreenSimpleCalibration()
{

}

bool CGUIWindowSettingsScreenSimpleCalibration::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl >= BUTTON_SCREEN_FORMAT_4_3 && iControl <= BUTTON_SCREEN_FORMAT_CUSTOM)
      {
        ChangeScreenFormat(iControl);
        SelectScreenFormatButton(iControl);
      }
      else if (iControl >= BUTTON_OVERSCAN_NONE && iControl <= BUTTON_OVERSCAN_CUSTOM)
      {
        ChangeOverscan(iControl);
        SelectOverscanButton(iControl);
      }
      else if (iControl == BUTTON_MANUAL)
      {
        g_windowManager.ActivateWindow(WINDOW_SCREEN_CALIBRATION);
        return true;
      }
      else if (iControl == BUTTON_RESET)
      {
        ChangeScreenFormat(BUTTON_SCREEN_FORMAT_16_9);
        SelectScreenFormatButton(BUTTON_SCREEN_FORMAT_16_9);
        ChangeOverscan(BUTTON_OVERSCAN_NONE);
        SelectOverscanButton(BUTTON_OVERSCAN_NONE);
        return true;
      }      
      else if (iControl == BUTTON_NEXT)
      {
        // if BUTTON_NEXT appear -> window was launch from login -> go to WINDOW_HOME
        g_windowManager.ChangeActiveWindow(WINDOW_HOME);
        return true;
    }
  }
  }
  
  return CGUIWindow::OnMessage(message);
}

bool CGUIWindowSettingsScreenSimpleCalibration::OnAction(const CAction &action)
{
  if (action.id == ACTION_PREVIOUS_MENU)
  {
    g_windowManager.PreviousWindow();
    return true;
  }
  
  return CGUIWindow::OnAction(action);  
}

void CGUIWindowSettingsScreenSimpleCalibration::OnInitWindow()
{
  CGUIWindow::OnInitWindow();
  
  m_savedScreenFormat = g_settings.GetCurrentScreenFormat();
  m_savedPixelRatio = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].fPixelRatio;
  m_savedOverscan = g_settings.GetCurrentOverscan();
  m_savedOverscanValues = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan;

  if (m_savedScreenFormat == SCREEN_FORMAT_CUSTOM)
  {
    SET_CONTROL_VISIBLE(BUTTON_SCREEN_FORMAT_CUSTOM);
  }
  else
  {
    SET_CONTROL_HIDDEN(BUTTON_SCREEN_FORMAT_CUSTOM);
  }
  
  if (m_savedOverscan == OVERSCAN_CUSTOM)
  {
    SET_CONTROL_VISIBLE(BUTTON_OVERSCAN_CUSTOM);
  }
  else
  {
    SET_CONTROL_HIDDEN(BUTTON_OVERSCAN_CUSTOM);
  }
  
  /////////////////////////////////////////////////////////////////////////////////////////
  // need to show BUTTON_NEXT only if the screen was open for the first user after login //
  /////////////////////////////////////////////////////////////////////////////////////////

  if (m_launchFromLogin)
  {
    SET_CONTROL_VISIBLE(BUTTON_NEXT);
  }
  else
  {
    SET_CONTROL_HIDDEN(BUTTON_NEXT);
  }

  switch (m_savedScreenFormat)
  {
  case SCREEN_FORMAT_16_9:
    SelectScreenFormatButton(BUTTON_SCREEN_FORMAT_16_9);
    break;
  case SCREEN_FORMAT_16_10:
    SelectScreenFormatButton(BUTTON_SCREEN_FORMAT_16_10);
    break;
  case SCREEN_FORMAT_4_3:
    SelectScreenFormatButton(BUTTON_SCREEN_FORMAT_4_3);
    break;
  case SCREEN_FORMAT_21_9:
    SelectScreenFormatButton(BUTTON_SCREEN_FORMAT_21_9);
    break;
  case SCREEN_FORMAT_CUSTOM:
    SelectScreenFormatButton(BUTTON_SCREEN_FORMAT_CUSTOM);
    break;
  }
  
  switch (m_savedOverscan)
  {
  case OVERSCAN_NONE:
    SelectOverscanButton(BUTTON_OVERSCAN_NONE);
    break;
  case OVERSCAN_3_0:
    SelectOverscanButton(BUTTON_OVERSCAN_3);
    break;
  case OVERSCAN_4_0:
    SelectOverscanButton(BUTTON_OVERSCAN_4);
    break;
  case OVERSCAN_5_0:
    SelectOverscanButton(BUTTON_OVERSCAN_5);
    break;
  case OVERSCAN_6_0:
    SelectOverscanButton(BUTTON_OVERSCAN_6);
    break;
  case OVERSCAN_CUSTOM:
    SelectOverscanButton(BUTTON_OVERSCAN_CUSTOM);
    break;
  }
}

void CGUIWindowSettingsScreenSimpleCalibration::ChangeScreenFormat(int iControl)
{
  switch (iControl)
  {
  case BUTTON_SCREEN_FORMAT_16_9:
    g_settings.SetCurrentScreenFormat(SCREEN_FORMAT_16_9);
    break;
  case BUTTON_SCREEN_FORMAT_16_10:
    g_settings.SetCurrentScreenFormat(SCREEN_FORMAT_16_10);
    break;
  case BUTTON_SCREEN_FORMAT_4_3:
    g_settings.SetCurrentScreenFormat(SCREEN_FORMAT_4_3);
    break;
  case BUTTON_SCREEN_FORMAT_21_9:
    g_settings.SetCurrentScreenFormat(SCREEN_FORMAT_21_9);
    break;
  case BUTTON_SCREEN_FORMAT_CUSTOM:
    g_settings.SetCurrentScreenFormat(m_savedPixelRatio);
    break;
  }
}

void CGUIWindowSettingsScreenSimpleCalibration::OnDeinitWindow(int nextWindowID)
{
  if (nextWindowID == WINDOW_HOME)
  {
    m_launchFromLogin = false;
  }

  CGUIWindow::OnDeinitWindow(nextWindowID);
}

void CGUIWindowSettingsScreenSimpleCalibration::ChangeOverscan(int iControl)
{
  switch (iControl)
  {
  case BUTTON_OVERSCAN_NONE:
    g_settings.SetCurrentOverscan(OVERSCAN_NONE);
    break;
  case BUTTON_OVERSCAN_3:
    g_settings.SetCurrentOverscan(OVERSCAN_3_0);
    break;
  case BUTTON_OVERSCAN_4:
    g_settings.SetCurrentOverscan(OVERSCAN_4_0);
    break;
  case BUTTON_OVERSCAN_5:
    g_settings.SetCurrentOverscan(OVERSCAN_5_0);
    break;
  case BUTTON_OVERSCAN_6:
    g_settings.SetCurrentOverscan(OVERSCAN_6_0);
    break;
  case BUTTON_OVERSCAN_CUSTOM:
    g_settings.SetCurrentOverscan(m_savedOverscanValues);
    break;    
  }  

  CopyOverscan();
}

void CGUIWindowSettingsScreenSimpleCalibration::SelectScreenFormatButton(int iControl)
{
  for (int i = BUTTON_SCREEN_FORMAT_4_3; i <= BUTTON_SCREEN_FORMAT_CUSTOM; i++)
  {
    CGUIToggleButtonControl* pControl = (CGUIToggleButtonControl*) GetControl(i);
    if (iControl != i)
    {
      pControl->SetToggleSelect(g_infoManager.TranslateString("false"));
    }
    else
    {
      pControl->SetToggleSelect(g_infoManager.TranslateString("true"));
    }            
  }  
}

void CGUIWindowSettingsScreenSimpleCalibration::SelectOverscanButton(int iControl)
{
  for (int i = BUTTON_OVERSCAN_NONE; i <= BUTTON_OVERSCAN_CUSTOM; i++)
  {
    CGUIToggleButtonControl* pControl = (CGUIToggleButtonControl*) GetControl(i);
    if (iControl != i)
    {
      pControl->SetToggleSelect(g_infoManager.TranslateString("false"));
    }
    else
    {
      pControl->SetToggleSelect(g_infoManager.TranslateString("true"));
    }            
  }
}

void CGUIWindowSettingsScreenSimpleCalibration::SetLaunchFromLogin(bool launchFromLogin)
{
  m_launchFromLogin = launchFromLogin;
}

// We need to copy the calibration to all matching resolutions
void CGUIWindowSettingsScreenSimpleCalibration::CopyOverscan()
{
  RESOLUTION iCurRes = g_graphicsContext.GetVideoResolution();
  RESOLUTION_INFO curInfo = g_settings.m_ResInfo[iCurRes];


  for(int i = 0; i < (int)g_settings.m_ResInfo.size(); i++)
  {
    RESOLUTION_INFO info = g_settings.m_ResInfo[i];

    if(i == iCurRes)
      continue;

    if(info.iWidth == curInfo.iWidth && info.iHeight == curInfo.iHeight && info.dwFlags == curInfo.dwFlags)
    {
      printf("For res %d %s Found matching %d %s\n", iCurRes, curInfo.strMode.c_str(), i, info.strMode.c_str());
      g_settings.m_ResInfo[i].Overscan = curInfo.Overscan;
      g_settings.m_ResInfo[i].iSubtitles = curInfo.iSubtitles;
      g_settings.m_ResInfo[i].fPixelRatio = curInfo.fPixelRatio;
    }
  }
}

