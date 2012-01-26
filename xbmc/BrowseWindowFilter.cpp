
#include "BrowseWindowFilter.h"
#include "VideoInfoTag.h"
#include "MusicInfoTag.h"
#include "utils/log.h"
#include "boxee.h"
#include "Util.h"

#define WINDOWS_FOLDER_SEPARATOR '\\'
#define LINUX_FOLDER_SEPARATOR '/'

CBrowseWindowFilter::CBrowseWindowFilter(int id, const CStdString& strName)
{
	m_iId = id;
	m_strName = strName;
}

CBrowseWindowFilter::~CBrowseWindowFilter()
{
}

CBrowseWindowPropertyFilter::CBrowseWindowPropertyFilter(int id, const CStdString& strName, const CStdString& strProperty) :
CBrowseWindowLocalFilter(id, strName)
{
	m_strProperty = strProperty;
}

bool CBrowseWindowPropertyFilter::Apply(const CFileItem *pItem)
{
	if (!pItem->GetPropertyBOOL(m_strProperty))
		return false;

	return true;
}

CBrowseWindowPropertyValueFilter::CBrowseWindowPropertyValueFilter(int id, const CStdString& strName, const CStdString& strProperty, const CStdString& strPropertyValue) :
CBrowseWindowLocalFilter(id, strName)
{
  m_strProperty = strProperty;
  m_strPropertyValue = strPropertyValue;
}

bool CBrowseWindowPropertyValueFilter::Apply(const CFileItem *pItem)
{
  if (pItem->GetProperty(m_strProperty) != m_strPropertyValue)
  {
    return false;
  }

  return true;
}

CBrowseWindowVideoGenreFilter::CBrowseWindowVideoGenreFilter(int id, const CStdString& strName, const CStdString& strGenre) :
CBrowseWindowLocalFilter(id, strName)
{
	m_strGenre = strGenre;
}

bool CBrowseWindowVideoGenreFilter::Apply(const CFileItem *pItem)
{
  if (pItem->HasVideoInfoTag())
  {
    const CVideoInfoTag* pInfoTag = pItem->GetVideoInfoTag();
    CStdString genreString = pInfoTag->m_strGenre;
    CStdString genre = m_strGenre;
    genre.ToLower(); 
    genreString.ToLower();
    CLog::Log(LOGDEBUG,"CBrowseWindowVideoGenreFilter::Apply, filter = %s, genre = %s (filter)", genreString.c_str(), genre.c_str());
    if (genreString.Find(genre) != -1)
    {
      return true;
    }
    return false;
  }
	return true;
}

CBrowseWindowMediaItemFilter::CBrowseWindowMediaItemFilter(int id, const CStdString& strName) :
CBrowseWindowLocalFilter(id, strName)
{
  
}

bool CBrowseWindowMediaItemFilter::Apply(const CFileItem *pItem)
{
  if (pItem->HasProperty("NeedVerify"))
    return true;
  
  if (!pItem->IsVideo() && !pItem->IsAudio() && !pItem->IsPicture() && !pItem->m_bIsFolder)
    return false;
  
  return true;
}

// video filter

CBrowseWindowVideoFilter::CBrowseWindowVideoFilter(int id, const CStdString& strName) :
CBrowseWindowLocalFilter(id, strName)
{
  
}

bool CBrowseWindowVideoFilter::Apply(const CFileItem *pItem)
{
  if (pItem->HasProperty("NeedVerify"))
    return true;
  
  if (!pItem->IsVideo() && !pItem->m_bIsFolder)
    return false;
  
  return true;
}

// audio filter

CBrowseWindowAudioFilter::CBrowseWindowAudioFilter(int id, const CStdString& strName) :
CBrowseWindowLocalFilter(id, strName)
{
  
}

bool CBrowseWindowAudioFilter::Apply(const CFileItem *pItem)
{
  if (pItem->HasProperty("NeedVerify"))
    return true;
  
  if (!pItem->IsAudio() && !pItem->m_bIsFolder)
    return false;
  
  return true;
}

// picture filter

CBrowseWindowPictureFilter::CBrowseWindowPictureFilter(int id, const CStdString& strName) :
CBrowseWindowLocalFilter(id, strName)
{
  
}

