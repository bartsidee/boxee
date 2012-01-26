
#include <vector>
#include <map>
#include "BoxeeMediaSourceList.h"
#include "Application.h"
#include "Util.h"
#include "URL.h"
#include "LocalizeStrings.h"
#include "utils/log.h"

#define SOURCE_VIDEO   0
#define SOURCE_MUSIC   1
#define SOURCE_PICTURE 2

CBoxeeMediaSourceList::CBoxeeMediaSourceList()
{
  load();
}

CBoxeeMediaSourceList::CBoxeeMediaSourceList(VECSOURCES& videoSources, VECSOURCES& musicSources, VECSOURCES& pictureSources)
{
   m_sources.clear();

   loadShares(videoSources, SOURCE_VIDEO);
   loadShares(musicSources, SOURCE_MUSIC);
   loadShares(pictureSources, SOURCE_PICTURE);  
}

void CBoxeeMediaSourceList::deleteSource(CStdString name)
{
  BoxeeMediaSourceMap::iterator findIterator = m_sources.find(name);
  if (findIterator != m_sources.end())
  {
    CBoxeeMediaSource& source = (*findIterator).second;
    if (source.isVideo) g_settings.DeleteSource("video", source.name, source.path);
    if (source.isMusic) g_settings.DeleteSource("music", source.name, source.path);
    if (source.isPicture) g_settings.DeleteSource("pictures", source.name, source.path);
    g_settings.SaveSources();
    load();
  }
}

void CBoxeeMediaSourceList::deleteSourceByAppId(CStdString appId)
{
  bool deleted = false;
  
  BoxeeMediaSourceMap::iterator iterator;
  for (iterator = m_sources.begin(); iterator != m_sources.end(); iterator++)
  {
    CBoxeeMediaSource& source = (*iterator).second;
    if (!CUtil::IsApp(source.path))
    {
      continue;
    }
    
    CURI appUrl(source.path);
    CStdString sourceAppId = appUrl.GetHostName();
    
    if (sourceAppId.CompareNoCase(appId) == 0)
    {
      if (source.isVideo) g_settings.DeleteSource("video", source.name, source.path);
      if (source.isMusic) g_settings.DeleteSource("music", source.name, source.path);
      if (source.isPicture) g_settings.DeleteSource("pictures", source.name, source.path);
      deleted = true;
    }
  }

  if (deleted)
  {
    g_settings.SaveSources();
    load();      
  }
}

void CBoxeeMediaSourceList::deleteDuplicateApps()
{
  std::map<CStdString, CBoxeeMediaSource> pathsProcessed;
  std::vector<CStdString> sourcesToDelete;
  int counter;
  
  // Go over all the sources
  BoxeeMediaSourceMap::iterator iterator;
  for (iterator = m_sources.begin(); iterator != m_sources.end(); iterator++)
  {
    CBoxeeMediaSource& source = (*iterator).second;
    
    // Only look into apps
    if (!CUtil::IsApp(source.path))
    {
      continue;
    }
    
    // If we already processed this path, no need to do it again
    if (pathsProcessed.find(source.path) != pathsProcessed.end())
    {
      continue;
    }
    
    counter = 0;
    
    // Look for sources with the same path
    BoxeeMediaSourceMap::iterator iterator2;
    for (iterator2 = m_sources.begin(); iterator2 != m_sources.end(); iterator2++)
    {
      CBoxeeMediaSource& source2 = (*iterator2).second;
      
      if (source.path.CompareNoCase(source2.path) == 0)
      {
        ++counter;
      }
      
      // Do not delete the first one
      if (counter > 1)
      {
        sourcesToDelete.push_back(source2.name);
        --counter;
      }
    }
    
    pathsProcessed[source.path] = source;
  }
  
  // Delete all the sources that we collected
  if (!sourcesToDelete.empty())
  {
    for (size_t i = 0; i < sourcesToDelete.size(); i++)
    {
      CLog::Log(LOGDEBUG,"CBoxeeMediaSourceList::deleteDuplicateApps, deleting source %s\n", sourcesToDelete[i].c_str());
      deleteSource(sourcesToDelete[i]);
    }
  }
  
  g_settings.SaveSources();
  load();  
}

