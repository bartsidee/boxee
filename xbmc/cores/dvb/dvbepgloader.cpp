/* This code was taken from ts.c, from the vlc code */

#include "dvbepgloader.h"

#if defined(HAS_DVB) && defined(_LINUX) && !defined(__APPLE__)

#include "StdString.h"

#include <fcntl.h>
#include <dvbpsi.h>
#include <demux.h>
#include <descriptor.h>
#include <tables/eit.h>
#include <descriptors/dr.h>
#include <psi.h>
#include <iconv.h>
#include <pthread.h>
#include "Util.h"
#include <boost/foreach.hpp>
#include "SingleLock.h"

#ifdef __GNUC__
#   define likely(p)   __builtin_expect(!!(p), 1)
#   define unlikely(p) __builtin_expect(!!(p), 0)
#else
#   define likely(p)   (!!(p))
#   define unlikely(p) (!!(p))
#endif

typedef struct EitData
{
  void* p_cb_data;
  dvbpsi_eit_t *p_eit;
  bool b_current_following;
} EitData;

static inline unsigned clz (unsigned x)
{
#ifdef __GNUC_
    return __builtin_clz (x);
#else
    unsigned i = sizeof (x) * 8;

    while (x)
    {
        x = x >> 1;
        i--;
    }
    return i;
#endif
}

#define clz8( x ) (clz(x) - ((sizeof(unsigned) - sizeof (uint8_t)) * 8))

/**
 * Converts the first character from a UTF-8 sequence into a code point.
 *
 * @param str an UTF-8 bytes sequence
 * @return 0 if str points to an empty string, i.e. the first character is NUL;
 * number of bytes that the first character occupies (from 1 to 4) otherwise;
 * -1 if the byte sequence was not a valid UTF-8 sequence.
 */
static size_t vlc_towc (const char *str, uint32_t *restrict pwc)
{
    uint8_t *ptr = (uint8_t *)str, c;
    uint32_t cp = 0;

    assert (str != NULL);

    c = *ptr;
    if (unlikely(c > 0xF4))
        return -1;

    int charlen = clz8 (c ^ 0xFF);
    switch (charlen)
    {
        case 0: // 7-bit ASCII character -> short cut
            *pwc = c;
            return c != '\0';

        case 1: // continuation byte -> error
            return -1;

        case 2:
            if (unlikely(c < 0xC2)) // ASCII overlong
                return -1;
            cp = (c & 0x1F) << 6;
            break;

        case 3:
            cp = (c & 0x0F) << 12;
            break;

        case 4:
            cp = (c & 0x07) << 16;
            break;

        default:
            assert (0);
    }

    /* Unrolled continuation bytes decoding */
    switch (charlen)
    {
        case 4:
            c = *++ptr;
            if (unlikely((c >> 6) != 2)) // not a continuation byte
                return -1;
            cp |= (c & 0x3f) << 12;

            if (unlikely(cp >= 0x110000)) // beyond Unicode range
                return -1;
            /* fall through */
        case 3:
            c = *++ptr;
            if (unlikely((c >> 6) != 2)) // not a continuation byte
                return -1;
            cp |= (c & 0x3f) << 6;

            if (unlikely(cp >= 0xD800 && cp < 0xC000)) // UTF-16 surrogate
                return -1;
            if (unlikely(cp < (1u << (5 * charlen - 4)))) // non-ASCII overlong
                return -1;
            /* fall through */
        case 2:
            c = *++ptr;
            if (unlikely((c >> 6) != 2)) // not a continuation byte
                return -1;
            cp |= (c & 0x3f);
            break;
    }

    *pwc = cp;
    return charlen;
}

/**
 * Replaces invalid/overlong UTF-8 sequences with question marks.
 * Note that it is not possible to convert from Latin-1 to UTF-8 on the fly,
 * so we don't try that, even though it would be less disruptive.
 *
 * @return str if it was valid UTF-8, NULL if not.
 */
static char *EnsureUTF8( char *str )
{
    char *ret = str;
    size_t n;
    uint32_t cp;

    while ((n = vlc_towc (str, &cp)) != 0)
        if (likely(n != (size_t)-1))
            str += n;
        else
        {
            *str++ = '?';
            ret = NULL;
        }
    return ret;
}

