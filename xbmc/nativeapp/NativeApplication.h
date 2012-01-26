#ifndef __NATIVE__APPLICATION__
#define __NATIVE__APPLICATION__

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

#include "StdString.h"
#include "DllNativeApp.h"
#include "AppDescriptor.h"
#include "AppRegistry.h"
#include "utils/CriticalSection.h"
#include "ApplicationMessenger.h"
#include "FileSystem/PipesManager.h"

#include <vector>

namespace BOXEE
{
  class NativeApplication : public XFILE::IPipeListener
  {
    public:
      NativeApplication();
      virtual ~NativeApplication();
     
      bool Launch(const CAppDescriptor &desc, const CStdString &launchPath);

      CAppRegistry &GetRegistry();
      CAppDescriptor &GetDescriptor();
      CAppRegistry &GetPersistentRegistry();

      void OnStopApp();
      void OnLeft(BX_WindowHandle win);
      void OnRight(BX_WindowHandle win);
      void OnUp(BX_WindowHandle win);
      void OnDown(BX_WindowHandle win);
      void OnEnter(BX_WindowHandle win);
      void OnBack(BX_WindowHandle win);
      void OnPlay(BX_WindowHandle win);
      void OnPause(BX_WindowHandle win);
      void OnStop(BX_WindowHandle win);
      void OnSkipFw(BX_WindowHandle win);
      void OnSkipBw(BX_WindowHandle win);
      void OnFastFW(BX_WindowHandle win);
      void OnRewind(BX_WindowHandle win);
      void OnKey(BX_WindowHandle win, short unicode);
      void OnMouseClick(BX_WindowHandle win, int x, int y);
      void OnMouseMove(BX_WindowHandle win, int x, int y);
      void OnScreensaverShown(BX_Handle hApp);
      void OnScreensaverHidden(BX_Handle hApp);
      BX_PlayerState GetPlayerState(BX_Handle hApp);
      void OnDisplayRender(BX_WindowHandle win);

      void ExecuteRenderOperations();
      void PushRenderOperation(IGUIThreadTask* op);
      void SetScreensaverState();

      virtual void OnPipeOverFlow();
      virtual void OnPipeUnderFlow();
      
      void OnPlaybackEOF();

      void SetPlayer(BX_PlayerHandle p);
    
      void SetPos(unsigned int pos);
      unsigned int GetPos();
    
      void Flip();
    protected:
      void InitializeCallbacks(BX_Callbacks &m_callbacks);
    
      BX_Callbacks        m_callbacks;
      BX_App_Methods      m_appMethods;
      struct _BX_Handle   m_handle;
      BX_PlayerHandle     m_player;
      CAppDescriptor      m_desc;
      CAppRegistry        m_registry;
#ifdef HAS_EMBEDDED
      CAppRegistry        m_persistentRegistry;
#endif
      DllNativeApp        m_dll;
    
      unsigned int        m_lastSetPos;
      bool                m_screenSaverState;
      bool                m_playerCaching;

      std::vector<IGUIThreadTask*>  m_renderOpsQueueBack; 
      std::vector<IGUIThreadTask*>  m_renderOpsQueueFront; 
      CCriticalSection    m_lock;
  };

};

#endif