void CBoxeeMediaSourceList::updateSource(CStdString oldName,const CStdString &strUpdateChild, const CStdString &strUpdateValue)
{
  BoxeeMediaSourceMap::iterator findIterator = m_sources.find(oldName);
  if (findIterator == m_sources.end())
  {
	  CLog::Log(LOGERROR,"can not find name %s in the sources list ", oldName.c_str());
  }

  CBoxeeMediaSource& source = (*findIterator).second;

  if (source.isVideo)
  {
	  g_settings.UpdateSource("video",oldName, strUpdateChild, strUpdateValue );
  }
  if (source.isMusic)
  {
	  g_settings.UpdateSource("music",oldName, strUpdateChild, strUpdateValue );
  }
  if (source.isPicture)
  {
	  g_settings.UpdateSource("pictures",oldName, strUpdateChild, strUpdateValue );
  }

  g_settings.SaveSources();

  load();
}

void CBoxeeMediaSourceList::addOrEditSource(CBoxeeMediaSource orig_source, CBoxeeMediaSource source)
{
  CMediaSource xbmcSource;
  std::vector<CStdString> paths;
  paths.push_back(source.path);
  xbmcSource.m_iScanType = source.scanType;
  xbmcSource.m_strThumbnailImage = source.thumbPath;
  xbmcSource.m_adult = source.isAdult;
  xbmcSource.m_country = source.country;
  xbmcSource.m_countryAllow = source.countryAllow;

  // for each type - check if we need to add , delete or untouch

  //VIDEO
  if (source.isVideo == orig_source.isVideo)
  {
    CLog::Log(LOGDEBUG,"CBoxeeMediaSourceList::addOrEditSource video scan type wasn't change - don't delete or add");
}
  else
  {
    if (orig_source.isVideo)
    {
      CLog::Log(LOGDEBUG,"CBoxeeMediaSourceList::addOrEditSource source use to be video - erase video source");
      g_settings.DeleteSource("video", orig_source.name, source.path);
    }
    else
    {
      CLog::Log(LOGDEBUG,"CBoxeeMediaSourceList::addOrEditSource source scan type change to video - add it ");
      xbmcSource.FromNameAndPaths("video", orig_source.name, paths);
      g_settings.AddShare("video", xbmcSource);
    }
  }

  //MUSIC
  if (source.isMusic == orig_source.isMusic)
  {
    CLog::Log(LOGDEBUG,"CBoxeeMediaSourceList::addOrEditSource music scan type wasn't change - don't delete or add");
  }
  else
  {
    if (orig_source.isMusic)
    {
      CLog::Log(LOGDEBUG,"CBoxeeMediaSourceList::addOrEditSource source use to be music - erase music source");
      g_settings.DeleteSource("music", orig_source.name, source.path);
    }
    else
    {
      CLog::Log(LOGDEBUG,"CBoxeeMediaSourceList::addOrEditSource source scan type change to music - add it ");
      xbmcSource.FromNameAndPaths("music", source.name, paths);
      g_settings.AddShare("music", xbmcSource);
    }
  }

  //PICTURE
  if (source.isPicture == orig_source.isPicture)
  {
    CLog::Log(LOGDEBUG,"CBoxeeMediaSourceList::addOrEditSource picture scan type wasn't change - don't delete or add");
  }
  else
  {
    if (orig_source.isPicture)
    {
      CLog::Log(LOGDEBUG,"CBoxeeMediaSourceList::addOrEditSource source use to be picture - erase picture source");
      g_settings.DeleteSource("pictures", orig_source.name, source.path);
    }
    else
    {
      CLog::Log(LOGDEBUG,"CBoxeeMediaSourceList::addOrEditSource source scan type change to picture - add it ");
      xbmcSource.FromNameAndPaths("pictures", source.name, paths);
      g_settings.AddShare("pictures", xbmcSource);
    }
  }

  g_settings.SaveSources();
  load();

}

