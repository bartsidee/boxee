#ifndef __LINUX_INPUT__H_
#define __LINUX_INPUT__H_

#include "../system.h"
#include "../StdString.h" 

#define LINUX_INPUT_EVENT_WHEEL  8
#define LINUX_INPUT_EVENT_HWHEEL 6
#define LINUX_INPUT_SCROLL_SENSITIVITY 100

class CLinuxInput
{
public:
   CLinuxInput();
   virtual ~CLinuxInput();
   bool Initialize();
   void Reset();
   void Update();
   WORD GetButton();
   bool IsHolding();

   void SetDeviceName(const CStdString &strName);

private:
   int   m_fd;
   bool  m_isHolding;
   WORD  m_button;
   bool  m_bInitialized;
   bool  m_skipHold;
   bool  m_bNeedLongClick;
   uint32_t m_firstClickTime;
   uint32_t m_lastScrollTime;
   int m_prevEventCode;
   int m_prevEventValue;
   CStdString m_strDevice;
};

#endif    

