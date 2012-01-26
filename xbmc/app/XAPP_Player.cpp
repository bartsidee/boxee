
#include "XAPP_Player.h"
#include "PlayListPlayer.h"
#include "Application.h"
#include "GUIInfoManager.h"
#include "lib/libPython/XBPython.h"
#include "AppManager.h"
#include "GUIDialogBoxeeMediaAction.h"
#include "utils/log.h"
#include "GUIUserMessages.h"

namespace XAPP
{

class MyActionCallBack : public IActionCallback
{
public:
  MyActionCallBack(Player& player) : m_player(player)
  {
  }
  
  virtual void OnActionNextItem()
  {
    CLog::Log(LOGDEBUG, "XAPP::ActionCallback::OnActionNextItem, (flow)");
    m_player.SetLastPlayerAction(Player::XAPP_PLAYER_ACTION_NEXT);
  }
  
  virtual void OnActionStop()
  {
    CLog::Log(LOGDEBUG, "XAPP::ActionCallback::OnActionStop, (flow)");
    m_player.SetLastPlayerAction(Player::XAPP_PLAYER_ACTION_STOP);
  }
  
private:
  XAPP::Player& m_player;
};

/*
class MyPlayerCallBack : public IPlayerCallback
{
public:
  MyPlayerCallBack(Player& player) : m_player(player)
  {
  };
  
  virtual void OnPlayBackEnded()
  {
    // WARINNG: This function should only be called from the locked python context
    CLog::Log(LOGDEBUG, "XAPP::PlayerCallback::OnPlayBackEnded, app = %s (flow)", CAppManager::GetLastLaunchedId().c_str());
    m_player.m_func(Player::EVENT_ENDED, m_player.m_clientdata);
    
    // Set the flag
    m_player.SetLastPlayerEvent(Player::EVENT_ENDED);
  }
  
  virtual void OnPlayBackStarted()
  {
    // WARINNG: This function should only be called from the locked python context
    CLog::Log(LOGDEBUG, "XAPP::PlayerCallback::OnPlayBackStarted, app = %s (flow)", CAppManager::GetLastLaunchedId().c_str());
    m_player.m_func(Player::EVENT_STARTED, m_player.m_clientdata);
    
    m_player.SetLastPlayerEvent(Player::EVENT_STARTED);
  }
  
  virtual void OnPlayBackStopped()
  {
    // WARINNG: This function should only be called from the locked python context
    CLog::Log(LOGDEBUG, "XAPP::PlayerCallback::OnPlayBackStopped, app = %s (flow)", CAppManager::GetLastLaunchedId().c_str());
    m_player.m_func(Player::EVENT_STOPPED, m_player.m_clientdata);
    
    m_player.SetLastPlayerEvent(Player::EVENT_STOPPED);
  }
  
  virtual void OnQueueNextItem()
  {
    // WARINNG: This function should only be called from the locked python context
    CLog::Log(LOGDEBUG, "XAPP::PlayerCallback::OnQueueNextItem, app = %s (flow)", CAppManager::GetLastLaunchedId().c_str());
    m_player.m_func(Player::EVENT_NEXT_ITEM, m_player.m_clientdata);
    
    m_player.SetLastPlayerEvent(Player::EVENT_NEXT_ITEM);
  }
  
private:
  XAPP::Player& m_player;
};

*/
class MyEventCallBack : public IEventCallback
{
public:
  MyEventCallBack(Player& player) : m_player(player)
  {
  };
  
  virtual void OnPlayBackEnded(bool bError, const CStdString& error)
  {
    CLog::Log(LOGDEBUG, "XAPP::EventCallback::OnPlayBackEnded (flow)");
    m_player.SetLastPlayerEvent(Player::EVENT_ENDED);
  }
  
  virtual void OnPlayBackStarted()
  {
    CLog::Log(LOGDEBUG, "XAPP::EventCallback::OnPlayBackStarted (flow)");
    m_player.SetLastPlayerEvent(Player::EVENT_STARTED);
  }
  
  virtual void OnPlayBackStopped()
  {
    CLog::Log(LOGDEBUG, "XAPP::EventCallback::OnPlayBackStopped (flow)");
    m_player.SetLastPlayerEvent(Player::EVENT_STOPPED);
  }
  
