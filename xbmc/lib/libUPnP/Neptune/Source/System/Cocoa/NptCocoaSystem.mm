/*****************************************************************
|
|      Neptune - System Support: Cocoa Implementation
|
|      (c) 2002-2006 Gilles Boccon-Gibod
|      Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#if !defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE
#include <Cocoa/Cocoa.h>
#else
#include <UIKit/UIKit.h> 
#endif
#import <SystemConfiguration/SystemConfiguration.h>

#include "NptConfig.h"
#include "NptSystem.h"

/*----------------------------------------------------------------------
|   NPT_System::GetMachineName
+---------------------------------------------------------------------*/
NPT_Result
NPT_System::GetMachineName(NPT_String& name)
{
    // we need a pool because of UTF8String
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    CFStringRef _name = SCDynamicStoreCopyComputerName(NULL, NULL);
    name = [(NSString *)_name UTF8String];
    [(NSString *)_name release];
    [pool release];
    return NPT_SUCCESS;
}
