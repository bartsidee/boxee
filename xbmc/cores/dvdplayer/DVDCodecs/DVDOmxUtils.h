#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "utils/Event.h"
#include "utils/CriticalSection.h"

class CSemaphore {
public:
    CSemaphore(int sCountMax, int sCountInitial);
    ~CSemaphore();
    bool Increment();
    bool Decrement(int sTimeoutMs = -1);

private:
    CCriticalSection *m_pMutex;
    CEvent *m_pEvent;
    int m_sCount;
    int m_sCountMax;
};

class CQueue {
public:
    CQueue(int sQueueSize, int sElemSize);
    ~CQueue();
    bool Put(void *pElement, int sTimeoutMs);
    bool Get(void *pElement, int sTimeoutMs);
    int GetSize();
    bool Peek(void *pElement, int *psElements);
    bool WaitForNotEmpty(int sTimeoutMs);
private:
    int                 m_sNextGet;
    int                 m_sNextPut;
    unsigned char     * m_pQueueData;
    CSemaphore        * m_pSemGet;
    CSemaphore        * m_pSemPut;
    CCriticalSection  * m_pMutex;
    CEvent            * m_pGotElementEvent;
    int                 m_sItems;
    int                 m_sItemSize;
    int                 m_sQueueSize;
};

struct OMX_BUFFERHEADERTYPE;
struct OmxDecState;

enum EOMXEvent {
    eOMXEventStateChanged,
    eOMXEventFlush,
    eOMXEventPortEnable,
    eOMXEventPortDisable,
    eOMXEventEndOfStream,
    eOMXEventPortSettingsChanged,
    eOMXEventFirstFrameDisplayed,
    eOMXEventUndefined
};

typedef void (*ON_OMX_DEC_EMPTY_BUFFER_DONE)(void *pClientThis, OMX_BUFFERHEADERTYPE *pBuffer);
typedef void (*ON_OMX_DEC_FILL_BUFFER_DONE)(void *pClientThis, OMX_BUFFERHEADERTYPE *pBuffer);
typedef void (*ON_OMX_EVENT_COMPLETION)(void *pClientThis, OmxDecState *pOmxDecState, EOMXEvent eOmxEvent, unsigned int uData1, unsigned int uData2);

struct OmxCallBacks {
    ON_OMX_DEC_EMPTY_BUFFER_DONE OnOmxDecEmptyBufferDone;
    ON_OMX_DEC_FILL_BUFFER_DONE  OnOmxDecFillBufferDone;
    ON_OMX_EVENT_COMPLETION      OnOmxEventCompletion;
};

struct OmxDecState {
    OmxCallBacks *pCallBacks;
    void         *pClientThis;
    void         *pNvEvent;
    CEvent       *pStateDoneEvent;
};

enum EOmxDecoderTypes {
    eAudioDecoderAAC,
    eAudioDecoderADTS,
    eAudioDecoderEAACP,
    eAudioDecoderBSAC,
    eAudioDecoderMP3,
    eAudioDecoderWAV,
    eAudioDecoderWMA,
    eAudioDecoderWMAPRO,
    eAudioDecoderWMALOSSLESS,
    eAudioDecoderVORBIS,
    eAudioDecoderAMR,
    eAudioDecoderAMRWB,
    eVideoDecoderMPEG4,
    eVideoDecoderMPEG4Ext,
    eVideoDecoderSorenson,
    eVideoDecoderMPEG2,
    eVideoDecoderVP6,
    eVideoDecoderH264,
    eVideoDecoderH264Ext,
    eVideoDecoderVC1,
    eAudioRenderer,
    eVideoRendererVGA,
    eVideoRendererHDMI,
    eClockComponent
};

#if 0
// Bellagio Codec names
#define OMX_DECODER_COMPONENT_NAME_LIST \
    "OMX.st.audio_decoder.aac", \
    "OMX.st.audio_decoder.adts", \
    "OMX.st.audio_decoder.eaacp", \
    "OMX.st.audio_decoder.bsac", \
    "OMX.st.audio_decoder.mp3", \
    "OMX.st.audio_decoder.wav", \
    "OMX.st.audio_decoder.wma", \
    "OMX.st.audio_decoder.wmapro", \
    "OMX.st.audio_decoder.wmalossless", \
    "OMX.st.audio_decoder.vorbis", \
    "OMX.st.audio_decoder.amr", \
    "OMX.st.audio_decoder.amrwb", \
    "OMX.st.video_decoder.mpeg4", \
    "OMX.st.video_decoder.mpeg4ext", \
    "OMX.st.video_decoder.sorenson", \
    "OMX.st.video_decoder.mpeg2", \
    "OMX.st.video_decoder.vp6", \
    "OMX.st.video_decoder.avc", \
    "OMX.st.video_decoder.avc", \
    "OMX.st.video_decoder.vc1"
    "OMX.st.audio.render", \
    "OMX.st.std.iv_renderer.overlay.yuv420", \
    "OMX.st.render.hdmi.overlay.yuv420", \
    "OMX.st.clock.component"
#else
// NVIDIA Codecs names
#define OMX_DECODER_COMPONENT_NAME_LIST \
    "OMX.Nvidia.aac.decoder", \
    "OMX.Nvidia.adts.decoder", \
    "OMX.Nvidia.eaacp.decoder", \
    "OMX.Nvidia.bsac.decoder", \
    "OMX.Nvidia.mp3.decoder", \
    "OMX.Nvidia.wav.decoder", \
    "OMX.Nvidia.wma.decoder", \
    "OMX.Nvidia.wmapro.decoder", \
    "OMX.Nvidia.wmalossless.decoder", \
    "OMX.Nvidia.vorbis.decoder", \
    "OMX.Nvidia.amr.decoder", \
    "OMX.Nvidia.amrwb.decoder", \
    "OMX.Nvidia.mp4.decode", \
    "OMX.Nvidia.mp4ext.decode", \
    "OMX.Nvidia.sorenson.decode", \
    "OMX.Nvidia.mpeg2v.decode", \
    "OMX.Nvidia.vp6.decode", \
    "OMX.Nvidia.h264.decode", \
    "OMX.Nvidia.h264ext.decode", \
    "OMX.Nvidia.vc1.decode", \
    "OMX.Nvidia.audio.render", \
    "OMX.Nvidia.std.iv_renderer.overlay.yuv420", \
    "OMX.Nvidia.render.hdmi.overlay.yuv420", \
    "OMX.Nvidia.clock.component"
#endif

class COmxUtils {
public:
    bool OmxInit();
    bool OmxDeinit();
    const char *OmxGetComponentName(EOmxDecoderTypes eType);

};
