
#include "GUIDialogBoxeeMusicCtx.h"
#include "GUIWindowSettingsCategory.h"
#include "BoxeeUtils.h"
#include "Application.h"
#include "utils/GUIInfoManager.h"
#include "GUIWindowMusicInfo.h"
#include "bxobject.h"
#include "bxmetadataengine.h"
#include "FileSystem/BoxeeDatabaseDirectory.h"
#include "bxmetadata.h"
#include "boxee.h"
#include "PlayListPlayer.h"
#include "FileSystem/IDirectory.h"
#include "FileSystem/FactoryDirectory.h"
#include "GUIWindowMusicBase.h"
#include "MusicInfoTag.h"
#include "GUIWindowBoxeeMediaInfo.h"
#include "GUIWindowBoxeeAlbumInfo.h"
#include "GUIWindowManager.h"
#include "GUIWindowBoxeeBrowseTracks.h"
#include "GUIListContainer.h"
#include "GUIUserMessages.h"

using namespace BOXEE;

#define LIST_CONTROL          9000

#define BUTTON_TRACKS         9001

#define HIDDEN_ITEM_ID        5000

#define BTN_STOP_ID           9102
#define BTN_PLAY_PAUSE_ID     9103
#define BTN_SHUFFLE           9105
#define BTN_REPEAT            9116
#define BTN_OSD1              9110
#define BTN_NEXT_TRACK        9104

#define NEXT_TRACK_RIGHT_CLICK  9155
#define SHUFFLE_RIGHT_CLICK     9156
#define OSD1_LEFT_CLICK         9157

CGUIDialogBoxeeMusicCtx::CGUIDialogBoxeeMusicCtx() : CGUIDialogBoxeeSeekableCtx(WINDOW_DIALOG_BOXEE_MUSIC_CTX, "boxee_music_context.xml")
{

}

CGUIDialogBoxeeMusicCtx::~CGUIDialogBoxeeMusicCtx()
{
}

void CGUIDialogBoxeeMusicCtx::Update()
{
  CGUIDialogBoxeeSeekableCtx::Update();
}

bool CGUIDialogBoxeeMusicCtx::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_NOTIFY_ALL:
    if (message.GetParam1() == GUI_MSG_UPDATE_ITEM && message.GetItem())
    {
      SetItem(*message.GetItem());
    }
    break;

  case GUI_MSG_CLICKED:
  {
    unsigned int iControl = message.GetSenderId();
    if (iControl == BUTTON_TRACKS)
    {
      CStdString strPath;

      m_item.Dump();

      /* Should implement CDDA
      if (m_item.IsCDDA())
      {
        strPath = m_item.m_strPath;
      }
      else
      {
        strPath = "boxeedb://tracks/";
        strPath += m_item.GetProperty("BoxeeDBAlbumId");
      }
      */

      g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_TRACKS, m_item.GetProperty("BoxeeDBAlbumId"));
      g_windowManager.CloseDialogs(true);
      return true;
    }
    else if (iControl == NEXT_TRACK_RIGHT_CLICK)
    {
      if (GetControl(BTN_SHUFFLE)->IsVisible())
      {
        SET_CONTROL_FOCUS(BTN_SHUFFLE, 0);
      }
      else if (GetControl(BTN_REPEAT)->IsVisible())
      {
        SET_CONTROL_FOCUS(BTN_REPEAT, 0);
      }
      else if (GetControl(BTN_OSD1)->IsVisible())
      {
        SET_CONTROL_FOCUS(BTN_OSD1, 0);
      }
    }
    else if (iControl == OSD1_LEFT_CLICK)
    {
      if (GetControl(BTN_REPEAT)->IsVisible())
      {
        SET_CONTROL_FOCUS(BTN_REPEAT, 0);
      }
      else if (GetControl(BTN_SHUFFLE)->IsVisible())
      {
        SET_CONTROL_FOCUS(BTN_SHUFFLE, 0);
      }
      else if (GetControl(BTN_NEXT_TRACK)->IsVisible())
      {
        SET_CONTROL_FOCUS(BTN_NEXT_TRACK, 0);
      }
    }
    else if (iControl == SHUFFLE_RIGHT_CLICK)
    {
      if (GetControl(BTN_REPEAT)->IsVisible())
      {
        SET_CONTROL_FOCUS(BTN_REPEAT, 0);
      }
      else if (GetControl(BTN_OSD1)->IsVisible())
      {
        SET_CONTROL_FOCUS(BTN_OSD1, 0);
      }
    }
  }
  break;
  case GUI_MSG_WINDOW_DEINIT:
  {
    m_item.Reset();
  }
  }

  return CGUIDialogBoxeeSeekableCtx::OnMessage(message);
}

void CGUIDialogBoxeeMusicCtx::OnInitWindow()
{
  if (m_item.m_strPath.IsEmpty()) 
  {
    m_item = g_application.CurrentFileItem();
  }

  if (m_item.GetPropertyBOOL("isradio") || m_item.IsInternetStream() || !m_item.HasMusicInfoTag() || m_item.GetPropertyBOOL("isFolderItem"))
  {
    m_item.SetProperty("showtracksbutton", false);
  }
  else
  {
    m_item.SetProperty("showtracksbutton", true);
  }

  CFileItemPtr pItem (new CFileItem(m_item));
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), HIDDEN_ITEM_ID, 0);
  OnMessage(msg);
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), HIDDEN_ITEM_ID, 0, 0, pItem);
  OnMessage(winmsg);

  CGUIDialogBoxeeSeekableCtx::OnInitWindow();

  CGUIListContainer* pControl = (CGUIListContainer*)GetControl(LIST_CONTROL);

  if (pControl)
  {
    // need to update visibility in order for buttons in the static list to be shown (tmp fix before version release)
    pControl->UpdateVisibility();
  }

  if (m_item.IsLastFM() || m_item.IsShoutCast() || m_item.HasProperty("isradio"))
  {
    SET_CONTROL_FOCUS(BTN_PLAY_PAUSE_ID,0);
  }
}

void CGUIDialogBoxeeMusicCtx::OnPlay()
{
  CFileItemList queuedItems;
  BoxeeUtils::BuildPlaylist(&m_item, queuedItems, true, false, false);

  if (queuedItems.Size() > 0)
  {
    queuedItems.Sort(SORT_METHOD_FILE, SORT_ORDER_ASC);
    g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
    g_playlistPlayer.Reset();
    g_playlistPlayer.Add(PLAYLIST_MUSIC, queuedItems);
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);

    g_playlistPlayer.Play();
  }
}

void CGUIDialogBoxeeMusicCtx::OnMoreInfo() 
{
  CLog::Log(LOGDEBUG, "CGUIDialogBoxeeMusicCtx::OnMoreInfo");
  m_item.Dump();
  
  if (m_item.GetPropertyBOOL("albumTrack"))
  {
    g_windowManager.ActivateWindow(WINDOW_BOXEE_ALBUM_INFO);
    //CFileItemPtr albumItem(new CFileItem(m_item));  
    //CGUIWindowBoxeeAlbumInfo::Show(albumItem, true);
  }
  else 
  {
    CGUIWindowBoxeeMediaInfo::Show(&m_item);
  }
}

