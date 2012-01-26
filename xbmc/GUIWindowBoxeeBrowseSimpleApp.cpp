#include "GUIWindowBoxeeBrowseSimpleApp.h"
#include "utils/log.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "GUIDialogBoxeeMainMenu.h"
#include "Application.h"
#include "ItemLoader.h"


#define ITEM_SUMMARY    9018
#define ITEM_SUMMARY_FLAG "item-summary"


CGUIBrowseSimpleAppState::CGUIBrowseSimpleAppState(CGUIWindowBoxeeBrowse *pWindow) : CBrowseWindowStateWithHistory(pWindow)
{

}

CGUIBrowseSimpleAppState::~CGUIBrowseSimpleAppState()
{

}

CBrowseWindowStateWithHistory* CGUIBrowseSimpleAppState::Clone()
{
  return (new CGUIBrowseSimpleAppState(m_pWindow));
}

bool CGUIBrowseSimpleAppState::OnBack()
{
  if (!CBrowseWindowStateWithHistory::OnBack())
  {
    g_windowManager.PreviousWindow();
    return true;
  }
  return false;
}

void CGUIBrowseSimpleAppState::SetStartingPath(const CStdString& strStartingPath)
{
  m_sourceController.SetNewSource(new CBrowseWindowSource("simpleappsource", strStartingPath, m_pWindow->GetID()));
  CBrowseWindowStateHistory::ResetHistory(m_pWindow->GetID());
}

CStdString CGUIBrowseSimpleAppState::GetItemSummary()
{
  return m_strLabel;
}

void CGUIBrowseSimpleAppState::Refresh(bool bResetSelected)
{
  CBrowseWindowStateWithHistory::Refresh(bResetSelected);

  m_pWindow->SetProperty( ITEM_SUMMARY_FLAG , GetItemSummary());
}

void CGUIBrowseSimpleAppState::OnPathChanged(CStdString strPath, bool bResetSelected)
{
  CBrowseWindowSource* source = new CBrowseWindowSource("simpleappsource", strPath, m_pWindow->GetID());

  source->SetSortMethod(m_sourceController.GetSortMethod());

  m_sourceController.SetNewSource(source);

  CBrowseWindowStateWithHistory::OnPathChanged(strPath,bResetSelected);
}

//////////////////////////////////////////////////////////////////////////////////////



CGUIWindowBoxeeBrowseSimpleApp::CGUIWindowBoxeeBrowseSimpleApp() : CGUIWindowBoxeeBrowse(WINDOW_BOXEE_BROWSE_SIMPLE_APP,"boxee_browse_simple_app.xml")
{
  SetWindowState(new CGUIBrowseSimpleAppState(this));
}
  
CGUIWindowBoxeeBrowseSimpleApp::~CGUIWindowBoxeeBrowseSimpleApp()
{

}

void CGUIWindowBoxeeBrowseSimpleApp::Show(const CStdString &strPath, const CStdString &strLabel, const CStdString &strBackgroundImage, bool bResetHistory, const CStdString &strAppId)
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseSimpleApp::Show - enter function with [path=%s] (sapp)", strPath.c_str());

  CGUIWindowBoxeeBrowseSimpleApp* pWindow = (CGUIWindowBoxeeBrowseSimpleApp*)g_windowManager.GetWindow(WINDOW_BOXEE_BROWSE_SIMPLE_APP);

  if (pWindow)
  {
    pWindow->ResetWindowState(strPath, strLabel, strBackgroundImage, bResetHistory);
    pWindow->SetAppData(strAppId, strLabel);
    g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_SIMPLE_APP);
  }
}


void CGUIWindowBoxeeBrowseSimpleApp::SetAppData(const CStdString& strAppId, const CStdString& strAppName)
{
  m_strAppId = strAppId;
  m_strAppName = strAppName;
}

bool CGUIWindowBoxeeBrowseSimpleApp::HasAppData()
{
  return (!m_strAppId.IsEmpty());
}

void CGUIWindowBoxeeBrowseSimpleApp::SetItemWithAppData(CFileItem& item)
{
  item.SetProperty("appid",m_strAppId);
  item.SetProperty("provider",m_strAppName);
}

void CGUIWindowBoxeeBrowseSimpleApp::ResetAppData()
{
  m_strAppId.clear();
  m_strAppName.clear();
}

bool CGUIWindowBoxeeBrowseSimpleApp::OnBind(CGUIMessage& message)
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseSimpleApp::OnBind, sender = %d, control = %d (browse)", message.GetSenderId(), message.GetControlId());

  SetProperty("loading", false);

  // Check if control id is 0, meaning that this message is from the item loader
  // Otherwise this message is destined for the specific view control
  if (message.GetPointer() && message.GetControlId() == 0)
  {
    CFileItemList *items = (CFileItemList *)message.GetPointer();

    if (HasAppData())
    {
      for (int i=0; i<items->Size(); i++)
      {
        SetItemWithAppData(*((*items)[i]));
      }
    }
  }

  return false;
}

bool CGUIWindowBoxeeBrowseSimpleApp::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_SET_CONTAINER_PATH:
    {
      // Path was set by external source
      CFileItem *pItem = (CFileItem *)message.GetPointer();
      message.SetPointer(NULL);
      if (!pItem)
        return true;

      if (!pItem->GetProperty("appid").IsEmpty())
      {
        SetAppData(pItem->GetProperty("appid"), pItem->GetProperty("provider"));
      }
      else
      {
        ResetAppData();
      }

      //Handle the background image
      if (pItem->HasProperty("BrowseBackgroundImage"))
      {
        CStdString strBackgroundImage = pItem->GetProperty("BrowseBackgroundImage");
        g_application.GetItemLoader().LoadBackgroundImage(GetID(), strBackgroundImage);
      }

      //m_windowState->SetInitialPath(pItem->m_strPath);
      m_windowState->InitState();

      return true;
    }
  }

  //default
  return CGUIWindowBoxeeBrowse::OnMessage(message);
}

void CGUIWindowBoxeeBrowseSimpleApp::ResetWindowState(const CStdString &strPath, const CStdString &strLabel, const CStdString &strBackgroundImage, bool bResetHistory)
{
  ClearFileItems();
  if (bResetHistory)
    ((CBrowseWindowStateWithHistory*)m_windowState)->ResetHistory();
  else
    ((CBrowseWindowStateWithHistory*)m_windowState)->UpdateHistory();

  // Configuration of the browse window is initialized using specific type string such as "video", "music", or "pictures"
  ((CGUIBrowseSimpleAppState*)m_windowState)->SetStartingPath(strPath);

  m_windowState->SetLabel(strLabel);
}
