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

#include "PlayListM3U.h"
#include "FileSystem/File.h"
#include "Util.h"
#include "StringUtils.h"
#include "utils/CharsetConverter.h"
#include "utils/RegExp.h"
#ifndef _LINUX
#include "cores/dllloader/exports/emu_msvcrt.h"
#endif

#include "AdvancedSettings.h"
#include "MusicInfoTag.h"
#include "AdvancedSettings.h"

using namespace PLAYLIST;
using namespace XFILE;

#define M3U_START_MARKER "#EXTM3U"
#define M3U_INFO_MARKER  "#EXTINF"
#define M3U_ARTIST_MARKER  "#EXTART"
#define M3U_ALBUM_MARKER  "#EXTALB"

// example m3u file:
//   #EXTM3U
//   #EXTART:Demo Artist
//   #EXTALB:Demo Album
//   #EXTINF:5,demo
//   E:\Program Files\Winamp3\demo.mp3
//   #EXTINF:5,demo
//   E:\Program Files\Winamp3\demo.mp3

// M3U8 tags
#define EXT_X_TARGETDURATION    "#EXT-X-TARGETDURATION"
#define EXT_X_MEDIA_SEQUENCE    "#EXT-X-MEDIA-SEQUENCE"
#define EXT_X_KEY               "#EXT-X-KEY"
#define EXT_X_PROGRAM_DATE_TIME "#EXT-X-PROGRAM-DATE-TIME"
#define EXT_X_ALLOW_CACHE       "#EXT-X-ALLOW-CACHE"
#define EXT_X_STREAM_INF        "#EXT-X-STREAM-INF"
#define EXT_X_ENDLIST           "#EXT-X-ENDLIST"
#define EXT_X_DISCONTINUITY     "#EXT-X-DISCONTINUITY"

CPlayListM3U::CPlayListM3U(void)
{
  m_isM3U8 = false;
  m_levels = 0;
  m_overallLength = -1;
  m_endListTagWasRead = false;
}

CPlayListM3U::~CPlayListM3U(void)
{

}

void CPlayListM3U::Reset()
{
  Clear();

  m_isM3U8 = false;
  m_levels = 0;
  m_overallLength = -1;
  m_endListTagWasRead = false;
}

bool CPlayListM3U::Load(const CStdString& strFileName)
{
  CLog::Log(LOGDEBUG,"CPlayListM3U::Load - Going to call LoadM3u with [strFileName=%s] (m3u)", strFileName.c_str());

  Reset();

  m_strPlayListName = CUtil::GetFileName(strFileName);
  CUtil::GetParentPath(strFileName, m_strBasePath);
 
  CURI url(m_strBasePath);
  if(!url.GetOptions().IsEmpty())
  { 
    m_strBasePath = url.GetUrlWithoutOptions();
  }

  bool succeeded = LoadM3uFile(strFileName,0);

  CLog::Log(LOGDEBUG,"CPlayListM3U::Load - Call to LoadM3u with [strFileName=%s] returned [%d] (m3u)", strFileName.c_str(),succeeded);

  return succeeded;
}

