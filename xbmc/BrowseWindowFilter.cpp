
#include "BrowseWindowFilter.h"
#include "VideoInfoTag.h"
#include "MusicInfoTag.h"
#include "utils/log.h"
#include "boxee.h"

CBrowseWindowFilter::CBrowseWindowFilter() {
	// TODO Auto-generated constructor stub

}

CBrowseWindowFilter::CBrowseWindowFilter(int id, const CStdString& strName)
{
	m_iId = id;
	m_strName = strName;
}

CBrowseWindowFilter::~CBrowseWindowFilter() {
	// TODO Auto-generated destructor stub
}

CBrowseWindowPropertyFilter::CBrowseWindowPropertyFilter(int id, const CStdString& strName, const CStdString& strProperty) :
	CBrowseWindowFilter(id, strName)
{
	m_strProperty = strProperty;
}

bool CBrowseWindowPropertyFilter::Apply(const CFileItem *pItem)
{
	if (!pItem->GetPropertyBOOL(m_strProperty))
		return false;

	return CBrowseWindowFilter::Apply(pItem);
}

CBrowseWindowPropertyValueFilter::CBrowseWindowPropertyValueFilter(int id, const CStdString& strName, const CStdString& strProperty, const CStdString& strPropertyValue) :
  CBrowseWindowFilter(id, strName)
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

  return CBrowseWindowFilter::Apply(pItem);
}

CBrowseWindowVideoGenreFilter::CBrowseWindowVideoGenreFilter(int id, const CStdString& strName, const CStdString& strGenre) :
	CBrowseWindowFilter(id, strName)
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
	return CBrowseWindowFilter::Apply(pItem);
}

CBrowseWindowMediaItemFilter::CBrowseWindowMediaItemFilter(int id, const CStdString& strName) :
  CBrowseWindowFilter(id, strName)
{
  
}

bool CBrowseWindowMediaItemFilter::Apply(const CFileItem *pItem)
{
  if (pItem->HasProperty("NeedVerify"))
    return true;
  
  if (!pItem->IsVideo() && !pItem->IsAudio() && !pItem->IsPicture() && !pItem->m_bIsFolder)
    return false;
  
  return CBrowseWindowFilter::Apply(pItem);
}

// video filter

CBrowseWindowVideoFilter::CBrowseWindowVideoFilter(int id, const CStdString& strName) :
  CBrowseWindowFilter(id, strName)
{
  
}

bool CBrowseWindowVideoFilter::Apply(const CFileItem *pItem)
{
  if (pItem->HasProperty("NeedVerify"))
    return true;
  
  if (!pItem->IsVideo() && !pItem->m_bIsFolder)
    return false;
  
  return CBrowseWindowFilter::Apply(pItem);
}

// audio filter

CBrowseWindowAudioFilter::CBrowseWindowAudioFilter(int id, const CStdString& strName) :
  CBrowseWindowFilter(id, strName)
{
  
}

bool CBrowseWindowAudioFilter::Apply(const CFileItem *pItem)
{
  if (pItem->HasProperty("NeedVerify"))
    return true;
  
  if (!pItem->IsAudio() && !pItem->m_bIsFolder)
    return false;
  
  return CBrowseWindowFilter::Apply(pItem);
}

// picture filter

CBrowseWindowPictureFilter::CBrowseWindowPictureFilter(int id, const CStdString& strName) :
  CBrowseWindowFilter(id, strName)
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

  return CBrowseWindowFilter::Apply(pItem);
}

CBrowseWindowAlbumGenreFilter::CBrowseWindowAlbumGenreFilter(int id, const CStdString& strName, const CStdString& strGenreValue) :
  CBrowseWindowFilter(id, strName)
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

  return CBrowseWindowFilter::Apply(pItem);
}

CBrowseWindowTvShowGenreFilter::CBrowseWindowTvShowGenreFilter(int id, const CStdString& strName, const CStdString& strGenreValue) :
  CBrowseWindowFilter(id, strName)
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

  return CBrowseWindowFilter::Apply(pItem);
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

  return CBrowseWindowFilter::Apply(pItem);
}

CBrowseWindowFirstLetterFilter::CBrowseWindowFirstLetterFilter(int id, const CStdString& strName, const CStdString& strLetter) :
  CBrowseWindowFilter(id, strName)
{
  m_strLetter = strLetter;
}

bool CBrowseWindowFirstLetterFilter::Apply(const CFileItem *pItem)
{
  // Check if the item label begins with the provided letter
  if (pItem->GetLabel().Left(1).CompareNoCase(m_strLetter) != 0)
    return false;


  return CBrowseWindowFilter::Apply(pItem);
}

CBrowseWindowTvShowUnwatchedFilter::CBrowseWindowTvShowUnwatchedFilter(int id, const CStdString& strName, bool bUnwatched) :
  CBrowseWindowFilter(id, strName)
{
  m_bUnwatched = bUnwatched;
}

bool CBrowseWindowTvShowUnwatchedFilter::Apply(const CFileItem *pItem)
{
  if (m_bUnwatched && pItem->GetPropertyBOOL("watched"))
    return false;

  return CBrowseWindowFilter::Apply(pItem);
}

CBrowseWindowTvEpisodeFreeFilter::CBrowseWindowTvEpisodeFreeFilter(int id, const CStdString& strName, bool bFreeOnly) :
  CBrowseWindowFilter(id, strName)
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

  if (m_bFreeOnly && !hasServiceLink && !pItem->GetPropertyBOOL("free-play") && !pItem->GetPropertyBOOL("free-play-local"))
  {
    return false;
  }

  return CBrowseWindowFilter::Apply(pItem);
}

CBrowseWindowAllowFilter::CBrowseWindowAllowFilter(int id, const CStdString& strName) : CBrowseWindowFilter(id, strName)
{

}

bool CBrowseWindowAllowFilter::Apply(const CFileItem* pItem)
{
  bool isAllow = pItem->IsAllowed();

  CLog::Log(LOGDEBUG,"CBrowseWindowAllowFilter::Apply - For item [label=%s] going to return [isAllow=%d] (iaf)(filter)",pItem->GetLabel().c_str(),isAllow);

  return isAllow;
}

