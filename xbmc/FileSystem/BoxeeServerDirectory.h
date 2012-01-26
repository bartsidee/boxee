#ifndef CBOXEESERVERDIRECTORY_H_
#define CBOXEESERVERDIRECTORY_H_

#include "IDirectory.h"
#include <vector>

namespace DIRECTORY
{

/**
 * The purpose of this class is to return media items from both the Boxee Server 
 * and local media database. The following types of queries are supported:
 * 
From TV Shows Browse Screen:

1. Most popular by Genre (including All Genres) (remote only) - 100 results
2. A to Z by Genre (remote and local) (w/ pagination)
3. By Letter by Genre (remote and local) (w/ pagination)
4. Search (remote + local)
5. * My shows by Genre (send list of ids and return list of shows) - will be implemented in client

From Movies Browse Screen (no pagination in all queries)

1. Most popular by Genre (remote only)
2. Release date by Genre (remote only)
3. IMDB rating by Genre (remote only)
4. Local movies sorted by AtoZ by Genre
5. Local sorted by recently added by Genre
6. Search (remote + local) no Filter
 * 
 */
class CBoxeeServerDirectory : public IDirectory
{
public:
  CBoxeeServerDirectory();
  virtual ~CBoxeeServerDirectory();
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual bool Exists(const char* strPath);
  
  virtual DIR_CACHE_TYPE GetCacheType(const CStdString& strPath) const;
  
  /*
   * The function checks whether an option with the specified name exists in the map and sets optionValue
   * output parameter if it does
   * returns true if option exists and false otherwise
   */
  static bool HasOption(const std::map<CStdString, CStdString>& mapOptions, const CStdString& optionName, CStdString& optionValue);
  
  static void AddParametersToRemoteRequest(std::map<CStdString, CStdString>& mapRemoteOptions);

private:

  void MergeByBoxeeId(CFileItemList& left, CFileItemList& right);

  bool HandleTvShows(const CStdString& strRequest, std::map<CStdString, CStdString>& mapOptions, CFileItemList &items);
  bool HandleMovies(const CStdString& strRequest, std::map<CStdString, CStdString>& mapOptions, CFileItemList &items);
  bool HandleTrailers(const CStdString& strRequest, std::map<CStdString, CStdString>& mapOptions, CFileItemList &items);
  bool HandleClips(const CStdString& strRequest, std::map<CStdString, CStdString>& mapOptions, CFileItemList &items);
  bool HandleGlobalSearch(const CStdString& strRequest, std::map<CStdString, CStdString>& mapOptions, CFileItemList &items);
  bool HandleSources(const CStdString& strRequest, std::map<CStdString, CStdString>& mapOptions, CFileItemList &items);
  bool GetTvShowSubscriptions(CFileItemList &items);
  };

}

#endif /*CBOXEESERVERDIRECTORY_H_*/