bool CPlayListM3U::LoadM3uFile(const CStdString& strFileName, unsigned int level)
{
  CLog::Log(LOGDEBUG,"CPlayListM3U::LoadM3uFile - Enter function with [strFileName=%s][level=%d] (m3u)", strFileName.c_str(),level);

  char szLine[4096];
  CStdString strLine;

  ///////////////////
  // playlist tags //
  ///////////////////

  bool hasEXTM3U = false;
  CStdString targetDurationTag = "";
  long playlistSequenceNo = -1;

  CStdString bandwidth = "";
  CStdString programId = "";
  CStdString codecs = "";
  bool nextUrlIsPlaylist = false;

  CStdString encryptionMethod = "";
  CStdString encryptKeyUri = "";
  CStdString encryptIv = "";
  int startTime = -1;
  int numOfUris = 0;
  bool discontinuity = false;
  bool readStartTime = false;

  //////////////
  // uri tags //
  //////////////

  CStdString strInfo = "";
  long lDuration = 0;

  CFile file;
  if (!file.Open(strFileName) )
  {
    file.Close();
    return false;
  }

  CLog::Log(LOGDEBUG,"CPlayListM3U::LoadM3uFile - Start reading file [strFileName=%s]. [level=%d][m_isM3U8=%d] (m3u)", strFileName.c_str(),level,m_isM3U8);

  int lineCounter = 0;

  while (file.ReadString(szLine, 1024))
  {
    strLine = szLine;
    lineCounter++;

    //CLog::Log(LOGDEBUG,"CPlayListM3U::Load - [%d] - [%s]. [level=%d][m_isM3U8=%d] (m3u)",lineCounter,strLine.c_str(),level,m_isM3U8);

    if (m_endListTagWasRead)
    {
      CLog::Log(LOGDEBUG,"CPlayListM3U::Load - #EXT-X-ENDLIST was read -> Going to break. [level=%d][m_isM3U8=%d] (m3u)",level,m_isM3U8);
      break;
    }

    StringUtils::RemoveCRLF(strLine);

    if (ParseFirstLine(strLine, lineCounter, hasEXTM3U))
    {
      if (hasEXTM3U)
      {
        // first line is #EXTM3U -> continue
        continue;
      }
    }

    if (strLine[0] == '#')
    {
      ////////////////////////////
      // line is tag or comment //
      ////////////////////////////

      if (strLine.Left( (int)strlen(M3U_INFO_MARKER) ) == M3U_INFO_MARKER)
      {
        // Handle #EXTINF
        ParseEXTINF(strLine, strInfo, lDuration);
      }
      else if (strLine.Left((int)strlen(EXT_X_TARGETDURATION)) == EXT_X_TARGETDURATION)
      {
        // Handle #EXT-X-TARGETDURATION
        if (!ParseEXTXTARGETDURATION(strLine,targetDurationTag,hasEXTM3U))
        {
          CLog::Log(LOGERROR,"CPlayListM3U::LoadM3uFile - FAILED to parse of #EXT-X-TARGETDURATION. [strFileName=%s][level=%d][m_isM3U8=%d] (m3u)",strFileName.c_str(),level,m_isM3U8);
          file.Close();
          return false;
        }
      }
      else if (strLine.Left((int)strlen(EXT_X_MEDIA_SEQUENCE)) == EXT_X_MEDIA_SEQUENCE)
      {
        // Handle #EXT-X-MEDIA-SEQUENCE
        if (!ParseEXTXMEDIASEQUENCE(strLine,playlistSequenceNo,hasEXTM3U))
        {
          CLog::Log(LOGERROR,"CPlayListM3U::LoadM3uFile - FAILED to parse of #EXT-X-MEDIA-SEQUENCE. [strFileName=%s][level=%d][m_isM3U8=%d] (m3u)",strFileName.c_str(),level,m_isM3U8);
          file.Close();
          return false;
        }
      }
      else if (strLine.Left((int)strlen(EXT_X_STREAM_INF)) == EXT_X_STREAM_INF)
      {
        // Handle #EXT-X-STREAM-INF
        if (!ParseEXTXSTREAMINF(strLine,bandwidth,programId,codecs,nextUrlIsPlaylist,hasEXTM3U))
        {
          CLog::Log(LOGERROR,"CPlayListM3U::LoadM3uFile - FAILED to parse of #EXT-X-STREAM-INF. [strFileName=%s][level=%d][m_isM3U8=%d] (m3u)",strFileName.c_str(),level,m_isM3U8);
          file.Close();
          return false;
        }
      }
      else if (strLine.Left((int)strlen(EXT_X_KEY)) == EXT_X_KEY)
      {
        // Handle #EXT-X-KEY
        if (!ParseEXTXKEY(strLine,encryptionMethod,encryptKeyUri,encryptIv,hasEXTM3U))
        {
          CLog::Log(LOGERROR,"CPlayListM3U::LoadM3uFile - FAILED to parse of #EXT-X-KEY. [strFileName=%s][level=%d][m_isM3U8=%d] (m3u)",strFileName.c_str(),level,m_isM3U8);
          file.Close();
          return false;
        }
      }
      else if (strLine.Left((int)strlen(EXT_X_ENDLIST)) == EXT_X_ENDLIST)
      {
        // Handle #EXT-X-ENDLIST

        m_endListTagWasRead = true;
      }
      else if (strLine.Left((int)strlen(EXT_X_PROGRAM_DATE_TIME)) == EXT_X_PROGRAM_DATE_TIME)
      {
        CStdStringArray arr;
        StringUtils::SplitString(strLine, "T", arr);
        if (arr.size() > 4)
        {
          int nHours=0, nMin=0, nSec=0;
          sscanf(arr[4].c_str(), "%d:%d:%d",&nHours,&nMin,&nSec);
          startTime = nHours*3600 + nMin*60 + nSec;
          readStartTime = true;
        }
      }
      else if (strLine.Left((int)strlen(EXT_X_DISCONTINUITY)) == EXT_X_DISCONTINUITY)
      {
        // Handle #EXT-X-DISCONTINUITY
        discontinuity = true;
      }
    }
    else
    {
      /////////////////
      // line is URI //
      /////////////////

      CStdString strUri = strLine;

      if (strUri.length() > 0)
      {
        g_charsetConverter.unknownToUTF8(strUri);

        // If no info was read from from the extended tag information, use the file name
        if (strInfo.length() == 0)
        {
          strInfo = CUtil::GetFileName(strUri);
        }

        // should substitition occur before or after charset conversion??
        if (CUtil::IsRemote(m_strBasePath) && g_advancedSettings.m_pathSubstitutions.size() > 0)
        {
          strUri = CUtil::SubstitutePath(strUri);
        }

        // Get the full path file name and add it to the the play list
        CUtil::GetQualifiedFilename(m_strBasePath, strUri);
        CFileItemPtr newItem(new CFileItem(strInfo));
        newItem->m_strPath = strUri;
        newItem->GetMusicInfoTag()->SetDuration(lDuration);

        if (m_isM3U8)
        {
          /////////////////////////////
          // set item with M3U8 tags //
          /////////////////////////////

          if (targetDurationTag.IsEmpty() && !nextUrlIsPlaylist && !newItem->IsPlayList())
          {
            CLog::Log(LOGERROR,"CPlayListM3U::LoadM3uFile - CANNOT add [uri=%s] because the value of #EXT-X-TARGETDURATION is empty. [strFileName=%s][level=%d][m_isM3U8=%d] (m3u)",newItem->m_strPath.c_str(),strFileName.c_str(),level,m_isM3U8);
            file.Close();
            return false;
          }

          if (playlistSequenceNo == -1)
          {
            // #EXT-X-MEDIA-SEQUENCE tag wasn't read -> set sequence of first URI to 1
            playlistSequenceNo = 1;
          }

          if(startTime != -1)
          {
            if(readStartTime == false)
              startTime += lDuration;

            newItem->SetProperty("m3u8-startDate",startTime);
          }

          newItem->SetProperty("m3u8-playlistSequenceNo",(unsigned long)playlistSequenceNo);
          newItem->SetProperty("m3u8-durationInSec",(unsigned long)lDuration);
          newItem->SetProperty("m3u8-targetDurationInSec",atoi(targetDurationTag));

          if (!encryptionMethod.IsEmpty() && (encryptionMethod != "NONE"))
          {
            newItem->SetProperty("m3u8-encryptionMethod",encryptionMethod);

            if (!encryptKeyUri.IsEmpty())
            {
              newItem->SetProperty("m3u8-encryptKeyUri",encryptKeyUri);
            }

            if (!encryptIv.IsEmpty())
            {
              newItem->SetProperty("m3u8-encryptIv",encryptIv);
            }
          }

          if (!programId.IsEmpty())
          {
            newItem->SetProperty("m3u8-programId",programId);
          }

          if (!bandwidth.IsEmpty())
          {
            newItem->SetProperty("m3u8-bandwidth",bandwidth);
          }

          if (!codecs.IsEmpty())
          {
            newItem->SetProperty("m3u8-codecs",codecs);
          }

          if (nextUrlIsPlaylist || newItem->IsPlayList())
          {
            newItem->SetProperty("m3u8-isPlaylist",true);
          }

          if(discontinuity)
          {
            newItem->SetProperty("m3u8-discontinuity", true);
          }

          //CLog::Log(LOGDEBUG,"CPlayListM3U::LoadM3uFile - After set item [path=%s] with [SequenceNo=%lu]. [level=%d][m_isM3U8=%d] (m3u)",newItem->m_strPath.c_str(),newItem->GetPropertyULong("m3u8-playlistSequenceNo"),level,m_isM3U8);

          playlistSequenceNo++;
        }

        AccumulatePlaylistOverallDuration(lDuration);

        Add(newItem);

        numOfUris++;
        //CLog::Log(LOGDEBUG,"CPlayListM3U::LoadM3uFile - After adding item [path=%s] to playlist. [playlistSize=%d][level=%d][m_isM3U8=%d] (m3u)",newItem->m_strPath.c_str(),size(),level,m_isM3U8);

        // Reset the values just in case there part of the file have the extended marker
        // and part don't
        strInfo = "";
        lDuration = 0;

        bandwidth = "";
        programId = "";
        codecs = "";
        nextUrlIsPlaylist = false;
        discontinuity = false;
        readStartTime = false;

        if(hasEXTM3U == false)
        {
          CLog::Log(LOGWARNING,"CPlayListM3U::LoadM3uFile - EXTM3U tag not found (m3u)");
        }
      }
    }
  }

  file.Close();

  if (level > m_levels)
  {
    // new level was found
    m_levels = level;
  }

  CLog::Log(LOGDEBUG,"CPlayListM3U::LoadM3uFile - Finish reading file [strFileName=%s]. [level=%d][m_isM3U8=%d][numOfUris=%d][playlistOverallDuration=%lld][%lld] (m3u)", strFileName.c_str(),level,m_isM3U8,numOfUris,GetOverallLength(),m_overallLength);

  return true;
}

