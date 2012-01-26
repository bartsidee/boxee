/*
 * GUIWindowSettingsScreenSimpleCalibration.h
 *
 *  Created on: Jul 20, 2009
 *      Author: yuvalt
 */

#ifndef GUIWINDOWSETTINGSSCREENSIMPLECALIBRATION_H_
#define GUIWINDOWSETTINGSSCREENSIMPLECALIBRATION_H_

#include "Settings.h"
#include "GUIWindow.h"

class CGUIWindowSettingsScreenSimpleCalibration : public CGUIWindow
{
public:
  CGUIWindowSettingsScreenSimpleCalibration();
  virtual ~CGUIWindowSettingsScreenSimpleCalibration();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action); 
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  
  void SetLaunchFromLogin(bool launchFromLogin);

private:
  void ChangeScreenFormat(int iControl);
  void ChangeOverscan(int iControl);
  void SelectScreenFormatButton(int iControl);
  void SelectOverscanButton(int iControl);
  // We need to copy the calibration to all matching resolutions
  void CopyOverscan();
  
  ScreenFormatType m_savedScreenFormat;
  OverscanType m_savedOverscan;
  float m_savedPixelRatio;
  OVERSCAN m_savedOverscanValues;  

  int m_launchFromLogin;
};

#endif /* GUIWINDOWSETTINGSSCREENSIMPLECALIBRATION_H_ */
