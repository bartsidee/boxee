/*
 *      Copyright (C) 2005-2009 Team XBMC
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
#ifdef HAVE_LIBBLURAY

#include "DVDInputStreamBluray.h"
#include "LocalizeStrings.h"
#include "DynamicDll.h"
#include "Util.h"
#include "utils/log.h"
#include "FileSystem/File.h"
#include "FileSystem/Directory.h"
#include "IDVDPlayer.h"

#define LIBBLURAY_BYTESEEK 0

#ifndef WIN32
#define DLL_PATH_LIBBLURAY "libbluray.so"
#endif

extern "C"
{
// libbluray
#include <lib/libbluray/src/libbluray/bluray.h>
#include <lib/libbluray/src/file/filesystem.h>
#include <lib/libbluray/src/util/log_control.h>
}

//#define __USE_BR_NEWVER

class DllLibblurayInterface
{
public:
  virtual ~DllLibblurayInterface() {};
  virtual uint32_t bd_get_titles(BLURAY *bd, uint8_t flags, uint32_t min_title_length)=0;
  virtual BLURAY_TITLE_INFO* bd_get_title_info(BLURAY *bd, uint32_t title_idx, unsigned angle)=0;
  virtual BLURAY_TITLE_INFO* bd_get_playlist_info(BLURAY *bd, uint32_t playlist, unsigned angle)=0;
  virtual void bd_free_title_info(BLURAY_TITLE_INFO *title_info)=0;
  virtual BLURAY *bd_open(const char* device_path, const char* keyfile_path)=0;
  virtual void bd_close(BLURAY *bd)=0;
  virtual int64_t bd_seek(BLURAY *bd, uint64_t pos)=0;
  virtual int64_t bd_seek_time(BLURAY *bd, uint64_t tick)=0;
  virtual int bd_read(BLURAY *bd, unsigned char *buf, int len)=0;
  virtual int64_t bd_seek_chapter(BLURAY *bd, unsigned chapter)=0;
  virtual int64_t bd_chapter_pos(BLURAY *bd, unsigned chapter)=0;
  virtual uint32_t bd_get_current_chapter(BLURAY *bd)=0;
  virtual int64_t bd_seek_mark(BLURAY *bd, unsigned mark)=0;
  virtual int bd_select_playlist(BLURAY *bd, uint32_t playlist)=0;
  virtual int bd_select_title(BLURAY *bd, uint32_t title)=0;
  virtual int bd_select_angle(BLURAY *bd, unsigned angle)=0;
  virtual void bd_seamless_angle_change(BLURAY *bd, unsigned angle)=0;
  virtual uint64_t bd_get_title_size(BLURAY *bd)=0;
  virtual uint32_t bd_get_current_title(BLURAY *bd)=0;
  virtual unsigned bd_get_current_angle(BLURAY *bd)=0;
  virtual uint64_t bd_tell(BLURAY *bd)=0;
  virtual uint64_t bd_tell_time(BLURAY *bd)=0;
  virtual BD_FILE_OPEN bd_register_file(BD_FILE_OPEN p)=0;
  virtual BD_DIR_OPEN bd_register_dir(BD_DIR_OPEN p)=0;
  virtual int bd_set_player_setting(BLURAY *bd, uint32_t idx, uint32_t value)=0;
  virtual int bd_set_player_setting_str(BLURAY *bd, uint32_t idx, const char *s)=0;
#ifdef __USE_BR_NEWVER
  virtual int bd_play(BLURAY *bd)=0;
  virtual int bd_read_ext(BLURAY *bd, unsigned char *buf, int len, BD_EVENT *event)=0;
  virtual int bd_get_event(BLURAY *bd, BD_EVENT *event)=0;
  virtual int bd_play_title(BLURAY *bd, unsigned title)=0;
  virtual int bd_menu_call(BLURAY *bd)=0;
  virtual void bd_register_overlay_proc(BLURAY *bd, void *handle, bd_overlay_proc_f func)=0;
  virtual void bd_user_input(BLURAY *bd, int64_t pts, uint32_t key)=0;
  virtual void bd_set_debug_handler(BD_LOG_FUNC func)=0;
  virtual void bd_set_debug_mask(uint32_t mask)=0;
  virtual uint32_t bd_get_debug_mask(void)=0;
#endif
  virtual uint64_t bd_get_title_duration(BLURAY *bd, uint32_t title_idx)=0;
};

class DllLibbluray : public DllDynamic, DllLibblurayInterface
{
  DECLARE_DLL_WRAPPER(DllLibbluray, DLL_PATH_LIBBLURAY)

  DEFINE_METHOD3(uint32_t,            bd_get_titles,          (BLURAY *p1, uint8_t p2, uint32_t p3))
  DEFINE_METHOD3(BLURAY_TITLE_INFO*,  bd_get_title_info,      (BLURAY *p1, uint32_t p2, uint32_t p3))
  DEFINE_METHOD3(BLURAY_TITLE_INFO*,  bd_get_playlist_info,   (BLURAY *p1, uint32_t p2,  uint32_t p3))
  DEFINE_METHOD1(void,                bd_free_title_info,     (BLURAY_TITLE_INFO *p1))
  DEFINE_METHOD2(BLURAY*,             bd_open,                (const char* p1, const char* p2))
  DEFINE_METHOD1(void,                bd_close,               (BLURAY *p1))
  DEFINE_METHOD2(int64_t,             bd_seek,                (BLURAY *p1, uint64_t p2))
  DEFINE_METHOD2(int64_t,             bd_seek_time,           (BLURAY *p1, uint64_t p2))
  DEFINE_METHOD3(int,                 bd_read,                (BLURAY *p1, unsigned char *p2, int p3))
  DEFINE_METHOD2(int64_t,             bd_seek_chapter,        (BLURAY *p1, unsigned p2))
  DEFINE_METHOD2(int64_t,             bd_chapter_pos,         (BLURAY *p1, unsigned p2))
  DEFINE_METHOD1(uint32_t,            bd_get_current_chapter, (BLURAY *p1))
  DEFINE_METHOD2(int64_t,             bd_seek_mark,           (BLURAY *p1, unsigned p2))
  DEFINE_METHOD2(int,                 bd_select_playlist,     (BLURAY *p1, uint32_t p2))
  DEFINE_METHOD2(int,                 bd_select_title,        (BLURAY *p1, uint32_t p2))
  DEFINE_METHOD2(int,                 bd_select_angle,        (BLURAY *p1, unsigned p2))
  DEFINE_METHOD2(void,                bd_seamless_angle_change,(BLURAY *p1, unsigned p2))
  DEFINE_METHOD1(uint64_t,            bd_get_title_size,      (BLURAY *p1))
  DEFINE_METHOD1(uint32_t,            bd_get_current_title,   (BLURAY *p1))
  DEFINE_METHOD1(unsigned,            bd_get_current_angle,   (BLURAY *p1))
  DEFINE_METHOD1(uint64_t,            bd_tell,                (BLURAY *p1))
  DEFINE_METHOD1(uint64_t,            bd_tell_time,           (BLURAY *p1))
  DEFINE_METHOD1(BD_FILE_OPEN,        bd_register_file,       (BD_FILE_OPEN p1))
  DEFINE_METHOD1(BD_DIR_OPEN,         bd_register_dir,        (BD_DIR_OPEN p1))
  DEFINE_METHOD3(int,                 bd_set_player_setting,  (BLURAY *p1, uint32_t p2, uint32_t p3))
  DEFINE_METHOD3(int,                 bd_set_player_setting_str, (BLURAY *p1, uint32_t p2, const char *p3))
#ifdef __USE_BR_NEWVER
  DEFINE_METHOD1(int,                 bd_play,                (BLURAY *p1))
  DEFINE_METHOD4(int,                 bd_read_ext,            (BLURAY *p1, unsigned char *p2, int p3, BD_EVENT *p4))
  DEFINE_METHOD2(int,                 bd_get_event,           (BLURAY *p1, BD_EVENT *p2))
  DEFINE_METHOD2(int,                 bd_play_title,          (BLURAY *p1, unsigned p2))
  DEFINE_METHOD1(int,                 bd_menu_call,           (BLURAY *p1))
  DEFINE_METHOD3(void,                bd_register_overlay_proc,(BLURAY *p1, void *p2, bd_overlay_proc_f p3));
  DEFINE_METHOD3(void,                bd_user_input,          (BLURAY *p1, int64_t p2, uint32_t p3));
  DEFINE_METHOD1(void,                bd_set_debug_handler,   (BD_LOG_FUNC p1));
  DEFINE_METHOD1(void,                bd_set_debug_mask,      (uint32_t p1));
  DEFINE_METHOD0(uint32_t,            bd_get_debug_mask);
#endif
  DEFINE_METHOD2(uint64_t,            bd_get_title_duration,  (BLURAY *p1, uint32_t p2));

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(bd_get_titles,        bd_get_titles)
    RESOLVE_METHOD_RENAME(bd_get_title_info,    bd_get_title_info)
    RESOLVE_METHOD_RENAME(bd_get_playlist_info, bd_get_playlist_info)
    RESOLVE_METHOD_RENAME(bd_free_title_info,   bd_free_title_info)
    RESOLVE_METHOD_RENAME(bd_open,              bd_open)
    RESOLVE_METHOD_RENAME(bd_close,             bd_close)
    RESOLVE_METHOD_RENAME(bd_seek,              bd_seek)
    RESOLVE_METHOD_RENAME(bd_seek_time,         bd_seek_time)
    RESOLVE_METHOD_RENAME(bd_read,              bd_read)
    RESOLVE_METHOD_RENAME(bd_seek_chapter,      bd_seek_chapter)
    RESOLVE_METHOD_RENAME(bd_chapter_pos,       bd_chapter_pos)
    RESOLVE_METHOD_RENAME(bd_get_current_chapter, bd_get_current_chapter)
    RESOLVE_METHOD_RENAME(bd_seek_mark,         bd_seek_mark)
    RESOLVE_METHOD_RENAME(bd_select_playlist,   bd_select_playlist)
    RESOLVE_METHOD_RENAME(bd_select_title,      bd_select_title)
    RESOLVE_METHOD_RENAME(bd_select_angle,      bd_select_angle)
    RESOLVE_METHOD_RENAME(bd_seamless_angle_change, bd_seamless_angle_change)
    RESOLVE_METHOD_RENAME(bd_get_title_size,    bd_get_title_size)
    RESOLVE_METHOD_RENAME(bd_get_current_title, bd_get_current_title)
    RESOLVE_METHOD_RENAME(bd_get_current_angle, bd_get_current_angle)
    RESOLVE_METHOD_RENAME(bd_tell,              bd_tell)
    RESOLVE_METHOD_RENAME(bd_tell_time,         bd_tell_time)
    RESOLVE_METHOD_RENAME(bd_register_file,     bd_register_file)
    RESOLVE_METHOD_RENAME(bd_register_dir,      bd_register_dir)
    RESOLVE_METHOD_RENAME(bd_set_player_setting,bd_set_player_setting)
    RESOLVE_METHOD_RENAME(bd_set_player_setting_str, bd_set_player_setting_str)
#ifdef __USE_BR_NEWVER
    RESOLVE_METHOD_RENAME(bd_play,                bd_play)
    RESOLVE_METHOD_RENAME(bd_read_ext,            bd_read_ext)
    RESOLVE_METHOD_RENAME(bd_get_event,           bd_get_event)
    RESOLVE_METHOD_RENAME(bd_play_title,          bd_play_title)
    RESOLVE_METHOD_RENAME(bd_menu_call,           bd_menu_call)
    RESOLVE_METHOD_RENAME(bd_register_overlay_proc, bd_register_overlay_proc)
    RESOLVE_METHOD_RENAME(bd_user_input,          bd_user_input)
    RESOLVE_METHOD_RENAME(bd_set_debug_handler,   bd_set_debug_handler)
    RESOLVE_METHOD_RENAME(bd_set_debug_mask,      bd_set_debug_mask)
    RESOLVE_METHOD_RENAME(bd_get_debug_mask,      bd_get_debug_mask)
#endif 
    RESOLVE_METHOD_RENAME(bd_get_title_duration, bd_get_title_duration)
  END_METHOD_RESOLVE()
};


using namespace std;
using namespace XFILE;

#ifdef __USE_BR_NEWVER
static void __bdlog(const char* str)
{
  if( str )
    printf("**** BR: %s", str);
}
#endif 

static bool is_udf_iso_path(const char* filename)
{
  bool bResult = false;

  const char* ptr = strcasestr(filename, ".iso");
  if(ptr)
  {
    ptr += strlen(".iso");
    if(*ptr == '/' && strlen(++ptr) > 0)
    {
      bResult = true;
    }
  }
  return bResult;
}

static void file_close(BD_FILE_H *file)
{
  if (file)
  {
    delete static_cast<CFile*>(file->internal);
    delete file;

    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Closed file (%p)\n", file);
  }
}

static int64_t file_seek(BD_FILE_H *file, int64_t offset, int32_t origin)
{
  return static_cast<CFile*>(file->internal)->Seek(offset, origin);
}

static int64_t file_tell(BD_FILE_H *file)
{
  return static_cast<CFile*>(file->internal)->GetPosition();
}

static int file_eof(BD_FILE_H *file)
{
  if(static_cast<CFile*>(file->internal)->GetPosition() == static_cast<CFile*>(file->internal)->GetLength())
    return 1;
  else
    return 0;
}

static int64_t file_read(BD_FILE_H *file, uint8_t *buf, int64_t size)
{
  return static_cast<CFile*>(file->internal)->Read(buf, size);
}

static int64_t file_write(BD_FILE_H *file, const uint8_t *buf, int64_t size)
{
    return -1;
}

static BD_FILE_H *file_open(const char* filename, const char *mode)
{
  CStdString cleanName = filename;
  CUtil::RemovePasswordFromPath( cleanName );

    BD_FILE_H *file = new BD_FILE_H;
    CStdString strFilename = filename;

    file->close = file_close;
    file->seek  = file_seek;
    file->read  = file_read;
    file->write = file_write;
    file->tell  = file_tell;
    file->eof   = file_eof;

    if(is_udf_iso_path(filename))
    { 
      CUtil::URLEncode(strFilename);
      strFilename.Format("udf://%s", strFilename);
      CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Opening file udf iso file %s... (%p)", cleanName.c_str(), file);
    }
    else
    {
      CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Opening file %s... (%p)", cleanName.c_str(), file);
    }

    CFile* fp = new CFile();
    if(fp->Open(strFilename.c_str()))
    {
      file->internal = (void*)fp;
      return file;
    }

    delete fp;
    delete file;

    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Error opening file! (%p)", file);

    return NULL;
}

struct SDirState
{
  SDirState() 
    : curr(0)
  {}

  CFileItemList list;
  int           curr;
};

static void dir_close(BD_DIR_H *dir)
{
  if (dir)
  {
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Closed dir (%p)\n", dir);
    delete static_cast<SDirState*>(dir->internal);
    delete dir;
  }
}


static int dir_read(BD_DIR_H *dir, BD_DIRENT *entry)
{
    SDirState* state = static_cast<SDirState*>(dir->internal);

    if(state->curr >= state->list.Size())
      return 1;

    strncpy(entry->d_name, state->list[state->curr]->GetLabel(), sizeof(entry->d_name));
    entry->d_name[sizeof(entry->d_name)-1] = 0;
    state->curr++;

    return 0;
}

static BD_DIR_H *dir_open(const char* dirname)
{
  CStdString cleanName = dirname;
  CUtil::RemovePasswordFromPath( cleanName );
  
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Opening dir %s\n", cleanName.c_str());

    CStdString strDirname = dirname;
    SDirState *st = new SDirState();

    if(is_udf_iso_path(dirname))
    {
      CUtil::URLEncode(strDirname);
      strDirname.Format("udf://%s", strDirname);
      CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Opening udf dir %s... (%p)", cleanName.c_str(), dirname);
    }

    if(!DIRECTORY::CDirectory::GetDirectory(strDirname, st->list))
    {
      CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Error opening dir! (%s)\n", cleanName.c_str());
      delete st;
      return NULL;
    }

    BD_DIR_H *dir = new BD_DIR_H;
    dir->close    = dir_close;
    dir->read     = dir_read;
    dir->internal = (void*)st;

    return dir;
}

CDVDInputStreamBluray::CDVDInputStreamBluray(IDVDPlayer* player) :
  CDVDInputStream(DVDSTREAM_TYPE_BLURAY)
{
  m_player = player;
  m_title = NULL;
  m_titleCount = 0;
  m_allTitlesCount = 0;
  m_bd    = NULL;
  m_dll = new DllLibbluray;
  m_dll->Load();

  m_cachedTime = m_lastTime = 0;
}

CDVDInputStreamBluray::~CDVDInputStreamBluray()
{
  Close();
  delete m_dll;
}

bool CDVDInputStreamBluray::IsEOF()
{
  return false;
}

bool CDVDInputStreamBluray::Open(const char* strFile, const std::string& content)
{
  CStdString strPath;
  CUtil::GetDirectory(strFile,strPath);
  CUtil::RemoveSlashAtEnd(strPath);

  std::vector<CStdString> args;
  CUtil::Tokenize(strFile, args, "?");
  int specificTitle = -1;
  
  if( args.size() > 0 )
  {
    std::vector<CStdString>::const_iterator it = args.begin();
    while(it != args.end())
    {
      if( 1 == sscanf( (*it).c_str(), "title=%d", &specificTitle) )
      {
        specificTitle--;
        break;
      }
      it++;
    }
  }
  
  if(CUtil::GetFileName(strPath) == "PLAYLIST")
  {
    CUtil::GetDirectory(strPath,strPath);
    CUtil::RemoveSlashAtEnd(strPath);
  }

  if(CUtil::GetFileName(strPath) == "BDMV")
  {
    CUtil::GetDirectory(strPath,strPath);
    CUtil::RemoveSlashAtEnd(strPath);
  }

  m_dll->bd_register_dir(dir_open);
  m_dll->bd_register_file(file_open);

#ifdef __USE_BR_NEWVER
  m_dll->bd_set_debug_handler(__bdlog);
  m_dll->bd_set_debug_mask(/*DBG_STREAM |*/ DBG_CONFIGFILE | DBG_MKB | DBG_MMC | DBG_BLURAY | DBG_NAV | DBG_BDPLUS | DBG_CRIT | DBG_HDMV | DBG_BDJ);