void CPlayListM3U::Save(const CStdString& strFileName) const
{
  if (m_isM3U8)
  {
    CLog::Log(LOGERROR,"Saving M3U8 isn't implemented. [%s]", strFileName.c_str());
    return;
  }

  if (!m_vecItems.size()) return ;
  CStdString strPlaylist = CUtil::MakeLegalPath(strFileName);
    CFile file;
  if (!file.OpenForWrite(strPlaylist,true))
  {
    CLog::Log(LOGERROR, "Could not save M3U playlist: [%s]", strPlaylist.c_str());
    return ;
  }
  CStdString strLine;
  strLine.Format("%s\n",M3U_START_MARKER);
  file.Write(strLine.c_str(),strLine.size());
  for (int i = 0; i < (int)m_vecItems.size(); ++i)
  {
    CFileItemPtr item = m_vecItems[i];
    CStdString strDescription=item->GetLabel();
    g_charsetConverter.utf8ToStringCharset(strDescription);
    strLine.Format( "%s:%i,%s\n", M3U_INFO_MARKER, item->GetMusicInfoTag()->GetDuration() / 1000, strDescription.c_str() );
    file.Write(strLine.c_str(),strLine.size());
    CStdString strFileName=item->m_strPath;
    g_charsetConverter.utf8ToStringCharset(strFileName);
    strLine.Format("%s\n",strFileName.c_str());
    file.Write(strLine.c_str(),strLine.size());
  }
  file.Close();
}

