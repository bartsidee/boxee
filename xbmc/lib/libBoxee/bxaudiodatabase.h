// Copyright © 2008 BOXEE. All rights reserved.
#ifndef BXMUSICDATABASE_H_
#define BXMUSICDATABASE_H_

#include "bxdatabase.h"

namespace BOXEE
{

class BXMetadata;
class BXArtist;
class BXAlbum;
class BXAudio;
class BXFolder;

class BXAudioDatabase : public BOXEE::BXDatabase
{
public:
  BXAudioDatabase();
  virtual ~BXAudioDatabase();
	
  int AddArtist(const BXArtist* pArtist);
  int AddAlbum(const BXAlbum* pAlbum);
  int AddAudio(const BXAudio* pAudio);
  
  int UpdateAlbum(const BXAlbum* pAlbum);
  int UpdateAlbumStatus(int iId, int iStatus);
  void ResetAlbumStatus();
    
  int GetAudios(std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit);

  bool GetAlbums(std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit);
  bool SearchMusic(const std::string& strTitle, std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit);
  bool CreateAlbumsFromDataset(dbiplus::Dataset* pDS, std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit);

  int GetArtists(std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit, const std::string& strName = "");
  
  /**
   * Gets all unresolved albums from the database
   * TODO: Check whether it is ok resolve all albums and not only the albums that belong to the user
   * In most cases there is only one user
   * Unresolved albums are albums with status NEW, UNRESOLVED_ONCE and UNRESOLVED_TWICE
   */
  int GetUnresolvedAlbums(std::vector<BXAlbum*> &vecUnresolvedAlbums);
  
  // NOT USED, DELETE WHEN NO LONGER REQUIRED
  int GetAlbums2(std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit);
 
 
  int GetSongsFromAlbum(int iAlbumId, std::vector<BXMetadata*> &vecMediaFiles);
  
  bool GetArtistByName(const std::string& strName, BXArtist* pArtist);
  bool GetArtistById(int iArtistId, BXArtist* pArtist);
  int GetAllArtists(std::map<std::string, BXArtist> &mapArtists);
  
  bool GetAlbumById(int iAlbumId, BXAlbum* pAlbum, bool bGetDropped = false);

  int GetAlbumByPath(const std::string& strPath, BXAlbum* pAlbum, bool bGetDropped = false);
  int GetAlbumsByArtist(int iArtistId, std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter);
  
  bool GetAlbum(const std::string& strAlbum, const std::string& strArtist, BXMetadata* pMetadata,  bool bGetDropped = false);

  int GetMusicGenres(std::vector<std::string>& vecGenres);

  /**
   * Used to get album by title and artist id, like in case of groups
   */ 
  int GetAlbumsByTitleAndArtist(const std::string& strTitle, int iArtistId, std::vector<BXMetadata*> &vecMediaFiles, bool bGetDropped = false);
  int GetAlbumsByTitleAndArtist(const std::string& strTitle, const std::string& strArtist, std::vector<BXMetadata*> &vecMediaFiles, bool bGetDropped = false);
  
  int GetAudioByPath(const std::string& strPath, BXMetadata* pMetadata);
  
  int GetAudiosByFolder(std::map<std::string, BXMetadata*> &mapMediaFiles, const std::string& strFolderPath);
  
  int RemoveAudioByPath(const std::string& strPath);
  int RemoveAudioByFolder(const std::string& strFolderPath);
  int RemoveAlbumById(int iId);
  
  bool DropAlbumById(int iId);
	
  virtual bool UpdateTables() { return true; }

  int AddAudioFile(const std::string& strPath, const std::string& strSharePath, int FolderId, int64_t size);
  int UpdateAudioFileStatus(const std::string& strPath, int iStatus, int idVideo=-1);
  int RemoveUnresolvedAudioByPath(const std::string& strPath);
  int RemoveUnresolvedAudioByFolder(const std::string& strFolder);
  int GetShareUnresolvedAudioFilesCount(const std::string& iStrSharePath, int iStatus);
  int GetUserUnresolvedAudioFilesCount(const std::string& iSharesList, int iStatus);
  bool AreAudioFilesBeingScanned(const std::string& iSharesList);
  int GetUnresolvedAudioFilesByFolder(std::map<std::string, BXMetadata*> &mapMediaFiles, int folderId);


private:
  
  bool LoadAlbumFromDataset(dbiplus::Dataset* pDS, BOXEE::BXAlbum* pAlbum);
  bool LoadAudioFromDataset(dbiplus::Dataset* pDS, BOXEE::BXAudio* pAudio);
  bool LoadArtistFromDataset(dbiplus::Dataset* pDS, BOXEE::BXArtist* pArtist);
};

}

#endif /*BXMUSICDATABASE_H_*/
