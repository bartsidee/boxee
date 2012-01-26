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
 
#include "system.h"
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#ifdef _LINUX
#include "stdint.h"
#else
#define INT64_C __int64
#endif
#include "DVDDemuxFFmpeg.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "DVDInputStreams/DVDInputStreamNavigator.h"
#include "DVDInputStreams/DVDInputStreamFile.h"
#include "DVDInputStreams/DVDInputStreamBluray.h"
#ifdef HAS_FILESYSTEM_MMS
#include "DVDInputStreams/DVDInputStreamMMS.h"
#endif
#include "DVDInputStreams/DVDInputStreamRTMP.h"
#include "DVDInputStreams/DVDInputStreamPlaylist.h"
#include "DVDDemuxUtils.h"
#include "DVDClock.h" // for DVD_TIME_BASE
#include "utils/Win32Exception.h"
#include "AdvancedSettings.h"
#include "GUISettings.h"
#include "FileSystem/File.h"
#include "utils/log.h"
#include "Thread.h"
#include "utils/TimeUtils.h"
#include "Util.h"

#include "xbmc/cores/dvb/dvbmanager.h"

void CDemuxStreamAudioFFmpeg::GetStreamInfo(std::string& strInfo)
{
  if(!m_stream) return;
  char temp[128];
  m_parent->m_dllAvCodec.avcodec_string(temp, 128, m_stream->codec, 0);

  // Respect requirements for certification to have proper labeling
  CStdString strInfoTemp = temp;
  if (strInfoTemp.Find("eac3") != -1)                  strInfoTemp.Replace("eac3", "Dolby Digital+");
  else if (strInfoTemp.Find("ac3") != -1)              strInfoTemp.Replace("ac3", "Dolby Digital");
  else if (strInfoTemp.Find("dca (DTS)") != -1)        strInfoTemp.Replace("dca (DTS)", "DTS");
  else if (strInfoTemp.Find("dca (DTS-HD MA)") != -1)  strInfoTemp.Replace("dca (DTS-HD MA)", "DTS-HD MA");
  else if (strInfoTemp.Find("dca (DTS-ES)") != -1)     strInfoTemp.Replace("dca (DTS-ES)", "DTS-ES");
  else if (strInfoTemp.Find("dca (DTS 96/24)") != -1)  strInfoTemp.Replace("dca (DTS 96/24)", "DTS 96/24");
  else if (strInfoTemp.Find("dca (DTS-HD HRA)") != -1) strInfoTemp.Replace("dca (DTS-HD HRA)", "DTS-HD HRA");

  strInfo = strInfoTemp;
}

void CDemuxStreamAudioFFmpeg::GetStreamName(std::string& strInfo)
{
  if (!m_stream)
    return;

  if (!m_description.empty())
  {
    if (strInfo.compare(m_description))
      strInfo = m_description;
  }
}

void CDemuxStreamSubtitleFFmpeg::GetStreamName(std::string& strInfo)
{
  if(!m_stream)
    return;

  if(!m_description.empty())
  {
    if(strInfo.compare(m_description))
      strInfo = m_description;
  }
}

void CDemuxStreamVideoFFmpeg::GetStreamInfo(std::string& strInfo)
{
  if(!m_stream) return;
  char temp[128];
  m_parent->m_dllAvCodec.avcodec_string(temp, 128, m_stream->codec, 0);
  strInfo = temp;
}

void CDemuxStreamSubtitleFFmpeg::GetStreamInfo(std::string& strInfo)
{
  if(!m_stream) return;
  char temp[128];
  m_parent->m_dllAvCodec.avcodec_string(temp, 128, m_stream->codec, 0);
  strInfo = temp;
}

// these need to be put somewhere that are compiled, we should have some better place for it

CCriticalSection DllAvCodec::m_critSection;
std::map<uintptr_t, CStdString> g_logbuffer;

void ff_avutil_log(void* ptr, int level, const char* format, va_list va)
{
#ifdef HAS_INTEL_SMD
#if 0
  CStdString m,p;
  m.FormatV(format,va);
  p.Format("ffmpeg: ");
  printf("%s%s", p.c_str(), m.c_str());
#endif // 0
  return;
#endif // HAS_INTEL_SMD

  CSingleLock lock(DllAvCodec::m_critSection);
  uintptr_t threadId = (uintptr_t)CThread::GetCurrentThreadId();
  CStdString &buffer = g_logbuffer[threadId];

  AVClass* avc= ptr ? *(AVClass**)ptr : NULL;

  int type;
  switch(level)
  {
    case AV_LOG_INFO   : type = LOGINFO;    break;
    case AV_LOG_ERROR  : type = LOGERROR;   break;
    case AV_LOG_DEBUG  :
    default            : type = LOGDEBUG;   break;
  }

  CStdString message, prefix;
  message.FormatV(format, va);

  prefix.Format("ffmpeg[%X]: ", threadId);
  if(avc)
  {
    if(avc->item_name)
      prefix += CStdString("[") + avc->item_name(ptr) + "] ";
    else if(avc->class_name)
      prefix += CStdString("[") + avc->class_name + "] ";
  }

  buffer += message;
  int pos, start = 0;
  while( (pos = buffer.find_first_of('\n', start)) >= 0 )
  {
    if(pos>start)
      CLog::Log(type, "%s%s", prefix.c_str(), buffer.substr(start, pos-start).c_str());
    start = pos+1;
  }
  buffer.erase(0, start);
}

static void ff_flush_avutil_log_buffers(void)
{
  CSingleLock lock(DllAvCodec::m_critSection);

  /* Loop through the logbuffer list and remove any blank buffers
     If the thread using the buffer is still active, it will just
     add a new buffer next time it writes to the log */
  std::map<uintptr_t, CStdString>::iterator it;
  for (it = g_logbuffer.begin(); it != g_logbuffer.end(); )
    if ((*it).second.IsEmpty())
      g_logbuffer.erase(it++);
    else
      ++it;
}

#ifdef _MSC_VER
static __declspec(thread) CDVDDemuxFFmpeg* g_demuxer = 0;
#else
static TLS g_tls;
#define g_demuxer (*((CDVDDemuxFFmpeg**)g_tls.Get()))
#endif

