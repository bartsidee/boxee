
#include "GUIWindowApp.h"
#include "GUIBaseContainer.h"
#include "GUIWindowManager.h"
#include "Application.h"
#include "AppDescriptor.h"
#include "GUIDialogBoxeeMediaAction.h"
#include "GUIControl.h"
#include "GUIControlGroup.h"
#include <queue>
#include "GUIDialogProgress.h"
#include "Util.h"
#include "utils/log.h"
#include "utils/SingleLock.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "AppManager.h"
#include "GUILabelControl.h"

CGUIWindowApp::CGUIWindowApp(DWORD dwID, const CStdString &xmlFile, const CAppDescriptor appDescriptor) :
  CGUIWindow(dwID, xmlFile)
{
  m_appDescriptor = appDescriptor;
  m_mediaPath = m_appDescriptor.GetMediaPath();
  ClearStateStack();
  ResetControlStates();
  m_loadingContainers = 0;
  m_savedState = NULL;
  m_isSavedState = false;
}

CGUIWindowApp::~CGUIWindowApp(void)
{
  if (m_savedState)
    delete m_savedState;
  m_savedState = NULL;
}

void CGUIWindowApp::Render()
{
  XAPP::WindowEvent event;
  event.windowId = GetID();
  for (size_t i = 0; i < m_windowListeners.size(); i++)
  {
    m_windowListeners[i]->WindowRender(event);
  }


  g_Windowing.ClearBuffers(0, 0, 0, 1.0f);

  CGUIWindow::Render();
}

