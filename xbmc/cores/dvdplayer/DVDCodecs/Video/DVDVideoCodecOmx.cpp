/*
 * Boxee
 * Some portions copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
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

#define USE_OMX_TUNNEL

#include "system.h"

#ifdef HAS_OPENMAX

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include "DVDVideoCodecOmx.h"
#include "DVDDemuxers/DVDDemux.h"
#include "DVDStreamInfo.h"
#include "DVDClock.h"
#include "DVDCodecs/DVDCodecs.h"
#include "../../../../utils/Win32Exception.h"
#if defined(_LINUX) || defined(_WIN32PC)
#include "utils/CPUInfo.h"
#endif
#include "AdvancedSettings.h"
#include "GUISettings.h"
#include "utils/log.h"

#ifndef _LINUX
#define RINT(x) ((x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5)))
#else
#include <math.h>
#define RINT lrint
#endif

#define INIT_PARAM(_X_)  (memset(&(_X_), 0, sizeof(_X_)), ((_X_).nSize = sizeof (_X_)), (_X_).nVersion = vOMX)

#include "cores/VideoRenderers/RenderManager.h"

#include <OMX_Core.h>
#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Audio.h>
#include <OMX_Video.h>
#include <NVOMX_IndexExtensions.h>

static OMX_ERRORTYPE OmxDecEventHandler(
    OMX_OUT OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_PTR pAppData,
    OMX_OUT OMX_EVENTTYPE eEvent,
    OMX_OUT OMX_U32 Data1,
    OMX_OUT OMX_U32 Data2,
    OMX_OUT OMX_PTR pEventData);
static OMX_ERRORTYPE OmxDecEmptyBufferDone(
    OMX_OUT OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_PTR pAppData,
    OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);
static OMX_ERRORTYPE OmxDecFillBufferDone(
    OMX_OUT OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_PTR pAppData,
    OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);

OMX_CALLBACKTYPE OmxVideoDecCallbacks = {
    OmxDecEventHandler,
    OmxDecEmptyBufferDone,
    OmxDecFillBufferDone
};

extern bool g_bFirstFrameDecoded;

CDVDVideoCodecOmx::CDVDVideoCodecOmx() : CDVDVideoCodec()
{
  m_iPictureWidth = 0;
  m_iPictureHeight = 0;
  m_iScreenWidth = 0;
  m_iScreenHeight = 0;

  m_bRunning = false;
  m_bAborting = false;
  m_bHasFrame = false;
  m_bMP4_h264_FileMode = false;
  m_bOmxInitialized = false;
  m_pExtraData = NULL;
  m_pPictureFrameData = NULL;
  m_uOmxInOutstandingBuffer = 0;
  m_uOmxAbortPortCount = 0;
  m_InBufferSemaphore = NULL;
  m_OutBufferSemaphore = NULL;
  m_VideoFreeQueue = NULL;
  m_VideoDecodedQueue = NULL;
  m_pLastSentOmxBuffer = NULL;
  m_pDataSave = NULL;
  m_bSendTunneledFirstFrame = false;
  m_bIsDirectRendering = true;
}

CDVDVideoCodecOmx::~CDVDVideoCodecOmx()
{
    Dispose();
    if(m_InBufferSemaphore) {
        delete m_InBufferSemaphore;
    }
    if(m_OutBufferSemaphore) {
        delete m_OutBufferSemaphore;
    }
    if(m_VideoFreeQueue) {
        delete m_VideoFreeQueue;
    }
    if(m_VideoDecodedQueue) {
        delete m_VideoDecodedQueue;
    }
}

bool CDVDVideoCodecOmx::ProcessExtraData(void *pData, unsigned int uSize)
{
    /* nothing to filter */
    if(!pData || uSize < 6) {
        return true;
    }

    BYTE *pOut = NULL;
    unsigned int total_size = 0;
    // Check 'avcC' signature
    if(*(BYTE *)pData == 0x01) {
        /* retrieve sps and pps NAL units from extradata */
        unsigned short unit_size;
        BYTE unit_nb, sps_done = 0, length_size;
        const BYTE *extradata = (BYTE *)pData + 4;
        static const BYTE nalu_header[4] = {0, 0, 0, 1};

        /* retrieve length coded size */
        length_size = (*extradata++ & 0x3) + 1;
        if(length_size == 3)
            return false;

        /* retrieve sps and pps unit(s) */
        unit_nb = *extradata++ & 0x1f; /* number of sps unit(s) */
        if(!unit_nb) {
            unit_nb = *extradata++; /* number of pps unit(s) */
            sps_done++;
        }

        while (unit_nb--) {
            unit_size = (*extradata << 8) | *(extradata + 1);
            total_size += unit_size+4;
            if(extradata + 2 + unit_size > (BYTE *)pData + uSize) {
                free(pOut);
                return false;
            }
            pOut = (BYTE *)realloc(pOut, total_size);
            if(!pOut)
                return false;
            memcpy(pOut + total_size - unit_size - 4, nalu_header, 4);
            memcpy(pOut + total_size - unit_size,     extradata + 2, unit_size);
            extradata += 2 + unit_size;

            if(!unit_nb && !sps_done++)
                unit_nb = *extradata++; /* number of pps unit(s) */
        }
    } else {
        // mpeg-4 so use the data as is
        pOut = (BYTE *)malloc(uSize);
        if(!pOut) {
            return false;
        }
        memcpy(pOut, pData, uSize);
        total_size = uSize;
    }

    m_pExtraData = pOut;
    m_uExtraData_Size = total_size;

    return true;
}

BYTE *CDVDVideoCodecOmx::GetNal(BYTE uNalType)
{
    for(unsigned int i = 0; i < m_uExtraData_Size - 4; i++) {
        if(m_pExtraData[i + 0] == 0 &&
           m_pExtraData[i + 1] == 0 &&
           m_pExtraData[i + 2] == 0 &&
           m_pExtraData[i + 3] == 1 &&
           (m_pExtraData[i + 4] & 0x1F) == uNalType) {
               return m_pExtraData + i + 5;
        }
    }
    return NULL;
}

