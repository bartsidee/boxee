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

#ifdef HAS_DX

#include <initguid.h>
#include "system.h"
#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include "DVDVideoCodecDXVA.h"
#include "DVDDemuxers/DVDDemux.h"
#include "DVDStreamInfo.h"
#include "DVDClock.h"
#include "DVDCodecs/DVDCodecs.h"
#include "../../../../utils/Win32Exception.h"
#include "utils/CPUInfo.h"
#include "AdvancedSettings.h"
#include "GUISettings.h"
#include "utils/log.h"
#include "Util.h"
#include "WindowingFactory.h"

extern "C"
{
  #include "FfmpegContext.h"
}

#include <math.h>
#define RINT lrint

#include "cores/VideoRenderers/RenderManager.h" 

typedef HRESULT (__stdcall *PFNDXVA2CREATEDEVICEMANAGER)(UINT* pResetToken, IDirect3DDeviceManager9** ppDeviceManager);

// Additionnal DXVA GUIDs

// Intel ClearVideo VC1 bitstream decoder
DEFINE_GUID(DXVA_Intel_VC1_ClearVideo, 0xBCC5DB6D, 0xA2B6,0x4AF0,0xAC,0xE4,0xAD,0xB1,0xF7,0x87,0xBC,0x89);

// Intel ClearVideo H264 bitstream decoder
DEFINE_GUID(DXVA_Intel_H264_ClearVideo, 0x604F8E68, 0x4951,0x4C54,0x88,0xFE,0xAB,0xD2,0x5C,0x15,0xB3,0xD6);

#define NV_DXVA_PPCONTROL   0xFFFFE700

// DXVA modes supported for Mpeg2	TODO
DXVA_PARAMS		DXVA_Mpeg2 =
{
  14,		// PicEntryNumber
  1,		// PreferedConfigBitstream
  { &DXVA_ModeMPEG2_A,			&DXVA_ModeMPEG2_C,				&GUID_NULL },
  { DXVA_RESTRICTED_MODE_MPEG2_A,  DXVA_RESTRICTED_MODE_MPEG2_C,	 0 }
};

// DXVA modes supported for H264
DXVA_PARAMS		DXVA_H264 =
{
  16,		// PicEntryNumber
  2,		// PreferedConfigBitstream
  { &DXVA2_ModeH264_E, &DXVA2_ModeH264_F, &DXVA_Intel_H264_ClearVideo, &GUID_NULL },
  { DXVA_RESTRICTED_MODE_H264_E,	 0}
};

DXVA_PARAMS		DXVA_H264_VISTA =
{
  22,		// PicEntryNumber
  2,		// PreferedConfigBitstream
  { &DXVA2_ModeH264_E, &DXVA2_ModeH264_F, &DXVA_Intel_H264_ClearVideo, &GUID_NULL },
  { DXVA_RESTRICTED_MODE_H264_E,	 0}
};

// DXVA modes supported for VC1
DXVA_PARAMS		DXVA_VC1 =
{
  14,		// PicEntryNumber
  1,		// PreferedConfigBitstream
  { &DXVA2_ModeVC1_D,				&GUID_NULL },
  { DXVA_RESTRICTED_MODE_VC1_D,	 0}
};

typedef struct
{
  const enum CodecID	nFFCodec;
  const DXVA_PARAMS*	DXVAModes;

  int					DXVAModeCount()		
  {
    if (!DXVAModes) return 0;
    for (int i=0; i<MAX_SUPPORTED_MODE; i++)
    {
      if (DXVAModes->Decoder[i] == &GUID_NULL) return i;
    }
    return MAX_SUPPORTED_MODE;
  }
} FFMPEG_CODECS;

typedef struct
{
  IDirect3DSurface9*	m_pSurface;
  DWORD						    m_dwSurfaceId;
} DXVA2Sample;


CDVDVideoCodecDXVA::CDVDVideoCodecDXVA() : CDVDVideoCodec()
{
  m_hDXVA2Library = NULL;
  m_pCodecContext = NULL;

  m_iPictureWidth = 0;
  m_iPictureHeight = 0;

  m_iScreenWidth = 0;
  m_iScreenHeight = 0;

  m_pDXVAparams = NULL;
  m_pDeviceManager = NULL;
  m_pDecoderService = NULL;
  m_nResetToken = 0;
  m_ppRTSurfaceArray = NULL;
  m_nSurfaceArrayCount = 0;
  m_DXVADecoderGUID		= GUID_NULL;
  m_pDXVADecoder = NULL;
  m_pDirectXVideoDec = NULL;
  m_rtAvrTimePerFrame = 1;
  memset (&m_BFrames, 0, sizeof(m_BFrames));
  m_nPosB	= 1;
  m_bReorderBFrame		= true;

  m_rtDisplayFrameStart = 0;
  m_rtDisplayFrameEnd = 0;
  m_nLastFrame = 0;
  m_bIsHarwareCodec = true;

  m_bResourcesAllocated = false;

  ff_avcodec_default_get_buffer		= avcodec_default_get_buffer;
  ff_avcodec_default_release_buffer	= avcodec_default_release_buffer;
  ff_avcodec_default_reget_buffer		= avcodec_default_reget_buffer;
}

