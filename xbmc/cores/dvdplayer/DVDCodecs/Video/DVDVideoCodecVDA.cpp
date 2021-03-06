/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "system.h"

#if defined(HAVE_LIBVDADECODER)
#include "DynamicDll.h"
#include "GUISettings.h"
#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "DVDVideoCodecVDA.h"
#include "DllAvFormat.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreVideo/CoreVideo.h>

/*
 * if extradata size is greater than 7, then have a valid quicktime 
 * avcC atom header.
 *
 *      -: avcC atom header :-
 *  -----------------------------------
 *  1 byte  - version
 *  1 byte  - h.264 stream profile
 *  1 byte  - h.264 compatible profiles
 *  1 byte  - h.264 stream level
 *  6 bits  - reserved set to 63
 *  2 bits  - NAL length 
 *            ( 0 - 1 byte; 1 - 2 bytes; 3 - 4 bytes)
 *  3 bit   - reserved
 *  5 bits  - number of SPS 
 *  for (i=0; i < number of SPS; i++) {
 *      2 bytes - SPS length
 *      SPS length bytes - SPS NAL unit
 *  }
 *  1 byte  - number of PPS
 *  for (i=0; i < number of PPS; i++) {
 *      2 bytes - PPS length 
 *      PPS length bytes - PPS NAL unit 
 *  }
*/

// missing in 10.4/10.5 SDKs.
#if (MAC_OS_X_VERSION_MAX_ALLOWED < 1060)
#include "dlfcn.h"
enum {
  // component Y'CbCr 8-bit 4:2:2, ordered Cb Y'0 Cr Y'1 .
  kCVPixelFormatType_422YpCbCr8 = FourCharCode('2vuy')
};
#endif

////////////////////////////////////////////////////////////////////////////////////////////
// http://developer.apple.com/mac/library/technotes/tn2010/tn2267.html
// VDADecoder API (keep this until VDADecoder.h is public).
// #include <VideoDecodeAcceleration/VDADecoder.h>
enum {
  kVDADecoderNoErr = 0,
  kVDADecoderHardwareNotSupportedErr = -12470,		
  kVDADecoderFormatNotSupportedErr = -12471,
  kVDADecoderConfigurationError = -12472,
  kVDADecoderDecoderFailedErr = -12473,
};
enum {
  kVDADecodeInfo_Asynchronous = 1UL << 0,
  kVDADecodeInfo_FrameDropped = 1UL << 1
};
enum {
  // tells the decoder not to bother returning a CVPixelBuffer
  // in the outputCallback. The output callback will still be called.
  kVDADecoderDecodeFlags_DontEmitFrame = 1 << 0
};
enum {
  // decode and return buffers for all frames currently in flight.
  kVDADecoderFlush_EmitFrames = 1 << 0		
};

typedef struct OpaqueVDADecoder* VDADecoder;

typedef void (*VDADecoderOutputCallback)(
  void *decompressionOutputRefCon, 
  CFDictionaryRef frameInfo, 
  OSStatus status, 
  uint32_t infoFlags,
  CVImageBufferRef imageBuffer);

////////////////////////////////////////////////////////////////////////////////////////////
class DllLibVDADecoderInterface
{
public:
  virtual ~DllLibVDADecoderInterface() {}
  virtual OSStatus VDADecoderCreate(
    CFDictionaryRef decoderConfiguration, CFDictionaryRef destinationImageBufferAttributes,
    VDADecoderOutputCallback *outputCallback, void *decoderOutputCallbackRefcon, VDADecoder *decoderOut) = 0;
  virtual OSStatus VDADecoderDecode(
    VDADecoder decoder, uint32_t decodeFlags, CFTypeRef compressedBuffer, CFDictionaryRef frameInfo) = 0;
  virtual OSStatus VDADecoderFlush(VDADecoder decoder, uint32_t flushFlags) = 0;
  virtual OSStatus VDADecoderDestroy(VDADecoder decoder) = 0;
  virtual CFStringRef Get_kVDADecoderConfiguration_Height() = 0;
  virtual CFStringRef Get_kVDADecoderConfiguration_Width() = 0;
  virtual CFStringRef Get_kVDADecoderConfiguration_SourceFormat() = 0;
  virtual CFStringRef Get_kVDADecoderConfiguration_avcCData() = 0;
};

