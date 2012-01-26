
#ifndef _DLL_FLASH_LIB_H__
#define _DLL_FLASH_LIB_H__

#include "DynamicDll.h"
#include "FlashLib.h"
#include "FlashPlayerListener.h"

class DllFlashLibInterface
{
public:
  virtual ~DllFlashLibInterface() {}
  virtual FW_HANDLE FlashCreate(int nDestPitch) =0;
  virtual bool      FlashOpen(FW_HANDLE handle, int nArgc, const char **argn, const char **argv)=0;
  virtual void      FlashClose (FW_HANDLE handle)=0;

  virtual void      FlashUpdate(FW_HANDLE handle, int usecs_timeout)=0; 
  virtual void      FlashUpdatePlayerCount(FW_HANDLE handle, bool add)=0;

  virtual void      FlashLockImage(FW_HANDLE handle)=0;
  virtual void      FlashUnlockImage(FW_HANDLE handle)=0;
  virtual void      *FlashGetImage(FW_HANDLE handle)=0;

  virtual int       FlashGetWidth(FW_HANDLE handle)=0;
  virtual int       FlashGetHeight(FW_HANDLE handle)=0;
  virtual void      FlashSetWidth(FW_HANDLE handle, int nWidth) =0;
  virtual void      FlashSetHeight(FW_HANDLE handle, int nHeight) =0;

  virtual int       FlashGetPitch(FW_HANDLE handle)=0;
  virtual int       FlashGetDepth(FW_HANDLE handle)=0;
  virtual void      FlashSetDestPitch(FW_HANDLE handle, int nDestPitch)=0;
  virtual void      FlashSetWorkingPath(FW_HANDLE handle, const char *strPath)=0;
  virtual void      FlashSendKeyStroke(FW_HANDLE handle, int key)=0;
  virtual void      FlashSendMouseClick(FW_HANDLE handle, int x, int y)=0;
  virtual void      FlashSendMouseMove(FW_HANDLE handle, int x, int y)=0;
  virtual void      FlashScroll(FW_HANDLE handle, int x, int y) = 0;
  virtual void      FlashSetCrop(FW_HANDLE handle, int nTop, int nBottom, int nLeft, int nRight)=0;
  virtual void      FlashSetDestRect(FW_HANDLE handle, int x, int y, int w, int h)=0;
  virtual void      FlashSetCallback(FW_HANDLE handle, IFlashPlayerListener *listener) = 0;

  virtual void      FlashBigStep(FW_HANDLE handle, bool bBack) = 0;
  virtual void      FlashSmallStep(FW_HANDLE handle, bool bBack) = 0;
  virtual void      FlashPause(FW_HANDLE handle) = 0;
  virtual void      FlashPlay(FW_HANDLE handle) = 0;
  virtual bool      FlashIsPaused(FW_HANDLE handle) = 0;
  virtual bool      FlashIsDone(FW_HANDLE handle) = 0;
  virtual void      FlashSetVolume(FW_HANDLE handle, int nLevel) = 0; // 0-100	
  virtual void      FlashActivateExt(FW_HANDLE handle, int nID) = 0;
  virtual void      FlashUserText(FW_HANDLE handle, const char *text, const char *callback, bool bConfirmed) = 0;
  virtual void      FlashOpenDialog(FW_HANDLE handle, const char *callback, bool bConfirmed) = 0;
  virtual void      FlashJsonCommand(FW_HANDLE handle, const char *cmd) = 0;
};