static char *EITConvertToUTF8(const unsigned char *psz_instring,
    size_t i_length, bool b_broken)
{
  const char *psz_encoding;
  char *psz_outstring;
  char psz_encbuf[sizeof("ISO_8859-123")];
  size_t i_in, i_out, offset = 1;
  iconv_t iconv_handle;

  if (i_length < 1)
    return NULL;
  if (psz_instring[0] >= 0x20)
  {
    /* According to ETSI EN 300 468 Annex A, this should be ISO6937,
     * but some broadcasters use different charset... */
    if (b_broken)
      psz_encoding = "ISO_8859-1";
    else
      psz_encoding = "ISO_6937";

    offset = 0;
  }
  else
    switch (psz_instring[0])
    {
    case 0x01:
      psz_encoding = "ISO_8859-5";
      break;
    case 0x02:
      psz_encoding = "ISO_8859-6";
      break;
    case 0x03:
      psz_encoding = "ISO_8859-7";
      break;
    case 0x04:
      psz_encoding = "ISO_8859-8";
      break;
    case 0x05:
      psz_encoding = "ISO_8859-9";
      break;
    case 0x06:
      psz_encoding = "ISO_8859-10";
      break;
    case 0x07:
      psz_encoding = "ISO_8859-11";
      break;
    case 0x08:
      psz_encoding = "ISO_8859-12";
      break;
    case 0x09:
      psz_encoding = "ISO_8859-13";
      break;
    case 0x0a:
      psz_encoding = "ISO_8859-14";
      break;
    case 0x0b:
      psz_encoding = "ISO_8859-15";
      break;
    case 0x10:
#warning Is Latin-10 (psz_instring[2] == 16) really illegal?
      if (i_length < 3 || psz_instring[1] != 0x00 || psz_instring[2] > 15
          || psz_instring[2] == 0)
      {
        psz_encoding = "UTF-8";
        offset = 0;
      }
      else
      {
        sprintf(psz_encbuf, "ISO_8859-%u", psz_instring[2]);
        psz_encoding = psz_encbuf;
        offset = 3;
      }
      break;
    case 0x11:
#warning Is there a BOM or do we use a fixed endianess?
      psz_encoding = "UTF-16";
      break;
    case 0x12:
      psz_encoding = "KSC5601-1987";
      break;
    case 0x13:
      psz_encoding = "GB2312"; /* GB-2312-1980 */
      break;
    case 0x14:
      psz_encoding = "BIG-5";
      break;
    case 0x15:
      psz_encoding = "UTF-8";
      break;
    default:
      /* invalid */
      psz_encoding = "UTF-8";
      offset = 0;
    }

  i_in = i_length - offset;
  i_out = i_in * 6 + 1;

  psz_outstring = (char*) malloc(i_out);
  if (!psz_outstring)
  {
    return NULL;
  }

  iconv_handle = iconv_open("UTF-8", psz_encoding);
  if (iconv_handle == (iconv_t)(-1))
  {
    /* Invalid character set (e.g. ISO_8859-12) */
    memcpy(psz_outstring, &psz_instring[offset], i_in);
    psz_outstring[i_in] = '\0';
    EnsureUTF8( psz_outstring );
  }
  else
  {
    const char *psz_in = (const char *) &psz_instring[offset];
    char *psz_out = psz_outstring;

    while (iconv(iconv_handle, (char**) &psz_in, &i_in, &psz_out, &i_out)
        == (size_t) (-1))
    {
      /* skip naughty byte. This may fail terribly for multibyte stuff,
       * but what can we do anyway? */
      psz_in++;
      i_in--;
      iconv(iconv_handle, NULL, NULL, NULL, NULL); /* reset */
    }
    iconv_close(iconv_handle);

    *psz_out = '\0';

    /* Convert EIT-coded CR/LFs */
    unsigned char *pbuf = (unsigned char *) psz_outstring;
    for (; pbuf < (unsigned char *) psz_out; pbuf++)
    {
      if (pbuf[0] == 0xc2 && pbuf[1] == 0x8a)
      {
        pbuf[0] = ' ';
        pbuf[1] = '\n';
      }
    }

  }
  return psz_outstring;
}

