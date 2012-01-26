
#include "GUIDialogBoxeeManualResolve.h"
#include "GUIDialogBoxeeManualResolveMovie.h"
#include "GUIDialogBoxeeManualResolveEpisode.h"
#include "GUIWindowManager.h"
#include "Util.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "MetadataResolverVideo.h"
#include "MetadataResolver.h"
#include "BoxeeUtils.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "lib/libBoxee/bxmetadataengine.h"
#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxutils.h"
#include "lib/libBoxee/bxmetadata.h"
#include "tinyXML/tinyxml.h"
#include "GUIEditControl.h"
#include "RssSourceManager.h"
#include "MetadataResolver.h"
#include "GUIDialogOK2.h"
#include "SpecialProtocol.h"
#include "FileSystem/Directory.h"
#include "FileSystem/BoxeeDatabaseDirectory.h"
#include "Picture.h"
#include "FileSystem/File.h"
#include "GUIDialogYesNo2.h"

using namespace BOXEE;

#define TITLE_EDIT 5010
#define BTN_SEARCH 5020
#define BTN_DROP   5025
#define RESULT_LIST 5030

CGetResultListBG::CGetResultListBG(const CStdString& strTitle, CFileItemList& items) : m_items(items)
{
  m_strTitle = strTitle;
}

void CGetResultListBG::Run()
{
  CUtil::URLEncode(m_strTitle);
  // Get the list of the manual resolve results
  CStdString strLink = BOXEE::BXConfiguration::GetInstance().GetStringParam("Boxee.ManualResolve","http://res.boxee.tv");
  //strLink += "/titles/msearch/movie?name=";
  strLink += "/titles/msearch?name=";
  strLink += m_strTitle;

  CLog::Log(LOGDEBUG, "CGetResultListBG::Run, get manual resolve results from %s (manual)", strLink.c_str());

  CStdString strHtml;
  if (m_http.Get(strLink, strHtml, true))
  {
    m_bJobResult = ParseResultListXml(strHtml, m_items);
  }
  else
  {
    CLog::Log(LOGDEBUG, "CGetResultListBG::Run, could not get results from %s (manual)", strLink.c_str());
    m_bJobResult = false;
  }
}