bool CPlayListM3U::ParseFirstLine(const CStdString& strLine, int lineCounter, bool& hasEXTM3U)
{
  if (lineCounter != 1)
  {
    return false;
  }

  // case of file first line -> check if it is #EXTM3U
  if (strLine.Left((int)strlen(M3U_START_MARKER)) == M3U_START_MARKER)
  {
    hasEXTM3U = true;
  }
  else
  {
    hasEXTM3U = false;
  }

  return true;
}

bool CPlayListM3U::ParseEXTINF(const CStdString& strLine, CStdString& strInfo, long& lDuration)
{
  int iColon = (int)strLine.find(":");
  int iComma = (int)strLine.find(",");
  if (iColon >= 0 && iComma >= 0 && iComma > iColon)
  {
    // Read the info and duration
    iColon++;
    CStdString strLength = strLine.Mid(iColon, iComma - iColon);
    lDuration = atoi(strLength.c_str());
    iComma++;
    strInfo = strLine.Right((int)strLine.size() - iComma);
    g_charsetConverter.unknownToUTF8(strInfo);

    //CLog::Log(LOGDEBUG,"CPlayListM3U::ParseEXTINF - After set [lDuration=%ld][strInfo=%s]. [m_isM3U8=%d] (m3u)",lDuration,strInfo.c_str(),m_isM3U8);
    return true;
  }
  else
  {
    CLog::Log(LOGERROR,"CPlayListM3U::ParseEXTINF - FAILED to parse line [%s]. [m_isM3U8=%d] (m3u)",strLine.c_str(),m_isM3U8);
    return false;
  }
}

