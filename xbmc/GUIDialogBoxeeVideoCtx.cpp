
#include "GUIDialogBoxeeVideoCtx.h"
#include "GUIWindowBoxeeMediaInfo.h"
#include "BoxeeUtils.h"
#include "GUIWindowManager.h"
#include "Application.h"
#include "GUIUserMessages.h"
#include "MouseStat.h"
#include "BoxeeItemLauncher.h"
#include "ItemLoader.h"

using namespace BOXEE;

#define BTN_PREV_ITEM_ID 9101
#define BTN_STOP_ID      9104
#define BTN_NEXT_ITEM_ID 9108

#define EXT_0_BUTTON_ID 9109

#define HIDDEN_ITEM_ID 5000

CGUIDialogBoxeeVideoCtx::CGUIDialogBoxeeVideoCtx(void) : 
  CGUIDialogBoxeeCtx(WINDOW_DIALOG_BOXEE_VIDEO_CTX, "boxee_video_context.xml")
{
}

CGUIDialogBoxeeVideoCtx::~CGUIDialogBoxeeVideoCtx(void)
{
}

void CGUIDialogBoxeeVideoCtx::Update()
{
  CGUIDialogBoxeeCtx::Update();
}

bool CGUIDialogBoxeeVideoCtx::OnAction(const CAction &action)
{
  return CGUIDialogBoxeeCtx::OnAction(action);
}

bool CGUIDialogBoxeeVideoCtx::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
    {
      if (message.GetControlId() == WINDOW_DIALOG_BOXEE_VIDEO_CTX)
      {
        if (message.GetSenderId() == BTN_PREV_ITEM_ID)
        {
          if (m_item.GetPrevItem().get())
          {
            Close();
            CBoxeeItemLauncher::Launch(*(m_item.GetPrevItem().get()));
          }
          return true;
        }
        else if (message.GetSenderId() == BTN_NEXT_ITEM_ID)
        {        
          if (m_item.GetNextItem().get())
          {
            Close();
            CBoxeeItemLauncher::Launch(*(m_item.GetNextItem().get()));
          }
          return true;
        }        
        else if (message.GetSenderId() == EXT_0_BUTTON_ID)
        { 
          if (g_application.m_pPlayer)
            g_application.m_pPlayer->OSDExtensionClicked(0);
          Close();
          return true;
        }        
      }
    }
    break;
  }
  return CGUIDialogBoxeeCtx::OnMessage(message);
}

void CGUIDialogBoxeeVideoCtx::Render()
{
  if (m_autoClosing)
  {
    // check for movement of mouse or a submenu open
    if (g_Mouse.HasMoved() || g_windowManager.IsWindowActive(WINDOW_DIALOG_AUDIO_OSD_SETTINGS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_VIDEO_OSD_SETTINGS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_VIDEO_BOOKMARKS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_SUBTITLE_OSD_SETTINGS))
      SetAutoClose(3000);
  }
  CGUIDialog::Render();
}

void CGUIDialogBoxeeVideoCtx::OnInitWindow()
{
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

  CGUIDialogBoxeeCtx::OnInitWindow();

  if (g_application.m_pPlayer && (!g_application.m_pPlayer->CanPause()))
  {
    SET_CONTROL_FOCUS(BTN_STOP_ID, 0);
  }
}

void CGUIDialogBoxeeVideoCtx::OnMoreInfo() 
{
  g_windowManager.CloseDialogs(true);
  CGUIWindowBoxeeMediaInfo::Show(&m_item);
}