bool CDVDVideoCodecOmx::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Open width:%d heigth:%d codec:%d stream type:%d\n", hints.width, hints.height, 
        hints.codec, hints.type);
    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Open fpsscale:%d fpsrate:%d calculated fps:%f\n", hints.fpsscale, hints.fpsrate, 
        hints.fpsscale ? (double) hints.fpsrate / (double) hints.fpsscale : 0.0);
    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Open aspect:%f variable framerate:%d\n", hints.aspect, hints.vfr); 

    switch(hints.codec) {
/*
        case CODEC_ID_MPEG2VIDEO:
            m_eVideoDecoderType = eVideoDecoderMPEG2;
            break;
        case CODEC_ID_MPEG4:
            m_eVideoDecoderType = eVideoDecoderMPEG4;
            break;
*/
        case CODEC_ID_H264:
            m_eVideoDecoderType = eVideoDecoderH264;
            break;
/*
        case CODEC_ID_SVQ1:
        case CODEC_ID_SVQ3:
            m_eVideoDecoderType = eVideoDecoderSorenson;
            break;
*/

        default:
            CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Open() Unable to find codec %d", hints.codec);
            return false;
    }

    g_bFirstFrameDecoded = false;
    m_dFrameTime = hints.fpsrate ? (double)hints.fpsscale / (double)hints.fpsrate * 1000000.0 : 0.0;
    m_dCurPts = DVD_NOPTS_VALUE;
    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Open Frame Time:%f usec\n", m_dFrameTime); 

    if(hints.extradata && hints.extrasize > 0) {
        CLog::Log(LOGDEBUG, "Extra Data %p size:%d\n", hints.extradata, hints.extrasize);
#if 1
        for(unsigned int i = 0; i < hints.extrasize; i++)
            printf("%02X ", *((BYTE*)hints.extradata + i));
        printf("\n");
#endif
        ProcessExtraData(hints.extradata, hints.extrasize);
        if(m_pExtraData) {
            CLog::Log(LOGDEBUG, "Processed Extra Data %p size:%d\n", m_pExtraData, m_uExtraData_Size);
#if 1
            for(unsigned int i = 0; i < m_uExtraData_Size; i++)
                printf("%02X ", *(m_pExtraData + i));
            printf("\n");
#endif
        }
        if(hints.codec == CODEC_ID_H264) {
            m_bMP4_h264_FileMode = true;
            if(m_pExtraData && m_uExtraData_Size >= 6) {
                BYTE *pSpsNal = GetNal(0x07);
                BYTE *pPpsNal = GetNal(0x08);
                if(pSpsNal && pPpsNal) {
                    // Get Profile
                    BYTE uProfile = pSpsNal[0];
                    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Open H.264 Profile:%u\n", uProfile);
                    // Get entropy_coding_mode_flag
                    BYTE uSeqParamBitPos = 0;
                    // Do quick Exp-Golomb code skip 
                    // Skip pic_parameter_set_id and seq_parameter_set_id
                    for(int i = 0; i < 2; i++) {
                        BYTE uLeadZeros = 0;
                        while((pPpsNal[uSeqParamBitPos / 8] & (1 << (7 - (uSeqParamBitPos % 8)))) == 0) {
                            uLeadZeros++;
                            uSeqParamBitPos++;
                        }
                        uSeqParamBitPos += (uLeadZeros + 1);
                    }
                    bool bEntropyCoding = pPpsNal[uSeqParamBitPos / 8] & (1 << (7 - (uSeqParamBitPos % 8)));
                    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Open H.264 Entropy Coding:%d\n", bEntropyCoding);
                    // If Profile is Main Profile and Entropy code is set then use Extended decoder
                    if(uProfile >= 77 && bEntropyCoding) {
                        m_eVideoDecoderType = eVideoDecoderH264Ext;
                    }
                }
            }
        }
    }

    // Setup callbacks
    m_OmxvideodecCallBacks.OnOmxDecEmptyBufferDone = OnOmxDecEmptyBufferDone;
    m_OmxvideodecCallBacks.OnOmxDecFillBufferDone = OnOmxDecFillBufferDone;
    m_OmxvideodecCallBacks.OnOmxEventCompletion = OnOmxEventCompletion;

    CLog::Log(LOGDEBUG, "OMX Init\n");
    bool bOmxInitStatus = m_oOmxUtils.OmxInit();
    if(!bOmxInitStatus) {
        CLog::Log(LOGDEBUG, "OmxInit() failed\n");
        return false;
    }
    m_bOmxInitialized = true;

    // Required OMX version
    OMX_VERSIONTYPE vOMX;
    vOMX.s.nVersionMajor = 1;
    vOMX.s.nVersionMinor = 1;
    vOMX.s.nRevision = 0;//2;
    vOMX.s.nStep = 0;

    strcpy(m_szComponentName, m_oOmxUtils.OmxGetComponentName(m_eVideoDecoderType));
    CLog::Log(LOGDEBUG, "OMX Component: %s This:%p\n", m_szComponentName, this);
    m_OmxDecState.pClientThis = (void *)this;
    m_OmxDecState.pCallBacks = &m_OmxvideodecCallBacks;
    m_OmxDecState.pStateDoneEvent = new CEvent;

    // Open component
    OMX_ERRORTYPE err = OMX_GetHandle(&m_hComponentHandle, m_szComponentName, (OMX_PTR)&m_OmxDecState, &OmxVideoDecCallbacks);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGDEBUG, "OMX_GetHandle() failed = %X\n", err);
        return false;
    }

#ifdef USE_OMX_TUNNEL
    m_OmxRendererState.pClientThis = (void *)this;
    m_OmxRendererState.pCallBacks = &m_OmxvideodecCallBacks;
    m_OmxRendererState.pStateDoneEvent = new CEvent;

    strcpy(m_szVidrendComponentName, m_oOmxUtils.OmxGetComponentName(eVideoRendererHDMI));
    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Open OMX Video Render Component: %s\n", m_szVidrendComponentName);
    // Open Renderer component, 
    err = OMX_GetHandle(&m_hVidRendComponentHandle, m_szVidrendComponentName, (OMX_PTR)&m_OmxRendererState, &OmxVideoDecCallbacks);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGERROR, "CDVDVideoCodecOmx::Open OMX_GetHandle(m_hVidRendComponentHandle) failed = %X\n", err);
        return false;
    }


    //
    //  Create Clock component
    //
    m_OmxClockState.pClientThis = (void *)this;
    m_OmxClockState.pCallBacks = &m_OmxvideodecCallBacks;
    m_OmxClockState.pStateDoneEvent = new CEvent;

    strcpy(m_szClockComponentName, m_oOmxUtils.OmxGetComponentName(eClockComponent));
    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Open OMX Clock Component: %s\n", m_szClockComponentName);
    // Open Clock component, 
    err = OMX_GetHandle(&m_hClockComponentHandle, m_szClockComponentName, (OMX_PTR)&m_OmxClockState, &OmxVideoDecCallbacks);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGERROR, "CDVDVideoCodecOmx::Open OMX_GetHandle(m_szClockComponentName) failed = %X\n", err);
        return false;
    }

    // Disable All ports
    err = OMX_SendCommand(m_hClockComponentHandle, OMX_CommandPortDisable, -1, 0);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGERROR, "CDVDVideoCodecOmx::Open OMX_CommandPortDisable(m_szClockComponentName) failed = %X\n", err);
        return false;
    }

    OMX_TIME_CONFIG_ACTIVEREFCLOCKTYPE ActiveClockType;
    INIT_PARAM(ActiveClockType);
    ActiveClockType.eClock = OMX_TIME_RefClockVideo;
    err = OMX_SetConfig(m_hClockComponentHandle, OMX_IndexConfigTimeActiveRefClock, &ActiveClockType); 
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGERROR, "CDVDVideoCodecOmx::Open OMX_IndexConfigTimeActiveRefClock(m_szClockComponentName) failed = %X\n", err);
        return false;
    }

    NVX_CONFIG_DISABLETSUPDATES TimeUpdateConf;
    OMX_INDEXTYPE eIndex;
    INIT_PARAM(TimeUpdateConf);
     
    err = OMX_GetExtensionIndex(m_hClockComponentHandle, (char *)NVX_INDEX_CONFIG_DISABLETIMESTAMPUPDATES, &eIndex);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGERROR, "CDVDVideoCodecOmx::Open OMX_GetExtensionIndex(NVX_INDEX_CONFIG_DISABLETIMESTAMPUPDATES) failed = %X\n", err);
        return false;
    }
#if 0
    TimeUpdateConf.bDisableTSUpdates = OMX_TRUE;
    err = OMX_SetConfig(m_hClockComponentHandle, eIndex, &TimeUpdateConf); 
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGERROR, "CDVDVideoCodecOmx::Open OMX_SetConfig(NVX_INDEX_CONFIG_DISABLETIMESTAMPUPDATES) failed = %X\n", err);
        return false;
    }
