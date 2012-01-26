#include "ThumbnailsManager.h"
#include "utils/log.h"
#include "Util.h"
#include "utils/SingleLock.h"
#include "SpecialProtocol.h"
#include "AdvancedSettings.h"
#include <inttypes.h>
#include "Directory.h"
#include "FileItem.h"
#include <exception>
#include "Settings.h"

#ifdef WIN32
#define F_OK 0
#include <io.h>
#endif

class GdbmFatalError : public std::exception
{
public:
  GdbmFatalError(const char* msg)
  {
    m_msg = msg;
  }

  virtual ~GdbmFatalError() throw()
  {

  }

  virtual const char* what() const throw()
  {
    return m_msg.c_str();
  }

private:
  std::string m_msg;
};

#if GDBM_VERSION_MAJOR >= 1 && GDBM_VERSION_MINOR >= 9
static void gdbmFatalErrorHandler(const char *msg)
{
  throw GdbmFatalError(msg);
}
#else
static void gdbmFatalErrorHandler()
{
  throw GdbmFatalError("");
}
#endif

CThumbnailManager::CThumbnailManager(): m_file(NULL), m_thumbNum(0)
{
  m_path = NULL;
}

CThumbnailManager::~CThumbnailManager()
{
  if (m_path != NULL)
  {
    free(m_path);
	m_path = NULL;
  }
}

bool CThumbnailManager::IsExternalThumbFile(const std::string &strThumb)
{
  CStdString strMasterProfile = _P("special://masterprofile/Thumbnails");
  CStdString strProfile = _P("special://profile/Thumbnails");
  CStdString strThumbFile = _P(strThumb);

  if(strThumbFile.Find(strMasterProfile) == -1 &&
     strThumbFile.Find(strProfile) == -1)
  {
    return true;
  }

  return false;
}

bool CThumbnailManager::Initialize(const std::string &strLocalThumbPath)
{
  std::string strTranslatedPath = PTH_IC(strLocalThumbPath);
  std::string fullName = CUtil::AddFileToFolder(strTranslatedPath, "thumb_files.gdbm");

  m_path = (char*)malloc(fullName.size() + 1);
  memset(m_path,0, fullName.size() + 1);
  strcpy(m_path, fullName.c_str());

  // if the file doesn't exist - create it.
  try
  {
    CSingleLock  lock(m_lock);

    if (!OpenFile(GDBM_WRCREAT))
    {
      return false;
    }

    CloseFile();

    Reorganize();
  }
  catch (GdbmFatalError& e)
  {
    CLog::Log(LOGINFO, "Got gdbm fatal exception, cleaning up: %s", e.what());
    HardReset();
  }

  CLog::Log(LOGDEBUG,"%s initialized Thumbnails manager. contains %d files, average time %lu", __FUNCTION__, m_thumbNum, m_averageTime);
  return true;
}

void CThumbnailManager::Deinitialize()
{
  try
  {
    CloseFile();
  }
  catch (GdbmFatalError& e)
  {
    CLog::Log(LOGINFO, "Got gdbm fatal exception, cleaning up: %s", e.what());
    HardReset();
  }

  if (m_path != NULL)
  {
    free(m_path);
	  m_path = NULL;
  }
}

