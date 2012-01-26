
#include "LCDFactory.h"
#include "../linux/XLCDproc.h"

ILCD* g_lcd = NULL;
CLCDFactory::CLCDFactory(void)
{}

CLCDFactory::~CLCDFactory(void)
{}

ILCD* CLCDFactory::Create()
{
#ifdef _XBOX
  switch (g_guiSettings.GetInt("lcd.modchip"))
  {
  case MODCHIP_XENIUM:
    return new CXeniumLCD();
    break;

  case MODCHIP_SMARTXX:
    return new CSmartXXLCD();
    break;

  case MODCHIP_XECUTER3:
    return new CX3LCD();
    break;
  }
  return new CSmartXXLCD();
#endif
#ifdef _LINUX
  return new XLCDproc(); 
#endif
}
