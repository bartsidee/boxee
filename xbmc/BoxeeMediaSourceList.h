#ifndef BOXEE_SOURCE_LIST
#define BOXEE_SOURCE_LIST

#include <map>
#include "MediaSource.h"

struct CBoxeeMediaSource
{
  CBoxeeMediaSource() 
  {
    name = "";
    path = "";
    isVideo = false;
    isMusic = false;
    isPicture = false;
    isPrivate = false;
    isAdult = false;
    country = "all";
    countryAllow = true;
    thumbPath = "";
    scanType = CMediaSource::SCAN_TYPE_ONCE;
  }
      
  CStdString name;
  CStdString path;
  bool isVideo;
  bool isMusic;
  bool isPicture;
  bool isPrivate; // TODO: get rid of this field
  int  scanType;
  bool isNetwork;
  bool isLocal;
  CStdString thumbPath;
  bool isAdult;
  CStdString country;
  bool countryAllow;
};

static CBoxeeMediaSource BOXEE_NULL_SOURCE;

typedef std::map<CStdString, CBoxeeMediaSource> BoxeeMediaSourceMap;

class CBoxeeMediaSourceList
{
public:
  CBoxeeMediaSourceList();
  CBoxeeMediaSourceList(VECSOURCES& videoSources, VECSOURCES& musicSources, VECSOURCES& pictureSources);
  void deleteSource(CStdString name);
  void deleteSourceByAppId(CStdString appId);
  void deleteDuplicateApps();
  void addOrEditSource(CBoxeeMediaSource orig_source, CBoxeeMediaSource source);
  void updateSource(CStdString oldName,const CStdString &strUpdateChild, const CStdString &strUpdateValue);
  void addSource(CBoxeeMediaSource source); 
  CBoxeeMediaSource& getBySourceName(CStdString name);
  bool sourceNameExists(CStdString name);
  CStdString suggestSourceName(CStdString path);
  
  BoxeeMediaSourceMap& getMap();

private:
  void load();
  void loadShares(VECSOURCES& shares, int sourceType);
  BoxeeMediaSourceMap m_sources;
};

#endif
