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

#include "Application.h"
#include "NativeApplication.h"
#include "utils/SingleLock.h"
#include "Util.h"
#include "utils/Base64.h"
#include "FileSystem/File.h"
#include "FileSystem/SpecialProtocol.h"
#include "KeyboardStat.h"
#include "GUIDialogProgress.h"
#include "GUIWindowManager.h"
#include "AppManager.h"
#include "GUISettings.h"
#include "FrameBufferObject.h"

#ifdef CANMORE
#include "IntelSMDGlobals.h"
#endif

#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/bio.h>

CFrameBufferObject g_fbo;

const char *BXGetVersionString ( BX_Handle hApp );
void BXGetUniqueId( BX_Handle hApp, char* uid);
const char* BXGetTempDir(BX_Handle hApp);
void BXGetDisplayResolution (BX_Handle hApp, unsigned int *nWidth, unsigned int *nHeight);
void BXGetSkinResolution (BX_Handle hApp, unsigned int *nWidth, unsigned int *nHeight);  
BX_Bool BXIsInternetConnected ( BX_Handle hApp );
void BXNotifyAppStopped(BX_Handle hApp);
void BXRegistryLoad();
void BXRegistrySave();
const char* BXRegistryGet( BX_Handle hApp, const char* key );
void BXRegistrySet( BX_Handle hApp, const char* key, const char* value );
void BXRegistryUnset( BX_Handle hApp, const char* key );
void BXRegistryClear( BX_Handle hApp);
void BXRegistryPushBack (BX_Handle hApp, const char* key, const char* value, unsigned int nLimit);
void BXRegistryPushFront (BX_Handle hApp, const char* key, const char* value, unsigned int nLimit);
void BXLogDebug( BX_Handle hApp, const char *fmtString, ...);
void BXLogInfo( BX_Handle hApp, const char *fmtString, ...);
void BXLogError( BX_Handle hApp, const char *fmtString, ...);
BX_Surface* BXSurfaceCreate( BX_Handle hApp, BX_PixelFormat pixelFormat, unsigned int nWidth, unsigned int nHeight );
BX_Surface* BXSurfaceCreateFromImage( BX_Handle hApp, const char *image, unsigned int nSize );
BX_Bool BXSurfaceLoadImage( BX_Surface* surface, const char *image, unsigned int nSize, unsigned nWidth, unsigned nHeight );
void BXSurfaceRelease (BX_Surface* surface );
void BXSurfaceLock( BX_Surface* surface );
void BXSurfaceUnlock( BX_Surface* surface );
void BXSurfaceFillRect( BX_Surface* surface, BX_Color c, BX_Rect rect, BX_BlendMethod blend);
void BXSurfaceBlit( BX_Surface* sourceSurface, BX_Surface* destSurface, BX_Rect sourceRect, BX_Rect destRect, BX_BlendMethod blend, unsigned char alpha );
void BXSurfaceBlitWithColor( BX_Surface* sourceSurface, BX_Surface* destSurface, BX_Rect sourceRect, BX_Rect destRect, BX_BlendMethod blend, BX_Color color );
BX_WindowHandle BXWindowCreate ( BX_Handle hApp );
void BXWindowDestroy( BX_WindowHandle hWindow );
BX_Surface* BXWindowGetFramebuffer( BX_WindowHandle hWindow);
void BXWindowFlip( BX_WindowHandle hWindow );
BX_PlayerHandle BXPlayerCreate (BX_Handle hApp);
void            BXPlayerDestroy (BX_PlayerHandle hPlayer);
unsigned int    BXPlayerFeed  (BX_PlayerHandle hPlayer, const char *data, unsigned int nDataLen );
BX_Bool         BXPlayerPlay  (BX_PlayerHandle hPlayer, const char *mimeType);
void            BXPlayerStop  (BX_PlayerHandle hPlayer);
void            BXPlayerEof   (BX_PlayerHandle hPlayer);
void            BXPlayerSetVolume         ( BX_PlayerHandle hPlayer, unsigned int nVol );
void            BXPlayerSetSpeed  ( BX_PlayerHandle hPlayer, float nSpeed );
float           BXPlayerGetSpeed  ( BX_PlayerHandle hPlayer);
void            BXPlayerSetPos  ( BX_PlayerHandle hPlayer, unsigned int nSecond );
unsigned int    BXPlayerGetPos  ( BX_PlayerHandle hPlayer);
BX_MessageBoxResult BXMessageBox (const char *title, const char *text, BX_MessageBoxType type);
void BXPersistentLoad();
void BXPersistentSave();
const char* BXPersistentGet( BX_Handle hApp, const char* key );
void BXPersistentSet( BX_Handle hApp, const char* key, const char* value );
void BXPersistentUnset( BX_Handle hApp, const char* key );
void BXPersistentClear( BX_Handle hApp);
void BXPersistentPushBack (BX_Handle hApp, const char* key, const char* value, unsigned int nLimit);
void BXPersistentPushFront (BX_Handle hApp, const char* key, const char* value, unsigned int nLimit);
void BXGetDisplayOverscan(BX_Overscan* overscan);
void BXBoxeeRenderingEnabled(bool enabled);
BX_Mode3D BXDimensionalityGetSupportedModes(void);
void BXDimensionalitySetMode(BX_Mode3D mode);
BX_Mode3D BXDimensionalityGetMode(void);
void BXShowTip(const char *text, int duration);
void BXAudioGetOutputPortCaps(BX_AudioOutputPort* output, BX_AudioOutputCaps* caps);