bool CGetResultListBG::ParseResultListXml(const CStdString& strHtml, CFileItemList& items)
{
  TiXmlDocument xmlDoc;
  xmlDoc.Parse(strHtml);

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (!pRootElement)
    return false;

  if (strcmpi(pRootElement->Value(), "search") != 0)
  {
    CLog::Log(LOGERROR, "CGetResultListBG::ParseResultListXml, could not parse manual resolution results (manual)");
    return false;
  }

  const TiXmlNode* pTag = 0;
  while ((pTag = pRootElement->IterateChildren(pTag)))
  {
    if (pTag->ValueStr() == "title")
    {
      const TiXmlNode *pValue = pTag->FirstChild();
      CStdString strValue = pValue->ValueStr();

      // Find the id attribute, we do not know its name but we know that there are two attributes and it's not the one called 'year'
      CStdString idAttributeName;
      CStdString idAttributeValue;

      TiXmlAttribute* attribute = ((TiXmlElement*)pTag)->FirstAttribute();
      while (attribute)
      {
        if ((strcmpi(attribute->Name(), "year") != 0) && (strcmpi(attribute->Name(), "type") != 0))
        {
          idAttributeName = attribute->Name();
          idAttributeValue = attribute->Value();
        }
        attribute = attribute->Next();
      }

      if (idAttributeName.IsEmpty() || idAttributeValue.IsEmpty())
      {
        // this should not happen, each search result should have an id
        CLog::Log(LOGERROR, "CGetResultListBG::ParseResultListXml, search result without id, value = %s (manual)", strValue.c_str());
        continue;
      }

      // Get the year
      CStdString strYear = ((TiXmlElement*)pTag)->Attribute("year");

      CStdString strType = ((TiXmlElement*)pTag)->Attribute("type");

      bool bIsMovie = false;
      if (strType == "movie")
      {
        bIsMovie = true;
      }
      else if (strType == "tv")
      {
        bIsMovie = false;
      }
      else
      {
        CLog::Log(LOGERROR, "CGetResultListBG::ParseResultListXml, invalid type = %s (manual)", strType.c_str());
        continue;
      }

      CStdString strMovieTypeLabel = "Movie";
      CStdString strTvTypeLabel = "TV";

      // Format label and create file item
      CStdString strLabel;
      if (strYear.IsEmpty())
        strLabel.Format("%s (%s)", strValue.c_str(), bIsMovie ? strMovieTypeLabel.c_str() : strTvTypeLabel.c_str());
      else
        strLabel.Format("%s (%s) (%s)", strValue.c_str(), strYear.c_str(), bIsMovie ? strMovieTypeLabel.c_str() : strTvTypeLabel.c_str());

      CFileItemPtr resultItem (new CFileItem(strLabel));
      resultItem->SetProperty("type", "msearch");
      resultItem->SetProperty("manualresolve::Title", strValue);
      resultItem->SetProperty("manualresolve::Year", strYear);
      resultItem->SetProperty("manualresolve::idName", idAttributeName);
      resultItem->SetProperty("manualresolve::idValue", idAttributeValue);
      resultItem->SetProperty("manualresolve::isMovie", bIsMovie);
      items.Add(resultItem);

      CLog::Log(LOGDEBUG, "CGetResultListBG::ParseResultListXml, added item, title = %s, type = %s, year = %s, idName = %s, idValue = %s (manual)",
          strValue.c_str(), bIsMovie ? "movie" : "tv", strYear.c_str(), idAttributeName.c_str(), idAttributeValue.c_str());
    }
  }

  return items.Size() > 0;
}

CGetDetailsBG::CGetDetailsBG(CVideoFileContext& context, BOXEE::BXMetadata* pMetadata) : m_context(context)
{
  m_pMetadata = pMetadata;
}

void CGetDetailsBG::Run()
{
  m_context.bUseHash = false;

  CLog::Log(LOGDEBUG, "CGetDetailsBG::Run, get details for title = %s (manual)",m_context.strName.c_str());
  m_bJobResult = CMetadataResolverVideo::ResolveVideo(m_context, m_pMetadata);

  if (m_bJobResult)
  {
    BXVideo* pVideo = (BXVideo*)m_pMetadata->GetDetail(MEDIA_DETAIL_VIDEO);
    BXSeries* pSeries = (BXSeries*)m_pMetadata->GetDetail(MEDIA_DETAIL_SERIES);

    CFileItemPtr tmpItem(new CFileItem(_P(m_context.strPath), false));

    if (m_context.bMovie)
      tmpItem->SetThumbnailImage(pVideo->m_strCover);
    else
      tmpItem->SetThumbnailImage(pSeries->m_strCover);

    CStdString cachedVideoThumbPath = tmpItem->GetCachedVideoThumb();
    CStdString cachedPictureThumbPath = tmpItem->GetCachedPictureThumb();

    // TODO: Move image loading to a separate job, in case for some reason the image is not avaialble
    CPicture::CreateThumbnail(tmpItem->GetThumbnailImage(), cachedVideoThumbPath, false);
    CPicture::CreateThumbnail(tmpItem->GetThumbnailImage(), cachedPictureThumbPath, false);

    CLog::Log(LOGDEBUG, "CGetDetailsBG::Run, strItemPath = %s, strThumbnailPath = %s, cached pic path = %s (manual)",
          m_context.strPath.c_str(), tmpItem->GetThumbnailImage().c_str(), cachedPictureThumbPath.c_str());
  }
}

CSendConfirmationBG::CSendConfirmationBG(CVideoFileContext& context) : m_context(context)
{
}

void CSendConfirmationBG::Run()
{
  // Send confirmation of successful resolution to the server
  // http://virgola.boxee.tv/titles/mres/movie?id=<boxeeid>&name=<title name>&h=<hash>&year=<year>

  CStdString strLink = BOXEE::BXConfiguration::GetInstance().GetStringParam("Boxee.ManualResolve","http://res.boxee.tv");
  strLink += "/titles/mres/movie";

  m_context.bUseHash = true;

  std::map<CStdString, CStdString> mapOptions;
  m_context.GetFileOptionsMap(mapOptions);

  strLink += BoxeeUtils::BuildParameterString(mapOptions);

  // Add all relecvant parameters from the video item

  CLog::Log(LOGDEBUG, "CSendConfirmationBG::Run, send confirmation, link: %s (manual)", strLink.c_str());

  CStdString strHtml;
  if (m_http.Get(strLink, strHtml, true))
  {
    m_bJobResult = true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "CSendConfirmationBG::Run, could not get results from %s (manual)", strLink.c_str());
    m_bJobResult = false;
  }
}

/*
 *  Main dialog for the manual resolve
 */
CGUIDialogBoxeeManualResolve::CGUIDialogBoxeeManualResolve() :
      CGUIDialog(WINDOW_DIALOG_BOXEE_MANUAL_RESOLVE, "boxee_manual_resolve.xml"), m_resovedVideoMetadata(MEDIA_ITEM_TYPE_VIDEO)
      {
        m_bConfirmed = false;
      }

CGUIDialogBoxeeManualResolve::~CGUIDialogBoxeeManualResolve() {
}

bool CGUIDialogBoxeeManualResolve::Show(CFileItemPtr pItem)
{
  if (!pItem) return false;

  CGUIDialogBoxeeManualResolve *pDialog = (CGUIDialogBoxeeManualResolve*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_MANUAL_RESOLVE);
  if (pDialog)
  {
    // Copy the item into the dialog
    pDialog->Reset();
    pDialog->m_unresolvedVideoItem = pItem;
    pDialog->DoModal();

    return pDialog->m_bConfirmed;
  }

  return false;
}

void CGUIDialogBoxeeManualResolve::Reset()
{
  m_resovedVideoMetadata.Reset();
  m_videoFileContext.Reset();
  m_videoParts.Clear();
}

void CGUIDialogBoxeeManualResolve::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  m_bConfirmed = false;

  m_resultListItems.Clear();

  m_unresolvedVideoItem->Dump();

  // Send the item to the special container to allow skin acceess
  CFileItemPtr itemPtr(new CFileItem(*m_unresolvedVideoItem.get()));
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), 5000, 0, 0, itemPtr);
  g_windowManager.SendThreadMessage(winmsg);

  CLog::Log(LOGERROR, "CGUIDialogBoxeeManualResolve::OnInitWindow, initialized manual resolve video, path = %s (manual)", m_unresolvedVideoItem->m_strPath.c_str());

  if (m_unresolvedVideoItem->IsStack())
  {
    CLog::Log(LOGERROR, "CGUIDialogBoxeeManualResolve::OnInitWindow, initialized manual resolve for mutlipart video (manual)");
    // Separate the stack into its parts
    CFileItemList parts;

    DIRECTORY::CDirectory::GetDirectory(m_unresolvedVideoItem->m_strPath, parts);

    m_videoParts.Clear();
    for (int i = 0; i < parts.Size(); i++)
    {
      CFileItemPtr partItem = parts.Get(i);
      partItem->SetLabel(CUtil::GetFileName(partItem->m_strPath));

      m_videoParts.Add(partItem);

      CLog::Log(LOGERROR, "CGUIDialogBoxeeManualResolve::OnInitWindow, mutlipart video, part %d, path = %s (manual)", i , partItem->m_strPath.c_str());
      if (i == 0)
      {
        // Load context for the first part only
        m_videoFileContext.Load(_P(partItem->m_strPath.c_str()));
      }
    }

  }
  else
  {
    m_videoFileContext.Load(_P(m_unresolvedVideoItem->m_strPath));
  }

  m_resolvedVideoItem.reset(new CFileItem(m_unresolvedVideoItem->m_strPath, false));

  // Set extracted name as a starting point for manual resolving
  ((CGUIEditControl*)GetControl(TITLE_EDIT))->SetLabel2(m_videoFileContext.strName);

}

