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

#include "FileSystem/StackDirectory.h"
#include "ThumbLoader.h"
#include "Util.h"
#include "URL.h"
#include "Picture.h"
#include "FileSystem/File.h"
#include "FileItem.h"
#include "GUISettings.h"
#include "TextureManager.h"
#include "VideoInfoTag.h"
#include "MusicInfoTag.h"
#include "utils/log.h"
#include "utils/log.h"

#include "cores/dvdplayer/DVDFileInfo.h"

using namespace XFILE;
using namespace DIRECTORY;

CThumbLoader::CThumbLoader(int nThreads) :
  CBackgroundInfoLoader(nThreads)
{
}

CThumbLoader::~CThumbLoader()
{
}

bool CThumbLoader::LoadRemoteThumb(CFileItem *pItem)
{
  // look for remote thumbs
  CStdString thumb(pItem->GetThumbnailImage());
  if (!g_TextureManager.CanLoad(thumb))
  {
    CStdString cachedThumb(pItem->GetCachedVideoThumb());
    if (CFile::Exists(cachedThumb))
      pItem->SetThumbnailImage(cachedThumb);
    else
    {
      if (CPicture::CreateThumbnail(thumb, cachedThumb))
        pItem->SetThumbnailImage(cachedThumb);
      else
        pItem->SetThumbnailImage("");
    }
  }
  return pItem->HasThumbnail();
}

CVideoThumbLoader::CVideoThumbLoader() : 
  CThumbLoader(), m_pStreamDetailsObs(NULL)
{
}

CVideoThumbLoader::~CVideoThumbLoader()
{
  StopThread();
}

void CVideoThumbLoader::OnLoaderStart()
{
}

void CVideoThumbLoader::OnLoaderFinish()
{
}

bool CVideoThumbLoader::ExtractThumb(const CStdString &strPath, const CStdString &strTarget, CStreamDetails *pStreamDetails)
{
  if (!g_guiSettings.GetBool("myvideos.autothumb"))
    return false;

  CStdString strExt;
  CUtil::GetExtension(strPath, strExt);

  if (CUtil::IsLiveTV(strPath)
#ifndef HAS_UPNP_AV
  ||  CUtil::IsUPnP(strPath)
#endif
  ||  CUtil::IsDAAP(strPath)
    || strExt.ToLower() == ".swf")
    return false;

  CLog::Log(LOGDEBUG,"%s - trying to extract thumb from video file %s", __FUNCTION__, strPath.c_str());
  return CDVDFileInfo::ExtractThumb(strPath, strTarget, pStreamDetails);
}

