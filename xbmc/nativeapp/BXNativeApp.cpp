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

#include "BXNativeApp.h"
#include "utils/GUIInfoManager.h"
#include "Application.h"
#include "AppManager.h"
#include "FileSystem/SpecialProtocol.h"
#include "GraphicContext.h"
#include "utils/log.h"
#include "utils/CriticalSection.h"
#include "Picture.h"
#include "DllImageLib.h"
#include "ApplicationMessenger.h"
#include "NativeApplication.h"
#include "NativeApplicationWindow.h"
#include "GUIWindowManager.h"
#include "GUIDialogOK2.h"
#include "GUIDialogYesNo2.h"
#include "LocalizeStrings.h"

#ifdef _LINUX
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <resolv.h>
#include <net/if_arp.h>
#endif

#ifdef __APPLE__
#include <ifaddrs.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#endif

#if defined(HAS_DX)
#include "NativeApplicationRenderHelpersDX.h"
#else
#include "NativeApplicationRenderHelpersGL.h"
#endif

#include "PipesManager.h"
#include "FilePipe.h"

#ifdef CANMORE
#include "IntelSMDGlobals.h"
#endif


#ifndef NULL
#define NULL 0
#endif

#ifdef HAS_NATIVE_APPS

#undef NATIVE_APP_VERBOSE_DEBUG

#ifdef NATIVE_APP_VERBOSE_DEBUG
#define TRACE_APP CLog::Log
#else
#define TRACE_APP(...)
#endif

//
// jobs to create/destroy app window (from the gui thread)
//
class CreateWindowJob : public IGUIThreadTask
{
public: 
  CreateWindowJob(BX_Handle app) : hApp(app), h(NULL) { }
  virtual void DoWork()
  {
    h = new _BX_WindowHandle;
    h->hApp = hApp;
    BOXEE::NativeApplicationWindow *win = new BOXEE::NativeApplicationWindow(hApp, h);
    h->priv = win;
    g_windowManager.ActivateWindow(win->GetID());
  }  
  BX_Handle hApp;
  BX_WindowHandle h;
};

class DestroyWindowJob : public IGUIThreadTask
{
public: 
  DestroyWindowJob(BX_WindowHandle h) : hWin(h) { }
  virtual void DoWork()
  {
    if (hWin)
    {
      if (hWin->priv)
      {
        delete ((BOXEE::NativeApplicationWindow*)(hWin->priv));
      }
      delete hWin;
    }    
  }  
  BX_WindowHandle hWin;
};

class FlipWindowJob : public IGUIThreadTask
{
public: 
  FlipWindowJob(BX_WindowHandle h) : hWin(h) { }
  virtual void DoWork()
  {
    if (hWin)
    {
      ((BOXEE::NativeApplicationWindow*)(hWin->priv))->Flip();
    }    
  }  
  BX_WindowHandle hWin;
};

class TerminateAppJob : public IGUIThreadTask
{
public: 
  TerminateAppJob(BOXEE::NativeApplication *app) : m_app(app) { }
  virtual void DoWork()
  {
    m_app->OnStopApp();
    CStdString id = m_app->GetDescriptor().GetId(); // since the descriptor returns a reference to the id and 
                                                    // the next call actaully deletes the app and descriptor - we HAVE to copy the id
                                                    // to a new CStdString
    CAppManager::GetInstance().RemoveNativeApp(id);
  }  
  BOXEE::NativeApplication *m_app;
};

const char *BXGetVersionString ( BX_Handle hApp )
{
  static CStdString ver = g_infoManager.GetVersion();
  return ver.c_str();
}

const char* BXRegistryGet( BX_Handle hApp, const char* key )
{
  TRACE_APP(LOGDEBUG,"nativeapp: registry get <%s>", key);
  BOXEE::NativeApplication *app = (BOXEE::NativeApplication *)hApp->boxeeData;
  if (!app->GetRegistry().Has(key))
    return NULL;
  return app->GetRegistry().Get(key).c_str();
}

const char* BXPersistentGet( BX_Handle hApp, const char* key )
{
  TRACE_APP(LOGDEBUG,"nativeapp: persistent get <%s>", key);
  BOXEE::NativeApplication *app = (BOXEE::NativeApplication *)hApp->boxeeData;
  if (!app->GetPersistentRegistry().Has(key))
    return NULL;
  return app->GetPersistentRegistry().Get(key).c_str();
}