using namespace BOXEE;

NativeApplication::NativeApplication()
{
  m_player = NULL;
  m_lastSetPos = 0;
  m_screenSaverState = false;
  m_playerCaching = true;
}

NativeApplication::~NativeApplication()
{
  // this is called from the main thread so we cleanup our queues
  Flip();
  ExecuteRenderOperations();

  m_dll.Unload();

#ifdef HAS_GDL
  // restore HDPC status to false
  gdl_boolean_t hdcpControlEnabled = GDL_FALSE;
  if (gdl_port_set_attr(GDL_PD_ID_HDMI, GDL_PD_ATTR_ID_HDCP,
      &hdcpControlEnabled) != GDL_SUCCESS)
  {
    CLog::Log(LOGERROR, "NativeApplication::~NativeApplication Could not disable HDCP control");
  }
#endif
}

void NativeApplication::InitializeCallbacks(BX_Callbacks &m_callbacks)
{
  memset(&m_callbacks, 0, sizeof(m_callbacks));
  m_callbacks.boxeeApiVersion = BOXEE_API_VERSION;
  m_callbacks.boxeeGetVersionString = BXGetVersionString;
  m_callbacks.boxeeGetTempDir = BXGetTempDir;
  m_callbacks.boxeeGetDisplayResolution = BXGetDisplayResolution;
  m_callbacks.boxeeGetSkinResolution = BXGetSkinResolution;
  m_callbacks.boxeeIsInternetConnected = BXIsInternetConnected;
  m_callbacks.boxeeGetUniqueId = BXGetUniqueId;
  m_callbacks.boxeeNotifyAppStopped = BXNotifyAppStopped;
  m_callbacks.logDebug = BXLogDebug;
  m_callbacks.logInfo = BXLogInfo;
  m_callbacks.logError = BXLogError;
  m_callbacks.windowCreate = BXWindowCreate;
  m_callbacks.windowDestroy = BXWindowDestroy;
  m_callbacks.windowGetFramebuffer = BXWindowGetFramebuffer;
  m_callbacks.windowFlip = BXWindowFlip;
  m_callbacks.surfaceCreate = BXSurfaceCreate;
  m_callbacks.surfaceCreateFromImage = BXSurfaceCreateFromImage;
  m_callbacks.surfaceRelease = BXSurfaceRelease;
  m_callbacks.surfaceLock = BXSurfaceLock;
  m_callbacks.surfaceUnlock = BXSurfaceUnlock;
  m_callbacks.surfaceLoadImage = BXSurfaceLoadImage;
  m_callbacks.surfaceFillRect = BXSurfaceFillRect;
  m_callbacks.surfaceBlit = BXSurfaceBlit;
  m_callbacks.surfaceBlitWithColor = BXSurfaceBlitWithColor;
  m_callbacks.playerCreate = BXPlayerCreate;
  m_callbacks.playerDestroy = BXPlayerDestroy;
  m_callbacks.playerFeed = BXPlayerFeed;
  m_callbacks.playerPlay = BXPlayerPlay;
  m_callbacks.playerStop  = BXPlayerStop;
  m_callbacks.playerEOF  = BXPlayerEof;
  m_callbacks.playerSetVolume = BXPlayerSetVolume;
  m_callbacks.playerSetSpeed = BXPlayerSetSpeed;
  m_callbacks.playerGetSpeed = BXPlayerGetSpeed;
  m_callbacks.playerSetPos = BXPlayerSetPos;
  m_callbacks.playerGetPos = BXPlayerGetPos;
  m_callbacks.registryGet = BXRegistryGet;
  m_callbacks.registrySet = BXRegistrySet;
  m_callbacks.registryUnset = BXRegistryUnset;
  m_callbacks.registryPushBack = BXRegistryPushBack;
  m_callbacks.registryPushFront  = BXRegistryPushFront;
  m_callbacks.registryClear = BXRegistryClear;
  m_callbacks.showMessage = BXMessageBox;
  m_callbacks.getDisplayOverscan = BXGetDisplayOverscan;
  m_callbacks.persistentGet = BXPersistentGet;
  m_callbacks.persistentSet = BXPersistentSet;
  m_callbacks.persistentUnset = BXPersistentUnset;
  m_callbacks.persistentPushBack = BXPersistentPushBack;
  m_callbacks.persistentPushFront  = BXPersistentPushFront;
  m_callbacks.persistentClear = BXPersistentClear;
  m_callbacks.boxeeRenderingEnabled = BXBoxeeRenderingEnabled;
  m_callbacks.dimensionalityGetSupportedModes = BXDimensionalityGetSupportedModes;
  m_callbacks.dimensionalitySetMode = BXDimensionalitySetMode;
  m_callbacks.dimensionalityGetMode = BXDimensionalityGetMode;
  m_callbacks.showTip = BXShowTip;
  m_callbacks.audioGetOutputPortCaps = BXAudioGetOutputPortCaps;
}

