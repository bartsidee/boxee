//
//  CocoaUtils.m
//  XBMC
//
//  Created by Elan Feingold on 1/5/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//
#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>
#include <CoreFoundation/CoreFoundation.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <mach/mach_port.h>
#include <mach/mach_interface.h>
#include <mach/mach_init.h>

#include <IOKit/pwr_mgt/IOPMLib.h>
#include <IOKit/IOKitLib.h>
#include "CocoaUtils.h"
#include <SDL/SDL.h>

#import "SmartCrashReportsInstall.h"

#define MAX_DISPLAYS 32
static NSWindow* blankingWindows[MAX_DISPLAYS];

void* InitializeAutoReleasePool()
{
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  return pool;
}

void DestroyAutoReleasePool(void* aPool)
{
  NSAutoreleasePool* pool = (NSAutoreleasePool* )aPool;
  [pool release];
}

void Cocoa_GL_MakeCurrentContext(void* theContext)
{
  NSOpenGLContext* context = (NSOpenGLContext* )theContext;
  [ context makeCurrentContext ];
}

void Cocoa_GL_ReleaseContext(void* theContext)
{
  NSOpenGLContext* context = (NSOpenGLContext* )theContext;
  [ NSOpenGLContext clearCurrentContext ];
  [ context clearDrawable ];
  [ context release ];
}

NSWindow* childWindow = nil;
NSWindow* mainWindow = nil;

void Cocoa_MakeChildWindow()
{
  NSOpenGLContext* context = (NSOpenGLContext*)Cocoa_GL_GetCurrentContext();
  NSView* view = [context view];
  NSWindow* window = [view window];

  // Create a child window.
  childWindow = [[NSWindow alloc] initWithContentRect:[window frame]
                                            styleMask:NSBorderlessWindowMask
                                              backing:NSBackingStoreBuffered
                                                defer:NO];
                                          
  [childWindow setContentSize:[view frame].size];
  [childWindow setBackgroundColor:[NSColor blackColor]];
  [window addChildWindow:childWindow ordered:NSWindowAbove];
  mainWindow = window;
  //childWindow.alphaValue = 0.5; 
}

void Cocoa_DestroyChildWindow()
{
  if (childWindow != nil)
  {
    [mainWindow removeChildWindow:childWindow];
    [childWindow close];
    childWindow = nil;
  }
}

void Cocoa_GL_SwapBuffers(void* theContext)
{
  [ (NSOpenGLContext*)theContext flushBuffer ];
}

int Cocoa_GetNumDisplays()
{
  return [[NSScreen screens] count];
}

int Cocoa_GetDisplay(int screen_index)
{
  CGDirectDisplayID displayArray[MAX_DISPLAYS];
  CGDisplayCount    numDisplays;

  // Get the list of displays.
  CGGetActiveDisplayList(MAX_DISPLAYS, displayArray, &numDisplays);
  return( (int)displayArray[screen_index]);
}

CGDirectDisplayID Cocoa_GetDisplayIDFromScreen(NSScreen *screen)
{
  NSDictionary* screenInfo = [screen deviceDescription];
  NSNumber* screenID = [screenInfo objectForKey:@"NSScreenNumber"];
 
  return (CGDirectDisplayID)[screenID longValue];
}

int Cocoa_GetDisplayIndex(CGDirectDisplayID display)
{
  CGDirectDisplayID displayArray[MAX_DISPLAYS];
  CGDisplayCount    numDisplays;

  // Get the list of displays.
  CGGetActiveDisplayList(MAX_DISPLAYS, displayArray, &numDisplays);
  while (numDisplays > 0)
  {
    if (display == displayArray[--numDisplays])
          return numDisplays;
  }
  return -1;
}

void Cocoa_GetScreenResolutionOfAnotherScreen(int screen, int* w, int* h)
{
  CFDictionaryRef mode = CGDisplayCurrentMode(Cocoa_GetDisplay(screen));
  CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayWidth), kCFNumberSInt32Type, w);
  CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayHeight), kCFNumberSInt32Type, h);
}