void BXRegistrySet( BX_Handle hApp, const char* key, const char* value )
{
  TRACE_APP(LOGDEBUG,"nativeapp: registry set <%s=%s>", key, value);
  BOXEE::NativeApplication *app = (BOXEE::NativeApplication *)hApp->boxeeData;
  app->GetRegistry().Set(key,value);
}

void BXPersistentSet( BX_Handle hApp, const char* key, const char* value )
{
  TRACE_APP(LOGDEBUG,"nativeapp: persistent set <%s=%s>", key, value);
  BOXEE::NativeApplication *app = (BOXEE::NativeApplication *)hApp->boxeeData;
  app->GetPersistentRegistry().Set(key,value);
}

void BXGetUniqueId( BX_Handle hApp, char* uid)
{
#ifndef HAS_EMBEDDED
  const char *strId = BXRegistryGet(hApp, "boxee-id");
#else
  const char *strId = BXPersistentGet(hApp, "boxee-id");
#endif
  if (strId && *strId)
  {
    strcpy(uid,strId);
    return;
  }
  
  CStdString strMac;
  CNetworkInterfacePtr net = g_application.getNetwork().GetInterfaceByName("eth0");
  if (!net.get())
    net = g_application.getNetwork().GetInterfaceByName("en0");

  if (net.get())
  {
    strMac = net->GetMacAddress();
    strMac.Replace(":","");
  }
  
  if (strMac.IsEmpty())
  {
    std::vector<CNetworkInterfacePtr> interfaces = g_application.getNetwork().GetInterfaceList();
    for (size_t i=0; i<interfaces.size(); i++)
    {
      const CStdString &name = interfaces[i]->GetName();
      strMac = interfaces[i]->GetMacAddress();
      strMac.Replace(":","");
    
      if (!name.IsEmpty() && !strMac.IsEmpty() && name.size() >= 2 && name.Left(2) != "lo")
        break;
    }
  }
  
  CStdString randPostfix;
  randPostfix.Format("%lu",(rand()%899999)+100000);
  strMac += randPostfix;
  
#ifndef HAS_EMBEDDED
  BXRegistrySet(hApp, "boxee-id", strMac.c_str());
#else
  BXPersistentSet(hApp, "boxee-id", strMac.c_str());
#endif
  strcpy(uid, strMac.c_str());
}

const char* BXGetTempDir(BX_Handle hApp)
{
  static CStdString tmpDir = _P("special://temp");
  return tmpDir.c_str();
}

void BXGetDisplayResolution (BX_Handle hApp, unsigned int *nWidth, unsigned int *nHeight)
{
  if (!nWidth || !nHeight)
    return;
  *nWidth  = g_graphicsContext.GetWidth();
  *nHeight = g_graphicsContext.GetHeight();
}

void BXGetSkinResolution (BX_Handle hApp, unsigned int *nWidth, unsigned int *nHeight)
{
  if (!nWidth || !nHeight)
    return;
  *nWidth  = 1280;
  *nHeight = 720;
}

BX_Bool BXIsInternetConnected ( BX_Handle hApp )
{
  bool bConnected = g_application.IsConnectedToInternet() && 
                    g_application.getNetwork().IsConnected() && 
                    g_application.IsConnectedToNet();
  
  return bConnected ? BX_TRUE : BX_FALSE;
}

void BXNotifyAppStopped(BX_Handle hApp)
{
  BOXEE::NativeApplication *app = (BOXEE::NativeApplication *)hApp->boxeeData;
  TerminateAppJob *j = new TerminateAppJob(app);
  g_application.getApplicationMessenger().ExecuteOnMainThread(j, false, true);
}

void BXRegistryUnset( BX_Handle hApp, const char* key )
{
  TRACE_APP(LOGDEBUG,"nativeapp: registry unset <%s>", key);
  BOXEE::NativeApplication *app = (BOXEE::NativeApplication *)hApp->boxeeData;
  app->GetRegistry().Unset(key);
}

void BXRegistryClear( BX_Handle hApp)
{
  TRACE_APP(LOGDEBUG,"nativeapp: registry clear");
  BOXEE::NativeApplication *app = (BOXEE::NativeApplication *)hApp->boxeeData;
  app->GetRegistry().Clear();
}