void CThumbnailManager::TouchThumbnailFile(const std::string &strLocalThumbFile, int64_t newFileSize)
{
  if (IsExternalThumbFile(strLocalThumbFile))
  {
    CLog::Log(LOGDEBUG,"%s invalid or external thumbnail %s (thm)", __FUNCTION__, strLocalThumbFile.c_str());
    return;
  }

  CLog::Log(LOGDEBUG,"%s touch thumbnail file %s size %lld (thm)", __FUNCTION__, strLocalThumbFile.c_str(), newFileSize);

  try
  {
    CSingleLock  lock(m_lock);

    if (!OpenFile(GDBM_READER))
    {
      return;
    }

    datum thumbKey, thumbData;
    time_t now = time(NULL);

    if (!SetThumKey(thumbKey,strLocalThumbFile))
    {
      return;
    }

    // update the thumbnails file number and average access time (in case that the file was already exist)
    thumbData.dptr = NULL;
    thumbData.dsize = 0;
    thumbData = gdbm_fetch(m_file, thumbKey);

    CloseFile();

    ThumbObjRecord *thumbRec = NULL;
    bool foundKey = false;

    if (thumbData.dptr == NULL)
    {
      EnsureThumnailsQuota(newFileSize);

      m_thumbNum ++;
      m_averageTime = ((uint64_t)m_averageTime * (m_thumbNum -1) + now ) / m_thumbNum;
      CLog::Log(LOGDEBUG,"%s touch thumbnail file %s doesn't exist - add new  total %d files average time %lu (thm)",
          __FUNCTION__, strLocalThumbFile.c_str(), m_thumbNum, m_averageTime);

      // prepare the new record
      thumbRec = (ThumbObjRecord *)malloc(sizeof(ThumbObjRecord));
      memset(thumbRec,0,sizeof(ThumbObjRecord));
      if (thumbRec == NULL)
      {
        CLog::Log(LOGERROR,"%s SEVERE ERROR malloc failed (thm)", __FUNCTION__);
        return;
      }
      memset(thumbRec,0, sizeof(ThumbObjRecord));   // set
      thumbRec->m_size = 0;
    }
    else
    {
      foundKey = true;
      thumbRec = (ThumbObjRecord *)thumbData.dptr;
      m_averageTime = (((uint64_t)m_averageTime * m_thumbNum) + (now  - thumbRec->m_accessTime) ) / m_thumbNum;
      CLog::Log(LOGDEBUG,"%s touch thumbnail file %s already exist - update the average time total %d files average time %lu(thm)", __FUNCTION__, strLocalThumbFile.c_str(), m_thumbNum, m_averageTime);
    }

    if (OpenFile(GDBM_WRITER))
    {
      thumbRec->m_accessTime = now;
      if (newFileSize != 0 )
      {
        thumbRec->m_size = newFileSize;
      }

      // the data is the last access time
      thumbData.dptr = (char *)thumbRec;
      thumbData.dsize = sizeof(ThumbObjRecord);

      // check if the url is already exist - if it is just update its access time
      if (gdbm_store(m_file, thumbKey, thumbData, GDBM_REPLACE) != 0)
      {
        CLog::Log(LOGERROR,"%s Can not insert/replace tnumbnail file %s to the gdbm file (thm)", __FUNCTION__, strLocalThumbFile.c_str());
      }
    }
    else
    {
      CLog::Log(LOGERROR,"%s Can not open tnumbnail gdbm file for write(thm)", __FUNCTION__);
    }

    // clear all the pre allocated data
    if (thumbKey.dptr != NULL)
    {
      free(thumbKey.dptr);
    }

    if (foundKey)
    {
      FreeDatum(thumbData);
    }
    else
    {
      if (thumbRec != NULL)
      {
        free(thumbRec);
      }
    }

    CloseFile();
  }
  catch (GdbmFatalError& e)
  {
    CLog::Log(LOGINFO, "Got gdbm fatal exception, cleaning up: %s", e.what());
    HardReset();
  }
}

void  CThumbnailManager::CloseFile()
{
  // make sure that the file is close
  if (m_file != NULL)
  {
    gdbm_close(m_file);
    m_file = NULL;
  }
}

bool CThumbnailManager::OpenFile(int permission)
{
  if (!m_path)
  {
    CLog::Log(LOGERROR,"CThumbnailManager::OpenFile - FAILED to open ThumbnailManager file, path is empty [permission=%d] (thm)",permission);
    return false;   
  }

  m_file = gdbm_open(m_path, 1024, permission, 0644, gdbmFatalErrorHandler);

  if (m_file == NULL)
  {
    CLog::Log(LOGERROR,"CThumbnailManager::OpenFile - FAILED to open ThumbnailManager file. [path=%s][permission=%d] (thm)", m_path,permission);
    return false;
  }

  return true;
}