class DllLibVDADecoder : public DllDynamic, DllLibVDADecoderInterface
{
  DECLARE_DLL_WRAPPER(DllLibVDADecoder, "/System/Library/Frameworks/VideoDecodeAcceleration.framework/Versions/Current/VideoDecodeAcceleration")

  DEFINE_METHOD5(OSStatus, VDADecoderCreate, (CFDictionaryRef p1, CFDictionaryRef p2, VDADecoderOutputCallback* p3, void* p4, VDADecoder* p5))
  DEFINE_METHOD4(OSStatus, VDADecoderDecode, (VDADecoder p1, uint32_t p2, CFTypeRef p3, CFDictionaryRef p4))
  DEFINE_METHOD2(OSStatus, VDADecoderFlush, (VDADecoder p1, uint32_t p2))
  DEFINE_METHOD1(OSStatus, VDADecoderDestroy, (VDADecoder p1))
  DEFINE_GLOBAL(CFStringRef, kVDADecoderConfiguration_Height)
  DEFINE_GLOBAL(CFStringRef, kVDADecoderConfiguration_Width)
  DEFINE_GLOBAL(CFStringRef, kVDADecoderConfiguration_SourceFormat)
  DEFINE_GLOBAL(CFStringRef, kVDADecoderConfiguration_avcCData)
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(VDADecoderCreate)
    RESOLVE_METHOD(VDADecoderDecode)
    RESOLVE_METHOD(VDADecoderFlush)
    RESOLVE_METHOD(VDADecoderDestroy)
    RESOLVE_METHOD(kVDADecoderConfiguration_Height)
    RESOLVE_METHOD(kVDADecoderConfiguration_Width)
    RESOLVE_METHOD(kVDADecoderConfiguration_SourceFormat)
    RESOLVE_METHOD(kVDADecoderConfiguration_avcCData)
  END_METHOD_RESOLVE()
};