bool CGUIWindowApp::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_SAVE_STATE:
    {
      PushWindowState();
      return true;
    }

    case GUI_MSG_RESTORE_STATE:
    {
      bool resetAll = (message.GetParam1() == 1);
      if (resetAll)
      {
        ResetWindowState();
      }
      else
      {
        PopWindowState(true);        
      }
      return true;
    }

    case GUI_MSG_RESET_STATE:
    {
      int popTo = message.GetParam1();
      if (popTo == 0) // Pop only one
      {
        PopWindowState(false);
      }
      else
      {
        PopToWindowState(false, popTo);        
      }
      return true;
    }

    case GUI_MSG_LABEL_BIND:
    {
      CSingleLock lock(m_loadingContainersLock);
      m_loadingContainers--;
      bool ret = CGUIWindow::OnMessage(message);
      CFileItemList* pList = (CFileItemList*)message.GetPointer();
      if (pList)
        delete pList;
      
      // Remove adult content and geo locked content
      CGUIBaseContainer* pControl = (CGUIBaseContainer*) GetControl(message.GetControlId());
      if (pControl)
      {
        try
        {
          std::vector< CGUIListItemPtr >& items = pControl->GetItemsByRef();
          for (std::vector< CGUIListItemPtr >::iterator it = items.begin(); it != items.end(); /* BLANK */)
          {
            CFileItem* fileItem = dynamic_cast<CFileItem*> ((*it).get());
            if (fileItem && !fileItem->IsAllowed())
            {
               it = items.erase(it);
            } 
            else 
            {
              UpdateItemWithAppData(fileItem);
               ++it;
            }
          }
        }
        catch (const std::bad_cast& e)
        {
          // Failed cast...nothing to do much here
        }
      }
      
      return ret;
    }

    case GUI_MSG_WINDOW_DEINIT:
    {  
      CAppManager& mgr = CAppManager::GetInstance();
      CStdString strAppId = mgr.GetLastLaunchedId();
      
      if(message.GetParam1() == 0 && !strAppId.empty())
      {
        mgr.Close(mgr.GetLastLaunchedId());
      }
      break;
    }

    case GUI_MSG_LOAD_FAILED:
    {
      CSingleLock lock(m_loadingContainersLock);
      m_loadingContainers--;
      break;
    }

    case GUI_MSG_SET_CONTAINER_PATH:
    {
      CFileItem *pItem = (CFileItem *) message.GetPointer();
      message.SetPointer(NULL);
      if (!pItem)
        return true;

      // Get control id
      int iControl = message.GetControlId();

      const CGUIControl* pControl = GetControl(iControl);
      if (pControl && pControl->IsContainer())
      {
        SetContainerPath(pControl->GetID(), pItem->m_strPath);
      }

      delete pItem;

      return true;
    }

    case GUI_MSG_CLICKED:
    {
      {
        XAPP::KeyEvent event;
        event.windowId = GetID();
        event.key = XAPP::KeyEvent::XAPP_KEY_SELECT;
        for (size_t i = 0; i < m_keyListeners.size(); i++)
        {
          if (m_keyListeners[i]->KeyPressed(event))
            return true;
        }
      }

      int iControl = message.GetSenderId();
      int iAction = message.GetParam1();

      XAPP::ActionEvent event;
      event.controlId = iControl;
      event.windowId = GetID();
      for (size_t i = 0; i < m_actionListeners.size(); i++)
      {
        if (m_actionListeners[i]->ActionPerformed(event))
        {
          return true;
        }
      }

      // Check action type
      if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
      {
        const CGUIControl* pControl = GetControl(iControl);
        if (pControl && pControl->IsContainer())
        {
          CFileItem* pSelectedItem = (CFileItem*) ((CGUIBaseContainer*) pControl)->GetListItem(0).get();
          if (pSelectedItem)
          {
            pSelectedItem->Dump();
            if (pSelectedItem->IsPicture())
            {
              pSelectedItem->SetProperty("parentfolder", ((CGUIBaseContainer*) pControl)->GetPath());
              g_application.GetBoxeeItemsHistoryList().AddItemToHistory(*pSelectedItem);
              CLog::Log(LOGDEBUG, "CGUIWindowApp::OnMessage, PICTURES, run slideshow for path = %s",pSelectedItem->m_strPath.c_str());
              CGUIDialogBoxeeMediaAction::RunSlideshow(((CGUIBaseContainer*) pControl)->GetPath(),pSelectedItem->m_strPath);
            }
            else
            {
              CGUIDialogBoxeeMediaAction::ShowAndGetInput(pSelectedItem);
            }
          }
        }
      }

      break;
    }
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowApp::UpdateItemWithAppData(CFileItem* fileItem)
{
  if (!fileItem)
  {
    return;
  }
  
  //////////////////
  // update appid //
  //////////////////

  CStdString appId = m_appDescriptor.GetId();
  if (fileItem->GetProperty("appid").IsEmpty() && !appId.IsEmpty())
  {
    fileItem->SetProperty("appid",appId);
  }

  /////////////////////
  // update provider //
  /////////////////////

  CStdString appName = m_appDescriptor.GetName();
  if (fileItem->GetProperty("provider_source").IsEmpty() && !appName.IsEmpty())
  {
    fileItem->SetProperty("provider_source",appName);
  }
}

void CGUIWindowApp::SetContainerPath(DWORD controlId, CStdString& path)
{

  // We have to use a message here since this function is called directly from python code
  CGUIMessage msg(GUI_MSG_SET_PATH, GetID(), controlId);
  msg.SetLabel(path);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, GetID(), true);

    CSingleLock lock(m_loadingContainersLock);
  g_application.GetItemLoader().AddControl(GetID(), controlId, path);
    m_loadingContainers++;
  
  }

void CGUIWindowApp::OnInitWindow()
{
  XAPP::WindowEvent event;
  event.windowId = GetID();
  for (size_t i = 0; i < m_windowListeners.size(); i++)
  {
    m_windowListeners[i]->WindowOpening(event);
  }

  {
    CSingleLock lock(m_loadingContainersLock);
    m_loadingContainers = 0;
  }
	
  SetRunActionsManually();
  CGUIWindow::OnInitWindow();

  if (!RestoreWindowState())
  {
    RegisterContainers();

    CGUIWindowAppWaitLoading* job = new CGUIWindowAppWaitLoading();
    CUtil::RunInBG(job);
  }

  RunLoadActions();

  for (size_t i = 0; i < m_windowListeners.size(); i++)
  {
    m_windowListeners[i]->WindowOpened(event);
  }
}