bool CBrowseWindowPictureFilter::Apply(const CFileItem *pItem)
{
  if (pItem->HasProperty("NeedVerify"))
    return true;
  
  if (!pItem->IsPicture() && !pItem->m_bIsFolder)
  {
    return false;
  }
 
  if (pItem->IsApp())
  {
    return false;
  }

  return true;
}

CBrowseWindowAlbumGenreFilter::CBrowseWindowAlbumGenreFilter(int id, const CStdString& strName, const CStdString& strGenreValue) :
  CBrowseWindowLocalFilter(id, strName)
{
  m_strGenreValue = strGenreValue;
}

bool CBrowseWindowAlbumGenreFilter::Apply(const CFileItem *pItem)
{
  if (!pItem->HasMusicInfoTag())
    return false;

  if (pItem->GetMusicInfoTag()->GetGenre().CompareNoCase(m_strGenreValue) != 0)
  {
    CLog::Log(LOGDEBUG,"CBrowseWindowAlbumGenreFilter::Apply, filter = %s, genre = %s (filter)", pItem->GetMusicInfoTag()->GetGenre().c_str(), m_strGenreValue.c_str());
    return false;
  }

  return true;
}

CBrowseWindowTvShowGenreFilter::CBrowseWindowTvShowGenreFilter(int id, const CStdString& strName, const CStdString& strGenreValue) :
CBrowseWindowLocalFilter(id, strName)
{
  m_strGenreValue = strGenreValue;
  m_strGenreValue.ToLower();
}


bool CBrowseWindowTvShowGenreFilter::Apply(const CFileItem *pItem)
{
  if (!pItem->HasVideoInfoTag())
    return false;

  CStdString showGenre = pItem->GetVideoInfoTag()->m_strGenre;
  showGenre.ToLower();

  if (showGenre.Find(m_strGenreValue) == -1)
  {
    return false;
  }

  return true;
}

CBrowseWindowTvShowSourceFilter::CBrowseWindowTvShowSourceFilter(int id, const CStdString& strName, const CStdString& strSourceValue) :
  CBrowseWindowFilter(id, strName)
{
  m_strSourceValue = strSourceValue;
  m_strSourceValue.ToLower();
}

bool CBrowseWindowTvShowSourceFilter::Apply(const CFileItem *pItem)
{
  const CFileItemList* linksList = pItem->GetLinksList();

  if (!linksList)
    return false;

  // If at least one link has the specified source the item is in
  for (int i = 0; i < linksList->Size(); i++)
  {
    CStdString provider = linksList->Get(i)->GetProperty("link-provider");
    provider  = provider.ToLower();
    if (provider.Find(m_strSourceValue) != -1)
    {
      return true;
    }
  }

  return false;

}

//CBrowseWindowFirstLetterFilter::CBrowseWindowFirstLetterFilter(int id, const CStdString& strName, const CStdString& strLetter) :
//  CBrowseWindowFilter(id, strName)
//{
//  m_strLetter = strLetter;
//}
//
//bool CBrowseWindowFirstLetterFilter::Apply(const CFileItem *pItem)
//{
//  // Check if the item label begins with the provided letter
//  if (pItem->GetLabel().Left(1).CompareNoCase(m_strLetter) != 0)
//    return false;
//
//
//  return true;
//}

CBrowseWindowTvShowUnwatchedFilter::CBrowseWindowTvShowUnwatchedFilter(int id, const CStdString& strName, bool bUnwatched) :
CBrowseWindowLocalFilter(id, strName)
{
  m_bUnwatched = bUnwatched;
}

bool CBrowseWindowTvShowUnwatchedFilter::Apply(const CFileItem *pItem)
{
  if (m_bUnwatched && pItem->GetPropertyBOOL("watched"))
    return false;

  return true;
}

CBrowseWindowLocalSourceFilter::CBrowseWindowLocalSourceFilter(int id, const CStdString& strName, const CStdString& sourcePath) :
CBrowseWindowLocalFilter(id, strName)
{
  m_sourcePath = sourcePath;
}

