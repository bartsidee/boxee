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

#include <dxva.h>
#include <dxva2api.h>
#include "D3DResource.h"
#include "DVDVideoCodec.h"
#include "DXVADecoderH264.h"
#include "TlibavcodecExt.h"
#include "cores/ffmpeg/DllAvCodec.h"
#include "cores/ffmpeg/DllAvFormat.h"
#include "cores/ffmpeg/DllSwScale.h"

#include <list>


#define MAX_SUPPORTED_MODE			5

typedef struct
{
  const int			  PicEntryNumber;
  const UINT			PreferedConfigBitstream;
  const GUID*			Decoder[MAX_SUPPORTED_MODE];
  const WORD			RestrictedMode[MAX_SUPPORTED_MODE];
} DXVA_PARAMS;

typedef struct
{
  REFERENCE_TIME	rtStart;
  REFERENCE_TIME	rtStop;
} B_FRAME;

class CDVDVideoCodecDXVA : public CDVDVideoCodec, public TlibavcodecExt, public ID3DResource
{
public:
  CDVDVideoCodecDXVA();
  virtual ~CDVDVideoCodecDXVA();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);  
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize, double pts, double dts);
  virtual void Reset();
  virtual bool GetPicture(DVDVideoPicture* pDvdVideoPicture);
  virtual void SetDropState(bool bDrop);
  virtual const char* GetName() { return m_name.c_str(); }; // m_name is never changed after open
  
  // MPC functions
  DXVA2_ConfigPictureDecode*	GetDXVA2Config() { return &m_DXVA2Config; };
  inline int					        GetPCIVendor()  { return m_nPCIVendor; };
  inline AVCodecContext*		  GetAVCtx()		 { return m_pCodecContext; };
  HRESULT                     Deliver(int nIndex, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd);
  void                        GetSufrace(int nIndex, IDirect3DSurface9** ppSurface) { *ppSurface = m_ppRTSurfaceArray[nIndex]; }
  inline REFERENCE_TIME		    GetAvrTimePerFrame() { return m_rtAvrTimePerFrame; };
  void                        ReorderBFrames(REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop);

  
protected:

  int PictWidthRounded();
  int PictHeightRounded();
  BOOL IsSupportedDecoderConfig(const D3DFORMAT nD3DFormat, 
  const DXVA2_ConfigPictureDecode& config, 
  bool& bIsPrefered);
  HRESULT FindDXVA2DecoderConfiguration(IDirectXVideoDecoderService *pDecoderService,
  const GUID& guidDecoder, 
  DXVA2_ConfigPictureDecode *pSelectedConfig,
  BOOL *pbFoundDXVA2Configuration);
  void	FillInVideoDescription(DXVA2_VideoDesc *pDesc);
  BOOL IsSupportedDecoderMode(const GUID& mode);
  HRESULT ConfigureDXVA();
  void GetVideoAspect(AVCodecContext* CodecContext, unsigned int& iWidth, unsigned int& iHeight);
  void AllocExtraData(void* ExtraData, int ExtraDataSize);
  void DetectVideoCard();
  void OverideDefaults();
  bool AllocateResources();
  void ReleaseResources();
  void AllocateRenderSurfaces();
  void DisableDecoderPostProcessing(IDirectXVideoDecoder* pDirectXVideoDec);

  // === ID3DResource
  virtual void OnDestroyDevice();
  virtual void OnCreateDevice();
  virtual void OnLostDevice();
  virtual void OnResetDevice();

  static void LogLibAVCodec(void* par,int level,const char *fmt,va_list valist);

  AVCodecContext* m_pCodecContext;

  int m_iPictureWidth;
  int m_iPictureHeight;

  int m_iScreenWidth;
  int m_iScreenHeight;

  DllAvCore  m_dllAvCore;
  DllAvCodec m_dllAvCodec;
  DllAvUtil  m_dllAvUtil;
  std::string m_name;
  bool m_bResourcesAllocated;
  CCriticalSection m_resourceSection;

  // DXVA stuff
  HMODULE                       m_hDXVA2Library;
  CDXVADecoder*                 m_pDXVADecoder;
  IDirectXVideoDecoder*	        m_pDirectXVideoDec;
  CodecID                       m_CodecID;
  DXVA2_VideoDesc               m_VideoDesc;
  DXVA_PARAMS*                  m_pDXVAparams;
  DXVA2_ConfigPictureDecode		  m_DXVA2Config;
  GUID									        m_DXVADecoderGUID;
  IDirect3DDeviceManager9*      m_pDeviceManager;
  UINT                          m_nResetToken;
  IDirectXVideoDecoderService*	m_pDecoderService;
  IDirect3DSurface9**		        m_ppRTSurfaceArray;
  UINT					                m_nSurfaceArrayCount;
  int										        m_nPCIVendor;
  int										        m_nPCIDevice;
  LARGE_INTEGER						      m_VideoDriverVersion;
  std::string									  m_strDeviceDescription;
  bool                          m_bReorderBFrame;
  std::list<UINT>               m_lBuffersIndices;

  // Reference time
  REFERENCE_TIME							  m_rtAvrTimePerFrame;
  B_FRAME									      m_BFrames[2];
  int										        m_nPosB;

  // last frame
  int                           m_nLastFrame;
  REFERENCE_TIME                m_rtDisplayFrameStart;
  REFERENCE_TIME                m_rtDisplayFrameEnd;
};

