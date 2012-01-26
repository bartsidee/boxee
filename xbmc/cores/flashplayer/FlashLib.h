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

#ifndef __FLASH_LIB_H__
#define __FLASH_LIB_H__

#include "FlashPlayerListener.h"

#define FW_HANDLE void*

#if defined (_WINDOWS) || defined(_WIN32)
  #ifdef FLASH_LIB_DLL
    #define FLASHLIB_API __declspec(dllexport)
  #else
    #define FLASHLIB_API __declspec(dllimport)
  #endif
#else
  #define FLASHLIB_API 
#endif

extern "C"
{
  FLASHLIB_API FW_HANDLE FlashCreate(int nDestPitch);
  FLASHLIB_API bool      FlashOpen(FW_HANDLE handle, int nArgc, const char **argn, const char **argv);
  FLASHLIB_API void      FlashClose (FW_HANDLE handle);

  // called periodically to activate the internal engine
  FLASHLIB_API void      FlashUpdate(FW_HANDLE handle, int usecs_timeout); 
  FLASHLIB_API void      FlashUpdatePlayerCount(FW_HANDLE handle, bool add);

  FLASHLIB_API void      FlashLockImage(FW_HANDLE handle);
  FLASHLIB_API void      FlashUnlockImage(FW_HANDLE handle);
  FLASHLIB_API void      *FlashGetImage(FW_HANDLE handle);

  FLASHLIB_API int       FlashGetHeight(FW_HANDLE handle);
  FLASHLIB_API int       FlashGetWidth(FW_HANDLE handle);
  FLASHLIB_API void      FlashSetHeight(FW_HANDLE handle, int nHeight);
  FLASHLIB_API void      FlashSetWidth(FW_HANDLE handle, int nWidth);
  FLASHLIB_API int       FlashGetPitch(FW_HANDLE handle);
  FLASHLIB_API int       FlashGetDepth(FW_HANDLE handle);

  FLASHLIB_API void      FlashSetDestPitch(FW_HANDLE handle, int nDestPitch);
  FLASHLIB_API void      FlashSetWorkingPath(FW_HANDLE handle, const char *strPath);

  FLASHLIB_API void      FlashSendKeyStroke(FW_HANDLE handle, int key);
  FLASHLIB_API void      FlashSendSelect(FW_HANDLE handle);
  FLASHLIB_API void      FlashSendMouseClick(FW_HANDLE handle, int x, int y);
  FLASHLIB_API void      FlashSendMouseMove(FW_HANDLE handle, int x, int y);
  FLASHLIB_API void      FlashScroll(FW_HANDLE handle, int x, int y);

  FLASHLIB_API void      FlashSetCrop(FW_HANDLE handle, int nTop, int nBottom, int nLeft, int nRight);
  FLASHLIB_API void      FlashSetDestRect(FW_HANDLE handle, int x, int y, int w, int h);
  
  FLASHLIB_API void      FlashSetCallback(FW_HANDLE handle, IFlashPlayerListener *listener);

  FLASHLIB_API void      FlashBigStep(FW_HANDLE handle, bool bBack);
  FLASHLIB_API void      FlashSmallStep(FW_HANDLE handle, bool bBack);
  FLASHLIB_API void      FlashPause(FW_HANDLE handle);
  FLASHLIB_API void      FlashPlay(FW_HANDLE handle);
  FLASHLIB_API bool      FlashIsPaused(FW_HANDLE handle);
  FLASHLIB_API bool      FlashIsDone(FW_HANDLE handle);
  FLASHLIB_API void      FlashSetVolume(FW_HANDLE handle, int nLevel); // 0-100	

  FLASHLIB_API void      FlashActivateExt(FW_HANDLE handle, int nID); 
  FLASHLIB_API void      FlashUserText(FW_HANDLE handle, const char *text, const char *callback, bool bConfirmed);
  FLASHLIB_API void      FlashOpenDialog(FW_HANDLE handle, const char *callback, bool bConfirmed);

  FLASHLIB_API void      FlashJsonCommand(FW_HANDLE handle, const char *cmd);
}

#endif

