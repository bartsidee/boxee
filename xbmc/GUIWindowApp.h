#ifndef GUIWINDOWAPP_H_
#define GUIWINDOWAPP_H_

#pragma once

#include <map>
#include "GUIWindow.h"
#include "FileItem.h"
#include "ItemLoader.h"
#include "AppDescriptor.h"
#include "Thread.h"

#include "app/XAPP_Control.h"
#include "app/XAPP_Window.h"

class CWindowAppStateControl
{
public:
  CWindowAppStateControl();
  
  int controlId;
  bool isVisible;
  CGUIInfoLabel label;
};

class CWindowAppStateContainer
{
public:
  CWindowAppStateContainer();
  CWindowAppStateContainer(const CWindowAppStateContainer& item);
  
  virtual ~CWindowAppStateContainer();

  const CWindowAppStateContainer& operator =(const CWindowAppStateContainer& item);

  int controlId;
  CStdString path;
  int selectedItem;
  bool isVisible;
  
  size_t guiListItemArraySize;
  CGUIListItem** guiListItemArray;
};

class CWindowAppState
{
public:
  CWindowAppState();
  virtual ~CWindowAppState();
  
  std::vector<CWindowAppStateContainer*> containers;
  std::vector<CWindowAppStateControl*> controls;
  int focusedControl;
};

/**
 * Implements a window in an app. handles loading of rss to the lists.
 */
class CGUIWindowApp : public CGUIWindow, public IBackgroundLoaderObserver
{
public:
	CGUIWindowApp(DWORD dwID, const CStdString &xmlFile, const CAppDescriptor appDescriptor);
  virtual ~CGUIWindowApp(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnItemLoaded(CFileItem* pItem);
  virtual bool OnAction(const CAction &action);
  virtual void AllocResources(bool forceLoad = false);
  virtual void FreeResources(bool forceUnLoad = false);
  void ClearStateStack();
  void PushWindowState();
  int GetNumberOfLoadingContainers() const;
  void SetContainerPath(DWORD controlId, CStdString& path);
  
  virtual void Render();

  void AddActionListener(XAPP::ActionListener* listener);
  void RemoveActionListener(XAPP::ActionListener* listener);
  void AddWindowListener(XAPP::WindowListener* listener);
  void RemoveWindowListener(XAPP::WindowListener* listener);
  void AddKeyListener(XAPP::KeyListener* listener);
  void RemoveKeyListener(XAPP::KeyListener* listener);

protected:
  void PopWindowState(bool restoreState);
  void PopToWindowState(bool restoreState, int numberInStack);
  void ResetWindowState();  
  void SaveWindowState();
  bool RestoreWindowState();
  CWindowAppState* GetWindowState();
  void SetWindowState(CWindowAppState* state);
  
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  void RegisterContainers(CWindowAppState* state = NULL);
  
  void GetAllControls(std::vector<CGUIControl* >& controls);
 
  void UpdateItemWithAppData(CFileItem* fileItem);

  CAppDescriptor m_appDescriptor;
  CStdString     m_mediaPath;
  std::stack<CWindowAppState*> m_windowStateHistory;
  CWindowAppState* m_savedState;
  bool m_isSavedState;
  int m_loadingContainers;
  
  CCriticalSection m_loadingContainersLock;

  std::vector<XAPP::ActionListener*> m_actionListeners;
  std::vector<XAPP::WindowListener*> m_windowListeners;
  std::vector<XAPP::KeyListener*> m_keyListeners;
};

class CGUIWindowAppWaitLoading : public IRunnable
{
  virtual void Run();
};

#endif /*GUIWINDOWBAPP_H_*/
