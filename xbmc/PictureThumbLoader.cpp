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

#include "PictureThumbLoader.h"
#include "Picture.h"
#include "URL.h"
#include "FileSystem/File.h"
#include "FileItem.h"
#include "VideoInfoTag.h"
#include "TextureManager.h"
#include "utils/log.h"
#include "Util.h"

using namespace XFILE;

CPictureThumbLoader::CPictureThumbLoader()
{
  m_regenerateThumbs = false;
}

CPictureThumbLoader::~CPictureThumbLoader()
{
  StopThread();
}

bool CPictureThumbLoader::LoadItem(CFileItem* pItem, bool bCanBlock)
{
  pItem->SetCachedPictureThumb();
  if (pItem->m_bIsShareOrDrive) return true;
  if(pItem->HasThumbnail())
  {
    CStdString thumb(pItem->GetThumbnailImage());
    
    // small hack for the case that our thumb cache got erased. we make sure the images are loaded
    if (thumb.Right(4) == ".tbn" && CUtil::IsHD(thumb))
    {
      bool bExists = CFile::Exists(thumb);
      if (!bExists && pItem->HasProperty("OriginalThumb") && !pItem->GetProperty("OriginalThumb").IsEmpty())
      {
        pItem->SetThumbnailImage(pItem->GetProperty("OriginalThumb"));
        pItem->SetCachedPictureThumb();
        thumb = pItem->GetThumbnailImage();
        pItem->SetProperty("needsloading",true);
      }
    }
    
    // look for remote thumbs
    if (!g_TextureManager.CanLoad(thumb))
    {     
      pItem->SetProperty("OriginalThumb", thumb);
      CStdString cachedThumb(pItem->GetCachedPictureThumb());
      if(CFile::Exists(cachedThumb))
      {
        pItem->SetThumbnailImage(cachedThumb);
      }
      else if (bCanBlock)
      {
        // see if we have additional info to download this thumb with
        if (pItem->HasVideoInfoTag())
        {
          if (!DownloadVideoThumb(pItem, cachedThumb))
          {
            if(CPicture::CreateThumbnail(thumb, cachedThumb))
              pItem->SetThumbnailImage(cachedThumb);
            else
              pItem->SetThumbnailImage("");
          }
        }
        else
        {
          if (CPicture::CreateThumbnail(thumb, cachedThumb))
            pItem->SetThumbnailImage(cachedThumb);
          else
            pItem->SetThumbnailImage("");
        }
      }
      else
      {
        return false;
      }
    }
    else if (m_regenerateThumbs)
    {
      CFile::Delete(thumb);
      pItem->SetThumbnailImage("");
    }
  }

  if ((pItem->IsPicture() && !pItem->IsZIP() && !pItem->IsRAR() && !pItem->IsCBZ() && !pItem->IsCBR() ) && !pItem->HasThumbnail())
  { 
    // load the thumb from the image file
    if (bCanBlock)
    {
      CPicture::CreateThumbnail(pItem->m_strPath, pItem->GetCachedPictureThumb());
    }
    else 
    {
      return false;
    }
  }

  if(pItem->m_bIsFolder && (pItem->IsHD() || (pItem->IsSmb() || (pItem->IsApp()))))
  {
    // In case the item is a folder,
    //
    // 1) If we didn't enter it (we are currently in CPictureThumbLoader because we browsing its parent folder) 
    //    we don't want to create a thumbnail for it -> so just return TRUE (The thumbnail for this folder will be
    //    created when we enter the folder with GetDirectory() call)
    //
    // 2) If we have already entered this folder (we are currently in CPictureThumbLoader because we are back to its parent folder)
    //    its thumbnail should be created already -> so we need to set it.

    CStdString folderCacheThumbPath = pItem->GetCachedPictureThumb();
    
    CLog::Log(LOGDEBUG,"CPictureThumbLoader::LoadItem - Item [label=%s][label2=%s][path=%s] is a folder. [folderCacheThumbPath=%s] (foldert)",pItem->GetLabel().c_str(),pItem->GetLabel2().c_str(),pItem->m_strPath.c_str(),folderCacheThumbPath.c_str());

    if(CFile::Exists(folderCacheThumbPath))
    {
      pItem->SetThumbnailImage(folderCacheThumbPath);
      CLog::Log(LOGDEBUG,"CPictureThumbLoader::LoadItem - CahceFolderThumb for item [label=%s][label2=%s][path=%s] exist and was set as ThumbnailImage [ItemThumbnailImage=%s] (foldert)",pItem->GetLabel().c_str(),pItem->GetLabel2().c_str(),pItem->m_strPath.c_str(),pItem->GetThumbnailImage().c_str());
    }
    else
    {
      CLog::Log(LOGDEBUG,"CPictureThumbLoader::LoadItem - CahceFolderThumb for item [label=%s][label2=%s][path=%s] DOESN'T exist. [OriginalThumb=%s] (foldert)",pItem->GetLabel().c_str(),pItem->GetLabel2().c_str(),pItem->m_strPath.c_str(),pItem->GetProperty("OriginalThumb").c_str());
    }
    
    CLog::Log(LOGDEBUG,"CPictureThumbLoader::LoadItem - For item [label=%s][label2=%s][path=%s] going to return TRUE [Not queue for background loading] (foldert)",pItem->GetLabel().c_str(),pItem->GetLabel2().c_str(),pItem->m_strPath.c_str());
    return true;
  }

  //////////////////////////////////////////////////////
  // load general extra images that the item may have //
  //////////////////////////////////////////////////////
  
  if(bCanBlock)
  {
    LoadCustomImages(pItem,true);
    LoadCustomButtons(pItem,true);
    LoadItemLinks(pItem,true);
  }
  else
  {
    return false;
  }
  
  // refill in the thumb to get it to update
  pItem->SetCachedPictureThumb();
  pItem->FillInDefaultIcon();

  return true;
}

void CPictureThumbLoader::OnLoaderFinish()
{
  m_regenerateThumbs = false;
}

bool CPictureThumbLoader::DownloadVideoThumb(CFileItem *item, const CStdString &cachedThumb)
{
  if (item->GetVideoInfoTag()->m_strPictureURL.m_url.size())
  { // yep - download using this thumb
    if (CScraperUrl::DownloadThumbnail(cachedThumb, item->GetVideoInfoTag()->m_strPictureURL.m_url[0]))
      item->SetThumbnailImage(cachedThumb);
    else
      item->SetThumbnailImage("");
  }
  else if (item->GetVideoInfoTag()->m_fanart.GetNumFanarts() > 0 && item->HasProperty("fanart_number"))
  { // yep - download our fanart preview
    if (item->GetVideoInfoTag()->m_fanart.DownloadThumb(item->GetPropertyInt("fanart_number"), cachedThumb))
      item->SetThumbnailImage(cachedThumb);
    else
      item->SetThumbnailImage("");
  }
  else
  {
    return false;
  }
  
  if (item->HasProperty("labelonthumbload"))
    item->SetLabel(item->GetProperty("labelonthumbload"));
  return true; // we don't need to do anything else here
}