void CThumbnailManager::FreeDatum(datum  &thumbData)
{
#ifndef _WIN32
  if (thumbData.dptr != NULL)
  {
    free(thumbData.dptr);
	thumbData.dptr = NULL;
  }
#else

	// an ugly patch
	// gdbm_fetch allocate a memory for thumbdata.dptr, and the user is responsible to release it.
	// since we use a gdbm3.dll it compiled with a differnt malloc then ours, so we cant use the resular
	// free methond (for windows only)
	// so when ever we enter gdbm_fetch it first release the thumbData (its a static global variable)
	// and try to fetch the new key. 
	// in order to free the memory we shold access fetch with a non existing key
	datum thumKey;
	SetThumKey(thumKey,"NON-EXISTING-KEY");
	thumbData = gdbm_fetch(m_file, thumKey);

	if (thumbData.dptr != NULL)
	{
		CLog::Log(LOGERROR, "Failed to release thumb Data PTR (thm)");
	}
#endif

}

bool CThumbnailManager::SetThumKey(datum  &thumbKey, const std::string &keyStr)
{
  // the key is the file name
  char *tmpPath = NULL;
  int  pathSize = keyStr.size() + 1;
  tmpPath = (char*)malloc(pathSize);
  if (tmpPath == NULL)
  {
    CLog::Log(LOGERROR,"%s SEVERE ERROR malloc failed (thm)", __FUNCTION__);
    return false;
  }
  memset(tmpPath,0, pathSize);
  strcpy(tmpPath, keyStr.c_str());

  thumbKey.dptr = tmpPath;
  thumbKey.dsize = pathSize;

  return true;
}


void CThumbnailManager::EnsureThumnailsQuota(int64_t newFileSize)
{
  CFileItemList items;
  CStdString profilesDir = _P("special://home/profiles");
  unsigned int totalSize = 0;

  DIRECTORY::CDirectory::GetDirectory(profilesDir, items);

  for(int i=0; i<items.Size(); i++)
  {
    CStdString thumbDir;
    CUtil::AddFileToFolder(items[i]->m_strPath, "Thumbnails", thumbDir);

    totalSize += CUtil::GetDirSize(thumbDir);
  }

  if (totalSize > g_advancedSettings.m_thumbsMaxSize / 1024)
  {
    CLog::Log(LOGDEBUG,"%s Thumbnails dir size %u exceeded max size %"PRIu64" clear oldest thumbnails (thm)",__FUNCTION__, totalSize, g_advancedSettings.m_thumbsMaxSize);
    ClearThumnails(m_averageTime, (uint64_t) (g_advancedSettings.m_thumbsMaxSize * 0.35f), true);
  }

}

struct ErasedThumbT
{
  public:
    std::string     m_path;
    uint64_t        m_size;
};

bool LessByTime (time_t time1, time_t time2)
{
  return time1<time2;
}