#endif

  CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::Open - opening %s", strPath.c_str());
  m_bd = m_dll->bd_open(strPath.c_str(), NULL);

  if(!m_bd)
  {
    CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - failed to open %s", strPath.c_str());
    return false;
  }

  CStdString filename = CUtil::GetFileName(strFile);
  std::map<int, uint64_t> titlesDurationMap;
  int longest = -1;
  if(filename.Equals("index.bdmv"))
  {
    m_titleCount = m_dll->bd_get_titles(m_bd, TITLES_RELEVANT, 0);

    for(int i=0; i < m_titleCount && (!m_player || !m_player->IsAborting()); i++)
    {
      uint64_t duration = m_dll->bd_get_title_duration(m_bd, i);

      if( longest < 0 || titlesDurationMap[longest] < duration )
        longest = i;

      titlesDurationMap[i] = duration;
    }
  }
  else if(CUtil::GetExtension(filename).Equals(".mpls"))
  {
    int titles = m_dll->bd_get_titles(m_bd, TITLES_ALL, 0);
    do
    {
      if(titles < 0)
      {
        CLog::Log(LOGERROR, "get_playlist_title - unable to get list of titles");
        m_title = NULL;
        break;
      }

      unsigned int playlist;
      if(sscanf(filename.c_str(), "%05d.mpls", &playlist) != 1)
      {
        CLog::Log(LOGERROR, "get_playlist_title - unsupported playlist file selected %s", filename.c_str());
        m_title = NULL;
        break;
      }

      BLURAY_TITLE_INFO *t;
      for(int i=0; i < m_titleCount; i++)
      {
        t = m_dll->bd_get_title_info(m_bd, i, 0);
        if(!t)
        {
          CLog::Log(LOGDEBUG, "get_playlist_title - unable to get title %d", i);        
          continue;
        }
        if(t->playlist == playlist)
        {
          m_title = t;
          break;
        }
        m_dll->bd_free_title_info(t);
      }

      m_title = NULL;
      break;

    } while(false);
  }
  else
  {
    CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - unsupported bluray file selected %s", strPath.c_str());
    return false;
  }

  m_title = m_dll->bd_get_title_info(m_bd, specificTitle == -1 ? longest : specificTitle, 0);
  if(!m_title)
  {
    CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - failed to get title info");
    return false;
  }

  if(m_dll->bd_select_title(m_bd, m_title->idx) == 0 )
  {
    CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - failed to select title %d", m_title->idx);
    return false;
  }

  return true;
}

