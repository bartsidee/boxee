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

#include "GUIWindowManager.h"
#include "GUIAudioManager.h"
#include "GUIDialog.h"
#include "Application.h"
#include "GUIPassword.h"
#include "utils/GUIInfoManager.h"
#include "utils/SingleLock.h"
#include "Util.h"
#include "GUISettings.h"
#include "Settings.h"
#include "WindowingFactory.h"
#include "Texture.h"
#include "FrameBufferObject.h"
#include "PowerManager.h"
#include "AppManager.h"

using namespace std;

CGUIWindowManager g_windowManager;
CFrameBufferObject g_WindowManagerFBO;

CGUIWindowManager::CGUIWindowManager(void)
{
  m_pCallback = NULL;
  m_bShowOverlay = true;

  m_bIsDirty = true;

  m_bUseFBO = false;
  m_bFBOValid = false;
  m_bFBOCreated = false;
}

CGUIWindowManager::~CGUIWindowManager(void)
{
}

void CGUIWindowManager::Initialize()
{
  LoadNotOnDemandWindows();
  InitializeFBO();
}

void CGUIWindowManager::InitializeFBO()
{
  m_bFBOCreated = false;
  m_bFBOValid = false;

  m_colorBufferSW.StartZero();

  unsigned int width;
  unsigned int height;

  width = g_graphicsContext.GetWidth();
  height = g_graphicsContext.GetHeight();

  bool bRenderLowRes = g_graphicsContext.GetRenderLowresGraphics();

  CLog::Log(LOGDEBUG, "CGUIWindowManager::InitializeFBO bRenderLowRes %d", bRenderLowRes);

  if(bRenderLowRes)
  {
    RESOLUTION graphicsResolution = g_graphicsContext.GetGraphicsResolution();

    width = g_settings.m_ResInfo[graphicsResolution].iWidth;
    height= g_settings.m_ResInfo[graphicsResolution].iHeight;
  }

  g_WindowManagerFBO.Cleanup();
  g_WindowManagerFBO.Initialize();
#if defined(HAS_GL) || HAS_GLES == 2
  if(g_WindowManagerFBO.CreateAndBindToTexture(GL_TEXTURE_2D, (int)width, (int)height))
  {
    m_bFBOCreated = true;
  }  
#endif
}

bool CGUIWindowManager::SendMessage(int message, int senderID, int destID, int param1, int param2)
{
  CGUIMessage msg(message, senderID, destID, param1, param2);
  return SendMessage(msg);
}

bool CGUIWindowManager::SendMessage(CGUIMessage& message)
{
  bool handled = false;
//  CLog::Log(LOGDEBUG,"SendMessage: mess=%d send=%d control=%d param1=%d", message.GetMessage(), message.GetSenderId(), message.GetControlId(), message.GetParam1());
  // Send the message to all none window targets
  for (int i = 0; i < (int) m_vecMsgTargets.size(); i++)
  {
    IMsgTargetCallback* pMsgTarget = m_vecMsgTargets[i];

    if (pMsgTarget)
    {
      if (pMsgTarget->OnMessage( message )) handled = true;
    }
  }

  //  A GUI_MSG_NOTIFY_ALL is send to any active modal dialog
  //  and all windows whether they are active or not
  if (message.GetMessage()==GUI_MSG_NOTIFY_ALL)
  {
    CSingleLock lock(g_graphicsContext);
    for (rDialog it = m_activeDialogs.rbegin(); it != m_activeDialogs.rend(); ++it)
    {
      CGUIWindow *dialog = *it;
      dialog->OnMessage(message);
    }

    for (WindowMap::iterator it = m_mapWindows.begin(); it != m_mapWindows.end(); it++)
    {
      CGUIWindow *pWindow = (*it).second;
      pWindow->OnMessage(message);
    }
    return true;
  }

  // Normal messages are sent to:
  // 1. All active modeless dialogs
  // 2. The topmost dialog that accepts the message
  // 3. The underlying window (only if it is the sender or receiver if a modal dialog is active)

  bool hasModalDialog(false);
  bool modalAcceptedMessage(false);
  // don't use an iterator for this loop, as some messages mean that m_activeDialogs is altered,
  // which will invalidate any iterator
  CSingleLock lock(g_graphicsContext);
  unsigned int topWindow = m_activeDialogs.size();
  while (topWindow)
  {
    CGUIWindow* dialog = m_activeDialogs[--topWindow];
    lock.Leave();
    if (!modalAcceptedMessage && dialog->IsModalDialog())
    { // modal window
      hasModalDialog = true;
      if (!modalAcceptedMessage && dialog->OnMessage( message ))
      {
        modalAcceptedMessage = handled = true;
      }
    }
    else if (!dialog->IsModalDialog())
    { // modeless
      if (dialog->OnMessage( message ))
        handled = true;
    }
    lock.Enter();
    if (topWindow > m_activeDialogs.size())
      topWindow = m_activeDialogs.size();
  }
  lock.Leave();

  // now send to the underlying window
  CGUIWindow* window = GetWindow(GetActiveWindow());
  if (window)
  {
    if (hasModalDialog)
    {
      // only send the message to the underlying window if it's the recipient
      // or sender (or we have no sender)
      if (message.GetSenderId() == window->GetID() ||
          message.GetControlId() == window->GetID() ||
          message.GetSenderId() == 0 )
      {
        if (window->OnMessage(message)) handled = true;
      }
    }
    else
    {
      if (window->OnMessage(message)) handled = true;
    }
  }
  return handled;
}

