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

#include "DVDInputStream.h"

class IDVDPlayer;
class DllLibbluray;
typedef struct bluray BLURAY;
typedef struct bd_title_info BLURAY_TITLE_INFO;

class CDVDInputStreamBluray 
  : public CDVDInputStream
  , public CDVDInputStream::IDisplayTime
  , public CDVDInputStream::IChapter
  , public CDVDInputStream::ISeekTime
{
public:
  CDVDInputStreamBluray(IDVDPlayer* player = NULL);
  virtual ~CDVDInputStreamBluray();
  virtual bool Open(const char* strFile, const std::string &content);
  virtual void Close();
  virtual int Read(BYTE* buf, int buf_size);
  virtual __int64 Seek(__int64 offset, int whence);
  virtual bool Pause(double dTime) { return false; };
  virtual bool IsEOF();
  virtual __int64 GetLength();
  
  int GetChapter();
  int GetChapterCount();
  void GetChapterName(std::string& name);
  bool SeekChapter(int ch);

  int GetTotalTime();
  int GetTime();
  bool SeekTime(int ms);

  void GetStreamInfo(int pid, char* language);

  void GetChapterInfo(int chap, uint64_t& startTime, uint64_t& duration, std::string& title);

  void GetTitleInfo(int title, uint64_t& duration, std::string& name);
  int GetTitleCount();
  int GetCurrentTitle();
  
protected:
  IDVDPlayer* m_player;
  DllLibbluray *m_dll;
  BLURAY* m_bd;
  BLURAY_TITLE_INFO* m_title;
  int m_allTitlesCount;
  int m_titleCount;
  std::map<int, BLURAY_TITLE_INFO*> m_titlesInfoMap;
  int64_t m_cachedTime;
  int64_t m_lastTime;
};