/* i_year: year - 1900  i_month: 0-11  i_mday: 1-31 i_hour: 0-23 i_minute: 0-59 i_second: 0-59 */
static int64_t vlc_timegm(int i_year, int i_month, int i_mday, int i_hour,
    int i_minute, int i_second)
{
  static const int pn_day[12 + 1] =
  { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};int64_t i_day;
  int i;

  if( i_year < 70 ||
      i_month < 0 || i_month > 11 || i_mday < 1 || i_mday > 31 ||
      i_hour < 0 || i_hour > 23 || i_minute < 0 || i_minute > 59 || i_second < 0 || i_second > 59 )
  return -1;

  /* Count the number of days */
  i_day = 365 * (i_year-70) + pn_day[i_month] + i_mday - 1;
#define LEAP(y) ( ((y)%4) == 0 && (((y)%100) != 0 || ((y)%400) == 0) ? 1 : 0)
  for( i = 70; i < i_year; i++ )
  i_day += LEAP(1900+i);
  if( i_month > 1 )
  i_day += LEAP(1900+i_year);
#undef LEAP
  /**/
  return ((24*i_day + i_hour)*60 + i_minute)*60 + i_second;
}

#define CVT_FROM_BCD(v) ((((v) >> 4)&0xf)*10 + ((v)&0xf))

static void EITDecodeMjd(int i_mjd, int *p_y, int *p_m, int *p_d)
{
  const int yp = (int) (((double) i_mjd - 15078.2) / 365.25 );
  const int mp = (int) (((double) i_mjd - 14956.1 - (int) (yp * 365.25))
      / 30.6001 );
  const int c = (mp == 14 || mp == 15 ) ? 1 : 0;

  *p_y = 1900 + yp + c * 1;
  *p_m = mp - 1 - c * 12;
  *p_d = i_mjd - 14956 - (int) (yp * 365.25) - (int) (mp * 30.6001);
}

static int64_t EITConvertStartTime(uint64_t i_date)
{
  const int i_mjd = i_date >> 24;
  const int i_hour = CVT_FROM_BCD(i_date >> 16);
  const int i_minute = CVT_FROM_BCD(i_date >> 8);
  const int i_second = CVT_FROM_BCD(i_date );
  int i_year;
  int i_month;
  int i_day;

  /* if all 40 bits are 1, the start is unknown */
  if (i_date == uint64_t(0xffffffffffLL))
    return -1;

  EITDecodeMjd(i_mjd, &i_year, &i_month, &i_day);
  return vlc_timegm(i_year - 1900, i_month - 1, i_day, i_hour, i_minute,
      i_second);
}

static int EITConvertDuration(uint32_t i_duration)
{
  return CVT_FROM_BCD(i_duration >> 16) * 3600
      + CVT_FROM_BCD(i_duration >> 8 ) * 60 + CVT_FROM_BCD(i_duration );
}