void BXRegistryPushBack (BX_Handle hApp, const char* key, const char* value, unsigned int nLimit)
{
  BOXEE::NativeApplication *app = (BOXEE::NativeApplication *)hApp->boxeeData;
  app->GetRegistry().PushBack(key,value,nLimit);
}

void BXRegistryPushFront (BX_Handle hApp, const char* key, const char* value, unsigned int nLimit)
{
  BOXEE::NativeApplication *app = (BOXEE::NativeApplication *)hApp->boxeeData;
  app->GetRegistry().PushFront(key,value,nLimit);
}

void BXPersistentUnset( BX_Handle hApp, const char* key )
{
  TRACE_APP(LOGDEBUG,"nativeapp: persistent unset <%s>", key);
  BOXEE::NativeApplication *app = (BOXEE::NativeApplication *)hApp->boxeeData;
  app->GetPersistentRegistry().Unset(key);
}

void BXPersistentClear( BX_Handle hApp)
{
  TRACE_APP(LOGDEBUG,"nativeapp: persistent clear");
  BOXEE::NativeApplication *app = (BOXEE::NativeApplication *)hApp->boxeeData;
  app->GetPersistentRegistry().Clear();
}

void BXPersistentPushBack (BX_Handle hApp, const char* key, const char* value, unsigned int nLimit)
{
  BOXEE::NativeApplication *app = (BOXEE::NativeApplication *)hApp->boxeeData;
  app->GetPersistentRegistry().PushBack(key,value,nLimit);
}

void BXPersistentPushFront (BX_Handle hApp, const char* key, const char* value, unsigned int nLimit)
{
  BOXEE::NativeApplication *app = (BOXEE::NativeApplication *)hApp->boxeeData;
  app->GetPersistentRegistry().PushFront(key,value,nLimit);
}

void BXLogDebug( BX_Handle hApp, const char *fmtString, ...)
{
  va_list args;
  va_start(args, fmtString);
  CStdString msg;
  msg.FormatV(fmtString, args);
  va_end(args);
  
  CLog::Log(LOGDEBUG,"app: [%s]: %s", hApp->appName, msg.c_str());
}

void BXLogInfo( BX_Handle hApp, const char *fmtString, ...)
{
  va_list args;
  va_start(args, fmtString);
  CStdString msg;
  msg.FormatV(fmtString, args);
  va_end(args);
  
  CLog::Log(LOGINFO,"app: [%s]: %s", hApp->appName, msg.c_str());
}

void BXLogError( BX_Handle hApp, const char *fmtString, ...)
{
  va_list args;
  va_start(args, fmtString);
  CStdString msg;
  msg.FormatV(fmtString, args);
  va_end(args);
  
  CLog::Log(LOGERROR,"app: [%s]: %s", hApp->appName, msg.c_str());
}

BX_Surface* BXSurfaceCreate( BX_Handle hApp, BX_PixelFormat pixelFormat, unsigned int nWidth, unsigned int nHeight )
{
  TRACE_APP(LOGDEBUG,"nativeapp: creating surface (%d x %d)", nWidth, nHeight);
  CreateSurfaceJob j(hApp, pixelFormat, nWidth, nHeight);
  j.DoWork();  // allocate synchroniously. no gfx calls in init.
  TRACE_APP(LOGDEBUG,"nativeapp: surface %x created. (%d x %d)", j.surface, nWidth, nHeight);
  return j.surface;
}

void BXSurfaceLock( BX_Surface* surface )
{
  TRACE_APP(LOGDEBUG,"nativeapp: locking surface %x", surface);
  if (!surface || !surface->priv)
    return;
  
  LockSurfaceJob j(surface);
  g_application.getApplicationMessenger().ExecuteOnMainThread(&j);
  
}

void BXSurfaceUnlock( BX_Surface* surface )
{
  TRACE_APP(LOGDEBUG,"nativeapp: unlocking surface %x", surface);
  if (!surface || !surface->priv)
    return;
  
  UnlockSurfaceJob *j = new UnlockSurfaceJob(surface);
  ((BOXEE::NativeApplication *)(surface->hApp->boxeeData))->PushRenderOperation(j);  
}

