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
 
#include "DVDInputStreamFile.h"
#include "FileItem.h"
#include "FileSystem/File.h"
#include "utils/log.h"

using namespace XFILE;

CDVDInputStreamFile::CDVDInputStreamFile(DVDStreamType streamType) : CDVDInputStream(streamType)
{
  m_pFile = NULL;
  m_eof = true;
}

CDVDInputStreamFile::~CDVDInputStreamFile()
{
  Close();
}

bool CDVDInputStreamFile::HasFileError(CStdString *err) { 
  if (!m_pFile) {
    *err = "Player failed to open media.";
	  return true;
  }
  return m_pFile->HasFileError(err); 
}

bool CDVDInputStreamFile::IsEOF()
{
  return !m_pFile || m_eof;
}

bool CDVDInputStreamFile::Open(const char* strFile, const std::string& content)
{
  CStdString stdFile = strFile;
  
  CURL url(stdFile);
  if (!CDVDInputStream::Open(strFile, content)) return false;

  m_pFile = new CFile();
  if (!m_pFile) return false;

  unsigned int flags = READ_TRUNCATED;

  if( CFileItem(stdFile, false).IsInternetStream() )
    flags |= READ_CACHED;

  // open file in binary mode
  if (!m_pFile->Open(stdFile, flags))
  {
    delete m_pFile;
    m_pFile = NULL;
    return false;
  }
  
  if (m_pFile->GetImplemenation() && (content.empty() || content == "application/octet-stream"))
    m_content = m_pFile->GetImplemenation()->GetContent();

  m_eof = true;
  return true;
}

// close file and reset everyting
void CDVDInputStreamFile::Close()
{
  if (m_pFile)
  {
    m_pFile->Close();
    delete m_pFile;
  }

  CDVDInputStream::Close();
  m_pFile = NULL;
  m_eof = true;
}

int CDVDInputStreamFile::Read(BYTE* buf, int buf_size)
{
  if(!m_pFile) return -1;

  unsigned int ret = m_pFile->Read(buf, buf_size);

  /* we currently don't support non completing reads */
  if( ret <= 0 ) m_eof = true;
  return (int)(ret & 0xFFFFFFFF);
}

__int64 CDVDInputStreamFile::Seek(__int64 offset, int whence)
{
  if(!m_pFile) return -1;
  __int64 ret = m_pFile->Seek(offset, whence);

  /* if we succeed, we are not eof anymore */
  if( ret >= 0 ) m_eof = false;

  return ret;
}

__int64 CDVDInputStreamFile::GetLength()
{
  if (m_pFile)
    return m_pFile->GetLength();
  return 0;
}

BitstreamStats CDVDInputStreamFile::GetBitstreamStats() const 
{
  if (!m_pFile)
    return m_stats; // dummy return. defined in CDVDInputStream

  return m_pFile->GetBitstreamStats();
}

__int64 CDVDInputStreamFile::GetPosition() 
{
  if (!m_pFile)
    return 0;
  return m_pFile->GetPosition();
}

DWORD CDVDInputStreamFile::GetReadAhead() 
{
  if (!m_pFile || !m_pFile->GetImplemenation())
    return 0;
  return m_pFile->GetImplemenation()->GetAvailableRead();  
}

bool CDVDInputStreamFile::Skip(int skipActionType)
{
  CLog::Log(LOGERROR,"CDVDInputStreamFile::Skip - Skip action isn't handle in this CDVDInputStream [type=%d]",m_streamType);
  return false;
}





