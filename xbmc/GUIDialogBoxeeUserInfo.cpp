
#include "FileItem.h"
#include "GUIWindowManager.h"
#include "GUIDialogBoxeeUserInfo.h"
#include "GUIDialogFileBrowser.h"
#include "GUIDialogContextMenu.h"

#include "MediaManager.h"
#include "Util.h"
#include "GUIPassword.h"
#include "Picture.h"
#include "BoxeeUtils.h"
#include "Application.h"
#include "FileSystem/Directory.h"
#include "lib/libBoxee/bxutils.h"
#include "GUIWindowBoxeeBrowse.h"
#include "GUIDialogBoxeeMediaAction.h"
#include "GUIImage.h"
#include "utils/log.h"
#include "GUIUserMessages.h"
#include "ItemLoader.h"

using namespace XFILE;
using namespace DIRECTORY;
using namespace BOXEE;

#define INFO_USER_NAME 11
#define INFO_USER_IMAGE 20

#define INFO_USER_ACTIONS 30
#define BTN_ACTIONS 31
#define INFO_USER_FRIENDS 40
#define BTN_FRIENDS 41

#define INFO_USER_LABEL 91
#define INFO_USER_BDAY 92
#define INFO_USER_LOCATION 93
#define INFO_USER_DESC 94

CGUIDialogBoxeeUserInfo::CGUIDialogBoxeeUserInfo(void)
: CGUIWindow(WINDOW_DIALOG_BOXEE_USER_INFO, "boxee_user_profile.xml")
{
	m_bUserThumbLoaded = false;
	m_userThumbLoader = NULL;
	Clear();
}

CGUIDialogBoxeeUserInfo::~CGUIDialogBoxeeUserInfo(void)
{
	if (m_userThumbLoader)
	{
		m_userThumbLoader->SetObserver(NULL);
		m_userThumbLoader->StopThread(true);
	}
}

bool CGUIDialogBoxeeUserInfo::OnAction(const CAction &action)
{
  if (action.id == ACTION_PARENT_DIR)
  {
    OnBack();
    return true;
  }
  else if (action.id == ACTION_PREVIOUS_MENU)
  {
    OnBack();
    return true;
  }
  return CGUIWindow::OnAction(action);
}

void CGUIDialogBoxeeUserInfo::OnBack()
{
  if (m_history.empty()) {
    g_windowManager.PreviousWindow();
  }
  else {
    Friend lastFriend = m_history.back();
    m_currentFriend = lastFriend;
    m_history.pop_back();
    UpdateWindow();
  }
}