CDVDVideoCodecDXVA::~CDVDVideoCodecDXVA()
{
  Dispose();
}

bool CDVDVideoCodecDXVA::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  AVCodec* pCodec;

  if(hints.codec != CODEC_ID_H264)
    return false;

  m_iPictureWidth = hints.width;
  m_iPictureHeight = hints.height;

  m_CodecID = hints.codec;

  float fFrameRate = 0;
  m_rtAvrTimePerFrame = 0;
  if (hints.fpsrate && hints.fpsscale)
  {
    fFrameRate = (float)hints.fpsrate / hints.fpsscale; 
    m_rtAvrTimePerFrame = (REFERENCE_TIME)(10000000i64 / fFrameRate);
    CLog ::Log(LOGDEBUG, "m_rtAvrTimePerFrame = %I64d", m_rtAvrTimePerFrame);
  }
  else
    m_rtAvrTimePerFrame = 400000;

  // register into the main device so we can free resources when the device is reset
  g_Windowing.Register(this);

  if (!m_dllAvUtil.Load() || !m_dllAvCore.Load() || !m_dllAvCodec.Load() ) return false;

  m_dllAvCodec.avcodec_register_all();
  
  m_pCodecContext = m_dllAvCodec.avcodec_alloc_context();
  OverideDefaults();

  pCodec = NULL;
  pCodec = m_dllAvCodec.avcodec_find_decoder(hints.codec);

  if(pCodec == NULL)
  {
    CLog::Log(LOGDEBUG,"CDVDVideoCodecDXVA::Open() Unable to find codec %d", hints.codec);
    return false;
  }

  CLog::Log(LOGNOTICE,"CDVDVideoCodecDXVA::Open() Using codec: %s",pCodec->long_name ? pCodec->long_name : pCodec->name);

  m_pCodecContext->opaque = (void*)this;
  m_pCodecContext->debug_mv = 0;
  m_pCodecContext->debug = 0;
  m_pCodecContext->workaround_bugs = FF_BUG_AUTODETECT;
  m_pCodecContext->intra_matrix			= (uint16_t*)calloc(sizeof(uint16_t),64);
  m_pCodecContext->inter_matrix			= (uint16_t*)calloc(sizeof(uint16_t),64);
  m_pCodecContext->error_concealment		= FF_EC_DEBLOCK | FF_EC_GUESS_MVS;
  m_pCodecContext->error_recognition		= FF_ER_CAREFUL;
  m_pCodecContext->idct_algo				= FF_IDCT_AUTO;
  m_pCodecContext->skip_loop_filter		= (AVDiscard)AVDISCARD_DEFAULT;
  //m_pCodecContext->postgain				= 1.0f;

  m_pCodecContext->opaque					= this;
  AllocExtraData(hints.extradata, hints.extrasize);
  ConnectTo (m_pCodecContext); 

  av_log_set_callback(LogLibAVCodec);
  
  DetectVideoCard();

  m_bReorderBFrame = false;

  /* some decoders (eg. dv) do not know the pix_fmt until they decode the
   * first frame. setting to -1 avoid enabling DR1 for them.
   */
  m_pCodecContext->pix_fmt = (PixelFormat) - 1;  

  if (pCodec->id != CODEC_ID_H264 && pCodec->capabilities & CODEC_CAP_DR1)
    m_pCodecContext->flags |= CODEC_FLAG_EMU_EDGE;

  // Hack to correct wrong frame rates that seem to be generated by some
  // codecs
  if (m_pCodecContext->time_base.den > 1000 && m_pCodecContext->time_base.num == 1)
    m_pCodecContext->time_base.num = 1000;

  // if we don't do this, then some codecs seem to fail.
  m_pCodecContext->coded_height = hints.height;
  m_pCodecContext->coded_width = hints.width;

  // set acceleration
  m_pCodecContext->dsp_mask = FF_MM_FORCE | FF_MM_MMX | FF_MM_MMXEXT | FF_MM_SSE;
  
  // set any special options
  for(CDVDCodecOptions::iterator it = options.begin(); it != options.end(); it++)
  {
    m_dllAvCodec.av_set_string(m_pCodecContext, it->m_name.c_str(), it->m_value.c_str());
  }

  if (m_dllAvCodec.avcodec_open(m_pCodecContext, pCodec) < 0)
  {
    CLog::Log(LOGDEBUG,"CDVDVideoCodecDXVA::Open() Unable to open codec");
    return false;
  }

  if(pCodec->name)
    m_name = CStdString("DXVA-") + pCodec->name;
  else
    m_name = "DXVA";

  int		nCompat;
  if (hints.codec == CODEC_ID_H264)
  {
    nCompat = FFH264CheckCompatibility (PictWidthRounded(), PictHeightRounded(), m_pCodecContext, (BYTE*)m_pCodecContext->extradata, m_pCodecContext->extradata_size, m_nPCIVendor, m_VideoDriverVersion);
  }

  //if we didn't get the width and height from the hints, get it from the context
  if(m_iPictureWidth == 0 || m_iPictureHeight == 0)
  {
    int mbWidth, mbHeight;
    FFH264GetMBSize(m_pCodecContext, &mbWidth, &mbHeight);
    m_iPictureWidth = mbWidth * 16;
    m_iPictureHeight = mbHeight * 16;
  }

  if(!SUCCEEDED(ConfigureDXVA()))
  {
    CLog::Log(LOGDEBUG,"CDVDVideoCodecDXVA::Open() ConfigureDXVA2 failed");
    return false;
  }

  // Force single thread for DXVA !
  FFSetThreadNumber(m_pCodecContext, 1);

  return true;
}