bool CGUIWindowManager::SendMessage(CGUIMessage& message, int window)
{
  CGUIWindow* pWindow = GetWindow(window);
  if(pWindow)
    return pWindow->OnMessage(message);
  else
    return false;
}

void CGUIWindowManager::AddUniqueInstance(CGUIWindow *window)
{
  CSingleLock lock(g_graphicsContext);
  // increment our instance (upper word of windowID)
  // until we get a window we don't have
  int instance = 0;
  while (GetWindow(window->GetID()))
    window->SetID(window->GetID() + (++instance << 16));
  Add(window);
}

void CGUIWindowManager::Add(CGUIWindow* pWindow)
{
  if (!pWindow)
  {
    CLog::Log(LOGERROR, "Attempted to add a NULL window pointer to the window manager.");
    return;
  }
  // push back all the windows if there are more than one covered by this class
  CSingleLock lock(g_graphicsContext);
  for (int i = 0; i < pWindow->GetIDRange(); i++)
  {
    WindowMap::iterator it = m_mapWindows.find(pWindow->GetID() + i);
    if (it != m_mapWindows.end())
    {
      CLog::Log(LOGERROR, "Error, trying to add a second window with id %u "
                          "to the window manager",
                pWindow->GetID());
      return;
    }
    m_mapWindows.insert(pair<int, CGUIWindow *>(pWindow->GetID() + i, pWindow));
  }
}

void CGUIWindowManager::AddCustomWindow(CGUIWindow* pWindow)
{
  CSingleLock lock(g_graphicsContext);
  Add(pWindow);
  m_vecCustomWindows.push_back(pWindow);
}

void CGUIWindowManager::RemoveCustomWindow(CGUIWindow* pWindow)
{
  CSingleLock lock(g_graphicsContext);

  for (int i = 0; i < (int)m_vecCustomWindows.size(); i++)
  {
    if(m_vecCustomWindows[i] == pWindow)
    {
      Remove(pWindow->GetID());
      m_vecCustomWindows.erase(m_vecCustomWindows.begin()+i);
    }
  }
}

void CGUIWindowManager::AddModeless(CGUIWindow* dialog)
{
  CSingleLock lock(g_graphicsContext);
  // only add the window if it's not already added
  for (iDialog it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
    if (*it == dialog) return;
  m_activeDialogs.push_back(dialog);
}

void CGUIWindowManager::Remove(int id)
{
  CSingleLock lock(g_graphicsContext);
  WindowMap::iterator it = m_mapWindows.find(id);
  if (it != m_mapWindows.end())
  {
    m_mapWindows.erase(it);
  }
  else
  {
    CLog::Log(LOGWARNING, "Attempted to remove window %u "
        "from the window manager when it didn't exist",
        id);
  }
}

// removes and deletes the window.  Should only be called
// from the class that created the window using new.
void CGUIWindowManager::Delete(int id)
{
  CSingleLock lock(g_graphicsContext);
  CGUIWindow *pWindow = GetWindow(id);
  if (pWindow)
  {
    Remove(id);
    delete pWindow;
  }
}

void CGUIWindowManager::PreviousWindow()
{
  // deactivate any window
  CSingleLock lock(g_graphicsContext);
  CLog::Log(LOGDEBUG,"CGUIWindowManager::PreviousWindow: Deactivate");
  int currentWindow = GetActiveWindow();
  CGUIWindow *pCurrentWindow = GetWindow(currentWindow);
  if (!pCurrentWindow)
    return;     // no windows or window history yet

  // check to see whether our current window has a <previouswindow> tag
  if (pCurrentWindow->GetPreviousWindow() != WINDOW_INVALID)
  {
    // TODO: we may need to test here for the
    //       whether our history should be changed

    // don't reactivate the previouswindow if it is ourselves.
    if (currentWindow != pCurrentWindow->GetPreviousWindow())
      ActivateWindow(pCurrentWindow->GetPreviousWindow());
    return;
  }
  // get the previous window in our stack
  if (m_windowHistory.size() < 2)
  { // no previous window history yet - check if we should just activate home
    if (GetActiveWindow() != WINDOW_INVALID && GetActiveWindow() != WINDOW_HOME)
    {
      ClearWindowHistory();
      ActivateWindow(WINDOW_HOME);
    }
    return;
  }
  m_windowHistory.pop();
  int previousWindow = GetActiveWindow();
  m_windowHistory.push(currentWindow);

  CGUIWindow *pNewWindow = GetWindow(previousWindow);
  if (!pNewWindow)
  {
    CLog::Log(LOGERROR, "Unable to activate the previous window");
    ClearWindowHistory();
    ActivateWindow(WINDOW_HOME);
    return;
  }

  // ok to go to the previous window now

  // tell our info manager which window we are going to
  g_infoManager.SetNextWindow(previousWindow);

  // set our overlay state (enables out animations on window change)
  HideOverlay(pNewWindow->GetOverlayState());

  // deinitialize our window
  g_audioManager.PlayWindowSound(pCurrentWindow->GetID(), SOUND_DEINIT);
  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0);
  pCurrentWindow->OnMessage(msg);

  g_infoManager.SetNextWindow(WINDOW_INVALID);
  g_infoManager.SetPreviousWindow(currentWindow);

  // remove the current window off our window stack
  m_windowHistory.pop();

  // ok, initialize the new window
  CLog::Log(LOGDEBUG,"CGUIWindowManager::PreviousWindow: Activate new");
  g_audioManager.PlayWindowSound(pNewWindow->GetID(), SOUND_INIT);
  CGUIMessage msg2(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, GetActiveWindow());
  pNewWindow->OnMessage(msg2);

  g_infoManager.SetPreviousWindow(WINDOW_INVALID);
  return;
}