bool NativeApplication::Launch(const CAppDescriptor &desc, const CStdString &launchPath)
{
  /*
   when starting a native app we must:
   - Enable HDCP
   - Clear old video planes
   */
#ifdef HAS_GDL
  // Video plane might contain garbage
  gdl_plane_reset(GDL_VIDEO_PLANE);

  gdl_boolean_t hdcpControlEnabled = GDL_TRUE;
  if (gdl_port_set_attr(GDL_PD_ID_HDMI, GDL_PD_ATTR_ID_HDCP,
      &hdcpControlEnabled) != GDL_SUCCESS)
  {
    CLog::Log(LOGERROR, "NativeApplication::Launch Could not enable HDCP control");
  }
#endif

  g_graphicsContext.Clear();

  CStdString path = desc.GetLocalPath();
  m_desc = desc;
  CStdString appPath;
#ifdef __APPLE__
  CUtil::AddFileToFolder(path, "boxeeapp-x86-osx.so", appPath);
#elif defined (CANMORE)
  CUtil::AddFileToFolder(path, "boxeeapp-i686-cm-linux.so", appPath);
#elif defined (_LINUX) && defined (__x86_64__)
  CUtil::AddFileToFolder(path, "boxeeapp-x86_64-linux.so", appPath);
#elif defined (_LINUX)  && defined(__mips__) && defined (_MIPS_ARCH_MIPS32R2) && defined(MIPSEL)
  CUtil::AddFileToFolder(path, "boxeeapp-mipsisa32r2el-linux.so", appPath);
#elif defined (_LINUX) 
  CUtil::AddFileToFolder(path, "boxeeapp-i486-linux.so", appPath);
#else  
  CUtil::AddFileToFolder(path, "boxeeapp.dll", appPath);
#endif
  
  if (!CUtil::CheckFileSignature(appPath, desc.GetSignature()))
  {
    CLog::Log(LOGERROR,"application can not be verified. aborting.");
    return false;
  }

  m_dll.SetFile(appPath);
  m_dll.EnableDelayedUnload(false);

  CGUIDialogProgress *dlg = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

  if (!m_dll.Load())
  {
    CLog::Log(LOGERROR, "NativeApp::Launch: could not load app dll");
    if (dlg)
      dlg->Close();
    return false;
  }
  
  m_registry.Load(m_desc);
#ifdef HAS_EMBEDDED
  m_persistentRegistry.Load(m_desc, true);
#endif
  InitializeCallbacks(m_callbacks);
  memset(&m_appMethods, 0, sizeof(m_appMethods)); 
  memset(&m_handle, 0, sizeof(m_handle));
  m_handle.boxeeData = this;
  bool bOk = (m_dll.BX_App_Initialize(&m_handle, &m_callbacks, &m_appMethods) == BX_TRUE);
  if (bOk)
  {
    if (m_appMethods.onAppStart)
    {
      const char *params[1] = { launchPath.c_str() };
      bOk = ( m_appMethods.onAppStart(&m_handle, 1, (char **)params) == BX_TRUE);
    }
    else
    {
      bOk = false;
      if (m_appMethods.onAppStop)
        m_appMethods.onAppStop(&m_handle);
    }
  }

  if (dlg)
    dlg->Close();

  return bOk;
}

