/*
* XBMC Media Center
* Linux OpenGL Renderer
* Copyright (c) 2007 Frodo/jcmarshall/vulkanr/d4rk
*
* Based on XBoxRenderer by Frodo/jcmarshall
* Portions Copyright (c) by the authors of ffmpeg / xvid /mplayer
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "system.h"
#if (defined HAVE_CONFIG_H) && (!defined WIN32)
#include "config.h"
#endif

// This define enables the App thread to do the OMX Overlay (Renderer) Flip.  If not defined, the OMX flip is done by the decoder


#if defined(HAS_OPENMAX)      // this is defined in guilib/system.h
#include <locale.h>
#include "LinuxRendererOMX.h"
#include "Application.h"
#include "MathUtils.h"
#include "Settings.h"
#include "AdvancedSettings.h"
#include "GUISettings.h"
#include "FrameBufferObject.h"
#include "WindowingFactory.h"
#include "Texture.h"

#include <OMX_Core.h>
#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Audio.h>
#include <OMX_Video.h>
#include <NVOMX_IndexExtensions.h>


#define INIT_PARAM(_X_)  (memset(&(_X_), 0, sizeof(_X_)), ((_X_).nSize = sizeof (_X_)), (_X_).nVersion = m_vOMX)
 
using namespace Shaders;

static const char *pszOmxYuvOverlayComponentsName[] =
{
  "OMX.Nvidia.render.hdmi.overlay.yuv420",
  "OMX.Nvidia.std.iv_renderer.overlay.yuv420",
};


static OMX_ERRORTYPE OmxRendererEventHandler(
    OMX_OUT OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_PTR pAppData,
    OMX_OUT OMX_EVENTTYPE eEvent,
    OMX_OUT OMX_U32 Data1,
    OMX_OUT OMX_U32 Data2,
    OMX_OUT OMX_PTR pEventData);
static OMX_ERRORTYPE OmxRendererEmptyBufferDone(
    OMX_OUT OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_PTR pAppData,
    OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);
static OMX_ERRORTYPE OmxRendererFillBufferDone(
    OMX_OUT OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_PTR pAppData,
    OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);

OMX_CALLBACKTYPE OmxRendererCallbacks = {
    OmxRendererEventHandler,
    OmxRendererEmptyBufferDone,
    OmxRendererFillBufferDone
};


static OMX_ERRORTYPE OmxRendererEventHandler(
    OMX_OUT OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_PTR pAppData,
    OMX_OUT OMX_EVENTTYPE eEvent,
    OMX_OUT OMX_U32 Data1,
    OMX_OUT OMX_U32 Data2,
    OMX_OUT OMX_PTR pEventData)
{
    EOMXRendererEvent eOmxEvent = eOMXRendererEventUndefined;

    CLog::Log(LOGDEBUG, "OMXRENDER OmxRendererEventHandler() %x called\n", eEvent);

    switch(eEvent) {
        case OMX_EventCmdComplete:
            CLog::Log(LOGDEBUG, "OMXRENDER Event: OMX_EventCmdComplete nData1: %lu nData2: %lu\n", Data1, Data2);
            switch(Data1) {
                case OMX_CommandStateSet:
                    eOmxEvent = eOMXRendererEventStateChanged;
                    CLog::Log(LOGDEBUG, "OMXRENDER State Reached: ");
                    switch ((int)Data2) {
                        case OMX_StateInvalid:
                            CLog::Log(LOGDEBUG, "OMXRENDER OMX_StateInvalid\n");
                            break;
                        case OMX_StateLoaded:
                            CLog::Log(LOGDEBUG, "OMXRENDER OMX_StateLoaded\n");
                            break;
                        case OMX_StateIdle:
                            CLog::Log(LOGDEBUG, "OMXRENDER OMX_StateIdle\n");
                            break;
                        case OMX_StateExecuting:
                            CLog::Log(LOGDEBUG, "OMXRENDER OMX_StateExecuting\n");
                            break;
                        case OMX_StatePause:
                            CLog::Log(LOGDEBUG, "OMXRENDER OMX_StatePause\n");
                            break;
                        case OMX_StateWaitForResources:
                            CLog::Log(LOGDEBUG, "OMXRENDER OMX_StateWaitForResources\n");
                            break;
                        default:
                            CLog::Log(LOGDEBUG, "OMXRENDER Invalid State\n");
                            break;
                    }
                    break;
                case OMX_CommandFlush:
                    eOmxEvent = eOMXRendererEventFlush;
                    break;
                case OMX_CommandPortDisable:
                    eOmxEvent = eOMXRendererEventPortDisable;
                default:
                    break;
            }
            break;
        case OMX_EventError:
            CLog::Log(LOGDEBUG, "OMXRENDER Event: OMX_EventError Error code: %lx Port:%lu\n", Data1, Data2);
            break;
        case OMX_EventMark:
            CLog::Log(LOGDEBUG, "OMXRENDER Event: OMX_EventMark\n");
            break;
        case OMX_EventPortSettingsChanged:
            CLog::Log(LOGDEBUG, "OMXRENDER Event: OMX_EventPortSettingsChanged Port: %lu\n", Data1);
            eOmxEvent = eOMXRendererEventPortSettingsChanged ;
            break;
        case OMX_EventBufferFlag:
            eOmxEvent = eOMXRendererEventEndOfStream;
            CLog::Log(LOGDEBUG, "OMXRENDER Event: OMX_EventBufferFlag Port: %lu nFlags: %lx\n", Data1, Data2);
            break;
        case OMX_EventResourcesAcquired:
            CLog::Log(LOGDEBUG, "OMXRENDER Event: OMX_EventResourcesAcquired\n");
            break;
        case OMX_EventComponentResumed:
            CLog::Log(LOGDEBUG, "OMXRENDER Event: OMX_EventComponentResumed\n");
            break;
        case OMX_EventDynamicResourcesAvailable:
            CLog::Log(LOGDEBUG, "OMXRENDER Event: OMX_EventDynamicResourcesAvailable\n");
            break;
        case OMX_EventPortFormatDetected:
            CLog::Log(LOGDEBUG, "OMXRENDER Event: OMX_EventPortFormatDetected\n");
            break;
        case NVX_EventFirstFrameDisplayed:
            CLog::Log(LOGDEBUG, "OMXRENDER Event: NVX_EventFirstFrameDisplayed\n");
            eOmxEvent = eOMXRendererEventFirstFrameDisplayed;
            break;
        default:
            CLog::Log(LOGDEBUG, "OMXRENDER Event: Unknown\n");
            break;
    }

    ((OmxRendererState *)pAppData)->pCallBacks->OnOmxRendererEventCompletion(((OmxRendererState *)pAppData)->pClientThis, (OmxRendererState *)pAppData, eOmxEvent, Data1, Data2);
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE OmxRendererEmptyBufferDone(
    OMX_OUT OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_PTR pAppData,
    OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
{
    CLog::Log(LOGDEBUG, "OMXRENDER OmxRendererEmptyBufferDone appdata:%p client:%p callbacks:%p callback:%p buff:%p databuff:%p comp:%p\n", 
        pAppData, 
        ((OmxRendererState *)pAppData)->pClientThis, 
        ((OmxRendererState *)pAppData)->pCallBacks,
        ((OmxRendererState *)pAppData)->pCallBacks->OnOmxRendererEmptyBufferDone,
        pBuffer,
        pBuffer->pBuffer,
        hComponent);

    ((OmxRendererState *)pAppData)->pCallBacks->OnOmxRendererEmptyBufferDone(((OmxRendererState *)pAppData)->pClientThis, pBuffer);
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE OmxRendererFillBufferDone(
    OMX_OUT OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_PTR pAppData,
    OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
{
    CLog::Log(LOGDEBUG, "OMXRENDER OmxRendererFillBufferDone\n");

    return OMX_ErrorNone;
}

void CLinuxRendererOMX::OnOmxRendererEmptyBufferDone(void *pClientThis, OMX_BUFFERHEADERTYPE *pBuffer)
{
    static unsigned int k;
    CLog::Log(LOGDEBUG, "OMXRENDER CLinuxRendererOMX::OnOmxRendererEmptyBufferDone start\n");
    if(!pBuffer) {
        CLog::Log(LOGDEBUG, "OMXRENDER CLinuxRendererOMX::OnOmxRendererEmptyBufferDone (%u) pBuffer NULL\n", k++);
        return;
    }
    CLinuxRendererOMX *pThis = reinterpret_cast<CLinuxRendererOMX*>(pClientThis);

    /* Signal Empty Buffer Done */
    pThis->m_oEventOmxEmptyBufferDone.Set();

    CLog::Log(LOGDEBUG, "OMXRENDER OnOmxRendererEmptyBufferDone (%u) pAppPrivate: %p\n", k++, pBuffer->pAppPrivate);
}

