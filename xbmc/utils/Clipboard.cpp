#include "Clipboard.h"

#ifdef _WIN32
extern HWND g_hWnd;
#endif

#ifdef __APPLE__
#include "CocoaInterface.h"
#endif

#include "utils/log.h"

bool CClipboard::Paste(CStdString& result)
{
#ifdef __APPLE__
  const char *szStr = Cocoa_Paste();
  if (szStr)
  {
   result = szStr;
   return true;
  }
#elif defined _LINUX
  FILE* file = popen("/usr/bin/xsel -o", "r");
  if (!file)
  {
    CLog::Log(LOGERROR, "%s - unable to execute xsel tool", __FUNCTION__);
    return false;
  }
   
  char line[2048]; // maximum clipboard size
  if (!fgets(line, sizeof(line) - 1, file))
  {
    pclose(file);
    return false;
  }
  
  result = line;
  pclose(file);
  
  return true;  
  
#elif defined _WIN32
  HGLOBAL   hglb;
  LPTSTR    lptstr; 
  if (OpenClipboard(g_hWnd)) 
  {
    hglb = GetClipboardData(CF_TEXT); 
    if (hglb != NULL) 
    { 
        lptstr = (LPTSTR)GlobalLock(hglb); 
        if (lptstr != NULL) 
        { 
            result = (char*)(lptstr);
            GlobalUnlock(hglb);
			CloseClipboard(); 
            return true;
        } 
    } 
    CloseClipboard(); 
  }
#endif    

  return false;
}
