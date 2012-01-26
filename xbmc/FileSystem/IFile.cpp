/*
* XBMC Media Center
* Copyright (c) 2002 Frodo
* Portions Copyright (c) by the authors of ffmpeg and xvid
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

#include "IFile.h"
#include <cstring>
#include <errno.h>

using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IFile::IFile() : m_bSkipEol(false)
{
}

IFile::~IFile()
{
}

int IFile::Stat(struct __stat64* buffer)
{
  memset(buffer, 0, sizeof (buffer));
  errno = ENOENT;
  return -1;
}

bool IFile::ReadString(char *szLine, int iLineLength)
{
  memset(szLine,0,iLineLength);
  char *pChar = szLine;
  int nRead = 0;
  int nReadRc = 0;
  while ( (nReadRc = Read( (unsigned char*)pChar, 1)) > 0 && pChar < szLine + iLineLength - 1)
  {
    while (m_bSkipEol && (*pChar == '\r' || *pChar == '\n') && (nReadRc = Read( (unsigned char*)pChar, 1)) > 0)
             ;

    if (nReadRc <= 0)
      break;

    m_bSkipEol = false;
    nRead++;

    if (*pChar == '\n' || *pChar == '\r')
    {
      m_bSkipEol = true;
      *pChar = '\0';
      break;
    }

    pChar++;
  }
  return (nRead>0);
}

