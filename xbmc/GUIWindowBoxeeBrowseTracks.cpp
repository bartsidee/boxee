
#include "GUIWindowBoxeeBrowseTracks.h"
#include "PlayListPlayer.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "GUISettings.h"
#include "MusicInfoTag.h"
#include "BoxeeShortcut.h"
#include "Settings.h"
#include "GUIWindowManager.h"
#include "GUIDialogBoxeeShare.h"
#include "Application.h"
#include "BoxeeUtils.h"
#include "Directory.h"
#include "ThumbLoader.h"

using namespace std;

#define BUTTON_SHARE 150
#define BUTTON_SHORTCUT 160

#define TRACKS_WINDOW_SHORTCUT_COMMAND   "ActivateWindow(10485,%s)"

// STATE IMPLEMENTATION

CTracksWindowState::CTracksWindowState(CGUIWindow* pWindow) : CBrowseWindowState(pWindow)
{
}

CStdString CTracksWindowState::CreatePath()
{
  if (m_configuration.m_strPath.IsEmpty())
  {
    CStdString strPath = "boxeedb://tracks/";
    strPath += m_strAlbumId;

    return strPath;
  }

  return m_configuration.m_strPath;
}

void CTracksWindowState::InitState(const CStdString& strPath)
{
  CBrowseWindowState::InitState(strPath);

  // check if item has shortcut
  CBoxeeShortcut cut;
  CStdString command;
  CStdString str = TRACKS_WINDOW_SHORTCUT_COMMAND;
  command.Format(str.c_str(), GetCurrentPath());
  cut.SetCommand(command);

  if (g_settings.GetShortcuts().IsInShortcut(cut))
  {
    m_bHasShortcut = true;
  }
  else
  {
    m_bHasShortcut = false;
  }
}

void CTracksWindowState::SortItems(CFileItemList &items)
{
  items.Sort(SORT_METHOD_TRACKNUM, SORT_ORDER_ASC);
}

void CTracksWindowState::SetAlbumData(const CStdString& strAlbumId, const CStdString& strAlbumName, const CStdString& strAlbumThumb)
{
  m_strAlbumId = strAlbumId;
  m_strAlbumName = strAlbumName;
  m_strAlbumThumb = strAlbumThumb;
}

bool CTracksWindowState::HasShortcut()
{
  return m_bHasShortcut;
}

bool CTracksWindowState::OnShortcut()
{
  // Add currently path to all tv shows, unless it already exists?
  CBoxeeShortcut cut;
  cut.SetName(m_strAlbumName);

  CStdString command = "ActivateWindow(10485,";
  command += GetCurrentPath();
  command += ")";
  cut.SetCommand(command);

  cut.SetThumbPath(m_strAlbumThumb);
  cut.SetCountry("all");
  cut.SetCountryAllow(true);
  cut.SetReadOnly(false);

  if (!m_bHasShortcut)
  {
    // add to shortcut
    if (g_settings.GetShortcuts().AddShortcut(cut))
    {
      m_bHasShortcut = true;
    }
  }
  else
  {
    // remove from shortcut
    if (g_settings.GetShortcuts().RemoveShortcut(cut))
    {
      m_bHasShortcut = false;
    }
  }
  return m_bHasShortcut;

}

// WINDOW IMPLEMENTATION

CGUIWindowBoxeeBrowseTracks::CGUIWindowBoxeeBrowseTracks()
: CGUIWindowBoxeeBrowse(WINDOW_BOXEE_BROWSE_TRACKS, "boxee_browse_music_tracks.xml")
{
  SetWindowState(new CTracksWindowState(this));
}

CGUIWindowBoxeeBrowseTracks::~CGUIWindowBoxeeBrowseTracks()
{

}

void CGUIWindowBoxeeBrowseTracks::OnInitWindow()
{
  CGUIWindowBoxeeBrowse::OnInitWindow();

  if (((CTracksWindowState*)m_windowState)->HasShortcut())
  {
    CStdString label = g_localizeStrings.Get(53716);
    SET_CONTROL_LABEL(BUTTON_SHORTCUT, label.ToUpper());
  }
  else
  {
    CStdString label = g_localizeStrings.Get(53715);
    SET_CONTROL_LABEL(BUTTON_SHORTCUT, label.ToUpper());
  }
}