void CGUIWindowManager::ChangeActiveWindow(int newWindow, const CStdString& strPath)
{
  vector<CStdString> params;
  if (!strPath.IsEmpty())
    params.push_back(strPath);
  ActivateWindow(newWindow, params, true);
}

void CGUIWindowManager::ActivateWindow(int iWindowID, const CStdString& strPath)
{
  vector<CStdString> params;
  if (!strPath.IsEmpty())
    params.push_back(strPath);
  ActivateWindow(iWindowID, params, false);
}

void CGUIWindowManager::ActivateWindow(int iWindowID, const vector<CStdString>& params, bool swappingWindows)
  {
  if (!g_application.IsCurrentThread())
  {
    // make sure graphics lock is not held
    int nCount = ExitCriticalSection(g_graphicsContext);
    g_application.getApplicationMessenger().ActivateWindow(iWindowID, params, swappingWindows);
    RestoreCriticalSection(g_graphicsContext, nCount);
  }
  else
    ActivateWindow_Internal(iWindowID, params, swappingWindows);
}

void CGUIWindowManager::ActivateWindow_Internal(int iWindowID, const vector<CStdString>& params, bool swappingWindows)
{
  bool passParams = true;
  // translate virtual windows
  // virtual music window which returns the last open music window (aka the music start window)
  if (iWindowID == WINDOW_MUSIC)
  {
    iWindowID = g_stSettings.m_iMyMusicStartWindow;
    // ensure the music virtual window only returns music files and music library windows
    if (iWindowID != WINDOW_MUSIC_NAV)
      iWindowID = WINDOW_MUSIC_FILES;
    // destination path cannot be used with virtual window
    passParams = false;
  }
  // virtual video window which returns the last open video window (aka the video start window)
  if (iWindowID == WINDOW_VIDEOS)
  {
    iWindowID = g_stSettings.m_iVideoStartWindow;
    // ensure the virtual video window only returns video windows
    if (iWindowID != WINDOW_VIDEO_NAV)
      iWindowID = WINDOW_VIDEO_FILES;
    // destination path cannot be used with virtual window
    passParams = false;
  }

  // stop video player when entering home screen
  if(iWindowID == WINDOW_HOME && g_application.IsPlayingVideo())
  {
    CLog::Log(LOGDEBUG,"CGUIWindowManager::ActivateWindow_Internal - [iWindowID=%d=WINDOW_HOME][IsPlayingVideo=TRUE] - Going to stop the video (ev)",iWindowID);
    g_application.StopPlaying();
  }

  // debug
  CLog::Log(LOGDEBUG, "Activating window ID: %i", iWindowID);

  if(!g_passwordManager.CheckMenuLock(iWindowID))
  {
    CLog::Log(LOGERROR, "MasterCode is Wrong: Window with id %d will not be loaded! Enter a correct MasterCode!", iWindowID);
    return;
  }

  // first check existence of the window we wish to activate.
  CGUIWindow *pNewWindow = GetWindow(iWindowID);
  if (!pNewWindow)
  { // nothing to see here - move along
    CLog::Log(LOGERROR, "Unable to locate window with id %d.  Check skin files", iWindowID - WINDOW_HOME);
    return ;
  }
  else if (pNewWindow->IsDialog())
  { // if we have a dialog, we do a DoModal() rather than activate the window
    if (!pNewWindow->IsDialogRunning())
      ((CGUIDialog *)pNewWindow)->DoModal(iWindowID, (passParams && params.size()) ? params[0] : "");
    return;
  }

  g_infoManager.SetNextWindow(iWindowID);

  // set our overlay state
  HideOverlay(pNewWindow->GetOverlayState());

  // deactivate any window
  int currentWindow = GetActiveWindow();
  CGUIWindow *pWindow = GetWindow(currentWindow);
  if (pWindow)
  {
    // If we got here from the screen saver, don't close the dialogs as we want to see
    // them when we come back
    if (!g_application.GetInSlideshowScreensaver())
      CloseDialogs();

    //  Play the window specific deinit sound
    g_audioManager.PlayWindowSound(pWindow->GetID(), SOUND_DEINIT);
    CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0, iWindowID);
    pWindow->OnMessage(msg);
  }
  g_infoManager.SetNextWindow(WINDOW_INVALID);

  // Add window to the history list (we must do this before we activate it,
  // as all messages done in WINDOW_INIT will want to be sent to the new
  // topmost window).  If we are swapping windows, we pop the old window
  // off the history stack
  if (swappingWindows && m_windowHistory.size())
    m_windowHistory.pop();
  AddToWindowHistory(iWindowID);

  g_infoManager.SetPreviousWindow(currentWindow);
  g_audioManager.PlayWindowSound(pNewWindow->GetID(), SOUND_INIT);
  // Send the init message
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, currentWindow, iWindowID);
  if (passParams)
    msg.SetStringParams(params);
  pNewWindow->OnMessage(msg);