void CDVDVideoCodecDXVA::Dispose()
{
  if (m_pCodecContext)
  {
    if (m_pCodecContext->codec) m_dllAvCodec.avcodec_close(m_pCodecContext);
    if (m_pCodecContext->extradata)
    {
      free(m_pCodecContext->extradata);
      m_pCodecContext->extradata = NULL;
      m_pCodecContext->extradata_size = 0;
    }

    if(m_pCodecContext->intra_matrix)
    {
      free(m_pCodecContext->intra_matrix);
      m_pCodecContext->intra_matrix = NULL;
    }

    if(m_pCodecContext->inter_matrix)
    {
      free(m_pCodecContext->inter_matrix);
      m_pCodecContext->inter_matrix = NULL;
    }

    m_dllAvUtil.av_free(m_pCodecContext);
    m_pCodecContext = NULL;
  }
  
  m_dllAvCodec.Unload();
  m_dllAvCore.Unload();
  m_dllAvUtil.Unload();

  ReleaseResources();

  SAFE_RELEASE(m_pDeviceManager);
  SAFE_DELETE_ARRAY(m_ppRTSurfaceArray);

  if (m_hDXVA2Library != NULL)
  {
    FreeLibrary(m_hDXVA2Library);
    m_hDXVA2Library = NULL;
  }

  m_nSurfaceArrayCount = 0;

  g_Windowing.Unregister(this);
}

void CDVDVideoCodecDXVA::SetDropState(bool bDrop)
{
}

union pts_union
{
  double  pts_d;
  int64_t pts_i;
};

static int64_t pts_dtoi(double pts)
{
  pts_union u;
  u.pts_d = pts;
  return u.pts_i;
}

static double pts_itod(int64_t pts)
{
  pts_union u;
  u.pts_i = pts;
  return u.pts_d;
}

int CDVDVideoCodecDXVA::Decode(BYTE* pData, int iSize, double pts, double dts)
{
  int iGotPicture = 0, len = 0, result = 0;

  if (!m_pCodecContext) 
    return VC_ERROR;

  HRESULT			hr;
  BYTE*			pDataIn = pData;
  int				nSize = iSize;
  REFERENCE_TIME	rtStart = _I64_MIN;
  REFERENCE_TIME	rtStop  = _I64_MIN;

  // make sure we're not in the middle of resource allocation
  CSingleLock lock(m_resourceSection);

  m_nLastFrame = -1;

  if(!m_bResourcesAllocated)
  {
    AllocateResources();
  }

  if(!m_pDXVADecoder)
    return VC_ERROR;

  rtStart = pts;
  rtStop = 0;

  /*
  if(rtStart < 0)
    rtStart = 0;
    */

  if (rtStop <= rtStart)
    rtStop = rtStart + m_rtAvrTimePerFrame;
  
  m_pCodecContext->reordered_opaque  = rtStart;
  //m_pCodecContext->reordered_opaque2 = rtStop;

  if (m_pCodecContext->has_b_frames)
  {
    m_BFrames[m_nPosB].rtStart	= rtStart;
    m_BFrames[m_nPosB].rtStop	= rtStop;
    m_nPosB						= 1-m_nPosB;
  }

  //UpdateAspectRatio();
  hr = m_pDXVADecoder->DecodeFrame (pDataIn, nSize, rtStart, rtStop);

  if(FAILED(hr))
    result = VC_ERROR;
  else
    result = VC_PICTURE | VC_BUFFER;

  return result;
}

void CDVDVideoCodecDXVA::Reset()
{
  try 
  {
    m_dllAvCodec.avcodec_flush_buffers(m_pCodecContext);

    m_nPosB = 1;
    memset (&m_BFrames, 0, sizeof(m_BFrames));

    if (m_pDXVADecoder)
      m_pDXVADecoder->Flush();
  } 
  catch (win32_exception e) 
  {
    e.writelog(__FUNCTION__);
  }
}