static int interrupt_cb(void)
{
  if(g_demuxer && g_demuxer->Aborted())
    return 1;
  return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
/*
static int dvd_file_open(URLContext *h, const char *filename, int flags)
{
  return -1;
}
*/

static int dvd_file_read(URLContext *h, BYTE* buf, int size)
{
  if(interrupt_cb())
    return -1;

  CDVDInputStream* pInputStream = (CDVDInputStream*)h->priv_data;
  return pInputStream->Read(buf, size);
}
static int dvd_file_swab_read(URLContext* h, BYTE* buf, int size)
{
  if(interrupt_cb())
    return -1;

  CDVDInputStream* pInputStream = (CDVDInputStream*)h->priv_data;
  int len = pInputStream->Read(buf, size);
  if( len > 0 )
    swab( (char*)buf, (char*)buf, len );

  return len;
}

/*
static int dvd_file_write(URLContext *h, BYTE* buf, int size)
{
  return -1;
}
*/
static offset_t dvd_file_seek(URLContext *h, offset_t pos, int whence)
{
  if(interrupt_cb())
    return -1;

  CDVDInputStream* pInputStream = (CDVDInputStream*)h->priv_data;
  if(whence == AVSEEK_SIZE)
    return pInputStream->GetLength();
  else
    return pInputStream->Seek(pos, whence);
}

static int dvd_file_close(URLContext *h)
{
  return 0;
}

URLProtocol dvd_file_protocol = {
                                  "CDVDInputStream",
                                  NULL,                 /*url_open*/
                                  dvd_file_read,        /*url_read*/
                                  NULL,                 /*url_write*/
                                  dvd_file_seek,        /*url_seek*/
                                  dvd_file_close,       /*url_close*/
                                  NULL,                 /*URLProtocol*/
                                  NULL,                 /*url_read_pause*/
                                  NULL,                 /*url_read_seek*/
                                  NULL,                 /*url_get_file_handle*/
                                  0,                    /*priv_data_size*/
                                  NULL                  /*priv_data_class*/
                                };

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

CDVDDemuxFFmpeg::CDVDDemuxFFmpeg() : CDVDDemux()
{
  m_pFormatContext = NULL;
  m_pInput = NULL;
  m_ioContext = NULL;
  InitializeCriticalSection(&m_critSection);
  for (int i = 0; i < MAX_STREAMS; i++) m_streams[i] = NULL;
  m_iCurrentPts = DVD_NOPTS_VALUE;
  m_swab = false;
  m_bBluray = false;
  m_lastVideoDTS = m_lastAudioDTS = 0.0f;
  m_currentAudioStream = -1;
  m_dtsCorrection = 0.0f;
  m_bAVI = false;
  m_bASF = false;
}

CDVDDemuxFFmpeg::~CDVDDemuxFFmpeg()
{
  Dispose();
  DeleteCriticalSection(&m_critSection);
  ff_flush_avutil_log_buffers();
}

bool CDVDDemuxFFmpeg::Aborted()
{
  if(!m_timeout)
    return false;

  if(CTimeUtils::GetTimeMS() > m_timeout)
    return true;

  return false;
}

bool CDVDDemuxFFmpeg::Open(CDVDInputStream* pInput)
{
  AVInputFormat* iformat = NULL;
  std::string strFile;
  m_iCurrentPts = DVD_NOPTS_VALUE;
  m_speed = DVD_PLAYSPEED_NORMAL;
  g_demuxer = this;

  if (!pInput) return false;

  if (!m_dllAvUtil.Load() || !m_dllAvCore.Load() || !m_dllAvCodec.Load() || !m_dllAvFormat.Load())  {
    CLog::Log(LOGERROR,"CDVDDemuxFFmpeg::Open - failed to load ffmpeg libraries");
    return false;
  }

  // register codecs
  m_dllAvFormat.av_register_all();
  m_dllAvFormat.url_set_interrupt_cb(interrupt_cb);

  // could be used for interupting ffmpeg while opening a file (eg internet streams)
  // url_set_interrupt_cb(NULL);

  m_pInput = pInput;
  strFile = m_pInput->GetFileName();

  bool streaminfo = true; /* set to true if we want to look for streams before playback*/

  if( m_pInput->GetContent().length() > 0 )
  {
    std::string content = m_pInput->GetContent();

    /* check if we can get a hint from content */
    if( content.compare("audio/aacp") == 0 )
      iformat = m_dllAvFormat.av_find_input_format("aac");
    else if( content.compare("audio/aac") == 0 )
      iformat = m_dllAvFormat.av_find_input_format("aac");
    else if( content.compare("video/x-vobsub") == 0 )
      iformat = m_dllAvFormat.av_find_input_format("mpeg");
    else if( content.compare("video/x-dvd-mpeg") == 0 )
      iformat = m_dllAvFormat.av_find_input_format("mpeg");
    else if( content.compare("video/flv") == 0 )
      iformat = m_dllAvFormat.av_find_input_format("flv");
    else if( content.compare("video/x-flv") == 0 )
      iformat = m_dllAvFormat.av_find_input_format("flv");
    else if( content.compare("video/nsv") == 0 )
      iformat = m_dllAvFormat.av_find_input_format("nsv");
    else if( content.compare("application/vnd.apple.mpegurl") == 0 )
      iformat = m_dllAvFormat.av_find_input_format("mpegts");
    else if( content.compare("video/x-mpegts") == 0 )
      iformat = m_dllAvFormat.av_find_input_format("mpegts");

    /* these are likely pure streams, and as such we don't */
    /* want to try to look for streaminfo before playback */
    if( iformat )
      streaminfo = false;
  }

  if( m_pInput->IsStreamType(DVDSTREAM_TYPE_FFMPEG) )
  {
    m_timeout = CTimeUtils::GetTimeMS() + 10000;

    // special stream type that makes avformat handle file opening
    // allows internal ffmpeg protocols to be used
    if( m_dllAvFormat.av_open_input_file(&m_pFormatContext, strFile.c_str(), iformat, FFMPEG_FILE_BUFFER_SIZE, NULL) < 0 )
    {
      CLog::Log(LOGDEBUG, "Error, could not open file %s", strFile.c_str());
      Dispose();
      return false;
    }
  }
  else
  {
    m_timeout = 0;

    // initialize url context to be used as filedevice
    URLContext* context = (URLContext*)m_dllAvUtil.av_mallocz(sizeof(struct URLContext) + strFile.length() + 1);
    context->prot = &dvd_file_protocol;
    context->priv_data = (void*)m_pInput;
    context->max_packet_size = FFMPEG_FILE_BUFFER_SIZE;

    // By default, if we can seek then mark as not streamed so ffmpeg will issue seeks
    context->is_streamed = (m_pInput->Seek(0, SEEK_POSSIBLE) == 0);

    // Reset this in case we were swapping bytes earlier
    if( !m_swab )
    {
      dvd_file_protocol.url_read = dvd_file_read;
    }

    if (m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD))
    {
      context->max_packet_size = FFMPEG_DVDNAV_BUFFER_SIZE;
      context->is_streamed = 1;
    }
#ifdef HAVE_LIBBLURAY
    if (m_pInput->IsStreamType(DVDSTREAM_TYPE_BLURAY))
    {
      context->max_packet_size = 6144;
      context->is_streamed = 1;
      m_bBluray = true;
    }
#endif
    else if (m_pInput->IsStreamType(DVDSTREAM_TYPE_TV))
    {
      if(m_pInput->Seek(0, SEEK_POSSIBLE) == 0)
        context->is_streamed = 1;

      // this actually speeds up channel changes by almost a second
      // however, it alsa makes player not buffer anything, this
      // leads to buffer underruns in audio renderer
      //if(context->is_streamed)
      //  streaminfo = false;
    }

    CLog::Log(LOGDEBUG, "%s - marking content streaming status as %s",
              __FUNCTION__, context->is_streamed?"true":"false");

#if LIBAVFORMAT_VERSION_INT >= (52<<16)
    context->filename = (char *) &context[1];
#endif

    strcpy(context->filename, strFile.c_str());  

    // open our virtual file device
    if(m_dllAvFormat.url_fdopen(&m_ioContext, context) < 0)
    {
      CLog::Log(LOGERROR, "%s - Unable to init io context", __FUNCTION__);
      m_dllAvUtil.av_free(context);
      Dispose();
      return false;
    }

    if( iformat == NULL )
    {
      // let ffmpeg decide which demuxer we have to open
      AVProbeData pd;
      BYTE probe_buffer[FFMPEG_FILE_BUFFER_SIZE + AVPROBE_PADDING_SIZE];

      // init probe data
      pd.buf = probe_buffer;
      pd.filename = strFile.c_str();

      // read data using avformat's buffers
        pd.buf_size = m_dllAvFormat.get_buffer(m_ioContext, pd.buf, context->max_packet_size);

      if (pd.buf_size <= 0)
      {
        CLog::Log(LOGERROR, "%s - error reading from input stream, %s", __FUNCTION__, strFile.c_str());
        return false;
      }
      memset(pd.buf+pd.buf_size, 0, AVPROBE_PADDING_SIZE);

      // restore position again
      m_dllAvFormat.url_fseek(m_ioContext , 0, SEEK_SET);

      iformat = m_dllAvFormat.av_probe_input_format(&pd, 1);

      if (!iformat)
      {
        // probing the input format failed. there are a couple of cases -
        //  libavformat needs more info
        //  the input stream is byteswapped

        // for now, we can swab AC3/EAC3 standalone streams only. we cannot handle
        // byte reversed substreams etc
        if( 0x77 == probe_buffer[0]
         && 0x0b == probe_buffer[1] )
        {
          // found AC3 syncword. try to swab the data
          BYTE swab_buffer[FFMPEG_FILE_BUFFER_SIZE + AVPROBE_PADDING_SIZE];
          swab( (char*) probe_buffer, (char*)swab_buffer, pd.buf_size );
          pd.buf = swab_buffer;
          iformat = m_dllAvFormat.av_probe_input_format(&pd, 1);
          if( iformat )
          {
            CLog::Log(LOGDEBUG, "DVDDemuxFFmpeg: Reopening as byteswapped stream\n");
            // we need to swab everything now. reopen the stream
            m_swab = true;
            dvd_file_protocol.url_read = dvd_file_swab_read;

            m_dllAvUtil.av_free(context);
            Dispose();

            pInput->Seek(0, SEEK_SET);
            // Do things the easy way and recurse one level
            return Open(pInput);
          }
          else
          {
            // Not byteswapped, restore things to their previous order
            pd.buf = probe_buffer;
          }
        }
      }

      if( !iformat )
      {
        // av_probe_input_format failed, re-probe the ffmpeg/ffplay method.
        // av_open_input_file uses av_probe_input_format2 for probing format, 
        // starting at 2048, up to max buffer size of 1048576. We just probe to 
        // the buffer size allocated above so as to avoid seeks on content that 
        // might not be seekable.
        int max_buf_size = pd.buf_size;
        for (int probe_size=std::min(2048, pd.buf_size); probe_size <= max_buf_size && !iformat; probe_size<<=1) 
        {
          CLog::Log(LOGDEBUG, "%s - probing failed, re-probing with probe size [%d]", __FUNCTION__, probe_size); 
          int score= probe_size < max_buf_size ? AVPROBE_SCORE_MAX/4 : 0;

       // Use this workaround as av_probe_input_format2 is failing
          score = 0;
          pd.buf_size = probe_size;
          iformat = m_dllAvFormat.av_probe_input_format2(&pd, 1, &score);
        }
      }
      if (!iformat)
      {
        CLog::Log(LOGERROR, "%s - error probing input format, %s", __FUNCTION__, strFile.c_str());
        return false;
      }
      else
      {
        if (iformat->name)
          CLog::Log(LOGDEBUG, "%s - probing detected format [%s]", __FUNCTION__, iformat->name);
        else
          CLog::Log(LOGDEBUG, "%s - probing detected unnamed format", __FUNCTION__);
      }
    }


    // open the demuxer
    if (m_dllAvFormat.av_open_input_stream(&m_pFormatContext, m_ioContext, strFile.c_str(), iformat, NULL) < 0)
    {
      CLog::Log(LOGERROR, "%s - Error, could not open stream %s", __FUNCTION__, strFile.c_str());
      Dispose();
      return false;
    }

  }
  
  // we need to know if this is matroska later
  m_bMatroska = strncmp(m_pFormatContext->iformat->name, "matroska", 8) == 0; // for "matroska.webm"

  // We use avi for hardware decoder gen_pts flag
  m_bAVI = strcmp(m_pFormatContext->iformat->name, "avi") == 0;

  m_bASF = strcmp(m_pFormatContext->iformat->name, "asf") == 0;

  // Prevent ffmpeg from looking into playlist for type
  if (m_pInput->IsStreamType(DVDSTREAM_TYPE_PLAYLIST))
  {
    streaminfo = true;
  }

  if (streaminfo)
  {
    /* too speed up dvd switches, only analyse very short */
    if (m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD))
      m_pFormatContext->max_analyze_duration = 500000;

    CLog::Log(LOGDEBUG, "%s - av_find_stream_info starting", __FUNCTION__);
    int iErr = m_dllAvFormat.av_find_stream_info(m_pFormatContext);
    if (iErr < 0)
    {
      CLog::Log(LOGWARNING, "could not find codec parameters for %s", strFile.c_str());

      if (m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD) || (m_pFormatContext->nb_streams == 1
              && m_pFormatContext->streams[0]->codec->codec_id == CODEC_ID_AC3))
      {
        // special case, our codecs can still handle it.
      }
      else
      {
        Dispose();
        return false;
      }
    }

    // workaround for HLS video playback issues
    // we had to remove codec's extradata, otherwise MLB video will be jittering
    // TODO: check if occurs after update to new version of ffmpeg
    if (m_pInput->IsStreamType(DVDSTREAM_TYPE_PLAYLIST))
    {
      // workaround for backword seek issue in hls live streams
      m_pFormatContext->start_time = (int64_t) AV_NOPTS_VALUE;
      m_pFormatContext->raw_packet_buffer_remaining_size = 0;

      for(unsigned int i=0; i<m_pFormatContext->nb_streams; i++)
      {
        if(m_pFormatContext->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
        {
          AVCodecContext* codec = m_pFormatContext->streams[i]->codec;
          if(codec->extradata_size > 0 && codec->extradata)
          {
            codec->extradata_size = 0;
            m_dllAvUtil.av_free(codec->extradata);
            codec->extradata = NULL;
            break;
          }
        }
      }

    }
    CLog::Log(LOGDEBUG, "%s - av_find_stream_info finished", __FUNCTION__);
  }

  // reset any timeout
  m_timeout = 0;

  // if format can be nonblocking, let's use that
  m_pFormatContext->flags |= AVFMT_FLAG_NONBLOCK;

  if( m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD) )
    m_pFormatContext->flags |= AVFMT_FLAG_GENPTS;

  // print some extra information
  m_dllAvFormat.dump_format(m_pFormatContext, 0, strFile.c_str(), 0);

  UpdateCurrentPTS();


  // This was comment since we need to know if mms contains video/audio for SMD time/untimed playback
  /*
  if (m_pInput->IsStreamType(DVDSTREAM_TYPE_MMS)) // HACK: until we figure out how to detect which mms streams are active
    return true;
    */