bool CGUIDialogBoxeeUserInfo::OnMessage(CGUIMessage &message)
{
	switch (message.GetMessage())
	{
	case GUI_MSG_WINDOW_DEINIT:
	{
	  // Need to save the current m_strId in case this GUI_MSG_WINDOW_DEINIT
	  // is because of hit on the BTN_ACTIONS or BTN_FRIENDS.
	  // (the m_strId is going to be reset in Clear() and we need its value for handling BTN_ACTIONS or BTN_FRIENDS)
	  if (leavingForGood) {
	  	  Clear();
	  }
	  
	  leavingForGood = true;

		if (m_userThumbLoader)
		{
			m_userThumbLoader->StopThread(true);
		}

		m_userThumbLoader = NULL;
	}
	break;
	case MSG_ITEM_LOADED:
	{

		CFileItem *pItem = (CFileItem *)message.GetPointer();
		message.SetPointer(NULL);

		if (!pItem)
		{
			return true;
		}

		CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUserInfo::OnMessage, MSG_ITEM_LOADED, label = %s", pItem->GetLabel().c_str());
		CGUIImage *pImage = (CGUIImage*)GetControl(INFO_USER_IMAGE);

		if (pImage && pImage->GetFileName() == pItem->m_strPath && !pItem->GetCachedPictureThumb().IsEmpty())
		{
			pImage->FreeResources();
			pImage->SetFileName(pItem->GetCachedPictureThumb());
		}

		delete pItem;
	}
	return true;
	
	case GUI_MSG_UPDATE:
	{
		CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUserInfo::OnMessage, NEWUI, GUI_MSG_UPDATE");

		if (message.GetSenderId() == WINDOW_DIALOG_BOXEE_USER_INFO)
		{
			CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUserInfo::OnMessage, NEWUI, GUI_MSG_UPDATE, sender = WINDOW_DIALOG_BOXEE_USER_INFO");
			if (!m_bUserThumbLoaded && !m_currentFriend.m_strThumb.IsEmpty())
			{
				m_bUserThumbLoaded = true;
				if (m_userThumbLoader)
					m_userThumbLoader->StopThread(true);
				m_userThumbLoader = NULL;

				CFileItemPtr pFile ( new CFileItem() );
				pFile->m_strPath = m_currentFriend.m_strThumb;
				pFile->SetCachedPictureThumb();

				m_userThumbLoader = new CPictureThumbLoader;
				CFileItemList *pList = new CFileItemList;
				pList->Add(pFile);
				m_userThumbLoader->SetObserver(this);
				m_userThumbLoader->Load(*pList);
			}
			
			SetupScreen();
		
		}
	}
  break;
	case GUI_MSG_CLICKED:
	{
		int iControl = message.GetSenderId();
		int iAction = message.GetParam1();
		switch (iControl)
		{
		case INFO_USER_FRIENDS:
		{
		  if(iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
		  {
			  OnFriendsListClick(iAction);
		  }
		}
    break;
		case INFO_USER_ACTIONS:
		{
          if(iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
          {
            OnActionsClick(iAction);
          }
		}
		break;

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
	}

	return CGUIWindow::OnMessage(message);
}

void CGUIDialogBoxeeUserInfo::OnFriendsListClick(int nAction)
{
  CLog::Log(LOGDEBUG,"Enter to CGUIDialogBoxeeUserInfo::OnFriendsListClick with [action=%d] (fr) (ac)",nAction);
 
  // Get the item that was clicked and print its details
  CGUIBaseContainer* pContainer = (CGUIBaseContainer*)GetControl(INFO_USER_FRIENDS);
  CGUIListItemPtr pSelectedItem = pContainer->GetListItem(0);

  if (pSelectedItem == NULL)
    return;
  
  // Current friend is history
  m_history.push_back(m_currentFriend);
  
  Friend newFriend;

  newFriend.m_strId = pSelectedItem->GetProperty("userid");
  newFriend.m_strName = pSelectedItem->GetLabel();
  newFriend.m_strThumb = pSelectedItem->GetThumbnailImage();
  newFriend.m_strBDay = pSelectedItem->GetProperty("birthday");
  newFriend.m_strGender = pSelectedItem->GetProperty("gender");
  newFriend.m_strLocation= pSelectedItem->GetProperty("location");
  newFriend.m_strDesc= pSelectedItem->GetProperty("description");
  
  m_currentFriend = newFriend;
  
  m_bUserThumbLoaded = false;
  
  UpdateWindow();
}

void CGUIDialogBoxeeUserInfo::OnActionsClick(int nAction)
{
  leavingForGood = false;
  CGUIBaseContainer* pContainer = (CGUIBaseContainer*)GetControl(INFO_USER_ACTIONS);
  CGUIListItemPtr pSelectedItem = pContainer->GetListItem(0);

  if (pSelectedItem == NULL)
    return;
  
  CFileItem* item = (CFileItem*)pSelectedItem.get();
  item->Dump();
  
  CGUIDialogBoxeeMediaAction::ShowAndGetInput(item);
  
}

void CGUIDialogBoxeeUserInfo::Clear()
{
	EnterCriticalSection(m_lock);	
	m_currentFriend.m_strId = "";
	m_currentFriend.m_strName = "";
	m_currentFriend.m_strThumb = "";
	m_currentFriend.m_strBDay = "";
	m_currentFriend.m_strGender = "";
	m_currentFriend.m_strLocation= "";
	m_currentFriend.m_strDesc= "";
	LeaveCriticalSection(m_lock);
}

void CGUIDialogBoxeeUserInfo::LoadFromFileItem(const CFileItem* pItem)
{
  EnterCriticalSection(m_lock);
  m_currentFriend.m_strId = pItem->GetProperty("userid");
  m_currentFriend.m_strName = pItem->GetLabel();
  m_currentFriend.m_strThumb = pItem->GetProperty("userthumb");
  m_currentFriend.m_strBDay = pItem->GetProperty("birthday");
  m_currentFriend.m_strGender = pItem->GetProperty("gender");
  m_currentFriend.m_strLocation= pItem->GetProperty("location");
  m_currentFriend.m_strDesc= pItem->GetProperty("description");

  LeaveCriticalSection(m_lock);
}

void CGUIDialogBoxeeUserInfo::LoadFromBoxeeObj(const BOXEE::BXObject &userObj)
{
	EnterCriticalSection(m_lock);
	m_currentFriend.m_strId = userObj.GetID();
	m_currentFriend.m_strName = userObj.GetValue(MSG_KEY_NAME);
	m_currentFriend.m_strThumb = userObj.GetValue(MSG_KEY_THUMB);
	m_currentFriend.m_strBDay = userObj.GetValue(MSG_KEY_BDAY);
	m_currentFriend.m_strGender = userObj.GetValue(MSG_KEY_GENDER);
	m_currentFriend.m_strLocation= userObj.GetValue(MSG_KEY_LOCATION);
	m_currentFriend.m_strDesc= userObj.GetValue(MSG_KEY_DESCRIPTION);

	LeaveCriticalSection(m_lock);
}

//bool CGUIDialogBoxeeUserInfo::Show(const BXObject &userObj)
//{
//	CGUIDialogBoxeeUserInfo *dialog = (CGUIDialogBoxeeUserInfo *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_USER_INFO);
//	if (!dialog) return false;
//
//	dialog->LoadFromBoxeeObj(userObj);
//
//	dialog->DoModal();
//	return true;
//}

void CGUIDialogBoxeeUserInfo::SetUserId(const CStdString& strUserId)
{
  m_currentFriend.m_strId = strUserId;
}

//bool CGUIDialogBoxeeUserInfo::Show(const CFileItem* pUserItem)
//{
//  CGUIDialogBoxeeUserInfo *dialog = (CGUIDialogBoxeeUserInfo *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_USER_INFO);
//  if (!dialog) return false;
//
//  dialog->LoadFromFileItem(pUserItem);
//
//  dialog->DoModal();
//  return true;
//}

void CGUIDialogBoxeeUserInfo::SetupScreen()
{
	SET_CONTROL_LABEL(INFO_USER_NAME,m_currentFriend.m_strName);
	SET_CONTROL_LABEL(INFO_USER_LABEL,m_currentFriend.m_strName);
	SET_CONTROL_LABEL(INFO_USER_BDAY,m_currentFriend.m_strBDay);
	SET_CONTROL_LABEL(INFO_USER_LOCATION,m_currentFriend.m_strLocation);
	SET_CONTROL_LABEL(INFO_USER_DESC,m_currentFriend.m_strDesc);


//	if (m_strGender == "male")
//	{
//		SET_CONTROL_HIDDEN(INFO_USER_GENDER_FEMALE);
//		SET_CONTROL_VISIBLE(INFO_USER_GENDER_MALE);
//	}
//	else
//	{
//		SET_CONTROL_HIDDEN(INFO_USER_GENDER_MALE);
//		SET_CONTROL_VISIBLE(INFO_USER_GENDER_FEMALE);
//	}

	CGUIImage *pImage = (CGUIImage*)GetControl(INFO_USER_IMAGE);
	if (pImage && !m_currentFriend.m_strThumb.IsEmpty())
	{
		pImage->FreeResources();
		pImage->SetFileName(m_currentFriend.m_strThumb);
	}
}

void CGUIDialogBoxeeUserInfo::OnInitWindow()
{
  CGUIWindow::OnInitWindow();
 
  leavingForGood = true;
  if (m_currentFriend.m_strId == "") {
    // if no user id was set use the current user
    BOXEE::BXObject obj;
    BOXEE::Boxee::GetInstance().GetCurrentUser(obj);  
    LoadFromBoxeeObj(obj);
    RegisterContainers();
    m_bUserThumbLoaded = false;

    CGUIMessage winmsg(GUI_MSG_UPDATE, GetID(), 0);
    g_windowManager.SendThreadMessage(winmsg);
  }
  else {
    UpdateWindow();
  }
}

void CGUIDialogBoxeeUserInfo::UpdateWindow()
{
  CLog::Log(LOGDEBUG,"Enter to CGUIDialogBoxeeUserInfo::UpdateWindow with [id=%s] (fr) (ac)",m_currentFriend.m_strId.c_str());
  const CGUIControl* pControl = GetControl(INFO_USER_FRIENDS);
  if (pControl && pControl->IsContainer())
  {
    ((CGUIBaseContainer*)pControl)->SetPath("friends://user/" + m_currentFriend.m_strId);
  }
  
  pControl = GetControl(INFO_USER_ACTIONS);
  if (pControl && pControl->IsContainer())
  {
    ((CGUIBaseContainer*)pControl)->SetPath("actions://user/" + m_currentFriend.m_strId);
  }
  RegisterContainers();

  m_bUserThumbLoaded = false;

  CGUIMessage winmsg(GUI_MSG_UPDATE, GetID(), 0);
  g_windowManager.SendThreadMessage(winmsg);
}

void CGUIDialogBoxeeUserInfo::OnItemLoaded(CFileItem* pItem)
{
	CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUserInfo::OnItemLoaded, NEWUI, label = %s", pItem->GetLabel().c_str());
	if (!pItem)
		return;

	CGUIMessage winmsg(MSG_ITEM_LOADED, GetID(), 0);
	winmsg.SetPointer(new CFileItem(*pItem));
	g_windowManager.SendThreadMessage(winmsg, GetID());
}

// TODO: Framework function, consider redesign in order to avoid code duplication
void CGUIDialogBoxeeUserInfo::RegisterContainers()
{
  //CLog::Log(LOGDEBUG,"Enter to CGUIDialogBoxeeUserInfo::RegisterContainers with [loadCurrentUser=%ld] (fr) (ac)",loadCurrentUser,GetID());

	// Go over all containers and add them to the loader
	// TODO_ Add support for a flag that indicates whether the container should be added to the loader
	// g_application.GetItemLoader().AddControl(GetID(), 0, strPath);
	std::vector<CGUIControl *> vecContainers;
	GetContainers(vecContainers);

	for (size_t i = 0; i < vecContainers.size(); i++)
	{
		CGUIBaseContainer* pContainer = (CGUIBaseContainer*) vecContainers[i];

	  CGUIMessage clearmsg(GUI_MSG_LABEL_RESET, GetID(), pContainer->GetID(), 0, 0);
	  g_windowManager.SendThreadMessage(clearmsg);

	  g_application.GetItemLoader().AddControl(GetID(), pContainer->GetID(), pContainer->GetPath(), SORT_METHOD_LABEL, SORT_ORDER_ASC);
	}
}

