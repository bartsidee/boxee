/*
 * UPnP Support for XBMC
 * Based on CFileDAAP
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "FileUPnP.h"
#include "UPnPDirectory.h"
#include "FileItem.h"
#include "SectionLoader.h"
#include "FileFactory.h"
#include <sys/stat.h>

using namespace XFILE;
using namespace DIRECTORY;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFileUPnP::CFileUPnP()
{  
  m_file = NULL;
  m_bOpened = false;
}

CFileUPnP::~CFileUPnP()
{
  Close();
  
  if( m_file )
    delete m_file;
}



//*********************************************************************************************
bool CFileUPnP::Open(const CURI& url)
{
  // When we get an open request, we pull the underlying resource from the virtual url
  // and feed the resource paths back to the file factory until we get something we can
  // work with...
  
  if (m_bOpened) Close();

  CLog::Log(LOGDEBUG, "CFileUPnP::Open(%s)", url.GetFileName().c_str());

  CUPnPDirectory dir;
  CFileItem item;
  
  // query the upnp directory to determine the resource URLs for this virtual URL
  if( dir.GetResource( url, item ) )
  {
    // This loads the first resource string which is hopefully best; we could be more
    // articulate here as to what specific resource we want
    m_bOpened = true;
    m_file = CFileFactory::CreateLoader( item.m_strPath );
    if( m_file )
    {
      CLog::Log(LOGDEBUG, "CFileUPnP::Open on stream (%s)", item.m_strPath.c_str());
      return m_file->Open( item.m_strPath );
    }
  }
  return false;
}


//*********************************************************************************************
unsigned int CFileUPnP::Read(void *lpBuf, int64_t uiBufSize)
{
  return m_file->Read(lpBuf, uiBufSize);
}

//*********************************************************************************************
void CFileUPnP::Close()
{
  if( m_file )
    m_file->Close();
  
  m_bOpened = false;
}

//*********************************************************************************************
int64_t CFileUPnP::Seek(int64_t iFilePosition, int iWhence)
{
  return m_file->Seek(iFilePosition, iWhence);
}

//*********************************************************************************************
int64_t CFileUPnP::GetLength()
{
  return m_file->GetLength();
}

//*********************************************************************************************
int64_t CFileUPnP::GetPosition()
{
  return m_file->GetPosition();
}

bool CFileUPnP::Exists(const CURI& url)
{
  return false;
}

int CFileUPnP::Stat(const CURI& url, struct __stat64* buffer)
{
  int res = -1;
  CUPnPDirectory dir;
  CFileItem item;
  IFile* file;
  
  // query the upnp directory to determine the resource URLs for this virtual URL
  if( dir.GetResource( url, item ) )
  {
    // This loads the first resource string which is hopefully best; we could be more
    // articulate here as to what specific resource we want
    m_bOpened = true;
    file = CFileFactory::CreateLoader( item.m_strPath );
    if( file )
    {
      CURI pUrl( item.m_strPath );
      CLog::Log(LOGDEBUG, "CFileUPnP::Stat on stream (%s)", item.m_strPath.c_str());
      res = file->Stat( pUrl, buffer );
      delete file;
    }
  }
  return res;
}