#ifdef HAS_DVB
  // add the ffmpeg streams to our own stream array
  if(m_pInput->IsStreamType(DVDSTREAM_TYPE_TV) && m_pFormatContext->nb_programs)
  {
    CDVDInputStream::IChannel* input = dynamic_cast<CDVDInputStream::IChannel*>(m_pInput);
    int channelIndex = input->GetProgramId();
    DvbChannelPtr channel = DVBManager::GetInstance().GetChannels().GetChannelByIndex(channelIndex);
    int videoPid = channel->GetVideoPid();

    const std::vector<DvbAudioInfo>& audioInfo = channel->GetAudioPids();
    int audioPid = audioInfo[0].pid;

#if 0
    printf("search for: v=%d a=%d\n", videoPid, audioPid);

    for (unsigned int i = 0; i < m_pFormatContext->nb_programs; i++)
    {
      printf("PROGRAMS: %d\n", m_pFormatContext->programs[i]);
      for (unsigned int j = 0; j <  m_pFormatContext->programs[i]->nb_stream_indexes; j++)
      {
        printf("STREAM: index=%d\n", m_pFormatContext->programs[i]->stream_index[j]);
      }
    }

    for (unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
    {
      printf("STREAMS: index=%d id=%d\n", m_pFormatContext->streams[i]->index, m_pFormatContext->streams[i]->id);
    }
#endif

    int videoStreamIndex;
    int audioStreamIndex;

    for (unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
    {
      if (m_pFormatContext->streams[i]->id == videoPid)
      {
        videoStreamIndex = m_pFormatContext->streams[i]->index;
        printf("found video stream: %d\n", videoStreamIndex);
      }

      if (m_pFormatContext->streams[i]->id == audioPid)
      {
        audioStreamIndex = m_pFormatContext->streams[i]->index;
        printf("found audio stream: %d\n", audioStreamIndex);
      }
    }

    m_program = UINT_MAX;

    for (unsigned int i = 0; i < m_pFormatContext->nb_programs; i++)
    {
      for (unsigned int j = 0; j <  m_pFormatContext->programs[i]->nb_stream_indexes; j++)
      {
        if (m_pFormatContext->programs[i]->stream_index[j] == videoStreamIndex ||
            m_pFormatContext->programs[i]->stream_index[j] == audioStreamIndex)
        {
          m_program = i;
          printf("found program: %d\n", m_program);

          for (unsigned int n = 0; n < m_pFormatContext->programs[m_program]->nb_stream_indexes; n++)
            AddStream(m_pFormatContext->programs[m_program]->stream_index[n]);

          break;
        }
      }

      if (m_program != UINT_MAX && m_program != i)
      {
        m_pFormatContext->programs[i]->discard = AVDISCARD_ALL;
      }
    }

    if (m_program == UINT_MAX)
    {
      m_program = 0;
    }
  }
  else
#endif
  if (m_pFormatContext->nb_programs)
  {
    m_program = UINT_MAX;

    for (unsigned int i = 0; i < m_pFormatContext->nb_programs; i++)
    {
      // look for first non empty stream and discard nonselected programs
      if(m_program == UINT_MAX && m_pFormatContext->programs[i]->nb_stream_indexes > 0)
        m_program = i;

      if(i != m_program)
        m_pFormatContext->programs[i]->discard = AVDISCARD_ALL;
    }

    if(m_program == UINT_MAX)
      m_program = 0;

    // add streams from selected program
    for (unsigned int i = 0; i < m_pFormatContext->programs[m_program]->nb_stream_indexes; i++)
      AddStream(m_pFormatContext->programs[m_program]->stream_index[i]);
  }
  else
  {
    for (unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
      AddStream(i);
  }

  return true;
}

#ifdef _WIN32
// This code segment is ment to duplicate FFMpeg's code, minus the code that is causing the dis-connectivity bug, as seen in http://roundup.libav.org/issue1849

#include "dvdplayer/Codecs/ffmpeg/libavformat/avio.h"

int CDVDDemuxFFmpeg::url_close(URLContext *h)
{
    int ret = 0;
    if (!h) return 0; /* can happen when url_open fails */

    if (h->is_connected && h->prot->url_close)
        ret = h->prot->url_close(h);

	// The call to ff_network_close() is ommited.

    if (h->prot->priv_data_size)
        m_dllAvUtil.av_free(h->priv_data);
    m_dllAvUtil.av_free(h);
    return ret;
}

int CDVDDemuxFFmpeg::url_fclose(ByteIOContext *s)
{
    URLContext *h = (URLContext *) s->opaque;

    m_dllAvUtil.av_free(s->buffer);
    m_dllAvUtil.av_free(s);
    return url_close(h);
}
#endif

void CDVDDemuxFFmpeg::Dispose()
{
  g_demuxer = this;

  if (m_pFormatContext)
  {
    if (m_ioContext)
    {
      if(m_pFormatContext->pb && m_pFormatContext->pb != m_ioContext)
      {
        CLog::Log(LOGWARNING, "CDVDDemuxFFmpeg::Dispose - demuxer changed our byte context behind our back, possible memleak");
        m_ioContext = m_pFormatContext->pb;
      }
      m_dllAvFormat.av_close_input_stream(m_pFormatContext);
#ifdef _WIN32
      url_fclose(m_ioContext);	// Obviously, we don't need to pass a class member as an arg to the class method. This is only done to remain as close as possible to FFMpeg's url_fclose() code.
#else
	  m_dllAvFormat.url_fclose(m_ioContext);
#endif
    }
    else
      m_dllAvFormat.av_close_input_file(m_pFormatContext);
  }
  m_ioContext = NULL;
  m_pFormatContext = NULL;
  m_speed = DVD_PLAYSPEED_NORMAL;

  for (int i = 0; i < MAX_STREAMS; i++)
  {    
    if (m_streams[i]) 
    {
      if (m_streams[i]->ExtraData)
        delete[] (BYTE*)(m_streams[i]->ExtraData);
      delete m_streams[i];
    }
    m_streams[i] = NULL;
  }  
  m_pInput = NULL;
  m_lastVideoDTS = m_lastAudioDTS = m_dtsCorrection = 0.0f;
  m_currentAudioStream = -1;
  m_dllAvFormat.Unload();
  m_dllAvCodec.Unload();
  m_dllAvCore.Unload();
  m_dllAvUtil.Unload();
}

void CDVDDemuxFFmpeg::Reset()
{
  CDVDInputStream* pInputStream = m_pInput;
  Dispose();
  Open(pInputStream);
}

void CDVDDemuxFFmpeg::Flush()
{
  g_demuxer = this;

  // naughty usage of internal ffmpeg data
  // required so we clean out buffered data on seek ops
  if (m_pFormatContext)
    av_read_frame_flush(m_pFormatContext);

  m_iCurrentPts = DVD_NOPTS_VALUE;

  m_lastVideoDTS = m_lastAudioDTS = m_dtsCorrection = 0.0f;
}

void CDVDDemuxFFmpeg::Abort()
{
  m_timeout = 1;
}

void CDVDDemuxFFmpeg::SetSpeed(int iSpeed)
{
  g_demuxer = this;

  if(!m_pFormatContext)
    return;

  if(m_speed != DVD_PLAYSPEED_PAUSE && iSpeed == DVD_PLAYSPEED_PAUSE)
  {
    m_pInput->Pause((double)m_iCurrentPts);
    m_dllAvFormat.av_read_pause(m_pFormatContext);
  }
  else if(m_speed == DVD_PLAYSPEED_PAUSE && iSpeed != DVD_PLAYSPEED_PAUSE)
  {
    m_pInput->Pause((double)m_iCurrentPts);
    m_dllAvFormat.av_read_play(m_pFormatContext);
  }
  m_speed = iSpeed;

  AVDiscard discard = AVDISCARD_NONE;
  if(m_speed > 4*DVD_PLAYSPEED_NORMAL)
    discard = AVDISCARD_NONKEY;
  else if(m_speed > 2*DVD_PLAYSPEED_NORMAL)
    discard = AVDISCARD_BIDIR;
  else if(m_speed < DVD_PLAYSPEED_PAUSE)
    discard = AVDISCARD_NONKEY;


  for(unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
  {
    if(m_pFormatContext->streams[i])
    {
      if(m_pFormatContext->streams[i]->discard != AVDISCARD_ALL)
        m_pFormatContext->streams[i]->discard = discard;
    }
  }
}

double CDVDDemuxFFmpeg::ConvertTimestamp(int64_t pts, int den, int num)
{
  if (pts == (int64_t)AV_NOPTS_VALUE)
    return DVD_NOPTS_VALUE;

  // do calculations in floats as they can easily overflow otherwise
  // we don't care for having a completly exact timestamp anyway
  double timestamp = (double)pts * num  / den;
  double starttime = 0.0f;

  // for dvd's we need the original time
  if(m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD))
    starttime = static_cast<CDVDInputStreamNavigator*>(m_pInput)->GetTimeStampCorrection() / DVD_TIME_BASE;
  else if (m_pFormatContext->start_time != (int64_t)AV_NOPTS_VALUE)
    starttime = (double)m_pFormatContext->start_time / AV_TIME_BASE;

  if(timestamp > starttime)
    timestamp -= starttime;
  else if( timestamp + 0.1f > starttime )
    timestamp = 0;
  else if( timestamp < starttime )
    timestamp = 0;

  return timestamp*DVD_TIME_BASE;
}

DemuxPacket* CDVDDemuxFFmpeg::Read()
{
  g_demuxer = this;

  AVPacket pkt;
  DemuxPacket* pPacket = NULL;
  // on some cases where the received packet is invalid we will need to return an empty packet (0 length) otherwise the main loop (in CDVDPlayer) 
  // would consider this the end of stream and stop.
  bool bReturnEmpty = false;
  int av_read_frame_result = -1;
  Lock();
  if (m_pFormatContext)
  {
    // assume we are not eof
    if(m_pFormatContext->pb)
      m_pFormatContext->pb->eof_reached = 0;

    // keep track if ffmpeg doesn't always set these
    pkt.size = 0;
    pkt.data = NULL;
    pkt.stream_index = MAX_STREAMS;

    // timeout reads after 100ms
    m_timeout = CTimeUtils::GetTimeMS() + 20000;
    int result = 0;

    try
    {
      result = m_dllAvFormat.av_read_frame(m_pFormatContext, &pkt);
      av_read_frame_result = result;
    }
    catch(const win32_exception &e)
    {
      e.writelog(__FUNCTION__);
      result = AVERROR(EFAULT);
    }
    m_timeout = 0;

    if (result == AVERROR(EINTR) || result == AVERROR(EAGAIN) )
    {
      // timeout, probably no real error, return empty packet
      bReturnEmpty = true;
    }
    else if (result < 0)
    {
      Flush();
    }
    else if (pkt.size < 0 || pkt.stream_index >= MAX_STREAMS)
    {
      // XXX, in some cases ffmpeg returns a negative packet size
      if(m_pFormatContext->pb && !m_pFormatContext->pb->eof_reached)
      {
        CLog::Log(LOGERROR, "CDVDDemuxFFmpeg::Read() no valid packet");
        bReturnEmpty = true;
        Flush();
      }
      else
        CLog::Log(LOGERROR, "CDVDDemuxFFmpeg::Read() returned invalid packet and eof reached");

      m_dllAvCodec.av_free_packet(&pkt);
    }
    else
    {
      AVStream *stream = m_pFormatContext->streams[pkt.stream_index];

      if (m_pFormatContext->nb_programs)
      {
        /* check so packet belongs to selected program */
        for (unsigned int i = 0; i < m_pFormatContext->programs[m_program]->nb_stream_indexes; i++)
        {
          if(pkt.stream_index == (int)m_pFormatContext->programs[m_program]->stream_index[i])
          {
            pPacket = CDVDDemuxUtils::AllocateDemuxPacket(pkt.size);
            break;
          }
        }

        if (!pPacket)
          bReturnEmpty = true;
      }
      else
        pPacket = CDVDDemuxUtils::AllocateDemuxPacket(pkt.size);


      if (pPacket)
      {
        // lavf sometimes bugs out and gives 0 dts/pts instead of no dts/pts
        // since this could only happens on initial frame under normal
        // circomstances, let's assume it is wrong all the time
        if(pkt.dts == 0)
          pkt.dts = AV_NOPTS_VALUE;
        if(pkt.pts == 0)
          pkt.pts = AV_NOPTS_VALUE;

        if(m_bMatroska && stream->codec && stream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        { // matroska can store different timestamps
          // for different formats, for native stored
          // stuff it is pts, but for ms compatibility
          // tracks, it is really dts. sadly ffmpeg
          // sets these two timestamps equal all the
          // time, so we select it here instead
          if(stream->codec->codec_tag == 0)
            pkt.dts = AV_NOPTS_VALUE;
          else
            pkt.pts = AV_NOPTS_VALUE;
        }

        // we need to get duration slightly different for matroska embedded text subtitels
        if(m_bMatroska && stream->codec->codec_id == CODEC_ID_TEXT && pkt.convergence_duration != 0)
            pkt.duration = pkt.convergence_duration;

        // copy contents into our own packet
        pPacket->iSize = pkt.size;

        // maybe we can avoid a memcpy here by detecting where pkt.destruct is pointing too?
        if (pkt.data)
          memcpy(pPacket->pData, pkt.data, pPacket->iSize);

        pPacket->pts = ConvertTimestamp(pkt.pts, stream->time_base.den, stream->time_base.num);
        pPacket->dts = ConvertTimestamp(pkt.dts, stream->time_base.den, stream->time_base.num);
        pPacket->duration =  DVD_SEC_TO_TIME((double)pkt.duration * stream->time_base.num / stream->time_base.den);

#ifdef HAS_INTEL_SMD
        //  m2ts streams should all have the pts set; on a clipchange we tend to
        //  get a video packet with pts == NOPTS and a dts that is the old timestamp
        if (m_bBluray)
        {
          if (pPacket->dts > 0 && pPacket->pts == DVD_NOPTS_VALUE)
          {
            pPacket->dts = DVD_NOPTS_VALUE;
          }
        }

        // We need to check if there gaps in the stream and correct pts in case.
        // We detect gaps if these are more than 5 seconds
        if (pPacket->dts != DVD_NOPTS_VALUE && stream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
          if (m_lastVideoDTS && abs(((int) (m_dtsCorrection + pPacket->dts)) - m_lastVideoDTS) > DVD_SEC_TO_TIME(5))
          {
            // recalculate the DTS jump
            CLog::Log(LOGDEBUG, "m_dtsCorrection %.2f m_lastDTS %.2f pPacket->dts %.2f", m_dtsCorrection, m_lastVideoDTS, pPacket->dts);
            m_dtsCorrection = m_lastVideoDTS - pPacket->dts;
            CLog::Log(LOGDEBUG, "CDVDDemuxFFmpeg::gap detected, changing dts correction to %.2f\n", m_dtsCorrection);
          }

          m_lastVideoDTS = m_dtsCorrection + pPacket->dts;
        }

        if(pPacket->dts != DVD_NOPTS_VALUE)
          pPacket->dts += m_dtsCorrection;
        if(pPacket->pts != DVD_NOPTS_VALUE)
          pPacket->pts += m_dtsCorrection;

        // Matroska embedded DTS-HD has improper timing. This happens when mkv tools
        // remux the 90khz bluray timing for most DTS-HD sources into 100khz mkv timing,
        // and ffmpeg reduces the track timing to ms accuracy. So here we try to fix things. We anchor
        // the timing off one pts value - needed for sync after seek - then ignore the rest
        // of the timing data and just supply the duration
        if(m_bMatroska && stream->codec && (stream->codec->codec_id == CODEC_ID_DTS) && m_currentAudioStream == pkt.stream_index)
        {
          // calculate the proper frame duration since ffmpeg doesn't
          double duration = ((double)stream->codec->frame_size / stream->codec->sample_rate);

          if( m_lastAudioDTS == 0.0f && pPacket->pts != DVD_NOPTS_VALUE )
          {
            m_lastAudioDTS = pPacket->pts;
          }
          else
          {
            pPacket->dts = DVD_NOPTS_VALUE;
            pPacket->pts = DVD_NOPTS_VALUE;
          }
          pPacket->duration = duration * DVD_TIME_BASE;
        }

        // track packed b frames in avis
        if (m_bAVI && pPacket->iSize == 7 && stream->codec->codec_id == CODEC_ID_MPEG4 &&
            stream->codec->coded_width * stream->codec->coded_height > 720 * 576)
        {
          m_pFormatContext->flags |= AVFMT_FLAG_NOFILLIN;
        }
#endif

#if 0
        // demuxer debugging
        char type = (stream->codec->codec_type == CODEC_TYPE_VIDEO ?
                      'v' : stream->codec->codec_type == CODEC_TYPE_AUDIO ? 'a' : 's');
        printf("demux stream %d type %c pts %12.2f dts %12.2f dur %12.2f size %d\n", pkt.stream_index, type,
               pPacket->pts == DVD_NOPTS_VALUE ? -1.0f : pPacket->pts,
               pPacket->dts == DVD_NOPTS_VALUE ? -1.0f : pPacket->dts,
               pPacket->duration, pPacket->iSize);
#endif
        // used to guess streamlength
        if (pPacket->dts != DVD_NOPTS_VALUE && (pPacket->dts > m_iCurrentPts || m_iCurrentPts == DVD_NOPTS_VALUE))
          m_iCurrentPts = pPacket->dts;


        // check if stream has passed full duration, needed for live streams
        if(pkt.dts != (int64_t)AV_NOPTS_VALUE)
        {
            int64_t duration;
            duration = pkt.dts;
            if(stream->start_time != (int64_t)AV_NOPTS_VALUE)
              duration -= stream->start_time;

            if(duration > stream->duration)
            {
              stream->duration = duration;
              duration = m_dllAvUtil.av_rescale_rnd(stream->duration, stream->time_base.num * AV_TIME_BASE, stream->time_base.den, AV_ROUND_NEAR_INF);
              if ((m_pFormatContext->duration == (int64_t)AV_NOPTS_VALUE && m_pFormatContext->file_size > 0)
              ||  (m_pFormatContext->duration != (int64_t)AV_NOPTS_VALUE && duration > m_pFormatContext->duration))
                m_pFormatContext->duration = duration;
            }
        }

        // check if stream seem to have grown since start
        if(m_pFormatContext->file_size > 0 && m_pFormatContext->pb)
        {
          if(m_pFormatContext->pb->pos > m_pFormatContext->file_size)
            m_pFormatContext->file_size = m_pFormatContext->pb->pos;
        }

        pPacket->iStreamId = pkt.stream_index; // XXX just for now

      }
      m_dllAvCodec.av_free_packet(&pkt);
    }
  }
  Unlock();

  if (bReturnEmpty && !pPacket)
      pPacket = CDVDDemuxUtils::AllocateDemuxPacket(0);

  if (!pPacket)
  {
    //printf("### CDVDDemuxFFmpeg::Read returned NULL. pPacket %d bReturnEmpty %d eof_reached %d av_read_frame_result %d\n",
     //   pPacket, bReturnEmpty, m_pFormatContext->pb->eof_reached, av_read_frame_result);
    return NULL;
  }

  // check streams, can we make this a bit more simple?
  if (pPacket && pPacket->iStreamId >= 0 && pPacket->iStreamId <= MAX_STREAMS)
  {
    if (!m_streams[pPacket->iStreamId] ||
        m_streams[pPacket->iStreamId]->pPrivate != m_pFormatContext->streams[pPacket->iStreamId] ||
        m_streams[pPacket->iStreamId]->codec != m_pFormatContext->streams[pPacket->iStreamId]->codec->codec_id)
    {
      // content has changed, or stream did not yet exist
      AddStream(pPacket->iStreamId);
    }
    // we already check for a valid m_streams[pPacket->iStreamId] above
    else if (m_streams[pPacket->iStreamId]->type == STREAM_AUDIO)
    {
      if (((CDemuxStreamAudio*)m_streams[pPacket->iStreamId])->iChannels != m_pFormatContext->streams[pPacket->iStreamId]->codec->channels ||
          ((CDemuxStreamAudio*)m_streams[pPacket->iStreamId])->iSampleRate != m_pFormatContext->streams[pPacket->iStreamId]->codec->sample_rate)
      {
        // content has changed
        AddStream(pPacket->iStreamId);
      }
    }
    else if (m_streams[pPacket->iStreamId]->type == STREAM_VIDEO)
    {
      if (((CDemuxStreamVideo*)m_streams[pPacket->iStreamId])->iWidth != m_pFormatContext->streams[pPacket->iStreamId]->codec->width ||
          ((CDemuxStreamVideo*)m_streams[pPacket->iStreamId])->iHeight != m_pFormatContext->streams[pPacket->iStreamId]->codec->height)
      {
        // content has changed
        AddStream(pPacket->iStreamId);
      }
    }
  }
  return pPacket;
}

bool CDVDDemuxFFmpeg::SeekTime(int time, bool backwords, double *startpts)
{
  g_demuxer = this;

  if(time < 0)
    time = 0;

  if(!m_pInput)
    return false;

  m_lastVideoDTS = m_lastAudioDTS = m_dtsCorrection = 0.0f;

  if (m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CLog::Log(LOGDEBUG, "%s - seeking using navigator", __FUNCTION__);
    if (!((CDVDInputStreamNavigator*)m_pInput)->SeekTime(time))
      return false;

    if(startpts)
      *startpts = DVD_NOPTS_VALUE;
    
    Flush();

    // also empty the internal ffmpeg buffer (xbmc 32756)
    m_ioContext->buf_ptr = m_ioContext->buf_end;
    return true;
  }
#ifdef HAVE_LIBBLURAY
  if (m_pInput->IsStreamType(DVDSTREAM_TYPE_BLURAY))
  {
    if (!((CDVDInputStreamBluray*)m_pInput)->SeekTime(time))
      return false;

    if(startpts)
      *startpts = DVD_NOPTS_VALUE;

    Flush();

    m_ioContext->buf_ptr = m_ioContext->buf_end;
    return true;
  }
#endif
#ifdef HAS_FILESYSTEM_MMS 
  if (m_pInput->IsStreamType(DVDSTREAM_TYPE_MMS))
  {
    if (!((CDVDInputStreamMMS*)m_pInput)->SeekTime(time))
      return false;

    if(startpts)
      *startpts = DVD_NOPTS_VALUE;

    Flush();
    m_ioContext->buf_ptr = m_ioContext->buf_end;
    return true;
  }
#endif

  if (m_pInput->IsStreamType(DVDSTREAM_TYPE_RTMP))
  {
    if (!((CDVDInputStreamRTMP*)m_pInput)->SeekTime(time))
      return false;

    Flush();
    return true;
  }

  if (m_pInput->IsStreamType(DVDSTREAM_TYPE_PLAYLIST))
  {
    if (!((CDVDInputStreamPlaylist*)m_pInput)->SeekTime(time))
      return false;

	if(startpts)
      *startpts = DVD_NOPTS_VALUE;
    
    Flush();
    m_ioContext->buf_ptr = m_ioContext->buf_end;
    return true;
  }
  
  if(!m_pInput->Seek(0, SEEK_POSSIBLE) 
  && !m_pInput->IsStreamType(DVDSTREAM_TYPE_FFMPEG))
  {
    CLog::Log(LOGDEBUG, "%s - input stream reports it is not seekable", __FUNCTION__);
    return false;
  }

  __int64 seek_pts = (__int64)time * (AV_TIME_BASE / 1000);
  if (m_pFormatContext->start_time != (int64_t)AV_NOPTS_VALUE)
    seek_pts += m_pFormatContext->start_time;

  Lock();

  int ret = m_dllAvFormat.av_seek_frame(m_pFormatContext, -1, seek_pts, backwords ? AVSEEK_FLAG_BACKWARD : 0);
  
  if(ret >= 0)
    UpdateCurrentPTS();
  Unlock();

  if(m_iCurrentPts == DVD_NOPTS_VALUE)
    CLog::Log(LOGDEBUG, "%s - unknown position after seek", __FUNCTION__);
  else
    CLog::Log(LOGDEBUG, "%s - seek ended up on time %d", __FUNCTION__, (int)(m_iCurrentPts / DVD_TIME_BASE * 1000));

  // in this case the start time is requested time
  if(startpts)
    *startpts = DVD_MSEC_TO_TIME(time);

#if 0
  if(startpts)
    if(m_iCurrentPts != DVD_NOPTS_VALUE)
      *startpts = m_iCurrentPts;
#endif

  // demuxer will return failure, if you seek to eof
  if (m_pInput->IsEOF() && ret <= 0)
    return true;

  return (ret >= 0);
}

bool CDVDDemuxFFmpeg::SeekByte(__int64 pos)
{
  g_demuxer = this;

  Lock();
  int ret = m_dllAvFormat.av_seek_frame(m_pFormatContext, -1, pos, AVSEEK_FLAG_BYTE);

  if(ret >= 0)
    UpdateCurrentPTS();

  Unlock();
  return (ret >= 0);
}

void CDVDDemuxFFmpeg::UpdateCurrentPTS()
{
  m_iCurrentPts = DVD_NOPTS_VALUE;
  for(unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
  {
    AVStream *stream = m_pFormatContext->streams[i];
    if(stream && stream->cur_dts != (int64_t)AV_NOPTS_VALUE)
    {
      double ts = ConvertTimestamp(stream->cur_dts, stream->time_base.den, stream->time_base.num);
      if(m_iCurrentPts == DVD_NOPTS_VALUE || m_iCurrentPts > ts )
        m_iCurrentPts = ts;
    }
  }
}

int CDVDDemuxFFmpeg::GetStreamLength()
{
  if (!m_pFormatContext)
    return 0;

  /* apperently ffmpeg messes up sometimes, so check for negative value too */
  if (m_pFormatContext->duration == (int64_t)AV_NOPTS_VALUE || m_pFormatContext->duration < 0LL)
  {
    // no duration is available for us
    // try to calculate it
    int iLength = 0;
    if (m_iCurrentPts != DVD_NOPTS_VALUE && m_pFormatContext->file_size > 0 && m_pFormatContext->pb && m_pFormatContext->pb->pos > 0)
    {
      iLength = (int)(((m_iCurrentPts * m_pFormatContext->file_size) / m_pFormatContext->pb->pos) / 1000) & 0xFFFFFFFF;
    }
    return iLength;
  }

  return (int)(m_pFormatContext->duration / (AV_TIME_BASE / 1000));
}

CDemuxStream* CDVDDemuxFFmpeg::GetStream(int iStreamId)
{
  if (iStreamId < 0 || iStreamId >= MAX_STREAMS) return NULL;
  return m_streams[iStreamId];
}

int CDVDDemuxFFmpeg::GetNrOfStreams()
{
  int i = 0;
  while (i < MAX_STREAMS && m_streams[i]) i++;
  return i;
}

void CDVDDemuxFFmpeg::AddStream(int iId)
{
  AVStream* pStream = m_pFormatContext->streams[iId];
  if (pStream)
  {
    if (m_streams[iId]) 
    {
      if( m_streams[iId]->ExtraData ) delete[] (BYTE*)(m_streams[iId]->ExtraData);
      delete m_streams[iId];
    }

    switch (pStream->codec->codec_type)
    {
    case AVMEDIA_TYPE_AUDIO:
      {
        CDemuxStreamAudioFFmpeg* st = new CDemuxStreamAudioFFmpeg(this, pStream);
        m_streams[iId] = st;
        st->iChannels = pStream->codec->channels;
        st->iSampleRate = pStream->codec->sample_rate;
        st->iBlockAlign = pStream->codec->block_align;
        st->iBitRate = pStream->codec->bit_rate;
        st->iBitsPerSample = pStream->codec->bits_per_coded_sample;

        if(m_pInput->IsStreamType(DVDSTREAM_TYPE_PLAYLIST) &&
            (st->iChannels != 0 && st->iSampleRate != 0))
        {
          pStream->disposition |= CDemuxStream::FLAG_DEFAULT;
        }

        if(m_dllAvFormat.av_metadata_get(pStream->metadata, "title", NULL, 0))
          st->m_description = m_dllAvFormat.av_metadata_get(pStream->metadata, "title", NULL, 0)->value;

        break;
      }
    case AVMEDIA_TYPE_VIDEO:
      {
        CDemuxStreamVideoFFmpeg* st = new CDemuxStreamVideoFFmpeg(this, pStream);
        m_streams[iId] = st;
        if(strcmp(m_pFormatContext->iformat->name, "flv") == 0)
          st->bVFR = true;
        else
          st->bVFR = false;

#ifdef HAS_INTEL_SMD
        if(pStream->codec->codec_id == CODEC_ID_MPEG2VIDEO && !m_bASF)
        {
          m_pFormatContext->flags |= AVFMT_FLAG_GENPTS;
        }

        if (m_bAVI)
        {
          // this will be decoded with SMD
          if (pStream->codec->coded_width * pStream->codec->coded_height > 720 * 576)
          {
            m_pFormatContext->flags |= AVFMT_FLAG_GENPTS;
          }
        }
#endif

        if(pStream->r_frame_rate.den && pStream->r_frame_rate.num)
        {
          st->iFpsRate = pStream->r_frame_rate.num;
          st->iFpsScale = pStream->r_frame_rate.den;
        }
        else
        {
          st->iFpsRate  = 0;
          st->iFpsScale = 0;
        }
        st->iWidth = pStream->codec->width;
        st->iHeight = pStream->codec->height;
        if (pStream->sample_aspect_ratio.num == 0)
          st->fAspect = 0.0;
        else 
          st->fAspect = av_q2d(pStream->sample_aspect_ratio) * pStream->codec->width / pStream->codec->height;

        break;
      }
    case AVMEDIA_TYPE_DATA:
      {
#if LIBAVFORMAT_VERSION_MAJOR < 53 && LIBAVFORMAT_VERSION_MINOR < 64
        if (pStream->codec->codec_id == CODEC_ID_EBU_TELETEXT && g_guiSettings.GetBool("videoplayer.teletextenabled"))
        {
          CDemuxStreamTeletext* st = new CDemuxStreamTeletext();
          m_streams[iId] = st;
          m_streams[iId]->type = STREAM_TELETEXT;
          break;
        }
        else
#endif
        {
        m_streams[iId] = new CDemuxStream();
        m_streams[iId]->type = STREAM_DATA;
        break;
      }
      }
    case AVMEDIA_TYPE_SUBTITLE:
      {
        CDemuxStreamSubtitleFFmpeg* st = new CDemuxStreamSubtitleFFmpeg(this, pStream);
        m_streams[iId] = st;
        if(pStream->codec)
          st->identifier = pStream->codec->sub_id;

        if(m_dllAvFormat.av_metadata_get(pStream->metadata, "title", NULL, 0))
          st->m_description = m_dllAvFormat.av_metadata_get(pStream->metadata, "title", NULL, 0)->value;

        break;
      }
    case AVMEDIA_TYPE_ATTACHMENT:
      { //mkv attachments. Only bothering with fonts for now.
        if(pStream->codec->codec_id == CODEC_ID_TTF)
        {
          XFILE::CFile file;
          std::string fileName = "special://temp/";
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52,83,0)
          AVMetadataTag *nameTag = m_dllAvFormat.av_metadata_get(pStream->metadata, "filename", NULL, 0);
          if (!nameTag) {
            CLog::Log(LOGERROR, "%s: TTF attachment has no name", __FUNCTION__);
            break;
          }
          fileName += nameTag->value;
#else
          fileName += pStream->filename;
#endif
          if(file.OpenForWrite(fileName) && pStream->codec->extradata)
          {
            file.Write(pStream->codec->extradata, pStream->codec->extradata_size);
            file.Close();
          }
        }
        m_streams[iId] = new CDemuxStream();
        m_streams[iId]->type = STREAM_NONE;
        break;
      }
    default:
      {
        m_streams[iId] = new CDemuxStream();
        m_streams[iId]->type = STREAM_NONE;
        break;
      }
    }

    // generic stuff
    if (pStream->duration != (int64_t)AV_NOPTS_VALUE) m_streams[iId]->iDuration = (int)((pStream->duration / AV_TIME_BASE) & 0xFFFFFFFF);

    m_streams[iId]->codec = pStream->codec->codec_id;
    m_streams[iId]->codec_fourcc = pStream->codec->codec_tag;
    m_streams[iId]->iId = iId;
    m_streams[iId]->source = STREAM_SOURCE_DEMUX;
    m_streams[iId]->pPrivate = pStream;
    m_streams[iId]->flags = (CDemuxStream::EFlags)pStream->disposition;

#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52,83,0)
    // API added on: 2010-10-15
    // (Note that while the function was available earlier, the generic
    // metadata tags were not populated by default)
    AVMetadataTag *langTag = m_dllAvFormat.av_metadata_get(pStream->metadata, "language", NULL, 0);
    if (langTag)
      strncpy(m_streams[iId]->language, langTag->value, 3);
#else
    strcpy( m_streams[iId]->language, pStream->language );
#endif

    if( pStream->codec->extradata && pStream->codec->extradata_size > 0 )
    {
      m_streams[iId]->ExtraSize = pStream->codec->extradata_size;
      m_streams[iId]->ExtraData = new BYTE[pStream->codec->extradata_size];
      memcpy(m_streams[iId]->ExtraData, pStream->codec->extradata, pStream->codec->extradata_size);
    }

    //Sometimes CODEC_ID is not set correctly. In this case see if we can parse the 4cc
    if( CODEC_ID_NONE == m_streams[iId]->codec )
    {
      char fourcc[5];
      unsigned int cc = m_streams[iId]->codec_fourcc;
      fourcc[0] = cc & 0xff;
      fourcc[1] = (cc >> 8) & 0xff;
      fourcc[2] = (cc >> 16) & 0xff;
      fourcc[3] = (cc >> 24);
      fourcc[4] = 0;

      // For now, just handle ec-3
      if( 0 == strcmp( fourcc, "ec-3" ) )
      {
        m_streams[iId]->codec = CODEC_ID_EAC3;
      }
    }

    //FFMPEG has an error doesn't set type properly for DTS
    if( m_streams[iId]->codec == CODEC_ID_AC3 && (pStream->id >= 136 && pStream->id <= 143) )
      m_streams[iId]->codec = CODEC_ID_DTS;

    // *.dts files are wav files which ffmpeg pass as PCM
    CStdString extension;
    CUtil::GetExtension(m_pInput->GetFileName(), extension);
    if (extension.Equals(".dts", false) &&  pStream->codec->codec_type == AVMEDIA_TYPE_AUDIO)
    {
      m_streams[iId]->codec = CODEC_ID_DTS;
    }

#ifdef HAVE_LIBBLURAY
    if( m_pInput->IsStreamType(DVDSTREAM_TYPE_BLURAY) && pStream->codec->codec_type != AVMEDIA_TYPE_VIDEO)
    {
      static_cast<CDVDInputStreamBluray*>(m_pInput)->GetStreamInfo(pStream->id, m_streams[iId]->language);
    }
#endif
    if( m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD) )
    {
      if (m_streams[iId]->codec == CODEC_ID_PROBE)
      {
        // fix MPEG-1/MPEG-2 video stream probe returning CODEC_ID_PROBE for still frames.
        // ffmpeg issue 1871, regression from ffmpeg r22831.
        if ((pStream->id & 0xF0) == 0xE0)
        {
          m_streams[iId]->codec = CODEC_ID_MPEG2VIDEO;
          m_streams[iId]->codec_fourcc = MKTAG('M','P','2','V');
          m_streams[iId]->iPhysicalId = pStream->id & 0xF0;
          CLog::Log(LOGERROR, "%s - CODEC_ID_PROBE detected, forcing CODEC_ID_MPEG2VIDEO", __FUNCTION__);
        }
      }

      // this stuff is really only valid for dvd's.
      // this is so that the physicalid matches the 
      // id's reported from libdvdnav
      switch(m_streams[iId]->codec)
      {
        case CODEC_ID_AC3:
          m_streams[iId]->iPhysicalId = pStream->id - 128;
          break;
        case CODEC_ID_DTS:
          m_streams[iId]->iPhysicalId = pStream->id - 136;
          break;
        case CODEC_ID_MP2:
          m_streams[iId]->iPhysicalId = pStream->id - 448;
          break;
        case CODEC_ID_PCM_S16BE:
          m_streams[iId]->iPhysicalId = pStream->id - 160;
          break;
        case CODEC_ID_DVD_SUBTITLE:
          m_streams[iId]->iPhysicalId = pStream->id - 0x20;
          break;
        default:
          m_streams[iId]->iPhysicalId = pStream->id & 0x1f;
          break;
      }
    }
    else
      m_streams[iId]->iPhysicalId = pStream->id;
  }
}

std::string CDVDDemuxFFmpeg::GetFileName()
{
  if(m_pInput && m_pInput)
    return m_pInput->GetFileName(); 
  else 
    return ""; 
}

int CDVDDemuxFFmpeg::GetChapterCount()
{
  if(m_pInput && m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD))
    return ((CDVDInputStreamNavigator*)m_pInput)->GetChapterCount();
#ifdef HAVE_LIBBLURAY
  if(m_pInput && m_pInput->IsStreamType(DVDSTREAM_TYPE_BLURAY))
    return ((CDVDInputStreamBluray*)m_pInput)->GetChapterCount();
#endif
  if(m_pInput && m_pInput->IsStreamType(DVDSTREAM_TYPE_PLAYLIST))
   return ((CDVDInputStreamPlaylist*)m_pInput)->GetChapterCount();

  if(m_pFormatContext == NULL)
    return 0;
  #if (LIBAVFORMAT_VERSION_MAJOR == 52) && (LIBAVFORMAT_VERSION_MINOR >= 14)
  return m_pFormatContext->nb_chapters;
  #else
    return 0;
  #endif
}

int CDVDDemuxFFmpeg::GetChaptersInfo(std::vector<DemuxChapterInfo>& chapters)
{
#ifdef HAVE_LIBBLURAY
  if(m_pInput && m_pInput->IsStreamType(DVDSTREAM_TYPE_BLURAY))
  {
    // the demuxer doesn't have our chapter info; we pull it from the input stream
    CDVDInputStreamBluray* bluray = (CDVDInputStreamBluray*)m_pInput;
    int chapter_count = bluray->GetChapterCount();

    // libbluray chapters are 1 based.
    for( int i = 1; i <= chapter_count; i++ )
    {
      DemuxChapterInfo chapter;
      uint64_t start=0, dur=0;
      std::string name;

      bluray->GetChapterInfo(i, start, dur, name);
      chapter.id = i;
      chapter.title = name;
      chapter.startSecond = start / 1000;
      chapter.endSecond = (start + dur) / 1000;

      chapters.push_back(chapter);
    }

    return chapter_count;
  }
#endif
  if(m_pInput && m_pInput->IsStreamType(DVDSTREAM_TYPE_PLAYLIST))
  {
    CDVDInputStreamPlaylist* pl = (CDVDInputStreamPlaylist*)m_pInput;
    int chapter_count = pl->GetChapterCount();

    // playlist chapters are 1 based.
    for( int i = 1; i <= chapter_count; i++ )
    {
      DemuxChapterInfo chapter;
      uint64_t start=0, dur=0;
      std::string name;

      pl->GetChapterInfo(i, start, dur, name);
      chapter.id = i;
      chapter.title = name;
      chapter.startSecond = start / 1000;
      chapter.endSecond = (start + dur) / 1000;

      chapters.push_back(chapter);
    }

    return chapter_count;
  }  

  if (m_pFormatContext == NULL)
    return 0;

  for (unsigned int i = 0; i < m_pFormatContext->nb_chapters; i++)
  {
    AVChapter *avchapter = m_pFormatContext->chapters[i];

    DemuxChapterInfo chapter;
    chapter.id = i + 1;
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52,83,0)
    AVMetadataTag *titleTag = m_dllAvFormat.av_metadata_get(m_pFormatContext->chapters[i]->metadata,"title", NULL, 0);
    if (titleTag)
      chapter.title = titleTag->value;
#else
    chapter.title = avchapter->title;
#endif
    chapter.startSecond = (long int)(ConvertTimestamp(avchapter->start, avchapter->time_base.den, avchapter->time_base.num) / DVD_TIME_BASE);
    chapter.endSecond = (long int)(ConvertTimestamp(avchapter->end, avchapter->time_base.den, avchapter->time_base.num) / DVD_TIME_BASE);
    chapters.push_back(chapter);
  }

  return m_pFormatContext->nb_chapters;
}