void CLinuxRendererOMX::OnOmxRendererEventCompletion(void *pClientThis, OmxRendererState *pOmxRendererState, EOMXRendererEvent eOmxEvent, unsigned int uData1, unsigned int uData2)
{
    CLinuxRendererOMX *pThis = reinterpret_cast<CLinuxRendererOMX*>(pClientThis);
    OMX_VERSIONTYPE omxVersion;
    OMX_PARAM_PORTDEFINITIONTYPE oRendererInputPortDef;
    OMX_ERRORTYPE err;
    CLog::Log(LOGDEBUG, "OMXRENDER OnOmxRendererEventCompletion Event: %d\n", eOmxEvent);
    switch(eOmxEvent) {
        case eOMXRendererEventStateChanged:
            pThis->m_oEventOmxSetStateDone.Set();
            break;
        case eOMXRendererEventFlush:
            CLog::Log(LOGDEBUG, "OMXRENDER OnOmxRendererEventCompletion eOMXEventFlush: %d\n", eOmxEvent);
            break;
        case eOMXRendererEventEndOfStream:
            break;
        case eOMXRendererEventPortSettingsChanged:
            // Required OMX version
            omxVersion.s.nVersionMajor = 1;
            omxVersion.s.nVersionMinor = 1;
            omxVersion.s.nRevision = 2;
            omxVersion.s.nStep = 0;
            memset((void *)&oRendererInputPortDef, sizeof(oRendererInputPortDef), 0);
            oRendererInputPortDef.nSize = sizeof(oRendererInputPortDef);
            oRendererInputPortDef.nVersion.nVersion = omxVersion.nVersion;
            oRendererInputPortDef.nPortIndex = 0;
            err = OMX_GetParameter(pThis->m_hComponentHandle, OMX_IndexParamPortDefinition, &oRendererInputPortDef);
            if(err != OMX_ErrorNone) {
                CLog::Log(LOGDEBUG, "OMXRENDER OMX_GetParameter() failed = %X\n", err);
                return;
            }
            break;
        case eOMXRendererEventPortDisable:
            CLog::Log(LOGDEBUG, "OMXRENDER OnOmxRendererEventCompletion eOMXRendererEventPortDisable: %d\n", eOmxEvent);
            pThis->m_oEventOmxSetStateDone.Set();
            break;
        case eOMXRendererEventFirstFrameDisplayed:
           pThis->m_oEventOmxEmptyBufferDone.Set();
            break;
        case eOMXRendererEventUndefined:
            break;
    }
}

CLinuxRendererOMX::CLinuxRendererOMX()
{
  for (int i = 0; i < NUM_BUFFERS; i++)
  {
    m_eventTexturesDone[i] = CreateEvent(NULL,FALSE,TRUE,NULL);
    m_RenderBufferQueue[i] = -1;
  }

  m_iYV12RenderBuffer = 0;
  m_flipindex = 0;
  m_currentField = FIELD_FULL;

  m_upscalingWidth = 0;
  m_upscalingHeight = 0;
  memset(&m_imScaled, 0, sizeof(m_imScaled));

  memset(m_buffers, 0, sizeof(m_buffers));

  CLog::Log(LOGDEBUG, "OMXRENDER CLinuxRendererOMX()\n");

  m_bOmxInitialized = false;
  m_bOmxTunnelMode = false;
  m_hComponentHandle=NULL;

  m_uOmxRendererBufferCount = 0;
  m_uOmxRendererBufSize = 0;

}

CLinuxRendererOMX::~CLinuxRendererOMX()
{
  CLog::Log(LOGDEBUG, "OMXRENDER CLinuxRendererOMX::~CLinuxRendererOMX()\n");  
  UnInit();
  for (int i = 0; i < NUM_BUFFERS; i++)
    CloseHandle(m_eventTexturesDone[i]);

  for (int i=0; i<3; i++)
  {
    if (m_imScaled.plane[i])
    {
      delete [] m_imScaled.plane[i];
      m_imScaled.plane[i] = 0;
    }
  }
  
  CLog::Log(LOGDEBUG, "OMXRENDER ~CLinuxRendererOMX()\n");
}

void CLinuxRendererOMX::ManageTextures()
{
  CLog::Log(LOGDEBUG, "OMXRENDER CLinuxRendererOMX::~ManageTextures()\n");  
  m_NumYV12Buffers = 2;
  //m_iYV12RenderBuffer = 0;
  return;
}