#endif
    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Open OMX Clock Component enable port 0\n");
    err = OMX_SendCommand(m_hClockComponentHandle, OMX_CommandPortEnable, 0, 0);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGDEBUG, "OMX_SendCommand() failed = %X\n", err);
        return false;
    }
    CLog::Log(LOGDEBUG, "Waiting for Clock component\n"); 
    m_OmxClockState.pStateDoneEvent->Wait();

    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Open OMX Video Renderer Component enable port 1\n");
    err = OMX_SendCommand(m_hVidRendComponentHandle, OMX_CommandPortEnable, 1, 0);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGDEBUG, "OMX_SendCommand() failed = %X\n", err);
        return false;
    }
    CLog::Log(LOGDEBUG, "Waiting for Video Renderer component\n"); 
    m_OmxRendererState.pStateDoneEvent->Wait();

    //
    // Setup Tunnels
    //
    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Open Setup Tunnel Clock->Renderer\n");
    err = OMX_SetupTunnel(m_hClockComponentHandle, 0, m_hVidRendComponentHandle, 1);
    if(err != OMX_ErrorNone) {
          CLog::Log(LOGERROR, "OMXTUNNEL OMX_SetupTunnel(m_hClockComponentHandle) failed = %X\n", err);
          return false;
    }
    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Open Setup Tunnel Decoder->Renderer\n");
    err = OMX_SetupTunnel(m_hComponentHandle, 1, m_hVidRendComponentHandle, 0);
    if(err != OMX_ErrorNone) {
          CLog::Log(LOGERROR, "OMXTUNNEL OMX_SetupTunnel(m_hVidRendComponentHandle) failed = %X\n", err);
          return false;
    }
#endif  //OMX_TUNNEL

    OMX_PARAM_PORTDEFINITIONTYPE oDecInputPortDef;
    memset((void *)&oDecInputPortDef, sizeof(oDecInputPortDef), 0);
    oDecInputPortDef.nSize = sizeof(oDecInputPortDef);
    oDecInputPortDef.nVersion.nVersion = vOMX.nVersion;
    oDecInputPortDef.nPortIndex = 0;
    err = OMX_GetParameter(m_hComponentHandle, OMX_IndexParamPortDefinition, &oDecInputPortDef);
    if(err != OMX_ErrorNone)
    {
        CLog::Log(LOGDEBUG, "OMX_GetParameter() failed = %X\n", err);
        return false;
    }
    m_uOmxInBufferCount = oDecInputPortDef.nBufferCountMin;
    m_uOmxInBufferSize = oDecInputPortDef.nBufferSize;
    m_uOmxCurInBuffer = 0;
    CLog::Log(LOGDEBUG, "decInputPortDef.nBufferCountMin: %u (Actual: %lu) Size: %u\n", m_uOmxInBufferCount,
        oDecInputPortDef.nBufferCountActual, m_uOmxInBufferSize);

    CLog::Log(LOGDEBUG, "Set BufferCount:%u BufferSize:%u\n", m_uOmxInBufferCount, m_uOmxInBufferSize);
    for(unsigned int i = 0; i < m_uOmxInBufferCount; i++) {
        err = OMX_UseBuffer(m_hComponentHandle, &m_pOmxInBuffers[i], 0, NULL, m_uOmxInBufferSize, m_dummy_mem /*NULL*/);
        if(err != OMX_ErrorNone) {
            CLog::Log(LOGDEBUG, "OMX_UseBuffer() seq:%u failed = %X\n", i, err);
            return false;
        }
    }
    m_InBufferSemaphore = new CSemaphore(m_uOmxInBufferCount, m_uOmxInBufferCount);

    CLog::Log(LOGDEBUG, "OMX_UseBuffer() Done\n");

#ifndef USE_OMX_TUNNEL
    OMX_PARAM_PORTDEFINITIONTYPE oDecOutputPortDef;
    memset((void *)&oDecOutputPortDef, sizeof(oDecOutputPortDef), 0);
    oDecOutputPortDef.nPortIndex = 1;
    oDecOutputPortDef.nSize = sizeof(oDecOutputPortDef);
    oDecOutputPortDef.nVersion.nVersion = vOMX.nVersion;
    err = OMX_GetParameter(m_hComponentHandle, OMX_IndexParamPortDefinition, &oDecOutputPortDef);
    if(err != OMX_ErrorNone)
    {
        CLog::Log(LOGDEBUG, "OMX_GetParameter() failed = %X\n", err);
        return false;
    }
    m_uOmxOutBufferCount = oDecOutputPortDef.nBufferCountMin;
    m_uOmxOutBufferSize = oDecOutputPortDef.nBufferSize;
    CLog::Log(LOGDEBUG, "oDecOutputPortDef.nBufferCountMin: %u MinBuffer size: %u\n", m_uOmxOutBufferCount, m_uOmxOutBufferSize);

    unsigned int uOmxOutBufferSize = hints.width * hints.height * 3 / 2;
    if(uOmxOutBufferSize < m_uOmxOutBufferSize) {
        uOmxOutBufferSize = m_uOmxOutBufferSize;
    }
    m_uOmxOutBufferSize = uOmxOutBufferSize;
    CLog::Log(LOGDEBUG, "Buffer size: %u\n", m_uOmxOutBufferSize);
    for(unsigned int i = 0; i < m_uOmxOutBufferCount; i++) {
        err = OMX_AllocateBuffer(m_hComponentHandle, &m_pOmxOutBuffers[i], 1, NULL, m_uOmxOutBufferSize);
        if(err != OMX_ErrorNone) {
            CLog::Log(LOGDEBUG, "OMX_AllocateBuffer() seq:%u failed = %X\n", i, err);
            return false;
        }
    }
    m_OutBufferSemaphore = new CSemaphore(m_uOmxOutBufferCount, m_uOmxOutBufferCount);
    m_VideoFreeQueue = new CQueue(m_uOmxOutBufferCount, sizeof(void *));
    m_VideoDecodedQueue = new CQueue(m_uOmxOutBufferCount, sizeof(void *));
    for(unsigned int i = 0; i < m_uOmxOutBufferCount; i++) {
        bool status = m_VideoFreeQueue->Put((void *)&m_pOmxOutBuffers[i], 0);
        if(!status) {
            CLog::Log(LOGDEBUG, "Free queue fill failed\n");
            return false;
        }
    }
#endif

#ifdef USE_OMX_TUNNEL
    // Move to Idle state
    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Open Set Clock to Idle\n"); 
    err = OMX_SendCommand(m_hClockComponentHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGDEBUG, "OMX_SendCommand() failed = %X\n", err);
        return false;
    }
#endif
    // Move to Idle state
    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Open Set Decoder to Idle\n"); 
    err = OMX_SendCommand(m_hComponentHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGDEBUG, "OMX_SendCommand() failed = %X\n", err);
        return false;
    }
#ifdef USE_OMX_TUNNEL
    // Move to Idle state
    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Open Set Renderer to Idle\n"); 
    err = OMX_SendCommand(m_hVidRendComponentHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGDEBUG, "OMX_SendCommand() failed = %X\n", err);
        return false;
    }
    // Wait for Idle State notification
    CLog::Log(LOGDEBUG, "Waiting for Clock component to go IDLE\n"); 
    m_OmxClockState.pStateDoneEvent->Wait();
    // Wait for Idle State notification
    CLog::Log(LOGDEBUG, "Waiting for Renderer component to go IDLE\n"); 
    m_OmxRendererState.pStateDoneEvent->Wait();
#endif
    // Wait for Idle State notification
    CLog::Log(LOGDEBUG, "Waiting for Decoder component to go IDLE\n"); 
    m_OmxDecState.pStateDoneEvent->Wait();

#ifdef USE_OMX_TUNNEL
    OMX_TIME_CONFIG_CLOCKSTATETYPE ClockState;
    INIT_PARAM(ClockState);
    ClockState.nOffset = 0;
    ClockState.nStartTime = (OMX_TICKS)(0)*1000;
    ClockState.nWaitMask = 0;
    ClockState.eState = OMX_TIME_ClockStateRunning;
 
    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Open Start Clock\n"); 
    err = OMX_SetConfig(m_hClockComponentHandle, OMX_IndexConfigTimeClockState, &ClockState);
    while(OMX_ErrorNotReady == err)
    {
        CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Open Clock is not ready\n"); 
        sleep(1);
        err = OMX_SetConfig(m_hClockComponentHandle, OMX_IndexConfigTimeClockState, &ClockState);
    }
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGERROR, "CDVDVideoCodecOmx::Open OMX_IndexConfigTimeClockState failed = %X\n", err);
        return false;
    }
