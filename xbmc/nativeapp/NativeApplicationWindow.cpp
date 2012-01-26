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
#include "GUISettings.h"

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

#ifdef HAS_GLES
Shaders::CGLSLShaderProgram* g_TextureShaderProgram = NULL;
Shaders::CGLSLShaderProgram* g_ColorShaderProgram = NULL;
Shaders::CGLSLShaderProgram* g_AlphaShaderProgram = NULL;
#endif

NativeApplicationWindow::NativeApplicationWindow(BX_Handle hApp, BX_WindowHandle win) : CGUIWindow(CAppManager::GetInstance().AcquireWindowID(),"")
{
  m_bPlayingVideo = false;
  m_hApp = hApp;
  m_handle = win;
  m_fb = BXSurfaceCreate(hApp, BX_PF_BGRA8888, 1280, 720);
  m_flipEvent = CreateEvent(NULL, false, false, NULL);
  g_windowManager.Add(this);

#ifdef HAS_GLES
  NativeAppRenderToScreenHelper::LoadShaders();
#endif
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

#ifdef HAS_INTEL_SMD

  gdl_flip(GDL_PLANE_ID_UPP_B, GDL_SURFACE_INVALID, GDL_FLIP_ASYNC);
  gdl_flip(GDL_PLANE_ID_UPP_C, GDL_SURFACE_INVALID, GDL_FLIP_ASYNC);
  gdl_flip(GDL_PLANE_ID_UPP_D, GDL_SURFACE_INVALID, GDL_FLIP_ASYNC);

  g_graphicsContext.Clear();

  int m_top = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan.top;
  int m_bottom = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan.bottom;
  int m_left = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan.left;
  int m_right = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan.right;

  glDisable(GL_TEXTURE_2D);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  CRect rect(m_left, m_top, m_right, m_bottom);
  g_graphicsContext.PushTransform(TransformMatrix(), true);
  CGUITexture::DrawQuad(rect, 0x00000000);
  g_graphicsContext.PopTransform();

  ::SetEvent(m_flipEvent);

  glEnable(GL_TEXTURE_2D);
#endif

}

void NativeApplicationWindow::Render()
{
  BOXEE::NativeApplication *app = NULL;
  if (m_hApp)
    app = (BOXEE::NativeApplication *)m_hApp->boxeeData;

  if (app)
  {
    app->OnDisplayRender(m_handle);
  }

  g_graphicsContext.CaptureStateBlock();

  RESOLUTION iRes = g_graphicsContext.GetVideoResolution();
  int screenX1 = g_settings.m_ResInfo[iRes].Overscan.left;
  int screenY1 = g_settings.m_ResInfo[iRes].Overscan.top;
  int screenX2 = g_settings.m_ResInfo[iRes].Overscan.right;
  int screenY2 = g_settings.m_ResInfo[iRes].Overscan.bottom;

  if (g_application.IsPlayingVideo())
  {
    g_graphicsContext.PushTransform(TransformMatrix(), true);
    g_graphicsContext.SetViewWindow(screenX1, screenY1, screenX2, screenY2);
    if (!m_bPlayingVideo)
    {
      m_bPlayingVideo = true;
#ifndef HAS_INTEL_SMD
      ClearSurfaceJob j(m_fb,0.0);
      j.DoWork();
#endif
    }

    g_graphicsContext.Clear();

    color_t alpha = g_graphicsContext.MergeAlpha(0xFF000000) >> 24;
    g_renderManager.RenderUpdate(false, 0, alpha);
    g_graphicsContext.PopTransform();
  }
  else
    m_bPlayingVideo = false;

#ifdef HAS_INTEL_SMD
  g_graphicsContext.SetFullScreenVideo(false);
#else
  g_graphicsContext.SetFullScreenVideo(m_bPlayingVideo);
#endif
  
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
  if (action.id >= KEY_VKEY && action.id < KEY_ASCII)
  {
    BYTE b = action.id & 0xFF;
    if (b == 0x8)
    {
      code = 8;
    }
  }
 
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
    app->OnMouseClick(m_handle, (int) point.x, (int) point.y);
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