void Cocoa_GetScreenResolution(int* w, int* h)
{
  // Figure out the screen size. (default to main screen)
  CGDirectDisplayID display_id = kCGDirectMainDisplay;
  CFDictionaryRef mode  = CGDisplayCurrentMode(display_id);
  
  NSOpenGLContext* context = (NSOpenGLContext*)Cocoa_GL_GetCurrentContext();
  if (context)
  {
    NSView* view;
  
    view = [context view];
    if (view)
    {
      NSWindow* window;
      window = [view window];
      if (window)
      {
        display_id = Cocoa_GetDisplayIDFromScreen( [window screen] );      
        mode  = CGDisplayCurrentMode(display_id);
      }
    }
  }
  
  CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayWidth), kCFNumberSInt32Type, w);
  CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayHeight), kCFNumberSInt32Type, h);
}

// get a double value from a dictionary
static double getDictDouble(CFDictionaryRef refDict, CFStringRef key)
{
  double double_value;
  CFNumberRef number_value = (CFNumberRef) CFDictionaryGetValue(refDict, key);
  if (!number_value) // if can't get a number for the dictionary
    return -1;  // fail
  if (!CFNumberGetValue(number_value, kCFNumberDoubleType, &double_value)) // or if cant convert it
    return -1; // fail
  return double_value; // otherwise return the long value
}

double Cocoa_GetScreenRefreshRate(int screen)
{
  // Figure out the refresh rate.
  CFDictionaryRef mode = CGDisplayCurrentMode(Cocoa_GetDisplay(screen));
  return (mode != NULL) ? getDictDouble(mode, kCGDisplayRefreshRate) : 0.0f;
}

static NSView* windowedView = 0;

void* Cocoa_GL_ResizeWindow(void *theContext, int w, int h, void* sdlView)
{
  if (!theContext)
    return 0;
  
  NSOpenGLContext* context = Cocoa_GL_GetCurrentContext();
  NSView* view;
  NSWindow* window;
  
  view = [context view];
  
  // If we're moving to full-screen, we've probably changed contexts already,
  // so it's better to grab the view from the saved one.
  //
  if (windowedView != 0)
  {
    // First, resize that view, though.
    
    if (view)      
    {
      window = [view window];
      [window setContentSize:NSMakeSize(w, h)];      
      [window update];
      [view setFrameSize:NSMakeSize(w, h)];
      [context update];
      [window center];
    }
    
    view = windowedView;
    window = [view window];
  }
  
  if (view && w>0 && h>0)
  {
    window = [view window];
    if (window)
    {
      [window setContentSize:NSMakeSize(w, h)];
      [window update];
      [view setFrameSize:NSMakeSize(w, h)];
      [context update];
      [window center];
    }
  }

  // HACK: resize SDL's view manually so that mouse bounds are correctly
  // updated.

  if ( sdlView )
  {
    [ (NSQuickDrawView*)sdlView setFrameSize:NSMakeSize(w,h) ];
  }

  return context;
}

void Cocoa_GL_BlankOtherDisplays(int screen)
{
  int numDisplays = Cocoa_GetNumDisplays();
  int i = 0;

  // Blank.
  for (i=0; i<numDisplays; i++)
  {
    if (i != screen && blankingWindows[i] == 0)
    {
      // Get the size.
      NSScreen* pScreen = [[NSScreen screens] objectAtIndex:i];
      NSRect    screenRect = [pScreen frame];
          
      // Build a blanking window.
      screenRect.origin.x = 0.0;
      screenRect.origin.y = 0.0;
      blankingWindows[i] = [[NSWindow alloc] initWithContentRect:screenRect
                                             styleMask:NSBorderlessWindowMask
                                             backing:NSBackingStoreBuffered
                                             defer:NO 
                                             screen:pScreen];
                                            
      [blankingWindows[i] setBackgroundColor:[NSColor blackColor]];
      [blankingWindows[i] setLevel:CGShieldingWindowLevel()];
      [blankingWindows[i] makeKeyAndOrderFront:nil];
    }
  } 
}

void Cocoa_GL_UnblankOtherDisplays(int screen)
{
  int numDisplays = Cocoa_GetNumDisplays();
  int i = 0;

  for (i=0; i<numDisplays; i++)
  {
    if (blankingWindows[i] != 0)
    {
      // Get rid of the blanking window we created.
      [blankingWindows[i] close];
      [blankingWindows[i] release];
      blankingWindows[i] = 0;
    }
  }
}

static NSOpenGLContext* lastOwnedContext = 0;