#endif 
    CLog::Log(LOGDEBUG, "Sending Command to set Decoder Execute State\n");       
    err = OMX_SendCommand(m_hComponentHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGDEBUG, "OMX_SendCommand() failed = %X\n", err);
        return false;
    }
#ifdef USE_OMX_TUNNEL
    CLog::Log(LOGDEBUG, "Sending Command to set Clock Execute State\n");       
    err = OMX_SendCommand(m_hClockComponentHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGDEBUG, "OMX_SendCommand() failed = %X\n", err);
        return false;
    }

    CLog::Log(LOGDEBUG, "Sending Command to set Renderer Execute State\n");       
    err = OMX_SendCommand(m_hVidRendComponentHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGDEBUG, "OMX_SendCommand() failed = %X\n", err);
        return false;
    }
#endif
    CLog::Log(LOGDEBUG, "Wait for Decoder Execute State\n");       
    // Wait for Executing State
    m_OmxDecState.pStateDoneEvent->Wait();
#ifdef USE_OMX_TUNNEL
    CLog::Log(LOGDEBUG, "Wait for Clock Execute State\n");       
    // Wait for Executing State
    m_OmxClockState.pStateDoneEvent->Wait();
    CLog::Log(LOGDEBUG, "Wait for Renderer Execute State\n");       
    // Wait for Executing State
    m_OmxRendererState.pStateDoneEvent->Wait();
#endif
    m_iPictureWidth = hints.width;
    m_iPictureHeight = hints.height;

    CLog::Log(LOGDEBUG, "Returning Open\n");       

    return true;
}

void CDVDVideoCodecOmx::Dispose()
{
    OMX_ERRORTYPE err;
    CLog::Log(LOGDEBUG,"CDVDVideoCodecOmx::Dispose()");

    if(!m_bOmxInitialized)
        return;
#ifdef USE_OMX_TUNNEL
    OMX_VERSIONTYPE vOMX;
    vOMX.s.nVersionMajor = 1;
    vOMX.s.nVersionMinor = 1;
    vOMX.s.nRevision = 0;//2;
    vOMX.s.nStep = 0;

    OMX_TIME_CONFIG_CLOCKSTATETYPE ClockState;
    INIT_PARAM(ClockState);
    ClockState.nOffset = 0;
    ClockState.nStartTime = 0;
    ClockState.nWaitMask = 0;
    ClockState.eState = OMX_TIME_ClockStateStopped;
 
    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Dispose Stop Clock\n"); 
    err = OMX_SetConfig(m_hClockComponentHandle, OMX_IndexConfigTimeClockState, &ClockState);
    while(OMX_ErrorNotReady == err)
    {
        CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Dispose Clock is not ready\n"); 
        sleep(1);
        err = OMX_SetConfig(m_hClockComponentHandle, OMX_IndexConfigTimeClockState, &ClockState);
    }
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGERROR, "CDVDVideoCodecOmx::Dispose OMX_IndexConfigTimeClockState failed = %X\n", err);
        return;
    }
#endif 
#if 1
    CLog::Log(LOGDEBUG,"CDVDVideoCodecOmx::Dispose() Send Decoder Flush");
    err = OMX_SendCommand(m_hComponentHandle, OMX_CommandFlush, OMX_ALL, NULL);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGERROR, "OMX_SendCommand() OMX_CommandFlush failed = %X\n", err);
        return;
    }
    CLog::Log(LOGDEBUG,"CDVDVideoCodecOmx::Dispose() Wait for Decoder Flush");
    // Wait for Idle State notification
    m_OmxDecState.pStateDoneEvent->Wait();
#endif

    CLog::Log(LOGDEBUG,"CDVDVideoCodecOmx::Dispose() Set Decoder to Idle");
    err = OMX_SendCommand(m_hComponentHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGERROR, "OMX_SendCommand() failed = %X\n", err);
        return;
    }
#ifdef USE_OMX_TUNNEL
    CLog::Log(LOGDEBUG,"CDVDVideoCodecOmx::Dispose() Set Clock to Idle");
    err = OMX_SendCommand(m_hClockComponentHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGERROR, "OMX_SendCommand() failed = %X\n", err);
        return;
    }

    CLog::Log(LOGDEBUG,"CDVDVideoCodecOmx::Dispose() Set Renderer to Idle");
    err = OMX_SendCommand(m_hVidRendComponentHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGERROR, "OMX_SendCommand() failed = %X\n", err);
        return;
    }
#endif
    CLog::Log(LOGDEBUG,"CDVDVideoCodecOmx::Dispose() Wait for Decoder Idle");
    // Wait for Idle State notification
    m_OmxDecState.pStateDoneEvent->Wait();
#ifdef USE_OMX_TUNNEL
    CLog::Log(LOGDEBUG,"CDVDVideoCodecOmx::Dispose() Wait for Clock Idle");
    // Wait for Idle State notification
    m_OmxClockState.pStateDoneEvent->Wait();
    CLog::Log(LOGDEBUG,"CDVDVideoCodecOmx::Dispose() Wait for Renderer Idle");
    // Wait for Idle State notification
    m_OmxRendererState.pStateDoneEvent->Wait();
#endif

    err = OMX_SendCommand(m_hComponentHandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGERROR, "OMX_SendCommand() failed = %X\n", err);
        return;
    }
#ifdef USE_OMX_TUNNEL
    err = OMX_SendCommand(m_hVidRendComponentHandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGERROR, "OMX_SendCommand() failed = %X\n", err);
        return;
    }

    err = OMX_SendCommand(m_hClockComponentHandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGERROR, "OMX_SendCommand() failed = %X\n", err);
        return;
    }
#endif
    CLog::Log(LOGDEBUG,"CDVDVideoCodecOmx::Dispose() Free Input Buffers");
    for(unsigned int i = 0; i < m_uOmxInBufferCount; i++) {
        err = OMX_FreeBuffer(m_hComponentHandle, 0, m_pOmxInBuffers[i]);
        if(err != OMX_ErrorNone) {
            CLog::Log(LOGERROR, "OMX_FreeBuffer() seq:%u failed = %X\n", i, err);
            return;
        }
    }

#ifndef USE_OMX_TUNNEL
    CLog::Log(LOGDEBUG,"CDVDVideoCodecOmx::Dispose() Free Output Buffers");
    for(unsigned int i = 0; i < m_uOmxOutBufferCount; i++) {
        err = OMX_FreeBuffer(m_hComponentHandle, 1, m_pOmxOutBuffers[i]);
        if(err != OMX_ErrorNone) {
            CLog::Log(LOGERROR, "OMX_FreeBuffer() seq:%u failed = %X\n", i, err);
            return;
        }
    }
#endif

    CLog::Log(LOGDEBUG,"CDVDVideoCodecOmx::Dispose() Wait for Decoder Loaded State");
    // Wait for Idle State notification
    m_OmxDecState.pStateDoneEvent->Wait();
#ifdef USE_OMX_TUNNEL
    CLog::Log(LOGDEBUG,"CDVDVideoCodecOmx::Dispose() Wait for Clock Loaded State");
    // Wait for Idle State notification
    m_OmxClockState.pStateDoneEvent->Wait();
    CLog::Log(LOGDEBUG,"CDVDVideoCodecOmx::Dispose() Wait for Renderer Loaded State");
    // Wait for Idle State notification
    m_OmxRendererState.pStateDoneEvent->Wait();

    err = OMX_FreeHandle(m_hVidRendComponentHandle);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGERROR, "OMX_SendCommand() failed = %X\n", err);
        return;
    }

    err = OMX_FreeHandle(m_hClockComponentHandle);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGERROR, "OMX_SendCommand() failed = %X\n", err);
        return;
    }