  virtual void OnQueueNextItem()
  {
    CLog::Log(LOGDEBUG, "XAPP::EventCallback::OnQueueNextItem (flow)");
    m_player.SetLastPlayerEvent(Player::EVENT_NEXT_ITEM);
  }
  
private:
  XAPP::Player& m_player;
};

/*
void Player::set_method(CALLBACKFUNC func, void* clientdata)
{
  m_func = func;
  m_clientdata = clientdata;
  
  CLog::Log(LOGDEBUG, "XAPP::Player::set_method called (callback)");
  
  // Get current state
  PyThreadState* currentState = PyThreadState_Get();
  
  CLog::Log(LOGDEBUG, "XAPP::Player::set_method get current state (callback)");
  
  if (currentState != NULL) {
    CLog::Log(LOGDEBUG, "XAPP::Player::set_method current state is not NULL, get application id (callback)");
    CStdString strApplicationId = g_pythonParser.GetApllicationId(currentState);
    
    CLog::Log(LOGDEBUG, "XAPP::Player::set_method application id is %s (callback)", strApplicationId.c_str());
    
    if (strApplicationId != "") {
      // Register the callback with the application manager
      CAppManager::RegisterPlayerCallback(strApplicationId, m_playerCallback);
    }
  }
  else {
    CLog::Log(LOGDEBUG, "XAPP::Player::set_method current state is NULL (callback)");
  }
  
  
}

*/

Player::Player(bool bRegisterCallbacks)
{
  //m_playerCallback = new MyPlayerCallBack(*this);
  
  
  if (bRegisterCallbacks) {
    m_eventCallback = new MyEventCallBack(*this);
    m_actionCallback = new MyActionCallBack(*this);
    CAppManager::GetInstance().RegisterEventCallback(m_eventCallback);
    CAppManager::GetInstance().RegisterActionCallback(m_actionCallback);
  }
  else
  {
    m_eventCallback = NULL;
    m_actionCallback = NULL;
  }
  
  m_lastAction = XAPP_PLAYER_ACTION_NONE;
  m_lastEvent = EVENT_STOPPED;
  
}

Player::~Player()
{
  if (m_eventCallback) {
    CAppManager::GetInstance().RemoveEventCallback(m_eventCallback);
    delete m_eventCallback;
  }
  
  if (m_actionCallback) {
    CAppManager::GetInstance().RemoveActionCallback(m_actionCallback);
    delete m_actionCallback;
  }
}

void Player::PlaySelected(int iItem, int type)
{
  g_playlistPlayer.SetCurrentPlaylist(type);
  g_playlistPlayer.SetCurrentSong(iItem);

  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_PLAY, iItem};

  PyObject *app = PySys_GetObject((char*)"app-id");
  if (app)
  {
    const char *id = PyString_AsString(app);
    tMsg.strParam = id;
  }  
  
  g_application.getApplicationMessenger().SendMessage(tMsg, false);
}

void Player::Play(ListItem item)
{
  if (item.GetFileItem()->GetPropertyBOOL("add-to-history"))
  {
    if (item.GetFileItem()->HasExternlFileItem())
    {
      g_application.GetBoxeeItemsHistoryList().AddItemToHistory(*(item.GetFileItem()->GetExternalFileItem().get()));
    }
    else
    {      
      g_application.GetBoxeeItemsHistoryList().AddItemToHistory(*(item.GetFileItem().get()));
    }
  }
  
  PyObject *app = PySys_GetObject((char*)"app-id");
  if (app)
  {
    const char *id = PyString_AsString(app);
    (*(item.GetFileItem().get())).SetProperty("appid",id);
  }
  
  g_application.getApplicationMessenger().MediaPlay(*(item.GetFileItem().get()));
}

void Player::PlayInBackground(ListItem item)
{
  if (item.GetFileItem()->GetPropertyBOOL("add-to-history"))
  {
    if (item.GetFileItem()->HasExternlFileItem())
    {
      g_application.GetBoxeeItemsHistoryList().AddItemToHistory(*(item.GetFileItem()->GetExternalFileItem().get()));
    }
    else
    {
      g_application.GetBoxeeItemsHistoryList().AddItemToHistory(*(item.GetFileItem().get()));
    }
  }

  PyObject *app = PySys_GetObject((char*)"app-id");
  if (app)
  {
    const char *id = PyString_AsString(app);
    (*(item.GetFileItem().get())).SetProperty("appid",id);
  }

  // Set property DisableFullScreen to play the file in the background
  (*(item.GetFileItem().get())).SetProperty("DisableFullScreen",true);

  g_application.getApplicationMessenger().MediaPlay(*(item.GetFileItem().get()));
}

void Player::PlayWithActionMenu(ListItem item)
{
  CGUIDialogBoxeeMediaAction::ShowAndGetInput(item.GetFileItem().get());
}