CAppRegistry &NativeApplication::GetRegistry()
{
  return m_registry;
}

CAppRegistry &NativeApplication::GetPersistentRegistry()
{
#ifdef HAS_EMBEDDED
  return m_persistentRegistry;
#else
  return m_registry;
#endif
}

CAppDescriptor &NativeApplication::GetDescriptor()
{
  return m_desc;
}

void NativeApplication::ExecuteRenderOperations()
{
  CSingleLock lock(m_lock);
  size_t nSize = m_renderOpsQueueFront.size();
  for (size_t i=0; i< nSize; i++)
  {
    m_renderOpsQueueFront[i]->DoWork();
    delete m_renderOpsQueueFront[i]; 
  }
  m_renderOpsQueueFront.clear();
}

void NativeApplication::Flip()
{
  CSingleLock lock(m_lock);
  size_t nSize = m_renderOpsQueueBack.size();
  for (size_t i=0; i< nSize; i++)
    m_renderOpsQueueFront.push_back(m_renderOpsQueueBack[i]);
  m_renderOpsQueueBack.clear();
}

void NativeApplication::PushRenderOperation(IGUIThreadTask* op)
{
  CSingleLock lock(m_lock);
  m_renderOpsQueueBack.push_back(op);
}

void NativeApplication::OnStopApp()
{
  if (m_appMethods.onAppStop)
    m_appMethods.onAppStop(&m_handle);
}

void NativeApplication::OnLeft(BX_WindowHandle win)
{
  BX_KeyEvent e = { BX_KEY_LEFT, 0 };
  if (m_appMethods.onKey)
    m_appMethods.onKey(win, e);
}

void NativeApplication::OnRight(BX_WindowHandle win)
{
  BX_KeyEvent e = { BX_KEY_RIGHT, 0 };
  if (m_appMethods.onKey)
    m_appMethods.onKey(win, e);
}

void NativeApplication::OnUp(BX_WindowHandle win)
{
  BX_KeyEvent e = { BX_KEY_UP, 0 };
  if (m_appMethods.onKey)
    m_appMethods.onKey(win, e);
}

void NativeApplication::OnDown(BX_WindowHandle win)
{
  BX_KeyEvent e = { BX_KEY_DOWN, 0 };
  if (m_appMethods.onKey)
    m_appMethods.onKey(win, e);
}

void NativeApplication::OnEnter(BX_WindowHandle win)
{
  BX_KeyEvent e = { BX_KEY_SELECT, 0 };
  if (m_appMethods.onKey)
    m_appMethods.onKey(win, e);  
}

void NativeApplication::OnBack(BX_WindowHandle win)
{
  BX_KeyEvent e = { BX_KEY_BACK, 0 };
  if (m_appMethods.onKey)
    m_appMethods.onKey(win, e);  
}

void NativeApplication::OnPlay(BX_WindowHandle win)
{
  BX_KeyEvent e = { BX_KEY_PLAY, 0 };
  if (m_appMethods.onKey)
    m_appMethods.onKey(win, e);  
}

void NativeApplication::OnPause(BX_WindowHandle win)
{
  BX_KeyEvent e = { BX_KEY_PAUSE, 0 };
  if (m_appMethods.onKey)
    m_appMethods.onKey(win, e);  
}

