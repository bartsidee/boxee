
#ifndef _DLL_SKIN_NATIVE_APP__H__
#define _DLL_SKIN_NATIVE_APP__H__

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

#include "app/XAPP_Native.h"
#include "DynamicDll.h"

class DllSkinNativeAppInterface
{
public:
  virtual bool xapp_initialize(XAPP::NativeApp** app) = 0;
  
  DllSkinNativeAppInterface() {}
  virtual ~DllSkinNativeAppInterface() {}
};

class DllSkinNativeApp : public DllDynamic, DllSkinNativeAppInterface
{
  DEFINE_METHOD1(bool, xapp_initialize, (XAPP::NativeApp** p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(xapp_initialize)
  END_METHOD_RESOLVE()
};

#endif