#endif
    err = OMX_FreeHandle(m_hComponentHandle);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGERROR, "OMX_SendCommand() failed = %X\n", err);
        return;
    }

    CLog::Log(LOGDEBUG,"CDVDVideoCodecOmx::Dispose() DeInit OMX");
    bool bOmxDeinitStatus = m_oOmxUtils.OmxDeinit();
    if(!bOmxDeinitStatus) {
        CLog::Log(LOGDEBUG, "OmxDeinit() failed\n");
        return;
    }
    m_bOmxInitialized = false;
    CLog::Log(LOGDEBUG,"CDVDVideoCodecOmx::Dispose() Done");

    return;
}

void CDVDVideoCodecOmx::SetDropState(bool bDrop)
{
}

signed long long CDVDVideoCodecOmx::pts_dtoi(double pts)
{
  pts_union u;
  u.pts_d = pts;
  u.pts_i >>= 4;
  return u.pts_i;
}

double CDVDVideoCodecOmx::pts_itod(signed long long pts)
{
  pts_union u;
  pts <<= 4;
  u.pts_i = pts;
  return u.pts_d;
}

bool CDVDVideoCodecOmx::FillInputBuffer(BYTE* pData, int iSize, signed long long sPts)
{
    static unsigned int k;

    if(m_bAborting) {
        CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::FillInputBuffer Aborting\n");
        return true;
    }

#ifndef USE_OMX_TUNNEL
    // Check we have input buffer available
    if(!m_InBufferSemaphore->Decrement(0)) {
        // No free input buffer
        // Chekc we have decoded picture
        if(!m_VideoDecodedQueue->GetSize()) {
            // No Decode picture
            // We have to wait until a decoded picture becomes available
            m_VideoDecodedQueue->WaitForNotEmpty(8);
        }
        return false;
    }
#else
    m_InBufferSemaphore->Decrement(-1);
#endif

    OMX_BUFFERHEADERTYPE *pOmxBuffer = m_pOmxInBuffers[m_uOmxCurInBuffer];
    if(pData) {
        pOmxBuffer->pBuffer = (OMX_U8 *)malloc(iSize);
        if(!pOmxBuffer->pBuffer) {
            CLog::Log(LOGERROR, "CDVDVideoCodecOmx::FillInputBuffer - Out of memory\n");
            return false;
        }
        memcpy(pOmxBuffer->pBuffer, pData, iSize);
        pOmxBuffer->nFilledLen = iSize;
        pOmxBuffer->nOffset = 0;
        pOmxBuffer->nTimeStamp = sPts;
        pOmxBuffer->pAppPrivate = NULL;
    } else {
        pOmxBuffer->pBuffer = NULL;
        pOmxBuffer->nFilledLen = 0;
        pOmxBuffer->nOffset = 0;
        pOmxBuffer->pAppPrivate = NULL;
        pOmxBuffer->nTimeStamp = 0;
        pOmxBuffer->nFlags = OMX_BUFFERFLAG_EOS;
    }
    m_uOmxCurInBuffer = (m_uOmxCurInBuffer + 1) % m_uOmxInBufferCount;
    m_uOmxInOutstandingBuffer++;

    CLog::Log(LOGDEBUG, "Send Input Data: (%u) Outstanding: %u  Buff:%p Len:%lu nTimeStamp:%lld\n", k++, m_uOmxInOutstandingBuffer,
        pOmxBuffer->pBuffer, pOmxBuffer->nFilledLen, pOmxBuffer->nTimeStamp);
    OMX_ERRORTYPE err = OMX_EmptyThisBuffer(m_hComponentHandle, pOmxBuffer);
    if(err != OMX_ErrorNone)
    {
        CLog::Log(LOGERROR, "OMX_EmptyThisBuffer() failed = %X\n", err);
        return false;
    }

    return true;
}

bool CDVDVideoCodecOmx::FillOutputBuffer(void)
{
    static unsigned int k;

    if(m_bAborting) {
        CLog::Log(LOGERROR, "FillOutputBuffer Aborting\n");
        return true;
    }

    OMX_BUFFERHEADERTYPE *pOmxBuffer;
    while(m_VideoFreeQueue->Get((void *)&pOmxBuffer, 0)) {
        pOmxBuffer->nOffset = 0;

        OMX_ERRORTYPE err = OMX_FillThisBuffer(m_hComponentHandle, pOmxBuffer);
        if(err != OMX_ErrorNone)
        {
            CLog::Log(LOGERROR, "OMX_FillThisBuffer() failed = %X\n", err);
            return false;
        }
        CLog::Log(LOGDEBUG, "Get Output Data: (%u) %p\n", k++, pOmxBuffer->pBuffer);
    }

    return true;
}

extern double g_fCurrentAudioClock;

int CDVDVideoCodecOmx::Decode(BYTE* pData, int iSize, double pts)
{
    int iRes = 0;
    static unsigned int k;
    bool bInputSent = true;
    if(pData) {
        CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Decode pData=%p iSize=%d pts=%f\n", pData, iSize, pts);
    }

    if(!m_bRunning) {
        CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Decode set running\n");
    }


#ifdef USE_OMX_TUNNEL
    OMX_VERSIONTYPE vOMX;
    vOMX.s.nVersionMajor = 1;
    vOMX.s.nVersionMinor = 1;
    vOMX.s.nRevision = 0;//2;
    vOMX.s.nStep = 0;
/*
    OMX_CONFIG_POINTTYPE oPointType;

    INIT_PARAM(oPointType);
    oPointType.nX = 1900;
    oPointType.nY = 0;

    OMX_SetConfig(m_hVidRendComponentHandle, OMX_IndexConfigCommonOutputPosition, &oPointType);

    OMX_CONFIG_RECTTYPE oRect;

    INIT_PARAM(oRect);
    oRect.nPortIndex = 0;
    oRect.nLeft = 1900;
    oRect.nTop = 0;
    oRect.nWidth = 20;
    oRect.nHeight = 816;

    //OMX_SetConfig(m_hVidRendComponentHandle, OMX_IndexConfigCommonInputCrop, &oRect);

    OMX_FRAMESIZETYPE oSize;

    INIT_PARAM(oSize);
    oSize.nPortIndex = 0;
    oSize.nWidth = 640;
    oSize.nHeight = 480;

    OMX_SetConfig(m_hVidRendComponentHandle, OMX_IndexConfigCommonOutputSize, &oSize);
*/
    OMX_TIME_CONFIG_TIMESTAMPTYPE TimeStamp;
    INIT_PARAM(TimeStamp);
    TimeStamp.nPortIndex = OMX_ALL;
    OMX_ERRORTYPE err = OMX_GetConfig(m_hClockComponentHandle, OMX_IndexConfigTimeCurrentMediaTime, &TimeStamp);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGERROR, "CDVDVideoCodecOmx::Decode OMX_IndexConfigTimeCurrentMediaTime failed = %X\n", err);
        return false;
    }
    double fMediaTime = (double)TimeStamp.nTimestamp / 1000000.0;
    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Decode Current Clock Timestamp:%lld %.6f\n", TimeStamp.nTimestamp, fMediaTime);

    OMX_TIME_CONFIG_TIMESTAMPTYPE WallClock;
    INIT_PARAM(WallClock);
    WallClock.nPortIndex = OMX_ALL;
    err = OMX_GetConfig(m_hClockComponentHandle, OMX_IndexConfigTimeCurrentWallTime, &WallClock);
    if(err != OMX_ErrorNone) {
        CLog::Log(LOGERROR, "CDVDVideoCodecOmx::Decode OMX_IndexConfigTimeCurrentMediaTime failed = %X\n", err);
        return false;
    }
    double fWallTime = (double)WallClock.nTimestamp / 1000000.0;
    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Decode Current Wall Timestamp:%lld %.6f  Wall-Media:%.6f\n", WallClock.nTimestamp, fWallTime, fWallTime - fMediaTime);