void NativeApplication::OnStop(BX_WindowHandle win)
{
  BX_KeyEvent e = { BX_KEY_STOP, 0 };
  if (m_appMethods.onKey)
    m_appMethods.onKey(win, e);  
}

void NativeApplication::OnSkipFw(BX_WindowHandle win)
{
  BX_KeyEvent e = { BX_KEY_SKIP_FW, 0 };
  if (m_appMethods.onKey)
    m_appMethods.onKey(win, e);  
}

void NativeApplication::OnSkipBw(BX_WindowHandle win)
{
  BX_KeyEvent e = { BX_KEY_SKIP_BW, 0 };
  if (m_appMethods.onKey)
    m_appMethods.onKey(win, e);  
}

void NativeApplication::OnFastFW(BX_WindowHandle win)
{
  BX_KeyEvent e = { BX_KEY_FAST_FW, 0 };
  if (m_appMethods.onKey)
    m_appMethods.onKey(win, e);  
}

void NativeApplication::OnRewind(BX_WindowHandle win)
{
  BX_KeyEvent e = { BX_KEY_FAST_BW, 0 };
  if (m_appMethods.onKey)
    m_appMethods.onKey(win, e);  
}

void NativeApplication::OnKey(BX_WindowHandle win, short unicode)
{
  BX_KeyEvent e = { BX_KEY_UNICODE, unicode };
  if (m_appMethods.onKey)
    m_appMethods.onKey(win, e);    
}

void NativeApplication::OnMouseClick(BX_WindowHandle win, int x, int y)
{
  if (m_appMethods.onMouseClick)
    m_appMethods.onMouseClick(win, BX_MB_LEFT, x, y);    
}

void NativeApplication::OnMouseMove(BX_WindowHandle win, int x, int y)
{
  if (m_appMethods.onMouseMove)
    m_appMethods.onMouseMove(win, x, y);    
}

void NativeApplication::OnScreensaverShown(BX_Handle hApp)
{
  if (m_appMethods.onScreensaverShown)
    m_appMethods.onScreensaverShown(hApp);
}

void NativeApplication::OnScreensaverHidden(BX_Handle hApp)
{
  if (m_appMethods.onScreensaverHidden)
    m_appMethods.onScreensaverHidden(hApp);
}

BX_PlayerState NativeApplication::GetPlayerState(BX_Handle hApp)
{
  if (m_appMethods.getPlayerState)
    return m_appMethods.getPlayerState(hApp);
  else
    return BX_PLAYERSTATE_UNKNOWN;
}

void NativeApplication::OnDisplayRender(BX_WindowHandle win)
{
  if (m_appMethods.onDisplayRender)
    m_appMethods.onDisplayRender(win);
}

void NativeApplication::SetPlayer(BX_PlayerHandle p)
{
  m_player = p;
  m_playerCaching = true;
}

void NativeApplication::OnPipeOverFlow()
{
}

void NativeApplication::OnPipeUnderFlow()
{
  // we have to check the player's internal buffer as well and not report underrun if it still has data to work with
  if (m_appMethods.onPlaybackUnderrun && g_application.m_pPlayer && !m_playerCaching && g_application.m_pPlayer->GetCacheLevel() == 0
      && g_application.m_pPlayer->GetTime() > 1)
  {
     m_appMethods.onPlaybackUnderrun(m_player);
  }

  m_playerCaching = g_application.m_pPlayer->IsCaching();
}

void NativeApplication::OnPlaybackEOF()
{
  if (m_appMethods.onPlaybackEnded)
    m_appMethods.onPlaybackEnded(m_player);      
}

void NativeApplication::SetPos(unsigned int pos)
{
  m_lastSetPos = pos;
}

unsigned int NativeApplication::GetPos()
{
  return m_lastSetPos;
}

void NativeApplication::SetScreensaverState()
{
  BX_PlayerState playerState = GetPlayerState(&m_handle);
  if (playerState != BX_PLAYERSTATE_UNKNOWN && playerState != BX_PLAYERSTATE_STOPPED)
  {
    g_application.ResetScreenSaver();
  }

  bool currentScreensaverState = g_application.IsInScreenSaver();
  if (currentScreensaverState != m_screenSaverState)
  {
    m_screenSaverState = currentScreensaverState;

    if (currentScreensaverState)
    {
      OnScreensaverShown(&m_handle);
    }
    else
    {
      OnScreensaverHidden(&m_handle);
    }
  }
}

