
#ifndef _DLL_NATIVE_APP__H__
#define _DLL_NATIVE_APP__H__

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

#include "DynamicDll.h"
#include "BXNativeApp.h"

class DllNativeAppInterface
{
public:
  virtual BX_Bool BX_App_Initialize ( BX_Handle hApp, const BX_Callbacks *appCallbacks, BX_App_Methods *appMethods ) = 0;
  
  DllNativeAppInterface() {}
  virtual ~DllNativeAppInterface() {}
};

class DllNativeApp : public DllDynamic, DllNativeAppInterface
{
  DEFINE_METHOD3(BX_Bool, BX_App_Initialize, (BX_Handle p1, const BX_Callbacks *p2, BX_App_Methods *p3))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(BX_App_Initialize)
  END_METHOD_RESOLVE()
};

#endif