//  g_infoManager.SetPreviousWindow(WINDOW_INVALID);
}

void CGUIWindowManager::CloseDialogs(bool forceClose)
{
  CSingleLock lock(g_graphicsContext);
  std::vector<CGUIWindow*> vecCloseDialogs;

  vecCloseDialogs.insert(vecCloseDialogs.begin(),m_activeDialogs.begin(),m_activeDialogs.end());

  for (size_t dlg=0; dlg < vecCloseDialogs.size(); dlg++)
  {
    CGUIWindow* win = vecCloseDialogs[dlg];

    //attention: the Close() function below will remove the dialog from m_activeDialogs using this thread, using the same lock.
    win->Close(forceClose);
  }
}

bool CGUIWindowManager::OnAction(const CAction &action)
{
  if (g_powerManager.IsSuspended())
  {
    g_powerManager.Resume();
    return true;
  }
  
  // If the screen saver is active, it is on top of
  // existing dialogs. Pass the actions to the screen
  // saver and not to the dialogs
  if (!g_application.GetInSlideshowScreensaver()) 
  {
    CSingleLock lock(g_graphicsContext);
    unsigned int topMost = m_activeDialogs.size();
    while (topMost)
    {
      CGUIWindow *dialog = m_activeDialogs[--topMost];
      lock.Leave();
      if (dialog->IsModalDialog())
      { // we have the topmost modal dialog
        if (!dialog->IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
        {
          bool fallThrough = (dialog->GetID() == WINDOW_DIALOG_FULLSCREEN_INFO);
          if (dialog->OnAction(action))
            return true;
          // dialog didn't want the action - we'd normally return false
          // but for some dialogs we want to drop the actions through
          if (fallThrough)
            break;
          return false;
        }
        return true; // do nothing with the action until the anim is finished
      }
      // music or video overlay are handled as a special case, as they're modeless, but we allow
      // clicking on them with the mouse.
      if (action.id == ACTION_MOUSE && (dialog->GetID() == WINDOW_VIDEO_OVERLAY ||
                                        dialog->GetID() == WINDOW_MUSIC_OVERLAY))
      {
        if (dialog->OnAction(action))
          return true;
      }
      lock.Enter();
      if (topMost > m_activeDialogs.size())
        topMost = m_activeDialogs.size();
    }
    lock.Leave();
  }

  CGUIWindow* window = GetWindow(GetActiveWindow());
  if (window)
    return window->OnAction(action);
  return false;
}

void CGUIWindowManager::Render()
{

  if (!g_application.IsCurrentThread())
  {
    // make sure graphics lock is not held
    int nCount = ExitCriticalSection(g_graphicsContext);
    g_application.getApplicationMessenger().Render();
    RestoreCriticalSection(g_graphicsContext, nCount);
  }
  else
  {
    g_graphicsContext.ApplyGuiTransform();
    Render_Internal();
    g_graphicsContext.RestoreGuiTransform();
  }
}

void CGUIWindowManager::Render_Internal()
{
  CSingleLock lock(g_graphicsContext);

  int nActive = GetActiveWindow();
  CGUIWindow* pWindow = GetWindow(nActive);
  if(!pWindow && (nActive < WINDOW_APPS_START || nActive > WINDOW_APPS_END))
  {
    g_graphicsContext.Clear();
    return;
  }

  m_bUseFBO = g_WindowManagerFBO.IsSupported() && IsColorBufferActive();

  // FBO don't work in Win32 GLES emulator
#if defined(_WIN32) && defined(HAS_GLES)
  m_bUseFBO = false;
#endif

  if(g_graphicsContext.GetRenderLowresGraphics())
    m_bUseFBO = false;

#ifdef HAS_EMBEDDED
  if(g_application.IsPlaying())
    m_bUseFBO = false;
#endif

  if(!m_bUseFBO)
  {
    m_bFBOValid = false;
    pWindow->Render();
  }
  else
  {
    RenderUsingFBO(pWindow);
  }
}

void CGUIWindowManager::RenderUsingFBO(CGUIWindow* pWindow)
{
  ASSERT(pWindow);

  //Redraw the background every x seconds
  if(m_colorBufferSW.GetElapsedSeconds() >= 5.0f)
  {
    m_bFBOValid = false;
    m_colorBufferSW.StartZero();
  }

  unsigned int width = g_graphicsContext.GetWidth();
  unsigned int height = g_graphicsContext.GetHeight();

  RESOLUTION graphicsResolution = g_graphicsContext.GetGraphicsResolution();
  bool bRenderLowRes = g_graphicsContext.GetRenderLowresGraphics();

  if(bRenderLowRes)
  {
    width = g_settings.m_ResInfo[graphicsResolution].iWidth;
    height= g_settings.m_ResInfo[graphicsResolution].iHeight;
  }

  if (width != g_WindowManagerFBO.GetWidth() || height != g_WindowManagerFBO.GetHeight())
  {
    m_bFBOValid = false;
    InitializeFBO();
  }

  if(!m_bFBOCreated)
    return;

  if(!m_bFBOValid)
  {
    g_WindowManagerFBO.BeginRender();
    g_graphicsContext.Clear();
    pWindow->Render();
    g_WindowManagerFBO.EndRender();
    m_bFBOValid = true;
  }

  g_graphicsContext.PushTransform(TransformMatrix(), true);
  g_graphicsContext.PushViewPort(0, 0, 0, 0, false);
  bool clip = g_graphicsContext.SetClipRegion(0, 0, 0, 0, false);

  uint32_t diffuseColor = 0xffffffff;

  CTexture texture;
  texture.LoadFromTexture((int)width, (int)height, XB_FMT_R8G8B8A8, g_WindowManagerFBO.Texture());

  CTextureInfo textureInfo("");
  textureInfo.blendCenter = true;
  textureInfo.orientation = 3; // flipy
  CGUITexture guiTexture(0, 0, (float)width, (float)height, textureInfo, &texture);
  guiTexture.SetWidth(g_graphicsContext.GetWidth());
  guiTexture.SetHeight(g_graphicsContext.GetHeight());
  guiTexture.SetDiffuseColor(diffuseColor);
  guiTexture.Render();

  g_graphicsContext.PopTransform();
  g_graphicsContext.PopViewPort();
  if(clip)
    g_graphicsContext.RestoreClipRegion();
}

void CGUIWindowManager::BuildStencilBuffer(CGUIWindow* pWindow)
{
#if 0
  // test rendering the stencil buffer
  unsigned int width = g_graphicsContext.GetWidth();
  unsigned int height = g_graphicsContext.GetHeight();
#endif

  // build the topmost controls mask in the stencil buffer

  g_Windowing.ClearStencilBuffer(0);

  g_Windowing.EnableStencil(true);
  g_Windowing.SetColorMask(false, false, false, false);
  g_Windowing.SetStencilFunc(STENCIL_FUNC_ALWAYS, 1, 1);
  g_Windowing.SetStencilOp(STENCIL_OP_KEEP, STENCIL_OP_INCR, STENCIL_OP_INCR);

  if (pWindow)
    pWindow->Render();

  g_Windowing.SetColorMask(true, true, true, true);

  g_Windowing.SetStencilFunc(STENCIL_FUNC_EQUAL, 0, 1);
  g_Windowing.SetStencilOp(STENCIL_OP_KEEP, STENCIL_OP_KEEP, STENCIL_OP_KEEP);

  if (pWindow)
    pWindow->Render();

  g_Windowing.EnableStencil(false);


  return;
#if 0

// TEST: render the stencil buffer on screen
  static unsigned char* stencilBuf = NULL;
  if(stencilBuf == NULL)
    stencilBuf = new unsigned char[width * height];

  static unsigned char* rgbBuf = NULL;
  if(rgbBuf == NULL)
    rgbBuf = new unsigned char[width * height * 4];

  memset(stencilBuf, 0, width * height);
  memset(rgbBuf, 0, width * height * 4);

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, width, height, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilBuf);

  int scale = 16;

  for(unsigned int j = 0; j < height; j++)
    for(unsigned int i = 0; i < width; i++)
    {
      int pos = j * width + i;
      rgbBuf[4 * pos + 0] = scale * stencilBuf[pos];
      rgbBuf[4 * pos + 1] = scale * stencilBuf[pos];
      rgbBuf[4 * pos + 2] = scale * stencilBuf[pos];
      rgbBuf[4 * pos + 3] = 255;
    }

  TransformMatrix mat;
  g_graphicsContext.PushTransform(mat, true);
  g_graphicsContext.PushViewPort(0, 0, 0, 0, false);
  g_graphicsContext.SetClipRegion(0, 0, 0, 0, false);

  CTexture texture;
  texture.LoadFromMemory((int)width, (int)height, width * 4,  XB_FMT_R8G8B8A8, rgbBuf);

  CTextureInfo textureInfo("");
  textureInfo.blendCenter = false;
  textureInfo.orientation = 3; // flipy
  CGUITexture guiTexture(0, 0, (float)width, (float)height, textureInfo, &texture);
  guiTexture.SetDiffuseColor(0xffffffff);
  guiTexture.Render();

  g_graphicsContext.PopTransform();
  g_graphicsContext.PopViewPort();
  g_graphicsContext.RestoreClipRegion();
#endif
}