// close file and reset everyting
void CDVDInputStreamBluray::Close()
{
  if(m_bd)
    m_dll->bd_close(m_bd);
  m_bd = NULL;
  if(m_title)
    m_dll->bd_free_title_info(m_title);
  m_title = NULL;
  m_titleCount = 0;
 
  for(std::map<int,BLURAY_TITLE_INFO*>::iterator it = m_titlesInfoMap.begin(); it != m_titlesInfoMap.end(); it++)
  {
    BLURAY_TITLE_INFO *t = (*it).second;
   
    m_dll->bd_free_title_info(t);
  }
  
  m_titlesInfoMap.clear();
}

int CDVDInputStreamBluray::Read(BYTE* buf, int buf_size)
{
  return m_dll->bd_read(m_bd, buf, buf_size);
}

int CDVDInputStreamBluray::GetTotalTime()
{
  if(m_title)
    return m_title->duration / 90;
  else
    return 0;
}

int CDVDInputStreamBluray::GetTime()
{
  int64_t t = m_dll->bd_tell_time(m_bd) / 90;
  if( m_cachedTime && !m_lastTime && t )
  {
    // we just did a seek and libbluray is putting us back into a specific clip
    // what we do here is baseline the libbr timestamp w/ our cached time so m_cachedTime is now
    // the start of a clip
    m_cachedTime -= t;
  }
  else if( !t && m_cachedTime && m_lastTime )
  {
    // we just did a forward clip transition during normal playback
    m_cachedTime += m_lastTime;
  }

  m_lastTime = t;
  t = m_cachedTime + t;
  return t;
}