void Player::PlaySlideshow(ListItems pictures, bool bRandom, bool bNotRandom, const std::string& strPictureToStart, bool startPaused)
{
  CFileItemList* fileItems = new CFileItemList();

  for (size_t i = 0; i < pictures.size(); i++)
  {
    CFileItem* pItem =  pictures[i].GetFileItem().get();
    CFileItemPtr fileItem(new CFileItem(*pItem));
    fileItems->Add(fileItem);
  }

  unsigned int flags = 0;

  if (bRandom)     flags |= 2;
  if (bNotRandom)  flags |= 4;
  if (startPaused) flags |= 8;

  CGUIMessage msg(GUI_MSG_START_SLIDESHOW, 0, 0, flags);

  msg.SetPointer(fileItems);
  msg.SetStringParam(strPictureToStart);

  g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, WINDOW_SLIDESHOW, true);
}

void Player::Pause()
{
  g_application.getApplicationMessenger().MediaPause();
}

void Player::Stop()
{
  //g_application.getApplicationMessenger().MediaStop();
  ThreadMessage tMsg = {TMSG_MEDIA_STOP};
  g_application.getApplicationMessenger().SendMessage(tMsg, false);
}

void Player::PlayNext() 
{
  //g_application.m_eForcedNextPlayer = playerCore;
  g_application.getApplicationMessenger().PlayListPlayerNext();
}

void Player::PlayPrevious()
{
  //g_application.m_eForcedNextPlayer = playerCore;
  g_application.getApplicationMessenger().PlayListPlayerPrevious();
}

double Player::GetTime() throw (XAPP::AppException)
{
  if (!g_application.IsPlaying())
  {
    throw AppException("XBMC is not playing any media file");
  }
  return g_application.GetTime();
}

double Player::GetTotalTime() throw (XAPP::AppException)
{
  if (!g_application.IsPlaying())
  {
    throw AppException("XBMC is not playing any media file");
  }

  return g_application.GetTotalTime();
}

void Player::SeekTime(double seekTime)
{
   if (!g_application.IsPlaying())
   {
     return;
   }
   g_application.SeekTime( seekTime );
}

void Player::LockPlayerAction(XAPP::Player::PLAYER_ACTION action)
{
  switch (action) {
  case XAPP_PLAYER_ACTION_NONE:
    g_infoManager.LockPlayerAction(PLAYER_ACTION_ALLOW_ALL);
    break;
  case XAPP_PLAYER_ACTION_NEXT:
    g_infoManager.LockPlayerAction(PLAYER_ACTION_NEXT);
    break;
  case XAPP_PLAYER_ACTION_PREV:
    g_infoManager.LockPlayerAction(PLAYER_ACTION_PREV);
    break;
  default:
    g_infoManager.LockPlayerAction(PLAYER_ACTION_ALLOW_ALL);
  }
}

XAPP::ListItem Player::GetPlayingItem()
{
  CFileItemPtr item = CFileItemPtr(new CFileItem(g_infoManager.GetCurrentItem()));
  ListItem listItem(item);
  return listItem;
}

XAPP::Player::PLAYER_EVENT Player::GetLastPlayerEvent()
{
  return m_lastEvent;
}

void Player::SetLastPlayerEvent(XAPP::Player::PLAYER_EVENT event)
{
  m_lastEvent = event; 
}

XAPP::Player::PLAYER_ACTION Player::GetLastPlayerAction()
{
  return m_lastAction;
}

void Player::SetLastPlayerAction(XAPP::Player::PLAYER_ACTION action)
{
  m_lastAction = action; 
}

bool Player::IsPlaying()
{
  return g_application.IsPlaying();
}

bool Player::IsPlayingAudio()
{
  return g_application.IsPlayingAudio();
}

bool Player::IsPlayingVideo()
{
  return g_application.IsPlayingVideo();
}

bool Player::IsPaused()
{
  return g_application.IsPaused();
}

bool Player::IsCaching()
{
  return g_application.m_pPlayer->IsCaching();
}

bool Player::IsForwarding()
{
  return !g_application.IsPaused() && g_application.GetPlaySpeed() > 1;
}

bool Player::IsRewinding()
{
  return !g_application.IsPaused() && g_application.GetPlaySpeed() < 1;
}
  
void Player::SetVolume(int percent)
{
  g_application.SetVolume(percent);
}

int Player::GetVolume()
{
  return g_application.GetVolume();
}

void Player::ToggleMute()
{
  g_application.Mute();
}

}
