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
#include "PlayList.h"

namespace PLAYLIST
{

class CPlayListM3U :
      public CPlayList
{
public:
  CPlayListM3U(void);
  virtual ~CPlayListM3U(void);
  virtual bool Load(const CStdString& strFileName);
  virtual void Save(const CStdString& strFileName) const;

  virtual int64_t GetOverallLength();

  virtual bool CanAdd();

private:

  bool LoadM3uFile(const CStdString& strFileName, unsigned int level);
  void Reset();

  bool ParseFirstLine(const CStdString& strLine, int lineCounter, bool& hasEXTM3U);
  bool ParseEXTINF(const CStdString& strLine, CStdString& strInfo, long& lDuration);
  bool ParseEXTXTARGETDURATION(const CStdString& strLine, CStdString& targetDurationTag, bool hasEXTM3U);
  bool ParseEXTXMEDIASEQUENCE(const CStdString& strLine, long& playlistSequenceNo, bool hasEXTM3U);
  bool ParseEXTXSTREAMINF(const CStdString& strLine,CStdString& bandwidth,CStdString& programId,CStdString& codecs,bool& nextUrlIsPlaylist, bool hasEXTM3U);
  bool ParseEXTXKEY(const CStdString& strLine, CStdString& encryptMethod, CStdString& encryptKeyUri, CStdString& encryptIv, bool hasEXTM3U);

  bool isEncryptionMethodValid(const CStdString& encryptionMethod);
  void AccumulatePlaylistOverallDuration(int64_t duration);

  void RemoveQuotationMark(CStdString& text);

  bool m_isM3U8;
  unsigned int m_levels;

  bool m_endListTagWasRead;

  int64_t m_overallLength;
};

}
