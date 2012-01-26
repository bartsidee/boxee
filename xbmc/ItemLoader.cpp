
#include "ItemLoader.h"
#include "FileSystem/Directory.h"
#include "GUIWindowManager.h"
#include "BoxeeFeedItemsLoader.h"
#include "BoxeeServerItemsLoader.h"
#include "PictureThumbLoader.h"
#include "FileSystem/DirectoryCache.h"
#include "ThumbLoader.h"
#include "utils/SingleLock.h"
#include "Util.h"
#include "cores/dvdplayer/DVDFileInfo.h"
#include "Application.h"
#include "MetadataResolverVideo.h"
#include "FileSystem/File.h"
#include "FileSystem/FileCurl.h"
#include "VideoDatabase.h"
#include "lib/libBoxee/boxee.h"
#include "utils/TimeUtils.h"
#include "BoxeeUtils.h"

#define NUM_ITEM_LOADER_THREADS 5
#define NUM_DIRECTORY_LOADER_THREADS 5

using namespace DIRECTORY;
using namespace XFILE;

CGetDirectoryJob::CGetDirectoryJob(const CStdString& strPath, int iWindowId, int iControlId, IItemLoaderCallback* pCallback) : BXBGJob("CGetDirectoryJob")
{
  m_pCallback = pCallback;
  m_strPath = strPath;
  m_iWindowId = iWindowId;
  m_iControlId = iControlId;
  m_iSortMethod = SORT_METHOD_NONE;
  m_iSortOrder = SORT_ORDER_NONE;
  m_iSelectedItem = -1;
}

CGetDirectoryJob::CGetDirectoryJob(const CStdString& strPath, int iWindowId, int iControlId, IItemLoaderCallback* pCallback, 
    SORT_METHOD iSortMethod, SORT_ORDER iSortOrder, int iSelectedItem) : BXBGJob("CGetDirectoryJob")
{
  m_pCallback = pCallback;
  m_strPath = strPath;
  m_iWindowId = iWindowId;
  m_iControlId = iControlId;
  m_iSortMethod = iSortMethod;
  m_iSortOrder = iSortOrder;
  m_iSelectedItem = iSelectedItem;
}

CGetDirectoryJob::CGetDirectoryJob(CFileItemPtr pItem, int iWindowId, int iControlId, IItemLoaderCallback* pCallback) : BXBGJob("CGetDirectoryJob")
{
  m_pCallback = pCallback;
  m_pItem = pItem;
  m_strPath = pItem->m_strPath;
  m_iWindowId = iWindowId;
  m_iControlId = iControlId;
  m_iSortMethod = SORT_METHOD_NONE;
  m_iSortOrder = SORT_ORDER_NONE;
  m_iSelectedItem = -1;
}

CGetDirectoryJob::CGetDirectoryJob(CFileItemPtr pItem, int iWindowId, int iControlId, IItemLoaderCallback* pCallback, 
    SORT_METHOD iSortMethod, SORT_ORDER iSortOrder, int iSelectedItem) : BXBGJob("CGetDirectoryJob")
{
  m_pCallback = pCallback;
  m_pItem = pItem;
  m_strPath = pItem->m_strPath;
  m_iWindowId = iWindowId;
  m_iControlId = iControlId;
  m_iSortMethod = iSortMethod;
  m_iSortOrder = iSortOrder;
  m_iSelectedItem = iSelectedItem;
}

CGetDirectoryJob::~CGetDirectoryJob() 
{
}

