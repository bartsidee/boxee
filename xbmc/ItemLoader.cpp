
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

#ifdef HAS_EMBEDDED
#define NUM_VIDEO_ITEM_LOADER_THREAD 2
#endif

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

CGetDirectoryJob::~CGetDirectoryJob() 
{
}

void CGetDirectoryJob::DoWork()
{
  CFileItemList items;
  CLog::Log(LOGDEBUG,"CGetDirectoryJob::DoWork, NEWUI, loading directory, path = %s", m_strPath.c_str());

  bool bResult;

  items.SetProperty("dontUseGuestCredentials",true);
  if (m_pItem) 
  {
    bResult = DIRECTORY::CDirectory::GetDirectory(m_pItem.get(), items);
  }
  else 
  {
    bResult = DIRECTORY::CDirectory::GetDirectory(m_strPath, items, "", true, true);
  }

  CLog::Log(LOGDEBUG,"CGetDirectoryJob::DoWork, NEWUI, done loading directory, bResult = %d", bResult);

  if (bResult || items.GetPropertyBOOL("getcredentials"))
  {
    items.SetProperty("windowId", m_iWindowId);
    items.SetProperty("controlId", m_iControlId);
    items.SetProperty("selectedItem", m_iSelectedItem);
    items.SetProperty("requestId", GetId());
    items.m_strPath = m_strPath;


    // If the original item has background image copy it to the items
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

    items.Sort(m_iSortMethod,m_iSortOrder);
    m_pCallback->ItemsLoaded(items);
  }
  else 
  {
    if (items.HasProperty("NotErrorDontShowErrorDialog"))
    {
      CLog::Log(LOGDEBUG,"CGetDirectoryJob::DoWork, NEWUI, No Error, path = %s", m_strPath.c_str());
      m_pCallback->LoadFailed(m_iWindowId, m_iControlId, false);
    }
    else
    {
      CLog::Log(LOGDEBUG,"CGetDirectoryJob::DoWork, NEWUI, could not load directory, path = %s", m_strPath.c_str());
      m_pCallback->LoadFailed(m_iWindowId, m_iControlId);
    }
  }
}