bool CDVDVideoCodecDXVA::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  GetVideoAspect(m_pCodecContext, pDvdVideoPicture->iDisplayWidth, pDvdVideoPicture->iDisplayHeight);

  if(m_pCodecContext->coded_width  && m_pCodecContext->coded_width  < m_pCodecContext->width
                                   && m_pCodecContext->coded_width  > m_pCodecContext->width  - 10)
    pDvdVideoPicture->iWidth = m_pCodecContext->coded_width;
  else
  pDvdVideoPicture->iWidth = m_pCodecContext->width;

  if(m_pCodecContext->coded_height && m_pCodecContext->coded_height < m_pCodecContext->height
                                   && m_pCodecContext->coded_height > m_pCodecContext->height - 10)
    pDvdVideoPicture->iHeight = m_pCodecContext->coded_height;
  else
    pDvdVideoPicture->iHeight = m_pCodecContext->height;

  for (int i = 0; i < 4; i++)
    pDvdVideoPicture->data[i]      = NULL;
  for (int i = 0; i < 4; i++)
    pDvdVideoPicture->iLineSize[i] = 0;

  pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED; 

  if(m_rtDisplayFrameStart == 0)
    pDvdVideoPicture->pts = DVD_NOPTS_VALUE;
  else
    pDvdVideoPicture->pts = m_rtDisplayFrameStart;
  pDvdVideoPicture->format = DVDVideoPicture::FMT_DXVA;

  // Did we get a frame?
  if(m_nLastFrame != -1)
  {
    pDvdVideoPicture->opaque = &(m_ppRTSurfaceArray[m_nLastFrame]);
  }
  else
  {
    pDvdVideoPicture->iFlags |= DVP_FLAG_DROPPED; 
    pDvdVideoPicture->opaque = NULL;
  }

  return true;
}

/*
 * Calculate the height and width this video should be displayed in
 */
void CDVDVideoCodecDXVA::GetVideoAspect(AVCodecContext* pCodecContext, unsigned int& iWidth, unsigned int& iHeight)
{
  double aspect_ratio;

  /* XXX: use variable in the frame */
  if (pCodecContext->sample_aspect_ratio.num == 0) aspect_ratio = 0;
  else aspect_ratio = av_q2d(pCodecContext->sample_aspect_ratio) * pCodecContext->width / pCodecContext->height;

  if (aspect_ratio <= 0.0) aspect_ratio = (float)pCodecContext->width / (float)pCodecContext->height;

  /* XXX: we suppose the screen has a 1.0 pixel ratio */ // CDVDVideo will compensate it.
  iHeight = pCodecContext->height;
  iWidth = ((int)RINT(pCodecContext->height * aspect_ratio)) & -3;
  if (iWidth > (unsigned int)pCodecContext->width)
  {
    iWidth = pCodecContext->width;
    iHeight = ((int)RINT(pCodecContext->width / aspect_ratio)) & -3;
  }
}

int CDVDVideoCodecDXVA::PictWidthRounded()
{
  // Picture height should be rounded to 16 for DXVA
  return ((m_iPictureWidth + 15) / 16) * 16;
}

int CDVDVideoCodecDXVA::PictHeightRounded()
{
  // Picture height should be rounded to 16 for DXVA
  return ((m_iPictureHeight + 15) / 16) * 16;
}

void CDVDVideoCodecDXVA::FillInVideoDescription(DXVA2_VideoDesc *pDesc)
{
  memset (pDesc, 0, sizeof(DXVA2_VideoDesc));
  pDesc->SampleWidth			= PictWidthRounded();
  pDesc->SampleHeight			= PictHeightRounded();
  pDesc->Format				= D3DFMT_A8R8G8B8;
  pDesc->UABProtectionLevel	= 1;
}

BOOL CDVDVideoCodecDXVA::IsSupportedDecoderConfig(const D3DFORMAT nD3DFormat,
                                                  const DXVA2_ConfigPictureDecode& config, 
                                                  bool& bIsPrefered)
{
  bool	bRet = false;

  // TODO : not finished
  bRet = (nD3DFormat == MAKEFOURCC('N', 'V', '1', '2'));
  bIsPrefered = (config.ConfigBitstreamRaw == m_pDXVAparams->PreferedConfigBitstream);
  CLog::Log(LOGDEBUG, "IsSupportedDecoderConfig  0x%08x  %d", nD3DFormat, bRet);
  return bRet;
}

HRESULT CDVDVideoCodecDXVA::FindDXVA2DecoderConfiguration(IDirectXVideoDecoderService *pDecoderService,
                                                          const GUID& guidDecoder, 
                                                          DXVA2_ConfigPictureDecode *pSelectedConfig,
                                                          BOOL *pbFoundDXVA2Configuration)
{
  HRESULT hr = S_OK;
  UINT cFormats = 0;
  UINT cConfigurations = 0;
  bool bIsPrefered = false;

  D3DFORMAT                   *pFormats = NULL;           // size = cFormats
  DXVA2_ConfigPictureDecode   *pConfig = NULL;            // size = cConfigurations

  // Find the valid render target formats for this decoder GUID.
  hr = pDecoderService->GetDecoderRenderTargets(guidDecoder, &cFormats, &pFormats);
  CLog::Log(LOGDEBUG, "GetDecoderRenderTargets => %d", cFormats);

  if (SUCCEEDED(hr))
  {
    // Look for a format that matches our output format.
    for (UINT iFormat = 0; iFormat < cFormats;  iFormat++)
    {
      CLog::Log(LOGDEBUG, "DXVA: Try to negociate => 0x%08x", pFormats[iFormat]);

      // Fill in the video description. Set the width, height, format, and frame rate.
      FillInVideoDescription(&m_VideoDesc); // Private helper function.
      m_VideoDesc.Format = pFormats[iFormat];

      // Get the available configurations.
      hr = pDecoderService->GetDecoderConfigurations(guidDecoder, &m_VideoDesc, NULL, &cConfigurations, &pConfig);

      if (FAILED(hr))
      {
        continue;
      }

      // Find a supported configuration.
      for (UINT iConfig = 0; iConfig < cConfigurations; iConfig++)
      {
        if (IsSupportedDecoderConfig(pFormats[iFormat], pConfig[iConfig], bIsPrefered))
        {
          // This configuration is good.
          if (bIsPrefered || !*pbFoundDXVA2Configuration)
          {
            *pbFoundDXVA2Configuration = TRUE;
            *pSelectedConfig = pConfig[iConfig];
          }

          if (bIsPrefered) break;
        }
      }

      CoTaskMemFree(pConfig);
    } // End of formats loop.
  }

  CoTaskMemFree(pFormats);

  // Note: It is possible to return S_OK without finding a configuration.
  return hr;
}