BX_Surface* BXSurfaceCreateFromImage( BX_Handle hApp, const char *image, unsigned int nSize )
{
  TRACE_APP(LOGDEBUG,"nativeapp: creating surface from image ");
  DllImageLib img;
  img.Load();

  ImageInfo info;
  memset(&info, 0, sizeof(info));

  if (!img.DecodeFromMemory((BYTE*)image, nSize, &info))
  {
    TRACE_APP(LOGDEBUG,"nativeapp: FAILED to decode image (size = %d)", nSize);
    return NULL;
  }
  BX_Surface *s = BXSurfaceCreate(hApp, BX_PF_BGRA8888, info.width, info.height);
  if (!s)
  {
    TRACE_APP(LOGDEBUG,"nativeapp: FAILED to create surface (%d x %d)", info.width, info.height);
    img.ReleaseImage(&info);
    return NULL;
  }
  TRACE_APP(LOGDEBUG,"nativeapp: created surface %x from image (%d x %d)", s, info.width, info.height);
  
  s->pixels = new unsigned char[s->pitch * s->h];  
  unsigned int destPitch = s->pitch;
  unsigned int srcPitch = ((info.width + 1)* 3 / 4) * 4; // bitmap row length is aligned to 4 bytes
  
  for (unsigned int y = 0; y < info.height; y++)
  {
    unsigned char *dst = s->pixels + y * destPitch;
    unsigned char *src = info.texture + (info.height - 1 - y) * srcPitch;
    unsigned char *alpha = info.alpha + (info.height - 1 - y) * info.width;
    for (unsigned int x = 0; x < info.width; x++)
    {
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = (info.alpha) ? *alpha++ : 0xff;
    }
  }
  
  img.ReleaseImage(&info);
  return s;
}

BX_Bool BXSurfaceLoadImage( BX_Surface* surface, const char *image, unsigned int nSize, unsigned nWidth, unsigned nHeight )
{
  CLog::Log(LOGERROR,"nativeapp: load image to surface - not implemented! (%d x %d)", nWidth, nHeight);

#ifndef _WIN32
#warning not implemented 
#else
#pragma WARNING( not implemented )
#endif  
  return BX_FALSE;
}

void BXSurfaceRelease (BX_Surface* surface )
{
  TRACE_APP(LOGDEBUG,"nativeapp: releasing surface %x", surface);
  FreeSurfaceJob *j = new FreeSurfaceJob(surface);
  ((BOXEE::NativeApplication *)(surface->hApp->boxeeData))->PushRenderOperation(j);
}

void BXSurfaceFillRect( BX_Surface* surface, BX_Color c, BX_Rect rect, BX_BlendMethod blend)
{
  TRACE_APP(LOGDEBUG,"nativeapp: fill rect in surface %x. (%d %d %d %d. color %d.)", surface, rect.x, rect.y, rect.w, rect.h, c);
  FillRectJob *j = new FillRectJob;
  j->surface = surface;
  j->c = c;
  j->rect = rect;
  j->blend = blend;

  ((BOXEE::NativeApplication *)(surface->hApp->boxeeData))->PushRenderOperation(j);
}

void BXSurfaceBlit( BX_Surface* sourceSurface, BX_Surface* destSurface, BX_Rect sourceRect, BX_Rect destRect, BX_BlendMethod blend, unsigned char alpha )
{
  TRACE_APP(LOGDEBUG,"nativeapp: blit source surface %x (%d %d %d %d) on dest surface %x (%d %d %d %d)", 
            sourceSurface, sourceRect.x, sourceRect.y, sourceRect.w, sourceRect.h, 
            destSurface, destRect.x, destRect.y, destRect.w, destRect.h);
  
  BlitJob *j = new BlitJob;
  j->sourceSurface = sourceSurface;
  j->destSurface = destSurface;
  j->sourceRect = sourceRect;
  j->destRect = destRect;
  j->blend = blend;
  j->color = 0xffffffff; 
  j->alpha = alpha==0?255:alpha;
  ((BOXEE::NativeApplication *)(destSurface->hApp->boxeeData))->PushRenderOperation(j);
}

void BXSurfaceBlitWithColor( BX_Surface* sourceSurface, BX_Surface* destSurface, BX_Rect sourceRect, BX_Rect destRect, BX_BlendMethod blend, BX_Color color )
{
  TRACE_APP(LOGDEBUG,"nativeapp: blit source surface %x (%d %d %d %d) on dest surface %x (%d %d %d %d) with color %d", 
            sourceSurface, sourceRect.x, sourceRect.y, sourceRect.w, sourceRect.h, 
            destSurface, destRect.x, destRect.y, destRect.w, destRect.h, color);
  
  BlitJob *j = new BlitJob;
  j->sourceSurface = sourceSurface;
  j->destSurface = destSurface;
  j->sourceRect = sourceRect;
  j->destRect = destRect;
  j->blend = blend;
  j->color = color; 
  j->alpha = 255;
  ((BOXEE::NativeApplication *)(destSurface->hApp->boxeeData))->PushRenderOperation(j);
}