bool CDVDInputStreamBluray::SeekTime(int ms)
{
  int64_t newval = m_dll->bd_seek_time(m_bd, ms * 90);
  if( newval < 0)
    return false;
  else
  {
    m_cachedTime = ms; //m_dll->bd_tell_time(m_bd) / 90;
    m_lastTime = 0;
    return true;
  }
}

int CDVDInputStreamBluray::GetChapterCount()
{
  if(m_title)
    return m_title->chapter_count;
  else
    return 0;
}

int CDVDInputStreamBluray::GetChapter()
{
  if(m_title)
    return m_dll->bd_get_current_chapter(m_bd) + 1;
  else
    return 0;
}

void CDVDInputStreamBluray::GetChapterName(std::string& name)
{
  if(m_title)
  {
    // until libbluray can read chapter names
    int idx = GetChapter();
    CStdString title;
    title.Format("%s %d", g_localizeStrings.Get(21396).c_str(), idx);

    name.assign(title);
  }
  else
    return ;
}

bool CDVDInputStreamBluray::SeekChapter(int ch)
{
  if(m_title && m_dll->bd_seek_chapter(m_bd, ch-1) < 0)
    return false;
  else
  {
    // load the start time of the chapter
    m_cachedTime = m_title->chapters[ch-1].start / 90;
    m_lastTime = 0;
    return true;
  }
}