bool RenderOrderSortFunction(CGUIWindow *first, CGUIWindow *second)
{
  return first->GetRenderOrder() < second->GetRenderOrder();
}

bool CGUIWindowManager::IsColorBufferActive()
{
  CGUIWindow* pWindow = GetWindow(GetActiveWindow());

  if(!pWindow)
    return false;

  if(pWindow->HasDynamicContents())
    return false;

  if(pWindow->IsAnimating())
    return false;

  int mode3d = MODE_3D_NONE;
#ifdef HAS_EMBEDDED
  mode3d = g_guiSettings.GetInt("videoscreen.mode3d");
#endif

  if(mode3d != MODE_3D_NONE)
    return false;

  // find the window with the lowest render order
  vector<CGUIWindow *> renderList = m_activeDialogs;
  stable_sort(renderList.begin(), renderList.end(), RenderOrderSortFunction);

  // iterate through all dialogs. If the dialog enable color buffer return true;
  for (iDialog it = renderList.begin(); it != renderList.end(); ++it)
  {
    if ((*it)->IsDialogRunning()) 
    {
      if ((*it)->IsAnimating())
      {
        return false;
      }
      else if((*it)->HasColorBufferActive())
          return true;
    }
  }

  return false;
}

void CGUIWindowManager::RenderDialogs()
{
  // If the screen saver is active, do not render dialogs as
  // they will appear on top of it
  if (g_application.GetInSlideshowScreensaver())
     return;

  CSingleLock lock(g_graphicsContext);

  // find the window with the lowest render order
  vector<CGUIWindow *> renderList = m_activeDialogs;
  stable_sort(renderList.begin(), renderList.end(), RenderOrderSortFunction);

  // iterate through and render if they're running
  g_graphicsContext.ApplyGuiTransform();
  for (iDialog it = renderList.begin(); it != renderList.end(); ++it)
  {
    if ((*it)->IsDialogRunning())
    {
      (*it)->Render();
    }
  }
  g_graphicsContext.RestoreGuiTransform();
}

CGUIWindow* CGUIWindowManager::GetWindow(int id) const
{
  if (id == WINDOW_INVALID)
  {
    return NULL;
  }

  CSingleLock lock(g_graphicsContext);
  WindowMap::const_iterator it = m_mapWindows.find(id);
  if (it != m_mapWindows.end())
    return (*it).second;
  return NULL;
}

int CGUIWindowManager::GetPreviousWindow() const
{
  return m_iPreviousWindow;
}

void CGUIWindowManager::SetPreviousWindow(int iPreviousWindow)
{
  m_iPreviousWindow = iPreviousWindow;
}

// Shows and hides modeless dialogs as necessary.
void CGUIWindowManager::UpdateModelessVisibility()
{
  CSingleLock lock(g_graphicsContext);
  for (WindowMap::iterator it = m_mapWindows.begin(); it != m_mapWindows.end(); it++)
  {
    CGUIWindow *pWindow = (*it).second;
    if (pWindow && pWindow->IsDialog() && pWindow->GetVisibleCondition())
    {
      if (g_infoManager.GetBool(pWindow->GetVisibleCondition(), GetActiveWindow()))
        ((CGUIDialog *)pWindow)->Show();
      else
        ((CGUIDialog *)pWindow)->Close();
    }
  }
}

void CGUIWindowManager::Process(bool renderOnly /*= false*/)
{
  if (g_application.IsCurrentThread())
    Process_Internal(renderOnly);
}

void CGUIWindowManager::Process_Internal(bool renderOnly /*= false*/)
{
  if (m_pCallback)
  {
    if (!renderOnly)
    {
      m_pCallback->Process();
      m_pCallback->FrameMove();
    }
    m_pCallback->Render();
  }
}

void CGUIWindowManager::SetCallback(IWindowManagerCallback& callback)
{
  m_pCallback = &callback;
}

void CGUIWindowManager::DeInitialize()
{
  CSingleLock lock(g_graphicsContext);
  for (WindowMap::iterator it = m_mapWindows.begin(); it != m_mapWindows.end(); it++)
  {
    CGUIWindow* pWindow = (*it).second;
    if (IsWindowActive(it->first))
    {
      pWindow->DisableAnimations();
      CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0);
      pWindow->OnMessage(msg);
    }
    pWindow->ResetControlStates();
    pWindow->FreeResources(true);
  }
  UnloadNotOnDemandWindows();

  m_vecMsgTargets.erase( m_vecMsgTargets.begin(), m_vecMsgTargets.end() );

  // destroy our custom windows...
  for (int i = 0; i < (int)m_vecCustomWindows.size(); i++)
  {
    CGUIWindow *pWindow = m_vecCustomWindows[i];
    Remove(pWindow->GetID());
    delete pWindow;
  }

  // clear our vectors of windows
  m_vecCustomWindows.clear();
  m_activeDialogs.clear();
  ClearWindowHistory();
}

/// \brief Route to a window
/// \param pWindow Window to route to
void CGUIWindowManager::RouteToWindow(CGUIWindow* dialog)
{
  CSingleLock lock(g_graphicsContext);
  // Just to be sure: Unroute this window,
  // #we may have routed to it before
  RemoveDialog(dialog->GetID());

  m_activeDialogs.push_back(dialog);
}

/// \brief Unroute window
/// \param id ID of the window routed
void CGUIWindowManager::RemoveDialog(int id)
{
  CSingleLock lock(g_graphicsContext);
  for (iDialog it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    if ((*it)->GetID() == id)
    {
      m_activeDialogs.erase(it);
      return;
    }
  }
}

