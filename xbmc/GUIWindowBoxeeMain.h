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

#define LIST_RECOMMEND        8100
#define LIST_FEATURES         8200
#define LIST_QUEUE            8300

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

private:

  void HandleClickInList(int listType);

  void HandleClickOnRecommendationItem(const CFileItem& item);
  void HandleClickOnFeatureItem(const CFileItem& item);
  void HandleClickOnQueueItem(const CFileItem& item);

};

#endif /*GUIWINDOWBOXEEMAIN_H_*/