BX_WindowHandle BXWindowCreate ( BX_Handle hApp )
{
  CreateWindowJob j(hApp);
  g_application.getApplicationMessenger().ExecuteOnMainThread(&j);
  TRACE_APP(LOGDEBUG,"nativeapp: created window %x", j.h);
  return j.h;
}

void BXWindowDestroy( BX_WindowHandle hWindow )
{
  TRACE_APP(LOGDEBUG,"nativeapp: destroying window %x", hWindow);
  DestroyWindowJob j(hWindow);
  g_application.getApplicationMessenger().ExecuteOnMainThread(&j);
}

BX_Surface* BXWindowGetFramebuffer( BX_WindowHandle hWindow)
{
  if (!hWindow || !hWindow->priv)
    return NULL;
  BX_Surface *fb =  ((BOXEE::NativeApplicationWindow *)(hWindow->priv))->GetFrameBuffer();
  return fb;
}

void BXWindowFlip( BX_WindowHandle hWindow )
{
  TRACE_APP(LOGDEBUG,"nativeapp: flipping window %x", hWindow); 
  FlipWindowJob j(hWindow);
  j.DoWork(); // no gui access - just needs to wait for the framebuffer to be rendered. so we dont run from main thread, rather from this one.
}

BX_PlayerHandle BXPlayerCreate (BX_Handle hApp)
{
  BOXEE::NativeApplication *app = ((BOXEE::NativeApplication *)(hApp->boxeeData));

  XFILE::CFilePipe *p = new XFILE::CFilePipe;
  p->OpenForWrite(XFILE::PipesManager::GetInstance().GetUniquePipeName());
  p->AddListener(app);
  
  BX_PlayerHandle x = new sBXPlayer;
  x->hApp = hApp;
  x->priv = p;

  app->SetPlayer(x);
  
  return x;
}

void            BXPlayerDestroy (BX_PlayerHandle hPlayer)
{
  BX_Handle hApp = (BX_Handle)hPlayer->hApp;
  BOXEE::NativeApplication *app = ((BOXEE::NativeApplication *)(hApp->boxeeData));

  app->SetPlayer(NULL);
  
  XFILE::CFilePipe *p = (XFILE::CFilePipe *)(hPlayer->priv);
  if (p)
  {
    p->Close();
    delete p;
  }
  delete hPlayer;
}

unsigned int    BXPlayerFeed  (BX_PlayerHandle hPlayer, const char *data, unsigned int nDataLen )
{
  XFILE::CFilePipe *p = (XFILE::CFilePipe *)(hPlayer->priv);
  if (g_application.m_pPlayer && nDataLen)
  {
    g_application.m_pPlayer->SetCaching(false);
  }
  if (p->Write(data, nDataLen) > 0)   
    return nDataLen;
  return 0;
}

BX_Bool         BXPlayerPlay  (BX_PlayerHandle hPlayer, const char *mimeType)
{
  ThreadMessage tMsg (TMSG_MEDIA_STOP);
  g_application.getApplicationMessenger().SendMessage(tMsg, true);
  
  XFILE::CFilePipe *p = (XFILE::CFilePipe *)(hPlayer->priv);
  CFileItem item;
  item.m_strPath = p->GetName();
  item.SetContentType(mimeType);
  item.SetProperty("DisableFullScreen", true);
  item.SetProperty("UseMovieFPS", true);
  item.SetProperty("EmulateVideoFullScreen", true);
  item.SetProperty("FastPlayerStart", true);
  item.SetProperty("SilentCaching", true);
  item.SetProperty("DisableBoxeeUI", true);
  item.SetProperty("DisablePtsCorrection", true);
#ifdef CANMORE
  item.SetProperty("MaxAudioQueueSize", 2 * 1024 * 1024);
  item.SetProperty("MaxVideoQueueSize", 5 * 1024 * 1024);
#else
  item.SetProperty("MaxAudioQueueSize", 1 * 1024 * 1024);
  item.SetProperty("MaxVideoQueueSize", 4 * 1024 * 1024);
#endif

  g_application.getApplicationMessenger().PlayFile(item);

  return BX_TRUE;
}