class DllFlashLib : public DllDynamic, DllFlashLibInterface
{
#ifdef CANMORE
  DECLARE_DLL_WRAPPER(DllFlashLib, "special://xbmc/system/players/flashplayer/libFlashLib-i686-cm-linux.so")
#elif defined (__APPLE__)
  #ifdef __LP64__
    DECLARE_DLL_WRAPPER(DllFlashLib, "special://xbmc/system/players/flashplayer/libFlashLib-x86_64-osx.dylib")
  #else
    DECLARE_DLL_WRAPPER(DllFlashLib, "special://xbmc/system/players/flashplayer/libFlashLib-x86-osx.dylib")
  #endif
#else
  DECLARE_DLL_WRAPPER(DllFlashLib, DLL_PATH_LIBFLASH)
#endif
  DEFINE_METHOD1(FW_HANDLE, FlashCreate, (int p1))
  DEFINE_METHOD4(bool, FlashOpen, (FW_HANDLE p1, int p2, const char **p3, const char **p4))
  DEFINE_METHOD1(void, FlashClose, (FW_HANDLE p1))
  DEFINE_METHOD2(void, FlashUpdate,(FW_HANDLE p1, int p2))
  DEFINE_METHOD2(void, FlashUpdatePlayerCount,(FW_HANDLE p1, bool p2))
  DEFINE_METHOD1(void, FlashLockImage,(FW_HANDLE p1))
  DEFINE_METHOD1(void, FlashUnlockImage,(FW_HANDLE p1))
  DEFINE_METHOD1(void*, FlashGetImage,(FW_HANDLE p1))
  DEFINE_METHOD1(int,  FlashGetWidth,(FW_HANDLE p1))
  DEFINE_METHOD1(int,  FlashGetHeight,(FW_HANDLE p1))
  DEFINE_METHOD2(void,  FlashSetWidth,(FW_HANDLE p1, int p2))
  DEFINE_METHOD2(void,  FlashSetHeight,(FW_HANDLE p1, int p2))
  DEFINE_METHOD1(int,  FlashGetPitch,(FW_HANDLE p1))
  DEFINE_METHOD1(int,  FlashGetDepth,(FW_HANDLE p1))
  DEFINE_METHOD2(void,  FlashSetDestPitch,(FW_HANDLE p1, int p2))
  DEFINE_METHOD2(void,  FlashSetWorkingPath,(FW_HANDLE p1, const char *p2))
  DEFINE_METHOD2(void,  FlashSendKeyStroke,(FW_HANDLE p1, int p2))
  DEFINE_METHOD3(void,  FlashSendMouseClick,(FW_HANDLE p1, int p2, int p3))
  DEFINE_METHOD3(void,  FlashSendMouseMove,(FW_HANDLE p1, int p2, int p3))
  DEFINE_METHOD3(void,  FlashScroll,(FW_HANDLE p1, int p2, int p3))
  DEFINE_METHOD5(void,  FlashSetCrop,(FW_HANDLE p1, int p2, int p3, int p4, int p5))
  DEFINE_METHOD5(void,  FlashSetDestRect,(FW_HANDLE p1, int p2, int p3, int p4, int p5))
  DEFINE_METHOD2(void,  FlashSetCallback,(FW_HANDLE p1, IFlashPlayerListener *p2))
  DEFINE_METHOD2(void,  FlashBigStep,(FW_HANDLE p1, bool p2))
  DEFINE_METHOD2(void,  FlashSmallStep,(FW_HANDLE p1, bool p2))
  DEFINE_METHOD1(void,  FlashPause,(FW_HANDLE p1))
  DEFINE_METHOD1(void,  FlashPlay,(FW_HANDLE p1))
  DEFINE_METHOD1(bool,  FlashIsPaused,(FW_HANDLE p1))
  DEFINE_METHOD1(bool,  FlashIsDone,(FW_HANDLE p1))
  DEFINE_METHOD2(void,  FlashSetVolume,(FW_HANDLE p1, int p2))
  DEFINE_METHOD2(void,  FlashActivateExt,(FW_HANDLE p1, int p2))
  DEFINE_METHOD4(void,  FlashUserText,(FW_HANDLE p1, const char *p2, const char *p3, bool p4))
  DEFINE_METHOD3(void,  FlashOpenDialog,(FW_HANDLE p1, const char *p2, bool p3))
  DEFINE_METHOD2(void,  FlashJsonCommand,(FW_HANDLE p1, const char *p2))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(FlashCreate)
    RESOLVE_METHOD(FlashOpen)
    RESOLVE_METHOD(FlashClose)
    RESOLVE_METHOD(FlashUpdate)
    RESOLVE_METHOD(FlashUpdatePlayerCount)
    RESOLVE_METHOD(FlashLockImage)
    RESOLVE_METHOD(FlashUnlockImage)
    RESOLVE_METHOD(FlashGetImage)
    RESOLVE_METHOD(FlashGetWidth)
    RESOLVE_METHOD(FlashGetHeight)
    RESOLVE_METHOD(FlashSetWidth)
    RESOLVE_METHOD(FlashSetHeight)
    RESOLVE_METHOD(FlashGetPitch)
    RESOLVE_METHOD(FlashGetDepth)
    RESOLVE_METHOD(FlashSetDestPitch)
    RESOLVE_METHOD(FlashSetWorkingPath)
    RESOLVE_METHOD(FlashSendKeyStroke)
    RESOLVE_METHOD(FlashSendMouseClick)
    RESOLVE_METHOD(FlashSendMouseMove)
    RESOLVE_METHOD(FlashScroll)
    RESOLVE_METHOD(FlashSetCrop)
    RESOLVE_METHOD(FlashSetDestRect)
    RESOLVE_METHOD(FlashSetCallback)
    RESOLVE_METHOD(FlashBigStep)
    RESOLVE_METHOD(FlashSmallStep)
    RESOLVE_METHOD(FlashPause)
    RESOLVE_METHOD(FlashPlay)
    RESOLVE_METHOD(FlashIsPaused)
    RESOLVE_METHOD(FlashIsDone)
    RESOLVE_METHOD(FlashSetVolume)
    RESOLVE_METHOD(FlashActivateExt)
    RESOLVE_METHOD(FlashUserText)
    RESOLVE_METHOD(FlashOpenDialog)
    RESOLVE_METHOD(FlashJsonCommand)
  END_METHOD_RESOLVE()
};

#endif