void Cocoa_GL_SetFullScreen(int screen, int width, int height, bool fs, bool blankOtherDisplays, bool gl_FullScreen)
{
  static NSView* lastView = NULL;
  static CGDirectDisplayID fullScreenDisplayID = 0;
  static NSScreen* lastScreen = NULL;
  static NSWindow* mainWindow = NULL;
  static NSPoint last_origin;
  static NSPoint view_origin;
  int screen_index;

  // If we're already fullscreen then we must be moving to a different display.
  // Recurse to reset fullscreen mode and then continue.
  if (fs == true && lastScreen != NULL)
    Cocoa_GL_SetFullScreen(0, 0, 0, false, blankOtherDisplays, gl_FullScreen);
  
  NSOpenGLContext* context = (NSOpenGLContext*)Cocoa_GL_GetCurrentContext();
  
  if (!context)
    return;
  
  if (fs)
  {    
    // FullScreen Mode
    NSOpenGLContext* newContext = NULL;
  
    // Fade to black to hide resolution-switching flicker and garbage.
    CGDisplayFadeReservationToken fade_token = kCGDisplayFadeReservationInvalidToken;
    if (CGAcquireDisplayFadeReservation (5, &fade_token) == kCGErrorSuccess )
      CGDisplayFade(fade_token, 0.3, kCGDisplayBlendNormal, kCGDisplayBlendSolidColor, 0.0, 0.0, 0.0, TRUE);

    // Save and make sure the view is on the screen that we're activating (to hide it).
    lastView = [context view];
    lastScreen = [[lastView window] screen];
    screen_index = screen;
    
    if (gl_FullScreen)
    {
      // hide the window
      view_origin = [lastView frame].origin;
      last_origin = [[lastView window] frame].origin;
      [[lastView window] setFrameOrigin:[lastScreen frame].origin];
      // expand the mouse bounds in SDL view to fullscreen
      [ lastView setFrameOrigin:NSMakePoint(0.0,0.0)];
      [ lastView setFrameSize:NSMakeSize(width,height) ];

      // This is OpenGL FullScreen Mode
      // obtain fullscreen pixel format
      NSOpenGLPixelFormat* pixFmt = (NSOpenGLPixelFormat*)Cocoa_GL_GetFullScreenPixelFormat(screen_index);
      if (!pixFmt)
        return;
      
      // create our new context (sharing with the current one)
      newContext = (NSOpenGLContext*)Cocoa_GL_CreateContext((void*) pixFmt, (void*)context);
      
      // release pixelformat
      [pixFmt release];
      pixFmt = nil;
      
      if (!newContext)
        return;
      
      // clear the current context
      [NSOpenGLContext clearCurrentContext];
              
      // set fullscreen
      [newContext setFullScreen];
      
      // Capture the display before going fullscreen.
      fullScreenDisplayID = (CGDirectDisplayID)Cocoa_GetDisplayID(screen_index);
      if (blankOtherDisplays == true)
        CGCaptureAllDisplays();
      else
        CGDisplayCapture(fullScreenDisplayID);

      // If we don't hide menu bar, it will get events and interrupt the program.
      if (fullScreenDisplayID == kCGDirectMainDisplay)
        HideMenuBar();
    }
    else
    {
      // This is Cocca Windowed FullScreen Mode
      // Get the screen rect of our current display
      NSScreen* pScreen = [[NSScreen screens] objectAtIndex:screen_index];
      NSRect    screenRect = [pScreen frame];
      
      // remove frame origin offset of orginal display
      screenRect.origin = NSZeroPoint;
      
      // make a new window to act as the windowedFullScreen
      mainWindow = [[NSWindow alloc] initWithContentRect:screenRect
        styleMask:NSBorderlessWindowMask
        backing:NSBackingStoreBuffered
        defer:NO 
        screen:pScreen];
                                              
      [mainWindow setBackgroundColor:[NSColor blackColor]];
      [mainWindow makeKeyAndOrderFront:nil];
      
      [mainWindow setLevel:NSNormalWindowLevel]; // make our window the same level as the rest to enable cmd+tab switching
      //[mainWindow setLevel:CGShieldingWindowLevel()]; // this will make our window topmost and hide all system messages

      // ...and the original one beneath it and on the same screen.
      view_origin = [lastView frame].origin;
      last_origin = [[lastView window] frame].origin;
      [[lastView window] setLevel:NSNormalWindowLevel-1];
      [[lastView window] setFrameOrigin:[pScreen frame].origin];
      // expand the mouse bounds in SDL view to fullscreen
      [ lastView setFrameOrigin:NSMakePoint(0.0,0.0)];
      [ lastView setFrameSize:NSMakeSize(width,height) ];
          
      NSView* blankView = [[NSView alloc] init];
      [mainWindow setContentView:blankView];
      [mainWindow setContentSize:NSMakeSize(width, height)];
      [mainWindow update];
      [blankView setFrameSize:NSMakeSize(width, height)];
      
      // Obtain windowed pixel format and create a new context.
      NSOpenGLPixelFormat* pixFmt = (NSOpenGLPixelFormat*)Cocoa_GL_GetWindowPixelFormat();
      newContext = (NSOpenGLContext*)Cocoa_GL_CreateContext((void* )pixFmt, (void* )context);
      [pixFmt release];
      
      [newContext setView:blankView];
      
      // Hide the menu bar.
      fullScreenDisplayID = (CGDirectDisplayID)Cocoa_GetDisplayID(screen_index);
      if (fullScreenDisplayID == kCGDirectMainDisplay)
        HideMenuBar();
          
      // Blank other displays if requested.
      if (blankOtherDisplays)
        Cocoa_GL_BlankOtherDisplays(screen_index);
    }

    // Hide the mouse.
    [NSCursor hide];
    
    // Release old context if we created it.
    if (lastOwnedContext == context)
      Cocoa_GL_ReleaseContext((void*)context);

    // activate context
    [newContext makeCurrentContext];
    lastOwnedContext = newContext;
    
    if (fade_token != kCGDisplayFadeReservationInvalidToken) 
    {
      CGDisplayFade(fade_token, 0.5, kCGDisplayBlendSolidColor, kCGDisplayBlendNormal, 0.0, 0.0, 0.0, FALSE);
      CGReleaseDisplayFadeReservation(fade_token);
    }
    
    Cocoa_GL_ResizeWindow(newContext, width, height);
  }
  else
  {
    // Windowed Mode
  	// Fade to black to hide resolution-switching flicker and garbage.
  	CGDisplayFadeReservationToken fade_token = kCGDisplayFadeReservationInvalidToken;
    if (CGAcquireDisplayFadeReservation (5, &fade_token) == kCGErrorSuccess )
      CGDisplayFade(fade_token, 0.3, kCGDisplayBlendNormal, kCGDisplayBlendSolidColor, 0.0, 0.0, 0.0, TRUE);
    
    // exit fullscreen
    [context clearDrawable];

    [NSCursor unhide];
    
    // Show menubar.
    if (fullScreenDisplayID == kCGDirectMainDisplay)
      ShowMenuBar();

    if (gl_FullScreen)
    {
      // release displays
      CGReleaseAllDisplays();
    }
    else
    {
      [[lastView window] setLevel:NSNormalWindowLevel];
      
      // Get rid of the new window we created.
      [mainWindow close];
      [mainWindow release];

      // Unblank.
      if (blankOtherDisplays)
      {
        lastScreen = [[lastView window] screen];
        screen_index = Cocoa_GetDisplayIndex( Cocoa_GetDisplayIDFromScreen(lastScreen) );

        Cocoa_GL_UnblankDisplays();
      }
    }

    // obtain windowed pixel format
    NSOpenGLPixelFormat* pixFmt = (NSOpenGLPixelFormat*)Cocoa_GL_GetWindowPixelFormat();
    if (!pixFmt)
      return;
    
    // create our new context (sharing with the current one)
    NSOpenGLContext* newContext = (NSOpenGLContext*)Cocoa_GL_CreateContext((void* )pixFmt, (void* )context);
    
    // release pixelformat
    [pixFmt release];
    pixFmt = nil;
    
    if (!newContext)
      return;
    
    // Assign view from old context, move back to original screen.
    [newContext setView:lastView];
    [ [lastView window] setContentSize:NSMakeSize(width, height) ];
    [ [lastView window] setFrameOrigin:last_origin ];
    [ lastView setFrameSize:NSMakeSize(width, height) ];
    [ lastView setFrameOrigin:view_origin ];
    
    // Release the fullscreen context.
    if (lastOwnedContext == context)
      Cocoa_GL_ReleaseContext((void*)context);
    
    // Activate context.
    [newContext makeCurrentContext];
    lastOwnedContext = newContext;

    if (fade_token != kCGDisplayFadeReservationInvalidToken) 
    {
      CGDisplayFade(fade_token, 0.5, kCGDisplayBlendSolidColor, kCGDisplayBlendNormal, 0.0, 0.0, 0.0, FALSE);
      CGReleaseDisplayFadeReservation(fade_token);
    }
    
    // Reset.
    lastView = NULL;
    lastScreen = NULL;
    mainWindow = NULL;
    fullScreenDisplayID = 0;    
  }
}