void EITCallBack(void* p_cb_data, dvbpsi_eit_t *p_eit, bool b_current_following)
{
  dvbpsi_eit_event_t *p_evt;

  if (!p_eit->b_current_next)
  {
    dvbpsi_DeleteEIT( p_eit);
    return;
  }

#if 0
  printf("new EIT service_id=%d version=%d current_next=%d "
      "ts_id=%d network_id=%d segment_last_section_number=%d "
      "last_table_id=%d\n",
      p_eit->i_service_id, p_eit->i_version, p_eit->b_current_next,
      p_eit->i_ts_id, p_eit->i_network_id,
      p_eit->i_segment_last_section_number, p_eit->i_last_table_id );
#endif

  DvbEpgInfo epg;
  epg.networkId = p_eit->i_network_id;
  epg.serviceId = p_eit->i_service_id;
  epg.tsId = p_eit->i_ts_id;

  for (p_evt = p_eit->p_first_event; p_evt; p_evt = p_evt->p_next)
  {
    dvbpsi_descriptor_t *p_dr;
    char *psz_name = NULL;
    char *psz_text = NULL;
    char *psz_extra = strdup("");
    int64_t i_start;
    int i_duration;
    uint16_t i_event_id;
    CStdString strRating;

    i_start = EITConvertStartTime(p_evt->i_start_time);
    i_duration = EITConvertDuration(p_evt->i_duration);
    i_event_id = p_evt->i_event_id;

#if 0
    printf("  * event id=%d start_time:%d duration=%d "
        "running=%d free_ca=%d\n",
        p_evt->i_event_id, (int)i_start, (int)i_duration,
        p_evt->i_running_status, p_evt->b_free_ca );
#endif

    for (p_dr = p_evt->p_first_descriptor; p_dr; p_dr = p_dr->p_next)
    {
      if (p_dr->i_tag == 0x4d)
      {
        dvbpsi_short_event_dr_t *pE = dvbpsi_DecodeShortEventDr(p_dr);

        /* Only take first description, as we don't handle language-info
         for epg atm*/
        if (pE && psz_name == NULL)
        {
          psz_name = EITConvertToUTF8(pE->i_event_name, pE->i_event_name_length,
              true /*p_sys->b_broken_charset */);
          psz_text = EITConvertToUTF8(pE->i_text, pE->i_text_length,
              true /*p_sys->b_broken_charset */);
#if 0
          printf("    - short event lang=%3.3s '%s' : '%s'\n",
              pE->i_iso_639_code, psz_name, psz_text );
#endif
        }
      }
      else if (p_dr->i_tag == 0x4e)
      {
        dvbpsi_extended_event_dr_t *pE = dvbpsi_DecodeExtendedEventDr(p_dr);
        if (pE)
        {
#if 0
          printf("    - extended event lang=%3.3s [%d/%d]\n",
              pE->i_iso_639_code,
              pE->i_descriptor_number, pE->i_last_descriptor_number );
#endif

          if (pE->i_text_length > 0)
          {
            char *psz_text = EITConvertToUTF8(pE->i_text, pE->i_text_length,
                true /*p_sys->b_broken_charset*/);
            if (psz_text)
            {
#if 0
              printf("       - text='%s'\n", psz_text );
#endif
              psz_extra = (char*) realloc(psz_extra,
                  strlen(psz_extra) + strlen(psz_text) + 1);
              strcat(psz_extra, psz_text);
              free(psz_text);
            }
          }

          for (int i = 0; i < pE->i_entry_count; i++)
          {
            char *psz_dsc = EITConvertToUTF8(pE->i_item_description[i],
                pE->i_item_description_length[i],
                true /* p_sys->b_broken_charset */);
            char *psz_itm = EITConvertToUTF8(pE->i_item[i],
                pE->i_item_length[i], true /* p_sys->b_broken_charset */);

            if (psz_dsc && psz_itm)
            {
#if 0
              printf("       - desc='%s' item='%s'\n", psz_dsc, psz_itm );
#endif

#if 0
              psz_extra = xrealloc( psz_extra,
                  strlen(psz_extra) + strlen(psz_dsc) +
                  strlen(psz_itm) + 3 + 1 );
              strcat( psz_extra, "(" );
              strcat( psz_extra, psz_dsc );
              strcat( psz_extra, " " );
              strcat( psz_extra, psz_itm );
              strcat( psz_extra, ")" );
#endif
            }
            free(psz_dsc);
            free(psz_itm);
          }
        }
      }
      else if (p_dr->i_tag == 0x55) // parental_rating_descriptor
      {
        dvbpsi_parental_rating_dr_t *pD = dvbpsi_DecodeParentalRatingDr(p_dr);

        if(pD)
        {
          for(int i = 0; i< pD->i_ratings_number; i++)
          {
            uint8_t i_rating = pD->p_parental_rating[i].i_rating;
#if 0
            printf("    - parental rating i=%d country_code %d rating %d\n", i,
                pD->p_parental_rating[i].i_country_code,
                pD->p_parental_rating[i].i_rating);
#endif
            if(i_rating >= 0x1 && i_rating <= 0xF)
            {
              strRating.Format("TV-%d", i_rating + 3);
              break; // FIXME: only first country for now
            }
          }
        }
      }
      else
      {
#if 0
        printf("    - tag=0x%x(%d)\n", p_dr->i_tag, p_dr->i_tag );
#endif
      }
    }

    /* */
    DvbEpgEvent event;

    if (i_start > 0)
    {
      event.id = i_event_id;
      event.duration = i_duration;
      event.startTime = i_start;
      event.name = psz_name;
      event.text = psz_text;
      event.rating = strRating;
      if (psz_extra)
        event.extra = psz_extra;

      epg.events.push_back(event);
    }

    /* Update "now playing" field */
    if (p_evt->i_running_status == 0x04 && i_start > 0)
    {
      epg.current = event;
    }

    DvbEpgLoader* epgLoader = (DvbEpgLoader*) p_cb_data;
    epgLoader->OnEpgNewInfo(epg);

    free(psz_name);
    free(psz_text);

    free(psz_extra);
  }


  dvbpsi_DeleteEIT(p_eit);
}

static void EITCallBackCurrentFollowing(void *p_cb_data, dvbpsi_eit_t *p_eit)
{
  DvbEpgLoader* dvnEpgLoader = (DvbEpgLoader*)p_cb_data;

  dvnEpgLoader->OnNewEitData(p_cb_data, p_eit, true);
}

