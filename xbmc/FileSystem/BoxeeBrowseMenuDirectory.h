#ifndef BOXEEBROWSEMENUDIRECTORY_H_
#define BOXEEBROWSEMENUDIRECTORY_H_

#include "IDirectory.h"
#include "MediaSource.h"
#include "lib/libBoxee/bxgenresmanager.h"
#include "lib/libBoxee/bxappboxmanager.h"
#include "lib/libBoxee/bxtrailersmanager.h"

#include <vector>
#include <set>

using namespace BOXEE;

namespace DIRECTORY
{

class CBoxeeBrowseMenuDirectory : public IDirectory
{
public:

  CBoxeeBrowseMenuDirectory();
  virtual ~CBoxeeBrowseMenuDirectory();
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);

private:
  
  bool HandleMoviesRequest(const CStdString& strPath,const CStdString& strDir,const CStdString& strFile,std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList);
  bool HandleShowsRequest(const CStdString& strPath,const CStdString& strDir,const CStdString& strFile,std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList);
  bool HandleAppsRequest(const CStdString& strPath,const CStdString& strDir,const CStdString& strFile,std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList);
  bool HandleTrailersRequest(const CStdString& strPath,const CStdString& strMediaType,const CStdString& strSection,std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList);

  ////////////
  // genres //
  ////////////

  bool HandleGenresRequest(const CStdString& strMedia,const CStdString& strPath,std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList);
  bool HandleMoviesGenresRequest(const CStdString& strPath,std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList);
  bool HandleShowsGenresRequest(const CStdString& strPath,std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList);
  bool HandleTrailersGenresRequest(const CStdString& strPath,std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList);
  void ConvertGenresItemVecToFileItemList(const std::vector<GenreItem>& vecGenres, CFileItemList& itemsList, const CStdString& strMedia, std::map<CStdString, CStdString>& mapParams);
  void CreateMoviesLocalGenres(std::vector<BOXEE::GenreItem>& outputGenres);
  void CreateShowsLocalGenres(std::vector<GenreItem>& outputGenres);
  void CreateLocalGenres(const std::set<std::string>& setGenres,const std::vector<GenreItem>& vecGenres, std::vector<GenreItem>& outputGenres);

  //////////////
  // trailers //
  //////////////

  bool HandleTrailersSectionRequest(const CStdString& strPath,const std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList);
  void CreateTrailerSectionButtons(std::vector<BOXEE::TrailerSectionItem>& vecTrailer, CFileItemList& itemsList);

  ////////////////////////
  // providers/channels //
  ////////////////////////

  bool HandleProvidersRequest(const CStdString& strMedia,const CStdString& strPath,std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList);
  bool HandleMovieProvidersRequest(const CStdString& strPath,const std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList);
  bool HandleShowsProvidersRequest(const CStdString& strPath,const std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList);
  void CreateProvidersButtons(const CFileItemList& sourceList, CFileItemList& itemsList);

  /////////////
  // sources //
  /////////////

  bool HandleSourcesRequest(const CStdString& strPath,const std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList);
  void CreateSourcesButtons(const VECSOURCES& Sources,CFileItemList& itemsList);

  //////////
  // apps //
  //////////

  bool HandleAppsCategoriesRequest(const CStdString& strPath,const std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList);
  void CreateAppsCategoriesButtons(std::vector<AppCategoryItem>& vecCategories, CFileItemList& itemsList);

};

}

#endif /*BOXEEBROWSEMENUDIRECTORY_H_*/