void CGetDirectoryJob::DoWork()
{
  CFileItemList items;
  CLog::Log(LOGDEBUG,"CGetDirectoryJob::DoWork, NEWUI, loading directory, path = %s", m_strPath.c_str());

  bool bResult;

  if (m_pItem) 
  {
    bResult = DIRECTORY::CDirectory::GetDirectory(m_pItem.get(), items);
  }
  else 
  {
    bResult = DIRECTORY::CDirectory::GetDirectory(m_strPath, items);
  }

  if (bResult || items.GetPropertyBOOL("getcredentials"))
  {
    items.SetProperty("windowId", m_iWindowId);
    items.SetProperty("controlId", m_iControlId);
    items.SetProperty("selectedItem", m_iSelectedItem);
    items.m_strPath = m_strPath;
    if (m_pItem && m_pItem->HasProperty("BrowseBackgroundImage") && !items.HasProperty("BrowseBackgroundImage"))
      items.SetProperty("BrowseBackgroundImage", m_pItem->GetProperty("BrowseBackgroundImage").c_str());
    
    if (items.HasProperty("BrowseBackgroundImage"))
    {
      CStdString strImagePath = items.GetProperty("BrowseBackgroundImage");

      if (!CUtil::IsHD(strImagePath))
      {
        CStdString cacheImagePath = CFileItem::GetCachedThumb(strImagePath,g_settings.GetVideoFanartFolder());
        items.SetProperty("BrowseBackgroundImage", cacheImagePath);
      
        CFileCurl http;
        if (!http.Download(strImagePath,cacheImagePath))
          items.ClearProperty("BrowseBackgroundImage"); // failed. no image
      }
    }

    m_pCallback->ItemsLoaded(items);
  }
  else 
  {
    if (items.HasProperty("NotErrorDontShowErrorDialog"))
    {
      m_pCallback->LoadFailed(m_iWindowId, m_iControlId, false);
    }
    else
    {
      CLog::Log(LOGDEBUG,"CGetDirectoryJob::DoWork, NEWUI, could not load directory, path = %s", m_strPath.c_str());
      m_pCallback->LoadFailed(m_iWindowId, m_iControlId);
    }
  }
}

CLoadFileItemJob::CLoadFileItemJob(CFileItemPtr pItem, IBackgroundLoaderObserver* pCallback) : BXBGJob("CLoadFileItemJob")
{
  m_pCallback = pCallback;
  m_pItem = pItem;
}

CLoadFileItemJob::~CLoadFileItemJob()
{
}

void CLoadFileItemJob::DoWork()
{
  if (!CBackgroundLoaderFactory::RunBackgroundLoader(m_pItem, true))
  {
    // In case load has failed mark the item accordingly
    m_pItem->SetProperty("LoadFailed",true);
  }
  
  m_pCallback->OnItemLoaded(m_pItem.get());

}

CLoadMetadataJob::CLoadMetadataJob(CFileItemPtr pItem, IBackgroundLoaderObserver* pCallback) : BXBGJob("CLoadMetadataJob")
{
  m_pCallback = pCallback;
  m_pItem = pItem;
}

CLoadMetadataJob::~CLoadMetadataJob()
{
}

void CLoadMetadataJob::DoWork()
{
  if ((m_pItem->IsHD() || m_pItem->IsSmb()) && !m_pItem->m_bIsFolder && !m_pItem->m_bIsShareOrDrive &&
      !m_pItem->m_strPath.IsEmpty() && !m_pItem->GetPropertyBOOL("MetaDataExtracted") &&
      g_application.IsPathAvailable(m_pItem->m_strPath), false)
  {
    CDVDFileInfo::GetFileMetaData(m_pItem->m_strPath, m_pItem.get());
    m_pItem->SetProperty("MetaDataExtracted",true);
  }

  m_pCallback->OnItemLoaded(m_pItem.get());
}