bool CBrowseWindowLocalSourceFilter::Apply(const CFileItem *pItem)
{
  CStdString cleanPath = pItem->m_strPath;

  //in case the path is a DVD folder, use only the first path since they are all under the same source
  int iCommaPos = cleanPath.Find(" , ");
  if (iCommaPos != -1)
  {
    cleanPath = cleanPath.Left(iCommaPos);
  }

  cleanPath.Replace(WINDOWS_FOLDER_SEPARATOR, LINUX_FOLDER_SEPARATOR);

  int protocolIndex;
  CStdString protocol;
  bool RemovingProtocols = true;

  do
  {
    protocolIndex = cleanPath.Find("://");
    if (protocolIndex > 0 && protocolIndex < (int)cleanPath.size())
    {
      protocol = cleanPath.substr(0,protocolIndex);
    }
    else
    {
      protocol.clear();
      RemovingProtocols = false;
      break; //just quit the loop, we don't need to continue in that case
    }

    //allow only smb or locally attached storage such as USB
    if (protocol == "smb" || protocol == "upnp" || protocol == "nfs" || protocol == "afp" || protocol.IsEmpty())
    {
      if (protocol == "upnp")
        CUtil::UrlDecode(cleanPath);

      RemovingProtocols = false;
      break; //we're done here
    }
    else if (protocol == "rar")
    { // for some reason, resolved rar media has an encoded filename
      CUtil::UrlDecode(cleanPath);
    }

    if (RemovingProtocols)
    {
      cleanPath.Delete(0,protocol.size()+3);
    }

  }while (RemovingProtocols);

  if (protocol.IsEmpty())
  {
    //we need to add slash
#ifdef _WIN32
        char folderSeparator = WINDOWS_FOLDER_SEPARATOR;
#else
        char folderSeparator = LINUX_FOLDER_SEPARATOR;
#endif
    if (cleanPath[0] != folderSeparator)
      cleanPath = folderSeparator + cleanPath;
  }

  if (CUtil::IsPasswordHidden(m_sourcePath))
  {
    CUtil::RemovePasswordFromPath(cleanPath,false);
  }

  bool retVal = (cleanPath.Find(m_sourcePath,0)==0);

  CUtil::RemovePasswordFromPath(cleanPath,false);

  if (retVal)
  {
    CLog::Log(LOGDEBUG, "CBrowseWindowLocalSourceFilter::Apply, %s was passed through the filter. (filter)",cleanPath.c_str());
  }
  else
  {
    CLog::Log(LOGDEBUG, "CBrowseWindowLocalSourceFilter::Apply, %s was NOT passed through the filter. (filter)",cleanPath.c_str());
  }

  return retVal;
}

CBrowseWindowTvEpisodeFreeFilter::CBrowseWindowTvEpisodeFreeFilter(int id, const CStdString& strName, bool bFreeOnly) :
CBrowseWindowLocalFilter(id, strName)
{
  m_bFreeOnly = bFreeOnly;
}

bool CBrowseWindowTvEpisodeFreeFilter::Apply(const CFileItem *pItem)
{
  bool hasServiceLink = false;
  const CFileItemList* linkList = pItem->GetLinksList();
  if (linkList)
  {
    for (int i=0; i<linkList->Size() && !hasServiceLink; i++)
    {
      const CFileItemPtr linkItem = linkList->Get(i);
      CStdString linkProvider = linkItem->GetProperty("link-provider");

      hasServiceLink = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().IsRegisterToServices(linkProvider,BOXEE::CServiceIdentifierType::NAME);
    }
  }

  if (m_bFreeOnly && !hasServiceLink && !pItem->GetPropertyBOOL("haslink-free") && !pItem->GetPropertyBOOL("haslink-free-local"))
  {
    return false;
  }

  return true;
}

CBrowseWindowAllowFilter::CBrowseWindowAllowFilter(int id, const CStdString& strName) : CBrowseWindowLocalFilter(id, strName)
{

}

bool CBrowseWindowAllowFilter::Apply(const CFileItem* pItem)
{
  bool isAllow = pItem->IsAllowed();

  //CLog::Log(LOGDEBUG,"CBrowseWindowAllowFilter::Apply - For item [label=%s] going to return [isAllow=%d] (iaf)(filter)",pItem->GetLabel().c_str(),isAllow);

  return isAllow;
}