HRESULT CDVDVideoCodecDXVA::ConfigureDXVA()
{
  HRESULT hr					    = S_OK;
  UINT    pResetToken     = 0;
  UINT    cDecoderGuids		= 0;
  BOOL    bFoundDXVA2Configuration = FALSE;
  GUID    guidDecoder				 = GUID_NULL;
  PFNDXVA2CREATEDEVICEMANAGER pfnDXVA2CreateDeviceManager;

  DXVA2_ConfigPictureDecode config;
  ZeroMemory(&config, sizeof(config));

  GUID*									        pDecoderGuids = NULL;
  IDirect3DDevice9*             hDevice = NULL;
  HANDLE									      hDeviceHandle = INVALID_HANDLE_VALUE;

  m_hDXVA2Library = LoadLibraryW(L"dxva2.dll");
  if(m_hDXVA2Library == NULL)
  {
    CLog::Log(LOGDEBUG, "CDVDVideoCodecDXVA::ConfigureDXVA2: LoadLibrary dxva2.dll failed");
    return E_FAIL;
  }

  hDevice = g_Windowing.Get3DDevice();
  if(!hDevice)
  {
    CLog::Log(LOGDEBUG, "CDVDVideoCodecDXVA::ConfigureDXVA2: Failed to retrieve D3D device");
    return E_FAIL;
  }

  // Make sure the D3D device is created with the MULTITHREADED flag (otherwise, UMD is not thread-safe)
  D3DDEVICE_CREATION_PARAMETERS  d3dcp;
  memset(&d3dcp, 0, sizeof(d3dcp));
  hr = hDevice->GetCreationParameters(&d3dcp);
  if ((hr == NOERROR) && (!(d3dcp.BehaviorFlags & D3DCREATE_MULTITHREADED)))
  {
    CLog::Log(LOGDEBUG, "CDVDVideoCodecDXVA::ConfigureDXVA2: D3D device must be created with the _MULTITHREADED flag on Vista and above");
    return E_FAIL;
  }

  // set the correct DXVA params
  if(m_CodecID == CODEC_ID_H264)
  {
    if(IsVistaAndAbove())
      m_pDXVAparams = &DXVA_H264_VISTA;
    else
      m_pDXVAparams = &DXVA_H264;
  }
  else if(m_CodecID == CODEC_ID_VC1)
  {
    m_pDXVAparams = &DXVA_VC1;
  }
  else
  {
    CLog::Log(LOGDEBUG, "CDVDVideoCodecDXVA::ConfigureDXVA2 Codec not supported");
    return E_FAIL;
  }

  pfnDXVA2CreateDeviceManager = (PFNDXVA2CREATEDEVICEMANAGER)GetProcAddress(m_hDXVA2Library, "DXVA2CreateDirect3DDeviceManager9");
  if (pfnDXVA2CreateDeviceManager == NULL)
  {
    CLog::Log(LOGDEBUG, "CDVDVideoCodecDXVA::ConfigureDXVA2 GetProcAddress failed");
    return FALSE;
  }

  hr = pfnDXVA2CreateDeviceManager(&m_nResetToken, &m_pDeviceManager);
  if (FAILED(hr))
  {
    CLog::Log(LOGDEBUG, "CDVDVideoCodecDXVA::ConfigureDXVA2: Failed to create Device Manager (0x%08X)", hr);
    return E_FAIL;
  }

  hr = m_pDeviceManager->ResetDevice(hDevice, m_nResetToken);
  if (FAILED(hr))
  {
    CLog::Log(LOGDEBUG, "CDVDVideoCodecDXVA::ConfigureDXVA2: Failed to reset device with reset token (0x%08X)", hr);
    return E_FAIL;
  }

  hr = m_pDeviceManager->OpenDeviceHandle(&hDeviceHandle);
  if (FAILED(hr))
  {
    CLog::Log(LOGDEBUG, "CDVDVideoCodecDXVA::ConfigureDXVA2: m_pDeviceManager->OpenDeviceHandle failed (0x%08X)", hr);
    return E_FAIL;
  }
 
  hr = m_pDeviceManager->GetVideoService(hDeviceHandle, 
      __uuidof(IDirectXVideoDecoderService), 
      (void**)&m_pDecoderService);
  if (FAILED(hr))
  {
    CLog::Log(LOGDEBUG, "CDVDVideoCodecDXVA::ConfigureDXVA2: m_pDeviceManager->GetVideoService failed (0x%08X)", hr);
    return E_FAIL;
  }
  
  // Get the decoder GUIDs.
  hr = m_pDecoderService->GetDecoderDeviceGuids(&cDecoderGuids, &pDecoderGuids);

  if (SUCCEEDED(hr))
  {
    // Look for the decoder GUIDs we want.
    for (UINT iGuid = 0; iGuid < cDecoderGuids; iGuid++)
    {
      // Do we support this mode?
      if (!IsSupportedDecoderMode(pDecoderGuids[iGuid]))
      {
        continue;
      }

      // Find a configuration that we support. 
      hr = FindDXVA2DecoderConfiguration(m_pDecoderService, pDecoderGuids[iGuid], &config, &bFoundDXVA2Configuration);

      if (FAILED(hr))
      {
        break;
      }

      if (bFoundDXVA2Configuration)
      {
        // Found a good configuration. Save the GUID.
        guidDecoder = pDecoderGuids[iGuid];
      }
    }
  }

  if (pDecoderGuids) 
    CoTaskMemFree(pDecoderGuids);
  if (!bFoundDXVA2Configuration)
  {
    return E_FAIL; // Unable to find a configuration.
  }

  if (SUCCEEDED(hr))
  {
    // Store the things we will need later.
    m_DXVA2Config		= config;
    m_DXVADecoderGUID	= guidDecoder;
  }

  if(m_pDXVAparams)
    m_nSurfaceArrayCount = m_pDXVAparams->PicEntryNumber;

  m_pDeviceManager->CloseDeviceHandle(hDeviceHandle);

  // Allocate a new array of pointers.
  if(m_nSurfaceArrayCount > 0)
  {
    m_ppRTSurfaceArray = new IDirect3DSurface9*[m_nSurfaceArrayCount];
    if (m_ppRTSurfaceArray == NULL)
      hr = E_OUTOFMEMORY;
    else
      ZeroMemory(m_ppRTSurfaceArray, sizeof(IDirect3DSurface9*) * m_nSurfaceArrayCount);
  }

  return hr;
}

