#ifndef __BX_NATIVE_APP__
#define __BX_NATIVE_APP__

/* native applications - framework for boxee native applications
 * Copyright (C) 2010 Boxee.tv.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#define BOXEE_API_VERSION    2    /* Calls from app to boxee */
#define APP_API_VERSION      1    /* Calls from boxee to app (mainly events) */

#ifdef __cplusplus
extern "C"
{
#endif
  typedef enum { BX_FALSE = 0, BX_TRUE = 1 }   BX_Bool;

  /**
   * Handle to the app 
   */
  struct _BX_Handle
  {
    /**
     * Name for the app
     */
    char appName[128];
    
    /**
     * Private data stored by boxee. Should not be modified by the app.
     */
    void *boxeeData;
    
    /**
     * Private data stored by app. The app can set this to any value.
     */    
    void *appData;
  };
  
  typedef struct _BX_Handle* BX_Handle;
  
  //////////////////////////////////////////////////////////////////////////////////////
  //
  // Environment
  //
  //////////////////////////////////////////////////////////////////////////////////////

  /**
   * Retrieves the version of boxee
   * @param hApp [in] handle to the app
   * @returns version in form <major>.<minor>.<sub-minor>
   */
  typedef const char* (*BX_GetBOXEEVersionString_FuncType) (BX_Handle hApp);
  
  /**
   * Returns a path to a temporary directory
   * @param hApp [in] handle to the app
   * @returns path
   */
  typedef const char* (*BX_GetTempDir_FuncType) (BX_Handle hApp);
  
  /**
   * Retrieves the resolution of the display window or full screen.
   * @param hApp [in] handle to the app
   * @param nWidth [out] width of the window/fullscreen in pixels
   * @param nHeight [out] height of the window/fullscreen in pixels
   */
  typedef void (*BX_GetDisplayResolution_FuncType) (BX_Handle hApp, unsigned int *nWidth, unsigned int *nHeight);

  /**
   * Retrieves the resolution of the unscaled boxee skin.
   * @param hApp [in] handle to the app
   * @param nWidth [out] width of the skin in pixels. Current skin returns always 1280.
   * @param nHeight [out] height of the skin in pixels. Current skin returns always 720.
   */  
  typedef void (*BX_GetSkinResolution_FuncType) (BX_Handle hApp, unsigned int *nWidth, unsigned int *nHeight);

  /**
   * Checks whether the internet connection is active.
   * @param hApp [in] handle to the app
   * @returns BX_TRUE is the internet is connected, BX_FALSE otherwise
   */    
  typedef BX_Bool (*BX_IsInternetConnected_FuncType) (BX_Handle hApp);
  
  /**
   * Returns a unique id for the host running boxee.
   * @param hApp [in] handle to the app
   * @param uid [out] unique id, maximum size is 128 characters
   */      
  typedef void (*BX_GetUniqueId_FuncType) (BX_Handle hApp, char* uid);
  
  /**
   * Writes string to the boxee log file with DEBUG level.
   * @param hApp [in] handle to the app
   * @param fmtString [in] format string, like printf()
   */      
  typedef void (*BX_LogDebug_FuncType) (BX_Handle hApp, const char *fmtString, ...);

  /**
   * Writes string to the boxee log file with INFO level.
   * @param hApp [in] handle to the app
   * @param fmtString [in] format string, like printf()
   */        
  typedef void (*BX_LogInfo_FuncType) (BX_Handle hApp, const char *fmtString, ...);
  
  /**
   * Writes string to the boxee log file with ERROR level.
   * @param hApp [in] handle to the app
   * @param fmtString [in] format string, like printf()
   */        
  typedef void (*BX_LogError_FuncType) (BX_Handle hApp, const char *fmtString, ...);
  
  /**
   * Called by the app to notify boxee that the application has stopped. This could be caused
   * due to an event in the app or due to an appStop() request from boxee
   * @param hApp [in] handle to the app
   */        
  typedef void (*BX_NotifyAppStopped) (BX_Handle hApp);
    
  //////////////////////////////////////////////////////////////////////////////////////
  //
  // Graphics
  //
  //////////////////////////////////////////////////////////////////////////////////////

  /**
   * Pixel format
   */  
  typedef enum 
  {
    /**
     * BGRA - 32 bit
     */
    BX_PF_BGRA8888 = 1,
    /**
     * Alpha - 8 bit
     */
    BX_PF_A8 = 2 
  } BX_PixelFormat;
  
  /**
   * 32 bit color ALPHA-RED-GREEN-BLUE
   */
  typedef unsigned int BX_Color; 
  
  /**
   * Surface
   */
  typedef struct 
  {
    /**
     * Handle to the app
     */
    BX_Handle      hApp;
    /**
     * Pixel format
     */
    BX_PixelFormat pixelFormat;
    /**
     * Width in pixels
     */
    unsigned int   w;
    /**
     * Height in pixels
     */
    unsigned int   h;
    /**
     * Bytes per pixel
     */
    unsigned int   bpp;
    /**
     * Pitch -- number of bytes per surface line
     */
    unsigned int   pitch;
    /**
     * Pointer to the pixels in memory.
     */
    unsigned char* pixels;
    /**
     * Private data for boxee use only
     */
    void*          priv;
  } BX_Surface;

  /**
   * Rectangle
   */
  typedef struct 
  {
    /**
     * X position in pixels
     */
    int x;
    /**
     * Y position in pixels
     */
    int y;
    /**
     * Width in pixels
     */
    int w;
    /**
     * Height in pixels
     */
    int h;
  } BX_Rect;

  /**
   * Blending mode when blitting surfaces
   */
  typedef enum 
  {
    /**
     * No blending
     */
    BX_BLEND_NONE = 0, 
    /**
     * Source alpha blending
     */
    BX_BLEND_SOURCE_ALPHA, 
    /**
     * Destination alpha blending
     */
    BX_BLEND_DEST_ALPHA 
  } BX_BlendMethod;
  
  /**
   * Handle to a window
   */
  typedef struct 
  {
    /**
     * Handle to the app
     */
    BX_Handle hApp;
    /**
     * Private data for boxee.
     */
    void*     priv;
  } _BX_WindowHandle;
  
  typedef _BX_WindowHandle* BX_WindowHandle;
  
  typedef enum {
    MBT_OK,
    MBT_OK_CANCEL,
    MBT_YES_NO
  } BX_MessageBoxType;

  typedef enum {
    MBR_OK,
    MBR_CANCEL,
    MBR_YES,
    MBR_NO
  } BX_MessageBoxResult;

  typedef struct
  {
    int top;
    int left;
    int bottom;
    int right;
  } BX_Overscan;

  typedef enum
  {
    BX_MODE_3D_NONE           = 0x0,
    BX_MODE_3D_FRAME_PACKED   = 0x1,
    BX_MODE_3D_SIDE_BY_SIDE   = 0x2,
    BX_MODE_3D_TOP_AND_BOTTOM = 0x4,
  } BX_Mode3D;

  typedef enum
  {
    BX_AUDIO_CAPS_DISABLED = -1,
    BX_AUDIO_CAPS_NONE     = 0x00,
    BX_AUDIO_CAPS_PCM      = 0x01,
    BX_AUDIO_CAPS_LPCM71   = 0x02,
    BX_AUDIO_CAPS_DD       = 0x04,
    BX_AUDIO_CAPS_DDP      = 0x08,
    BX_AUDIO_CAPS_TRUEHD   = 0x10,
    BX_AUDIO_CAPS_DTS      = 0x20,
    BX_AUDIO_CAPS_DTSMA    = 0x40,
  } BX_AudioOutputCaps;

  typedef enum
  {
    BX_AUDIO_OUTPUT_HDMI    = 0x1,
    BX_AUDIO_OUTPUT_SPDIF   = 0x2,
    BX_AUDIO_OUTPUT_I2S0    = 0x4,
    BX_AUDIO_OUTPUT_I2S1    = 0x8,
  } BX_AudioOutputPort;

  /**
   * Create a window. The dimensions of the window are the same as the boxee skin.
   * @param hApp [in] handle to the app
   * @return handle to a window
   */
  typedef BX_WindowHandle (*BX_WindowCreate_FuncType) (BX_Handle hApp);
  
  /**
   * Destroy a window.
   * @param hWindow [in] handle to a window
   */  
  typedef void  (*BX_WindowDestroy_FuncType) (BX_WindowHandle hWindow);
  
  /**
   * Get the surface of the window. Use can blit on this surface to render to the window
   * @param hWindow [in] Handle to the window
   * @return the surface of the window
   */
  typedef BX_Surface* (*BX_GetWindowFramebuffer_FuncType) (BX_WindowHandle hWindow);
  
  /**
   * Render the window handle to the screen 
   */
  typedef void        (*BX_WindowFlip_FuncType)           (BX_WindowHandle hWindow);

  /**
   * Create an off-window surface
   * @param hApp [in] handle to the app
   * @param pixelFormat [in] the pixel format of the surface
   * @param nWidth [in] width of the surface in pixels
   * @param nHeight [in] height of the surface in pixels
   * @return a surface. Null if not enough memory exists.
   */
  typedef BX_Surface* (*BX_SurfaceCreate_FuncType)  (BX_Handle hApp, BX_PixelFormat pixelFormat, unsigned int nWidth, unsigned int nHeight);
  
  /**
   * Create a surface from an image stream. Image format could be either PNG or JPG.
   * 
   * @param hApp [in] handle to the app
   * @param image [in] pointer to the image stream
   * @param nSize [in] size of image stream
   * @return a surface. Null if the image could not be loaded.
   */
  typedef BX_Surface* (*BX_SurfaceCreateFromImage_FuncType)  (BX_Handle hApp, const char *image, unsigned int nSize);
  
  /**
   * Load an image to a surface. Image format could be either PNG or JPG. The image should scale to the size of width and height.
   * If width and height are set to 0, scale to fill entire surface.
   * 
   * @param surface [in] handle to the surface
   * @param image [in] pointer to the image stream
   * @param nSize [in] size of image stream
   * @param nWidth [in] width of the image. Zero to scale to entire surface width.
   * @param nHeight [in] height of the image. Zero to scale to entire surface height.
   * @return BX_TRUE on success, BX_FALSE on failure
   */
  typedef BX_Bool     (*BX_SurfaceLoadImage_FuncType)      (BX_Surface* surface, const char *image, unsigned int nSize, unsigned int nWidth, unsigned int nHeight);
  
  /**
   * Release a surface
   * 
   * @param surface [in] handle to the surface
   */
  typedef void (*BX_SurfaceRelease_FuncType) (BX_Surface* surface);
  
  /**
   * Lock a surface
   * 
   * @param surface [in] handle to the surface
   */  
  typedef void (*BX_SurfaceLock_FuncType) (BX_Surface* surface);
  
  /**
   * Unlock a surface
   * 
   * @param surface [in] handle to the surface
   */    
  typedef void (*BX_SurfaceUnlock_FuncType) (BX_Surface* surface);
  
  /**
   * Fill a surface with a rectangle of color
   * 
   * @param surface [in] the surface to draw the rectangle in
   * @param color [in] color of the rectangle
   * @param rect [in] rectangle coordinates
   * @param blend [in] how the rectangle will be blended with the current content of the surface
   */    
  typedef void (*BX_SurfaceFillRect_FuncType) (BX_Surface* surface, BX_Color color, BX_Rect rect, BX_BlendMethod blend);

  /**
   * Blit a surface to another surface
   * 
   * @param sourceSurface [in] the surface which is the source of the blit
   * @param destSurface [in] the surface which is the destination of the blit
   * @param sourceRect [in] rectangle in the source surface 
   * @param destRect [in] rectangle in the destination surface 
   * @param blend [in] how the blitted content will be blended with the current content of the surface
   */      
  typedef void (*BX_SurfaceBlit_FuncType) (BX_Surface* sourceSurface, BX_Surface* destSurface, BX_Rect sourceRect, BX_Rect destRect, BX_BlendMethod blend, unsigned char alpha);
  
  /**
   * Blit a surface to another surface. The source surface is an alpha-only surface which is blitted along with the color
   * 
   * @param sourceSurface [in] the surface which is the source of the blit (A8 pixel format)
   * @param destSurface [in] the surface which is the destination of the blit
   * @param sourceRect [in] rectangle in the source surface 
   * @param destRect [in] rectangle in the destination surface 
   * @param blend [in] how the blitted content will be blended with the current content of the surface
   * @param color [in] the color (RGB) to be added to the source surface (which has only alpha values) 
   */  
  typedef void  (*BX_SurfaceBlitWithColor_FuncType)  (BX_Surface* sourceSurface, BX_Surface* destSurface, BX_Rect sourceRect, BX_Rect destRect, BX_BlendMethod blend, BX_Color color);
    
  //
  // playback
  //  
  typedef struct sBXPlayer
  {
    BX_Handle hApp;
    void*     priv;
  } *BX_PlayerHandle;  
  
  typedef BX_PlayerHandle (*BX_PlayerCreate_FuncType) (BX_Handle hApp);
  typedef void            (*BX_PlayerDestroy_FuncType) (BX_PlayerHandle hPlayer);
  typedef unsigned int    (*BX_PlayerFeed_FuncType)  (BX_PlayerHandle hPlayer, const char *data, unsigned int nDataLen ); // returns number of consumed bytes

  typedef BX_Bool         (*BX_PlayerPlay_FuncType)  (BX_PlayerHandle hPlayer, const char *mimeType);
  typedef void            (*BX_PlayerStop_FuncType)  (BX_PlayerHandle hPlayer);
  typedef void            (*BX_PlayerEOF_FuncType)   (BX_PlayerHandle hPlayer);
  typedef void            (*BX_PlayerSetVolume_FuncType)         ( BX_PlayerHandle hPlayer, unsigned int nVol ); // 0-100
  typedef void            (*BX_PlayerSetSpeed_FuncType)  ( BX_PlayerHandle hPlayer, float nSpeed );
  typedef float           (*BX_PlayerGetSpeed_FuncType)  ( BX_PlayerHandle hPlayer);
  typedef void            (*BX_PlayerSetPos_FuncType)  ( BX_PlayerHandle hPlayer, unsigned int nSecond );
  typedef unsigned int    (*BX_PlayerGetPos_FuncType)  ( BX_PlayerHandle hPlayer);

  //////////////////////////////////////////////////////////////////////////////////////
  //
  // Registry
  //
  //////////////////////////////////////////////////////////////////////////////////////
  
  /**
   * Get a value from the registry
   * 
   * @param hApp [in] handle to the app
   * @param key [in] key to lookup
   * @return value or NULL if not found
   */
  typedef const char* (*BX_RegistryGet_FuncType) (BX_Handle hApp, const char* key);

  /**
   * Set a value in the registry
   * 
   * @param hApp [in] handle to the app
   * @param key [in] key to store
   * @param key [in] key to store
   */  
  typedef void (*BX_RegistrySet_FuncType) (BX_Handle hApp, const char* key, const char* value);
  
  /**
   * Delete a value in the registry
   * 
   * @param hApp [in] handle to the app
   * @param key [in] key to delete
   */    
  typedef void (*BX_RegistryUnset_FuncType) (BX_Handle hApp, const char* key);
  
  /**
   * Add additional value to a key in the registry (add first value)
   * 
   * @param hApp [in] handle to the app
   * @param key [in] key to edit
   * @param value [in] value to edit
   * @param nLimit [in] maximum values allowed
   */      
  typedef void (*BX_RegistryPushBack_FuncType) (BX_Handle hApp, const char* key, const char* value, unsigned int nLimit);
  
  /**
   * Add additional value to a key in the registry (add last value)
   * 
   * @param hApp [in] handle to the app
   * @param key [in] key to edit
   * @param value [in] value to edit
   * @param nLimit [in] maximum values allowed
   */        
  typedef void (*BX_RegistryPushFront_FuncType) (BX_Handle hApp, const char* key, const char* value, unsigned int nLimit);
  
  /**
   * Clear all key/values from the registry
   * 
   * @param hApp [in] handle to the app
   */        
  typedef void (*BX_RegistryClear_FuncType) (BX_Handle hApp);

  ////////////////////////////////////////////////////////////////////////////////////
  //
  // Persistent registry. Does not delete even when box is completely wiped. On
  // personal computers it is the same as registry functions.
  //
  //////////////////////////////////////////////////////////////////////////////////////

  /**
   * Get a value from the persistent registry
   *
   * @param hApp [in] handle to the app
   * @param key [in] key to lookup
   * @return value or NULL if not found
   */
  typedef const char* (*BX_PersistentGet_FuncType) (BX_Handle hApp, const char* key);

  /**
   * Set a value in the persistent registry
   *
   * @param hApp [in] handle to the app
   * @param key [in] key to store
   * @param key [in] key to store
   */
  typedef void (*BX_PersistentSet_FuncType) (BX_Handle hApp, const char* key, const char* value);

  /**
   * Delete a value in the persistent registry
   *
   * @param hApp [in] handle to the app
   * @param key [in] key to delete
   */
  typedef void (*BX_PersistentUnset_FuncType) (BX_Handle hApp, const char* key);

  /**
   * Add additional value to a key in the persistent registry (add first value)
   *
   * @param hApp [in] handle to the app
   * @param key [in] key to edit
   * @param value [in] value to edit
   * @param nLimit [in] maximum values allowed
   */
  typedef void (*BX_PersistentPushBack_FuncType) (BX_Handle hApp, const char* key, const char* value, unsigned int nLimit);

  /**
   * Add additional value to a key in the persistent registry (add last value)
   *
   * @param hApp [in] handle to the app
   * @param key [in] key to edit
   * @param value [in] value to edit
   * @param nLimit [in] maximum values allowed
   */
  typedef void (*BX_PersistentPushFront_FuncType) (BX_Handle hApp, const char* key, const char* value, unsigned int nLimit);

  /**
   * Clear all key/values from the persistent registry
   *
   * @param hApp [in] handle to the app
   */
  typedef void (*BX_PersistentClear_FuncType) (BX_Handle hApp);

  /**
   * display an "ok" message
   *
   * @param title   [in] title of the dialog
   * @param text    [in] the text to display
   * @param type    [in] type of message box to show
   */
  typedef BX_MessageBoxResult (*BX_MessageBox_FuncType) (const char *title, const char *text, BX_MessageBoxType type);

  typedef void (*BX_ShowTip_FuncType) (const char *text, int durationSeconds);

  typedef void (*BX_GetDisplayOverscan_FuncType) (BX_Overscan* overscan);

  typedef void (*BX_BoxeeRenderingEnabled_FuncType) (bool enabled);

  typedef BX_Mode3D (*BX_DimensionalityGetSupportedModes) (void);
  typedef void (*BX_DimensionalitySetMode) (BX_Mode3D mode);
  typedef BX_Mode3D (*BX_DimensionalityGetMode) (void);

  typedef void (*BX_AudioGetSettings_FuncType) (BX_AudioOutputPort* output, BX_AudioOutputCaps* caps);

  //
  // Initialization
  //
  typedef struct
  {
    unsigned int                       boxeeApiVersion;
    
    // general
    BX_GetBOXEEVersionString_FuncType  boxeeGetVersionString;
    BX_GetTempDir_FuncType             boxeeGetTempDir;
    BX_GetDisplayResolution_FuncType   boxeeGetDisplayResolution;
    BX_GetSkinResolution_FuncType      boxeeGetSkinResolution;
    BX_IsInternetConnected_FuncType    boxeeIsInternetConnected;
    BX_GetUniqueId_FuncType            boxeeGetUniqueId;
    BX_NotifyAppStopped                boxeeNotifyAppStopped;
    
    // logging
    BX_LogDebug_FuncType               logDebug;
    BX_LogInfo_FuncType                logInfo;
    BX_LogError_FuncType               logError;
    
    // graphics
    BX_WindowCreate_FuncType           windowCreate;
    BX_WindowDestroy_FuncType          windowDestroy;
    BX_GetWindowFramebuffer_FuncType   windowGetFramebuffer;
    BX_WindowFlip_FuncType             windowFlip;
    BX_SurfaceCreate_FuncType          surfaceCreate;
    BX_SurfaceCreateFromImage_FuncType surfaceCreateFromImage;
    BX_SurfaceRelease_FuncType         surfaceRelease;
    BX_SurfaceLock_FuncType            surfaceLock;
    BX_SurfaceUnlock_FuncType          surfaceUnlock;
    BX_SurfaceLoadImage_FuncType       surfaceLoadImage;
    BX_SurfaceFillRect_FuncType        surfaceFillRect;
    BX_SurfaceBlit_FuncType            surfaceBlit;
    BX_SurfaceBlitWithColor_FuncType   surfaceBlitWithColor;
    
    // playback
    BX_PlayerCreate_FuncType           playerCreate;
    BX_PlayerDestroy_FuncType          playerDestroy;
    BX_PlayerFeed_FuncType             playerFeed;
    BX_PlayerPlay_FuncType             playerPlay;
    BX_PlayerStop_FuncType             playerStop;
    BX_PlayerEOF_FuncType              playerEOF;
    BX_PlayerSetVolume_FuncType        playerSetVolume;
    BX_PlayerSetSpeed_FuncType         playerSetSpeed;
    BX_PlayerGetSpeed_FuncType         playerGetSpeed;
    BX_PlayerSetPos_FuncType           playerSetPos;
    BX_PlayerGetPos_FuncType           playerGetPos;
    
    // registry
    BX_RegistryGet_FuncType            registryGet;
    BX_RegistrySet_FuncType            registrySet;
    BX_RegistryUnset_FuncType          registryUnset;
    BX_RegistryPushBack_FuncType       registryPushBack;
    BX_RegistryPushFront_FuncType      registryPushFront;
    BX_RegistryClear_FuncType          registryClear;
    
    // messages
    BX_MessageBox_FuncType             showMessage;             
  
    // resolution
    BX_GetDisplayOverscan_FuncType     getDisplayOverscan;

    // persistent registry
    BX_PersistentGet_FuncType          persistentGet;
    BX_PersistentSet_FuncType          persistentSet;
    BX_PersistentUnset_FuncType        persistentUnset;
    BX_PersistentPushBack_FuncType     persistentPushBack;
    BX_PersistentPushFront_FuncType    persistentPushFront;
    BX_PersistentClear_FuncType        persistentClear;

    BX_BoxeeRenderingEnabled_FuncType  boxeeRenderingEnabled;

    // 3D
    BX_DimensionalityGetSupportedModes dimensionalityGetSupportedModes;
    BX_DimensionalitySetMode           dimensionalitySetMode;
    BX_DimensionalityGetMode           dimensionalityGetMode;

    BX_ShowTip_FuncType                showTip;

    BX_AudioGetSettings_FuncType       audioGetOutputPortCaps;
  } BX_Callbacks; 
  
  //////////////////////////////////////////////////////////////////////////////////////
  //
  // Events received by the app
  //
  //////////////////////////////////////////////////////////////////////////////////////
  
  typedef enum  
  { 
    /** 
     * character key
     */    
    BX_KEY_UNICODE, 
    /** 
     * up
     */    
    BX_KEY_UP, 
    /** 
     * down
     */    
    BX_KEY_DOWN, 
    /** 
     * left
     */    
    BX_KEY_LEFT, 
    /** 
     * right
     */    
    BX_KEY_RIGHT, 
    /** 
     * select / Enter
     */    
    BX_KEY_SELECT, 
    /** 
     * back / ESC
     */    
    BX_KEY_BACK,
    /** 
     * play/pause
     */    
    BX_KEY_PLAY_PAUSE,
    /** 
     * play
     */    
    BX_KEY_PLAY,
    /** 
     * pause
     */    
    BX_KEY_PAUSE,
    /** 
     * stop
     */    
    BX_KEY_STOP,
    /** 
     * skip fw
     */    
    BX_KEY_SKIP_FW,
    /** 
     * skip bw
     */    
    BX_KEY_SKIP_BW,
    /** 
     * fast fw
     */    
    BX_KEY_FAST_FW,
    /** 
     * fast bw
     */    
    BX_KEY_FAST_BW
  } BX_KeyType;

  /**
   * Key event structure
   */
  typedef struct
  {
    /** 
     * Key that was pressed 
     */
    BX_KeyType   keyType;
    
    /**
     * For BX_KEY_UNICODE, the actualy character
     */
    unsigned int unicode;
  } BX_KeyEvent;
  
  /** 
   * Mouse button
   */
  typedef enum 
  { 
    BX_MB_LEFT, 
    BX_MB_RIGHT, 
    BX_MB_MIDDLE, 
    BX_MB_WHEELUP, 
    BX_MB_WHEENDOWN 
  } BX_MouseButton;
  
  typedef enum
  {
    BX_PLAYERSTATE_UNKNOWN = -1,
    BX_PLAYERSTATE_STOPPED,
    BX_PLAYERSTATE_BUFFERING,
    BX_PLAYERSTATE_SEEKING,
    BX_PLAYERSTATE_PAUSED,
    BX_PLAYERSTATE_PLAYING,
  } BX_PlayerState;

  /**
   * Called whenever a key was pressed
   * 
   * @param hWindow [in] handle to window in which the key was pressed
   * @param key [in] key that was pressed
   */
  typedef void (*BX_App_OnKey_FuncType) (BX_WindowHandle hWindow, BX_KeyEvent key);
  
  /**
   * Called whenever a mouse was moved
   * 
   * @param hWindow [in] handle to window in which the key was pressed
   * @param x [in] X coordinate in the window
   * @param y [in] Y coordinate in the window
   */    
  typedef void (*BX_App_OnMouseMove_FuncType) (BX_WindowHandle hWindow, unsigned int x, unsigned int y);

  /**
   * Called whenever a mouse button was pressed
   * 
   * @param hWindow [in] handle to window in which the key was pressed
   * @param button [in] button that was pressed
   * @param x [in] X coordinate in the window
   * @param y [in] Y coordinate in the window
   */    
  typedef void (*BX_App_OnMouseClick_FuncType) (BX_WindowHandle hWindow, BX_MouseButton button, unsigned int x, unsigned int y);

  /**
   * Called when the app is starting. 
   * 
   * @param hApp [in] handle to the app
   * @param argc [in] number of arguments passed in argv
   * @param argv [in] arguments
   */
  typedef BX_Bool (*BX_App_OnStartApp_FuncType) (BX_Handle hApp, int argc, char** argv);  

  /**
   * Called to stop the app. 
   * 
   * @param hApp [in] handle to the app
   */  
  typedef void (*BX_App_OnStopApp_FuncType) (BX_Handle hApp);
  
  //
  // player events
  // 
  typedef void (*BX_App_OnPLaybackEnded_FuncType)    ( BX_PlayerHandle hPlayer );
  typedef void (*BX_App_OnPLaybackUnderrun_FuncType) ( BX_PlayerHandle hPlayer );

  //
  // Windowing events
  //
  typedef void (*BX_App_DisplayRevealed_FuncType)  ( void );
  typedef void (*BX_App_DisplayHidden_FuncType) ( void );
  typedef void (*BX_App_ReduceSurfaceMemoryNow_FuncType) ( unsigned int );
  typedef void (*BX_App_DisplayRender_FuncType) ( BX_WindowHandle hWindow );
 
  typedef void (*BX_App_ScreensaverStateShown_FuncType) ( BX_Handle hApp );
  typedef void (*BX_App_ScreensaverStateHidden_FuncType) ( BX_Handle hApp );
  
  typedef BX_PlayerState (*BX_App_GetPlayerState_FuncType) ( BX_Handle hApp );

  typedef struct
  {
    unsigned int                 appApiVersion;
    
    // input
    BX_App_OnKey_FuncType        onKey;
    BX_App_OnMouseMove_FuncType  onMouseMove;
    BX_App_OnMouseClick_FuncType onMouseClick;
    
    // app events
    BX_App_OnStartApp_FuncType   onAppStart;
    BX_App_OnStopApp_FuncType    onAppStop;
    
    // playback
    BX_App_OnPLaybackEnded_FuncType    onPlaybackEnded;
    BX_App_OnPLaybackUnderrun_FuncType onPlaybackUnderrun;

    // Windowing
    BX_App_DisplayRevealed_FuncType onDisplayRevealed;
    BX_App_DisplayHidden_FuncType onDisplayHidden;
    BX_App_ReduceSurfaceMemoryNow_FuncType onReduceSurfaceMemoryNow;
    BX_App_DisplayRender_FuncType onDisplayRender;
    
    // Screensaver
    BX_App_ScreensaverStateShown_FuncType onScreensaverShown;
    BX_App_ScreensaverStateHidden_FuncType onScreensaverHidden;

    // Video
    BX_App_GetPlayerState_FuncType getPlayerState;

  } BX_App_Methods;

  
  //
  // boxee will load the app and call its initialize methods.
  // @param hApp          - [input] a unique handle for this app. the app will use this handle in future calls into boxee
  // @param appCallbacks  - [input] param with pointers to available boxee callbacks
  // @param appMethods    - [output] the app should fill this struct with pointers to its methods
  //
  // @return - true if app initialized correctly. false if not (app activation will abort).
  //
  typedef BX_Bool (*BX_App_Initialize_FuncType)  ( BX_Handle hApp, const BX_Callbacks *appCallbacks, BX_App_Methods *appMethods );
  
  //----------------------------------------------------------------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif

