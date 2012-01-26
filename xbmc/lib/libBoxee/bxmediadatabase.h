// Copyright © 2008 BOXEE. All rights reserved.
#ifndef BXMEDIADATABASE_H_
#define BXMEDIADATABASE_H_

#include "bxdatabase.h"
#include "bxmetadata.h"
#include <SDL/SDL.h>
#include <set>

namespace BOXEE {

class BXMediaFile;
class BXFolder;

// INTERMEDIATE STATES,
#define MEDIA_FILE_STATUS_NEW "new"                 // folder has just been added to the database
#define MEDIA_FILE_STATUS_RESOLVING "resolving"     // folder is being resolving right now
#define MEDIA_FILE_STATUS_PENDING "pending"         // folder has been queued for resolving
#define MEDIA_FILE_STATUS_ABORTED "aborted"         // folder resolving has been aborted in the middle
#define MEDIA_FILE_STATUS_UNAVAILABLE "unavailable" // folder content is not available (due to network or disconnected drives)

// FINAL FOLDER STATE
#define MEDIA_FILE_STATUS_RESOLVED "resolved"
#define MEDIA_FILE_STATUS_UNRESOLVED "unresolved" // unresolved folder will be retried FOLDER_RETRY number of times
#define FOLDER_RETRY 1

/**
 * A singleton class that proivdes interface to Boxee media database
 */
class BXMediaDatabase : public BOXEE::BXDatabase
{

protected:

  // Flag that indicates whether new files are available
  static bool m_bHasNewFiles;



public:
  BXMediaDatabase();
  virtual ~BXMediaDatabase();

  virtual bool TablesExist();

  /**
   * Creates ALL database templates, this function is called from the
   * BXDatabase implementation and only during initialization phase
   */
  virtual bool CreateTables();
  
  bool CleanTables();

  /**
   * Perform migration of the database from the detected version to the latest version
   */
  virtual bool UpdateTables();

  int AddMediaFolder(const std::string& strPath, const std::string& strType, int iDateModified);

  int GetMediaFolderId(const std::string& strPath, const std::string& strType);

  int RemoveFolderByPath(const std::string& strPath);
	
  /**
   * Resets all files that are in intermediate state resolving
   * to 'new' state. This is done in order to resolve files that were interrupted
   * when the system was unexpectedly shutdown
   */
  bool ResetUnresolvedFiles();

  /**
   * Clears all metadata that has been collected in the database
   * and resets the status of all files in the media_files table to 'new'
   * Used mostly for debugging and activated through a configuration file
   */
  int ResetMetadata();
  
  
  int UpdateFolderResolving(const BXFolder* pFolder);  
  int UpdateFolderResolved(const std::string& strPath, int iMetadataId);
  int UpdateFolderUnresolved(const std::string& strPath , bool bDecreaseNeedsRescan = true);
  int UpdateFolderAborted(const std::string& strPath);
  int UpdateFolderNew(const std::string& strPath);
  bool IsFolderBeingResolved(const std::string& strPath);
  int UpdateFolderTreeNew(const std::string& strPath);
  int UpdateFolderAvailability(const std::string& strPath, bool bAvailable);

  int SetFolderMetadata(int iFolderId, int iMetadataId);
  bool UpdateIfModified(const std::string& strPath, int iModDate, bool & bModified); 

  int GetUnresolvedFolders(std::vector<BXFolder*>& vecFolders, const std::string& strType);
  int GetUnresolvedFoldersByPath(const std::string& strPath, std::vector<BXMetadata*>& vecFolders);
  int GetMediaFolders(std::vector<BXFolder*>& vecFolders, const std::string& strType);
  int GetChildFolders(std::set<std::string> &setFolders, const std::string &strPath, bool getImmediate = true);
  int GetFolderCount(const std::string &strSharePath, const std::string &strStatus);
  int GetUnresolvedVideoFolders(std::vector<BXMetadata*>& vecFolders, const std::vector<std::string>& vecPathFilter, int iItemLimit);

  //if you're searching for the watched / unwatched functions, it was moves to a new class bxuserprofiledatabase

  bool AddQueueItem(const std::string& strLabel, const std::string& strPath, const std::string& strThumbPath, const std::string& strClientId);
  bool GetQueueItem(const std::string& strClientId, std::string& strLabel, std::string& strPath, std::string& strThumbPath);
  bool RemoveQueueItem(const std::string& strClientId);

  bool AddMediaShare(const std::string& strLabel, const std::string& strPath, const std::string& strType, int iScanType);
  bool UpdateMediaShare(const std::string& strOrgLabel, const std::string& strOrgType, const std::string& strLabel, const std::string& strPath, const std::string& strType, int iScanType);
  bool DeleteMediaShare(const std::string& strLabel, const std::string& strPath, const std::string& strType);
  bool GetScanTime(const std::string& strLabel, const std::string& strPath, const std::string& strType, time_t& iLastScanned);
  bool UpdateScanTime(const std::string& strLabel, const std::string& strPath, const std::string& strType, time_t iLastScanned);

  unsigned int GetFolderDate(const std::string& strPath);

  int AddProviderPerf(const std::string& strProvider, int quality);
  int UpdateProviderPerf(const std::string& strProvider, int quality);
  int GetProviderPerfQuality(const std::string& strProvider);

  int AddPlayableFolder(const std::string& strPath);
  bool IsPlayableFolder(const std::string& strPath);
};

}

#endif /*BXMEDIADATABASE_H_*/