BOOL CDVDVideoCodecDXVA::IsSupportedDecoderMode(const GUID& mode)
{
  for (int i=0; i < MAX_SUPPORTED_MODE; i++)
  {
    if (*m_pDXVAparams->Decoder[i] == GUID_NULL) 
      break;
    else if (*m_pDXVAparams->Decoder[i] == mode)
      return true;
  }

  return false;
}

void CDVDVideoCodecDXVA::AllocExtraData(void* ExtraData, int ExtraDataSize)
{
  if(ExtraData == NULL || ExtraDataSize < 6)
    return;

  char* CodecPrivate = (char *)ExtraData;

  BYTE sps = CodecPrivate[5] & 0x1f;

  std::vector<BYTE> avcC;
  for(int i = 0, j = ExtraDataSize; i < j; i++)
    avcC.push_back(CodecPrivate[i]);

  std::vector<BYTE> sh;

  unsigned jj = 6;

  while (sps--) 
  {
    if (jj + 2 > avcC.size())
      return;
    unsigned spslen = ((unsigned)avcC[jj] << 8) | avcC[jj+1];
    if (jj + 2 + spslen > avcC.size())
      return;
    unsigned cur = sh.size();
    sh.resize(cur + spslen + 2, 0);
    std::copy(avcC.begin() + jj, avcC.begin() + jj + 2 + spslen,sh.begin() + cur);
    jj += 2 + spslen;
  }

  if (jj + 1 > avcC.size())
    return;

  unsigned pps = avcC[jj++];

  while (pps--)
  {
    if (jj + 2 > avcC.size())
      return;
    unsigned ppslen = ((unsigned)avcC[jj] << 8) | avcC[jj+1];
    if (jj + 2 + ppslen > avcC.size())
      return;
    unsigned cur = sh.size();
    sh.resize(cur + ppslen + 2, 0);
    std::copy(avcC.begin() + jj, avcC.begin() + jj + 2 + ppslen, sh.begin() + cur);
    jj += 2 + ppslen;
  }

  //m_pCodecContext->nal_length_size = (CodecPrivate[4] & 3) + 1;

  int size = sh.size();
  if(size == 0)
    return;
  m_pCodecContext->extradata_size = size;
  m_pCodecContext->extradata = (uint8_t*)calloc(1,size+FF_INPUT_BUFFER_PADDING_SIZE);
  memcpy(m_pCodecContext->extradata, (unsigned char *)&sh[0], size);
}