void CGUIWindowApp::OnDeinitWindow(int nextWindowID)
{
  XAPP::WindowEvent event;
  event.windowId = GetID();
  for (size_t i = 0; i < m_windowListeners.size(); i++)
  {
    m_windowListeners[i]->WindowClosing(event);
  }

  SaveWindowState();
  RunUnloadActions();

  CGUIDialogProgress* progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (progress) 
  {
    g_application.getApplicationMessenger().CloseDialog(progress, true);
  }
  
  CGUIWindow::OnDeinitWindow(nextWindowID);

  for (size_t i = 0; i < m_windowListeners.size(); i++)
  {
    m_windowListeners[i]->WindowClosed(event);
  }
}

void CGUIWindowApp::RegisterContainers(CWindowAppState* state)
{
  std::vector<CGUIControl *> vecContainers;
  GetContainers(vecContainers);

  for (size_t i = 0; i < vecContainers.size(); i++)
  {
    CGUIBaseContainer* pContainer = (CGUIBaseContainer*) vecContainers[i];

    if (pContainer->GetPath().IsEmpty())
    {
      continue;
    }

    CGUIMessage clearmsg(GUI_MSG_LABEL_RESET, GetID(), pContainer->GetID(), 0, 0);
    g_windowManager.SendMessage(clearmsg);
    CStdString newPath = pContainer->GetPath();
    newPath.Replace(' ', '+');

    int iSelectedItem = -1;
    if (state)
    {
      for (size_t i = 0; i < state->containers.size(); i++)
      {
        if (state->containers[i]->controlId == pContainer->GetID())
        {
          iSelectedItem = state->containers[i]->selectedItem;
          break;
        }
      }
    }

    pContainer->SetPath(newPath);
    g_application.GetItemLoader().AddControl(GetID(), pContainer->GetID(), pContainer->GetPath(), SORT_METHOD_NONE, SORT_ORDER_NONE, iSelectedItem);
    CSingleLock lock(m_loadingContainersLock);
    m_loadingContainers++;
  }
}

int CGUIWindowApp::GetNumberOfLoadingContainers() const
{
  CSingleLock lock(m_loadingContainersLock);
  return m_loadingContainers;
}

void CGUIWindowApp::OnItemLoaded(CFileItem* pItem)
{
  if (!pItem)
    return;

  CGUIMessage winmsg(MSG_ITEM_LOADED, GetID(), 0);
  winmsg.SetPointer(new CFileItem(*pItem));
  g_windowManager.SendThreadMessage(winmsg, GetID());
}

bool CGUIWindowApp::OnAction(const CAction &action)
{
  if (action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    XAPP::KeyEvent event;
    event.windowId = GetID();
    event.key = XAPP::KeyEvent::XAPP_KEY_BACK;
    for (size_t i = 0; i < m_keyListeners.size(); i++)
    {
      if (m_keyListeners[i]->KeyPressed(event))
        return true;
    }

#ifdef HAS_EMBEDDED
    if (g_application.IsPlaying())
    {
      g_application.GetItemLoader().Resume();
    }
#endif
    if (m_windowStateHistory.empty())
    {
      g_windowManager.PreviousWindow();
      return true;
    }
    else
    {
      PopWindowState(true);
      return true;
    }
  }

  else if (action.id == ACTION_MOVE_DOWN)
  {
    XAPP::KeyEvent event;
    event.windowId = GetID();
    event.key = XAPP::KeyEvent::XAPP_KEY_DOWN;
    for (size_t i = 0; i < m_keyListeners.size(); i++)
    {
      if (m_keyListeners[i]->KeyPressed(event))
        return true;
    }
  }

  else if (action.id == ACTION_MOVE_UP)
  {
    XAPP::KeyEvent event;
    event.windowId = GetID();
    event.key = XAPP::KeyEvent::XAPP_KEY_UP;
    for (size_t i = 0; i < m_keyListeners.size(); i++)
    {
      if (m_keyListeners[i]->KeyPressed(event))
        return true;
    }
  }

  else if (action.id == ACTION_MOVE_LEFT)
  {
    XAPP::KeyEvent event;
    event.windowId = GetID();
    event.key = XAPP::KeyEvent::XAPP_KEY_LEFT;
    for (size_t i = 0; i < m_keyListeners.size(); i++)
    {
      if (m_keyListeners[i]->KeyPressed(event))
        return true;
    }
  }

  else if (action.id == ACTION_MOVE_RIGHT)
  {
    XAPP::KeyEvent event;
    event.windowId = GetID();
    event.key = XAPP::KeyEvent::XAPP_KEY_RIGHT;
    for (size_t i = 0; i < m_keyListeners.size(); i++)
    {
      if (m_keyListeners[i]->KeyPressed(event))
        return true;
    }
  }

  else if (action.id == ACTION_OSD_EXT_CLICK)
  {
    printf("***** EXT CLICK!!!!\n");
  }

  return CGUIWindow::OnAction(action);
}