bool CGetDirectoryJob::Equals(const BXBGJob& other) const
{
  try
  {
    const CGetDirectoryJob& dcast_other = dynamic_cast<const CGetDirectoryJob&>(other);

    if (m_strPath == dcast_other.m_strPath && m_iWindowId == dcast_other.m_iWindowId &&
        m_iControlId == dcast_other.m_iControlId && m_iSortMethod == dcast_other.m_iSortMethod &&
        m_iSortOrder == dcast_other.m_iSortOrder)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  catch (const std::bad_cast& e)
  {
    CLog::Log(LOGERROR,"Bad Casting Exception: %s",e.what());
  }
  return false;
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

CLoadBackgroundImageJob::CLoadBackgroundImageJob(int windowId, const CStdString& strImagePath): BXBGJob("CLoadBackgroundImageJob")
{
  m_windowId = windowId;
  m_strImagePath = strImagePath;
}

CLoadBackgroundImageJob::~CLoadBackgroundImageJob()
{
}

void CLoadBackgroundImageJob::DoWork()
{
  if (!CUtil::IsHD(m_strImagePath))
  {
    CStdString cacheImagePath = CFileItem::GetCachedThumb(m_strImagePath, g_settings.GetVideoFanartFolder());

    CFileCurl http;
    if (!http.Download(m_strImagePath, cacheImagePath))
    {
      return;
    }
    else
    {
      m_strImagePath = cacheImagePath;
    }
  }

  CGUIMessage msg(GUI_MSG_BG_PICTURE, m_windowId, 0);
  msg.SetStringParam(m_strImagePath);
  g_windowManager.SendThreadMessage(msg, m_windowId);

}


bool CBackgroundLoaderFactory::RunBackgroundLoader(CFileItemPtr pItem, bool bCanBlock)
{
  bool result = false;
  bool bIsPlayableFolder = false;

  if(pItem->m_bIsFolder)
  {
    bIsPlayableFolder = BOXEE::Boxee::GetInstance().GetMetadataEngine().IsPlayableFolder(pItem->m_strPath);
  }

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
  else if (bIsPlayableFolder || pItem->IsVideo() || (pItem->GetPropertyBOOL("IsGroup") && pItem->GetProperty("grouptype") == "video") || pItem->GetPropertyBOOL("isvideo") || pItem->GetProperty("isvideo") == "true") 
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

  // confusing name - it means - mark the watched property of an item (either watched or unwatched)
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
#ifdef HAS_EMBEDDED
  m_videoProcessor.SetName("Video Item Loader");
#endif
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
#ifdef HAS_EMBEDDED
  bResult &= m_videoProcessor.Start(NUM_VIDEO_ITEM_LOADER_THREAD); 
#endif
  return bResult;
}

void  CItemLoader::SignalStop()
{
  m_bStopped = true;
#ifdef HAS_EMBEDDED
  m_videoProcessor.SignalStop();
#endif
  m_directoryProcessor.SignalStop();
  m_processor.SignalStop();
}

void  CItemLoader::Stop()
{
  m_bStopped = true;
#ifdef HAS_EMBEDDED
  m_videoProcessor.Stop();
#endif
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

//void CItemLoader::AddControl(int dwWindowId, int dwControlId, CFileItem* pItem, SORT_METHOD iSortMethod, SORT_ORDER iSortOrder, int iSelectedItem)
//{
//  CLog::Log(LOGDEBUG,"CItemLoader::AddControl, window = %d, control = %d, path = %s (browse)", dwWindowId, dwControlId, pItem->m_strPath.c_str());
//
//  if (!pItem)
//    return;
//
//  if (pItem->m_strPath == "")
//  {
//    delete pItem;
//    return;
//  }
//
//  CLog::Log(LOGDEBUG,"CItemLoader::AddControl, 2 GUI_MSG_LOADING, window = %d, control = %d (basecontainer)", dwWindowId, dwControlId);
//  CGUIMessage msg(GUI_MSG_LOADING, dwWindowId, dwControlId);
//  g_windowManager.SendMessage(msg);
//
//  CFileItemList items;
//
//  // Retreive the items corresponding the path
//  CFileItemPtr itemPtr(pItem);
//  CGetDirectoryJob* pJob = new CGetDirectoryJob(itemPtr, dwWindowId, dwControlId, this, iSortMethod, iSortOrder, iSelectedItem);
//
//  // python requires some stuff to run from the main thread. therefore we do not queue the job on pythons (plugins/scripts) but rather execute it
//  if (CUtil::IsPlugin(itemPtr->m_strPath) || CUtil::IsScript(itemPtr->m_strPath))
//  {
//    pJob->DoWork();
//    delete pJob;
//  }
//  else
//  {
//    if (!m_directoryProcessor.QueueJob(pJob))
//    {
//      CLog::Log(LOGERROR,"CItemLoader::AddControl, unable to queue directory job, path = %s (browse)", pItem->m_strPath.c_str());
//      delete pJob;
//      LoadFailed(dwWindowId, dwControlId);
//    }
//  }
//  CLog::Log(LOGDEBUG,"CItemLoader::AddControl, added directory job, path = %s (browse)", pItem->m_strPath.c_str());
//}

int CItemLoader::AddControl(int dwWindowId, int dwControlId, const CStdString& strPath, SORT_METHOD iSortMethod, SORT_ORDER iSortOrder, int iSelectedItem)
{
  int jobId = 0;
  CLog::Log(LOGDEBUG,"CItemLoader::AddControl, window = %d, control = %d, path = %s (browse)", dwWindowId, dwControlId, strPath.c_str());

  if (strPath != "")
  {
    CLog::Log(LOGDEBUG,"CItemLoader::AddControl, 3 GUI_MSG_LOADING, window = %d, control = %d (basecontainer)", dwWindowId, dwControlId);
    CGUIMessage msg(GUI_MSG_LOADING, dwWindowId, dwControlId);
    g_windowManager.SendMessage(msg, dwWindowId);
  
    CFileItemList items;

    // Retreive the items corresponding the path
    CGetDirectoryJob* pJob = new CGetDirectoryJob(strPath, dwWindowId, dwControlId, this, iSortMethod, iSortOrder, iSelectedItem);
    jobId = pJob->GetId();

    // python requires some stuff to run from the main thread. therefore we do not queue the job on pythons (plugins/scripts) but rather execute it
    if (CUtil::IsPlugin(strPath) || CUtil::IsScript(strPath))
    {
      CLog::Log(LOGDEBUG,"CItemLoader::AddControl, path = %s is either plugin ,script or database - Executing. (browse)", strPath.c_str());
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
      else
      {
        CLog::Log(LOGDEBUG,"CItemLoader::AddControl, added directory job, path = %s (browse)", strPath.c_str());
      }
    }
  }

  return jobId;
}

void CItemLoader::LoadBackgroundImage(int dwWindowId, const CStdString& strImagePath)
{
  BOXEE::BXBGJob* pJob = new CLoadBackgroundImageJob(dwWindowId, strImagePath);

  if (!m_processor.QueueFront(pJob))
  {
    CLog::Log(LOGERROR,"CItemLoader::LoadBackgroundImage, Could not queue job ");
    delete pJob;
  }
}

//void CItemLoader::LoadImage(int dwWindowId, int dwControlId, const CStdString& strPath)
//{
//  CFileItemPtr pItem ( new CFileItem );
//  pItem->m_strPath = strPath;
//  pItem->SetThumbnailImage(strPath);
//  pItem->SetProperty("IsPicture",true);
//  pItem->SetProperty("ControlID",(int)dwControlId);
//
//  BOXEE::BXBGJob* pJob = new CLoadFileItemJob(pItem, this);
//
//  if (!m_processor.QueueFront(pJob)) {
//    CLog::Log(LOGERROR,"CItemLoader::LoadImage, NEWUI, Could not queue job ");
//    delete pJob;
//  }
//}

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

  // we may have items, but the request timed out, so its not the complete list of items..
  if (items.GetPropertyBOOL("isRequestTimedOut"))
  {
    CGUIMessage msgErr(GUI_MSG_LOADING_TIMEDOUT, items.GetPropertyInt("windowId"), 0, items.GetPropertyInt("requestId"));
    msgErr.SetStringParam(items.m_strPath);
    g_windowManager.SendThreadMessage(msgErr, items.GetPropertyInt("windowId"));
  }

  // We create a new list and send it to the browse screen
  // the browse screen is responsible for deleting the items

  CFileItemList* pNewList = new CFileItemList();
  pNewList->Copy(items);
  
  std::vector<BOXEE::BXBGJob*> vecLoadJobs;
  vecLoadJobs.reserve(items.Size()+1);
  
#ifdef HAS_EMBEDDED 
  std::vector<BOXEE::BXBGJob*> vecVideoLoadJobs;
  vecVideoLoadJobs.reserve(items.Size()+1);
#endif

  // Go over all retrieved items and activate relevant background loading
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
#ifdef HAS_EMBEDDED
        if (!pItem->HasThumbnail() && !pItem->GetPropertyBOOL("IsBoxeeServerItem"))
        {
          vecVideoLoadJobs.push_back(pJob);
        }
        else
#endif
        {
           vecLoadJobs.push_back(pJob);
        }

        //pItem->SetThumbnailImage("defaultloading.gif");
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
  
  CLog::Log(LOGDEBUG,"CGetDirectoryJob::ItemsLoaded, sending GUI_MSG_LABEL_BIND windowId [%d] controlId[%d] size[%d]", windowId, controlId, pNewList->Size());

  CGUIMessage msg(GUI_MSG_LABEL_BIND, windowId, controlId, iSelectedItem, 0, pNewList);
  g_windowManager.SendThreadMessage(msg, windowId);

#ifdef HAS_EMBEDDED
  if (vecVideoLoadJobs.size())
  {
    std::reverse(vecVideoLoadJobs.begin(), vecVideoLoadJobs.end());
    m_videoProcessor.QueueJobs(vecVideoLoadJobs);    
  }
#endif  
  // Queue all jobs into the processor in reverse order 
  if (vecLoadJobs.size())
  {
    std::reverse(vecLoadJobs.begin(), vecLoadJobs.end());
    m_processor.QueueJobs(vecLoadJobs);
  }
}

void CItemLoader::LoadItem(CFileItem* pItem , bool bAsyncOnly)
{
  CFileItemPtr pNewItem ( new CFileItem(*pItem) );

  if (!bAsyncOnly)
  {
    CBackgroundLoaderFactory::RunBackgroundLoader(pNewItem, false);
    if (pNewItem->GetPropertyBOOL("isloaded"))
    {
      *pItem = *pNewItem;
      return;
    }
  }

  BOXEE::BXBGJob* pJob = new CLoadFileItemJob(pNewItem, this);
  //pItem->SetThumbnailImage(PLACEHOLDER_IMAGE);
#ifdef HAS_EMBEDDED
  if (!pNewItem->HasThumbnail() && !pNewItem->GetPropertyBOOL("IsBoxeeServerItem"))
  {
    m_videoProcessor.QueueFront(pJob);
  }
  else
#endif
  {
    m_processor.QueueFront(pJob);
  }
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

  //send to all observers
  CStdString itemId = pItem->GetProperty("itemid");

  std::map< CStdString , std::set<int> >::iterator it1 = m_itemLoadedObservers.find(itemId);

  if (it1 != m_itemLoadedObservers.end())
  {
    for (std::set<int>::iterator it2 = it1->second.begin(); it2 != it1->second.end() ; it2++)
    {
      windowId = (int)(*it2);

      CGUIMessage winmsg(GUI_MSG_ITEM_LOADED, windowId, 0);
      winmsg.SetPointer(new CFileItem(*pItem));

      g_windowManager.SendThreadMessage(winmsg, windowId);
    }
  }

#ifdef HAS_EMBEDDED
  uint64_t size = 0;
  struct __stat64 stat;
  CStdString thumb = pItem->GetThumbnailImage();
  if (CFile::Stat(thumb,&stat) == 0)
  {
    size = stat.st_size;
    g_application.GetThumbnailsManager().TouchThumbnailFile(thumb, size);
  }
#endif
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

void CItemLoader::Pause(bool bVideoProcessorOnly)
{
#ifdef HAS_EMBEDDED
  if(bVideoProcessorOnly)
    return m_videoProcessor.Pause();
  
  m_videoProcessor.Pause();
  m_processor.Pause();
#endif
}

void CItemLoader::Resume(bool bVideoProcessorOnly)
{
#ifdef HAS_EMBEDDED
  if(bVideoProcessorOnly)
    return m_videoProcessor.Resume();
  
  m_videoProcessor.Resume();  
  m_processor.Resume();
#endif
}

void CItemLoader::AddItemLoadedObserver(const CStdString& strItemId , int windowId)
{
  /*std::map< CStdString , std::set<int> >::iterator it;

  it = m_itemLoadedObservers.find(strItemId);

  if (it != m_itemLoadedObservers.end())
  {
    it->second.insert(windowId);
  }
  else*/
  {
    m_itemLoadedObservers[strItemId].insert(windowId);
  }
}

void CItemLoader::RemoveItemLoadedObserver(const CStdString& strItemId , int windowId)
{
  std::map< CStdString , std::set<int> >::iterator it;

  it = m_itemLoadedObservers.find(strItemId);

  if (it != m_itemLoadedObservers.end())
  {
    it->second.erase(windowId);
  }
}
