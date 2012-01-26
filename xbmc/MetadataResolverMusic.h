#ifndef METADATARESOLVERMUSIC_H_
#define METADATARESOLVERMUSIC_H_

#include <string>
#include <vector>

#include <time.h>

#include "FileItem.h"
#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bximetadataresolver.h"
#include "lib/libBoxee/bxmetadata.h"
#include "lib/libBoxee/bxxmldocument.h"
#include "../utils/md5.h"
#include "IProgressCallback.h"
#include "utils/MusicAlbumInfo.h"
#include "utils/MusicInfoScraper.h"

using namespace BOXEE;

typedef std::vector<BXMetadata> vectorMetadata;

class CResolvingTrack {
public:
  CResolvingTrack(const CStdString& _strPath) 
  {
    strPath = _strPath;
    bLoaded = false;
    iTrackNumber = -1;
    iDuration = 0;
  }

  CStdString strPath;
  CStdString strTitle;
  CStdString strAlbum;
  CStdString strArtist;
  CStdString strGenre;
  CStdString strAlbumArtist;
  int iYear;

  bool bLoaded;
  int iTrackNumber;
  int iDuration;

  CStdString strFolderPath;
  CStdString strEffectiveFolderName;

  void LoadTags();

};

class CResolvedAlbum {
public:

  CResolvedAlbum(const CStdString& _strName, const CStdString& _strArtist, const CStdString& _strFolderPath, const CStdString& _strGenre, int _iYear);

  // name of the album
  CStdString strName;
  // album artist
  CStdString strArtist;

  // the path of the folder holding the album
  CStdString strFolderPath;
  // effective name of the album folder (skipping CD1, disk1 or similar)
  CStdString strEffectiveFolderName;

  // Album can be either found locally or from a remote metadata provider
  CStdString strCover;

  CStdString strGenre;
  int iYear;

  // index of the album and artist in the database
  int iAlbumDbIndex;
  int iArtistDbIndex;

  bool bResolved; // indicates whether the album was resolved from the internet

  std::vector<CResolvingTrack> vecTracks;
};

class CResolvingFolder {
public:
  CStdString strFolderPath;
  CStdString strEffectiveFolderName;
  std::vector<CResolvingTrack> vecTracks;
  std::vector<CStdString> vecThumbs;
};

class CMetadataResolverMusic : public BOXEE::BXIAlbumResolver
{

public:
  CMetadataResolverMusic();
  virtual ~CMetadataResolverMusic();

  /**
   * Get remote metadata from the internet, updates resolved albums
   * Implementation of the BXIAlbumResolver interface for the second stage of album resolving
   */
  void ResolveAlbumMetadata(std::vector<BOXEE::BXAlbum*>& albums);

  virtual bool Stop();

  // Static functions

  /**
   * Performs music resolver initialization
   */
  static void InitializeAudioResolver();

  /**
   * Resolves music folder with provided path
   * @returns RESOLVER_SUCCESS on success, RESOLVER_FAILED on failure, RESOLVER_ABORTED on abort
   */
  static int ResolveMusicFolder(BXMetadataScannerJob* pJob, bool rescan=false);

  /**
   * Get remote metadta from the internet for a single album, Used by the BoxeeFeedItemLoader
   */
  static bool ResolveAlbumMetadata(const CStdString& _strAlbum, const CStdString& _strArtist,  BOXEE::BXMetadata * pMetadata);

  // helper functions for connecting and receiving info from the server
  static bool LoadAlbumsInfo(BOXEE::BXXMLDocument& doc, vectorMetadata& list);
  static bool LoadAlbumInfo(BOXEE::BXXMLDocument& doc , BXMetadata& albumRead);
  static bool LoadAlbumInfo(TiXmlElement* albumElement, BXMetadata& albumRead);

  static bool GetResultsFromServer(const CStdString& _strAlbum,const CStdString& _strArtist, const int _resultCount, BXXMLDocument& _resultDoc);

private:

  static int ReadMusicFolder(const CStdString& strPath, CResolvingFolder& folder, BXMetadataScannerJob* pJob);

  /**
   * Analyzes the folder tracks and creates one or more albums from them
   */
  static int CreateAlbumsFromFolder(const CResolvingFolder& folder, std::vector<CResolvedAlbum>& albums, BXMetadataScannerJob* pJob);

  /**
   * Look for the local thumbnail (album cover) in the album folder
   * @return true if local cover was found, album strCover member is updated with thumbnail path
   */
  static bool FindLocalThumbnail(CResolvedAlbum& album, const CResolvingFolder& folder);

  /**
   * Add resolved album and all the tracks inside in the metadata
   * Album is passed by reference and is updated with album and artist database ids
   * Encapsulates boxee database API
   */
  static bool AddAlbumToDatabase(CResolvedAlbum& album);

  static unsigned int GetLevenshteinDistance(const CStdString& s1, const CStdString& s2);

  /**
   * Finds best matching album in the vector
   * @return the index of the album or -1 if no match was found
   */
  static int FindAlbumInVector(const std::vector<CResolvedAlbum> &albums, const CStdString& _strAlbum, const CStdString& _strArtist);

  /**
   * Synchronizes between the tracks in the folder and tracks that appear in the database under that folder
   * Removes tracks that were already resolved from the folder and 
   * removes tracks that no longer exist in the folder from the database
   */
  static void SynchronizeFolderAndDatabase(CResolvingFolder& folder);

  static void CleanDeletedAudiosFromDatabase(const std::map<std::string, BOXEE::BXMetadata*> &mapDeletedAudios);

  static bool GetMetadataFromServer(const CStdString& _strAlbum, const CStdString& _strArtist, BOXEE::BXMetadata * pMetadata);

  // Returns 0 if successful, -1 if information was not found and -2 if information was not retreived due to network 
  // problems
  static bool GetMetadataFromAMG(const CStdString& _strAlbum, const CStdString& _strArtist, MUSIC_GRABBER::CMusicAlbumInfo& album);

  static bool m_bStopped;
  static bool m_bInitialized;

  static std::vector<CResolvedAlbum> albumCache;

public:

};

#endif /*METADATARESOLVER_H_*/