int CDVDDemuxFFmpeg::GetChapter()
{
  if(m_pInput && m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD))
    return ((CDVDInputStreamNavigator*)m_pInput)->GetChapter();
    
#ifdef HAVE_LIBBLURAY
  if(m_pInput && m_pInput->IsStreamType(DVDSTREAM_TYPE_BLURAY))
    return ((CDVDInputStreamBluray*)m_pInput)->GetChapter();
#endif
  if(m_pInput && m_pInput->IsStreamType(DVDSTREAM_TYPE_PLAYLIST))
    return ((CDVDInputStreamPlaylist*)m_pInput)->GetChapter();
  if(m_pFormatContext == NULL 
  || m_iCurrentPts == DVD_NOPTS_VALUE)
    return 0;

  #if (LIBAVFORMAT_VERSION_MAJOR == 52) && (LIBAVFORMAT_VERSION_MINOR >= 14)
  for(unsigned i = 0; i < m_pFormatContext->nb_chapters; i++)
  {
    AVChapter *chapter = m_pFormatContext->chapters[i];
    if(m_iCurrentPts >= ConvertTimestamp(chapter->start, chapter->time_base.den, chapter->time_base.num)
    && m_iCurrentPts <  ConvertTimestamp(chapter->end,   chapter->time_base.den, chapter->time_base.num))
      return i + 1;
  }
  #endif
  return 0;
}