#endif

#ifndef USE_OMX_TUNNEL
    if(m_pLastSentOmxBuffer) {
        m_VideoFreeQueue->Put((void *)&m_pLastSentOmxBuffer, 0);
        m_pLastSentOmxBuffer = NULL;
    }
    FillOutputBuffer();
#endif

    if(pData && m_dCurPts == DVD_NOPTS_VALUE) {
        m_dCurPts = pts;
        CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Decode Sync to pts: %f\n", m_dCurPts);
    }

    if(pData == NULL && m_pDataSave) {
        pData = m_pDataSave;
        iSize = m_iSizeSave;
        pts = m_dPtsSave;
    }

#if 0
    if(pData) {
        for(int i = 0; i < ((32 < iSize) ? 32 : iSize); i++)
            printf("%02X ", pData[i]);
        printf("\n");
    }
#endif

    if(m_bMP4_h264_FileMode && !m_pDataSave) {
        unsigned int uFixRemaining = iSize;
        BYTE *pFixData = pData;
        while(uFixRemaining) {
            unsigned int uChunkSize = (pFixData[0] << 24) | (pFixData[1] << 16) | (pFixData[2] << 8) | pFixData[3];
            //CLog::Log(LOGDEBUG, "Chunk size: %u\n", uChunkSize);
            pFixData[0] = 0;
            pFixData[1] = 0;
            pFixData[2] = 0;
            pFixData[3] = 1;
            pFixData += (uChunkSize + 4);
            uFixRemaining -= (uChunkSize + 4);
        }
    }

    //signed long long sPts = pts_dtoi(pts);
    signed long long sPts;
    double fTimeError = 0;
    if(g_fCurrentAudioClock) {
        fTimeError = (double)TimeStamp.nTimestamp - g_fCurrentAudioClock;
        if(fTimeError > m_fTimeError)
            m_fTimeError += 350.0;
        else
            m_fTimeError -= 350.0;
    }
    if(pts != DVD_NOPTS_VALUE) {
        sPts = (signed long long)(pts + m_fTimeError);
    } else {
        sPts = 0;
    }
    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::Decode Converted pts:%lld (0x%llX) Audio:%.6f Abs Error:%.6f Adj err:%.6f Delta:%.6f\n", 
        sPts, sPts, g_fCurrentAudioClock, fTimeError, m_fTimeError, m_fTimeError - fTimeError);

    if(!m_bRunning) {
        m_bRunning = true;
        if(m_pExtraData) {
            unsigned int uTotalSize;
            // Send Extra Header Data
            uTotalSize = m_uExtraData_Size + iSize;
            m_pExtraData = (BYTE *)realloc(m_pExtraData, uTotalSize);
            if(!m_pExtraData) {
                return VC_ERROR;
            }
            memcpy(m_pExtraData + m_uExtraData_Size, pData, iSize);
#if 1
            if(m_pExtraData) {
                for(unsigned int i = 0; i < ((128 < uTotalSize) ? 128 : uTotalSize); i++)
                    printf("%02X ", m_pExtraData[i]);
                printf("\n");
            }
#endif
            bInputSent = FillInputBuffer(m_pExtraData, uTotalSize, sPts);
        } else {
            bInputSent = FillInputBuffer(pData, iSize, sPts);
        }
    } else {
        // Send Buffer
        bInputSent = FillInputBuffer(pData, iSize, sPts);
    }

    k++;
    if(bInputSent) {
        m_pDataSave = NULL;
        iRes = VC_BUFFER | VC_PICTURE;
    } else {
        m_pDataSave = pData;
        m_iSizeSave = iSize;
        m_dPtsSave = pts;
    }

#ifndef USE_OMX_TUNNEL
    if(m_VideoDecodedQueue->GetSize()) {
        iRes |= VC_PICTURE;
    }
#endif

#ifdef USE_OMX_TUNNEL
    if(m_bSendTunneledFirstFrame) {
        iRes |= VC_PICTURE;
    }
#endif

    return iRes;
}

void CDVDVideoCodecOmx::Reset()
{
}

bool CDVDVideoCodecOmx::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::GetPicture\n");

    pDvdVideoPicture->iWidth = m_iPictureWidth;
    pDvdVideoPicture->iHeight = m_iPictureHeight;

    for (int i = 0; i < 4; i++)
      pDvdVideoPicture->data[i]      = NULL;
    for (int i = 0; i < 4; i++)
      pDvdVideoPicture->iLineSize[i] = 0;

    pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED; 
    pDvdVideoPicture->pts = DVD_NOPTS_VALUE;

    pDvdVideoPicture->format = DVDVideoPicture::FMT_INTERNAL;

    return true;



    if(m_bSendTunneledFirstFrame) {
        CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::GetPicture Send Tunneled First Frame\n");
        pDvdVideoPicture->iWidth  = m_iPictureWidth;
        pDvdVideoPicture->iHeight = m_iPictureHeight;
        pDvdVideoPicture->iDisplayWidth  = m_iPictureWidth;
        pDvdVideoPicture->iDisplayHeight = m_iPictureHeight;
        pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED | CONF_FLAGS_OMX_TUNNELING;
        pDvdVideoPicture->iRepeatPicture = 0;
        pDvdVideoPicture->pts = pts_itod(m_sCurPts);
        pDvdVideoPicture->format = DVDVideoPicture::FMT_YUV420P;
        m_bSendTunneledFirstFrame = false;
        return true;
    }

    OMX_BUFFERHEADERTYPE *pOmxBuffer;
    if(m_VideoDecodedQueue->Get((void *)&pOmxBuffer, 0)) {
        CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::GetPicture has picture\n");
        m_pLastSentOmxBuffer = pOmxBuffer;
        m_pPictureFrameData = pOmxBuffer->pBuffer;
        m_sCurPts = pOmxBuffer->nTimeStamp;
        CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::GetPicture nTimeStamp:%llx time:%f\n", m_sCurPts, pts_itod(m_sCurPts));


        if(m_pPictureFrameData) {
            pDvdVideoPicture->iWidth  = m_iPictureWidth;
            pDvdVideoPicture->iHeight = m_iPictureHeight;
            pDvdVideoPicture->iDisplayWidth  = m_iPictureWidth;
            pDvdVideoPicture->iDisplayHeight = m_iPictureHeight;
#if 0
            pDvdVideoPicture->data[0] = (BYTE *)pOmxBuffer;
            pDvdVideoPicture->data[1] = (BYTE *)this;
            pDvdVideoPicture->data[2] = (BYTE *)DecoderBufferCallback;
#else
            pDvdVideoPicture->data[0] = m_pPictureFrameData;
            pDvdVideoPicture->data[1] = pDvdVideoPicture->data[0] + m_iPictureWidth * m_iPictureHeight;
            pDvdVideoPicture->data[2] = pDvdVideoPicture->data[1] + m_iPictureWidth * m_iPictureHeight / 4;
#endif
            pDvdVideoPicture->iLineSize[0] = m_iPictureWidth;
            pDvdVideoPicture->iLineSize[1] = m_iPictureWidth / 2;
            pDvdVideoPicture->iLineSize[2] = m_iPictureWidth / 2;

            pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED;
            pDvdVideoPicture->iRepeatPicture = 0;
            pDvdVideoPicture->pts = pts_itod(m_sCurPts);
//            pDvdVideoPicture->pts = m_dCurPts;
            // Temporary hack for calculating time code
            m_dCurPts += m_dFrameTime;
            pDvdVideoPicture->format = DVDVideoPicture::FMT_YUV420P;

            CLog::Log(LOGDEBUG, "Picture: %dx%d pData:%p LineSize:%d pts:%f\n", pDvdVideoPicture->iWidth, pDvdVideoPicture->iHeight,
                pDvdVideoPicture->data[0], pDvdVideoPicture->iLineSize[0], pDvdVideoPicture->pts);
            CLog::Log(LOGDEBUG, "Data [0]:%p [1]:%p [2]:%p\n",
                pDvdVideoPicture->data[0],
                pDvdVideoPicture->data[1],
                pDvdVideoPicture->data[2]);

            return true;
        } else {
            CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::GetPicture - NULL OMX Data pointer\n");
        }
    } else {
        CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::GetPicture - Queue has no Picture\n");
    }

    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::GetPicture - No picture found\n");
    return false;
}