bool CVideoThumbLoader::LoadItem(CFileItem* pItem, bool bCanBlock)
{
  if (pItem->m_bIsShareOrDrive) return false;

  bool retVal = false;
  if (pItem->IsVideoDb() && pItem->HasVideoInfoTag() && !pItem->HasThumbnail())
  {
    if (pItem->m_bIsFolder && pItem->GetVideoInfoTag()->m_iSeason > -1)
      return false;
    CFileItem item(*pItem->GetVideoInfoTag());
    bool bResult = LoadItem(&item, bCanBlock);
    if (bResult)
    {
      pItem->SetProperty("HasAutoThumb",item.GetProperty("HasAutoThumb"));
      pItem->SetProperty("AutoThumbImage",item.GetProperty("AutoThumbImage"));
      pItem->SetProperty("fanart_image",item.GetProperty("fanart_image"));
      pItem->SetThumbnailImage(item.GetThumbnailImage());
      pItem->GetVideoInfoTag()->m_streamDetails = item.GetVideoInfoTag()->m_streamDetails;
    }
    return bResult;
  }

  CStdString cachedThumb(pItem->GetCachedVideoThumb());

  CLog::Log(LOGDEBUG, "CVideoThumbLoader::LoadItem, strItemPath = %s, cachedThumb = %s (thumb)", pItem->m_strPath.c_str(), cachedThumb.c_str());

  if (!pItem->HasThumbnail())
  {
    if (CFile::Exists(cachedThumb))
    {
      CLog::Log(LOGDEBUG, "CVideoThumbLoader::LoadItem, EXISTS, strItemPath = %s, cachedThumb = %s (thumb)", pItem->m_strPath.c_str(), cachedThumb.c_str());
      pItem->SetCachedVideoThumb();
    }
    else if (pItem->m_bIsFolder)
      pItem->SetUserVideoThumb();
    else
    {
      CStdString strPath, strFileName;
      CUtil::Split(cachedThumb, strPath, strFileName);

      // create unique thumb for auto generated thumbs
      cachedThumb = strPath + "auto-" + strFileName;
      if (pItem->IsVideo() && !pItem->IsInternetStream() && !pItem->IsPlayList() && !CFile::Exists(cachedThumb) && !pItem->m_bIsFolder)
      {
        if (!bCanBlock) 
        {
          // we should not retreive remote (e.g. SMB) thumbs if requested not to block
          return false;
        }
        
        CStreamDetails details;
        if (pItem->IsStack())
        {
          CStackDirectory stack;
          CVideoThumbLoader::ExtractThumb(stack.GetFirstStackedFile(pItem->m_strPath), cachedThumb, &details);
        }
        else
        {
          CVideoThumbLoader::ExtractThumb(pItem->m_strPath, cachedThumb, &details);
        }

        if (details.HasItems() && m_pStreamDetailsObs)
          m_pStreamDetailsObs->OnStreamDetails(details, pItem->m_strPath, -1);
      }

      if (CFile::Exists(cachedThumb))
      {
        pItem->SetProperty("HasAutoThumb", "1");
        pItem->SetProperty("AutoThumbImage", cachedThumb);
        pItem->SetThumbnailImage(cachedThumb);
        retVal = true;
      }
      else
        pItem->SetThumbnailImage("");
    }
  }
  else
  {
    // look for remote thumbs
    CStdString thumb(pItem->GetThumbnailImage());
    if (!CURI::IsFileOnly(thumb) && !CUtil::IsHD(thumb))
    {
      if (pItem->GetProperty("OriginalThumb").IsEmpty())
      {
        //when the item is loaded from the database we don't want to overwrite the original thumb by mistake because the user might have his own thumb
        //related to http://jira.boxee.tv/browse/BOXEE-8488
        pItem->SetProperty("OriginalThumb", thumb);
      }

      if(CFile::Exists(cachedThumb))
      {
        pItem->SetThumbnailImage(cachedThumb);
        retVal = true;
      }
      else
      {
        if (!bCanBlock) 
        {
          // we should not retreive remote thumbs if requested not to block
          return false;
        }
        
        if(CPicture::CreateThumbnail(thumb, cachedThumb))
          pItem->SetThumbnailImage(cachedThumb);
        else
          pItem->SetThumbnailImage("");
      }
    }
    else
    {
      if (!CFile::Exists(cachedThumb))
      {
        // Thumb can not be found. Going to create the thumb by fetching the original thumb and save it in cache
        if (!bCanBlock) 
        {
          if (pItem->GetProperty("OriginalThumb").IsEmpty())
          {
            pItem->SetProperty("OriginalThumb", thumb);
          }
          return false;
        }

        CStdString originalThumb = pItem->GetProperty("OriginalThumb");

        if (!pItem->GetThumbnailImage().IsEmpty() && (CUtil::IsHD(pItem->GetThumbnailImage()) || CUtil::IsSmb(pItem->GetThumbnailImage()) || CUtil::IsUPnP(pItem->GetThumbnailImage())) && CFile::Exists(pItem->GetThumbnailImage()))
        {
          //if the user has the thumb locally, use it
          originalThumb = pItem->GetThumbnailImage();
        }

        CStdString newCachedThumb = pItem->GetCachedPictureThumb();

        if(CPicture::CreateThumbnail(originalThumb, newCachedThumb,true ))
        {
          pItem->SetThumbnailImage(newCachedThumb);
        }
        else
        {
          pItem->SetThumbnailImage("");
        }
      }
    }
  }

  if (!pItem->HasProperty("fanart_image") && bCanBlock)
  {
    pItem->CacheFanart();
    if (CFile::Exists(pItem->GetCachedFanart()))
    {
      pItem->SetProperty("fanart_image",pItem->GetCachedFanart());
      retVal = true;
  }                          
  }

  if (!pItem->m_bIsFolder && !pItem->IsInternetStream() && 
       pItem->HasVideoInfoTag() && 
       g_guiSettings.GetBool("myvideos.extractflags")   &&
       !pItem->GetVideoInfoTag()->HasStreamDetails())
  {
    if (CDVDFileInfo::GetFileStreamDetails(pItem) && m_pStreamDetailsObs)
    {
      CVideoInfoTag *info = pItem->GetVideoInfoTag();
      m_pStreamDetailsObs->OnStreamDetails(info->m_streamDetails, "", info->m_iFileId);
      pItem->SetInvalid();
      retVal = true;
    }
  }

//  if (pItem->IsVideo() && !pItem->IsInternetStream())
//    CDVDPlayer::GetFileMetaData(pItem->m_strPath, pItem);

  return retVal;
}

