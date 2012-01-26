
#include "GUILoaderWindow.h"
#include "GUIWindowManager.h"
#include "Application.h"
#include "Util.h"
#include "utils/log.h"
#include "GUIUserMessages.h"
#include "ItemLoader.h"
#include "DirectoryCache.h"

CGUILoaderWindow::CGUILoaderWindow(DWORD dwID, const CStdString &xmlFile) : CGUIWindow(dwID, xmlFile)
{
}

CGUILoaderWindow::~CGUILoaderWindow()
{
}

bool CGUILoaderWindow::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() ) 
  {
  case GUI_MSG_UPDATE:
  {
    if (message.GetSenderId() == GetID())
    {
      RegisterContainers();
    }
  }
  break;
  case GUI_MSG_SET_CONTAINER_PATH:
  {
    CFileItem *pItem = (CFileItem *)message.GetPointer();
    message.SetPointer(NULL);
    if (!pItem)
    {
      return true;
    }

    // Get control id
    int iControl = message.GetControlId();

    const CGUIControl* pControl = GetControl(iControl);
    if (pControl && pControl->IsContainer())
    {
      ((CGUIBaseContainer*)pControl)->SetPath(pItem->m_strPath);
    }

    delete pItem;

    RegisterContainers();
    return true;
  }
  break;
  case GUI_MSG_LABEL_BIND:
  {
    bool result = CGUIWindow::OnMessage(message);
    RestoreControlStates();
    CFileItemList* pList = (CFileItemList*)message.GetPointer();

    if (pList)
    {
      delete pList;
    }

    CGUIControl* pControl = (CGUIControl*) GetControl(message.GetControlId());
    if (pControl && pControl->IsContainer())
    {
      OnContainersLoadSuccess(*(((CGUIBaseContainer*)pControl)));
    }

    return result;
  }
  break;
  case GUI_MSG_LOAD_FAILED:
  {
    CGUIControl* pControl = (CGUIControl*) GetControl(message.GetControlId());
    if (pControl && pControl->IsContainer())
    {
      OnContainersLoadFailed(*(((CGUIBaseContainer*)pControl)));
    }
  }
  break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUILoaderWindow::OnInitWindow()
{
  CGUIWindow::OnInitWindow();  
  RegisterContainers();  
}

void CGUILoaderWindow::RegisterContainers()
{
  CLog::Log(LOGDEBUG,"GUILoaderWindow::RegisterContainers");

  std::vector<CGUIControl *> vecContainers;
  GetContainers(vecContainers);

  for (size_t i = 0; i < vecContainers.size(); i++) 
  {
    RegisterContainer(vecContainers[i]->GetID());
  }
}

void CGUILoaderWindow::RegisterContainer(DWORD containerId)
{
  CLog::Log(LOGDEBUG,"GUILoaderWindow::RegisterContainer: %d", containerId);
  if (GetControl(containerId) && GetControl(containerId)->IsContainer())
  {
    CGUIBaseContainer* pContainer = (CGUIBaseContainer*) GetControl(containerId);

    if (pContainer && !pContainer->GetPath().IsEmpty())
    {
      CGUIMessage clearmsg(GUI_MSG_LABEL_RESET, GetID(), pContainer->GetID(), 0, 0);
      g_windowManager.SendMessage(clearmsg);
      g_application.GetItemLoader().AddControl(GetID(), pContainer->GetID(), pContainer->GetPath());
    }
  }
}

void CGUILoaderWindow::ReloadContainer(DWORD containerId, bool clearCacheFirst)
{
  if (clearCacheFirst)
  {
    CGUIBaseContainer* pContainer = (CGUIBaseContainer*) GetControl(containerId);

    if (pContainer && !pContainer->GetPath().IsEmpty())
    {
      g_directoryCache.ClearDirectory(pContainer->GetPath());
    }
    else
    {
      CLog::Log(LOGWARNING,"CGUILoaderWindow::ReloadContainer - FAILED to clear cache first. [containerId=%d] (home)",containerId);
    }
  }

  return RegisterContainer(containerId);
}

void CGUILoaderWindow::OnContainersLoadSuccess(CGUIBaseContainer& container)
{
  CUtil::FilterUnallowedItems(container.GetItemsByRef());  
}

void CGUILoaderWindow::OnContainersLoadFailed(CGUIBaseContainer& container)
{

}
