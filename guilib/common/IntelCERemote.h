#ifndef INTELCE_REMOTE_H
#define INTELCE_REMOTE_H

#include "../system.h"

#ifdef HAS_INTELCE

#include "StdString.h"
#include "SingleLock.h"

/* The code below is a hack so we don't get compilation errors because of weird definition in tdef.h */
#define _NALTYPES_H_
typedef char               INT8;
typedef unsigned char      UINT8;
typedef short              INT16;
typedef unsigned short     UINT16;
typedef int                INT32;
typedef unsigned int       UINT32;
typedef long long          INT64;
typedef unsigned long long UINT64;
typedef int                INTN;
typedef unsigned int       UINTN;
typedef INT8               CHAR;
#include <pic24/LR_PICInterface.h>

class CRemoteControl
{
public:
  CRemoteControl();
  ~CRemoteControl();
  void Initialize();
  void Disconnect();
  void Reset();
  void Update();
  WORD GetButton();
  bool IsHolding();
  void setDeviceName(const CStdString& value);
  void setUsed(bool value);
  static const char* TranslateToButtonName(int device, int number);
  void SetEvent(WORD button, bool isHolding);

private:
  CCriticalSection m_lock;
  bool m_newData;
  bool m_isHolding;
  WORD m_button;
  bool m_initialized;
  LR_PICInterface* m_pInterface;
};

extern CRemoteControl g_RemoteControl;

#endif

#endif