void Cocoa_GL_EnableVSync(bool enable)
{
#if 1
  NSOpenGLContext* context = (NSOpenGLContext*)Cocoa_GL_GetCurrentContext();
  
  // Flush synchronised with vertical retrace                       
  long theOpenGLCPSwapInterval = enable ? 1 : 0;
  [context setValues:&theOpenGLCPSwapInterval forParameter:NSOpenGLCPSwapInterval];
  
#else

  CGLContextObj cglContext;
  cglContext = CGLGetCurrentContext();
  if (cglContext)
  {
    long interval;
    if (enable)
      interval = 1;
    else
      interval = 0;
    
    int cglErr = CGLSetParameter(cglContext, kCGLCPSwapInterval, &interval);
    if (cglErr != kCGLNoError)
      printf("ERROR: CGLSetParameter for kCGLCPSwapInterval failed with error %d: %s", cglErr, CGLErrorString(cglErr));
  }
#endif
}

void* Cocoa_GL_GetWindowPixelFormat()
{
  NSOpenGLPixelFormatAttribute wattrs[] =
  {
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFAWindow,
    NSOpenGLPFANoRecovery,
    NSOpenGLPFAAccelerated,
    NSOpenGLPFADepthSize, 8,
    //NSOpenGLPFAColorSize, 32,
    //NSOpenGLPFAAlphaSize, 8,
    0
  };
  NSOpenGLPixelFormat* pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:wattrs];
  return (void*)pixFmt;
}