bool CBackgroundLoaderFactory::RunBackgroundLoader(CFileItemPtr pItem, bool bCanBlock)
{
  bool result = false;

  if (pItem->GetPropertyBOOL("ishistory")) 
  {
    CPictureThumbLoader loader;
    result = loader.LoadItem(pItem.get(), bCanBlock);
  }
  else if (pItem->GetPropertyBOOL("IsFeedItem")) 
  {
    CBoxeeFeedItemsLoader loader;
    result = loader.LoadItem(pItem.get(), bCanBlock);
  }
  else if (pItem->GetPropertyBOOL("IsBoxeeServerItem"))
  {
    CBoxeeServerItemsLoader loader;
    result = loader.LoadItem(pItem.get(), bCanBlock);
  }
  else if (pItem->IsRSS())
  {
    CPictureThumbLoader loader;
    result = loader.LoadItem(pItem.get(), bCanBlock);
  }
  else if (pItem->GetPropertyBOOL("IsFriend")) 
  {
    CPictureThumbLoader loader;
    result = loader.LoadItem(pItem.get(), bCanBlock);
  }
  else if (pItem->IsAudio() || pItem->GetPropertyBOOL("IsAlbum") || (pItem->GetPropertyBOOL("IsGroup")  && pItem->GetProperty("grouptype") == "music")) 
  {
    CLog::Log(LOGDEBUG,"CBackgroundLoaderFactory::RunBackgroundLoader, run music thumb loader on item label = %s, path = %s (musicthumb)",
        pItem->GetLabel().c_str(), pItem->m_strPath.c_str());

    CMusicThumbLoader loader;
    result = loader.LoadItem(pItem.get(), bCanBlock);
  }
  else if (pItem->GetPropertyBOOL("IsArtist")) 
  {
    CLog::Log(LOGDEBUG,"CBackgroundLoaderFactory::RunBackgroundLoader, run picture thumb loader on artist item %s (musicthumb)", pItem->m_strPath.c_str());
    CPictureThumbLoader loader;
    result = loader.LoadItem(pItem.get(), bCanBlock);
  }
  else if (pItem->IsVideo() || (pItem->GetPropertyBOOL("IsGroup") && pItem->GetProperty("grouptype") == "video") || pItem->GetPropertyBOOL("isvideo") || pItem->GetProperty("isvideo") == "true") 
  {
    CVideoThumbLoader loader;
    result = loader.LoadItem(pItem.get(), bCanBlock);
  }
  else if (pItem->IsPicture())
  {
    CPictureThumbLoader loader;
    result = loader.LoadItem(pItem.get(), bCanBlock);
  }
  else {
    CPictureThumbLoader loader;
    result = loader.LoadItem(pItem.get(), bCanBlock);
  }

  // confusing name- it means - mark the watched property of an item (either watched or unwatched)
  BoxeeUtils::MarkWatched(pItem.get());

  if (result)
    pItem->SetProperty("isloaded",true);
  
  return result;
}

CItemLoader::CItemLoader() 
{
  m_bStopped = false;
  m_processor.SetName("Item Loader");
  m_directoryProcessor.SetName("Directory Loader");
}

CItemLoader::~CItemLoader()
{
  Stop();
}

bool CItemLoader::Init()
{
  CLog::Log(LOGDEBUG,"CItemLoader::Init, NEWUI, item loader initialized");

  bool bResult = true;
  bResult &= m_processor.Start(NUM_ITEM_LOADER_THREADS);
  bResult &= m_directoryProcessor.Start(NUM_DIRECTORY_LOADER_THREADS);
  return bResult;
}

void  CItemLoader::SignalStop()
{
  m_bStopped = true;
  m_directoryProcessor.SignalStop();
  m_processor.SignalStop();
}

void  CItemLoader::Stop()
{
  m_bStopped = true;
  m_directoryProcessor.Stop();
  m_processor.Stop();
}

int CItemLoader::GetQueueSize()
{
  return m_processor.GetQueueSize();
}

void CItemLoader::AddControl(int dwWindowId, int dwControlId, CFileItemList items, SORT_METHOD iSortMethod, SORT_ORDER iSortOrder, int iSelectedItem)
{
  if (items.Size() == 0) 
  {
    return;
  }
  
  CLog::Log(LOGDEBUG,"CItemLoader::AddControl, 1 GUI_MSG_LOADING, window = %d, control = %d (basecontainer)", dwWindowId, dwControlId);
  CGUIMessage msg(GUI_MSG_LOADING, dwWindowId, dwControlId);
  g_windowManager.SendMessage(msg, dwWindowId);
  
  items.SetProperty("windowId", (int)dwWindowId);
  items.SetProperty("controlId", (int)dwControlId);
  items.SetProperty("selectedItem", (int)iSelectedItem);
  ItemsLoaded(items);
}