CProgramThumbLoader::CProgramThumbLoader()
{
}

CProgramThumbLoader::~CProgramThumbLoader()
{
}

bool CProgramThumbLoader::LoadItem(CFileItem *pItem, bool bCanBlock)
{
  if (pItem->m_bIsShareOrDrive) return true;
  if (!pItem->HasThumbnail())
    pItem->SetUserProgramThumb();
  else
    LoadRemoteThumb(pItem);
  return true;
}

CMusicThumbLoader::CMusicThumbLoader()
{
}

CMusicThumbLoader::~CMusicThumbLoader()
{
}

bool CMusicThumbLoader::LoadItem(CFileItem* pItem, bool bCanBlock)
{
  if (pItem->m_bIsShareOrDrive) return true;
  
  // Path to the thumb in the cache
  CStdString cachedThumb;

  // We calculate the path of cached thumb by either album and artist or by item path
  if (pItem->HasMusicInfoTag() && (!pItem->GetMusicInfoTag()->GetArtist().IsEmpty() || !pItem->GetMusicInfoTag()->GetAlbumArtist().IsEmpty())
      && pItem->GetMusicInfoTag()->GetAlbum() != "")
  {
    CStdString strAlbum = pItem->GetMusicInfoTag()->GetAlbum();
    CStdString strArtist = pItem->GetMusicInfoTag()->GetArtist();
    CStdString strAlbumArtist = pItem->GetMusicInfoTag()->GetAlbumArtist();

    CLog::Log(LOGDEBUG,"CMusicThumbLoader::LoadItem, album = %s, artist = %s, album artist = %s (musicthumb)",
        strAlbum.c_str(), strArtist.c_str(), strAlbumArtist.c_str());

    cachedThumb = CUtil::GetCachedAlbumThumb(strAlbum, strAlbumArtist.IsEmpty() ? strArtist : strAlbumArtist);
  }
  else
  {
    CLog::Log(LOGDEBUG,"CMusicThumbLoader::LoadItem, item path = %s (musicthumb)", pItem->m_strPath.c_str());

    cachedThumb = CUtil::GetCachedMusicThumb(pItem->m_strPath);
  }

  CLog::Log(LOGDEBUG,"CMusicThumbLoader::LoadItem, cached thumb = %s (musicthumb)", cachedThumb.c_str());

  if(CFile::Exists(cachedThumb))
  {
    pItem->SetThumbnailImage(cachedThumb);
    return true;
  }
  
  // We have thumbnail path, try to retreive and cache the thumbnail 
  if (pItem->HasThumbnail())
  {
    CStdString thumb(pItem->GetThumbnailImage());

    CLog::Log(LOGDEBUG,"CMusicThumbLoader::LoadItem, retrieve remote thumb = %s (musicthumb)", thumb.c_str());

    if (!CURI::IsFileOnly(thumb) && !CUtil::IsHD(thumb))
    {
      // Path points to remote location, save it first
      pItem->SetProperty("OriginalThumb", thumb);
      
        // We need to get the file from remote location
        if (bCanBlock)
        {
          if(CPicture::CreateThumbnail(thumb, cachedThumb))
          {
          CLog::Log(LOGDEBUG,"CMusicThumbLoader::LoadItem, set thumb = %s (musicthumb)", cachedThumb.c_str());
            pItem->SetThumbnailImage(cachedThumb);
          }
          return true;
        }
        else 
        {
          // we could not get thumb with local means and we should not block, return false
          return false;
        }
      }
    else
    {
      CLog::Log(LOGDEBUG,"CMusicThumbLoader::LoadItem, retrieve local thumb = %s (musicthumb)", thumb.c_str());
      // TODO: Check if this is an already cached thumb that got into the database by mistake
      if(CPicture::CreateThumbnail(thumb, cachedThumb,true ))
      {
        CLog::Log(LOGDEBUG,"CMusicThumbLoader::LoadItem, set thumb = %s (musicthumb)", cachedThumb.c_str());
        pItem->SetThumbnailImage(cachedThumb);
      }
      return true;
    }
  }
  else
  {
    return true;
  }
  
   return false;
}