void CThumbnailManager::ClearThumnails(time_t  byDate , uint64_t sizeToDelete, bool bEnsureThumnailsQuota)
{
  try
  {
    datum thumbKey, thumbData;
    std::set<time_t, bool(*)(time_t,time_t)> fileByTime(LessByTime);
    std::multimap<time_t, ErasedThumbT> fileToDelete;
    bool needToReorg = false;

    CSingleLock  lock(m_lock);

    if (!OpenFile(GDBM_READER))
    {
      return;
    }

    CLog::Log(LOGDEBUG,"%s files that are older then %lu will be erased (thm)", __FUNCTION__, byDate);
    thumbKey = gdbm_firstkey(m_file);
    while(thumbKey.dptr != NULL)
    {
      thumbData = gdbm_fetch(m_file, thumbKey);

      ThumbObjRecord *thumbRec = (ThumbObjRecord *)thumbData.dptr;
      if (bEnsureThumnailsQuota || (thumbRec->m_accessTime <= byDate))
      {
        ThumbObjRecord *thumbRec = (ThumbObjRecord *)thumbData.dptr;
        ErasedThumbT  eraseThum;
        eraseThum.m_path = thumbKey.dptr;
        eraseThum.m_size = thumbRec->m_size;

        CLog::Log(LOGDEBUG,"%s files %s was last accessed on %lu might be erased during cleanup (thm)", __FUNCTION__, thumbKey.dptr, *(time_t *)thumbData.dptr);
        fileByTime.insert(thumbRec->m_accessTime);
        fileToDelete.insert(std::make_pair(thumbRec->m_accessTime, eraseThum));
      }

      FreeDatum(thumbData);
      thumbKey = gdbm_nextkey(m_file, thumbKey);
    }
    CloseFile();


    if (!OpenFile(GDBM_WRITER))
    {
      return;
    }

    uint64_t deletedSize = 0;
    std::set<time_t, bool(*)(time_t,time_t)>::iterator  it = fileByTime.begin();
    while (it != fileByTime.end() && deletedSize < sizeToDelete)
    {

      std::multimap<time_t, ErasedThumbT>::iterator fileIt = fileToDelete.find(*it);

      while (deletedSize < sizeToDelete && fileIt != fileToDelete.end() )
      {
        ErasedThumbT fileToErase = fileIt->second;
        if (!SetThumKey(thumbKey,fileToErase.m_path))
        {
          return;
        }
        CLog::Log(LOGDEBUG,"%s erase files %s  access time %lu (thm)", __FUNCTION__, fileToErase.m_path.c_str(), fileIt->first);

        CStdString translatedPath = _P(fileToErase.m_path);
        bool fileExists = access(translatedPath.c_str(), F_OK) !=-1;

        if (fileExists && !IsExternalThumbFile(fileToErase.m_path))
        {
          bool fileDeleted = DeleteFile(translatedPath);

          if (gdbm_delete(m_file, thumbKey) != -1)
          {
            deletedSize += fileToErase.m_size;
            needToReorg = true;
          }
          else
          {
            CLog::Log(LOGERROR,"%s couldn't delete rec %s from gdbm file (thm)", __FUNCTION__, thumbKey.dptr);
          }

          if(!fileDeleted)
          {
            CLog::Log(LOGERROR,"%s couldn't delete file %s (thm)", __FUNCTION__, translatedPath.c_str());
          }
        }

        if (thumbKey.dptr)
        {
          free(thumbKey.dptr);
        }

        fileToDelete.erase(fileIt);
        fileIt = fileToDelete.find(*it);
      }

      it++;
    }

    CloseFile();

    // clear the containers
    fileToDelete.clear();
    fileByTime.clear();

    if (needToReorg)
    {
      Reorganize();
    }
  }
  catch (GdbmFatalError& e)
  {
    CLog::Log(LOGINFO, "Got gdbm fatal exception, cleaning up: %s", e.what());
    HardReset();
  }
}

void CThumbnailManager::Reorganize()
{
  datum thumbKey, thumbData;
  uint64_t   timeAcc = 0;

  m_thumbNum = 0;
  m_averageTime = 0;

  // open the gdbm file for write so we can reorganized it,
  // calculate the files number and the average size
  if (!OpenFile(GDBM_WRITER))
  {
    return;
  }
  gdbm_reorganize(m_file);

  m_averageTime = 0 ;
  m_thumbNum = 0;
  thumbKey = gdbm_firstkey(m_file);

  int i = 1;
  while(thumbKey.dptr != NULL)
  {
    m_averageTime = 0 ;
    m_thumbNum = 0;
    thumbData = gdbm_fetch(m_file, thumbKey);

    ThumbObjRecord *thumbRec = (ThumbObjRecord *)thumbData.dptr;
    //CLog::Log(LOGDEBUG,"%s %d. %s - %lu size %"PRIu64" (thm)", __FUNCTION__, i, thumbKey.dptr, thumbRec->m_accessTime, thumbRec->m_size);

    timeAcc += thumbRec->m_accessTime;
    thumbKey = gdbm_nextkey(m_file, thumbKey);
    m_thumbNum ++;

    FreeDatum(thumbData);
    i++;
  }

  if (m_thumbNum != 0)
  {
    m_averageTime = timeAcc / m_thumbNum;
  }

  CloseFile();
  //CLog::Log(LOGDEBUG,"%s thumbs file num %d average size %lu",__FUNCTION__, m_thumbNum, m_averageTime);
}

bool CThumbnailManager::HardReset()
{
  CSingleLock  lock(m_lock);

  try
  {
    CloseFile();

    CUtil::WipeDir(g_settings.GetThumbnailsFolder());
    g_settings.CreateThumbnailsFolders();

    ::DeleteFile(m_path);

    if (!OpenFile(GDBM_WRCREAT))
    {
      return false;
    }

    CloseFile();
  }
  catch (GdbmFatalError& e)
  {
    CLog::Log(LOGINFO, "Got gdbm fatal exception: %s", e.what());
    return false;
  }

  return true;
}