void CDVDVideoCodecOmx::DecoderBufferCallback(void *pClientThis, OMX_BUFFERHEADERTYPE *pOmxBuffer)
{
    CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::DecoderBufferCallback pClientThis:%p pOmxBuffer:%p\n", pClientThis, pOmxBuffer);
    CDVDVideoCodecOmx *pThis = reinterpret_cast<CDVDVideoCodecOmx*>(pClientThis);
    pThis->m_VideoFreeQueue->Put((void *)&pOmxBuffer, 0);
}

#if 0
/*
 * Calculate the height and width this video should be displayed in
 */
void CDVDVideoCodecOmx::GetVideoAspect(AVCodecContext* pCodecContext, unsigned int& iWidth, unsigned int& iHeight)
{
  double aspect_ratio;

  /* XXX: use variable in the frame */
  if(pCodecContext->sample_aspect_ratio.num == 0) aspect_ratio = 0;
  else aspect_ratio = av_q2d(pCodecContext->sample_aspect_ratio) * pCodecContext->width / pCodecContext->height;

  if(aspect_ratio <= 0.0) aspect_ratio = (float)pCodecContext->width / (float)pCodecContext->height;

  /* XXX: we suppose the screen has a 1.0 pixel ratio */ // CDVDVideo will compensate it.
  iHeight = pCodecContext->height;
  iWidth = ((int)RINT(pCodecContext->height * aspect_ratio)) & -3;
  if(iWidth > (unsigned int)pCodecContext->width)
  {
    iWidth = pCodecContext->width;
    iHeight = ((int)RINT(pCodecContext->width / aspect_ratio)) & -3;
  }
}
#endif

