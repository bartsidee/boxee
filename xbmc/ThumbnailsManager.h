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

#include "utils/CriticalSection.h"

#include <string>

#ifdef _WIN32
#include "lib/libgdbm/gdbm.h"
#else 
#include <gdbm.h>
#endif
//
// The reference count and progress information for the cache objects
//
struct ThumbObjRecord
  {
  public:
    time_t     m_accessTime;
    uint64_t   m_size;
  };

class CThumbnailManager
{
public:
  CThumbnailManager();
  ~CThumbnailManager();

  bool Initialize(const std::string &strLocalThumbFile);
  void Deinitialize();

  void TouchThumbnailFile(const std::string &strLocalThumbFile, int64_t newFileSize);

  // you can clear the thumbnails by two criterias:
  // 1. byDate - use for clearOldThumbnails - older then a specific date - then the sizeToDelete sould be equal
  //    to the whole quota (and only the older file will be deleted)
  // 2. by Size
  //    you can the size and time can be the avergae time or even now - the files will be deleted till
  //    the specific sizez was erased
  void ClearThumnails(time_t  byDate , uint64_t sizeToDelete, bool bEnsureThumnailsQuota=false);

  // Deletes all the thumbnails, the gdbm file and starts fresh
  bool HardReset();

protected:

  bool          OpenFile(int permission);
  void          CloseFile();
  void          EnsureThumnailsQuota(int64_t newFileSize);
  void          Reorganize();
  void          FreeDatum(datum  &thumbData);

  // allocate memeory for the key.dptr - need to be freed after use
  bool          SetThumKey(datum  &thumbKey, const std::string &keyStr);
  bool          IsExternalThumbFile(const std::string &strThumbFile);

  char*         m_path;

  CCriticalSection m_lock;
  GDBM_FILE        m_file;
  uint32_t         m_thumbNum;
  time_t           m_averageTime;
};