void CDVDDemuxFFmpeg::GetChapterName(std::string& strChapterName)
{
  if(m_pInput && m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD) )
    return ;
#ifdef HAVE_LIBBLURAY
  else if( m_pInput->IsStreamType(DVDSTREAM_TYPE_BLURAY) )
    return ((CDVDInputStreamBluray*)m_pInput)->GetChapterName(strChapterName);
#endif
  else if( m_pInput->IsStreamType(DVDSTREAM_TYPE_PLAYLIST) )
    return ((CDVDInputStreamPlaylist*)m_pInput)->GetChapterName(strChapterName);
  else 
  {
    #if (LIBAVFORMAT_VERSION_MAJOR == 52) && (LIBAVFORMAT_VERSION_MINOR >= 14)
    int chapterIdx = GetChapter();
    if(chapterIdx <= 0)
      return;
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52,83,0)
    // API added on: 2010-10-15
    // (Note that while the function was available earlier, the generic
    // metadata tags were not populated by default)
    AVMetadataTag *titleTag = m_dllAvFormat.av_metadata_get(m_pFormatContext->chapters[chapterIdx-1]->metadata,
                                                            "title", NULL, 0);
    if (titleTag)
      strChapterName = titleTag->value;
#else
    if(m_pFormatContext->chapters[chapterIdx-1]->title)
      strChapterName = m_pFormatContext->chapters[chapterIdx-1]->title;