void CItemLoader::AddControl(int dwWindowId, int dwControlId, CFileItem* pItem, SORT_METHOD iSortMethod, SORT_ORDER iSortOrder, int iSelectedItem)
{
  CLog::Log(LOGDEBUG,"CItemLoader::AddControl, window = %d, control = %d, path = %s (browse)", dwWindowId, dwControlId, pItem->m_strPath.c_str());

  if (!pItem)
    return;

  if (pItem->m_strPath == "")
  {
    delete pItem;
    return;
  }

  CLog::Log(LOGDEBUG,"CItemLoader::AddControl, 2 GUI_MSG_LOADING, window = %d, control = %d (basecontainer)", dwWindowId, dwControlId);
  CGUIMessage msg(GUI_MSG_LOADING, dwWindowId, dwControlId);
  g_windowManager.SendMessage(msg);

  CFileItemList items;

  // Retreive the items corresponding the path
  CFileItemPtr itemPtr(pItem);
  CGetDirectoryJob* pJob = new CGetDirectoryJob(itemPtr, dwWindowId, dwControlId, this, iSortMethod, iSortOrder, iSelectedItem);

  // python requires some stuff to run from the main thread. therefore we do not queue the job on pythons (plugins/scripts) but rather execute it
  if (CUtil::IsPlugin(itemPtr->m_strPath) || CUtil::IsScript(itemPtr->m_strPath))
  {
    pJob->DoWork();
    delete pJob;
  }
  else
  {
    if (!m_directoryProcessor.QueueJob(pJob))
    {
      CLog::Log(LOGERROR,"CItemLoader::AddControl, unable to queue directory job, path = %s (browse)", pItem->m_strPath.c_str());
      delete pJob;
      LoadFailed(dwWindowId, dwControlId);
    }
  }
  CLog::Log(LOGDEBUG,"CItemLoader::AddControl, added directory job, path = %s (browse)", pItem->m_strPath.c_str());
}

void CItemLoader::AddControl(int dwWindowId, int dwControlId, const CStdString& strPath, SORT_METHOD iSortMethod, SORT_ORDER iSortOrder, int iSelectedItem)
{
  CLog::Log(LOGDEBUG,"CItemLoader::AddControl, window = %d, control = %d, path = %s (browse)", dwWindowId, dwControlId, strPath.c_str());

  if (strPath == "") return;

  CLog::Log(LOGDEBUG,"CItemLoader::AddControl, 3 GUI_MSG_LOADING, window = %d, control = %d (basecontainer)", dwWindowId, dwControlId);
  CGUIMessage msg(GUI_MSG_LOADING, dwWindowId, dwControlId);
  g_windowManager.SendMessage(msg, dwWindowId);

  CFileItemList items;

  // Retreive the items corresponding the path
  CGetDirectoryJob* pJob = new CGetDirectoryJob(strPath, dwWindowId, dwControlId, this, iSortMethod, iSortOrder, iSelectedItem);
  
  // python requires some stuff to run from the main thread. therefore we do not queue the job on pythons (plugins/scripts) but rather execute it
  if (CUtil::IsPlugin(strPath) || CUtil::IsScript(strPath))
  {
    pJob->DoWork();
    delete pJob;
  }
  else
  {
    if (!m_directoryProcessor.QueueJob(pJob))
    {
      CLog::Log(LOGERROR,"CItemLoader::AddControl, unable to queue directory job, path = %s (browse)", strPath.c_str());
      delete pJob;
      LoadFailed(dwWindowId, dwControlId);
    }
  }
  CLog::Log(LOGDEBUG,"CItemLoader::AddControl, added directory job, path = %s (browse)", strPath.c_str());
}

void CItemLoader::LoadImage(int dwWindowId, int dwControlId, const CStdString& strPath)
{
  CFileItemPtr pItem ( new CFileItem );
  pItem->m_strPath = strPath;
  pItem->SetThumbnailImage(strPath);
  pItem->SetProperty("IsPicture",true);
  pItem->SetProperty("ControlID",(int)dwControlId);

  BOXEE::BXBGJob* pJob = new CLoadFileItemJob(pItem, this);

  if (!m_processor.QueueFront(pJob)) {
    CLog::Log(LOGERROR,"CItemLoader::LoadImage, NEWUI, Could not queue job ");
    delete pJob;
  }
}

void CItemLoader::LoadFailed(int windowId, int controlId, bool bErrorOccured)
{
  CGUIMessage msg(GUI_MSG_LOAD_FAILED, windowId, controlId, bErrorOccured ? 1 : 0);
  g_windowManager.SendThreadMessage(msg, windowId);
}