static OMX_ERRORTYPE OmxDecEventHandler(
    OMX_OUT OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_PTR pAppData,
    OMX_OUT OMX_EVENTTYPE eEvent,
    OMX_OUT OMX_U32 Data1,
    OMX_OUT OMX_U32 Data2,
    OMX_OUT OMX_PTR pEventData)
{
    EOMXEvent eOmxEvent = eOMXEventUndefined;

    CLog::Log(LOGDEBUG, "OmxDecEventHandler() %x called\n", eEvent);

    switch(eEvent) {
        case OMX_EventCmdComplete:
            CLog::Log(LOGDEBUG, "Event: OMX_EventCmdComplete nData1: %lu nData2: %lu\n", Data1, Data2);
            switch(Data1) {
                case OMX_CommandStateSet:
                    eOmxEvent = eOMXEventStateChanged;
                    CLog::Log(LOGDEBUG, "State Reached: ");
                    switch ((int)Data2) {
                        case OMX_StateInvalid:
                            CLog::Log(LOGDEBUG, "OMX_StateInvalid\n");
                            break;
                        case OMX_StateLoaded:
                            CLog::Log(LOGDEBUG, "OMX_StateLoaded\n");
                            break;
                        case OMX_StateIdle:
                            CLog::Log(LOGDEBUG, "OMX_StateIdle\n");
                            break;
                        case OMX_StateExecuting:
                            CLog::Log(LOGDEBUG, "OMX_StateExecuting\n");
                            break;
                        case OMX_StatePause:
                            CLog::Log(LOGDEBUG, "OMX_StatePause\n");
                            break;
                        case OMX_StateWaitForResources:
                            CLog::Log(LOGDEBUG, "OMX_StateWaitForResources\n");
                            break;
                        default:
                            CLog::Log(LOGDEBUG, "Invalid State\n");
                            break;
                    }
                    break;
                case OMX_CommandFlush:
                    CLog::Log(LOGDEBUG, "Event: OMX_CommandFlush Port:%lu\n", Data1);
                    eOmxEvent = eOMXEventFlush;
                    break;
                case OMX_CommandPortEnable:
                    CLog::Log(LOGDEBUG, "Event: OMX_CommandPortEnable Port:%lu\n", Data1);
                    eOmxEvent = eOMXEventPortEnable;
                    break;
                case OMX_CommandPortDisable:
                    CLog::Log(LOGDEBUG, "Event: OMX_CommandPortDisable Port:%lu\n", Data1);
                    eOmxEvent = eOMXEventPortDisable;
                    break;
                default:
                    break;
            }
            break;
        case OMX_EventError:
            CLog::Log(LOGDEBUG, "Event: OMX_EventError Error code: %lx Port:%lu\n", Data1, Data2);
            break;
        case OMX_EventMark:
            CLog::Log(LOGDEBUG, "Event: OMX_EventMark\n");
            break;
        case OMX_EventPortSettingsChanged:
            CLog::Log(LOGDEBUG, "Event: OMX_EventPortSettingsChanged Port: %lu\n", Data1);
            eOmxEvent = eOMXEventPortSettingsChanged ;
            break;
        case OMX_EventBufferFlag:
            eOmxEvent = eOMXEventEndOfStream;
            CLog::Log(LOGDEBUG, "Event: OMX_EventBufferFlag Port: %lu nFlags: %lx\n", Data1, Data2);
            break;
        case OMX_EventResourcesAcquired:
            CLog::Log(LOGDEBUG, "Event: OMX_EventResourcesAcquired\n");
            break;
        case OMX_EventComponentResumed:
            CLog::Log(LOGDEBUG, "Event: OMX_EventComponentResumed\n");
            break;
        case OMX_EventDynamicResourcesAvailable:
            CLog::Log(LOGDEBUG, "Event: OMX_EventDynamicResourcesAvailable\n");
            break;
        case OMX_EventPortFormatDetected:
            CLog::Log(LOGDEBUG, "Event: OMX_EventPortFormatDetected\n");
            break;
        case NVX_EventFirstFrameDisplayed:
            CLog::Log(LOGDEBUG, "Event: NVX_EventFirstFrameDisplayed\n");
            eOmxEvent = eOMXEventFirstFrameDisplayed;
            break;
        default:
            CLog::Log(LOGDEBUG, "Event: Unknown\n");
            break;
    }

    ((OmxDecState *)pAppData)->pCallBacks->OnOmxEventCompletion(((OmxDecState *)pAppData)->pClientThis, (OmxDecState *)pAppData, eOmxEvent, Data1, Data2);
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE OmxDecEmptyBufferDone(
    OMX_OUT OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_PTR pAppData,
    OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
{
    ((OmxDecState *)pAppData)->pCallBacks->OnOmxDecEmptyBufferDone(((OmxDecState *)pAppData)->pClientThis, pBuffer);
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE OmxDecFillBufferDone(
    OMX_OUT OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_PTR pAppData,
    OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
{
    ((OmxDecState *)pAppData)->pCallBacks->OnOmxDecFillBufferDone(((OmxDecState *)pAppData)->pClientThis, pBuffer);
    return OMX_ErrorNone;
}

void CDVDVideoCodecOmx::OnOmxDecEmptyBufferDone(void *pClientThis, OMX_BUFFERHEADERTYPE *pBuffer)
{
    static unsigned int k;
    if(!pBuffer) {
        CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::OnOmxDecEmptyBufferDone (%u) pBuffer NULL\n", k++);
        return;
    }
    CDVDVideoCodecOmx *pThis = reinterpret_cast<CDVDVideoCodecOmx*>(pClientThis);
    CLog::Log(LOGDEBUG, "OnOmxDecEmptyBufferDone (%u) pAppPrivate: %p\n", k++, pBuffer->pAppPrivate);

    // Free allocated input stream buffer
    if(pBuffer->pBuffer)
        free(pBuffer->pBuffer);

    if(!pBuffer->pAppPrivate) {
        pThis->m_uOmxInOutstandingBuffer--;
        pThis->m_InBufferSemaphore->Increment();
    } else {
        pThis->m_oEventOmxEndOfStream.Set();
    }
}

void CDVDVideoCodecOmx::OnOmxDecFillBufferDone(void *pClientThis, OMX_BUFFERHEADERTYPE *pBuffer)
{
    static unsigned int k;

    if(!pBuffer) {
        CLog::Log(LOGDEBUG, "CDVDVideoCodecOmx::OnOmxDecFillBufferDone (%u) pBuffer NULL\n", k++);
        return;
    }
    CDVDVideoCodecOmx *pThis = reinterpret_cast<CDVDVideoCodecOmx*>(pClientThis);
    CLog::Log(LOGDEBUG, "OnOmxDecFillBufferDone (%u) pBuf: %p Len: %lu nFlags: %lx\n", k++, pBuffer->pBuffer, pBuffer->nFilledLen, pBuffer->nFlags);

    if(!pThis->m_bRunning)
        return;
#if 0
    static FILE *f;
    if(!pBuffer->nFlags) {
        if(!f) {
            if(!(f = fopen("video.yuv", "wb"))) {
                CLog::Log(LOGDEBUG, "Error opening file\n");
                exit(1);
            }
        }
        if(fwrite(pBuffer->pBuffer, pBuffer->nFilledLen, 1, f) != 1) {
            CLog::Log(LOGDEBUG, "Error writing file\n");
            exit(1);
        }
    }
#endif

    if(!pBuffer->nFlags) {
        // Put buffer to decoded queue
        pThis->m_VideoDecodedQueue->Put((void *)&pBuffer, 0);
        // Fill OMX Output Queue
        pThis->FillOutputBuffer();
#if 0
        pThis->m_bHasFrame = true;
        // Save picture parameters
        pThis->m_pPictureFrameData = pBuffer->pBuffer;
        pThis->m_sCurPts = pBuffer->nTimeStamp;

        pThis->m_OutBufferSemaphore->Increment();
#endif
        CLog::Log(LOGDEBUG, "OnOmxDecFillBufferDone done\n");
    }
}

void CDVDVideoCodecOmx::OnOmxEventCompletion(void *pClientThis, OmxDecState *pOmxDecState, EOMXEvent eOmxEvent, unsigned int uData1, unsigned int uData2)
{
    CDVDVideoCodecOmx *pThis = reinterpret_cast<CDVDVideoCodecOmx*>(pClientThis);
    OMX_VERSIONTYPE vOMX;
    OMX_PARAM_PORTDEFINITIONTYPE oDecInputPortDef;
    OMX_ERRORTYPE err;
    CLog::Log(LOGDEBUG, "OnOmxEventCompletion Event: %d This:%p\n", eOmxEvent, pClientThis);
    switch(eOmxEvent) {
        case eOMXEventStateChanged:
            CLog::Log(LOGDEBUG, "OnOmxEventCompletion Set State Changed\n");
            pOmxDecState->pStateDoneEvent->Set();
            break;
        case eOMXEventFlush:
            pThis->m_uOmxAbortPortCount++;
            CLog::Log(LOGDEBUG, "Abort Event Count: %u\n", pThis->m_uOmxAbortPortCount);
            if(pThis->m_uOmxAbortPortCount == 1) {
                CLog::Log(LOGDEBUG, "Set Abort Event\n");
                pOmxDecState->pStateDoneEvent->Set();
                pThis->m_uOmxAbortPortCount = 0;
            }
            break;
        case eOMXEventEndOfStream:
            CLog::Log(LOGDEBUG, "OnOmxEventCompletion End of Stream\n");
            break;
        case eOMXEventPortSettingsChanged:
            // Required OMX version
            vOMX.s.nVersionMajor = 1;
            vOMX.s.nVersionMinor = 1;
            vOMX.s.nRevision = 2;
            vOMX.s.nStep = 0;
            memset((void *)&oDecInputPortDef, sizeof(oDecInputPortDef), 0);
            oDecInputPortDef.nSize = sizeof(oDecInputPortDef);
            oDecInputPortDef.nVersion.nVersion = vOMX.nVersion;
            oDecInputPortDef.nPortIndex = 0;
            err = OMX_GetParameter(pThis->m_hComponentHandle, OMX_IndexParamPortDefinition, &oDecInputPortDef);
            if(err != OMX_ErrorNone) {
                CLog::Log(LOGDEBUG, "OMX_GetParameter() failed = %X\n", err);
                return;
            }
            CLog::Log(LOGDEBUG, "New Parameters - pNativeRender: %p\n", oDecInputPortDef.format.video.pNativeRender);
            CLog::Log(LOGDEBUG, "New Parameters - nFrameWidth: %lu\n", oDecInputPortDef.format.video.nFrameWidth);
            CLog::Log(LOGDEBUG, "New Parameters - nFrameHeight: %lu\n", oDecInputPortDef.format.video.nFrameHeight);
            CLog::Log(LOGDEBUG, "New Parameters - nStride: %ld\n", oDecInputPortDef.format.video.nStride);
            CLog::Log(LOGDEBUG, "New Parameters - nSliceHeight: %lu\n", oDecInputPortDef.format.video.nSliceHeight);
            CLog::Log(LOGDEBUG, "New Parameters - nBitrate: %lu\n", oDecInputPortDef.format.video.nBitrate);
            CLog::Log(LOGDEBUG, "New Parameters - xFramerate: %lu\n", oDecInputPortDef.format.video.xFramerate);
            CLog::Log(LOGDEBUG, "New Parameters - bFlagErrorConcealment: %d\n", oDecInputPortDef.format.video.bFlagErrorConcealment);
            CLog::Log(LOGDEBUG, "New Parameters - eCompressionFormat: %d\n", oDecInputPortDef.format.video.eCompressionFormat);
            CLog::Log(LOGDEBUG, "New Parameters - eColorFormat: %d\n", oDecInputPortDef.format.video.eColorFormat);
            CLog::Log(LOGDEBUG, "New Parameters - pNativeWindow: %p\n", oDecInputPortDef.format.video.pNativeWindow);
            break;
        case eOMXEventPortEnable:
            CLog::Log(LOGDEBUG, "OnOmxEventCompletion Port Enable\n");
            pOmxDecState->pStateDoneEvent->Set();
            break;
        case eOMXEventPortDisable:
            CLog::Log(LOGDEBUG, "OnOmxEventCompletion Port Disable\n");
            pOmxDecState->pStateDoneEvent->Set();
            break;
        case eOMXEventFirstFrameDisplayed:
            CLog::Log(LOGDEBUG, "OnOmxEventCompletion First Frame Displayed\n");
            //pThis->m_bSendTunneledFirstFrame = true;
            g_bFirstFrameDecoded = true;
            break;
        case eOMXEventUndefined:
            break;
    }
}

#endif