bool CLinuxRendererOMX::ValidateRenderTarget()
{
  CLog::Log(LOGDEBUG, "OMXRENDER CLinuxRendererOMX::ValidateRenderTarget()\n");  
  if (!m_bValidated)
  {
    for (int i = 0 ; i < m_NumYV12Buffers ; i++)
    {
      CreateYV12Texture(i);
    }
    m_bValidated = true;
    return true;
  }
  return false;  
}

bool CLinuxRendererOMX::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{
  m_sourceWidth = width;
  m_sourceHeight = height;

  CLog::Log(LOGDEBUG, "OMXRENDER CLinuxRendererOMX::Configure( width:%d, height:%d, d_width:%d, d_height:%d, fps:%f, flags:0X%x )\n", width, height, d_width, d_height, fps, flags);
  // Save the flags.
  m_iFlags = flags;

  if(m_iFlags & CONF_FLAGS_OMX_TUNNELING) 
  {
    m_bOmxTunnelMode = true;
  }

  // Calculate the input frame aspect ratio.
  CalculateFrameAspectRatio(d_width, d_height);
  ChooseBestResolution(fps);
  SetViewMode(g_stSettings.m_currentVideoSettings.m_ViewMode);
  ManageDisplay();

  ChooseUpscalingMethod();

  m_bConfigured = true;
  m_bImageReady = false;

  // Ensure that textures are recreated and rendering starts only after the 1st 
  // frame is loaded after every call to Configure().
  m_bValidated = false;

  for (int i = 0 ; i<m_NumYV12Buffers ; i++)
  {
    m_RenderBufferQueue[i] = -1;
    m_buffers[i].image.flags = 0;
  }

  m_iLastRenderBuffer = -1;
  m_iLastReleasedBuffer = -1;

  char * pEnv = getenv("NVOMX_RENDER_APP_THREAD");  
  if (pEnv != NULL && !memcmp(pEnv,"1",1))
  {
    m_bRenderFromAppThread = true;
  }

  /* Initialize the OMX renderer */

  if (!m_bOmxInitialized) 
  {
    if(!m_bOmxTunnelMode)
    {

      CLog::Log(LOGDEBUG, "OMXRENDER OMX Init\n");
      OMX_ERRORTYPE err = OMX_Init();
      if(err != OMX_ErrorNone) {
        CLog::Log(LOGDEBUG, "OMXRENDER OMX_Init() failed = %X\n", err);
        return false;
      }
  
      // Setup callbacks        
      m_OmxrendererCallBacks.OnOmxRendererEmptyBufferDone = OnOmxRendererEmptyBufferDone;
      m_OmxrendererCallBacks.OnOmxRendererFillBufferDone = NULL;
      m_OmxrendererCallBacks.OnOmxRendererEventCompletion = OnOmxRendererEventCompletion;
  
      m_OmxRendererState.pClientThis = (void *)this;
      m_OmxRendererState.pCallBacks = &m_OmxrendererCallBacks;
  
      // Required OMX version

      CLog::Log(LOGDEBUG, "OMXRENDER OMX_GetHandle()\n");
  
      // Open component, 
      char * pDispOverride = getenv("NVKD_DISP");  // TODO, remove this HACK!! for now to make it easy to sitch between VGA and HDMI overlays without recompiling the binary
      if (pDispOverride != NULL && !memcmp(pDispOverride,"Tegra:VGA0",10))
        err = OMX_GetHandle(&m_hComponentHandle, (char *)pszOmxYuvOverlayComponentsName[1], (OMX_PTR)&m_OmxRendererState, &OmxRendererCallbacks);
      else
        err = OMX_GetHandle(&m_hComponentHandle, (char *)pszOmxYuvOverlayComponentsName[0], (OMX_PTR)&m_OmxRendererState, &OmxRendererCallbacks);
      if(err != OMX_ErrorNone) {
          CLog::Log(LOGDEBUG, "OMXRENDER OMX_GetHandle() failed = %X\n", err);
          return false;
      }
      
      SetProfileMode(m_hComponentHandle,1);
          
  #if 0
      OMX_CONFIG_POINTTYPE oPoint;
      OMX_FRAMESIZETYPE oSize;
  
      INIT_PARAM(oPoint);
      oPoint.nPortIndex = 0;
      oPoint.nX = 0;
      oPoint.nY = 0;
  
      err = OMX_GetConfig(m_hComponentHandle, OMX_IndexConfigCommonOutputPosition, &oPoint);
      if(err != OMX_ErrorNone)
      {
          CLog::Log(LOGDEBUG, "OMXRENDER OMX_GetConfig() OMX_IndexConfigCommonOutputPosition failed = %X\n", err);
          return false;
      }
      CLog::Log(LOGDEBUG, "OMXRENDER OMX_GetParameter() OMX_IndexConfigCommonOutputPosition X:%d Y:%d\n", oPoint.nX, oPoint.nY);
  #if 1
      err = OMX_SetConfig(m_hComponentHandle, OMX_IndexConfigCommonOutputPosition, &oPoint);
      if(err != OMX_ErrorNone)
      {
          CLog::Log(LOGDEBUG, "OMXRENDER OMX_SetConfig() OMX_IndexConfigCommonOutputPosition failed = %X\n", err);
          return false;
      }
  #endif
      INIT_PARAM(oSize);
      oSize.nPortIndex = 0;
  
      err = OMX_GetConfig(m_hComponentHandle, OMX_IndexConfigCommonOutputSize, &oSize);
      if(err != OMX_ErrorNone)
      {
          CLog::Log(LOGDEBUG, "OMXRENDER OMX_GetConfig() OMX_IndexConfigCommonOutputSize failed = %X\n", err);
          return false;
      }
      CLog::Log(LOGDEBUG, "OMXRENDER OMX_GetParameter() OMX_IndexConfigCommonOutputSize Width:%d Height:%d\n", oSize.nWidth, oSize.nHeight);
  #if 1
      oSize.nWidth = m_sourceWidth;
      oSize.nHeight = m_sourceHeight;
      err = OMX_SetConfig(m_hComponentHandle, OMX_IndexConfigCommonOutputSize, &oSize);
      if(err != OMX_ErrorNone)
      {
          CLog::Log(LOGDEBUG, "OMXRENDER OMX_SetConfig() OMX_IndexConfigCommonOutputSize failed = %X\n", err);
          return false;
      }
      err = OMX_GetConfig(m_hComponentHandle, OMX_IndexConfigCommonOutputSize, &oSize);
      if(err != OMX_ErrorNone)
      {
          CLog::Log(LOGDEBUG, "OMXRENDER OMX_GetConfig() OMX_IndexConfigCommonOutputSize failed = %X\n", err);
          return false;
      }
      CLog::Log(LOGDEBUG, "OMXRENDER OMX_GetParameter() after set OMX_IndexConfigCommonOutputSize Width:%d Height:%d\n", oSize.nWidth, oSize.nHeight);
  #endif
  #endif
                                  
      CLog::Log(LOGDEBUG, "OMXRENDER OMX_CommandStateSet");
  
      // Move to Idle state
      err = OMX_SendCommand(m_hComponentHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
      if(err != OMX_ErrorNone) {
          CLog::Log(LOGDEBUG, "OMXRENDER OMX_SendCommand() failed = %X\n", err);
          return false;
      }
  
      CLog::Log(LOGDEBUG, "OMXRENDER OMX_GetParameter");
  
      OMX_PARAM_PORTDEFINITIONTYPE oYuvOverlayInputPortDef;
      memset((void *)&oYuvOverlayInputPortDef, sizeof(oYuvOverlayInputPortDef), 0);
      oYuvOverlayInputPortDef.nSize = sizeof(oYuvOverlayInputPortDef);
      oYuvOverlayInputPortDef.nVersion = m_vOMX;
      oYuvOverlayInputPortDef.nPortIndex = 0;
      err = OMX_GetParameter(m_hComponentHandle, OMX_IndexParamPortDefinition, &oYuvOverlayInputPortDef);
      if(err != OMX_ErrorNone)
      {
          CLog::Log(LOGDEBUG, "OMXRENDER OMX_GetParameter() failed = %X\n", err);
          return false;
      }
  
      /* Set the renderer width and height parameters */
      /* TODO :: Need to set the stride for the surface */
      CLog::Log(LOGDEBUG, "OMXRENDER Setting source width %d and height %d\n", m_sourceHeight, m_sourceWidth); 
      oYuvOverlayInputPortDef.format.video.nFrameHeight = m_sourceHeight; 
      oYuvOverlayInputPortDef.format.video.nFrameWidth = m_sourceWidth; 
    
      err = OMX_SetParameter(m_hComponentHandle, OMX_IndexParamPortDefinition, &oYuvOverlayInputPortDef);
      if(err != OMX_ErrorNone)
      {
          CLog::Log(LOGDEBUG, "OMXRENDER OMX_SetParameter() failed = %X\n", err);
          return false;
      }
  
      err = OMX_GetParameter(m_hComponentHandle, OMX_IndexParamPortDefinition, &oYuvOverlayInputPortDef);
      if(err != OMX_ErrorNone)
      {
          CLog::Log(LOGDEBUG, "OMXRENDER OMX_GetParameter() failed = %X\n", err);
          return false;
      }
  
      
      // Buffer Count here must be same as the oDecOutputPortDef.nBufferCountMin and 
      // oDecOutputPortDef.nBufferSize
      m_uOmxRendererBufferCount = oYuvOverlayInputPortDef.nBufferCountMin;
      // Set the size of the overlay buffer to be 1.5 bytes * height * width (this may need to change to use the surface stride) for YV12
      oYuvOverlayInputPortDef.nBufferSize = (m_sourceHeight * m_sourceWidth * 3) / 2; // 1.5 bytes per pixel in YV12
      m_uOmxRendererBufSize = oYuvOverlayInputPortDef.nBufferSize;
  
      CLog::Log(LOGDEBUG, "OMXRENDER oYuvOverlayInputPortDef.nBufferCountMin: %u MinBuffer size: %u\n", 
                                m_uOmxRendererBufferCount, m_uOmxRendererBufSize);
  
      CLog::Log(LOGDEBUG, "OMXRENDER OMX_UseBuffer\n");
      for(unsigned int i = 0; i < m_uOmxRendererBufferCount; i++) {
          err = OMX_AllocateBuffer(m_hComponentHandle, &m_pOmxRendererBuffers[i], 0, NULL, m_uOmxRendererBufSize);
          if(err != OMX_ErrorNone) {
              CLog::Log(LOGDEBUG, "OMXRENDER OMX_UseBuffer() seq:%u failed = %X\n", i, err);
              return false;
          }
      }
  
      // Wait for Idle State notification
      CLog::Log(LOGDEBUG, "OMXRENDER Waiting for component to go IDLE\n"); 
      m_oEventOmxSetStateDone.Wait();
  
      
      err = OMX_SendCommand(m_hComponentHandle, OMX_CommandPortDisable, 1, 0);
      if(err != OMX_ErrorNone) {
          CLog::Log(LOGDEBUG, "OMXRENDER OMX_SendCommand() OMX_CommandPortDisable failed = %X\n", err);
          return false;
      }
  
      // Wait for OMX_CommandPortDisable notification
      CLog::Log(LOGDEBUG, "OMXRENDER Waiting for OMX_CommandPortDisable notification\n"); 
      m_oEventOmxSetStateDone.Wait();
  
      
      CLog::Log(LOGDEBUG, "OMXRENDER Sending Command to Execute\n");   	
  
      err = OMX_SendCommand(m_hComponentHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
      if(err != OMX_ErrorNone) {
          CLog::Log(LOGDEBUG, "OMXRENDER OMX_SendCommand() failed = %X\n", err);
          return false;
      }
  
      CLog::Log(LOGDEBUG, "OMXRENDER Wait for Execute State\n");   	
  
      // Wait for Executing State
      m_oEventOmxSetStateDone.Wait();
      /* We should rely on the number of buffer needed by the OMX render component */
      m_NumYV12Buffers = m_uOmxRendererBufferCount;
      
    }
    m_bOmxInitialized = true;    

  }
  else if(m_bOmxTunnelMode )
  {
    for(int i=0; i<NUM_BUFFERS; i++)
      for(int k=0; i<3; k++)
        m_buffers[i].image.plane[k] = 0;
  }

  CLog::Log(LOGDEBUG, "OMXRENDER CLinuxRendererOMX::Configure() m_NumYV12Buffers = %d\n", m_NumYV12Buffers);  

  return true;
}

void CLinuxRendererOMX::ChooseUpscalingMethod()
{
  CLog::Log(LOGDEBUG, "OMXRENDER CLinuxRendererOMX::ChooseUpscalingMethod()\n");  
  m_upscalingWidth  = m_destRect.Width();
  m_upscalingHeight = m_destRect.Height();
}

int CLinuxRendererOMX::NextYV12Texture()
{
  CLog::Log(LOGDEBUG, "OMXRENDER CLinuxRendererOMX::NextYV12Texture() m_iYV12RenderBuffer:%d m_NumYV12Buffers:%d\n", m_iYV12RenderBuffer, m_NumYV12Buffers);  
  return (m_iYV12RenderBuffer + 1) % m_NumYV12Buffers;
}

int CLinuxRendererOMX::GetImage(YV12Image *image, int source, bool readonly)
{
  CLog::Log(LOGDEBUG, "OMXRENDER CLinuxRendererOMX::GetImage()\n");  
  if (!image) return -1;
  if (!m_bValidated) return -1;

  CLog::Log(LOGDEBUG, "<< CLinuxRenderer::GetImage source:%d readonly:%d", source, readonly);

  /* take next available buffer */
  if( source == AUTOSOURCE )
    source = NextYV12Texture();

  CLog::Log(LOGDEBUG, "OMXRENDER CLinuxRendererOMX::GetImage(a) source:%d\n", source);      
  YV12Image &im = m_buffers[source].image;
  if (!im.plane[0])
  {
    CLog::Log(LOGDEBUG, "CLinuxRenderer::GetImage - image planes not allocated");
    return -1;
  }

  CLog::Log(LOGDEBUG, "OMXRENDER CLinuxRendererOMX::GetImage(b)\n");        
  
  if ((im.flags&(~IMAGE_FLAG_READY)) != 0)
  {
    CLog::Log(LOGDEBUG, "CLinuxRenderer::GetImage - request image but none to give");
    return -1;
  }

  if(m_bRenderFromAppThread)
  {  
    if(im.flags)
    {
      CLog::Log(LOGDEBUG, "CLinuxRenderer::GetImage() - request buffer %d but still in use with flags = 0x%x", source, im.flags);
      return -1;
    }
  }

    
  if( readonly )
    im.flags |= IMAGE_FLAG_READING;
  else 
  {
    if(!m_bOmxTunnelMode)
    {
      if( WaitForSingleObject(m_eventTexturesDone[source], 500) == WAIT_TIMEOUT )
        CLog::Log(LOGWARNING, "%s - Timeout waiting for texture %d", __FUNCTION__, source);
    }
    // If we're using RenderUpdate to flip ,
    // return -1 to Ouptput picture so he can decide to drop this frame to keep A/V synch
    im.flags |= IMAGE_FLAG_WRITING;
  }

    
  CLog::Log(LOGDEBUG, "OMXRENDER CLinuxRendererOMX::GetImage(c)\n");      
  // copy the image - should be operator of YV12Image
  for (int p=0;p<MAX_PLANES;p++)
  {
    image->plane[p]  = im.plane[p];
    image->stride[p] = im.stride[p];
  }
  image->width    = im.width;
  image->height   = im.height;
  image->flags    = im.flags;
  image->cshift_x = im.cshift_x;
  image->cshift_y = im.cshift_y;

  CLog::Log(LOGDEBUG, "CLinuxRenderer::GetImage  %d source >> ", source);
  return source;
}

void CLinuxRendererOMX::ReleaseImage(int source, bool preserve)
{
  YV12Image &im = m_buffers[source].image;

  CLog::Log(LOGDEBUG, "OMXRENDER CLinuxRendererOMX::ReleaseImage()\n");  
  CLog::Log(LOGDEBUG, "<< CLinuxRenderer::ReleaseImage  %d source ", source);

  
  im.flags &= ~IMAGE_FLAG_INUSE;
  im.flags |= IMAGE_FLAG_READY;
  /* if image should be preserved reserve it so it's not auto seleceted */

  if( preserve )
    im.flags |= IMAGE_FLAG_RESERVED;

  m_iLastReleasedBuffer = source;
  m_bImageReady = true;

  CLog::Log(LOGDEBUG, "CLinuxRenderer::ReleaseImage >> ");
}

void CLinuxRendererOMX::LoadTextures(int source)
{
  YUVBUFFER& buf    =  m_buffers[source];
  YV12Image* im     = &buf.image;

  if (!(im->flags&IMAGE_FLAG_READY))
  {
    CLog::Log(LOGDEBUG, "     OMXRENDER CLinuxRendererOMX::LoadTextures(A) source:%d, im->flags:0x%x\n", source, im->flags);
    SetEvent(m_eventTexturesDone[source]);
    return;
  }

  bool deinterlacing;
  if (m_currentField == FIELD_FULL)
    deinterlacing = false;
  else
    deinterlacing = true;
  
  
  if(m_bRenderFromAppThread)
  {
    if(!m_bOmxTunnelMode) 
    {
      OMX_BUFFERHEADERTYPE *pOmxBuffer = m_buffers[source].pOmxBuffer;
    
      CLog::Log(LOGDEBUG, "CLinuxRendererOMX::FlipPage buff:%p pBuffer:%p", pOmxBuffer, pOmxBuffer->pBuffer);
    
    #if 0
      unsigned char *pB = pOmxBuffer->pBuffer;
      printf("Buffer Data: ");
      for(int i = 0; i < 32; i++)
          printf("%02X ", pB[i]);
      printf("\n");
    #endif
      pOmxBuffer->nFilledLen = m_uOmxRendererBufSize;
      pOmxBuffer->nOffset = 0;
      OMX_ERRORTYPE err = OMX_EmptyThisBuffer(m_hComponentHandle, pOmxBuffer);
      if(err != OMX_ErrorNone)
      {
          CLog::Log(LOGERROR, "OMX_EmptyThisBuffer() failed = %X\n", err);
          return; 
      }
      // Wait for Empty Buffer is completed 
      m_oEventOmxEmptyBufferDone.Wait();
      CLog::Log(LOGDEBUG, "     OMXRENDER CLinuxRendererOMX::LoadTextures(B) source:%d, im->flags:0x%x queue[%d,%d,%d]\n", source, im->flags,
                              m_RenderBufferQueue[0], m_RenderBufferQueue[1], m_RenderBufferQueue[2]);
  
      //clear the flags and advance the queue before setting the done event
      im->flags = 0;
      for(int i=0; i < m_NumYV12Buffers-1; i++)
        m_RenderBufferQueue[i] = m_RenderBufferQueue[i+1];
  
      m_RenderBufferQueue[m_NumYV12Buffers-1] = -1;
    }
  }

  SetEvent(m_eventTexturesDone[source]);
}

void CLinuxRendererOMX::Reset()
{ 
  CLog::Log(LOGDEBUG, "<< CLinuxRendererOMX::Reset");
  for(int i=0; i<m_NumYV12Buffers; i++)
  {
    /* reset all image flags, this will cleanup textures later */
    m_buffers[i].image.flags = 0;
    /* reset texture locks, a bit ugly, could result in tearing */
    SetEvent(m_eventTexturesDone[i]);
  }
  CLog::Log(LOGDEBUG, "CLinuxRendererOMX::Reset >>");
}

void CLinuxRendererOMX::Update(bool bPauseDrawing)
{
  if (!m_bConfigured) return;
  ManageDisplay();
  ManageTextures();
}

void CLinuxRendererOMX::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  CLog::Log(LOGDEBUG, "<< CLinuxRendererOMX::RenderUpdate");
  if (!m_bConfigured) return;

  // if its first pass, just init textures and return
  if (ValidateRenderTarget())
    return;

  // this needs to be checked after texture validation
  if (!m_bImageReady) return;

  ManageDisplay();
  ManageTextures();
  
  // grab the index from the front of the queue
  int index;
  if(m_bRenderFromAppThread)
  {
    index = m_RenderBufferQueue[0];
    if(index < 0) return;
  }
  else
  {
    // If we're flipping from the decoder there is no need go proceed further
    return;
  }

  m_iLastRenderBuffer = index;
  Render(flags, index);

  CLog::Log(LOGDEBUG, "CLinuxRendererOMX::RenderUpdate >>");
}

void CLinuxRendererOMX::FlipPage(int source)
{
  if(m_bRenderFromAppThread)
  {
    if(m_iLastReleasedBuffer > -1)
    {
      for(int i=0; i < m_NumYV12Buffers; i++)
      {
        if (m_RenderBufferQueue[i] < 0)
        {
          m_RenderBufferQueue[i] = m_iLastReleasedBuffer;
          CLog::Log(LOGDEBUG, "CLinuxRendererOMX::FlipPage() LastReleasedBuffer:%d, queue_index:%d", m_iLastReleasedBuffer, i);
          break;
        }
      }
    }
  }
  else
  {
    if(!m_bOmxTunnelMode)
    {
      OMX_BUFFERHEADERTYPE *pOmxBuffer = m_buffers[m_iLastReleasedBuffer].pOmxBuffer;
      CLog::Log(LOGDEBUG, "CLinuxRendererOMX::FlipPage buff:%p pBuffer:%p", pOmxBuffer, pOmxBuffer->pBuffer);
  
      #if 0
        unsigned char *pB = pOmxBuffer->pBuffer;
        printf("Buffer Data: ");
        for(int i = 0; i < 32; i++)
            printf("%02X ", pB[i]);
        printf("\n");
      #endif
      pOmxBuffer->nFilledLen = m_uOmxRendererBufSize;
      pOmxBuffer->nOffset = 0;
      OMX_ERRORTYPE err = OMX_EmptyThisBuffer(m_hComponentHandle, pOmxBuffer);
      if(err != OMX_ErrorNone)
      {
          CLog::Log(LOGERROR, "OMX_EmptyThisBuffer() failed = %X\n", err);
          return; 
      }
      // Wait for Empty Buffer is completed 
      m_oEventOmxEmptyBufferDone.Wait();
  
      SetEvent(m_eventTexturesDone[m_iLastReleasedBuffer]);
  
      CLog::Log(LOGDEBUG, "CLinuxRendererOMX::FlipPage() empty_buffer:%d, im.flags:0x%x  >>", m_iLastReleasedBuffer, m_buffers[m_iLastReleasedBuffer].image.flags);
    }
  }

  CLog::Log(LOGDEBUG, "<< CLinuxRendererOMX::FlipPage %d source", source);
  if( source >= 0 && source < m_NumYV12Buffers )
    m_iYV12RenderBuffer = source;
  else
    m_iYV12RenderBuffer = NextYV12Texture();

  CLog::Log(LOGDEBUG, "CLinuxRendererOMX::FlipPage %d m_iYV12RenderBuffer", m_iYV12RenderBuffer);
  m_buffers[m_iYV12RenderBuffer].flipindex = ++m_flipindex;

  return;
}


unsigned int CLinuxRendererOMX::DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y)
{
  BYTE *s;
  BYTE *d;
  int i, p;

  CLog::Log(LOGDEBUG, "<< CLinuxRendererOMX::DrawSlice");

  int index = NextYV12Texture();
  if( index < 0 )
    return -1;

  YV12Image &im = m_buffers[index].image;
  // copy Y
  p = 0;
  d = (BYTE*)im.plane[p] + im.stride[p] * y + x;
  s = src[p];
  for (i = 0;i < h;i++)
  {
    memcpy(d, s, w);
    s += stride[p];
    d += im.stride[p];
  }

  w >>= im.cshift_x; h >>= im.cshift_y;
  x >>= im.cshift_x; y >>= im.cshift_y;

  // copy U
  p = 1;
  d = (BYTE*)im.plane[p] + im.stride[p] * y + x;
  s = src[p];
  for (i = 0;i < h;i++)
  {
    memcpy(d, s, w);
    s += stride[p];
    d += im.stride[p];
  }

  // copy V
  p = 2;
  d = (BYTE*)im.plane[p] + im.stride[p] * y + x;
  s = src[p];
  for (i = 0;i < h;i++)
  {
    memcpy(d, s, w);
    s += stride[p];
    d += im.stride[p];
  }

  SetEvent(m_eventTexturesDone[index]);
  CLog::Log(LOGDEBUG, "CLinuxRendererOMX::DrawSlice >>");
  return 0;
}

