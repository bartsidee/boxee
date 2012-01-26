/* native applications - framework for boxee native applications
 * Copyright (C) 2010 Boxee.tv.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "system.h"

#ifdef HAS_NATIVE_APPS

#include "NativeApplicationWindow.h"
#include "GUIWindowManager.h"
#include "AppManager.h"
#include "utils/SingleLock.h"
#include "BXNativeApp.h"
#include "Application.h"
#include "cores/VideoRenderers/RenderManager.h"

#if defined(HAS_DX)
#include "NativeApplicationRenderHelpersDX.h"
#else
#include "NativeApplicationRenderHelpersGL.h"
#endif

#ifdef __APPLE__
#include <Carbon/Carbon.h>
#endif

using namespace BOXEE;

BX_Surface* BXSurfaceCreate( BX_Handle hApp, BX_PixelFormat pixelFormat, unsigned int nWidth, unsigned int nHeight );
void BXSurfaceRelease (BX_Surface* surface );
void BXSurfaceLock( BX_Surface* surface );
void BXSurfaceUnlock( BX_Surface* surface );


NativeApplicationWindow::NativeApplicationWindow(BX_Handle hApp, BX_WindowHandle win) : CGUIWindow(CAppManager::GetInstance().AcquireWindowID(),"")
{
  m_bPlayingVideo = false;
  m_hApp   = hApp;
  m_handle = win;
  m_fb = BXSurfaceCreate(hApp, BX_PF_BGRA8888, 1280, 720);
  m_flipEvent = CreateEvent(NULL, false, false, NULL);
  g_windowManager.Add(this);
}

NativeApplicationWindow::~NativeApplicationWindow()
{
  if (GetID() == g_windowManager.GetActiveWindow())
    g_windowManager.PreviousWindow();

  CAppManager::GetInstance().ReleaseWindowID(GetID());
  g_windowManager.Remove(GetID());
  BXSurfaceRelease(m_fb);

  ::CloseHandle(m_flipEvent);
}

void NativeApplicationWindow::OnInitWindow()
{
  CGUIWindow::OnInitWindow();
  SetProperty("PassthroughKeys",true);
}

void NativeApplicationWindow::Render()
{  
  g_graphicsContext.CaptureStateBlock();
  
  if (g_application.IsPlayingVideo())
  {
    if (!m_bPlayingVideo)
    {
      m_bPlayingVideo = true;
      ClearSurfaceJob j(m_fb,0.0);
      j.DoWork();
    }
    
    color_t alpha = g_graphicsContext.MergeAlpha(0xFF000000) >> 24;
    g_renderManager.RenderUpdate(false, 0, alpha);
  }
  else
    m_bPlayingVideo = false;

  ((BOXEE::NativeApplication *)(m_hApp->boxeeData))->ExecuteRenderOperations();
  
  NativeAppRenderToScreenHelper h(m_fb);
  h.Render();
    
  ::SetEvent(m_flipEvent);
  g_graphicsContext.ApplyStateBlock();
}

bool NativeApplicationWindow::OnMessage(CGUIMessage& message)
{  
  return CGUIWindow::OnMessage(message);
}

bool NativeApplicationWindow::OnAction(const CAction &action)
{
  if (!m_hApp || !m_hApp->boxeeData)
    return CGUIWindow::OnAction(action);
    
  BOXEE::NativeApplication *app = (BOXEE::NativeApplication *)m_hApp->boxeeData;
  if (!app)
    return CGUIWindow::OnAction(action);

  switch (action.id)
  {
    case ACTION_PREVIOUS_MENU:
      app->OnBack(m_handle);
      return true;
    case ACTION_MOVE_LEFT:
      app->OnLeft(m_handle);
      return true;
    case ACTION_MOVE_RIGHT:
      app->OnRight(m_handle);
      return true;
    case ACTION_MOVE_UP:
      app->OnUp(m_handle);
      return true;
    case ACTION_MOVE_DOWN:
      app->OnDown(m_handle);
      return true;
    case ACTION_SELECT_ITEM:
      app->OnEnter(m_handle);
      return true;
    case ACTION_PAUSE:
      app->OnPause(m_handle);
      return true;
    case ACTION_PLAY:
      app->OnPlay(m_handle);
      return true;
    case ACTION_STEP_FORWARD:
      app->OnSkipFw(m_handle);
      return true;
    case ACTION_STEP_BACK:
      app->OnSkipBw(m_handle);
      return true;
    case ACTION_VOLUME_UP:
    case ACTION_VOLUME_DOWN:
      return CGUIWindow::OnAction(action);
  }
  
  wchar_t code = action.unicode;
  if (code == 0 && action.strAction == "PreviousMenu") // hack to get "delete" into the app
    code = 8;

  bool bAltDown = false;
#ifdef __APPLE__
  // workaround - if "ALT" (option) is pressed - for now - do not relay the message - crashes some nativeapps on apple
  bAltDown = (GetCurrentEventKeyModifiers() & optionKey) != 0;
#endif
  
  if (code && m_hApp && m_hApp->boxeeData && !bAltDown )
  {
    BOXEE::NativeApplication *app = (BOXEE::NativeApplication *)m_hApp->boxeeData;
    app->OnKey(m_handle, code);
    return true;
  }
  
  return CGUIWindow::OnAction(action);
}

bool NativeApplicationWindow::OnMouseClick(int button, const CPoint &point)
{
  if (m_hApp && m_hApp->boxeeData)
  {
    BOXEE::NativeApplication *app = (BOXEE::NativeApplication *)m_hApp->boxeeData;
    app->OnMouseClick(m_handle, point.x, point.y);
    return true;
  }  
  return false;
}

void NativeApplicationWindow::Flip()
{
  FlipJob* job = new FlipJob(m_fb);
  ((BOXEE::NativeApplication *)(m_hApp->boxeeData))->PushRenderOperation(job);
  ((BOXEE::NativeApplication *)(m_hApp->boxeeData))->Flip();

  ::WaitForSingleObject(m_flipEvent, 30);
}

BX_Surface *NativeApplicationWindow::GetFrameBuffer()
{ 
  return m_fb; 
}

#endif