void CGUIDialogBoxeeManualResolve::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
}

bool CGUIDialogBoxeeManualResolve::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();
    if (iControl == BTN_SEARCH)
    {
      GetList();
    }
    if (iControl == BTN_DROP)
    {
      if (CGUIDialogYesNo2::ShowAndGetInput(g_localizeStrings.Get(52129), m_videoFileContext.strPath))
      {
        BOXEE::BXMetadataEngine& MDE = BOXEE::Boxee::GetInstance().GetMetadataEngine();
        MDE.RemoveVideoByPath(m_videoFileContext.strPath);
        MDE.UpdateVideoFileStatus(m_videoFileContext.strPath, STATUS_UNRESOLVED);
        Close();
      }

      return true;
    }
    else if (iControl == RESULT_LIST)
    {
      // Get selected item from the list and open matching details dialog
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), RESULT_LIST);
      OnMessage(msg);

      int iSelectedItem = msg.GetParam1();

      if (iSelectedItem < 0 || iSelectedItem > m_resultListItems.Size() -1)
        return true;

      m_selectedItem = m_resultListItems.Get(iSelectedItem);

      if (m_selectedItem->HasProperty("listerror"))
      {
        return true;
      }

      // Extract the parameters we need to get video details from the server
      m_videoFileContext.bMovie = m_selectedItem->GetPropertyBOOL("manualresolve::isMovie");;
      m_videoFileContext.strExternalIdName = m_selectedItem->GetProperty("manualresolve::idName");
      m_videoFileContext.strExternalIdValue = m_selectedItem->GetProperty("manualresolve::idValue");
      m_videoFileContext.strName = m_selectedItem->GetProperty("manualresolve::Title");
      m_videoFileContext.strYear = m_selectedItem->GetProperty("manualresolve::Year");

      CGetDetailsBG* pJob = new CGetDetailsBG(m_videoFileContext, &m_resovedVideoMetadata);
      bool bResult = CUtil::RunInBG(pJob);

      if (bResult)
      {
        // Convert received metadata into a file item
        DIRECTORY::CBoxeeDatabaseDirectory::CreateVideoItem(&m_resovedVideoMetadata, m_resolvedVideoItem.get());


        BXVideo* pVideo = (BXVideo*)m_resovedVideoMetadata.GetDetail(MEDIA_DETAIL_VIDEO);
        BXSeries* pSeries = (BXSeries*)m_resovedVideoMetadata.GetDetail(MEDIA_DETAIL_SERIES);

        m_resolvedVideoItem->m_strPath = m_videoFileContext.strPath;

        if (m_videoFileContext.bMovie)
        {
          m_resolvedVideoItem->SetThumbnailImage(pVideo->m_strCover);
        }
        else
        {
          m_resolvedVideoItem->SetThumbnailImage(pSeries->m_strCover);
        }

        m_resolvedVideoItem->SetThumbnailImage(m_resolvedVideoItem->GetCachedPictureThumb());

        if (m_videoFileContext.bMovie)
        {
          m_bConfirmed = CGUIDialogBoxeeManualResolveMovie::Show(m_resolvedVideoItem, m_videoParts);
        }
        else
        {
          m_bConfirmed = CGUIDialogBoxeeManualResolveEpisode::Show(m_resolvedVideoItem);

          if (m_bConfirmed && (m_videoFileContext.iSeason != m_resolvedVideoItem->GetVideoInfoTag()->m_iSeason
              || m_videoFileContext.iEpisode != m_resolvedVideoItem->GetVideoInfoTag()->m_iEpisode))
          {
            // Update the file context
            m_videoFileContext.iSeason = m_resolvedVideoItem->GetVideoInfoTag()->m_iSeason;
            m_videoFileContext.iEpisode = m_resolvedVideoItem->GetVideoInfoTag()->m_iEpisode;

            m_resovedVideoMetadata.Reset();
            CGetDetailsBG* pJob = new CGetDetailsBG(m_videoFileContext, &m_resovedVideoMetadata);
            bool bResult = CUtil::RunInBG(pJob);

            if (!bResult)
            {
              CLog::Log(LOGERROR, "CGUIDialogBoxeeManualResolve::OnMessage, unable to get details for item, (manual)");
              CGUIDialogOK2::ShowAndGetInput("Sorry", "Unable to bring the movie details");
              return false;
            }
          }
        }

        if (m_bConfirmed)
        {
          bool bApplyToFolder = m_resolvedVideoItem->GetPropertyBOOL("manualresolve::ApplyToFolder");
          if (bApplyToFolder)
          {
            CStdString strDirectory;
            CUtil::GetParentPath(m_resolvedVideoItem->m_strPath, strDirectory);

            int iSavedSeason = m_resolvedVideoItem->GetVideoInfoTag()->m_iSeason;

            // Get all files from the folder
            CFileItemList episodeItems;
            DIRECTORY::CDirectory::GetDirectory(strDirectory, episodeItems);

            for (int i = 0; i < episodeItems.Size(); i++)
            {
              CFileItemPtr currentEpisode = episodeItems.Get(i);
              if (currentEpisode->IsVideo())
              {
                CVideoFileContext context(currentEpisode->m_strPath);
                context.Load();

                // Do not use hash during resolving
                context.bUseHash = false;

                // Set parameters from the previously resolved context
                context.bMovie = m_videoFileContext.bMovie;
                context.strExternalIdName = m_videoFileContext.strExternalIdName;
                context.strExternalIdValue = m_videoFileContext.strExternalIdValue;
                context.strName = m_videoFileContext.strName;
                context.strYear = m_videoFileContext.strYear;

                // Check if the item has a detectable season and episode tag and if the season matches the one we
                if (context.iSeason == iSavedSeason && context.iEpisode > 0)
                {
                  BXMetadata resolvedVideoMetadata(MEDIA_ITEM_TYPE_VIDEO);
                  // Get metadata for the specific episode
                  CGetDetailsBG* pJob = new CGetDetailsBG(context, &resolvedVideoMetadata);
                  bool bResult = CUtil::RunInBG(pJob);
                  if (bResult)
                  {
                    AddVideo(context, resolvedVideoMetadata);
                  }
                  else
                  {
                    CLog::Log(LOGERROR, "CGUIDialogBoxeeManualResolve::OnMessage, unable to get details for item, (manual)");
                  }
                }
              }
            }
          }
          else
          {
            // Check whether it is necessary to resolve the episode again, because the user updated season and episode numbers
            AddVideo(m_videoFileContext, m_resovedVideoMetadata);
          }

          Close();
        }

      }
      else
      {
        CLog::Log(LOGERROR, "CGUIDialogBoxeeManualResolve::OnMessage, unable to get details for item, (manual)");
        CGUIDialogOK2::ShowAndGetInput("Sorry", "Unable to bring the movie details");
        return false;
      }
    }
  }
  break;
  } // switch
  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogBoxeeManualResolve::GetList()
{
  // Get the name with which we should resolve
  CStdString title = ((CGUIEditControl*)GetControl(TITLE_EDIT))->GetLabel2();

  m_resultListItems.Clear();

  CLog::Log(LOGDEBUG, "CGUIDialogBoxeeManualResolve::GetList, get all results for title = %s (manual)", title.c_str());
  CGetResultListBG* pJob = new CGetResultListBG(title, m_resultListItems);
  bool bResult = CUtil::RunInBG(pJob);
  if (!bResult)
  {
    CLog::Log(LOGERROR, "CGUIDialogBoxeeManualResolve::GetList, could not get list for title = %s (manual)", title.c_str());
    CFileItemPtr itemPtr(new CFileItem(g_localizeStrings.Get(52116)));
    itemPtr->SetProperty("listerror", true);

    m_resultListItems.Add(itemPtr);
  }

  CGUIMessage message(GUI_MSG_LABEL_RESET, GetID(), RESULT_LIST);
  OnMessage(message);

  // Populate the list with results
  CGUIMessage message2(GUI_MSG_LABEL_BIND, GetID(), RESULT_LIST, 0, 0, &m_resultListItems);
  OnMessage(message2);

  return bResult;
}