void CBoxeeMediaSourceList::addSource(CBoxeeMediaSource source)
{
  CMediaSource xbmcSource;
  std::vector<CStdString> paths;
  paths.push_back(source.path);
  xbmcSource.m_iScanType = source.scanType;
  xbmcSource.m_strThumbnailImage = source.thumbPath;
  xbmcSource.m_adult = source.isAdult;
  xbmcSource.m_country = source.country;
  xbmcSource.m_countryAllow = source.countryAllow;

  std::string sourceType;
  
  if (source.isVideo)
  {
    xbmcSource.FromNameAndPaths("video", source.name, paths);
    g_settings.AddShare("video", xbmcSource);
	  sourceType = "video";
  }  

  if (source.isMusic)
  {
    xbmcSource.FromNameAndPaths("music", source.name, paths);
    g_settings.AddShare("music", xbmcSource);
	  sourceType = "music";
  }  

  if (source.isPicture)
  {
    xbmcSource.FromNameAndPaths("pictures", source.name, paths);
    g_settings.AddShare("pictures", xbmcSource);
	  sourceType = "pictures";
  }  

  CLog::Log(LOGDEBUG,"CBoxeeMediaSourceList::addSource, new source added, type = %s, path = %s", sourceType.c_str(), source.path.c_str());

  g_settings.SaveSources();
  load();
}
 
BoxeeMediaSourceMap& CBoxeeMediaSourceList::getMap()
{
  return m_sources;
}
  
void CBoxeeMediaSourceList::load()
{  
   m_sources.clear();

   loadShares(*g_settings.GetSourcesFromType("video"), SOURCE_VIDEO);
   loadShares(*g_settings.GetSourcesFromType("music"), SOURCE_MUSIC);
   loadShares(*g_settings.GetSourcesFromType("pictures"), SOURCE_PICTURE); 
}

void CBoxeeMediaSourceList::loadShares(VECSOURCES& shares, int sourceType)
{      
   for (size_t i = 0; i < shares.size(); i++)
   {
     // Check to see if this share was already loaded
     BoxeeMediaSourceMap::iterator findIterator = m_sources.find(shares[i].strName);
     if (findIterator != m_sources.end())
     {
       CBoxeeMediaSource& source = (*findIterator).second;
       if (sourceType == SOURCE_VIDEO) source.isVideo = true;
       else if (sourceType == SOURCE_MUSIC) source.isMusic = true;
       else if (sourceType == SOURCE_PICTURE) source.isPicture = true;
     }
     else
     {
       CBoxeeMediaSource source;
       
       source.name = shares[i].strName;
       source.path = shares[i].strPath;
       source.thumbPath = shares[i].m_strThumbnailImage;
       source.isLocal = !CUtil::IsRemote(shares[i].strPath);
       source.isNetwork = CUtil::IsRemote(shares[i].strPath);
       source.scanType = shares[i].m_iScanType;
       source.isAdult = shares[i].m_adult;
       source.country = shares[i].m_country;
       source.countryAllow = shares[i].m_countryAllow;

       if (sourceType == SOURCE_VIDEO) source.isVideo = true;
       else if (sourceType == SOURCE_MUSIC) source.isMusic = true;
       else if (sourceType == SOURCE_PICTURE) source.isPicture = true;
       
       m_sources[source.name] = source;
     }
   }
}

CBoxeeMediaSource& CBoxeeMediaSourceList::getBySourceName(CStdString name)
{
  BoxeeMediaSourceMap::iterator findIterator = m_sources.find(name);
  if (findIterator != m_sources.end())
  {
    return (*findIterator).second;
  }
  else
  {
    return BOXEE_NULL_SOURCE;
  }
}  

bool CBoxeeMediaSourceList::sourceNameExists(CStdString name)
{
  BoxeeMediaSourceMap::iterator iterator;
  for(iterator = m_sources.begin();iterator != m_sources.end();iterator++)
  {
    if(name.CompareNoCase((*iterator).first) == 0)
    {
      return true;
    }
  }

  return false;
}

CStdString CBoxeeMediaSourceList::suggestSourceName(CStdString path)
{
  CStdString temp = path;
  CUtil::RemoveSlashAtEnd(temp);
  CStdString baseShareName = CUtil::GetFileName(temp);
  CStdString shareName = baseShareName;
  int i = 1;
  while (sourceNameExists(shareName))
  {
    shareName.Format("%s (%d)", baseShareName, i);
    ++i;
  }
  
  return shareName;
}
