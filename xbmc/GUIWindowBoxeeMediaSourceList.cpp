
#include "GUIWindowBoxeeMediaSourceList.h"
#include "GUIWindowBoxeeMediaSources.h"
#include "GUIWindowBoxeeMediaSourceInfo.h"
#include "Application.h"
#include "GUIWindowManager.h"
#include "BoxeeMediaSourceList.h"
#include "Util.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "utils/log.h"

#define CONTROL_TITLE_LABEL       40
#define CONTROL_SOURCE_LIST       51
#define CONTROL_SOURCE_ADD_BUTTON 52

#define VIDEO_LISTITEMS      0
#define MUSIC_LISTITEMS      1
#define PICTURES_LISTITEMS   2

CGUIWindowBoxeeMediaSourceList::CGUIWindowBoxeeMediaSourceList(void)
    : CGUIDialog(WINDOW_BOXEE_MEDIA_SOURCE_LIST, "boxee_media_source_list.xml")
{
  
}

CGUIWindowBoxeeMediaSourceList::~CGUIWindowBoxeeMediaSourceList(void)
{
  
}

bool CGUIWindowBoxeeMediaSourceList::OnAction(const CAction &action)
{
   int iControl = GetFocusedControlID();
   
   bool bSelectAction = ((action.id == ACTION_SELECT_ITEM) || (action.id == ACTION_MOUSE_LEFT_CLICK));

   if (bSelectAction && iControl == CONTROL_SOURCE_LIST)
   {
     CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceList::OnAction - Enter GUI_MSG_CLICKED case with [iControl=CONTROL_SOURCE_LIST]. Going to call ProccessItemSelectedInControlSourceList() (msmk)");

     ProccessItemSelectedInControlSourceList();

     return true;
   }
   else if (bSelectAction && iControl == CONTROL_SOURCE_ADD_BUTTON)
   {
     CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceList::OnAction - Enter GUI_MSG_CLICKED case with [iControl=CONTROL_SOURCE_ADD_BUTTON] (msmk)");

     CGUIWindowBoxeeMediaSourceInfo *pDlgSourceInfo = (CGUIWindowBoxeeMediaSourceInfo*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_SOURCE_INFO);
     if (pDlgSourceInfo)
     {
       pDlgSourceInfo->SetEditedSource("");
     }
   }
   // Do not move to source list if no sources exist
   else if ((action.id == ACTION_MOVE_LEFT || action.id == ACTION_MOVE_UP) && iControl == CONTROL_SOURCE_ADD_BUTTON && !m_sourcesExist)
   {
     return true;
   }
   else if (action.id == ACTION_PREVIOUS_MENU)
   {
      Close();
      return true;
   }
   else if (action.id == ACTION_PARENT_DIR)
   {
      Close();
      return true;
   }

   return CGUIWindow::OnAction(action);
}

bool CGUIWindowBoxeeMediaSourceList::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {    
    DWORD senderId = message.GetSenderId();
    
    if(senderId == CONTROL_SOURCE_LIST)
    {
      if (message.GetParam1() != ACTION_BUILT_IN_FUNCTION)
      {
        // Handle only GUI_MSG_CLICKED on CONTROL_SOURCE_LIST that origin from navigation

        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceList::OnMessage - Enter GUI_MSG_CLICKED case with [SenderId=CONTROL_SOURCE_LIST]. Going to call ProccessItemSelectedInControlSourceList() (msmk)");

        ProccessItemSelectedInControlSourceList();
      
        return true;
      }
    }
  }
  break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIWindowBoxeeMediaSourceList::OnInitWindow()
{
  CGUIWindow::OnInitWindow();
  LoadAllShares();
}

void CGUIWindowBoxeeMediaSourceList::Refresh()
{
  LoadAllShares();
}