void CGUIWindowApp::AllocResources(bool forceLoad /*= FALSE */)
{
  // Add the path where to find the app skin files
  g_TextureManager.AddTexturePath(m_mediaPath);
  CGUIWindow::AllocResources(forceLoad);
}

void CGUIWindowApp::FreeResources(bool forceUnLoad /* = false */)
{
  CGUIWindow::FreeResources(forceUnLoad);
  g_TextureManager.RemoveTexturePath(m_mediaPath);
}

void CGUIWindowApp::ClearStateStack()
{
  while (!m_windowStateHistory.empty())
  {
    m_windowStateHistory.pop();
  }

  m_isSavedState = false;
}

void CGUIWindowApp::PushWindowState()
{
  CLog::Log(LOGDEBUG,"CGUIWindowApp::PushWindowState - Enter function. Going to call GetWindowState(). [WindowHistorySize=%zu] (was)",m_windowStateHistory.size());
  
  CWindowAppState* currentState = GetWindowState();
  
  if(currentState)
    m_windowStateHistory.push(currentState);
  else
  {
    CLog::Log(LOGDEBUG,"CGUIWindowApp::PushWindowState - Calling to GetWindowState() returned a NULL CWindowAppState object. Not going to save it in WindowHistory. [WindowHistorySize=%zu] (was)",m_windowStateHistory.size());    
  }
}

void CGUIWindowApp::PopWindowState(bool restoreState)
{
  CLog::Log(LOGDEBUG,"CGUIWindowApp::PopWindowState - Enter function with [restoreState=%d]. [WindowHistorySize=%zu] (was)",restoreState,m_windowStateHistory.size());

  if (!m_windowStateHistory.empty())
  {
    CWindowAppState* lastState = m_windowStateHistory.top();
    m_windowStateHistory.pop();
    
    if (restoreState)
    {
      if(lastState)
      {
        CLog::Log(LOGDEBUG,"CGUIWindowApp::PopWindowState - Because [restoreState=%d] going ot call SetWindowState() with CWindowAppState object that was pop. [WindowHistorySize=%zu] (was)",restoreState,m_windowStateHistory.size());

        SetWindowState(lastState);
      }
      else
      {
        CLog::Log(LOGERROR,"CGUIWindowApp::PopWindowState - Can't call SetWindowState() because a NULL CWindowAppState object was pop from WindowStateHistory (was)");
      }
    }
  }
}

void CGUIWindowApp::PopToWindowState(bool restoreState, int numberInStack)
{
  CLog::Log(LOGDEBUG,"CGUIWindowApp::PopToWindowState - Enter function with [restoreState=%d][numberInStack=%d]. [WindowHistorySize=%zu] (was)",restoreState,numberInStack,m_windowStateHistory.size());

  bool gotState = false;
  CWindowAppState* lastState = NULL;
  while (m_windowStateHistory.size() > (size_t) numberInStack)
  {
    if(lastState)
    {
      delete lastState;
      lastState = NULL;
    }
    
    lastState = m_windowStateHistory.top();
    gotState = true;
    m_windowStateHistory.pop();
    
    CLog::Log(LOGDEBUG,"CGUIWindowApp::PopToWindowState - A NULL CWindowAppState object was pop from WindowStateHistory (was)");
  }

  if (restoreState && gotState)
  {
    if(lastState)
    {
      CLog::Log(LOGDEBUG,"CGUIWindowApp::PopToWindowState - Because [restoreState=%d] and [gotState=%d] going ot call SetWindowState() with CWindowAppState object that was pop. [WindowHistorySize=%zu] (was)",restoreState,gotState,m_windowStateHistory.size());

      SetWindowState(lastState);
    }
    else
    {
      CLog::Log(LOGERROR,"CGUIWindowApp::PopToWindowState - Can't call SetWindowState() because a NULL CWindowAppState object was pop from WindowStateHistory (was)");
    }
  }
}

