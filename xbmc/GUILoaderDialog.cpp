
#include "GUILoaderDialog.h"
#include "GUIWindowManager.h"
#include "Application.h"
#include "Util.h"
#include "utils/log.h"
#include "GUIUserMessages.h"
#include "ItemLoader.h"

CGUILoaderDialog::CGUILoaderDialog(DWORD dwID, const CStdString &xmlFile) : CGUIDialog(dwID, xmlFile)
{
}

CGUILoaderDialog::~CGUILoaderDialog()
{
}

bool CGUILoaderDialog::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() ) 
  {
    case GUI_MSG_UPDATE:
      if (message.GetSenderId() == GetID()) 
      {
        RegisterContainers();
      }
      break;
      
    case GUI_MSG_SET_CONTAINER_PATH:
    {
      CFileItem *pItem = (CFileItem *)message.GetPointer();
      message.SetPointer(NULL);
      if (!pItem)
        return true;
        
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
    
    case GUI_MSG_LABEL_BIND:
    {
      if (GetID() == message.GetSenderId())
      {
        bool result = CGUIDialog::OnMessage(message);
        RestoreControlStates();
        CFileItemList* pList = (CFileItemList*)message.GetPointer();
        if (pList)
          delete pList;
      
        CGUIControl* pControl = (CGUIControl*) GetControl(message.GetControlId());
        if (pControl && pControl->IsContainer())
        {
          OnContainersLoadSuccess(*(((CGUIBaseContainer*)pControl)));
        }
      
        return result;
      }
    }
    
    case GUI_MSG_LOAD_FAILED:
    {
      CGUIControl* pControl = (CGUIControl*) GetControl(message.GetControlId());
      if (pControl && pControl->IsContainer())
      {
        OnContainersLoadFailed(*(((CGUIBaseContainer*)pControl)));
      }
      
      break;
    }    
  }
  
  return CGUIDialog::OnMessage(message);
}

void CGUILoaderDialog::OnInitWindow()
{
  CGUIDialog::OnInitWindow();
  RegisterContainers();
}

void CGUILoaderDialog::RegisterContainers()
{
  CLog::Log(LOGDEBUG,"GUILoaderWindow::RegisterContainers");
  
  std::vector<CGUIControl *> vecContainers;
  GetContainers(vecContainers);
  
  for (size_t i = 0; i < vecContainers.size(); i++) 
  {
    RegisterContainer(vecContainers[i]->GetID());
  }
}

void CGUILoaderDialog::RegisterContainer(DWORD containerId)
{
  CLog::Log(LOGDEBUG,"GUILoaderWindow::RegisterContainer: %d", containerId);
  if (GetControl(containerId) && GetControl(containerId)->IsContainer())
  {
    CGUIBaseContainer* pContainer = (CGUIBaseContainer*) GetControl(containerId);
      
    if (!pContainer->GetPath().IsEmpty())
    {
      CGUIMessage clearmsg(GUI_MSG_LABEL_RESET, GetID(), pContainer->GetID(), 0, 0);
      g_windowManager.SendThreadMessage(clearmsg);
      g_application.GetItemLoader().AddControl(GetID(), pContainer->GetID(), pContainer->GetPath(), SORT_METHOD_NONE, SORT_ORDER_NONE);
    }
  }
}

void CGUILoaderDialog::OnContainersLoadSuccess(CGUIBaseContainer& container)
{
  CUtil::FilterUnallowedItems(container.GetItemsByRef());  
}

void CGUILoaderDialog::OnContainersLoadFailed(CGUIBaseContainer& container)
{
}