unsigned int CLinuxRendererOMX::PreInit()
{
  CSingleLock lock(g_graphicsContext);

  CLog::Log(LOGDEBUG, "OMXRENDER CLinuxRendererOMX::PreInit()\n");

  m_bConfigured = false;
  m_bValidated = false;
  UnInit();
  m_resolution = RES_PAL_4x3;
  m_bRenderFromAppThread = false;

  m_iYV12RenderBuffer = 0;
  m_NumYV12Buffers = 2;

  m_vOMX.s.nVersionMajor = 1;
  m_vOMX.s.nVersionMinor = 1;
  m_vOMX.s.nRevision = 0;//2;
  m_vOMX.s.nStep = 0;

  // setup the background colour
  m_clearColour = (float)(g_advancedSettings.m_videoBlackBarColour & 0xff) / 0xff;
  m_aspecterror = g_guiSettings.GetFloat("videoplayer.aspecterror") * 0.01;

  CLog::Log(LOGDEBUG, "OMXRENDER PreInit DONE\n");

  return true;
}

void CLinuxRendererOMX::UnInit()
{
  CLog::Log(LOGDEBUG, "LinuxRendererGL: Cleaning up GL resources");
  CSingleLock lock(g_graphicsContext);

  CLog::Log(LOGDEBUG,"OMXRENDER OMXRENDER UnInit()");

  if(m_bOmxInitialized)
  {
    if(!m_bOmxTunnelMode)
    {
      OMX_ERRORTYPE err = OMX_SendCommand(m_hComponentHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
      if(err != OMX_ErrorNone) {
          CLog::Log(LOGERROR, "OMXRENDER OMX_SendCommand() failed = %X\n", err);
          return;
      }
    
      // Wait for Idle State notification
      m_oEventOmxSetStateDone.Wait();
    
      err = OMX_SendCommand(m_hComponentHandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
      if(err != OMX_ErrorNone) {
          CLog::Log(LOGERROR, "OMXRENDER OMX_SendCommand() failed = %X\n", err);
          return;
      }
    
      for(unsigned int i = 0; i < m_uOmxRendererBufferCount; i++) {
          err = OMX_FreeBuffer(m_hComponentHandle, 0, m_pOmxRendererBuffers[i]);
          if(err != OMX_ErrorNone) {
            CLog::Log(LOGERROR, "OMXRENDER OMX_FreeBuffer() seq:%u failed = %X\n", i, err);
            return;
          }
      }
    
      // Wait for Loaded State notification
      m_oEventOmxSetStateDone.Wait();
    
      err = OMX_FreeHandle(m_hComponentHandle);
      if(err != OMX_ErrorNone) {
          CLog::Log(LOGERROR, "OMXRENDER OMX_SendCommand() failed = %X\n", err);
          return;
      }
    
      err = OMX_Deinit();
      if(err != OMX_ErrorNone) {
          CLog::Log(LOGDEBUG, "OMXRENDER OMX_Deinit() failed = %X\n", err);
          return;
      }
    }
    m_bOmxInitialized = false;
  }

  // YV12 textures
  for (int i = 0; i < NUM_BUFFERS; ++i)
  {
    m_RenderBufferQueue[i] = -1;
    DeleteYV12Texture(i);
  }

  // cleanup framebuffer object if it was in use
  m_bValidated = false;
  m_bImageReady = false;
  m_bConfigured = false;
  return;
}

void CLinuxRendererOMX::Render(DWORD flags, int renderBuffer)
{
  CLog::Log(LOGDEBUG, "OMXRENDER CLinuxRendererOMX::Render( %d, %d )\n", flags, renderBuffer);
  // obtain current field, if interlaced
  if( flags & RENDER_FLAG_ODD)
    m_currentField = FIELD_ODD;

  else if (flags & RENDER_FLAG_EVEN)
    m_currentField = FIELD_EVEN;

  else if (flags & RENDER_FLAG_LAST)
  {
    switch(m_currentField)
    {
    case FIELD_ODD:
      flags = RENDER_FLAG_ODD;
      break;

    case FIELD_EVEN:
      flags = RENDER_FLAG_EVEN;
      break;
    }
  }
  else
    m_currentField = FIELD_FULL;

  LoadTextures(renderBuffer);
  RenderSinglePass(renderBuffer, m_currentField);
}

void CLinuxRendererOMX::AutoCrop(bool bCrop)
{
  RECT crop;
   
  CLog::Log(LOGDEBUG, "OMXRENDER CLinuxRendererOMX::AutoCrop( %d)\n", bCrop);
  
  if(!m_bValidated) return;

  if (bCrop)
  {
    YV12Image &im = m_buffers[m_iYV12RenderBuffer].image;
    crop.left   = g_stSettings.m_currentVideoSettings.m_CropLeft;
    crop.right  = g_stSettings.m_currentVideoSettings.m_CropRight;
    crop.top    = g_stSettings.m_currentVideoSettings.m_CropTop;
    crop.bottom = g_stSettings.m_currentVideoSettings.m_CropBottom;

    int black  = 16; // what is black in the image
    int level  = 8;  // how high above this should we detect
    int multi  = 4;  // what multiple of last line should failing line be to accept
    BYTE *s;
    int last, detect, black2;

    // top and bottom levels
    black2 = black * im.width;
    detect = level * im.width + black2;

    // Crop top
    s      = im.plane[0];
    last   = black2;
    for (unsigned int y = 0; y < im.height/2; y++)
    {
      int total = 0;
      for (unsigned int x = 0; x < im.width; x++)
        total += s[x];
      s += im.stride[0];

      if (total > detect)
      {
        if (total - black2 > (last - black2) * multi)
          crop.top = y;
        break;
      }
      last = total;
    }

    // Crop bottom
    s    = im.plane[0] + (im.height-1)*im.stride[0];
    last = black2;
    for (unsigned int y = (int)im.height; y > im.height/2; y--)
    {
      int total = 0;
      for (unsigned int x = 0; x < im.width; x++)
        total += s[x];
      s -= im.stride[0];

      if (total > detect)
      {
        if (total - black2 > (last - black2) * multi)
          crop.bottom = im.height - y;
        break;
      }
      last = total;
    }

    // left and right levels
    black2 = black * im.height;
    detect = level * im.height + black2;


    // Crop left
    s    = im.plane[0];
    last = black2;
    for (unsigned int x = 0; x < im.width/2; x++)
    {
      int total = 0;
      for (unsigned int y = 0; y < im.height; y++)
        total += s[y * im.stride[0]];
      s++;
      if (total > detect)
      {
        if (total - black2 > (last - black2) * multi)
          crop.left = x;
        break;
      }
      last = total;
    }

    // Crop right
    s    = im.plane[0] + (im.width-1);
    last = black2;
    for (unsigned int x = (int)im.width-1; x > im.width/2; x--)
    {
      int total = 0;
      for (unsigned int y = 0; y < im.height; y++)
        total += s[y * im.stride[0]];
      s--;

      if (total > detect)
      {
        if (total - black2 > (last - black2) * multi)
          crop.right = im.width - x;
        break;
      }
      last = total;
    }

    // We always crop equally on each side to get zoom
    // effect intead of moving the image. Aslong as the
    // max crop isn't much larger than the min crop
    // use that.
    int min, max;

    min = std::min(crop.left, crop.right);
    max = std::max(crop.left, crop.right);
    if(10 * (max - min) / im.width < 1)
      crop.left = crop.right = max;
    else
      crop.left = crop.right = min;

    min = std::min(crop.top, crop.bottom);
    max = std::max(crop.top, crop.bottom);
    if(10 * (max - min) / im.height < 1)
      crop.top = crop.bottom = max;
    else
      crop.top = crop.bottom = min;
  }
  else
  { // reset to defaults
    crop.left   = 0;
    crop.right  = 0;
    crop.top    = 0;
    crop.bottom = 0;
  }

  m_crop.x1 += ((float)crop.left   - m_crop.x1) * 0.1;
  m_crop.x2 += ((float)crop.right  - m_crop.x2) * 0.1;
  m_crop.y1 += ((float)crop.top    - m_crop.y1) * 0.1;
  m_crop.y2 += ((float)crop.bottom - m_crop.y2) * 0.1;

  crop.left   = MathUtils::round_int(m_crop.x1);
  crop.right  = MathUtils::round_int(m_crop.x2);
  crop.top    = MathUtils::round_int(m_crop.y1);
  crop.bottom = MathUtils::round_int(m_crop.y2);

  //compare with hysteresis
# define HYST(n, o) ((n) > (o) || (n) + 1 < (o))
  if(HYST(g_stSettings.m_currentVideoSettings.m_CropLeft  , crop.left)
    || HYST(g_stSettings.m_currentVideoSettings.m_CropRight , crop.right)
    || HYST(g_stSettings.m_currentVideoSettings.m_CropTop   , crop.top)
    || HYST(g_stSettings.m_currentVideoSettings.m_CropBottom, crop.bottom))
  {
    g_stSettings.m_currentVideoSettings.m_CropLeft   = crop.left;
    g_stSettings.m_currentVideoSettings.m_CropRight  = crop.right;
    g_stSettings.m_currentVideoSettings.m_CropTop    = crop.top;
    g_stSettings.m_currentVideoSettings.m_CropBottom = crop.bottom;
    SetViewMode(g_stSettings.m_currentVideoSettings.m_ViewMode);
  }
# undef HYST
}

void CLinuxRendererOMX::RenderSinglePass(int index, int field)
{
}

void CLinuxRendererOMX::CreateThumbnail(CBaseTexture* texture, unsigned int width, unsigned int height)
{
}

//********************************************************************************************************
// YV12 Texture creation, deletion, copying + clearing
//********************************************************************************************************
void CLinuxRendererOMX::DeleteYV12Texture(int index)
{

  CLog::Log(LOGDEBUG, "OMXRENDER CLinuxRendererOMX::DeleteYV12TextureA()\n");  
  CLog::Log(LOGDEBUG, "Deleted YV12 texture %i", index);

  YV12Image &im     = m_buffers[index].image;

  if(m_bOmxTunnelMode)
  {
#if 1    
    for(int p = 0;p<MAX_PLANES;p++)
    {
      if (im.plane[p])
      {
        delete[] im.plane[p];
        im.plane[p] = NULL;
      }
    }
#endif    
  }
}

void CLinuxRendererOMX::ClearYV12Texture(int index)
{
}

bool CLinuxRendererOMX::CreateYV12Texture(int index, bool clear)
{
  YV12Image &im     = m_buffers[index].image;
   CLog::Log(LOGDEBUG, "OMXRENDER CLinuxRendererOMX::CreateYV12TextureA(index = %d, clear = %d)\n", index, clear);
  if (clear)
  {
    DeleteYV12Texture(index);

    im.height = m_sourceHeight;
    im.width  = m_sourceWidth;
    im.cshift_x = 1;
    im.cshift_y = 1;

    im.stride[0] = im.width;
    im.stride[1] = im.width >> im.cshift_x;
    im.stride[2] = im.width >> im.cshift_x;
    if(m_bOmxTunnelMode)
    {
      im.plane[0] = new BYTE[im.stride[0] * im.height];
      im.plane[1] = new BYTE[im.stride[1] * ( im.height >> im.cshift_y )];
      im.plane[2] = new BYTE[im.stride[2] * ( im.height >> im.cshift_y )];
    }
    else
    {
      im.plane[0] = m_pOmxRendererBuffers[index]->pBuffer ; 
      im.plane[1] = im.plane[0] + (m_sourceWidth * m_sourceHeight); 
      im.plane[2] = im.plane[1] + m_sourceWidth * m_sourceHeight / 4; 
    }
    m_buffers[index].pOmxBuffer = m_pOmxRendererBuffers[index]; 
  }

  CLog::Log(LOGDEBUG, "OMXRENDER CLinuxRendererOMX::CreateYV12Texture() im.plane[0] = %p \n", im.plane[0]);
  
  SetEvent(m_eventTexturesDone[index]);
  return true;
}

bool CLinuxRendererOMX::SupportsBrightness()
{
  return false;
}

bool CLinuxRendererOMX::SupportsContrast()
{
  return false;
}

bool CLinuxRendererOMX::SupportsGamma()
{
  return false;
}

bool CLinuxRendererOMX::SupportsMultiPassRendering()
{
  return false;
}

int CLinuxRendererOMX::SetProfileMode(OMX_HANDLETYPE hComp,int bFlip)
{

    CLog::Log(LOGDEBUG, "CLinuxRendererOMX::SetProfileMode() called with flip = %d\n", (int)bFlip);
    
    OMX_INDEXTYPE eIndexConfigProfile;
    NVX_CONFIG_PROFILE oProf;
    OMX_ERRORTYPE eError;

    INIT_PARAM(oProf);

    eError = OMX_GetExtensionIndex(hComp, NVX_INDEX_CONFIG_PROFILE,
                                   &eIndexConfigProfile);

    if(eError != OMX_ErrorNone)
    {
      CLog::Log(LOGDEBUG, "ERROR! OMX_GetExtensionIndex() failed with %x\n", eError);
      return eError;
    }
    
    oProf.nPortIndex = 0;
    oProf.bProfile = OMX_FALSE;
    oProf.bVerbose = OMX_TRUE;
    oProf.bStubOutput = OMX_TRUE;
    oProf.nForceLocale = 0;
    oProf.nNvMMProfile = 0;
    oProf.bNoAVSync = OMX_TRUE;
    oProf.nAVSyncOffset = 0;
    oProf.bFlip = (OMX_BOOL)bFlip;
    oProf.nFrameDrop = 0;
    oProf.bSanity = OMX_FALSE;
    eError = OMX_SetConfig(hComp, eIndexConfigProfile, &oProf);
    
    if(eError != OMX_ErrorNone)
    {
      CLog::Log(LOGDEBUG, "ERROR! OMX_SetConfig() failed with %x\n", eError);
      return eError;
    }

    return eError;
}

bool CLinuxRendererOMX::Supports(EINTERLACEMETHOD method)
{
  return false;
}

bool CLinuxRendererOMX::Supports(ESCALINGMETHOD method)
{
  return false;
}

#endif  //HAS_OPENMAX