////////////////////////////////////////////////////////////////////////////////////////////
// helper function that wraps a time into a dictionary
static CFDictionaryRef MakeDictionaryWithDisplayTime(double inFrameDisplayTime)
{
  CFStringRef key = CFSTR("VideoDisplayTimeKey");
  CFNumberRef value = CFNumberCreate(
    kCFAllocatorDefault, kCFNumberDoubleType, &inFrameDisplayTime);

  return CFDictionaryCreate(
    kCFAllocatorDefault, (const void **)&key, (const void **)&value, 1,
    &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
}

// helper function to extract a time from a dictionary
static double GetFrameDisplayTimeFromDictionary(CFDictionaryRef inFrameInfoDictionary)
{
  double outValue = 0.0;
  CFNumberRef timeNumber = NULL;

  if (NULL == inFrameInfoDictionary)
    return 0.0;

  timeNumber = (CFNumberRef)CFDictionaryGetValue(inFrameInfoDictionary, CFSTR("VideoDisplayTimeKey"));
  if (timeNumber)
    CFNumberGetValue(timeNumber, kCFNumberDoubleType, &outValue);

  return outValue;
}

////////////////////////////////////////////////////////////////////////////////////////////
// TODO: refactor this so as not to need these ffmpeg routines. 
// These are not exposed in ffmpeg's API so we dupe them here.
// AVC helper functions for muxers, 
//  * Copyright (c) 2006 Baptiste Coudurier <baptiste.coudurier@smartjog.com>
// This is part of FFmpeg
//  * License as published by the Free Software Foundation; either
//  * version 2.1 of the License, or (at your option) any later version.
#define VDA_RB24(x)                          \
  ((((const uint8_t*)(x))[0] << 16) |        \
   (((const uint8_t*)(x))[1] <<  8) |        \
   ((const uint8_t*)(x))[2])

#define VDA_RB32(x)                          \
  ((((const uint8_t*)(x))[0] << 24) |        \
   (((const uint8_t*)(x))[1] << 16) |        \
   (((const uint8_t*)(x))[2] <<  8) |        \
   ((const uint8_t*)(x))[3])

static const uint8_t *avc_find_startcode_internal(const uint8_t *p, const uint8_t *end)
{
  const uint8_t *a = p + 4 - ((intptr_t)p & 3);

  for (end -= 3; p < a && p < end; p++)
  {
    if (p[0] == 0 && p[1] == 0 && p[2] == 1)
      return p;
  }

  for (end -= 3; p < end; p += 4)
  {
    uint32_t x = *(const uint32_t*)p;
    if ((x - 0x01010101) & (~x) & 0x80808080) // generic
    {
      if (p[1] == 0)
      {
        if (p[0] == 0 && p[2] == 1)
          return p;
        if (p[2] == 0 && p[3] == 1)
          return p+1;
      }
      if (p[3] == 0)
      {
        if (p[2] == 0 && p[4] == 1)
          return p+2;
        if (p[4] == 0 && p[5] == 1)
          return p+3;
      }
    }
  }

  for (end += 3; p < end; p++)
  {
    if (p[0] == 0 && p[1] == 0 && p[2] == 1)
      return p;
  }

  return end + 3;
}

const uint8_t *avc_find_startcode(const uint8_t *p, const uint8_t *end)
{
  const uint8_t *out= avc_find_startcode_internal(p, end);
  if (p<out && out<end && !out[-1])
    out--;
  return out;
}

const int avc_parse_nal_units(DllAvFormat *av_format_ctx,
  ByteIOContext *pb, const uint8_t *buf_in, int size)
{
  const uint8_t *p = buf_in;
  const uint8_t *end = p + size;
  const uint8_t *nal_start, *nal_end;

  size = 0;
  nal_start = avc_find_startcode(p, end);
  while (nal_start < end)
  {
    while (!*(nal_start++));
    nal_end = avc_find_startcode(nal_start, end);
    av_format_ctx->put_be32(pb, nal_end - nal_start);
    av_format_ctx->put_buffer(pb, nal_start, nal_end - nal_start);
    size += 4 + nal_end - nal_start;
    nal_start = nal_end;
  }
  return size;
}

const int avc_parse_nal_units_buf(DllAvUtil *av_util_ctx, DllAvFormat *av_format_ctx,
  const uint8_t *buf_in, uint8_t **buf, int *size)
{
  ByteIOContext *pb;
  int ret = av_format_ctx->url_open_dyn_buf(&pb);
  if (ret < 0)
    return ret;

  avc_parse_nal_units(av_format_ctx, pb, buf_in, *size);

  av_util_ctx->av_freep(buf);
  *size = av_format_ctx->url_close_dyn_buf(pb, buf);
  return 0;
}

const int isom_write_avcc(DllAvUtil *av_util_ctx, DllAvFormat *av_format_ctx, 
  ByteIOContext *pb, const uint8_t *data, int len)
{
  // extradata from bytestream h264, convert to avcC atom data for bitstream
  if (len > 6)
  {
    /* check for h264 start code */
    if (VDA_RB32(data) == 0x00000001 || VDA_RB24(data) == 0x000001)
    {
      uint8_t *buf=NULL, *end, *start;
      uint32_t sps_size=0, pps_size=0;
      uint8_t *sps=0, *pps=0;

      int ret = avc_parse_nal_units_buf(av_util_ctx, av_format_ctx, data, &buf, &len);
      if (ret < 0)
        return ret;
      start = buf;
      end = buf + len;

      /* look for sps and pps */
      while (buf < end)
      {
        unsigned int size;
        uint8_t nal_type;
        size = VDA_RB32(buf);
        nal_type = buf[4] & 0x1f;
        if (nal_type == 7) /* SPS */
        {
          sps = buf + 4;
          sps_size = size;
        }
        else if (nal_type == 8) /* PPS */
        {
          pps = buf + 4;
          pps_size = size;
        }
        buf += size + 4;
      }
      assert(sps);
      assert(pps);

      av_format_ctx->put_byte(pb, 1); /* version */
      av_format_ctx->put_byte(pb, sps[1]); /* profile */
      av_format_ctx->put_byte(pb, sps[2]); /* profile compat */
      av_format_ctx->put_byte(pb, sps[3]); /* level */
      av_format_ctx->put_byte(pb, 0xff); /* 6 bits reserved (111111) + 2 bits nal size length - 1 (11) */
      av_format_ctx->put_byte(pb, 0xe1); /* 3 bits reserved (111) + 5 bits number of sps (00001) */

      av_format_ctx->put_be16(pb, sps_size);
      av_format_ctx->put_buffer(pb, sps, sps_size);
      av_format_ctx->put_byte(pb, 1); /* number of pps */
      av_format_ctx->put_be16(pb, pps_size);
      av_format_ctx->put_buffer(pb, pps, pps_size);
      av_util_ctx->av_free(start);
    }
    else
    {
      av_format_ctx->put_buffer(pb, data, len);
    }
  }
  return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
CDVDVideoCodecVDA::CDVDVideoCodecVDA() : CDVDVideoCodec()
{
  m_dll = new DllLibVDADecoder;
  m_vda_decoder = NULL;
  m_pFormatName = "";

  m_queue_depth = 0;
  m_display_queue = NULL;
  pthread_mutex_init(&m_queue_mutex, NULL);

  m_convert_bytestream = false;
  m_swcontext = NULL;
  memset(&m_videobuffer, 0, sizeof(DVDVideoPicture));
}

CDVDVideoCodecVDA::~CDVDVideoCodecVDA()
{
  Dispose();
  delete m_dll;
}

bool CDVDVideoCodecVDA::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (g_guiSettings.GetBool("videoplayer.hwaccel") && !hints.software)
  {
    int32_t width;
    int32_t height;
    CFDataRef avcCData;
    uint8_t *extradata; // extra data for codec to use
    unsigned int extrasize; // size of extra data
    
    //
    width  = hints.width;
    height = hints.height;
    extrasize = hints.extrasize;
    extradata = (uint8_t*)hints.extradata;

    switch (hints.codec)
    {
      case CODEC_ID_H264:
        // TODO: need to quality h264 encoding (profile, level and number of reference frame)
        // source must be H.264 with valid avcC atom data in extradata
        if (extrasize < 7 || extradata == NULL)
        {
          CLog::Log(LOGNOTICE, "%s - avcC atom too data small or missing", __FUNCTION__);
          return false;
        }
        // valid avcC atom data always starts with the value 1 (version)
        if ( *extradata != 1 )
        {
          if (extradata[0] == 0 && extradata[1] == 0 && extradata[2] == 0 && extradata[3] == 1)
          {
            // video content is from x264 or from bytestream h264 (AnnexB format)
            // NAL reformating to bitstream format needed
            if (!m_dllAvUtil.Load() || m_dllAvCore.Load() || !m_dllAvFormat.Load())
              return false;

            ByteIOContext *pb;
            if (m_dllAvFormat.url_open_dyn_buf(&pb) < 0)
              return false;

            m_convert_bytestream = true;
            // create a valid avcC atom data from ffmpeg's extradata
            isom_write_avcc(&m_dllAvUtil, &m_dllAvFormat, pb, extradata, extrasize);
            // unhook from ffmpeg's extradata
            extradata = NULL;
            // extract the avcC atom data into extradata then write it into avcCData for VDADecoder
            extrasize = m_dllAvFormat.url_close_dyn_buf(pb, &extradata);
            // CFDataCreate makes a copy of extradata contents
            avcCData = CFDataCreate(kCFAllocatorDefault, (const uint8_t*)extradata, extrasize);
            // done with the converted extradata, we MUST free using av_free
            m_dllAvUtil.av_free(extradata);
          }
          else
          {
            CLog::Log(LOGNOTICE, "%s - invalid avcC atom data", __FUNCTION__);
            return false;
          }
        }
        else
        {
          // CFDataCreate makes a copy of extradata contents
          avcCData = CFDataCreate(kCFAllocatorDefault, (const uint8_t*)extradata, extrasize);
        }

        m_format = 'avc1';
        m_pFormatName = "vda-h264";
      break;
      default:
        return false;
      break;
    }

    // input stream is qualified, now we can load dlls.
    if (!m_dll->Load() || !m_dllSwScale.Load())
      return false;

    CFMutableDictionaryRef decoderConfiguration = CFDictionaryCreateMutable(
      kCFAllocatorDefault, 4, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    CFNumberRef avcWidth  = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &width);
    CFNumberRef avcHeight = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &height);
    CFNumberRef avcFormat = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &m_format);
    
    CFDictionarySetValue(decoderConfiguration, m_dll->Get_kVDADecoderConfiguration_Height(), avcHeight);
    CFDictionarySetValue(decoderConfiguration, m_dll->Get_kVDADecoderConfiguration_Width(),  avcWidth);
    CFDictionarySetValue(decoderConfiguration, m_dll->Get_kVDADecoderConfiguration_SourceFormat(), avcFormat);
    CFDictionarySetValue(decoderConfiguration, m_dll->Get_kVDADecoderConfiguration_avcCData(), avcCData);
    
    // create the VDADecoder object using defaulted '2vuy' video format buffers.
    OSStatus status;
    try
    {
      status = m_dll->VDADecoderCreate(decoderConfiguration, NULL,
      (VDADecoderOutputCallback *)&VDADecoderCallback, this, (VDADecoder*)&m_vda_decoder);
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s - exception",__FUNCTION__);
      status = kVDADecoderDecoderFailedErr;
    }
    
    CFRelease(decoderConfiguration);
    if (status != kVDADecoderNoErr) 
    {
      CLog::Log(LOGNOTICE, "%s - VDADecoder Codec failed to open,status(%d)", __FUNCTION__, (int)status);
      return false;
    }

    // allocate a YV12 DVDVideoPicture buffer.
    // first make sure all properties are reset.
    memset(&m_videobuffer, 0, sizeof(DVDVideoPicture));
    unsigned int iPixels = width * height;
    unsigned int iChromaPixels = iPixels/4;

    m_videobuffer.pts = DVD_NOPTS_VALUE;
    m_videobuffer.iFlags = DVP_FLAG_ALLOCATED;
    m_videobuffer.format = DVDVideoPicture::FMT_YUV420P;
    m_videobuffer.color_range  = 0;
    m_videobuffer.color_matrix = 4;
    m_videobuffer.iWidth  = width;
    m_videobuffer.iHeight = height;
    m_videobuffer.iDisplayWidth  = width;
    m_videobuffer.iDisplayHeight = height;

    m_videobuffer.iLineSize[0] = width;   //Y
    m_videobuffer.iLineSize[1] = width/2; //U
    m_videobuffer.iLineSize[2] = width/2; //V
    m_videobuffer.iLineSize[3] = 0;

    m_videobuffer.data[0] = (BYTE*)_aligned_malloc(iPixels, 16);       //Y
    m_videobuffer.data[1] = (BYTE*)_aligned_malloc(iChromaPixels, 16); //U
    m_videobuffer.data[2] = (BYTE*)_aligned_malloc(iChromaPixels, 16); //V
    m_videobuffer.data[3] = NULL;

    // set all data to 0 for less artifacts.. hmm.. what is black in YUV??
    memset(m_videobuffer.data[0], 0, iPixels);
    memset(m_videobuffer.data[1], 0, iChromaPixels);
    memset(m_videobuffer.data[2], 0, iChromaPixels);

    // pre-alloc ffmpeg swscale context.
    m_swcontext = m_dllSwScale.sws_getContext(
      m_videobuffer.iWidth, m_videobuffer.iHeight, PIX_FMT_UYVY422, 
      m_videobuffer.iWidth, m_videobuffer.iHeight, PIX_FMT_YUV420P, 
      SWS_FAST_BILINEAR, NULL, NULL, NULL);

    m_DropPictures = false;

    return true;
  }

  return false;
}