void CItemLoader::ItemsLoaded(CFileItemList& items)
{ 
  // Get the path for which items were loaded
  CStdString strPath = items.m_strPath;
  // Assign unique id to each of the items
  for (int j=0; j < items.Size() && !m_bStopped; j++)
  {
    CStdString strId;
    unsigned int now = CTimeUtils::GetTimeMS();

    strId.Format("%lu-%d",now, j);
    items[j]->SetProperty("itemid", strId);
    items[j]->SetProperty("directoryPath", strPath);
  }

  int windowId = items.GetPropertyInt("windowId");
  int controlId = items.GetPropertyInt("controlId");
  int iSelectedItem = items.GetPropertyInt("selectedItem");

  // We create a new list and send it to the browse screen
  // the browse screen is responsible for deleting the items

  CFileItemList* pNewList = new CFileItemList();
  pNewList->Copy(items);
  
  std::vector<BOXEE::BXBGJob*> vecLoadJobs;
  vecLoadJobs.reserve(items.Size()+1);
  
  // Go over all retreieved items and activate relevant background loading
  for (int j=pNewList->Size()-1; j >= 0 && !m_bStopped; j--)
  {
    CFileItemPtr pItem = pNewList->Get(j);
    
    if (!pItem->GetPropertyBOOL("isloaded"))
    {
      // Window and control id will be used by other loaders to match the item to its container
      pItem->SetProperty("windowId", (int)windowId);
      pItem->SetProperty("controlId", (int)controlId);

      if (pItem->GetPropertyBOOL("doload"))
      {
        // Create new item and send it to the background loader
        CFileItemPtr pNewItem ( new CFileItem(*pItem) );
        BOXEE::BXBGJob* pJob = new CLoadFileItemJob(pNewItem, this);
        vecLoadJobs.push_back(pJob);

        pItem->SetThumbnailImage("defaultloading.gif");
      }
      else
      {
        pItem->SetProperty("needsloading", true);
      }
    }
  }
  
  if (items.HasProperty("BrowseBackgroundImage"))
  {
    CGUIMessage msg(GUI_MSG_BG_PICTURE, windowId, controlId);
    msg.SetStringParam(items.GetProperty("BrowseBackgroundImage"));
    g_windowManager.SendThreadMessage(msg, windowId);
  }
  
  CGUIMessage msg(GUI_MSG_LABEL_BIND, windowId, controlId, iSelectedItem, 0, pNewList);
  g_windowManager.SendThreadMessage(msg, windowId);

  
  // Queue all jobs into the processor in reverse order 
  if (vecLoadJobs.size())
  {
    std::reverse(vecLoadJobs.begin(), vecLoadJobs.end());
    m_processor.QueueJobs(vecLoadJobs);
  }
}

void CItemLoader::LoadItem(CFileItem* pItem)
{
  CFileItemPtr pNewItem ( new CFileItem(*pItem) );
  CBackgroundLoaderFactory::RunBackgroundLoader(pNewItem, false);
  if (pNewItem->GetPropertyBOOL("isloaded"))
  {
    *pItem = *pNewItem;
    return;
  }
    
  BOXEE::BXBGJob* pJob = new CLoadFileItemJob(pNewItem, this);
  pItem->SetThumbnailImage("defaultloading.gif");
  m_processor.QueueFront(pJob);
}

// Implementation of the IBackgroundLoaderObserver interface
void CItemLoader::OnItemLoaded(CFileItem* pItem)
{
  int windowId = pItem->GetPropertyInt("windowId");
  int controlId = pItem->GetPropertyInt("controlId");
  
  pItem->FillInDefaultIcon();
  pItem->SetProperty("isloaded", true);

  // Update the cache
  g_directoryCache.UpdateItem(*pItem);

  CGUIMessage winmsg(GUI_MSG_ITEM_LOADED, windowId, controlId, 0, 0);
  winmsg.SetPointer(new CFileItem(*pItem));
  g_windowManager.SendThreadMessage(winmsg, windowId);

}

void CItemLoader::LoadFileMetadata(int dwWindowId, int dwControlId, CFileItem* pItem)
{
  pItem->SetProperty("windowId", (int)dwWindowId);
  pItem->SetProperty("controlId", (int)dwControlId);

  // Create new item and send it to the background loader
  CFileItemPtr pNewItem ( new CFileItem(*pItem) );

  BOXEE::BXBGJob* pJob = new CLoadMetadataJob(pNewItem, this);

  if (!m_processor.QueueFront(pJob)) {
    CLog::Log(LOGERROR,"CItemLoader::ItemsLoaded, NEWUI, Could not queue job ");
    delete pJob;
  }
}