void CGUIWindowApp::ResetWindowState()
{
  PopToWindowState(true, 0);
}

void CGUIWindowApp::SaveWindowState()
{
  if (m_savedState)
    delete m_savedState;
  m_savedState = NULL;
  
  m_savedState = GetWindowState();
  m_isSavedState = true;  
}

bool CGUIWindowApp::RestoreWindowState()
{
  CLog::Log(LOGDEBUG,"CGUIWindowApp::RestoreWindowState - Enter function. [m_isSavedState=%d] (was)",m_isSavedState);
  
  if (m_isSavedState)
  {
    SetWindowState(m_savedState);
    m_savedState = NULL;
    
    return true;
  }

  return false;
}

CWindowAppState* CGUIWindowApp::GetWindowState()
{
  CWindowAppState* result = new CWindowAppState();

  // For each container remember the path
  std::vector<CGUIControl *> containers;
  GetContainers(containers);
  for (size_t i = 0; i < containers.size(); i++)
  {
    CWindowAppStateContainer* stateContainer = new CWindowAppStateContainer();
    
    //CLog::Log(LOGDEBUG,"CGUIWindowApp::GetWindowState - [%lu] Allocate CWindowAppStateContainer (was)",i);

    stateContainer->controlId = containers[i]->GetID();
    stateContainer->isVisible = containers[i]->IsVisible();
    
    stateContainer->path = ((CGUIBaseContainer*) containers[i])->GetPath();
    if ((stateContainer->path).length() == 0)
    {
      /////////////////////////////////////////////////////////////////////
      // Path is empty -> Need to save the window CGUIListItemPtr vector //
      /////////////////////////////////////////////////////////////////////
      
      std::vector<CGUIListItemPtr> containerGuiListItemsVec = ((CGUIBaseContainer*) containers[i])->GetItems();
      
      stateContainer->guiListItemArraySize = containerGuiListItemsVec.size();
      stateContainer->guiListItemArray = new CGUIListItem*[stateContainer->guiListItemArraySize];

      for(size_t k = 0; k < stateContainer->guiListItemArraySize; k++)
      {
        stateContainer->guiListItemArray[k] = new CFileItem(*((CFileItem*) containerGuiListItemsVec[k].get()));        
        //CLog::Log(LOGDEBUG,"CGUIWindowApp::GetWindowState - [%lu][%lu] Allocate CFileItem for [label=%s] (was)",i,k,((stateContainer->guiListItemArray[k])->GetLabel()).c_str());
      }
    }

    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), stateContainer->controlId);
    OnMessage(msg);
    stateContainer->selectedItem = msg.GetParam1();

    result->containers.push_back(stateContainer);
    
    //CLog::Log(LOGDEBUG,"CGUIWindowApp::GetWindowState - [%lu] After push CWindowAppStateContainer [controlId=%d][selectedItem=%d]. [ContainersVecSize=%lu] (was)",i,stateContainer->controlId,stateContainer->selectedItem,(result->containers).size());
  }

  // For each control remember whether it's visible or not
  std::vector<CGUIControl *> controls;
  GetAllControls(controls);
  for (size_t i = 0; i < controls.size(); i++)
  {
    if (controls[i]->GetID() == 0)
    {
      continue;
    }

    if (controls[i]->GetVisibleCondition() != 0)
    {
      continue;
    }

    CWindowAppStateControl* stateControl = new CWindowAppStateControl();
    
    //CLog::Log(LOGDEBUG,"CGUIWindowApp::GetWindowState - [%d] Allocate CWindowAppStateControl (was)",i);

    stateControl->controlId = controls[i]->GetID();
    stateControl->isVisible = controls[i]->IsVisible();

    if (controls[i]->GetControlType() == CGUIControl::GUICONTROL_LABEL)
      stateControl->label = ((CGUILabelControl*)controls[i])->GetInfo();
    result->controls.push_back(stateControl);
    
    //CLog::Log(LOGDEBUG,"CGUIWindowApp::GetWindowState - [%d] After push CWindowAppStateControl. [ControlsVecSize=%d] (was)",i,(result->controls).size());
  }

  SetInitialVisibility();

  // Remember the focused control
  result->focusedControl = GetFocusedControlID();

  //CLog::Log(LOGDEBUG,"CGUIWindowApp::GetWindowState - Exit function and return CWindowAppState. [ContainersVecSize=%lu][ControlsVecSize=%lu] (was)",(result->containers).size(),(result->controls).size());

  return result;
}