void CGUIWindowBoxeeMediaSourceList::LoadAllShares()
{
  CGUIWindowBoxeeMediaSources *pDlgSources = (CGUIWindowBoxeeMediaSources*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_SOURCES);
  if (!pDlgSources)
    return;
  int selectedSource = pDlgSources->getSelectedSource();

  if (selectedSource == SOURCE_LOCAL)
  {
     SET_CONTROL_LABEL(CONTROL_TITLE_LABEL, "Settings / Media Sources / Local");
  }
  else if (selectedSource == SOURCE_NETWORK)
  {
     SET_CONTROL_LABEL(CONTROL_TITLE_LABEL, "Settings / Media Sources / Network");
  }
  else if (selectedSource == MANUALLY_ADD_SOURCE)
  {
     SET_CONTROL_LABEL(CONTROL_TITLE_LABEL, "Settings / Media Sources / Manually Add Sources");
  }
  
  // Clear the list of sources
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_SOURCE_LIST);
  OnMessage(msgReset);
  m_sources.Clear();
  m_sourcesExist = false;

  CBoxeeMediaSourceList sourceList;
  BoxeeMediaSourceMap::iterator sourcesIterator;
    
  for (sourcesIterator = sourceList.getMap().begin(); sourcesIterator != sourceList.getMap().end(); sourcesIterator++)
  {    
    CBoxeeMediaSource& source = (*sourcesIterator).second;
        
    switch(selectedSource)
    {
    case SOURCE_LOCAL:
    {
      if(!source.isLocal)
      {
        continue;
      }
    }
    break;
    case SOURCE_NETWORK:
    {
      if((!source.isNetwork) || (CUtil::IsPlugin(source.path) ||  CUtil::IsApp(source.path) || CUtil::IsRSS(source.path) ||CUtil::IsLastFM(source.path) || CUtil::IsShoutCast(source.path) || source.path.Left(9) == "script://"))
      {
        continue;
      }
    }
    break;
    case MANUALLY_ADD_SOURCE:
    {
      
    }
    break;
    default:
    {
      CLog::Log(LOGERROR,"In CGUIWindowBoxeeMediaSourceList::LoadAllShares - Failed to handle source [%s] because of an unknown selectedSourceType [%d]",(source.path).c_str(),selectedSource);
      continue;
    }
    }
    
    // Create new share FileItem
    CFileItemPtr share ( new CFileItem(source.name) );
    share->m_strPath = source.path;
    share->SetThumbnailImage(source.thumbPath);
    if (source.isVideo) share->SetProperty("IsVideo", true);
    if (source.isMusic) share->SetProperty("IsMusic", true);
    if (source.isPicture) share->SetProperty("IsPictures", true);
    if (source.isPrivate) share->SetProperty("IsPrivate", true);
    if (g_application.IsPathAvailable(source.path, true))
      share->SetLabel2("Connected");
    else
      share->SetLabel2("Not Connected");
    m_sources.Add(share);
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_SOURCE_LIST, 0, 0, share);
    OnMessage(msg);

    m_sourcesExist = true;
  }

  if (!m_sourcesExist)
  {
    SetProperty("sources-exist", false);
    SET_CONTROL_FOCUS(CONTROL_SOURCE_ADD_BUTTON, 0);
  }
  else
  {
    SetProperty("sources-exist", true);
    SET_CONTROL_FOCUS(CONTROL_SOURCE_LIST, 0);
  }
}

void CGUIWindowBoxeeMediaSourceList::ProccessItemSelectedInControlSourceList()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_SOURCE_LIST);
  OnMessage(msg);
  int iItem = msg.GetParam1();

  CGUIWindowBoxeeMediaSourceInfo *pDlgSourceInfo = (CGUIWindowBoxeeMediaSourceInfo*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_SOURCE_INFO);
  if (pDlgSourceInfo)
  {
    pDlgSourceInfo->SetEditedSource(m_sources[iItem]->GetLabel());
    pDlgSourceInfo->SetSourceThumbPath((m_sources[iItem]->GetThumbnailImage()));
    pDlgSourceInfo->SetEnableLocationEdit(false);
  }
  
  g_windowManager.ActivateWindow(WINDOW_BOXEE_MEDIA_SOURCE_INFO);  
}