void CDVDVideoCodecVDA::Dispose()
{
  if (m_vda_decoder)
  {
    m_dll->VDADecoderDestroy((VDADecoder)m_vda_decoder);
    m_vda_decoder = NULL;
  }
  if (m_videobuffer.iFlags & DVP_FLAG_ALLOCATED)
  {
    _aligned_free(m_videobuffer.data[0]);
    _aligned_free(m_videobuffer.data[1]);
    _aligned_free(m_videobuffer.data[2]);
    m_videobuffer.iFlags = 0;
  }
  if (m_convert_bytestream)
  {
    m_dllAvUtil.Unload();
    m_dllAvCore.Unload();
    m_dllAvFormat.Unload();
  }
  if (m_swcontext)
  {
    m_dllSwScale.sws_freeContext(m_swcontext);
    m_swcontext = NULL;
  }
  m_dllSwScale.Unload();
  m_dll->Unload();
}

void CDVDVideoCodecVDA::SetDropState(bool bDrop)
{
  m_DropPictures = bDrop;
}

int CDVDVideoCodecVDA::Decode(BYTE* pData, int iSize, double pts, double dts)
{
  OSStatus status;
  uint32_t avc_flags = 0;
  CFDataRef avc_demux;
  CFDictionaryRef avc_pts = MakeDictionaryWithDisplayTime(pts);
  //
  if (m_convert_bytestream)
  {
    // convert demuxer packet from bytestream (AnnexB) to bitstream
    ByteIOContext *pb;
    int demuxer_bytes;
    uint8_t *demuxer_content;
	
    if(m_dllAvFormat.url_open_dyn_buf(&pb) < 0)
      return VC_ERROR;
    demuxer_bytes = avc_parse_nal_units(&m_dllAvFormat, pb, pData, iSize);
    demuxer_bytes = m_dllAvFormat.url_close_dyn_buf(pb, &demuxer_content);
    avc_demux = CFDataCreate(kCFAllocatorDefault, demuxer_content, demuxer_bytes);
    m_dllAvUtil.av_free(demuxer_content);
  }
  else
  {
    avc_demux = CFDataCreate(kCFAllocatorDefault, pData, iSize);
  }

  if (m_DropPictures)
    avc_flags = kVDADecoderDecodeFlags_DontEmitFrame;

  status = m_dll->VDADecoderDecode((VDADecoder)m_vda_decoder, avc_flags, avc_demux, avc_pts);
  CFRelease(avc_pts);
  CFRelease(avc_demux);
  if (status != kVDADecoderNoErr) 
  {
    CLog::Log(LOGNOTICE, "%s - VDADecoderDecode failed, status(%d)", __FUNCTION__, (int)status);
    return VC_ERROR;
  }

  // TODO: queue depth is related to the number of reference frames in encoded h.264.
  // so we need to buffer until we get N ref frames + 1.
  if (m_queue_depth < 16)
    return VC_BUFFER;

  return VC_PICTURE | VC_BUFFER;
}

