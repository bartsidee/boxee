#ifndef CBOXEEDATABASEDIRECTORY_H_
#define CBOXEEDATABASEDIRECTORY_H_

#include "IDirectory.h"
#include "lib/libBoxee/bxmetadata.h"

namespace DIRECTORY
{
class CBoxeeDatabaseDirectory : public IDirectory
{
public:
  CBoxeeDatabaseDirectory();
  virtual ~CBoxeeDatabaseDirectory();
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual bool Exists(const char* strPath);
  
  bool GetMovies(std::map<std::string, std::string>& mapParams, CFileItemList &items, int iLimit);

  virtual DIR_CACHE_TYPE GetCacheType(const CStdString& strPath) const { return DIR_CACHE_ALWAYS; };
  
  // Set of functions that create various types of media items
  static bool CreateAudioItem(const BOXEE::BXMetadata* pMetadata, CFileItem* pItem);
  static bool CreateAlbumItem(const BOXEE::BXMetadata* pMetadata, CFileItem* pItem);
  static bool CreateArtistItem(const BOXEE::BXMetadata* pMetadata, CFileItem* pItem);
  static bool CreateDirectoryItem(const BOXEE::BXMetadata* pMetadata, CFileItem* pItem);
  static bool CreateFileItem(const BOXEE::BXMetadata* pMetadata, CFileItem* pItem);
  static bool CreateVideoItem(const BOXEE::BXMetadata* pMetadata, CFileItem* pItem);
  static bool CreatePictureItem(const BOXEE::BXMetadata* pMetadata, CFileItem* pItem);
  static bool CreateSeriesItem(const BOXEE::BXMetadata* pMetadata, CFileItem* pItem);
  static bool CreateSeasonItem(const BOXEE::BXMetadata* pMetadata, CFileItem* pItem);
  
  static CStdString ConstructVideoPath(const BOXEE::BXPath& path);

  static void FillItemDetails(CFileItem *pItem);
  
  static void GetMusicGenres(std::vector<CStdString>& vecGenres);

  static bool CreateShareFilter(const CStdString& strType, std::vector<std::string>& vecPathFilter, bool addSlash = true);

private:
	void CreateFileItemList(std::vector<BOXEE::BXMetadata*> vecMetadataItems, CFileItemList &items);
	void ClearMetadataVector(std::vector<BOXEE::BXMetadata*>& vecMediaItems);
	virtual bool GetArtists(CFileItemList &items, int iLimit);
	virtual bool GetPictures(CFileItemList &items, int iLimit);
	
};

}

#endif /*CBOXEEDATABASEDIRECTORY_H_*/