void CDVDVideoCodecDXVA::DetectVideoCard()
{
  IDirect3D9* pD3D9;
  m_nPCIVendor = 0;
  m_nPCIDevice = 0;
  m_VideoDriverVersion.HighPart = 0;
  m_VideoDriverVersion.LowPart = 0;

  if (pD3D9 = Direct3DCreate9(D3D_SDK_VERSION)) 
  {
    D3DADAPTER_IDENTIFIER9 adapterIdentifier;
    if (pD3D9->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &adapterIdentifier) == S_OK) 
    {
      m_nPCIVendor = adapterIdentifier.VendorId;
      m_nPCIDevice = adapterIdentifier.DeviceId;
      m_VideoDriverVersion = adapterIdentifier.DriverVersion;
      m_strDeviceDescription = adapterIdentifier.Description;
      //m_strDeviceDescription.AppendFormat (_T(" (%d)"), m_nPCIVendor);
    }
    pD3D9->Release();
  }
}

void CDVDVideoCodecDXVA::OverideDefaults()
{
  AVCodecContext *s = m_pCodecContext;

  memset(s, 0, sizeof(AVCodecContext));

  //s->av_class= &av_codec_context_class;

  s->time_base.num=0; s->time_base.den=1;

  s->get_buffer= avcodec_default_get_buffer;
  s->release_buffer= avcodec_default_release_buffer;
  s->get_format= avcodec_default_get_format;
  s->execute= avcodec_default_execute;
  s->sample_aspect_ratio.num=0; s->sample_aspect_ratio.den=1;
  s->pix_fmt= PIX_FMT_NONE;
  s->sample_fmt= SAMPLE_FMT_S16; // FIXME: set to NONE

  s->palctrl = NULL;
  s->reget_buffer= avcodec_default_reget_buffer;

  s->bit_rate= 800*1000;
  s->bit_rate_tolerance= s->bit_rate*10;
  s->qmin= 2;
  s->qmax= 31;
  s->mb_lmin= FF_QP2LAMBDA * 2;
  s->mb_lmax= FF_QP2LAMBDA * 31;
  s->cqp = -1;
  s->refs = 1;
  s->directpred = 2;
  s->qcompress= 0.5;
  s->complexityblur = 20.0;
  s->keyint_min = 25;
  s->flags2 = CODEC_FLAG2_FASTPSKIP;
  s->max_qdiff= 3;
  s->b_quant_factor=1.25;
  s->b_quant_offset=1.25;
  s->i_quant_factor=-0.8f;
  s->i_quant_offset=0.0;
  s->error_concealment= 3;
  s->error_recognition= 1;
  s->workaround_bugs= FF_BUG_AUTODETECT;
  s->gop_size= 50;
  s->me_method= ME_EPZS;   
  s->thread_count=1;
  s->me_subpel_quality=8;
  s->lmin= FF_QP2LAMBDA * s->qmin;
  s->lmax= FF_QP2LAMBDA * s->qmax;
  s->ildct_cmp= FF_CMP_VSAD;
  s->profile= FF_PROFILE_UNKNOWN;
  s->level= FF_LEVEL_UNKNOWN;
  s->me_penalty_compensation= 256;
  s->frame_skip_cmp= FF_CMP_DCTMAX;
  s->nsse_weight= 8;
  s->mv0_threshold= 256;
  s->b_sensitivity= 40;
  s->compression_level = FF_COMPRESSION_DEFAULT;
  s->use_lpc = -1;
  s->min_prediction_order = -1;
  s->max_prediction_order = -1;
  s->prediction_order_method = -1;
  s->min_partition_order = -1;
  s->max_partition_order = -1;
  s->intra_quant_bias= FF_DEFAULT_QUANT_BIAS;
  s->inter_quant_bias= FF_DEFAULT_QUANT_BIAS;
  s->rc_max_available_vbv_use = 1.0f / 3.0f;
  s->rc_min_vbv_overflow_use = 3;
  s->codec_tag = '1CVA';
}

bool CDVDVideoCodecDXVA::AllocateResources()
{
  CSingleLock lock(m_resourceSection);

  ReleaseResources();

  AllocateRenderSurfaces();

  if(m_ppRTSurfaceArray != NULL && m_nSurfaceArrayCount != 0)
  {
    HRESULT hr;

    hr = m_pDecoderService->CreateVideoDecoder (m_DXVADecoderGUID, &m_VideoDesc, &m_DXVA2Config, 
      m_ppRTSurfaceArray, m_nSurfaceArrayCount, &m_pDirectXVideoDec);

    if (SUCCEEDED (hr))
    {
      if (!m_pDXVADecoder)
      {
        if ((m_DXVADecoderGUID == DXVA2_ModeH264_E) || (m_DXVADecoderGUID == DXVA2_ModeH264_F) || (m_DXVADecoderGUID == DXVA_Intel_H264_ClearVideo))
          m_pDXVADecoder	= new CDXVADecoderH264 (this, m_pDirectXVideoDec, H264_VLD, m_nSurfaceArrayCount, &m_DXVA2Config);
        /* TBI
        else if (m_DXVADecoderGUID == DXVA2_ModeVC1_D || m_DXVADecoderGUID == DXVA_Intel_VC1_ClearVideo)
          m_pDXVADecoder	= new CDXVADecoderVC1 (pDirectXVideoDec, VC1_VLD, m_nSurfaceArrayCount, &m_DXVA2Config);
          */
        else
        {
          goto on_error;
        }
      }
        
      if (m_pDXVADecoder) 
      {
        m_pDXVADecoder->SetExtraData ((BYTE*)m_pCodecContext->extradata, m_pCodecContext->extradata_size);
      }
    }
    else
    {
      goto on_error;
    }
  } 
  else
    goto on_error;

  // Make sure to disable any unnecessary post-processing
  if(m_pDirectXVideoDec)
    DisableDecoderPostProcessing(m_pDirectXVideoDec);

  Reset();

  m_bResourcesAllocated = true;
  return true;

on_error:
  ReleaseResources();
  return false;
}

