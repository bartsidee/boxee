/*****************************************************************
|
|   Neptune - Debug Support: Win32 Implementation
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include <stdio.h>
#if defined(_XBOX)
#include <xtl.h>
#else
#include <windows.h>
#endif

#include "NptConfig.h"
#include "NptDefs.h"
#include "NptTypes.h"
#include "NptDebug.h"
#include "NptLogging.h"
#include "NptUtils.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
NPT_SET_LOCAL_LOGGER("neptune.debug.win32")

/*----------------------------------------------------------------------
|   NPT_DebugOutput
+---------------------------------------------------------------------*/
void
NPT_DebugOutput(const char* message)
{
#if !defined(_WIN32_WCE)
    OutputDebugString(message);
#endif

//#if defined(NPT_CONFIG_ENABLE_LOGGING)
//    // remove trailing \n from message
//    if (NPT_StringLength(message) > 0 && message[NPT_StringLength(message)-1] == '\n') 
//        ((char*)message)[NPT_StringLength(message)-1] = '\0';
//    
//    NPT_LOG_FINER_1("%s", message);
//#else
    printf("%s", message);
//#endif
}