void            BXPlayerStop  (BX_PlayerHandle hPlayer)
{
  XFILE::CFilePipe *p = (XFILE::CFilePipe *)(hPlayer->priv);
  p->SetEof();  
  p->Close();

  ThreadMessage tMsg (TMSG_MEDIA_STOP);
  g_application.getApplicationMessenger().SendMessage(tMsg, false);
}

void            BXPlayerEof  (BX_PlayerHandle hPlayer)
{
  XFILE::CFilePipe *p = (XFILE::CFilePipe *)(hPlayer->priv);
  p->SetEof();  
}

void            BXPlayerSetVolume         ( BX_PlayerHandle hPlayer, unsigned int nVol )
{
  if (g_application.m_pPlayer)
    g_application.m_pPlayer->SetVolume(nVol);  
}

void            BXPlayerSetSpeed  ( BX_PlayerHandle hPlayer, float nSpeed )
{
  if ( (nSpeed == 0 || nSpeed == 1 ) && g_application.m_pPlayer)
    g_application.m_pPlayer->Pause();
}

float             BXPlayerGetSpeed  ( BX_PlayerHandle hPlayer)
{
  bool bPaused = false;
  if (g_application.m_pPlayer)
    bPaused = g_application.m_pPlayer->IsPaused();
  return (bPaused?0.0:1.0);
}

void            BXPlayerSetPos  ( BX_PlayerHandle hPlayer, unsigned int nSecond )
{
  BX_Handle hApp = (BX_Handle)hPlayer->hApp;
  BOXEE::NativeApplication *app = ((BOXEE::NativeApplication *)(hApp->boxeeData));
  app->SetPos(nSecond);
}

unsigned int    BXPlayerGetPos  ( BX_PlayerHandle hPlayer)
{
  BX_Handle hApp = (BX_Handle)hPlayer->hApp;
  BOXEE::NativeApplication *app = ((BOXEE::NativeApplication *)(hApp->boxeeData));
  
  XFILE::CFilePipe *p = (XFILE::CFilePipe *)(hPlayer->priv);
  if (p && p->IsClosed())
  {
    app->OnPlaybackEOF();
  }

  if (p && p->IsEof() && p->IsEmpty() && !g_application.IsPlaying())
    app->OnPlaybackEOF();

  if (g_application.m_pPlayer)
  {
#ifdef CANMORE
    ismd_pts_t audioTime = g_IntelSMDGlobals.GetAudioCurrentTime();
    ismd_pts_t audioPauseTime = g_IntelSMDGlobals.GetAudioPauseCurrentTime();

    //ismd_time_t pauseCurrentTime = g_IntelSMDGlobals.GetPauseCurrentTime();

    if (audioTime == ISMD_NO_PTS)
      return g_application.m_pPlayer->GetStartTime();

    if (g_application.IsPaused())
      audioTime = audioPauseTime;

    int nAudioTimeMs = (int)DVD_TIME_TO_MSEC(g_IntelSMDGlobals.IsmdToDvdPts(audioTime));
    return nAudioTimeMs + g_application.m_pPlayer->GetStartTime() ;
#endif
    return g_application.m_pPlayer->GetTime() + g_application.m_pPlayer->GetStartTime();
  }
  return 0;
}

BX_MessageBoxResult BXMessageBox (const char *title, const char *text, BX_MessageBoxType type)
{
  if (type == MBT_OK)
  {
    CGUIDialogOK2::ShowAndGetInput(title, text);
    return MBR_OK;
  }
  else if (type == MBT_OK_CANCEL)
  {
    bool cancelled = true;
    bool result = CGUIDialogYesNo2::ShowAndGetInput(title, text, g_localizeStrings.Get(222), g_localizeStrings.Get(186), cancelled);
    if (cancelled || !result)
    {
      return MBR_CANCEL;
    }
    else
    {
      return MBR_OK;
    }
  }
  else if (type == MBT_YES_NO)
  {
    bool cancelled = true;
    bool result = CGUIDialogYesNo2::ShowAndGetInput(title, text, cancelled);

    if (cancelled || !result)
    {
      return MBR_NO;
    }
    else
    {
      return MBR_YES;
    }
  }

  return MBR_CANCEL;
}

#endif