void BXGetDisplayOverscan(BX_Overscan* overscan)
{
  RESOLUTION res = g_graphicsContext.GetVideoResolution();

  if (!overscan)
    return;

  overscan->left = g_settings.m_ResInfo[res].Overscan.left;
  overscan->right = g_settings.m_ResInfo[res].Overscan.right;
  overscan->top = g_settings.m_ResInfo[res].Overscan.top;
  overscan->bottom = g_settings.m_ResInfo[res].Overscan.bottom;
}

void BXBoxeeRenderingEnabled(bool enabled)
{
  g_application.SetRenderingEnabled(enabled);
}

BX_Mode3D BXDimensionalityGetSupportedModes(void)
{
  if (g_guiSettings.GetBool("videoscreen.tv3dfc"))
    return (BX_Mode3D) ((int) BX_MODE_3D_SIDE_BY_SIDE | (int) BX_MODE_3D_TOP_AND_BOTTOM);
  else
    return BX_MODE_3D_NONE;
}

void BXDimensionalitySetMode(BX_Mode3D mode)
{
  Mode3D renderMode;

  switch (mode)
  {
  case BX_MODE_3D_SIDE_BY_SIDE: renderMode = MODE_3D_SBS; break;
  case BX_MODE_3D_TOP_AND_BOTTOM: renderMode = MODE_3D_OU; break;
  default: renderMode = MODE_3D_NONE; break;
  }

  g_Windowing.SetMode3D(renderMode);
}

BX_Mode3D BXDimensionalityGetMode(void)
{
  Mode3D renderMode = g_Windowing.GetMode3D();

  switch (renderMode)
  {
  case MODE_3D_SBS: return BX_MODE_3D_SIDE_BY_SIDE;
  case MODE_3D_OU: return BX_MODE_3D_TOP_AND_BOTTOM;
  default: return BX_MODE_3D_NONE;
  }
}

void BXShowTip(const char *text, int duration)
{
  g_application.m_guiDialogKaiToast.QueueNotification("", "", text, duration * 1000);
}

void BXAudioGetOutputPortCaps(BX_AudioOutputPort* output, BX_AudioOutputCaps* caps)
{
  if (output)
  {
    int audioOutputMode = g_guiSettings.GetInt("audiooutput.mode");
    switch (audioOutputMode)
    {
    case AUDIO_ANALOG:         *output = BX_AUDIO_OUTPUT_I2S0; break;
    case AUDIO_DIGITAL_SPDIF:  *output = BX_AUDIO_OUTPUT_SPDIF; break;
    case AUDIO_DIGITAL_HDMI:   *output = BX_AUDIO_OUTPUT_HDMI; break;
#ifdef HAS_INTEL_SMD
    case AUDIO_ALL_OUTPUTS:    *output = (BX_AudioOutputPort) (BX_AUDIO_OUTPUT_I2S0 | BX_AUDIO_OUTPUT_SPDIF | BX_AUDIO_OUTPUT_HDMI); break;
#endif
    }
  }

  if (caps)
  {
    int capsInt = BX_AUDIO_CAPS_PCM;
    if (g_guiSettings.GetBool("audiooutput.lpcm71passthrough")) capsInt = BX_AUDIO_CAPS_LPCM71;
    if (g_guiSettings.GetBool("audiooutput.eac3passthrough"))   capsInt |= BX_AUDIO_CAPS_DDP;
    if (g_guiSettings.GetBool("audiooutput.ac3passthrough"))    capsInt |= BX_AUDIO_CAPS_DD;
    if (g_guiSettings.GetBool("audiooutput.truehdpassthrough")) capsInt |= BX_AUDIO_CAPS_TRUEHD;
    if (g_guiSettings.GetBool("audiooutput.dtspassthrough"))    capsInt |= BX_AUDIO_CAPS_DTS;
    if (g_guiSettings.GetBool("audiooutput.dtshdpassthrough"))  capsInt |= BX_AUDIO_CAPS_DTSMA;
    *caps = (BX_AudioOutputCaps) capsInt;
  }
}

#endif
