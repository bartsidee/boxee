

#include "BoxeeServerItemsLoader.h"
#include "BoxeeUtils.h"
#include "PictureThumbLoader.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "RssSourceManager.h"
#include "utils/log.h"

using namespace XFILE;
using namespace BOXEE;
using namespace MUSIC_INFO;

CBoxeeServerItemsLoader::CBoxeeServerItemsLoader()
{
}

CBoxeeServerItemsLoader::~CBoxeeServerItemsLoader()
{
}

bool CBoxeeServerItemsLoader::LoadItem(CFileItem* pItem, bool bCanBlock)
{
  if (!pItem) 
  {
    CLog::Log(LOGDEBUG, "CBoxeeServerItemsLoader::LoadItem, item is null (bsil)");
    return false;
  }
  
  CStdString strType = pItem->GetProperty("type");
  CStdString strBoxeeId = pItem->GetProperty("boxeeid");

  if (strBoxeeId.IsEmpty())
  {
    CLog::Log(LOGERROR,"CBoxeeServerItemsLoader::LoadItem - FAILED to load item because boxeeId is empty. [label=%s][path=%s][type=%s][boxeeId=%s] (bsil)(browse)",pItem->GetLabel().c_str(), pItem->m_strPath.c_str(), strType.c_str(), strBoxeeId.c_str());
    return false;
  }

  CLog::Log(LOGDEBUG,"CBoxeeServerItemsLoader::LoadItem - Going to resolve item [label=%s][path=%s][type=%s][boxeeId=%s] (bsil)(browse)", pItem->GetLabel().c_str(), pItem->m_strPath.c_str(), strType.c_str(), strBoxeeId.c_str());

  CFileItemList items;
  bool succeeded = false;
  if (bCanBlock) 
  {
	 succeeded = BoxeeUtils::ResolveItem(strBoxeeId, items);
  }
  if (!succeeded)
  {
    CLog::Log(LOGERROR,"CBoxeeServerItemsLoader::LoadItem - FAILED to resolve item [label=%s][path=%s][type=%s][boxeeId=%s]. [NumOfItems=%d] (bsil)(browse)", pItem->GetLabel().c_str(), pItem->m_strPath.c_str(), strType.c_str(), strBoxeeId.c_str(),items.Size());
    return false;
  }

  CLog::Log(LOGDEBUG,"CBoxeeServerItemsLoader::LoadItem - Resolve item [label=%s][path=%s][type=%s][boxeeId=%s] returned [NumOfItems=%d] (bsil)(browse)", pItem->GetLabel().c_str(), pItem->m_strPath.c_str(), strType.c_str(), strBoxeeId.c_str(),items.Size());

  //////////////////////////////////////////
  // find the match returned item by type //
  //////////////////////////////////////////

  for (int i=0; i<items.Size(); i++)
  {
    CFileItemPtr newItem = items.Get(i);

    // If the size is 1 always copy the item, if there are more than one item, only copy the one with matching type
    if (items.Size() == 1 || strType == newItem->GetProperty("boxee-mediatype"))
    {
      CLog::Log(LOGDEBUG,"CBoxeeServerItemsLoader::LoadItem - [%d/%d] - Found a match for item [label=%s][type=%s] (bsil)(browse)", i, items.Size(), pItem->GetLabel().c_str(), strType.c_str());

      //////////////////////////////////////////////////////////
      // Save important properties to copy them to a new item //
      //////////////////////////////////////////////////////////

      CStdString controlId = pItem->GetProperty("controlId");
      CStdString windowId = pItem->GetProperty("windowId");
      CStdString directoryPath = pItem->GetProperty("directoryPath");
      CStdString itemId = pItem->GetProperty("itemId");
      CStdString parentPath = pItem->GetProperty("parentPath");

      // Copy the newly retrieved item over the old one
      *pItem = *newItem;

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
    }
  }

  // In any case, activate the picture loader to retreive the thumb
  CPictureThumbLoader loader;
  bool bResult = loader.LoadItem(pItem, bCanBlock);
  return bResult;
};

