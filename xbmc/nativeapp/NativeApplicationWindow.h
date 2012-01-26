#ifndef __NATIVE_APP_WINDOW__
#define __NATIVE_APP_WINDOW__

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

#include "GUIWindow.h"
#include "BXNativeApp.h"
#include "utils/CriticalSection.h"
#include "utils/Event.h"

namespace BOXEE
{
  class NativeApplicationWindow : public CGUIWindow
  {
    public:
      NativeApplicationWindow(BX_Handle hApp, BX_WindowHandle win);
      virtual ~NativeApplicationWindow();

      virtual void Render();
      virtual bool OnMessage(CGUIMessage& message);
      virtual bool OnAction(const CAction &action);
      virtual bool OnMouseClick(int button, const CPoint &point);

      BX_Surface *GetFrameBuffer();
      void Flip();
    
      virtual void OnInitWindow();
      virtual bool WantsAllInput() const { return true; }

    protected:
      BX_Handle   m_hApp;
      BX_Surface* m_fb;
      BX_WindowHandle  m_handle;
      HANDLE m_flipEvent;
      bool             m_bPlayingVideo;
    
      CCriticalSection m_lock;
  };
}

#endif

