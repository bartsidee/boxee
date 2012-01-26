#pragma once

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

#include "system.h"

#ifdef HAS_OPENMAX

#include "DVDVideoCodec.h"
//#include "cores/ffmpeg/DllAvCodec.h"
#include "utils/Event.h"
#include "utils/CriticalSection.h"
#include "DVDCodecs/DVDOmxUtils.h" 
#include <string>


#define MAX_IN_BUFFER_COUNT 20
#define MAX_OUT_BUFFER_COUNT 20

class CDVDVideoCodecOmx : public CDVDVideoCodec
{
public:
    CDVDVideoCodecOmx();
    virtual ~CDVDVideoCodecOmx();
    virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
    virtual void Dispose();
    virtual int Decode(BYTE* pData, int iSize, double pts);
    virtual void Reset();
    virtual bool GetPicture(DVDVideoPicture* pDvdVideoPicture);
    virtual void SetDropState(bool bDrop);
    virtual const char* GetName() { return m_name.c_str(); }; // m_name is never changed after open

protected:
    union pts_union
    {
      double  pts_d;
      int64_t pts_i;
    };

    signed long long pts_dtoi(double pts);
    double pts_itod(signed long long pts);

    //void GetVideoAspect(AVCodecContext* CodecContext, unsigned int& iWidth, unsigned int& iHeight);
    static void OnOmxDecEmptyBufferDone(void *pClientThis, OMX_BUFFERHEADERTYPE *pBuffer);
    static void OnOmxDecFillBufferDone(void *pClientThis, OMX_BUFFERHEADERTYPE *pBuffer);
    static void OnOmxEventCompletion(void *pClientThis, OmxDecState *pOmxDecState, EOMXEvent eOmxEvent, unsigned int uData1, unsigned int uData2);
    static void DecoderBufferCallback(void *pClientThis, OMX_BUFFERHEADERTYPE *pOmxBuffer);

    bool FillInputBuffer(BYTE* pData, int iSize, signed long long sPts);
    bool FillOutputBuffer(void);
    bool ProcessExtraData(void *pData, unsigned int iSize);
    BYTE *GetNal(BYTE uNalType);

    int m_iPictureWidth;
    int m_iPictureHeight;
    int m_iScreenWidth;
    int m_iScreenHeight;
    std::string m_name;

    EOmxDecoderTypes m_eVideoDecoderType;
    void* m_hComponentHandle;
    void* m_hVidRendComponentHandle;
    void* m_hClockComponentHandle;
    char m_szComponentName[128];
    char m_szVidrendComponentName[128];
    char m_szClockComponentName[128];
    OmxDecState m_OmxDecState;
    OmxDecState m_OmxRendererState;
    OmxDecState m_OmxClockState;
    OmxCallBacks m_OmxvideodecCallBacks;
    OMX_BUFFERHEADERTYPE *m_pOmxInBuffers[MAX_IN_BUFFER_COUNT];
    OMX_BUFFERHEADERTYPE *m_pOmxOutBuffers[MAX_OUT_BUFFER_COUNT];
    OMX_BUFFERHEADERTYPE *m_pLastSentOmxBuffer;
    COmxUtils m_oOmxUtils;
    unsigned char m_dummy_mem[2048];
    unsigned int m_uOmxInBufferCount;
    unsigned int m_uOmxInBufferSize;
    unsigned int m_uOmxCurInBuffer;
    unsigned int m_uOmxOutBufferCount;
    unsigned int m_uOmxOutBufferSize;
    CEvent m_oEventOmxSetStateDone;
    CEvent m_oEventOmxEndOfStream;
    CEvent m_oEventOmxAbortDone;
    CSemaphore *m_InBufferSemaphore;
    CSemaphore *m_OutBufferSemaphore;
    CQueue *m_VideoFreeQueue;
    CQueue *m_VideoDecodedQueue;
    unsigned int m_uOmxInOutstandingBuffer;
    unsigned int m_uOmxAbortPortCount;
    bool m_bAborting;
    bool m_bRunning;
    bool m_bHasFrame;
    bool m_bMP4_h264_FileMode;
    bool m_bOmxInitialized;
    BYTE *m_pPictureFrameData;
    int m_iExtraDataSize;
    BYTE *m_pExtraData;
    unsigned int m_uExtraData_Size;
    signed long long m_sCurPts;
    double m_dFrameTime;
    double m_dCurPts;
    BYTE *m_pDataSave;
    int m_iSizeSave;
    double m_dPtsSave;
    bool m_bSendTunneledFirstFrame;
    double m_fTimeError;
};

#endif
