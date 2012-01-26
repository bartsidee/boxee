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
#include "lib/libBoxee/bxxmldocument.h"

#ifdef _WIN32
#define __func__ __FUNCTION__
#endif

using namespace XFILE;

CDVDInputStreamPlaylist::CDVDInputStreamPlaylist(DVDStreamType streamType) : CDVDInputStreamFile(streamType)
{
  m_iCurrentChapter = 0;
}

void CDVDInputStreamPlaylist::ClearChapters()
{
  std::map<int, PlaylistChapterInfo*>::iterator it = m_chaptersInfoMap.begin();
  while(it != m_chaptersInfoMap.end())
  {
    PlaylistChapterInfo* chapterInfo = it->second;

    //m_chaptersInfoMap.erase(it);
    delete chapterInfo;

    ++it;
  }

  m_chaptersInfoMap.clear();
}

CDVDInputStreamPlaylist::~CDVDInputStreamPlaylist()
{
  ClearChapters();
  Close();
}

bool CDVDInputStreamPlaylist::SetChapters(const CStdString& chapterInfo)
{
  CSingleLock lock(m_lock);

  ClearChapters();

  do
  {
    TiXmlElement* root = NULL;
    BOXEE::BXXMLDocument reader;
    int i = 0;

    if(chapterInfo.IsEmpty())
    {
      break;
    }

    if(!reader.LoadFromString(chapterInfo))
    {
      CLog::Log(LOGERROR, "CDVDInputStreamPlaylist::%s - FAILED to load chapters info", __func__);
      break;
    }

    root = reader.GetRoot();
    if(root == NULL)
    {
      CLog::Log(LOGERROR, "CDVDInputStreamPlaylist::%s - FAILED to get root", __func__);
      break;
    }

    if(strcmp(root->Value(), "chapters"))
    {
      CLog::Log(LOGERROR, "CDVDInputStreamPlaylist::%s - INVALID root value", __func__);
      break;
    }

    root = root->FirstChildElement();
    while(root)
    {
      if(strcmp(root->Value(),"chapter") == 0)
      {
        int nHours=0, nMin=0, nSec=0;
        PlaylistChapterInfo* chapterInfo = new PlaylistChapterInfo;
        chapterInfo->name = root->Attribute("name");

        sscanf(root->Attribute("startTime"), "%d:%d:%d",&nHours,&nMin,&nSec);
        chapterInfo->startTime = nHours*3600 + nMin*60 + nSec;

        chapterInfo->duration = atoi(root->Attribute("duration"));

        m_chaptersInfoMap[i++] = chapterInfo;
      }

      root = root->NextSiblingElement();
    }

    if((((CFilePlaylist*)(m_pFile->GetImplemenation())))->IsPlayingLiveStream())
    {
      // add "live" chapter
      PlaylistChapterInfo* chapterInfo = new PlaylistChapterInfo;

      chapterInfo->name = "Live";
      chapterInfo->startTime = -1;
      chapterInfo->duration = 0;

      m_chaptersInfoMap[i++] = chapterInfo;
    }

  } while(false);

  return true;
}

bool CDVDInputStreamPlaylist::Open(const char* strFile, const std::string& content)
{
  if (!CDVDInputStreamFile::Open(strFile, content)) return false;

  if(m_item.HasProperty("custom:chapters"))
  {

    CStdString chapterInfo = m_item.GetProperty("custom:chapters");
    if(!chapterInfo.IsEmpty())
    {
       SetChapters(chapterInfo);
    }
  }

  return true;
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

int CDVDInputStreamPlaylist::GetSeekTime(bool bPlus, bool bLargeStep)
{
  int smallSeekSkipSecs = -1;
  int bigSeekSkipSecs = -1;

  if(m_item.HasProperty("custom:smallSeekSkipSecs"))
  {
    smallSeekSkipSecs = m_item.GetPropertyULong("custom:smallSeekSkipSecs");
  }
  if(m_item.HasProperty("custom:bigSeekSkipSecs"))
  {
    bigSeekSkipSecs = m_item.GetPropertyULong("custom:bigSeekSkipSecs");
  }

  return (((CFilePlaylist*)(m_pFile->GetImplemenation())))->GetSeekTime(bPlus, bLargeStep, smallSeekSkipSecs, bigSeekSkipSecs);
}

unsigned int CDVDInputStreamPlaylist::StartTime()
{
  return (((CFilePlaylist*)(m_pFile->GetImplemenation())))->GetStartTime();
}

int CDVDInputStreamPlaylist::GetChapter()
{
  CSingleLock lock(m_lock);

  return m_iCurrentChapter;
}

int CDVDInputStreamPlaylist::GetChapterCount()
{
  CSingleLock lock(m_lock);

  return m_chaptersInfoMap.size();
}

void CDVDInputStreamPlaylist::GetChapterName(std::string& name)
{
  CSingleLock lock(m_lock);

  if(m_chaptersInfoMap.size() == 0)
    return;

  PlaylistChapterInfo* chapterInfo = m_chaptersInfoMap[m_iCurrentChapter];
  if(chapterInfo)
  {
    name.assign(chapterInfo->name.c_str());
  }
}

void CDVDInputStreamPlaylist::GetChapterInfo(int chap, uint64_t& startTime, uint64_t& duration, std::string& name)
{
  CSingleLock lock(m_lock);

  if(m_chaptersInfoMap.size() == 0 || chap < 1)
      return;

  PlaylistChapterInfo* chapterInfo = m_chaptersInfoMap[chap-1];

  if(chapterInfo)
  {
    if(chapterInfo->name == "Live")
    {
      chapterInfo->startTime = (((CFilePlaylist*)(m_pFile->GetImplemenation())))->GetLiveStartTime();
    }

    /* do not display chapter time */
    startTime = 0;
    duration = 0;
    name.assign(chapterInfo->name.c_str());
  }
}

bool CDVDInputStreamPlaylist::SeekChapter(int ch)
{
  CSingleLock lock(m_lock);

  bool bRet = false;
  if(m_chaptersInfoMap.size() == 0 || ch < 1)
    return false;

  PlaylistChapterInfo* chapterInfo = m_chaptersInfoMap[ch-1];
  if(chapterInfo)
  {
    bRet = (((CFilePlaylist*)(m_pFile->GetImplemenation())))->SeekChapter(chapterInfo->startTime);
    
    if(bRet) 
    {
      m_iCurrentChapter = ch-1;
    }
  }

  return bRet;
}

int CDVDInputStreamPlaylist::GetCurrentTimecode()
{
  return (((CFilePlaylist*)(m_pFile->GetImplemenation())))->GetCurrentTimecode();
}

DWORD CDVDInputStreamPlaylist::GetReadAhead()
{
  return (((CFilePlaylist*)(m_pFile->GetImplemenation())))->GetAvailableRead();
}

bool CDVDInputStreamPlaylist::CanSeek()
{
  return (((CFilePlaylist*)(m_pFile->GetImplemenation())))->CanSeek();
}

bool CDVDInputStreamPlaylist::CanPause()
{
  return (((CFilePlaylist*)(m_pFile->GetImplemenation())))->CanPause();
}

