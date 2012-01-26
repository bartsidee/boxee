/*
* Copyright (c) 2008, Boxee.tv, ltd
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice,
*       this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice,
*       this list of conditions and the following disclaimer
*       in the documentation and/or other materials provided with the distribution.
*     * Neither the name of Boxee.tv, ltd nor the names of its contributors may be used to endorse
*       or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Boxee.tv, ltd ''AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Boxee.tv, ltd BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __FLASH_LISTENER_H__
#define __FLASH_LISTENER_H__

#include "../../lib/libjson/include/json/json.h"

#include <string>

typedef enum { FLASHLIB_MODE_NORMAL = 0, 
               FLASHLIB_MODE_BROWSER 
             } FlashLibMode;

class IFlashPlayerListener
{
public:
  virtual ~IFlashPlayerListener() { }
  virtual void FlashNewFrame() { }

  virtual void FlashPlaybackEnded() = 0;
  virtual void FlashProcessCommand(const char *command) = 0;
};

#endif

