#ifndef METADATARESOLVER_H_
#define METADATARESOLVER_H_

#include <string>
#include <vector>

#include <time.h>

#include "FileItem.h"
#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bximetadataresolver.h"
#include "lib/libBoxee/bxmetadata.h"
#include "../utils/md5.h"
#include "IProgressCallback.h"

#define RESOLVED 0
#define NOT_RESOLVED -1
#define NETWORK_ERROR -2

class CMetadataResolver : public BOXEE::BXIMetadataResolver 
{
private:
	bool m_bStopped;

public:
	CMetadataResolver();
	virtual ~CMetadataResolver();
	virtual std::string GetId() { return "XBMC Metadata Resolver v1.0"; }
	virtual bool Stop();

	virtual int Resolve(BOXEE::BXMetadataScannerJob * pJob, std::vector<BOXEE::BXFolder*>& vecResults );
	virtual bool CheckPath(const std::string& strPath);

	static long GetMediaFileDurationInMin(const CStdString &strPath);
	static std::vector<CStdString> m_badWords;
	static bool m_bInitialized;
	
	static void CacheThumb(const CStdString& strItemPath, const CStdString& strThumbnailPath, bool bOverwrite = false);

private:

	void UpdateResolveLabel(const CStdString& path);

  int ResolveMusicFolder(BOXEE::BXMetadataScannerJob* pJob);
	int ResolveVideoFolder(BOXEE::BXMetadataScannerJob* pJob);

  void PurgeDeletedVideosFromDatabase(const CStdString& strFolderPath, const CFileItemList& items);
  bool ResolveDvdFolder(const CStdString& strFolderPath, int iFolderId, const CFileItemList& items, int& iResult);
  bool ResolveTwoVideoFolder(std::vector<CFileItemPtr>& items);
  bool ResolveBlurayFolder(const CStdString& strFolderPath, int iFolderId, const CFileItemList& items, int& iResult);

  //void UpdateMovieHash(const CStdString& strPath, int64_t iLength, std::map<std::string, std::pair<std::string, std::string> >& mapHashSizeToImdbId);

	void CleanDeletedVideos(const std::map<std::string, BOXEE::BXMetadata*> &mapDeletedVideos);
	void CleanDeletedFolders(const std::set<std::string> &setFolders);
	
};

#endif /*METADATARESOLVER_H_*/