void* Cocoa_GL_GetFullScreenPixelFormat(int screen)
{
  NSOpenGLPixelFormatAttribute fsattrs[] =
  {
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFAFullScreen,
    NSOpenGLPFANoRecovery,
    NSOpenGLPFAAccelerated,
    NSOpenGLPFADepthSize, 8,
    NSOpenGLPFAScreenMask,
    CGDisplayIDToOpenGLDisplayMask((CGDirectDisplayID)Cocoa_GetDisplay(screen)),
    0
  };
  NSOpenGLPixelFormat* pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:fsattrs];
  return (void*)pixFmt;
}

void* Cocoa_GL_GetCurrentContext()
{
  NSOpenGLContext* context = [NSOpenGLContext currentContext];
  return (void*)context;
}

void* Cocoa_GL_CreateContext(void* pixFmt, void* shareCtx)
{
  if (!pixFmt)
    return nil;
  NSOpenGLContext* newContext = [[NSOpenGLContext alloc] initWithFormat:pixFmt
                                                           shareContext:(NSOpenGLContext*)shareCtx];

  // Enable GL multithreading if available.                                                           
  //CGLContextObj theCGLContextObj = (CGLContextObj) [newContext CGLContextObj];
  //CGLEnable(theCGLContextObj, kCGLCEMPEngine);

  // Flush synchronised with vertical retrace                       
  long theOpenGLCPSwapInterval = 1;
  [newContext setValues:(const long*)&theOpenGLCPSwapInterval forParameter:(NSOpenGLContextParameter) NSOpenGLCPSwapInterval];

  return newContext;
}

void* Cocoa_GL_ReplaceSDLWindowContext()
{
  NSOpenGLContext* context = (NSOpenGLContext*)Cocoa_GL_GetCurrentContext();
  NSView* view = [context view];
  
  if (!view)
    return 0;
  
  // disassociate view from context
  [context clearDrawable];
  
  // release the context
  if (lastOwnedContext == context)
    Cocoa_GL_ReleaseContext((void*)context);
  
  // obtain window pixelformat
  NSOpenGLPixelFormat* pixFmt = (NSOpenGLPixelFormat*)Cocoa_GL_GetWindowPixelFormat();
  if (!pixFmt)
    return 0;
  
  NSOpenGLContext* newContext = (NSOpenGLContext*)Cocoa_GL_CreateContext((void*)pixFmt, nil);
  [pixFmt release];
  
  if (!newContext)
    return 0;
  
  // associate with current view
  [newContext setView:view];
  [newContext makeCurrentContext];
  lastOwnedContext = newContext;
  
  return newContext;
}

int Cocoa_DimDisplayNow()
{
	io_registry_entry_t r = IORegistryEntryFromPath(kIOMasterPortDefault, "IOService:/IOResources/IODisplayWrangler");
	if(!r) return 1;
	int err = IORegistryEntrySetCFProperty(r, CFSTR("IORequestIdle"), kCFBooleanTrue);
	IOObjectRelease(r);
	return err;
}