bool CGUIWindowManager::HasModalDialog() const
{
  // If the screen saver is active, don't tell anyone that there
  // are any dialogs open, so the window will get the events and
  // not the dialogs
  if (g_application.GetInSlideshowScreensaver())
     return false;

  CSingleLock lock(g_graphicsContext);
  for (ciDialog it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    CGUIWindow *window = *it;
    if (window->IsModalDialog())
    { // have a modal window
      if (!window->IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
        return true;
    }
  }
  return false;
}

bool CGUIWindowManager::HasDialogOnScreen() const
{
  return (m_activeDialogs.size() > 0);
}

/// \brief Get the ID of the top most routed window
/// \return id ID of the window or WINDOW_INVALID if no routed window available
int CGUIWindowManager::GetTopMostModalDialogID() const
{
  // If the screen saver is active, don't tell anyone that there
  // are any dialogs open, so the window will get the events and
  // not the dialogs
  if (g_application.GetInSlideshowScreensaver())
     return WINDOW_INVALID;

  CSingleLock lock(g_graphicsContext);
  for (crDialog it = m_activeDialogs.rbegin(); it != m_activeDialogs.rend(); ++it)
  {
    CGUIWindow *dialog = *it;
    if (dialog->IsModalDialog())
    { // have a modal window
      return dialog->GetID();
    }
  }
  return WINDOW_INVALID;
}

void CGUIWindowManager::SendThreadMessage(CGUIMessage& message)
{
  CSingleLock lock(m_critSection);

  CGUIMessage* msg = new CGUIMessage(message);
  m_vecThreadMessages.push_back( pair<CGUIMessage*,int>(msg,0) );
  lock.Leave();

  Sleep(0); // hopefully causes gui thread to take over and process the message (optimization)
}

void CGUIWindowManager::SendThreadMessage(CGUIMessage& message, int window)
{
  CSingleLock lock(m_critSection);

  CGUIMessage* msg = new CGUIMessage(message);
  m_vecThreadMessages.push_back( pair<CGUIMessage*,int>(msg,window) );
}

void CGUIWindowManager::DispatchThreadMessages()
{
  CSingleLock lock(m_critSection);
  vector< pair<CGUIMessage*,int> > messages(m_vecThreadMessages);
  m_vecThreadMessages.erase(m_vecThreadMessages.begin(), m_vecThreadMessages.end());
  lock.Leave();

  while ( messages.size() > 0 )
  {
    vector< pair<CGUIMessage*,int> >::iterator it = messages.begin();
    CGUIMessage* pMsg = it->first;
    int window = it->second;
    // first remove the message from the queue,
    // else the message could be processed more then once
    it = messages.erase(it);

    if (window)
      SendMessage( *pMsg, window );
    else
      SendMessage( *pMsg );
    delete pMsg;
  }
}

void CGUIWindowManager::AddMsgTarget( IMsgTargetCallback* pMsgTarget )
{
  bool bFound = false;
  for (size_t i=0; !bFound && i<m_vecMsgTargets.size(); i++)
    if ( m_vecMsgTargets[i] == pMsgTarget )
      bFound = true;

  if (!bFound)
    m_vecMsgTargets.push_back( pMsgTarget );
}

int CGUIWindowManager::GetActiveWindow() const
{
  if (!m_windowHistory.empty())
    return m_windowHistory.top();
  return WINDOW_INVALID;
}

// same as GetActiveWindow() except it first grabs dialogs
int CGUIWindowManager::GetFocusedWindow() const
{
  int dialog = GetTopMostModalDialogID();
  if (dialog != WINDOW_INVALID)
    return dialog;

  return GetActiveWindow();
}

bool CGUIWindowManager::IsWindowActive(int id, bool ignoreClosing /* = true */) const
{
  // mask out multiple instances of the same window
  id &= WINDOW_ID_MASK;
  if ((GetActiveWindow() & WINDOW_ID_MASK) == id) return true;
  // run through the dialogs
  CSingleLock lock(g_graphicsContext);
  for (ciDialog it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    CGUIWindow *window = *it;
    if ((window->GetID() & WINDOW_ID_MASK) == id && (!ignoreClosing || !window->IsAnimating(ANIM_TYPE_WINDOW_CLOSE)))
      return true;
  }
  return false; // window isn't active
}

bool CGUIWindowManager::IsWindowActive(const CStdString &xmlFile, bool ignoreClosing /* = true */) const
{
  CSingleLock lock(g_graphicsContext);
  CGUIWindow *window = GetWindow(GetActiveWindow());
  if (window && CUtil::GetFileName(window->GetXMLFile()).Equals(xmlFile)) return true;
  // run through the dialogs
  for (ciDialog it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    CGUIWindow *window = *it;
    if (CUtil::GetFileName(window->GetXMLFile()).Equals(xmlFile) && (!ignoreClosing || !window->IsAnimating(ANIM_TYPE_WINDOW_CLOSE)))
      return true;
  }
  return false; // window isn't active
}

bool CGUIWindowManager::IsWindowVisible(int id) const
{
  return IsWindowActive(id, false);
}

bool CGUIWindowManager::IsWindowVisible(const CStdString &xmlFile) const
{
  return IsWindowActive(xmlFile, false);
}

void CGUIWindowManager::LoadNotOnDemandWindows()
{
  CSingleLock lock(g_graphicsContext);
  for (WindowMap::iterator it = m_mapWindows.begin(); it != m_mapWindows.end(); it++)
  {
    CGUIWindow *pWindow = (*it).second;
    if (!pWindow ->GetLoadOnDemand())
    {
      pWindow->FreeResources(true);
      pWindow->Initialize();
    }
  }
}

void CGUIWindowManager::UnloadNotOnDemandWindows()
{
  CSingleLock lock(g_graphicsContext);
  for (WindowMap::iterator it = m_mapWindows.begin(); it != m_mapWindows.end(); it++)
  {
    CGUIWindow *pWindow = (*it).second;
    if (!pWindow->GetLoadOnDemand())
    {
      pWindow->FreeResources(true);
    }
  }
}

bool CGUIWindowManager::IsOverlayAllowed() const
{
  return m_bShowOverlay;
}

void CGUIWindowManager::ShowOverlay(CGUIWindow::OVERLAY_STATE state)
{
  if (state != CGUIWindow::OVERLAY_STATE_PARENT_WINDOW)
    m_bShowOverlay = state == CGUIWindow::OVERLAY_STATE_SHOWN;
}

void CGUIWindowManager::HideOverlay(CGUIWindow::OVERLAY_STATE state)
{
  if (state == CGUIWindow::OVERLAY_STATE_HIDDEN)
    m_bShowOverlay = false;
}

void CGUIWindowManager::AddToWindowHistory(int newWindowID)
{
  // Check the window stack to see if this window is in our history,
  // and if so, pop all the other windows off the stack so that we
  // always have a predictable "Back" behaviour for each window
  stack<int> historySave = m_windowHistory;
  while (historySave.size())
  {
    if (historySave.top() == newWindowID)
      break;
    historySave.pop();
  }
  if (!historySave.empty())
  { // found window in history
    m_windowHistory = historySave;
  }
  else
  { // didn't find window in history - add it to the stack
    m_windowHistory.push(newWindowID);
  }
} 

void CGUIWindowManager::GetActiveModelessWindows(vector<int> &ids)
{
  // run through our modeless windows, and construct a vector of them
  // useful for saving and restoring the modeless windows on skin change etc.
  CSingleLock lock(g_graphicsContext);
  for (iDialog it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    if (!(*it)->IsModalDialog())
      ids.push_back((*it)->GetID());
  }
}

CGUIWindow *CGUIWindowManager::GetTopMostDialog() const
{
  CSingleLock lock(g_graphicsContext);
  // find the window with the lowest render order
  vector<CGUIWindow *> renderList = m_activeDialogs;
  stable_sort(renderList.begin(), renderList.end(), RenderOrderSortFunction);

  if (!renderList.size())
    return NULL;

  // return the last window in the list
  return *renderList.rbegin();
}

bool CGUIWindowManager::IsWindowTopMost(int id) const
{
  CGUIWindow *topMost = GetTopMostDialog();
  if (topMost && (topMost->GetID() & WINDOW_ID_MASK) == id)
    return true;
  return false;
}

bool CGUIWindowManager::IsWindowTopMost(const CStdString &xmlFile) const
{
  CGUIWindow *topMost = GetTopMostDialog();
  if (topMost && CUtil::GetFileName(topMost->GetXMLFile()).Equals(xmlFile))
    return true;
  return false;
}

void CGUIWindowManager::ClearWindowHistory()
{
  while (m_windowHistory.size())
    m_windowHistory.pop();
}

//void CGUIWindowManager::AddWindowObserver(DWORD dwObserverWindow, DWORD dwObervationTarget)
//{
//  std::map<DWORD, std::set<DWORD> >::iterator it = m_mapWindowObservers.find(dwObervationTarget);
//  if (it == m_mapWindowObservers.end())
//  {
//    std::set<DWORD> observers;
//    observers.insert(dwObserverWindow);
//    m_mapWindowObservers[dwObervationTarget] = observers;
//  }
//  else
//  {
//    m_mapWindowObservers[dwObervationTarget].insert(dwObserverWindow);
//  }
//
//}

//std::set<DWORD> CGUIWindowManager::GetWindowObservers(DWORD dwWindowId)
//{
//  std::map<DWORD, std::set<DWORD> >::iterator it = m_mapWindowObservers.find(dwWindowId);
//  if (it != m_mapWindowObservers.end())
//  {
//    return it->second;
//  }
//  else
//  {
//    std::set<DWORD> empty;
//    return empty;
//  }
//}

#ifdef _DEBUG
void CGUIWindowManager::DumpTextureUse()
{
  CGUIWindow* pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
    pWindow->DumpTextureUse();

  CSingleLock lock(g_graphicsContext);
  for (iDialog it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    if ((*it)->IsDialogRunning())
      (*it)->DumpTextureUse();
  }
}
#endif


bool CGUIWindowManager::IsWindowInHistory(int id) const
{
  std::stack<int> temp = m_windowHistory;
  while (temp.size())
  {
    int top = temp.top();
    if (top == id) return true;
    temp.pop();
  }

  return false;
}
