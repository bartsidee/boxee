#ifndef GUIWINDOWBOXEEMAIN_H_
#define GUIWINDOWBOXEEMAIN_H_

#pragma once
#include "GUIWindowHome.h"
#include "GUILoaderWindow.h"
#include "FileItem.h"

#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxobject.h"
#include "lib/libBoxee/bxmessages.h"
#include "lib/libBoxee/bxboxeefeed.h"

#include "BoxeeFeedItemsLoader.h"

#include "WatchDog.h"

typedef enum { STATE_NORMAL, STATE_SELECT_SHORTCUT, STATE_MOVE_SHORTCUT } HomeWindowState;

#define LIST_FEATURES         8200

/**
 * Implements the new main screen for Boxee UI
 * The class extends the CGUIWindow with WINDOW_HOME and "Home.xml"
 */
class CGUIWindowBoxeeMain : public CGUILoaderWindow
{
public:
	CGUIWindowBoxeeMain(void);
  virtual ~CGUIWindowBoxeeMain(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  virtual void Render();
  
#ifdef HAS_DVB
  static bool  RunOnBoardingWizardIfNeeded(bool rescan);
#endif

private:
  void SetUserName(bool force);
  void HandleClickInList(int listType);
  void HandleClickOnFeatureItem(const CFileItem& item);
  
  bool m_lastConnectedStatus;
};

#endif /*GUIWINDOWBOXEEMAIN_H_*/