void CDVDVideoCodecVDA::Reset(void)
{
  m_dll->VDADecoderFlush((VDADecoder)m_vda_decoder, 0);

  while (m_queue_depth)
    DisplayQueuePop();
}

bool CDVDVideoCodecVDA::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  CVPixelBufferRef yuvframe;

  // clone the video picture buffer settings.
  *pDvdVideoPicture = m_videobuffer;

  // get the top yuv frame, we risk getting the wrong frame if the frame queue
  // depth is less than the number of encoded reference frames. If queue depth
  // is greater than the number of encoded reference frames, then the top frame
  // will never change and we can just grab a ref to the frame/pts. This way
  // we don't lockout the vdadecoder while doing UYVY422_to_YUV420P convert out.
  pthread_mutex_lock(&m_queue_mutex);
  yuvframe = m_display_queue->frame;
  // m_dts_queue gets popped in DisplayQueuePop
  pDvdVideoPicture->pts = m_display_queue->frametime;
  pthread_mutex_unlock(&m_queue_mutex);

  // lock the CVPixelBuffer down
  CVPixelBufferLockBaseAddress(yuvframe, 0);
  int yuv422_stride = CVPixelBufferGetBytesPerRowOfPlane(yuvframe, 0);
  uint8_t *yuv422_ptr = (uint8_t*)CVPixelBufferGetBaseAddressOfPlane(yuvframe, 0);
  if (yuv422_ptr)
    UYVY422_to_YUV420P(yuv422_ptr, yuv422_stride, pDvdVideoPicture);
  // unlock the CVPixelBuffer
  CVPixelBufferUnlockBaseAddress(yuvframe, 0);

  // now we can pop the top frame.
  DisplayQueuePop();

  return VC_PICTURE | VC_BUFFER;
}