#endif
    #endif
  }
}

bool CDVDDemuxFFmpeg::SeekChapter(int chapter, double* startpts)
{
  if(chapter < 1)
    chapter = 1;

  if(m_pInput && m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CLog::Log(LOGDEBUG, "%s - chapter seeking using navigator", __FUNCTION__);
    if(!((CDVDInputStreamNavigator*)m_pInput)->SeekChapter(chapter))
      return false;

    if(startpts)
      *startpts = DVD_NOPTS_VALUE;
    
    Flush();
    return true;
  }
#ifdef HAVE_LIBBLURAY
  if(m_pInput && m_pInput->IsStreamType(DVDSTREAM_TYPE_BLURAY))
  {
    CDVDInputStreamBluray* br = ((CDVDInputStreamBluray*)m_pInput);
    CLog::Log(LOGDEBUG, "%s - chapter seeking using navigator", __FUNCTION__);
    if( !br->SeekChapter(chapter) )
      return false;

    if(startpts)
    {
      // With bluray, we can't lie here. we need to give a decent startpts
      //*startpts = DVD_MSEC_TO_TIME(br->GetTime());
      *startpts = DVD_NOPTS_VALUE;
    }

    Flush();

    m_ioContext->buf_ptr = m_ioContext->buf_end;

    return true;
  }
#endif

  if(m_pInput && m_pInput->IsStreamType(DVDSTREAM_TYPE_PLAYLIST))
  {
    CDVDInputStreamPlaylist* pl = ((CDVDInputStreamPlaylist*)m_pInput);
    CLog::Log(LOGDEBUG, "%s - chapter seeking using navigator", __FUNCTION__);
    if( !pl->SeekChapter(chapter) )
      return false;

    if(startpts)
    {
      // With bluray, we can't lie here. we need to give a decent startpts
      //*startpts = DVD_MSEC_TO_TIME(br->GetTime());
      *startpts = DVD_NOPTS_VALUE;
    }

    Flush();

    m_ioContext->buf_ptr = m_ioContext->buf_end;

    return true;
  }

  if(m_pFormatContext == NULL)
    return false;

#if (LIBAVFORMAT_VERSION_MAJOR == 52) && (LIBAVFORMAT_VERSION_MINOR >= 14)
  if(chapter < 1 || chapter > (int)m_pFormatContext->nb_chapters)
    return false;

  AVChapter *ch = m_pFormatContext->chapters[chapter-1];
  double dts = ConvertTimestamp(ch->start, ch->time_base.den, ch->time_base.num);
  return SeekTime(DVD_TIME_TO_MSEC(dts), false, startpts);

#else
  return false;
#endif
}