static void EITCallBackSchedule(void *p_cb_data, dvbpsi_eit_t *p_eit)
{
  DvbEpgLoader* dvnEpgLoader = (DvbEpgLoader*)p_cb_data;

  dvnEpgLoader->OnNewEitData(p_cb_data, p_eit, false);
}

void PSINewTableCallBack(void *p_cb_data, dvbpsi_handle h_dvbpsi,
    uint8_t i_table_id, uint16_t i_extension)
{
  if (i_table_id == 0x4e || /* Current/Following */
      (i_table_id >= 0x50 && i_table_id <= 0x5f)) /* Schedule */
  {
#if 0
    printf("PSINewTableCallBack: table 0x%x(%d) ext=0x%x(%d)\n",
        i_table_id, i_table_id, i_extension, i_extension );
#endif

    dvbpsi_eit_callback cb =
        i_table_id == 0x4e ? (dvbpsi_eit_callback) EITCallBackCurrentFollowing :
            (dvbpsi_eit_callback) EITCallBackSchedule;

    dvbpsi_AttachEIT(h_dvbpsi, i_table_id, i_extension, cb, p_cb_data);
  }
}

DvbEpgLoader::DvbEpgLoader()
{
  pthread_mutex_init(&m_listenersLock, NULL);
  m_dvbpsi = dvbpsi_AttachDemux((dvbpsi_demux_new_cb_t) PSINewTableCallBack, this);

  pthread_mutex_init(&m_condLock, NULL);
  pthread_cond_init(&m_newEitData, NULL);

  Create();
}

DvbEpgLoader::~DvbEpgLoader()
{
  pthread_mutex_destroy(&m_listenersLock);
  dvbpsi_DetachDemux(m_dvbpsi);

  m_bStop = true;
  pthread_cond_broadcast(&m_newEitData);

  StopThread(true);

  pthread_mutex_destroy(&m_condLock);
  pthread_cond_destroy(&m_newEitData);
}

void DvbEpgLoader::OnNewEitData(void *p_cb_data, void *p_eit, bool b_current_following)
{
  EitData* data = new EitData;

  data->p_cb_data = p_cb_data;
  data->p_eit = (dvbpsi_eit_t*)p_eit;
  data->b_current_following = b_current_following;

  {
    CSingleLock lock(m_lock);
    m_eitData.push_back(data);
  }

  pthread_cond_broadcast(&m_newEitData);
}

bool DvbEpgLoader::ProcessPacket(uint8_t* data)
{
  uint16_t i_pid = ((uint16_t) (data[1] & 0x1f) << 8) + data[2];
  if (i_pid == 0x12)
  {
    dvbpsi_PushPacket(m_dvbpsi, data);
    return true;
  }

  return false;
}

void DvbEpgLoader::AddListener(IDvbEpgListener *alistener)
{
  pthread_mutex_lock(&m_listenersLock);
  bool bExists = false;

  foreach (IDvbEpgListener* listener, m_listeners)
  {
    if (alistener == listener)
    {
      bExists = true;
      break;
    }
  }

  if (!bExists)
    m_listeners.push_back(alistener);

  pthread_mutex_unlock(&m_listenersLock);
}

void DvbEpgLoader::RemoveListener(IDvbEpgListener *listener)
{
  pthread_mutex_lock(&m_listenersLock);
  m_listeners.remove(listener);
  pthread_mutex_unlock(&m_listenersLock);
}

void DvbEpgLoader::OnEpgNewInfo(const DvbEpgInfo& info)
{
  pthread_mutex_lock(&m_listenersLock);
  std::list<IDvbEpgListener *> tempListeners = m_listeners;
  pthread_mutex_unlock(&m_listenersLock);

  foreach (IDvbEpgListener* listener, tempListeners)
  {
    listener->OnEpgNewInfo(info);
  }
}

void DvbEpgLoader::Process()
{
  while(!m_bStop)
  {
      pthread_cond_wait(&m_newEitData, &m_condLock);

      while(m_eitData.size() > 0)
      {
        EitData* data = NULL;

        {
          CSingleLock lock(m_lock);
          data = m_eitData.front();
          m_eitData.pop_front();
        }

        if(data)
        {
          EITCallBack(data->p_cb_data, data->p_eit, data->b_current_following);
          delete data;
        }
      }
  }
}

#elif defined(__APPLE__) && defined(HAS_DVB)

DvbEpgLoader::DvbEpgLoader()
{
}

DvbEpgLoader::~DvbEpgLoader()
{
}

#endif