void CDVDVideoCodecVDA::UYVY422_to_YUV420P(uint8_t *yuv422_ptr, int yuv422_stride, DVDVideoPicture *picture)
{
  // convert PIX_FMT_UYVY422 to PIX_FMT_YUV420P.
  if (m_swcontext)
  {
    uint8_t *src[]  = { yuv422_ptr, 0, 0, 0 };
    int srcStride[] = { yuv422_stride, 0, 0, 0 };

    uint8_t *dst[]  = { picture->data[0], picture->data[1], picture->data[2], 0 };
    int dstStride[] = { picture->iLineSize[0], picture->iLineSize[1], picture->iLineSize[2], 0 };
  
    m_dllSwScale.sws_scale(m_swcontext, src, srcStride, 0, picture->iHeight, dst, dstStride);
  }
}

void CDVDVideoCodecVDA::DisplayQueuePop(void)
{
  if (!m_display_queue || m_queue_depth == 0)
    return;

  // pop the top frame off the queue
  pthread_mutex_lock(&m_queue_mutex);
  frame_queue *top_frame = m_display_queue;
  m_display_queue = m_display_queue->nextframe;
  m_queue_depth--;
  if (!m_dts_queue.empty())
    m_dts_queue.pop();
  pthread_mutex_unlock(&m_queue_mutex);

  // and release it
  CVPixelBufferRelease(top_frame->frame);
  free(top_frame);
}