bool CGUIWindowBoxeeBrowseTracks::OnBind(CGUIMessage& message)
{
  if (message.GetPointer() && message.GetControlId() == 0)
  {
    CFileItemList *items = (CFileItemList *)message.GetPointer();

    if (m_albumItem.get() == NULL || m_albumItem->GetProperty("BoxeeDBAlbumId") != items->GetProperty("BoxeeDBAlbumId") ||
        m_albumItem->m_strPath != items->m_strPath)
    {
      m_bResetPlaylist = true;
      m_albumItem = CFileItemPtr(new CFileItem(*items));

      CStdString strAlbumId = items->GetProperty("BoxeeDBAlbumId");
      CStdString strAlbumName = items->GetLabel();
      CStdString strAlbumThumb = items->GetThumbnailImage();

      ((CTracksWindowState*)m_windowState)->SetAlbumData(strAlbumId, strAlbumName, strAlbumThumb);

      CMusicThumbLoader loader;
      loader.LoadItem(m_albumItem.get(), false);

      SetProperty("title", m_albumItem->GetLabel());
      SetProperty("albumthumb", m_albumItem->GetThumbnailImage());
      SetProperty("albumyear", m_albumItem->GetMusicInfoTag()->GetYear());
      SetProperty("albumartist", m_albumItem->GetMusicInfoTag()->GetArtist());
      SetProperty("albumgenre", m_albumItem->GetMusicInfoTag()->GetGenre());
    }

  }

  return CGUIWindowBoxeeBrowse::OnBind(message);
}

bool CGUIWindowBoxeeBrowseTracks::OnClick(int iItem)
{
  if (m_bResetPlaylist || g_playlistPlayer.GetCurrentPlaylist() != PLAYLIST_MUSIC)
  {
    m_bResetPlaylist = false;

    g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
    g_playlistPlayer.Reset();
    g_playlistPlayer.Add(PLAYLIST_MUSIC, m_vecModelItems);
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
  }

  g_application.GetBoxeeItemsHistoryList().AddItemToHistory(*(m_albumItem.get()));
  g_playlistPlayer.Play(iItem);
  return true;
}

void CGUIWindowBoxeeBrowseTracks::OnBack()
{
  g_windowManager.PreviousWindow();
}

bool CGUIWindowBoxeeBrowseTracks::OnMessage(CGUIMessage& message)
{
  if (ProcessPanelMessages(message))
  {
    return true;
  }

  return CGUIWindowBoxeeBrowse::OnMessage(message);
}

bool CGUIWindowBoxeeBrowseTracks::ProcessPanelMessages(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();

    if (iControl == BUTTON_SHORTCUT)
    {
      if (((CTracksWindowState*)m_windowState)->OnShortcut())
      {
        // shortcut was added
        SET_CONTROL_LABEL(BUTTON_SHORTCUT, g_localizeStrings.Get(53716));
        g_application.m_guiDialogKaiToast.QueueNotification("", "", g_localizeStrings.Get(53737), 5000);
      }
      else
      {
        // shortcut was removed
        SET_CONTROL_LABEL(BUTTON_SHORTCUT, g_localizeStrings.Get(53715));
        g_application.m_guiDialogKaiToast.QueueNotification("", "", g_localizeStrings.Get(53739), 5000);
      }
      return true;
    }
    else if (iControl == BUTTON_SHARE)
    {
      return OnShare();
    }

    // else - break from switch and return false
    break;
  } // case GUI_MSG_CLICKED

  } // switch

  return false;
}

bool CGUIWindowBoxeeBrowseTracks::OnShare()
{
  CGUIDialogBoxeeShare* pShare = (CGUIDialogBoxeeShare *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_SHARE);

  if (pShare)
  {
    m_albumItem->Dump();

    pShare->SetItem(m_albumItem.get());
    pShare->DoModal();
    return true;
  }

  return false;
}