void CDVDDemuxFFmpeg::GetStreamCodecName(int iStreamId, CStdString &strName)
{
  CDemuxStream *stream = GetStream(iStreamId);
  if (stream)
  {
    unsigned int in = stream->codec_fourcc;
    // audio codecs have 2 byte IDs that don't make much sense so only use FourCC for video
    if (stream->type == STREAM_VIDEO && in != 0)
    {
      char fourcc[5];
#if defined(__powerpc__)
      fourcc[0] = in & 0xff;
      fourcc[1] = (in >> 8) & 0xff;
      fourcc[2] = (in >> 16) & 0xff;
      fourcc[3] = (in >> 24) & 0xff;
#else
      memcpy(fourcc, &in, 4);
#endif
      fourcc[4] = 0;
      // fourccs have to be 4 characters
      if (strlen(fourcc) == 4)
      {
        strName = fourcc;
        strName.MakeLower();
        return;
      }
    }

    if (m_dllAvCodec.IsLoaded())
    {
      AVCodec *codec = m_dllAvCodec.avcodec_find_decoder(stream->codec);
      if (codec)
      {
        // Respect requirements for certification to have proper labeling
        if (strcmp(codec->name, "ac3") == 0)                   strName = "Dolby Digital";
        else if (strcmp(codec->name, "eac3") == 0)             strName = "Dolby Digital+";
        else if (strcmp(codec->name, "truehd") == 0)           strName = "Dolby TrueHD";
        else if (strcmp(codec->name, "dca (DTS)") == 0)        strName = "DTS";
        else if (strcmp(codec->name, "dca (DTS-HD MA)") == 0)  strName = "DTS-HD MA";
        else if (strcmp(codec->name, "dca (DTS-ES)") == 0)     strName = "DTS-ES";
        else if (strcmp(codec->name, "dca (DTS 96/24)") == 0)  strName = "DTS 96/24";
        else if (strcmp(codec->name, "dca (DTS-HD HRA)") == 0) strName = "DTS-HD HRA";
        else strName = codec->name;
      }
    }
  }
}