void Cocoa_UpdateSystemActivity()
{
  UpdateSystemActivity(UsrActivity);   
}
                   
int Cocoa_SleepSystem()
{
  io_connect_t root_domain;
  mach_port_t root_port;

  if (KERN_SUCCESS != IOMasterPort(MACH_PORT_NULL, &root_port))
    return 1;

  if (0 == (root_domain = IOPMFindPowerManagement(root_port)))    
    return 2;

  if (kIOReturnSuccess != IOPMSleepSystem(root_domain))
    return 3;

  return 0;
}        

void Cocoa_HideMouse()
{
  [NSCursor hide];
}

void Cocoa_GetSmartFolderResults(const char* strFile, void (*CallbackFunc)(void* userData, void* userData2, const char* path), void* userData, void* userData2)
{
  NSString*     filePath = [[NSString alloc] initWithUTF8String:strFile];
  NSDictionary* doc = [[NSDictionary alloc] initWithContentsOfFile:filePath];
  NSString*     raw = [doc objectForKey:@"RawQuery"];
  NSArray*      searchPaths = [[doc objectForKey:@"SearchCriteria"] objectForKey:@"FXScopeArrayOfPaths"];

  if (raw == 0)
    return;

  // Ugh, Carbon from now on...
  MDQueryRef query = MDQueryCreate(kCFAllocatorDefault, (CFStringRef)raw, NULL, NULL);
  if (query)
  {
  	if (searchPaths)
  	  MDQuerySetSearchScope(query, (CFArrayRef)searchPaths, 0);
  	  
    MDQueryExecute(query, 0);

	// Keep track of when we started.
	CFAbsoluteTime startTime = CFAbsoluteTimeGetCurrent(); 
    for (;;)
    {
      CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, YES);
    
      // If we're done or we timed out.
      if (MDQueryIsGatheringComplete(query) == true ||
      	  CFAbsoluteTimeGetCurrent() - startTime >= 5)
      {
        // Stop the query.
        MDQueryStop(query);
      
    	CFIndex count = MDQueryGetResultCount(query);
    	char title[BUFSIZ];
    	int i;
  
    	for (i = 0; i < count; ++i) 
   		{
      	  MDItemRef resultItem = (MDItemRef)MDQueryGetResultAtIndex(query, i);
      	  CFStringRef titleRef = (CFStringRef)MDItemCopyAttribute(resultItem, kMDItemPath);
      
      	  CFStringGetCString(titleRef, title, BUFSIZ, kCFStringEncodingUTF8);
      	  CallbackFunc(userData, userData2, title);
      	  CFRelease(titleRef);
    	}  
    
        CFRelease(query);
    	break;
      }
    }
  }
  
  // Freeing these causes a crash when scanning for new content.
  CFRelease(filePath);
  CFRelease(doc);
}

static char strVersion[32];

const char* Cocoa_GetAppVersion()
{
  // Get the main bundle for the app and return the version.
  CFBundleRef mainBundle = CFBundleGetMainBundle();
  CFStringRef versStr = (CFStringRef)CFBundleGetValueForInfoDictionaryKey(mainBundle, kCFBundleVersionKey);
  
  memset(strVersion,0,32);
  
  if (versStr != NULL && CFGetTypeID(versStr) == CFStringGetTypeID())
  {
    bool res = CFStringGetCString(versStr, strVersion, 32,kCFStringEncodingUTF8);
    if (!res)
    {
      printf("Error converting version string\n");      
      strcpy(strVersion, "SVN");
    }
  }
  else
    strcpy(strVersion, "SVN");
  
  return strVersion;
}

void* Cocoa_GetDisplayPort()
{
  //NSOpenGLContext* context = (NSOpenGLContext*)Cocoa_GL_GetCurrentContext();
  //NSView* view = [context view];
  //WindowRef refWindow = [[view window] windowRef];
  //return GetWindowPort(refWindow);
  
  WindowRef refWindow = [childWindow windowRef];
  return (void* )GetWindowPort(refWindow);
}

const char *Cocoa_Paste() 
{
  NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
  NSString *type = [pasteboard availableTypeFromArray:[NSArray arrayWithObject:NSStringPboardType]];
  if (type != nil) {
    NSString *contents = [pasteboard stringForType:type];
    if (contents != nil) {
      return [contents UTF8String];
    }
  }
  
  return NULL;
}


