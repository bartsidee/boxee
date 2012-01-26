
#include "BoxeeFeedItemsLoader.h"
#include "Picture.h"
#include "Util.h"
#include "lib/libBoxee/bxobject.h"
#include "lib/libBoxee/bxmessages.h"
#include "lib/libBoxee/bxutils.h"
#include "BoxeeUtils.h"
#include "PictureThumbLoader.h"
#include "VideoInfoTag.h"
#include "MusicInfoTag.h"
#include "MetadataResolver.h"
#include "MetadataResolverMusic.h"
#include "MetadataResolverVideo.h"
#include "FileSystem/StackDirectory.h"
#include "FileSystem/BoxeeDatabaseDirectory.h"

using namespace XFILE;
using namespace BOXEE;
using namespace MUSIC_INFO;

CBoxeeFeedItemsLoader::CBoxeeFeedItemsLoader()
{
}

CBoxeeFeedItemsLoader::~CBoxeeFeedItemsLoader()
{
}

bool CBoxeeFeedItemsLoader::LoadItem(CFileItem* pItem, bool bCanBlock)
{
  if (!bCanBlock)
    return false;
  
  if (!pItem) 
  {
    CLog::Log(LOGDEBUG, "CBoxeeFeedItemsLoader::LoadItem, item is null (feedloader) (ri)");
    return false;
  }

  //CLog::Log(LOGDEBUG, "CBoxeeFeedItemsLoader::LoadItem, going to resolve item label =%s (feedloader) (ri)", pItem->GetLabel().c_str());

  bool succeeded = true;

  if (pItem->HasProperty("boxeeId"))
  {
    CFileItemList items;
    CStdString strBoxeeId = pItem->GetProperty("boxeeId");

    succeeded = BoxeeUtils::BuildItemInfo(*pItem,items,true);//BoxeeUtils::ResolveItem(strBoxeeId, items);

    //CLog::Log(LOGDEBUG, "CBoxeeFeedItemsLoader::LoadItem, going to resolve item for boxee id = %s, label =%s (feedloader) (ri)", strBoxeeId.c_str(), pItem->GetLabel().c_str());
    if (succeeded)
    {
      //CLog::Log(LOGDEBUG, "CBoxeeFeedItemsLoader::LoadItem, resolved item for boxee id = %s, label =%s (feedloader) (ri)", strBoxeeId.c_str(), pItem->GetLabel().c_str());

      CFileItemPtr resolvedItem = items.Get(0);

      //////////////////////////////////////////////////////////
      // Save important properties to copy them to a new item //
      //////////////////////////////////////////////////////////

      CStdString controlId = pItem->GetProperty("controlId");
      CStdString windowId = pItem->GetProperty("windowId");
      CStdString directoryPath = pItem->GetProperty("directoryPath");
      CStdString itemId = pItem->GetProperty("itemId");
      CStdString parentPath = pItem->GetProperty("parentPath");
      CStdString feedDesc = pItem->GetProperty("feeddesc");
      CStdString formattedDesc = pItem->GetProperty("formatteddesc");
      CStdString originalDesc = pItem->GetProperty("originaldesc");
      CStdString userId = pItem->GetProperty("user_id");
      CStdString userName = pItem->GetProperty("user_name");
      CStdString userThumb = pItem->GetProperty("user_thumb");
      CStdString thumbsUp = pItem->GetProperty("thumbsUp");
      CStdString userMessage = pItem->GetProperty("user_message");
      CStdString referral = pItem->GetProperty("referral");
      CStdString feedSource = pItem->GetProperty("feedsource");
      CStdString itemThumb = pItem->GetThumbnailImage();
      CFileItemPtr nextItem = pItem->GetNextItem();
      CStdString nextItemLabel = pItem->GetProperty("NextItemLabel");
      CStdString nextItemLabel2 = pItem->GetProperty("NextItemLabel2");
      CStdString nextItemPath = pItem->GetProperty("NextItemPath");
      CStdString hasNextItem = pItem->GetProperty("HasNextItem");
      CFileItemPtr prevItem = pItem->GetPrevItem();
      CStdString prevItemLabel = pItem->GetProperty("PrevItemLabel");
      CStdString prevItemLabel2 = pItem->GetProperty("PrevItemLabel2");
      CStdString prevItemPath = pItem->GetProperty("PrevItemPath");
      CStdString hasPrevItem = pItem->GetProperty("HasPrevItem");
      CStdString addToDiscoverDesc = pItem->GetProperty("addToDiscoverDesc");
      CStdString addToQueueDesc = pItem->GetProperty("addToQueueDesc");
      CStdString isSubscribe = pItem->GetProperty("IsSubscribe");
      CStdString isFeedItem = pItem->GetProperty("IsFeedItem");
      CStdString feedTypeItem = pItem->GetProperty("FeedTypeItem");

      CStdString label = "";
      CStdString thumb = "";
      CStdString desc = "";
      CStdString Plot = "";
      CStdString PlotOutline = "";
      bool isFeaturedItem = (feedTypeItem == MSG_ACTION_TYPE_FEATURED);
      if (isFeaturedItem)
      {
        label = pItem->GetLabel();
        thumb = pItem->GetThumbnailImage();
        desc = pItem->GetProperty("description");

        CVideoInfoTag* videoInfoTag = pItem->GetVideoInfoTag();
        if (videoInfoTag)
        {
          Plot = videoInfoTag->m_strPlot;
          PlotOutline = videoInfoTag->m_strPlotOutline;
        }
      }

      *pItem = *(resolvedItem.get());

      ///////////////////////////
      // copy saved properties //
      ///////////////////////////

      if (!controlId.IsEmpty())
      {
        pItem->SetProperty("controlId", controlId);
      }

      if (!windowId.IsEmpty())
      {
        pItem->SetProperty("windowId", windowId);
      }

      if (!directoryPath.IsEmpty())
      {
        pItem->SetProperty("directoryPath", directoryPath);
      }

      if (!itemId.IsEmpty())
      {
        pItem->SetProperty("itemId", itemId);
      }

      if (!parentPath.IsEmpty())
      {
        pItem->SetProperty("parentPath", parentPath);
      }

      if (!feedDesc.IsEmpty())
      {
      pItem->SetProperty("feeddesc", feedDesc);
      }

      if (!formattedDesc.IsEmpty())
      {
      pItem->SetProperty("formatteddesc", formattedDesc);
      }

      if (!originalDesc.IsEmpty())
      {
        pItem->SetProperty("originaldesc", originalDesc);
      }

      if (!userId.IsEmpty())
      {
        pItem->SetProperty("user_id", userId);
      }

      if (!userName.IsEmpty())
      {
        pItem->SetProperty("user_name", userName);
      }

      if (!userThumb.IsEmpty())
      {
        pItem->SetProperty("user_thumb", userThumb);
      }

      if (!thumbsUp.IsEmpty())
      {
        pItem->SetProperty("thumbsUp", thumbsUp);
      }

      if (!userMessage.IsEmpty())
      {
        pItem->SetProperty("user_message", userMessage);
      }

      if (!referral.IsEmpty())
      {
        pItem->SetProperty("referral", referral);
      }

      if (!feedSource.IsEmpty())
      {
        pItem->SetProperty("feedsource", feedSource);
        pItem->SetProperty("feedsource-lower",feedSource.ToLower());
      }

      // in case after resolve there is no thumb and the original feed item had a thumb -> set the the original feed item thumb
      if (pItem->GetThumbnailImage().IsEmpty() && !itemThumb.IsEmpty())
      {
        pItem->SetThumbnailImage(itemThumb);
      }

      if (nextItem.get())
      {
        pItem->SetNextItem(nextItem);
      }

      if (!nextItemLabel.IsEmpty())
      {
        pItem->SetProperty("NextItemLabel", nextItemLabel);
      }

      if (!nextItemLabel2.IsEmpty())
      {
        pItem->SetProperty("NextItemLabel2", nextItemLabel2);
      }

      if (!nextItemPath.IsEmpty())
      {
        pItem->SetProperty("NextItemPath", nextItemPath);
      }

      if (!hasNextItem.IsEmpty())
      {
        pItem->SetProperty("HasNextItem", hasNextItem);
      }

      if (prevItem.get())
      {
        pItem->SetPrevItem(prevItem);
      }

      if (!prevItemLabel.IsEmpty())
      {
        pItem->SetProperty("PrevItemLabel", prevItemLabel);
      }

      if (!prevItemLabel2.IsEmpty())
      {
        pItem->SetProperty("PrevItemLabel2", prevItemLabel2);
      }

      if (!prevItemPath.IsEmpty())
      {
        pItem->SetProperty("PrevItemPath", prevItemPath);
      }

      if (!hasPrevItem.IsEmpty())
      {
        pItem->SetProperty("HasPrevItem", hasPrevItem);
      }

      if (!addToDiscoverDesc.IsEmpty())
      {
        pItem->SetProperty("addToDiscoverDesc", addToDiscoverDesc);
      }

      if (!addToQueueDesc.IsEmpty())
      {
        pItem->SetProperty("addToQueueDesc", addToQueueDesc);
      }

      if (!isSubscribe.IsEmpty())
      {
        pItem->SetProperty("IsSubscribe", isSubscribe);
      }

      if (!isFeedItem.IsEmpty())
      {
        pItem->SetProperty("IsFeedItem", isFeedItem);
      }

      if (!feedTypeItem.IsEmpty())
      {
        pItem->SetProperty("FeedTypeItem", feedTypeItem);
      }

      if (isFeaturedItem)
      {
        if (!label.IsEmpty())
        {
          pItem->SetLabel(label);
        }

        if (!thumb.IsEmpty())
        {
          pItem->SetProperty("ResolveThumb",pItem->GetThumbnailImage());
          pItem->SetThumbnailImage(thumb);
        }

        if (!desc.IsEmpty())
        {
          pItem->SetProperty("description",desc);
        }

        CVideoInfoTag* videoInfoTag = pItem->GetVideoInfoTag();
        if (!Plot.IsEmpty() && videoInfoTag)
        {
          videoInfoTag->m_strPlot = Plot;
        }

        if (!PlotOutline.IsEmpty() && videoInfoTag)
        {
          videoInfoTag->m_strPlotOutline = PlotOutline;
        }
      }
    }
    else
    {
      CLog::Log(LOGERROR, "CBoxeeFeedItemsLoader::LoadItem - FAILED to resolve item with [boxeeId=%s] (feedloader)(ri)", strBoxeeId.c_str());
    }
  }
  else if (pItem->HasProperty("isfeedalbum"))
  {
    HandleAlbum(pItem, bCanBlock);
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeFeedItemsLoader::LoadItem - Not handling item [label=%s][showid=%s][boxeeid=%s][isfeedalbum=%s][referral=%s] (feedloader)(ri)",pItem->GetLabel().c_str(),pItem->GetProperty("showId").c_str(),pItem->GetProperty("boxeeId").c_str(),pItem->GetProperty("isfeedalbum").c_str(),pItem->GetProperty("referral").c_str());
  }

  if (succeeded)
  {
    // In case the item has CustomButtons we want to load the remote thumb
    // and replace the path to the loaded one (local)
    LoadCustomButtons(pItem,true);
  }

  // In any case, activate the picture loader to retreive the thumb
  CPictureThumbLoader loader;
  succeeded &= loader.LoadItem(pItem, bCanBlock);
  return succeeded;
};

