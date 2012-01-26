
#include "GUIDialogBoxeeVideoCtx.h"
#include "GUIWindowBoxeeMediaInfo.h"
#include "BoxeeUtils.h"
#include "GUIWindowManager.h"
#include "Application.h"
#include "GUIUserMessages.h"
#include "MouseStat.h"
#include "BoxeeItemLauncher.h"
#include "ItemLoader.h"
#include "GUIInfoManager.h"
#include "GUIWindowFullScreen.h"
#include "GUIDialogBoxeeTechInfo.h"

using namespace BOXEE;

#define BTN_PREV_ITEM_ID 9101
#define BTN_STOP_ID      9104
#define BTN_PAUSE_ID     9105
#define BTN_NEXT_ITEM_ID 9108

#define BTN_BIG_SKIP_BWD  9102
#define BTN_SMALL_SKIP_BWD  9103
#define BTN_BIG_SKIP_FWD  9106
#define BTN_SMALL_SKIP_FWD  9107

#define EXT_0_BUTTON_ID 9109
#define BTN_TECH_INFO   9012

#define CURSOR_BTN_ID  9116

#define HIDDEN_ITEM_ID 5000

CGUIDialogBoxeeVideoCtx::CGUIDialogBoxeeVideoCtx() : CGUIDialogBoxeeSeekableCtx(WINDOW_DIALOG_BOXEE_VIDEO_CTX, "boxee_video_context.xml")
{

}

CGUIDialogBoxeeVideoCtx::~CGUIDialogBoxeeVideoCtx()
{

}

void CGUIDialogBoxeeVideoCtx::Update()
{
  CGUIDialogBoxeeSeekableCtx::Update();
}

bool CGUIDialogBoxeeVideoCtx::OnAction(const CAction &action)
{
  return CGUIDialogBoxeeSeekableCtx::OnAction(action);
}

bool CGUIDialogBoxeeVideoCtx::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_VIDEO_MENU_STARTED:
  {
    // We have gone to the DVD menu, so close the OSD.
    Close();
    break;
  }
  case GUI_MSG_CLICKED:
    {
      if (message.GetControlId() == WINDOW_DIALOG_BOXEE_VIDEO_CTX)
      {
        bool blurayFolder = (m_item.IsPlayableFolder() && m_item.GetPropertyBOOL("isBlurayFolder")); 
        if (message.GetSenderId() == BTN_PREV_ITEM_ID)
        {
          if(blurayFolder && g_application.IsPlaying())
          {
            int iChapter = g_application.m_pPlayer->GetChapter() - 1;
             
            if(iChapter <= 0)
            {
              iChapter = g_application.m_pPlayer->GetChapterCount() - 1;
            }

            g_application.m_pPlayer->SeekChapter(iChapter);
          }
          else if (m_item.GetPrevItem().get())
          {
            Close();
            CBoxeeItemLauncher::Launch(*(m_item.GetPrevItem().get()));
          }
          return true;
        }
        else if (message.GetSenderId() == BTN_TECH_INFO)
        {
          OnTechInfo();
          return true;
        }
        else if (message.GetSenderId() == BTN_NEXT_ITEM_ID)
        {
          if(blurayFolder && g_application.IsPlaying())
          {
            int iChapter = g_application.m_pPlayer->GetChapter() + 1;
        
            if(iChapter >= g_application.m_pPlayer->GetChapterCount() - 1)
            {
              iChapter = 0;
            }

            g_application.m_pPlayer->SeekChapter(iChapter);
          }
          else if (m_item.GetNextItem().get())
          {
            Close();
            CBoxeeItemLauncher::Launch(*(m_item.GetNextItem().get()));
          }
          return true;
        }        
      }
    }
    break;
  }

  return CGUIDialogBoxeeSeekableCtx::OnMessage(message);
}

void CGUIDialogBoxeeVideoCtx::Render()
{
  if (m_autoClosing)
  {
    // check for movement of mouse or a submenu open
    if (g_Mouse.HasMoved() || g_windowManager.IsWindowActive(WINDOW_DIALOG_AUDIO_OSD_SETTINGS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_VIDEO_OSD_SETTINGS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_VIDEO_BOOKMARKS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_SUBTITLE_OSD_SETTINGS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_BOXEE_SEEK_BAR)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_BOXEE_SHARE))
      SetAutoClose(3000);
  }

  CGUIDialogBoxeeSeekableCtx::Render();
}

void CGUIDialogBoxeeVideoCtx::OnInitWindow()
{
  if (m_seekDirectionOnOpen != CSeekDirection::NONE)
  {
    g_Mouse.SetActive(false);
    CGUIDialogBoxeeSeekableCtx::OnInitWindow();

    SET_CONTROL_HIDDEN(9100);
    SET_CONTROL_HIDDEN(9150);
    SET_CONTROL_VISIBLE(8100);
    SET_CONTROL_FOCUS(8100,0);

    CAction action;
    action.id = (m_seekDirectionOnOpen == CSeekDirection::FORWARD) ? ACTION_MOVE_RIGHT : ACTION_MOVE_LEFT;
    //action.amount1 = 1.0f;
    OnAction(action);
  }
  else
  {
    SET_CONTROL_VISIBLE(9150);

    // Load metadata from video files
    if ((m_item.IsHD() || m_item.IsSmb()) && !m_item.m_bIsFolder && !m_item.m_bIsShareOrDrive &&
          !m_item.m_strPath.IsEmpty() && !m_item.GetPropertyBOOL("MetaDataExtracted") &&
          g_application.IsPathAvailable(m_item.m_strPath), false)
    {
      g_application.GetItemLoader().LoadFileMetadata(GetID(), HIDDEN_ITEM_ID, &m_item);
    }
    else
    {
      CFileItemPtr pItem (new CFileItem(m_item));
      CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), HIDDEN_ITEM_ID, 0);
      OnMessage(msg);
      CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), HIDDEN_ITEM_ID, 0, 0, pItem);
      OnMessage(winmsg);
    }

    g_Mouse.SetActive(false);

    CGUIDialogBoxeeSeekableCtx::OnInitWindow();
  
    SET_CONTROL_FOCUS(9150,0);

    if (g_application.m_pPlayer && g_application.m_pPlayer->CanPause())
    {
      SET_CONTROL_VISIBLE(BTN_PAUSE_ID);
      //SET_CONTROL_FOCUS(BTN_PAUSE_ID,0);
      SET_CONTROL_FOCUS(8100,0);
    }
    else if (g_infoManager.GetBool(PLAYER_IS_FLASH))
    {
      SET_CONTROL_VISIBLE(CURSOR_BTN_ID);
      SET_CONTROL_FOCUS(CURSOR_BTN_ID, 0);
    }
    else
    {
      SET_CONTROL_VISIBLE(BTN_STOP_ID);
      SET_CONTROL_FOCUS(BTN_STOP_ID, 0);
    }
  }
}

void CGUIDialogBoxeeVideoCtx::OnMoreInfo() 
{
  g_windowManager.CloseDialogs(true);
  CGUIWindowBoxeeMediaInfo::Show(&m_item);
}

void CGUIDialogBoxeeVideoCtx::OnTechInfo()
{
  CGUIDialogBoxeeTechInfo *pDialog = new CGUIDialogBoxeeTechInfo();

  if (pDialog)
  {
    pDialog->Show();
  }
}

void CGUIDialogBoxeeVideoCtx::Close(bool forceClose)
{
  CGUIDialogBoxeeSeekableCtx::Close(forceClose);
}

