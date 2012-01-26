#ifndef UPDATE_SOURCE_FILE_H_
#define UPDATE_SOURCE_FILE_H_

#include "MediaSource.h"
#include "tinyXML/tinyxml.h"

class CUpdateSourceFile
{
public:

  CUpdateSourceFile();
  virtual ~CUpdateSourceFile();
  
  bool UpdateProfilesSourceFile();
  bool UpgradeSettings();
  
protected:
  enum UPDATE_ACTION_TYPE {UPDATE_ADD,UPDATE_DELETE};
  void UpdateSourceFiles(const TiXmlNode* pAddTag, UPDATE_ACTION_TYPE updateActionType);
  void UpdateSourceFile(const TiXmlNode* pMediaTypeTag, const CStdString& mediaType, UPDATE_ACTION_TYPE updateActionType);
  void InitMediaSourceFromNode(const TiXmlNode* pSourceTag,CMediaSource& source, const CStdString& mediaType);
  bool IsNameExistInSource(const CStdString &strPath,const CStdString &mediaType);
  
  bool ReadSourcesFromFile(const CStdString& profilesFilePath,VECSOURCES& videoSources,VECSOURCES& musicSources,VECSOURCES& pictureSources);
  bool FixOldStyleDir(TiXmlElement* element);
  void DeleteDuplicateApps();
};

#endif /*UPDATE_PROFILE_SOURCE_FILE_H_*/