bool CGUIDialogBoxeeManualResolve::AddVideo(CVideoFileContext& context, BOXEE::BXMetadata& metadata)
{

  // Restore the original title of the video
  context.LoadNameFromPath();

  // The movie was accepted, we need to send the details to the server
  CSendConfirmationBG* pJob = new CSendConfirmationBG(context);
  bool bResult = CUtil::RunInBG(pJob);
  if (!bResult)
  {
    CLog::Log(LOGDEBUG, "CGUIDialogBoxeeManualResolve::AddMovie, unable to send confirmation (manual)");
  }

  BOXEE::BXMetadataEngine& MDE = BOXEE::Boxee::GetInstance().GetMetadataEngine();

  CStdString strPath = context.strPath;

  CLog::Log(LOGDEBUG, "CGUIDialogBoxeeManualResolve::AddMovie, adding resolved movie to database, path = %s (manual)", strPath.c_str());

  metadata.SetPath(strPath);
  metadata.SetMediaFileId(1);

  // Remove existing movie, if it indeed exists

  CLog::Log(LOGDEBUG, "CGUIDialogBoxeeManualResolve::AddMovie, remove previously existing video, path = %s (manual)", strPath.c_str());
  MDE.RemoveVideoByPath(strPath);

  //
  BXVideo* pVideo = (BXVideo*)metadata.GetDetail(MEDIA_DETAIL_VIDEO);

  // patch !! fetch the folder Id from the database
  CStdString strDirectory = BXUtils::GetEffectiveFolderPath(strPath);
  pVideo->m_iFolderId = MDE.GetMediaFolderId(strDirectory, "videoFolder");

  // Add to video to the database
  int iVideoId = -1;
  if (m_videoParts.Size() > 1)
  {
    // In case there are several parts, add each part seaprately
    pVideo->m_strType = "part";


    pVideo->m_strPath = strDirectory;
    metadata.SetPath(strDirectory);

    CMetadataResolver::CacheThumb(strDirectory, pVideo->m_strCover);

    CLog::Log(LOGDEBUG, "CGUIDialogBoxeeManualResolve::AddMovie, add multipart video, path = %s (manual)", strDirectory.c_str());
    iVideoId = MDE.AddVideo(&metadata);
    if (iVideoId == -1)
    {
      CLog::Log(LOGERROR, "CGUIDialogBoxeeManualResolve::AddMovie, unable to add video to database, path = %s (manual)", strPath.c_str());
      return false;
    }

    for (int i = 0; i < m_videoParts.Size(); i++)
    {
      CStdString strPartPath = m_videoParts.Get(i)->m_strPath;

      CLog::Log(LOGDEBUG, "CGUIDialogBoxeeManualResolve::AddMovie, add part %d, path = %s (manual)", i, strPartPath.c_str());
      MDE.RemoveVideoByPath(strPartPath);
      MDE.AddPart(iVideoId, i, strPartPath);
      MDE.UpdateVideoFileStatus(strPartPath, STATUS_RESOLVED);
    }
  }
  else
  {
    pVideo->m_strPath = strPath;
    iVideoId = MDE.AddVideo(&metadata);
    if (iVideoId == -1)
    {
      CLog::Log(LOGERROR, "CGUIDialogBoxeeManualResolve::AddMovie, unable to add video to database, path = %s (manual)", strPath.c_str());
      return false;
    }
    MDE.UpdateVideoFileStatus(strPath, STATUS_RESOLVED);
  }

  return true;
}