void CDVDVideoCodecVDA::VDADecoderCallback(
  void                *decompressionOutputRefCon,
   CFDictionaryRef    frameInfo,
   OSStatus           status, 
   uint32_t           infoFlags,
   CVImageBufferRef   imageBuffer)
{
  CDVDVideoCodecVDA *ctx = (CDVDVideoCodecVDA*)decompressionOutputRefCon;

  if (imageBuffer == NULL)
  {
    CLog::Log(LOGERROR, "%s - imageBuffer is NULL", __FUNCTION__);
    return;
  }
  if (CVPixelBufferGetPixelFormatType(imageBuffer) != kCVPixelFormatType_422YpCbCr8)
  {
    CLog::Log(LOGERROR, "%s - imageBuffer format is not '2vuy", __FUNCTION__);
    return;
  }
  if (kVDADecodeInfo_FrameDropped & infoFlags)
  {
    CLog::Log(LOGDEBUG, "%s - frame dropped", __FUNCTION__);
    // don't forget to pop the dts queue since we are dropping this frame.
    pthread_mutex_lock(&ctx->m_queue_mutex);
    if (!ctx->m_dts_queue.empty())
      ctx->m_dts_queue.pop();
    pthread_mutex_unlock(&ctx->m_queue_mutex);
    return;
  }

  // allocate a new frame and populate it with some information.
  // this pointer to a frame_queue type keeps track of the newest decompressed frame
  // and is then inserted into a linked list of frame pointers depending on the display time
  // parsed out of the bitstream and stored in the frameInfo dictionary by the client
  frame_queue *newFrame = (frame_queue*)calloc(sizeof(frame_queue), 1);
  newFrame->nextframe = NULL;
  newFrame->frame = CVPixelBufferRetain(imageBuffer);
  newFrame->frametime = GetFrameDisplayTimeFromDictionary(frameInfo);

  // since the frames we get may be in decode order rather than presentation order
  // our hypothetical callback places them in a queue of frames which will
  // hold them in display order for display on another thread
  pthread_mutex_lock(&ctx->m_queue_mutex);
  //
  frame_queue *queueWalker = ctx->m_display_queue;
  if (!queueWalker || (newFrame->frametime < queueWalker->frametime))
  {
    // we have an empty queue, or this frame earlier than the current queue head.
    newFrame->nextframe = queueWalker;
    ctx->m_display_queue = newFrame;
  } else {
    // walk the queue and insert this frame where it belongs in display order.
    bool frameInserted = false;
    frame_queue *nextFrame = NULL;
    //
    while (!frameInserted)
    {
      nextFrame = queueWalker->nextframe;
      if (!nextFrame || (newFrame->frametime < nextFrame->frametime))
      {
        // if the next frame is the tail of the queue, or our new frame is ealier.
        newFrame->nextframe = nextFrame;
        queueWalker->nextframe = newFrame;
        frameInserted = true;
      }
      queueWalker = nextFrame;
    }
  }
  ctx->m_queue_depth++;
  //
  pthread_mutex_unlock(&ctx->m_queue_mutex);	
}

#endif