__int64 CDVDInputStreamBluray::Seek(__int64 offset, int whence)
{
#if LIBBLURAY_BYTESEEK
  if(whence == SEEK_POSSIBLE)
    return 1;
  else if(whence == SEEK_CUR)
  {
    if(offset == 0)
      return m_dll->bd_tell(m_bd);
    else
      offset += bd_tell(m_bd);
  }
  else if(whence == SEEK_END)
    offset += m_dll->bd_get_title_size(m_bd);
  else if(whence != SEEK_SET)
    return -1;

  int64_t pos = m_dll->bd_seek(m_bd, offset);
  if(pos < 0)
  {
    CLog::Log(LOGERROR, "CDVDInputStreamBluray::Seek - seek to %"PRId64", failed with %"PRId64, offset, pos); 
    return -1;
  }

  if(pos != offset)
    CLog::Log(LOGWARNING, "CDVDInputStreamBluray::Seek - seek to %"PRId64", ended at %"PRId64, offset, pos); 

  return offset;
#else
  if(whence == SEEK_POSSIBLE)
    return 0;
  else if( whence == SEEK_SET && offset == 0 )
    return m_dll->bd_seek(m_bd,0);
  return -1;
#endif
}

__int64 CDVDInputStreamBluray::GetLength()
{
  return m_dll->bd_get_title_size(m_bd);
}