bool CPlayListM3U::ParseEXTXTARGETDURATION(const CStdString& strLine, CStdString& targetDurationTag, bool hasEXTM3U)
{
  if (!hasEXTM3U)
  {
    CLog::Log(LOGERROR,"CPlayListM3U::ParseEXTXTARGETDURATION - File doesn't has #EXTM3U tag. [m_isM3U8=%d] (m3u)",m_isM3U8);
    return false;
  }

  m_isM3U8 = true;

  int iColon = (int)strLine.find(":");
  if (iColon >= (int)strlen(EXT_X_TARGETDURATION))
  {
    iColon++;
    targetDurationTag = strLine.Mid(iColon);

    CLog::Log(LOGDEBUG,"CPlayListM3U::ParseEXTXTARGETDURATION - After set [targetDurationTag=%s]. [m_isM3U8=%d] (m3u)",targetDurationTag.c_str(),m_isM3U8);
    return true;
  }
  else
  {
    CLog::Log(LOGERROR,"CPlayListM3U::ParseEXTXTARGETDURATION - FAILED to parse line [%s]. [m_isM3U8=%d] (m3u)",strLine.c_str(),m_isM3U8);
    return false;
  }
}

bool CPlayListM3U::ParseEXTXMEDIASEQUENCE(const CStdString& strLine, long& playlistSequenceNo, bool hasEXTM3U)
{
  if (!hasEXTM3U)
  {
    CLog::Log(LOGERROR,"CPlayListM3U::ParseEXTXTARGETDURATION - File doesn't has #EXTM3U tag. [m_isM3U8=%d] (m3u)",m_isM3U8);
    return false;
  }

  m_isM3U8 = true;

  if (playlistSequenceNo > -1)
  {
    CLog::Log(LOGERROR,"CPlayListM3U::ParseEXTXMEDIASEQUENCE - [SequenceNo=%ld] was already set. [m_isM3U8=%d] (m3u)",playlistSequenceNo,m_isM3U8);
    return false;
  }

  int iColon = (int)strLine.find(":");
  if (iColon >= (int)strlen(EXT_X_MEDIA_SEQUENCE))
  {
    iColon++;
    CStdString strSequence = strLine.Mid(iColon);
    long sequenceNo = atol(strSequence.c_str());

    if (sequenceNo < 0)
    {
      CLog::Log(LOGERROR,"CPlayListM3U::ParseEXTXMEDIASEQUENCE - Value of #EXT-X-MEDIA-SEQUENCE [%ld] < 0. [%s][m_isM3U8=%d] (m3u)",sequenceNo,strLine.c_str(),m_isM3U8);
      return false;
    }

    playlistSequenceNo = sequenceNo;

    CLog::Log(LOGDEBUG,"CPlayListM3U::ParseEXTXMEDIASEQUENCE - After set [playlistSequenceNo=%ld]. [m_isM3U8=%d] (m3u)",playlistSequenceNo,m_isM3U8);
    return true;
  }
  else
  {
    CLog::Log(LOGERROR,"CPlayListM3U::ParseEXTXMEDIASEQUENCE - FAILED to parse line [%s]. [m_isM3U8=%d] (m3u)",strLine.c_str(),m_isM3U8);
    return false;
  }
}

bool CPlayListM3U::ParseEXTXSTREAMINF(const CStdString& strLine,CStdString& bandwidth,CStdString& programId,CStdString& codecs,bool& nextUrlIsPlaylist, bool hasEXTM3U)
{
  if (!hasEXTM3U)
  {
    CLog::Log(LOGERROR,"CPlayListM3U::ParseEXTXSTREAMINF - File doesn't has #EXTM3U tag. [m_isM3U8=%d] (m3u)",m_isM3U8);
    return false;
  }

  m_isM3U8 = true;
  nextUrlIsPlaylist = true;

  int iProgramId = strLine.Find("PROGRAM-ID=");
  int iBandwidth = strLine.Find("BANDWIDTH=");
  int iCodecs = strLine.Find("CODECS=");

  int iComma = -1;

  if (iProgramId != (-1))
  {
    //////////////////////////
    // get PROGRAM-ID value //
    //////////////////////////

    iProgramId += 11;

    iComma = strLine.Find(",",iProgramId);

    if (iComma != (-1))
    {
      programId = strLine.Mid(iProgramId, iComma - iProgramId);
    }
    else
    {
      programId = strLine.Mid(iProgramId);
    }
  }

  iComma = -1;

  if (iBandwidth != (-1))
  {
    /////////////////////////
    // get BANDWIDTH value //
    /////////////////////////

    iBandwidth += 10;

    iComma = strLine.Find(",",iBandwidth);

    if (iComma != (-1))
    {
      bandwidth = strLine.Mid(iBandwidth, iComma - iBandwidth);
    }
    else
    {
      bandwidth = strLine.Mid(iBandwidth);
    }
  }

  iComma = -1;

  if (iCodecs != (-1))
  {
    //////////////////////
    // get CODECS value //
    //////////////////////

    iCodecs += 7;

    iComma = strLine.Find(",",iCodecs);

    if (iComma != (-1))
    {
      codecs = strLine.Mid(iCodecs, iComma - iBandwidth);
    }
    else
    {
      codecs = strLine.Mid(iCodecs);
    }
  }

  CLog::Log(LOGDEBUG,"CPlayListM3U::ParseEXTXSTREAMINF - After set [programId=%s][bandwidth=%s][codecs=%s]. [m_isM3U8=%d] (m3u)",programId.c_str(),bandwidth.c_str(),codecs.c_str(),m_isM3U8);

  return true;
}

bool CPlayListM3U::ParseEXTXKEY(const CStdString& strLine, CStdString& encryptionMethod, CStdString& encryptKeyUri, CStdString& encryptIv, bool hasEXTM3U)
{
  if (!hasEXTM3U)
  {
    CLog::Log(LOGERROR,"CPlayListM3U::ParseEXTXKEY - File doesn't has #EXTM3U tag. [m_isM3U8=%d] (m3u)",m_isM3U8);
    return false;
  }

  m_isM3U8 = true;

  // for test !!! //
  //CStdString line = "#EXT-X-KEY:METHOD=AES-128,URI=\"https://priv.example.com/key.php?r=53\",IV="ABCDE"";
  //CStdString strLine = line;
  //////////

  int iMethod = strLine.Find("METHOD");
  int iURI = strLine.Find(",URI");
  int iPreIvComma = strLine.Find(",IV");

  //////////////////////
  // get METHOD value //
  //////////////////////

  if (iMethod >= (int)strlen(EXT_X_KEY))
  {
    iMethod += 7;

    CStdString encryptionMethodStr;

    if (iURI != (-1))
    {
      encryptionMethodStr = strLine.Mid(iMethod, iURI - iMethod);
    }
    else
    {
      encryptionMethodStr = strLine.Mid(iMethod);
    }

    encryptionMethodStr.Trim();

    if (isEncryptionMethodValid(encryptionMethodStr))
    {
      encryptionMethod = encryptionMethodStr;
    }
    else
    {
      // ERROR log was written in isEncryptionMethodValid
      return false;
    }
  }

  ///////////////////
  // get URI value //
  ///////////////////

  if (iURI > iMethod)
  {
    iURI += 5;

    if (iPreIvComma != (-1))
    {
      encryptKeyUri = strLine.Mid(iURI, iPreIvComma - iURI);
  }
    else
    {
      encryptKeyUri = strLine.Mid(iURI);
    }

    RemoveQuotationMark(encryptKeyUri);
  }

  if (encryptionMethod != "NONE" && encryptKeyUri.IsEmpty())
  {
    CLog::Log(LOGERROR,"CPlayListM3U::ParseEXTXKEY - [encryptKeyUri=%s] is empty although [encryptionMethod=%s]. [m_isM3U8=%d] (m3u)",encryptKeyUri.c_str(),encryptionMethod.c_str(),m_isM3U8);
    return false;
  }

  //////////////////
  // get IV value //
  //////////////////

  if (iPreIvComma != -1)
  {
    iPreIvComma += 4;

    encryptIv = strLine.Mid(iPreIvComma);

    RemoveQuotationMark(encryptIv);
  }

  //CLog::Log(LOGDEBUG,"CPlayListM3U::ParseEXTXKEY - After set [encryptionMethod=%s][encryptKeyUri=%s][encryptIv=%s]. [m_isM3U8=%d] (m3u)",encryptionMethod.c_str(),encryptKeyUri.c_str(),encryptIv.c_str(),m_isM3U8);

  return true;
}

bool CPlayListM3U::isEncryptionMethodValid(const CStdString& encryptionMethod)
{
  if (encryptionMethod.IsEmpty())
  {
    CLog::Log(LOGERROR,"CPlayListM3U::isEncryptionMethodValid - [EncryptionMethod=%s] is empty (m3u)",encryptionMethod.c_str());
    return false;
  }

  if (encryptionMethod == "NONE" ||
      encryptionMethod == "AES-128")
  {
    //CLog::Log(LOGDEBUG,"CPlayListM3U::isEncryptionMethodValid - [EncryptionMethod=%s] is valid (m3u)",encryptionMethod.c_str());
    return true;
  }

  CLog::Log(LOGERROR,"CPlayListM3U::isEncryptionMethodValid - [EncryptionMethod=%s] ISN'T valid (m3u)",encryptionMethod.c_str());
  return false;
}

int64_t CPlayListM3U::GetOverallLength()
{
  return m_overallLength;
}

void CPlayListM3U::AccumulatePlaylistOverallDuration(int64_t duration)
{
  if (duration > 0)
  {
    if (m_overallLength < 0)
    {
      m_overallLength = 0;
    }

    m_overallLength += duration;
  }
}

void CPlayListM3U::RemoveQuotationMark(CStdString& text)
{
  if (text.Left(1) == "\"" && text.Right(1) == "\"")
  {
    // remove the '"' from start and end of text
    text = text.Mid(1,text.length() - 2);
  }
}

bool CPlayListM3U::CanAdd()
{
  return !m_endListTagWasRead;
}

