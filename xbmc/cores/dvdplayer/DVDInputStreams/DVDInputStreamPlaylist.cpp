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
 
#include "DVDInputStreamPlaylist.h"
#include "FileItem.h"
#include "FileSystem/File.h"
#include "FileSystem/FilePlaylist.h"
#include "utils/log.h"

using namespace XFILE;

CDVDInputStreamPlaylist::CDVDInputStreamPlaylist(DVDStreamType streamType) : CDVDInputStreamFile(streamType)
{
}

CDVDInputStreamPlaylist::~CDVDInputStreamPlaylist()
{
  Close();
}

bool CDVDInputStreamPlaylist::OnAction(const CAction &action)
{
  if (!m_pFile)
    return false;
  return (((CFilePlaylist*)(m_pFile->GetImplemenation())))->OnAction(action); 
}

bool CDVDInputStreamPlaylist::IsEOF()
{
  return (((CFilePlaylist*)(m_pFile->GetImplemenation())))->IsEOF(); 
}

void CDVDInputStreamPlaylist::Abort()
{
  (((CFilePlaylist*)(m_pFile->GetImplemenation())))->StopThread();
}

unsigned int CDVDInputStreamPlaylist::GetTime()
{
  return (((CFilePlaylist*)(m_pFile->GetImplemenation())))->GetCurrentTime();   
}

unsigned int CDVDInputStreamPlaylist::GetTotalTime()
{
  return (((CFilePlaylist*)(m_pFile->GetImplemenation())))->GetTotalTime();   
}

bool CDVDInputStreamPlaylist::SeekTime(__int64 millis)
{
  return (((CFilePlaylist*)(m_pFile->GetImplemenation())))->SeekToTime((unsigned int)(millis / 1000));
}

unsigned int CDVDInputStreamPlaylist::StartTime()
{
  return (((CFilePlaylist*)(m_pFile->GetImplemenation())))->GetStartTime();
}
