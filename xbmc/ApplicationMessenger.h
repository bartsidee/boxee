#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "utils/CriticalSection.h"
#include "StdString.h"

#include <deque>

class CFileItem;
#include "GUIDialog.h"


//Boxee
#include "lib/libBoxee/bxbgprocess.h"
//end Boxee

// defines here
#define TMSG_DIALOG_DOMODAL       100
#define TMSG_WRITE_SCRIPT_OUTPUT  101
#define TMSG_EXECUTE_SCRIPT       102
#define TMSG_EXECUTE_BUILT_IN     103
#define TMSG_EXECUTE_OS           104

#define TMSG_MEDIA_PLAY           200
#define TMSG_MEDIA_STOP           201
#define TMSG_MEDIA_PAUSE          202
#define TMSG_MEDIA_RESTART        203

#define TMSG_PLAYLISTPLAYER_PLAY  210
#define TMSG_PLAYLISTPLAYER_NEXT  211
#define TMSG_PLAYLISTPLAYER_PREV  212

#define TMSG_PICTURE_SHOW         220
#define TMSG_PICTURE_SLIDESHOW    221
#define TMSG_SLIDESHOW_SCREENSAVER  222

#define TMSG_SHUTDOWN             300
#define TMSG_POWERDOWN            301
#define TMSG_QUIT                 302
#define TMSG_DASHBOARD            TMSG_QUIT
#define TMSG_HIBERNATE            303
#define TMSG_SUSPEND              304
#define TMSG_RESTART              305
#define TMSG_RESET                306
#define TMSG_RESTARTAPP           307
#define TMSG_SWITCHTOFULLSCREEN   308
#define TMSG_MINIMIZE             309
#define TMSG_LOGOUT               310

#define TMSG_HTTPAPI              400

#define TMSG_NETWORKMESSAGE         500

#define TMSG_LOAD_STRINGS         599
#define TMSG_GUI_DO_MODAL             600
#define TMSG_GUI_SHOW                 601
#define TMSG_GUI_WIN_MANAGER_PROCESS  602
#define TMSG_GUI_WIN_MANAGER_RENDER   603
#define TMSG_GUI_ACTIVATE_WINDOW      604

#define TMSG_OPTICAL_MOUNT        700 
#define TMSG_OPTICAL_UNMOUNT      701 

#define TMSG_ACTIVATE_WINDOW  818
#define TMSG_INIT_WINDOW      819
#define TMSG_GENERAL_MESSAGE  820
#define TMSG_SET_CONTROL_LABEL 821
#define TMSG_DELETE_BG_LOADER 823
#define TMSG_LOAD_LANG_INFO   824
#define TMSG_CLOSE_DIALOG     825
#define TMSG_PREVIOUS_WINDOW  826
#define TMSG_DIALOG_PROGRESS_SHOWMODAL 827
#define TMSG_LAUNCH_APP  830
#define TMSG_TOGGLEFULLSCREEN   831

#define TMSG_FREE_WINDOW_RESOURCES 840
#define TMSG_FREE_TEXTURE           841
#define TMSG_VIDEO_RENDERER_PREINIT 842
#define TMSG_VIDEO_RENDERER_UNINIT 843
#define TMSG_SHOW_POST_PLAY_DIALOG 844
#define TMSG_DELETE_PLAYER         845
#define TMSG_SHOW_PLAY_ERROR       846
#define TMSG_CLOSE_SLIDESHOWPIC    847

#define TMSG_EXECUTE_ON_MAIN_THREAD 900

typedef struct
{
  DWORD dwMessage;
  DWORD dwParam1;
  DWORD dwParam2;
  CStdString strParam;
  std::vector<CStdString> params;
  std::string strParam2;
  HANDLE hWaitEvent;
  LPVOID lpVoid;
  ThreadIdentifier waitingThreadId;
}
ThreadMessage;

class IGUIThreadTask
{
public:
  IGUIThreadTask() {}
  virtual ~IGUIThreadTask() {}
  virtual void DoWork() = 0;
};

class CApplicationMessenger
{

public:
  CApplicationMessenger();
  virtual ~CApplicationMessenger();
  void Cleanup();
  // if a message has to be send to the gui, use MSG_TYPE_WINDOW instead
  void SendMessage(ThreadMessage& msg, bool wait = false);
  void ProcessMessages(); // only call from main thread.
  void ProcessWindowMessages();

  void MediaPlay(std::string filename);
  void MediaPlay(const CFileItem &item);
  void MediaStop();
  void MediaPause();
  void MediaRestart(bool bWait);

  void PlayListPlayerPlay();
  void PlayListPlayerPlay(int iSong);
  void PlayListPlayerNext();
  void PlayListPlayerPrevious();
  void PlayFile(const CFileItem &item, bool bRestart = false); // thread safe version of g_application.PlayFile()
  void PictureShow(std::string filename);
  void PictureSlideShow(std::string pathname, bool bScreensaver = false);
  void Shutdown();
  void Powerdown();
  void Quit();
  void Hibernate();
  void Suspend();
  void Logout();
  void Restart();
  void RebootToDashBoard();
  void RestartApp();
  void Reset();
  void SwitchToFullscreen(); //
  void ToggleFullscreen(); //
  void Minimize(bool wait = false); 
  void ExecOS(const CStdString command, bool waitExit = false);  

  CStdString GetResponse();
  int SetResponse(CStdString response);
  void HttpApi(std::string cmd, bool wait = false);
  void ExecBuiltIn(const CStdString &command);

  void NetworkMessage(DWORD dwMessage, DWORD dwParam = 0);

  void DoModal(CGUIDialog *pDialog, int iWindowID, const CStdString &param = "");
  void Show(CGUIDialog *pDialog);
  void LoadStrings(const CStdString& strFileName, const CStdString& strFallbackFileName="special://xbmc/language/english/strings.xml");
  void WindowManagerProcess(bool renderOnly = false); // will call g_windowManager.Process on the rendering thread
  void Render(); // will call g_windowManager.Render on the rendering thread
  void ActivateWindow(int windowID, const std::vector<CStdString> &params, bool swappingWindows);

  void OpticalMount(CStdString device, bool bautorun=false); 
  void OpticalUnMount(CStdString device);

//Boxee
  bool InitializeWindow(CGUIWindow *pWindow);
  bool RunJob(BOXEE::BXBGJob *aJob);
  bool LoadLangInfo(const CStdString &strFileName);
  void CloseDialog(CGUIDialog *dlg, bool bForce=false);
  void SetLabel(CGUIControl* control, int iWindowID, const CStdString &label);
  void SendGUIMessageToWindow(CGUIMessage& message, int iWindowID, bool wait = true);
  void DialogProgressShow();
  void PreviousWindow();
  void ExecuteOnMainThread(IGUIThreadTask *t, bool wait = true, bool autoDel = false);
  void ReleaseThreadMessages(ThreadIdentifier thread);
  void SetSwallowMessages(bool bSwallow);
//end Boxee

  void WindowFreeResources(CGUIWindow *win);
  
private:
  void ProcessMessage(ThreadMessage *pMsg);

  std::deque<ThreadMessage*> m_vecMessages;
  std::deque<ThreadMessage*> m_vecWindowMessages;
  CCriticalSection m_critSection;
  CCriticalSection m_critBuffer;
  CStdString bufferResponse;

//Boxee
  BOXEE::BXBGProcess m_bgProcessor;
  bool m_bSwallowMessages;
//end Boxee
};