void CDVDVideoCodecDXVA::ReleaseResources()
{
  CSingleLock lock(m_resourceSection);

  if(m_pDXVADecoder)
    m_pDXVADecoder->Flush();

  if (m_ppRTSurfaceArray)
  {
    for (unsigned long i = 0; i < m_nSurfaceArrayCount; i++)
    {
      if (m_ppRTSurfaceArray[i] != NULL)
      {
        m_ppRTSurfaceArray[i]->Release();
        m_ppRTSurfaceArray[i] = NULL;
      }
    }
  }

  /*
  if(m_pDeviceManager)
    hr = m_pDeviceManager->ResetDevice(g_Windowing.Get3DDevice(), m_nResetToken);
    */

  m_nResetToken = 0;
  
  SAFE_RELEASE(m_pDirectXVideoDec);
  SAFE_DELETE(m_pDXVADecoder);

  m_bResourcesAllocated = false;
}

void CDVDVideoCodecDXVA::AllocateRenderSurfaces()
{
  HRESULT	hr = S_OK;

  int widthRound = PictWidthRounded();
  int heightRound = PictHeightRounded();

  if(m_pDecoderService == NULL)
    return;

  // Allocate the surfaces.
  D3DFORMAT dwFormat = m_VideoDesc.Format;
    hr = m_pDecoderService->CreateSurface(
      widthRound,
      heightRound,
      m_nSurfaceArrayCount - 1,
      (D3DFORMAT)dwFormat,
      D3DPOOL_DEFAULT,
      0,
      DXVA2_VideoDecoderRenderTarget,
      m_ppRTSurfaceArray,
      NULL
      );
  }

void CDVDVideoCodecDXVA::DisableDecoderPostProcessing(IDirectXVideoDecoder* pDirectXVideoDec)
{
  HRESULT hr;

  DXVA2_DecodeExecuteParams dep;
  DXVA2_DecodeExtensionData ed;
  DWORD dwPPControl, dwRetVal = 0;

  //dwPPControl = (pdci->DeinterlaceMode == VideoDeinterlaceMode_Adaptive) ? 0x01 : 0x03; // Disable PP (+force bob)
  dwPPControl = 0x03;
  dep.NumCompBuffers = 0;
  dep.pCompressedBuffers = NULL;
  dep.pExtensionData = &ed;
  ed.Function = NV_DXVA_PPCONTROL;
  ed.pPrivateInputData = &dwPPControl;
  ed.PrivateInputDataSize = sizeof(dwPPControl);
  ed.pPrivateOutputData = &dwRetVal;
  ed.PrivateOutputDataSize = sizeof(dwRetVal);
  hr = pDirectXVideoDec->Execute(&dep);
  if (FAILED(hr))
  {
    CLog::Log(LOGDEBUG, "Warning: Post-Process control extension not supported");
  }
}


HRESULT CDVDVideoCodecDXVA::Deliver(int nIndex, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd)
{
  HRESULT hr = S_OK;

  m_rtDisplayFrameStart = rtStart;
  m_rtDisplayFrameEnd = rtEnd;

  m_nLastFrame = nIndex;

  return hr;
}

void CDVDVideoCodecDXVA::ReorderBFrames(REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop)
{
  // Re-order B-frames if needed
  if (m_pCodecContext->has_b_frames && m_bReorderBFrame)
  { 
    rtStart	= m_BFrames [m_nPosB].rtStart;
    rtStop	= m_BFrames [m_nPosB].rtStop;
  }
}

void CDVDVideoCodecDXVA::LogLibAVCodec(void* par,int level,const char *fmt,va_list valist)
{
#if defined(_DEBUG)
  char		Msg [500];
  vsnprintf_s (Msg, sizeof(Msg), _TRUNCATE, fmt, valist);
  CLog::Log(101, "AVLIB : %s", Msg);
#endif
}

// === ID3DResource
void CDVDVideoCodecDXVA::OnDestroyDevice()
{
  ReleaseResources();
}

void CDVDVideoCodecDXVA::OnCreateDevice()
{
  
}

void CDVDVideoCodecDXVA::OnLostDevice()
{
  ReleaseResources();
}

void CDVDVideoCodecDXVA::OnResetDevice()
{
  AllocateResources();
}


#endif // HAS_DX