static bool find_stream(int pid, BLURAY_STREAM_INFO *info, int count, char* language)
{
  int i=0;
  for(;i<count;i++,info++)
  {
    if(info->pid == pid)
      break;
  }
  if(i==count)
    return false;
  memcpy(language, info->lang, 4);
  return true;
}

static bool find_stream(BLURAY_TITLE_INFO *title, int pid,  char* language)
{
  BLURAY_CLIP_INFO *clip = title->clips;

  if(find_stream(pid, clip->audio_streams, clip->audio_stream_count, language))
    return true;
  if(find_stream(pid, clip->pg_streams, clip->pg_stream_count, language))
    return true;
  if(find_stream(pid, clip->ig_streams, clip->ig_stream_count, language))
    return true;

  return false; 
}


void CDVDInputStreamBluray::GetStreamInfo(int pid, char* language)
{
  if(m_title->clip_count == 0)
    return;

  if(find_stream(m_title, pid, language))
    return;

  std::map<int,BLURAY_TITLE_INFO*>::iterator it;
  for(it = m_titlesInfoMap.begin(); it != m_titlesInfoMap.end(); it++)
  {
     if(find_stream((*it).second, pid, language))
       return;  
  }

  if(m_allTitlesCount == 0)
  {  
    m_allTitlesCount = m_dll->bd_get_titles(m_bd, TITLES_ALL, 0);
  }

  for(size_t i = m_allTitlesCount - 1; i >= m_titlesInfoMap.size(); i--)
  {
    if(m_titlesInfoMap[i] == NULL)
    { 
      BLURAY_TITLE_INFO *t =  m_dll->bd_get_title_info(m_bd, i, 0);
      if(t)
      {
        m_titlesInfoMap[i] = t;
        if(find_stream(t, pid, language))
         return;
      }  
    }
  }
}

void CDVDInputStreamBluray::GetChapterInfo(int chap, uint64_t& startTime, uint64_t& duration, std::string& title)
{
  if( !m_title || chap < 1 || chap > (int) m_title->chapter_count )
    return ;

  startTime = m_title->chapters[chap-1].start / 90;
  duration = m_title->chapters[chap-1].duration / 90;

  CStdString str;
  str.Format("%s %d", g_localizeStrings.Get(21396).c_str(), chap);
  title.assign(str);
}

void CDVDInputStreamBluray::GetTitleInfo(int title, uint64_t& duration, std::string& name)
{
  
  if( title < 1 || title > m_titleCount) return ;
  title--;

  duration = m_dll->bd_get_title_duration(m_bd, title) / 90;
   
  name.clear();
}
int CDVDInputStreamBluray::GetTitleCount()
{
  return m_titleCount;
}
int CDVDInputStreamBluray::GetCurrentTitle()
{
  return m_title->idx + 1;
}

#endif