void CGUIWindowApp::SetWindowState(CWindowAppState* state)
{
  if(state)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowApp::SetWindowState - Enter function with CWindowAppState. [ContainersVecSize=%zu][ControlsVecSize=%zu] (was)",(state->containers).size(),(state->controls).size());
  }

  int selectedItem = 0;

  // Set path for each container
  for (size_t i = 0; i < (state->containers).size(); i++)
  {
    const CGUIControl* pControl = GetControl((state->containers[i])->controlId);
    if (pControl && pControl->IsContainer())
    {
      ((CGUIBaseContainer*) pControl)->SetVisible((state->containers[i])->isVisible);

      ((CGUIBaseContainer*) pControl)->SetPath((state->containers[i])->path);
      if(((state->containers[i])->path).length() == 0)
      {
        ////////////////////////////////////////////////////////////////////////
        // Path is empty -> Need to restore the window CGUIListItemPtr vector //
        ////////////////////////////////////////////////////////////////////////
        
        if((state->containers[i])->guiListItemArraySize > 0)
        {
          std::vector<CGUIListItemPtr> savedContainerGuiListItemsArray;
          
          for(size_t k = 0; k < (state->containers[i])->guiListItemArraySize; k++)
          {
            CFileItem* fileItem = new CFileItem(*((CFileItem*) state->containers[i]->guiListItemArray[k]));
            savedContainerGuiListItemsArray.push_back(CGUIListItemPtr(fileItem));
          }  
          
          ((CGUIBaseContainer*) pControl)->SetItems(savedContainerGuiListItemsArray);

          SET_CONTROL_FOCUS(state->containers[i]->controlId, state->containers[i]->selectedItem);

          if (state->containers[i]->controlId == state->focusedControl)
            selectedItem = state->containers[i]->selectedItem;
        }
      }
    }
  }

  // Load all of them
  RegisterContainers(state);

  // Set visibility to all controls
  for (size_t i = 0; i < (state->controls).size(); i++)
  {
    if (state->controls[i]->isVisible)
    {
      SET_CONTROL_VISIBLE(state->controls[i]->controlId);
    }
    else
    {
      SET_CONTROL_HIDDEN(state->controls[i]->controlId);
    }

    if (GetControl(state->controls[i]->controlId)->GetControlType() == CGUIControl::GUICONTROL_LABEL)
       ((CGUILabelControl*)GetControl(state->controls[i]->controlId))->SetInfo(state->controls[i]->label);
  }

  // Restore the focused control
  SetInitialVisibility();
  SET_CONTROL_VISIBLE(state->focusedControl);
  SetInitialVisibility();

  SET_CONTROL_FOCUS(state->focusedControl, selectedItem);
  
  if(state)
  {
    delete state;
    state = NULL;
  }
}

void CGUIWindowApp::GetAllControls(std::vector<CGUIControl*>& controls)
{
  std::vector<CGUIControl*> temp;
  std::queue<CGUIControlGroup*> groups;

  bool firstTime = true;

  while (groups.size() > 0 || firstTime)
  {
    if (firstTime)
    {
      GetControls(temp);
      firstTime = false;
    }
    else
    {
      groups.front()->GetControls(temp);
      groups.pop();
    }

    for (size_t i = 0; i < temp.size(); i++)
    {
      CGUIControl* pControl = temp[i];
      if (pControl->GetControlType() == CGUIControl::GUICONTROL_GROUP || pControl->GetControlType()
          == CGUIControl::GUICONTROL_GROUPLIST || pControl->GetControlType() == CGUIControl::GUICONTROL_LISTGROUP)
      {
        groups.push((CGUIControlGroup*) pControl);
      }

      controls.push_back(pControl);
    }
  }
}

