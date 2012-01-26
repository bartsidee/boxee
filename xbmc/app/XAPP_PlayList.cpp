
#include "XAPP_PlayList.h"
#include "PlayListPlayer.h"
#include "Application.h"
#include "utils/log.h"
#include "PlayList.h"

using namespace PLAYLIST;

namespace XAPP
{
  
  PlayList::PlayList(PlayList::PlayListType type) {
    iPlayList = type;
    if (type == PLAYLIST_MUSIC) {
      CLog::Log(LOGDEBUG, "XAPP::PlayList::PlayList, created new music playlist (python)");
      m_playList = &g_playlistPlayer.GetPlaylist(0);
    }
    else if (type == PLAYLIST_VIDEO) {
      CLog::Log(LOGDEBUG, "XAPP::PlayList::PlayList, created new video playlist (python)");
      m_playList = &g_playlistPlayer.GetPlaylist(1);
    }
    else {
      m_playList = NULL;
      // error
    }
    
    
  }
  
  void PlayList::Play(int iItem)
  {
    g_playlistPlayer.SetCurrentPlaylist(iPlayList);
    g_playlistPlayer.SetCurrentSong(iItem);
    
    ThreadMessage tMsg (TMSG_PLAYLISTPLAYER_PLAY, iItem);
    g_application.getApplicationMessenger().SendMessage(tMsg, false);
  }

  void PlayList::Add(ListItem item) {
    if (m_playList)
      m_playList->Add(item.GetFileItem());
  }
   
  void PlayList::AddBackground(ListItem item) {
    if (m_playList)
    {
      (*(item.GetFileItem().get())).SetProperty("DisableFullScreen",true);
      m_playList->Add(item.GetFileItem());
    }
  }

   
  ListItem PlayList::GetItem(int index) {
    if (m_playList && m_playList->size()>0)
    {
      CFileItemPtr item = (*m_playList)[index];
      //CFileItemPtr item = CFileItemPtr(m_playList[index]);
      ListItem listItem(item);
      return listItem;
    }
    else
    {
      ListItem listItem;
      return listItem;
    }
  }
   
  void PlayList::Clear() {
    if (m_playList) {
      m_playList->Clear();
    }
  }
  
  int PlayList::GetPosition() {
    if (m_playList)
      return g_playlistPlayer.GetCurrentSong();
    else
      return -1;
  }
  
  int PlayList::Size() {
    if (m_playList)
      return m_playList->size();
    else
      return -1;
  }
  
  bool PlayList::IsShuffle()
  {
    return m_playList->IsShuffled();
  }

}