int CDVDDemuxFFmpeg::GetStartTime() 
{
  if (m_pFormatContext)
  {
    double t = ((double)m_pFormatContext->start_time / AV_TIME_BASE) * DVD_TIME_BASE;
    return DVD_TIME_TO_MSEC(t);
  }
  return 0;
}

void CDVDDemuxFFmpeg::AudioStreamSelected(int stream)
{
  m_currentAudioStream = stream;
  m_lastAudioDTS = 0;
}

#include <libavformat/avformat.h>
/* Taken from libavformat/utils.c */
void CDVDDemuxFFmpeg::flush_packet_queue(AVFormatContext *s)
{
    AVPacketList *pktl;

    for(;;)
    {
        pktl = s->packet_buffer;
        if (!pktl)
            break;
        s->packet_buffer = pktl->next;
        m_dllAvCodec.av_free_packet(&pktl->pkt);
        m_dllAvUtil.av_free(pktl);
    }
    while(s->raw_packet_buffer){
        pktl = s->raw_packet_buffer;
        s->raw_packet_buffer = pktl->next;
        m_dllAvCodec.av_free_packet(&pktl->pkt);
        m_dllAvUtil.av_free(pktl);
    }
    s->packet_buffer_end=
      s->raw_packet_buffer_end= NULL;
    s->raw_packet_buffer_remaining_size = RAW_PACKET_BUFFER_SIZE;
    
}


/* Taken from libavformat/utils.c */
void CDVDDemuxFFmpeg::av_read_frame_flush(AVFormatContext *s)
{
    AVStream *st;
    unsigned int i, j;

    flush_packet_queue(s);

    s->cur_st = NULL;

    /* for each stream, reset read state */
    for(i = 0; i < s->nb_streams; i++) {
        st = s->streams[i];

        if (st->parser) {
            m_dllAvCodec.av_parser_close(st->parser);
            st->parser = NULL;
            m_dllAvCodec.av_free_packet(&st->cur_pkt);
        }
        st->last_IP_pts = AV_NOPTS_VALUE;
        st->cur_dts = AV_NOPTS_VALUE; /* we set the current DTS to an unspecified origin */
        st->reference_dts = AV_NOPTS_VALUE;
        /* fail safe */
        st->cur_ptr = NULL;
        st->cur_len = 0;

        st->probe_packets = MAX_PROBE_PACKETS;

        for(j=0; j<MAX_REORDER_DELAY+1; j++)
            st->pts_buffer[j]= AV_NOPTS_VALUE;
    }
}

AVMetadataTag* CDVDDemuxFFmpeg::AvGetMetaData(AVMetadata *p1, const char *p2, const AVMetadataTag *p3, int p4)
{
  if (!m_dllAvFormat.IsLoaded())
    return NULL;

  return m_dllAvFormat.av_metadata_get(p1, p2, p3, p4);
}