bool CBoxeeFeedItemsLoader::HandleAlbum(CFileItem* pItem, bool bCanBlock)
{
  CStdString strAlbum  = pItem->GetProperty("feedAlbum");
  CStdString strArtist = pItem->GetProperty("feedArtist");
  CStdString strThumb  = pItem->GetThumbnailImage();

  BXMetadata metadata(MEDIA_ITEM_TYPE_AUDIO);
  BXAlbum* pAlbum = (BXAlbum*) metadata.GetDetail(MEDIA_DETAIL_ALBUM);
  BXArtist* pArtist = (BXArtist*) metadata.GetDetail(MEDIA_DETAIL_ARTIST);

  CLog::Log(LOGDEBUG, "CBoxeeFeedItemsLoader::HandleAlbum, load feed album <%s> of artist <%s> (feedloader)", strAlbum.c_str(), strArtist.c_str());


  if (BOXEE::Boxee::GetInstance().GetMetadataEngine().GetAlbum(strAlbum, strArtist, &metadata) != MEDIA_DATABASE_OK)
  {
    if (!bCanBlock)
      return false;

    // We should resolve the album
    CLog::Log(LOGDEBUG, "CBoxeeFeedItemsLoader::HandleAlbum, attempting to resolve, album <%s> of artist <%s> (feedloader)", strAlbum.c_str(), strArtist.c_str());

    bool bResult = CMetadataResolverMusic::ResolveAlbumMetadata(strAlbum, strArtist, &metadata);
    if (bResult)
    {
      CLog::Log(LOGDEBUG, "CBoxeeFeedItemsLoader::HandleAlbum, resolved, album <%s> of artist <%s> (feedloader)", strAlbum.c_str(), strArtist.c_str());

      pArtist->m_strName = strArtist;

      if (!BXUtils::GetArtistData(strArtist, pArtist))
      {
        CLog::Log(LOGWARNING,"CBoxeeFeedItemsLoader::HandleAlbum, Could not load artist data from network. %s (feedloader)", strArtist.c_str());
      }

      pAlbum->m_iArtistId = BOXEE::Boxee::GetInstance().GetMetadataEngine().AddArtist(pArtist);

      // Mark album as virtual
      pAlbum->m_iVirtual = 1;
      pAlbum->m_iId = BOXEE::Boxee::GetInstance().GetMetadataEngine().AddAlbum(pAlbum);
    }
    else
    {
      CLog::Log(LOGDEBUG, "CBoxeeFeedItemsLoader::HandleAlbum, could not resolve album <%s> by artist <%s> (feedloader)", strAlbum.c_str(), strArtist.c_str());

      pItem->GetMusicInfoTag()->SetAlbum(strAlbum);
      pItem->GetMusicInfoTag()->SetArtist(strArtist);

      return false;
    }
  }
  else
  {
    // We have this album, add local path
  }

  DIRECTORY::CBoxeeDatabaseDirectory::CreateAlbumItem(&metadata, pItem);

  pItem->m_strPath = "boxeedb://album/?id=";
  pItem->m_strPath += BXUtils::IntToString(pAlbum->m_iId);
  pItem->m_strPath += "/";

  return true;

}