void CGUIWindowApp::AddActionListener(XAPP::ActionListener* listener)
{
  m_actionListeners.push_back(listener);
}

void CGUIWindowApp::RemoveActionListener(XAPP::ActionListener* listener)
{
  for (size_t i = 0; i < m_actionListeners.size(); i++)
  {
    if (m_actionListeners[i] == listener)
    {
      m_actionListeners.erase(m_actionListeners.begin() + i);
      break;
    }
  }
}

void CGUIWindowApp::AddWindowListener(XAPP::WindowListener* listener)
{
  m_windowListeners.push_back(listener);
}

void CGUIWindowApp::RemoveWindowListener(XAPP::WindowListener* listener)
{
  for (size_t i = 0; i < m_windowListeners.size(); i++)
  {
    if (m_windowListeners[i] == listener)
    {
      m_windowListeners.erase(m_windowListeners.begin() + i);
      break;
    }
  }
}

void CGUIWindowApp::AddKeyListener(XAPP::KeyListener* listener)
{
  m_keyListeners.push_back(listener);
}

void CGUIWindowApp::RemoveKeyListener(XAPP::KeyListener* listener)
{
  for (size_t i = 0; i < m_keyListeners.size(); i++)
  {
    if (m_keyListeners[i] == listener)
    {
      m_keyListeners.erase(m_keyListeners.begin() + i);
      break;
    }
  }
}

void CGUIWindowAppWaitLoading::Run()
{
  CGUIWindowApp* pWindow = (CGUIWindowApp*) g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
  int nRounds = 0;
  while (pWindow->GetNumberOfLoadingContainers() > 0)
  {
    Sleep(200);
    if ((pParent && !pParent->IsRunning()) || (nRounds >= 18000 /* 1 min */))
    {
      break;
    }
    ++nRounds;
  }
}

CWindowAppState::CWindowAppState()
{
  containers.clear();
  controls.clear();
  focusedControl = -1;
}

CWindowAppState::~CWindowAppState()
{
  for(size_t k = 0; k < containers.size(); k++)
  {            
    delete containers[k];
  }
  
  containers.clear();

  for(size_t k = 0; k < controls.size(); k++)
  {            
    delete controls[k];
  }
  
  controls.clear();
}

CWindowAppStateControl::CWindowAppStateControl()
{
  controlId = -1;
  isVisible = false;  
}

CWindowAppStateContainer::CWindowAppStateContainer()
{
  controlId = -1;
  path = "";
  selectedItem = -1;
  isVisible = false;
  guiListItemArraySize = -1;
  guiListItemArray = NULL;
}

CWindowAppStateContainer::CWindowAppStateContainer(const CWindowAppStateContainer& item)
{
  controlId = -1;
  path = "";
  selectedItem = -1;
  isVisible = false;
  guiListItemArraySize = -1;
  guiListItemArray = NULL;

  *this = item;
}

CWindowAppStateContainer::~CWindowAppStateContainer()
{
  if(guiListItemArray)
  {
    for(size_t k = 0; k < guiListItemArraySize; k++)
    {            
      delete guiListItemArray[k];
      guiListItemArray[k] = NULL;
    }
    
    delete[] guiListItemArray;
    guiListItemArray = NULL;
  }
}

const CWindowAppStateContainer& CWindowAppStateContainer::operator =(const CWindowAppStateContainer& item)
{
  if(&item == this)
  {
    return * this;
  }
  
  controlId = item.controlId;
  path = item.path;
  selectedItem = item.selectedItem;
  isVisible = item.isVisible;
  guiListItemArraySize = item.guiListItemArraySize;
  guiListItemArray = item.guiListItemArray;
  
  return *this;
}
