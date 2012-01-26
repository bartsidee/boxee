
#include "BoxeeBrowseMenuDirectory.h"
#include "utils/log.h"
#include "BoxeeUtils.h"
#include "lib/libBoxee/boxee.h"
#include "Directory.h"
#include "BoxeeDatabaseDirectory.h"
#include "MetadataResolverVideo.h"
#include "lib/libBoxee/bxutils.h"

using namespace BOXEE;

namespace DIRECTORY
{

CBoxeeBrowseMenuDirectory::CBoxeeBrowseMenuDirectory()
{

}

CBoxeeBrowseMenuDirectory::~CBoxeeBrowseMenuDirectory()
{

}

bool CBoxeeBrowseMenuDirectory::GetDirectory(const CStdString& strPath, CFileItemList &itemsList)
{
  CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuDirectory::GetDirectory - Enter function with [path=%s] (bmd)",strPath.c_str());

  CURI url(strPath);
  CStdString strProtocol = url.GetProtocol();

  // Check if request if for "browsemenu" protocol
  if (strProtocol.CompareNoCase("browsemenu") != 0)
  {
    CLog::Log(LOGERROR,"CBoxeeBrowseMenuDirectory::GetDirectory - Trying to retrieve files with invalid path [%s] - (bmd)",strPath.c_str());
    return false;
  }

  CStdString strMediaType = url.GetHostName();
  CStdString strSection = url.GetShareName();
  std::map<CStdString, CStdString> mapParams = url.GetOptionsAsMap();

  CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuDirectory::GetDirectory - [strPath=%s] was parsed to [strMediaType=%s][section=%s] (bmd)",strPath.c_str(),strMediaType.c_str(), strSection.c_str());

  if (strMediaType == "movies")
  {
    return HandleMoviesRequest(strPath,strMediaType,strSection,mapParams,itemsList);
  }
  else if (strMediaType == "shows")
  {
    return HandleShowsRequest(strPath,strMediaType,strSection,mapParams,itemsList);
  }
  else if (strMediaType == "apps")
  {
    return HandleAppsRequest(strPath,strMediaType,strSection,mapParams,itemsList);
  }
  else if (strMediaType == "trailers")
  {
    return HandleTrailersRequest(strPath,strMediaType,strSection,mapParams,itemsList);
  }

  CLog::Log(LOGERROR,"CBoxeeBrowseMenuDirectory::GetDirectory - FAILED to handle [path=%s] (bmd)",strPath.c_str());

	return true;
}

bool CBoxeeBrowseMenuDirectory::HandleMoviesRequest(const CStdString& strPath,const CStdString& strMediaType,const CStdString& strSection,std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList)
{
  CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuDirectory::HandleMoviesRequest - enter function with [strPath=%s][strMediaType=%s][section=%s][numOfParams=%zu] (bmd)",strPath.c_str(),strMediaType.c_str(),strSection.c_str(),mapParams.size());

  if (strSection == "genres")
  {
    return HandleGenresRequest("movies",strPath,mapParams,itemsList);
  }
  else if (strSection == "trailers")
  {
    return HandleTrailersSectionRequest(strPath,mapParams,itemsList);
  }
  else if (strSection == "providers")
  {
    return HandleProvidersRequest("movies",strPath,mapParams,itemsList);
  }
  else if (strSection == "sources")
  {
    return HandleSourcesRequest(strPath,mapParams,itemsList);
  }

  CLog::Log(LOGERROR,"CBoxeeBrowseMenuDirectory::HandleMoviesRequest - FAILED to handle [strSection=%s]. [strPath=%s][dir=%s][section=%s][numOfParams=%zu] (bmd)",strSection.c_str(),strPath.c_str(),strMediaType.c_str(),strSection.c_str(),mapParams.size());
  return false;
}

bool CBoxeeBrowseMenuDirectory::HandleShowsRequest(const CStdString& strPath,const CStdString& strMediaType,const CStdString& strSection,std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList)
{
  CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuDirectory::HandleShowsRequest - enter function with [strPath=%s][strMediaType=%s][section=%s][numOfParams=%zu] (bmd)",strPath.c_str(),strMediaType.c_str(),strSection.c_str(),mapParams.size());

  if (strSection == "genres")
  {
    return HandleGenresRequest("shows",strPath,mapParams,itemsList);
  }
  else if (strSection == "providers")
  {
    return HandleProvidersRequest("shows",strPath,mapParams,itemsList);
  }

  CLog::Log(LOGERROR,"CBoxeeBrowseMenuDirectory::HandleShowsRequest - FAILED to handle [strSection=%s]. [strPath=%s][strMediaType=%s][section=%s][numOfParams=%zu] (bmd)",strSection.c_str(),strPath.c_str(),strMediaType.c_str(),strSection.c_str(),mapParams.size());
  return false;
}

bool CBoxeeBrowseMenuDirectory::HandleAppsRequest(const CStdString& strPath,const CStdString& strMediaType,const CStdString& strSection,std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList)
{
  CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuDirectory::HandleAppsRequest - enter function with [strPath=%s][strMediaType=%s][section=%s][numOfParams=%zu] (bmd)",strPath.c_str(),strMediaType.c_str(),strSection.c_str(),mapParams.size());

  if (strSection == "categories")
  {
    return HandleAppsCategoriesRequest(strPath,mapParams,itemsList);
  }

  CLog::Log(LOGERROR,"CBoxeeBrowseMenuDirectory::HandleAppsRequest - FAILED to handle [strSection=%s]. [strPath=%s][strMediaType=%s][section=%s][numOfParams=%zu] (bmd)",strSection.c_str(),strPath.c_str(),strMediaType.c_str(),strSection.c_str(),mapParams.size());
  return false;
}

bool CBoxeeBrowseMenuDirectory::HandleTrailersRequest(const CStdString& strPath,const CStdString& strMediaType,const CStdString& strSection,std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList)
{
  CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuDirectory::HandleTrailersRequest - enter function with [strPath=%s][strMediaType=%s][section=%s][numOfParams=%zu] (bmd)",strPath.c_str(),strMediaType.c_str(),strSection.c_str(),mapParams.size());

  if (strSection == "genres")
  {
    return HandleGenresRequest("trailers",strPath,mapParams,itemsList);
  }

  CLog::Log(LOGERROR,"CBoxeeBrowseMenuDirectory::HandleTrailersRequest - FAILED to handle [strSection=%s]. [strPath=%s][strMediaType=%s][section=%s][numOfParams=%zu] (bmd)",strSection.c_str(),strPath.c_str(),strMediaType.c_str(),strSection.c_str(),mapParams.size());
  return false;
}

////////////
// Genres //
////////////

bool CBoxeeBrowseMenuDirectory::HandleGenresRequest(const CStdString& strMedia,const CStdString& strPath,std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList)
{
  CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuDirectory::HandleGenresRequest - enter function with [strMedia=%s][strPath=%s][numOfParams=%zu] (bmd)",strMedia.c_str(),strPath.c_str(),mapParams.size());

  if (strMedia == "movies")
  {
    return HandleMoviesGenresRequest(strPath,mapParams,itemsList);
  }
  else if (strMedia == "shows")
  {
    return HandleShowsGenresRequest(strPath,mapParams,itemsList);
  }
  else if (strMedia == "trailers")
  {
    return HandleTrailersGenresRequest(strPath,mapParams,itemsList);
  }

  CLog::Log(LOGERROR,"CBoxeeBrowseMenuDirectory::HandleGenresRequest - FAILED to handle [strMedia=%s]. [strPath=%s][numOfParams=%zu] (bmd)",strMedia.c_str(),strPath.c_str(),mapParams.size());
  return false;
}

bool CBoxeeBrowseMenuDirectory::HandleMoviesGenresRequest(const CStdString& strPath, std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList)
{
  std::vector<GenreItem> vecGenres;

  const CStdString genreType = mapParams["type"];
  if (genreType == "all")
  {
    CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuDirectory::HandleMoviesGenresRequest - handle request for library genres [genreType=%s]. [strPath=%s][numOfParams=%zu] (bmd)",genreType.c_str(),strPath.c_str(),mapParams.size());
    BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetMovieGenres(vecGenres);
  }
  else if (genreType == "local")
  {
    CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuDirectory::HandleMoviesGenresRequest - handle request for local genres [genreType=%s]. [strPath=%s][numOfParams=%zu] (bmd)",genreType.c_str(),strPath.c_str(),mapParams.size());
    CreateMoviesLocalGenres(vecGenres);
  }
  else
  {
    CLog::Log(LOGERROR,"CBoxeeBrowseMenuDirectory::HandleMoviesGenresRequest - FAILED to handle request for genres type [%s] (bmd)",genreType.c_str());
    return false;
  }

  mapParams["category"] = genreType;
  ConvertGenresItemVecToFileItemList(vecGenres,itemsList,"movies",mapParams);
  return true;
}

bool CBoxeeBrowseMenuDirectory::HandleShowsGenresRequest(const CStdString& strPath,std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList)
{
  std::vector<GenreItem> vecGenres;

  CStdString genreType = mapParams["type"];
  if (genreType == "all")
  {
    CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuDirectory::HandleShowsGenresRequest - handle request for library genres [genreType=%s]. [strPath=%s][numOfParams=%zu] (bmd)",genreType.c_str(),strPath.c_str(),mapParams.size());
    BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetTvGenres(vecGenres);
  }
  else if (genreType == "local")
  {
    CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuDirectory::HandleShowsGenresRequest - handle request for local genres [genreType=%s]. [strPath=%s][numOfParams=%zu] (bmd)",genreType.c_str(),strPath.c_str(),mapParams.size());
    CreateShowsLocalGenres(vecGenres);
  }
  else
  {
    CLog::Log(LOGERROR,"CBoxeeBrowseMenuDirectory::HandleShowsGenresRequest - FAILED to handle request for genres type [%s] (bmd)",genreType.c_str());
    return false;
  }

  mapParams["category"] = genreType;
  ConvertGenresItemVecToFileItemList(vecGenres,itemsList,"shows",mapParams);
  return true;
}

bool CBoxeeBrowseMenuDirectory::HandleTrailersGenresRequest(const CStdString& strPath,std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList)
{
  std::vector<GenreItem> vecGenres;
  BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetMovieGenres(vecGenres);

  mapParams["category"] = "trailers";
  mapParams["section"] = GENRE_SECTION;

  ConvertGenresItemVecToFileItemList(vecGenres,itemsList,"trailers",mapParams);
  return true;
}

void CBoxeeBrowseMenuDirectory::ConvertGenresItemVecToFileItemList(const std::vector<GenreItem>& vecGenres, CFileItemList& itemsList, const CStdString& strMedia, std::map<CStdString, CStdString>& mapParams)
{
  for (size_t i = 0; i<vecGenres.size(); i++)
  {
    CFileItemPtr genreItem(new CFileItem());
    genreItem->SetLabel(vecGenres[i].m_genreText);
    genreItem->SetProperty("genre_label",vecGenres[i].m_genreText);
    genreItem->SetProperty("genre_id",vecGenres[i].m_genreId);
    genreItem->SetProperty("isClickable",true);
    genreItem->m_strPath = "boxeeui://";
    genreItem->m_strPath += strMedia;

    mapParams["genre"] = vecGenres[i].m_genreId;

    genreItem->m_strPath += BoxeeUtils::BuildParameterString(mapParams);

    itemsList.Add(genreItem);
  }
}

void CBoxeeBrowseMenuDirectory::CreateMoviesLocalGenres(std::vector<BOXEE::GenreItem>& outputGenres)
{
  std::set<std::string> setGenres;
  std::vector<GenreItem> vecGenres;

  CMetadataResolverVideo::GetLocalMoviesGenres(setGenres);
  BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetMovieGenres(vecGenres);
  CreateLocalGenres(setGenres,vecGenres, outputGenres);
}

void CBoxeeBrowseMenuDirectory::CreateShowsLocalGenres(std::vector<GenreItem>& outputGenres)
{
  std::set<std::string> setGenres;
  std::vector<GenreItem> vecGenres;

  CMetadataResolverVideo::GetLocalShowsGenres(setGenres);
  BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetTvGenres(vecGenres);
  CreateLocalGenres(setGenres,vecGenres, outputGenres);
}

void CBoxeeBrowseMenuDirectory::CreateLocalGenres(const std::set<std::string>& setGenres,const std::vector<GenreItem>& vecGenres, std::vector<GenreItem>& outputGenres)
{
  for (std::set<std::string>::const_iterator it = setGenres.begin(); it != setGenres.end() ; it++)
  {
    std::string strCurrent = (*it);
    BOXEE::BXUtils::StringToUpper(strCurrent);

    for (std::vector<BOXEE::GenreItem>::const_iterator it2 = vecGenres.begin() ; it2 != vecGenres.end() ; it2++)
    {
      // we need to compare the genre ids we read in the db to those we got from the server
      if (it2->m_genreId.compare(strCurrent) == 0)
      {
        //insert it into the output set
        outputGenres.push_back((*it2));
      }
    }
  }
}

//////////
// Apps //
//////////

bool CBoxeeBrowseMenuDirectory::HandleAppsCategoriesRequest(const CStdString& strPath,const std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList)
{
  CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuDirectory::HandleAppsCategoriesRequest - enter function with [strPath=%s][numOfParams=%zu] (bmd)",strPath.c_str(),mapParams.size());

  std::vector<AppCategoryItem> vecAppCategories;
  BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetAppsCategories(vecAppCategories);
  CreateAppsCategoriesButtons(vecAppCategories, itemsList);
  return true;
}

void CBoxeeBrowseMenuDirectory::CreateAppsCategoriesButtons(std::vector<AppCategoryItem>& vecAppCategories, CFileItemList& itemsList)
{
  for (size_t i = 0; i<vecAppCategories.size(); i++)
  {
    CFileItemPtr appCategoryItem(new CFileItem());
    appCategoryItem->SetLabel(vecAppCategories[i].m_Text);
    appCategoryItem->m_strPath = "boxeeui://apps/?category=all&categoryfilter=";
    appCategoryItem->m_strPath += vecAppCategories[i].m_Id;
    appCategoryItem->SetProperty("isClickable",true);
    itemsList.Add(appCategoryItem);
  }
}

//////////////
// Trailers //
//////////////

bool CBoxeeBrowseMenuDirectory::HandleTrailersSectionRequest(const CStdString& strPath,const std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList)
{
  CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuDirectory::HandleTrailersSectionRequest - enter function with [strPath=%s][numOfParams=%zu] (bmd)",strPath.c_str(),mapParams.size());

  std::vector<BOXEE::TrailerSectionItem> outputTrailers;
  BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetMovieTrailerSections(outputTrailers);
  CreateTrailerSectionButtons(outputTrailers, itemsList);
  return true;
}

void CBoxeeBrowseMenuDirectory::CreateTrailerSectionButtons(std::vector<BOXEE::TrailerSectionItem>& vecTrailer, CFileItemList& itemsList)
{
  for (size_t i = 0; i<vecTrailer.size(); i++)
  {
    CFileItemPtr trailerCategoryItem(new CFileItem());
    trailerCategoryItem->SetLabel(vecTrailer[i].m_Text);

    if (!vecTrailer[i].m_strPath.empty())
    {
      trailerCategoryItem->SetProperty("isClickable",false);

      CStdString path = vecTrailer[i].m_strPath;
      path.Replace("boxee://movies","browsemenu://trailers");
      trailerCategoryItem->SetProperty("child",path);
    }
    else
    {
      trailerCategoryItem->m_strPath.Format("boxeeui://movies/?category=trailers&section=%s",vecTrailer[i].m_Id.c_str());
      trailerCategoryItem->SetProperty("isClickable",true);
      trailerCategoryItem->SetProperty("openInWindow",WINDOW_BOXEE_BROWSE_MOVIES);
    }

    itemsList.Add(trailerCategoryItem);
  }
}

////////////////////////
// Providers/channels //
////////////////////////

bool CBoxeeBrowseMenuDirectory::HandleProvidersRequest(const CStdString& strMedia,const CStdString& strPath,std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList)
{
  CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuDirectory::HandleProvidersRequest - enter function with [strMedia=%s][strPath=%s][numOfParams=%zu] (bmd)",strMedia.c_str(),strPath.c_str(),mapParams.size());

  if (strMedia == "movies")
  {
    return HandleMovieProvidersRequest(strPath,mapParams,itemsList);
  }
  else if (strMedia == "shows")
  {
    return HandleShowsProvidersRequest(strPath,mapParams,itemsList);
  }

  CLog::Log(LOGERROR,"CBoxeeBrowseMenuDirectory::HandleProvidersRequest - FAILED to handle [strMedia=%s]. [strPath=%s][numOfParams=%zu] (bmd)",strMedia.c_str(),strPath.c_str(),mapParams.size());
  return false;
}

bool CBoxeeBrowseMenuDirectory::HandleMovieProvidersRequest(const CStdString& strPath,const std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList)
{
  CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuDirectory::HandleProvidersRequest - enter function with [strPath=%s][numOfParams=%zu] (bmd)",strPath.c_str(),mapParams.size());

  CFileItemList sourceList;
  DIRECTORY::CDirectory::GetDirectory("boxee://sources/movies",sourceList);
  CreateProvidersButtons(sourceList, itemsList);
  itemsList.m_strPath = sourceList.m_strPath;
  return true;
}

bool CBoxeeBrowseMenuDirectory::HandleShowsProvidersRequest(const CStdString& strPath,const std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList)
{
  CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuDirectory::HandleProvidersRequest - enter function with [strPath=%s][numOfParams=%zu] (bmd)",strPath.c_str(),mapParams.size());

  CFileItemList sourceList;
  DIRECTORY::CDirectory::GetDirectory("boxee://sources/shows",sourceList);
  CreateProvidersButtons(sourceList, itemsList);
  itemsList.m_strPath = sourceList.m_strPath;
  return true;
}

void CBoxeeBrowseMenuDirectory::CreateProvidersButtons(const CFileItemList& sourceList, CFileItemList& itemsList)
{
  for (int i=0; i<sourceList.Size(); i++)
  {
    CFileItemPtr sourceItem(new CFileItem());
    sourceItem->SetLabel(sourceList.Get(i)->GetLabel());
    sourceItem->m_strPath = "boxeeui://movies/?category=store&provider=";
    sourceItem->m_strPath += sourceList.Get(i)->GetProperty("sourceid");
    sourceItem->SetProperty("isClickable",true);
    itemsList.Add(sourceItem);
  }
}

/////////////
// Sources //
/////////////

bool CBoxeeBrowseMenuDirectory::HandleSourcesRequest(const CStdString& strPath,const std::map<CStdString, CStdString>& mapParams,CFileItemList &itemsList)
{
  CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuDirectory::HandleSourcesRequest - enter function with [strPath=%s][numOfParams=%zu] (bmd)",strPath.c_str(),mapParams.size());

  // NOTE: currently handling only "video" sources
  VECSOURCES* videoSources = g_settings.GetSourcesFromType("video");
  CreateSourcesButtons(*videoSources,itemsList);
  return true;
}

void CBoxeeBrowseMenuDirectory::CreateSourcesButtons(const VECSOURCES& Sources, CFileItemList& itemsList)
{
  // NOTE: currently handling only "video" sources

  for (VECSOURCES::const_iterator it = Sources.begin(); it != Sources.end(); it++)
  {
    CFileItemPtr sourceItem(new CFileItem());
    sourceItem->SetLabel(it->strName);
    sourceItem->m_strPath = "boxeeui://movies/?category=local&source=";
    sourceItem->m_strPath += it->strPath;
    sourceItem->SetProperty("isClickable",true);
    itemsList.Add(sourceItem);
  }
}

} // namespace


