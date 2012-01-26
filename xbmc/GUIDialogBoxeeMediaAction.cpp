/* CGUIDialogBoxeeMediaAction.cpp
 *
 */

#include "GUIWindowManager.h"
#include "GUIDialogBoxeeMediaAction.h"
#include "Util.h"
#include "lib/libBoxee/bxobject.h"
#include "lib/libBoxee/bxutils.h"
#include "lib/libBoxee/bxmetadata.h"
#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxuserprofiledatabase.h"
#include "lib/libBoxee/bxvideodatabase.h"
#include "BoxeeUtils.h"
#include "URL.h"
#include "FileSystem/Directory.h"
#include "Application.h"
#include "MusicInfoTag.h"
#include "FileSystem/PluginDirectory.h"
#include "FileSystem/Directory.h"
#include "FileSystem/BoxeeDatabaseDirectory.h"
#include "FileSystem/DirectoryCache.h"
#include "GUIImage.h"
#include "VideoInfoTag.h"
#ifdef HAS_LASTFM
#include "LastFmManager.h"
#endif
#include "GUIWindowBoxeeMediaInfo.h"
#include "GUIWindowMusicInfo.h"
#include "GUIWindowBoxeeBrowse.h"
#include "GUIWindowSlideShow.h"
#include "MetadataResolverVideo.h"
#include "GUIDialogBoxeeManualResolve.h"
#include "GUIDialogOK.h"
#include "AppManager.h"
#include "GUIWindowBoxeeAlbumInfo.h"
#include "FileSystem/FileCurl.h"
#include "xbox/IoSupport.h"
#include "FileSystem/cdioSupport.h"
#include "DetectDVDType.h"
#include "LocalizeStrings.h"
#include "VideoDatabase.h"
#include "GUIUserMessages.h"
#include "GUISettings.h"
#include "GUIDialogOK2.h"
#include "GUIDialogBoxeeShare.h"
#include "SpecialProtocol.h"
#include "BoxeeItemLauncher.h"
#include "GUIDialogYesNo2.h"
#include "GUIDialogProgress.h"
#include "GUIWindowBoxeeBrowseTracks.h"
#include "GUIButtonControl.h"
#include "GUIWindowBoxeeBrowseApps.h"
#include "GUIWindowBoxeeMain.h"
#include "GUILabelControl.h"
#include "MediaManager.h"
#include "GUIDialogSelect.h"
#include "GUIDialogBoxeeVideoQuality.h"
#include "GUIDialogBoxeeSelectionList.h"
#include "GUIDialogBoxeeVideoResume.h"
#include "GUIDialogBoxeeNetworkNotification.h"
#include "GUIDialogBoxeePaymentProducts.h"
#include "GUIDialogBoxeePaymentOkPlay.h"
#include "ItemLoader.h"
#include "GUIWindowBoxeeBrowseTvEpisodes.h"

#include "GUIFixedListContainer.h"
#include "GUIPanelContainer.h"

#ifdef HAVE_LIBBLURAY
// Fugly but needed so we can look up dvd title info
#include "cores/dvdplayer/DVDDemuxers/DVDDemux.h"
#include "cores/dvdplayer/DVDInputStreams/DVDInputStreamBluray.h"
#include "GUIDialogBoxeeChapters.h"
#endif

#ifdef _WIN32
#include "WIN32Util.h"
#endif

using namespace DIRECTORY;
using namespace BOXEE;
using namespace XFILE;

#define TRAILER_BUTTON                         4
#define LINKS_GROUP                            6000

#define ADDITIONAL_LINKS_LIST                  6020

#define BUTTONS_GROUP                          6100
#define TRAILER_LIST                           6110
#define BUTTONS_LIST_GROUP                     6120
#define BUTTONS_LIST_MOVIE                     6121
#define BUTTONS_LIST_NOT_MOVIE                 6122

#define ADDITIONAL_MOVIE_BUTTONS_LIST          4000

#define INFO_GROUP                             6200
#define BACK_GROUP                             6210

#define MORE_INFO_SCROLLBAR                    6900

#define APP_GROUP                              6300
#define APP_LUANCH_BUTTON                      6310
#define APP_ADD_BUTTON                         6320
#define APP_REMOVE_BUTTON                      6330
#define APP_SHORTCUT_BUTTON                    6340

#define ITEM_PATH_LABEL                        6742

#define CAST_PANEL_ID                          6800

#define HIDDEN_CONTAINER                       5000
#define NOTIFICATION_APPEARANCE_IN_SEC         5000

#define RESUME_DIALOG_POS_X                    710
#define RESUME_DIALOG_POS_Y                    334

#define BUTTON_ACTION_PROPERTY_NAME            "button-action"
#define ACTION_MORE_INFO                       "more-info"
#define ACTION_ADD_TO_QUEUE                    "add-to-queue"
#define ACTION_REMOVE_FROM_QUEUE               "remove-from-queue"
#define ACTION_MARK_AS_SEEN                    "mark-as-seen"
#define ACTION_MARK_AS_UNSEEN                  "mark-as-unseen"
#define ACTION_ADD_AS_SHORTCUT                 "add-as-shortcut"
#define ACTION_REMOVE_FROM_SHORTCUT            "remove-from-shortcut"
#define ACTION_GOTO_SHOW                       "goto-show"
#define ACTION_PLAY_TRAILER                    "play-trailer"
#define ACTION_SAHRE_ITEM                      "share-item"
#define ACTION_REMOVE_FROM_HISTORY             "remove-from-history"
#define ACTION_EJECT                           "eject"
#define ACTION_BROWSE                          "browse"
#define ACTION_RESOLVE                         "resolve"
#define ACTION_ADD_TO_FAVORITE                 "add-to-favorite"
#define ACTION_REMOVE_FROM_FAVORITE            "remove-from-favorite"

#define PLAY_BUTTON_ISHD                       "is-hd"
#define PLAY_BUTTON_THUMB                      "pb_provider_thumb"

#define MAX_NUM_OF_CAST_MEMBER_TO_SHOW         10

CGUIDialogBoxeeMediaAction::CGUIDialogBoxeeMediaAction(void) : CGUIDialog(WINDOW_DIALOG_BOXEE_MEDIA_ACTION, "boxee_media_action.xml")
{
  m_bConfirmed = true;
  m_numOfButtons = 0;
}

CGUIDialogBoxeeMediaAction::~CGUIDialogBoxeeMediaAction()
{

}

void CGUIDialogBoxeeMediaAction::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  int activeWindow = g_windowManager.GetActiveWindow();

  if (m_item.IsApp() && !m_item.GetProperty("appid").IsEmpty() && (activeWindow == WINDOW_BOXEE_BROWSE_APPS || activeWindow == WINDOW_BOXEE_BROWSE_REPOSITORIES || m_item.GetPropertyBOOL("IsSearchItem")))
  {
    InitAppItem();
    return;
  }

  if (m_item.IsVideo() && m_item.GetPropertyBOOL("isFolderItem"))
  {
    BoxeeUtils::GetLocalVideoMetadata(m_item);
  }

  InitNotAppItem();

  m_refreshActiveWindow = false;
}

void CGUIDialogBoxeeMediaAction::InitAppItem()
{
  CGUIMessage winmsg1(GUI_MSG_LABEL_RESET, GetID(), HIDDEN_CONTAINER);
  g_windowManager.SendMessage(winmsg1);

  m_item.SetProperty("isAppLauncher",true);

  // Send the item to the special container to allow skin access
  CFileItemPtr itemPtr(new CFileItem(m_item));
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), HIDDEN_CONTAINER, 0, 0, itemPtr);
  g_windowManager.SendThreadMessage(winmsg);

  SET_CONTROL_HIDDEN(LINKS_GROUP);
  SET_CONTROL_HIDDEN(ADDITIONAL_LINKS_LIST);
  SET_CONTROL_HIDDEN(BUTTONS_LIST_GROUP);

  SET_CONTROL_HIDDEN(INFO_GROUP);

  SET_CONTROL_VISIBLE(APP_GROUP);
  SET_CONTROL_VISIBLE(BUTTONS_GROUP);

  bool succeeded = BuildAppFileItemLinksList();

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::InitAppItem - BuildAppFileItemLinksList returned [linksFileItemListSize=%d] (bma)",(int)m_linksFileItemList.size());

  if (!succeeded)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::InitAppItem - FAILED to initialize item links. [ItemLabel=%s][ItemPath=%s] (bma)",m_item.GetLabel().c_str(),m_item.m_strPath.c_str());
    return;
  }

  if ((int)m_linksFileItemList.size() > 2)
  {
    ///////////////////////
    // set launch button //
    ///////////////////////

    CFileItemPtr launchAppFileItem = m_linksFileItemList[0];

    CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), APP_LUANCH_BUTTON, 0, 0, launchAppFileItem);
    OnMessage(winmsg);

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::InitAppItem - The LUANCH_BUTTON was set with item [label=%s][path=%s] (bma)",launchAppFileItem->GetLabel().c_str(),launchAppFileItem->m_strPath.c_str());

    SET_CONTROL_VISIBLE(APP_LUANCH_BUTTON);
    SET_CONTROL_FOCUS(APP_LUANCH_BUTTON, 0);

    /////////////////////////////
    // set other links buttons //
    /////////////////////////////

    CFileItemPtr manageAppFileItem = m_linksFileItemList[1];

    if (manageAppFileItem->GetLabel() == g_localizeStrings.Get(53761))
    {
      ////////////////////////
      // set APP_ADD_BUTTON //
      ////////////////////////

      CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), APP_ADD_BUTTON, 0, 0, manageAppFileItem);
      OnMessage(winmsg);

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::InitAppItem - The APP_ADD_BUTTON was set with item [label=%s][path=%s] (bma)",manageAppFileItem->GetLabel().c_str(),manageAppFileItem->m_strPath.c_str());

      SET_CONTROL_VISIBLE(APP_ADD_BUTTON);
      SET_CONTROL_HIDDEN(APP_REMOVE_BUTTON);
    }
    else if (manageAppFileItem->GetLabel() == g_localizeStrings.Get(53762))
    {
      ///////////////////////////
      // set APP_REMOVE_BUTTON //
      ///////////////////////////

      CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), APP_REMOVE_BUTTON, 0, 0, manageAppFileItem);
      OnMessage(winmsg);

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::InitAppItem - The APP_REMOVE_BUTTON was set with item [label=%s][path=%s] (bma)",manageAppFileItem->GetLabel().c_str(),manageAppFileItem->m_strPath.c_str());

      SET_CONTROL_VISIBLE(APP_REMOVE_BUTTON);
      SET_CONTROL_HIDDEN(APP_ADD_BUTTON);
    }

    /////////////////////////////
    // set APP_SHORTCUT_BUTTON //
    /////////////////////////////

    CFileItemPtr shortcutFileItem = m_linksFileItemList[2];

    CGUIMessage winAddShortcutMsg(GUI_MSG_LABEL_ADD, GetID(), APP_SHORTCUT_BUTTON, 0, 0, shortcutFileItem);
    OnMessage(winAddShortcutMsg);

    SET_CONTROL_VISIBLE(APP_SHORTCUT_BUTTON);
    SET_CONTROL_LABEL(APP_SHORTCUT_BUTTON,shortcutFileItem->GetLabel());
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::InitAppItem - Enter function with [NUmOfLinks=%zu] < 3 (bma)",m_linksFileItemList.size());
  }
}

bool CGUIDialogBoxeeMediaAction::BuildAppFileItemLinksList()
{
  m_linksFileItemList.clear();

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildAppFileItemLinksList - Handling item [label=%s][appId=%s] (bbma)",m_item.GetLabel().c_str(),m_item.GetProperty("appid").c_str());

  /////////////////////////////
  // add link for launch app //
  /////////////////////////////

  CFileItemPtr launchAppFileItem(new CFileItem(g_localizeStrings.Get(53760)));
  launchAppFileItem->m_strPath = m_item.m_strPath;
  m_linksFileItemList.push_back(launchAppFileItem);

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildAppFileItemLinksList - After add LAUNCH [label=%s][path=%s] for item [label=%s][appId=%s]. [linksFileItemList=%zu] (bbma)",launchAppFileItem->GetLabel().c_str(),launchAppFileItem->m_strPath.c_str(),m_item.GetLabel().c_str(),m_item.GetProperty("appid").c_str(),m_linksFileItemList.size());

  /////////////////////////////////////
  // add link for install/remove app //
  /////////////////////////////////////

  bool isTestApp = !(m_item.GetProperty("testapp").IsEmpty());
  bool isInUserApplications = false;

  if (!isTestApp)
  {
    isInUserApplications = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().IsInUserApplications(m_item.GetProperty("appid"));
  }

  if (!isTestApp && !isInUserApplications)
  {
    CFileItemPtr installAppFileItem(new CFileItem(g_localizeStrings.Get(53761)));
    m_linksFileItemList.push_back(installAppFileItem);

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildAppFileItemLinksList - After add INSTALL [label=%s] for item [label=%s][appId=%s]. [linksFileItemList=%zu] (bbma)",installAppFileItem->GetLabel().c_str(),m_item.GetLabel().c_str(),m_item.GetProperty("appid").c_str(),m_linksFileItemList.size());
  }
  else
  {
    CFileItemPtr uninstallAppFileItem(new CFileItem(g_localizeStrings.Get(53762)));
    m_linksFileItemList.push_back(uninstallAppFileItem);

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildAppFileItemLinksList - After add REMOVE [label=%s] for item [label=%s][appId=%s]. [linksFileItemList=%zu] (bbma)",uninstallAppFileItem->GetLabel().c_str(),m_item.GetLabel().c_str(),m_item.GetProperty("appid").c_str(),m_linksFileItemList.size());
  }

  ////////////////////////////////////
  // add button for shortcut to app //
  ////////////////////////////////////

  CFileItemPtr shortcutButton;
  CStdString buttonType;

  if (CanAddAsShortcut(m_item))
  {
    shortcutButton = CFileItemPtr(new CFileItem(g_localizeStrings.Get(53715)));
    buttonType = g_localizeStrings.Get(53715);
  }
  else
  {
    shortcutButton = CFileItemPtr(new CFileItem(g_localizeStrings.Get(53716)));
    buttonType = g_localizeStrings.Get(53716);
  }

  m_linksFileItemList.push_back(shortcutButton);

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildAppFileItemLinksList - After add SORTCUT [label=%s][type=%s] for item [label=%s][appId=%s]. [linksFileItemList=%zu] (bma)",shortcutButton->GetLabel().c_str(),buttonType.c_str(),m_item.GetLabel().c_str(),m_item.GetProperty("appid").c_str(),m_linksFileItemList.size());

  return true;
}

void CGUIDialogBoxeeMediaAction::InitNotAppItem()
{
  SET_CONTROL_HIDDEN(APP_GROUP);

  // reset the window property "link-path-to-show"
  SetLinkPathAsWindowProperty("");

  CStdString focusedLinkPathToShow = "";

  SetProperty("is-hd", 0);

  m_bConfirmed = true;

  m_linksFileItemList.clear();

  m_tarilerWasAddedToLinkList = false;

  m_visibleButtonsListId = 0;

  m_numOfButtons = 0;

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::InitNotAppItem - Main item has properties [haslink-free=%s][haslink-clip=%s][haslink-trailer=%s][haslink-rent=%s][haslink-buy=%s][haslink-sub=%s][haslink-exsub=%s] (bma)",m_item.GetProperty("haslink-free").c_str(),m_item.GetProperty("haslink-clip").c_str(),m_item.GetProperty("haslink-trailer").c_str(),m_item.GetProperty("haslink-rent").c_str(),m_item.GetProperty("haslink-buy").c_str(),m_item.GetProperty("haslink-sub").c_str(),m_item.GetProperty("haslink-exsub").c_str());

  CGUIMessage winmsg1(GUI_MSG_LABEL_RESET, GetID(), HIDDEN_CONTAINER);
  g_windowManager.SendMessage(winmsg1);

  // Send the item to the special container to allow skin access
  CFileItemPtr itemPtr(new CFileItem(m_item));
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), HIDDEN_CONTAINER, 0, 0, itemPtr);
  g_windowManager.SendThreadMessage(winmsg);

  CGUIMessage winmsg2(GUI_MSG_LABEL_RESET, GetID(), ADDITIONAL_LINKS_LIST);
  OnMessage(winmsg2);

  CGUIMessage winmsg3(GUI_MSG_LABEL_RESET, GetID(), ADDITIONAL_MOVIE_BUTTONS_LIST);
  OnMessage(winmsg3);

  int linksAddedCounter = 0;

  if (BoxeeUtils::CanPlay(m_item) || m_item.GetPropertyBOOL("ishistory"))
  {
    if (m_item.HasLinksList())
    {
      /////////////////////////////////////////////////////////////////////////////////////////////
      // in case there are links in the FileItem -> build a list and add them to the dialog list //
      /////////////////////////////////////////////////////////////////////////////////////////////

      if (m_item.GetPropertyBOOL("ismovie") && !m_item.GetProperty("boxeeId").IsEmpty())
      {
        //load the first trailer from the database
        BXVideoDatabase vdb;
        std::vector<BXVideoLink> videoLinks;
        vdb.GetVideoLinks(videoLinks, m_item.GetProperty("boxeeId"), "trailer" , 1);

        if (!videoLinks.empty())
        {
          BXVideoLink trailer = videoLinks.front();

          m_item.AddLink(trailer.m_strTitle.c_str(), trailer.m_strURL.c_str(), trailer.m_strType.c_str(), CFileItemList::GetLinkBoxeeTypeAsEnum(trailer.m_strBoxeeType.c_str()),
               trailer.m_strProvider.c_str(), trailer.m_strProviderName.c_str(), trailer.m_strProviderThumb.c_str(), trailer.m_strCountryCodes.c_str(), BXUtils::StringToInt(trailer.m_strCountryRel),
               trailer.m_strQualityLabel.c_str(), BXUtils::StringToInt(trailer.m_strQuality.c_str()), CFileItemList::GetLinkBoxeeOfferAsEnum(trailer.m_strOffer.c_str()), "");
        }
      }

      bool succeeded = BuildLinksFileItemList();

      if (!succeeded)
      {
        CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::InitNotAppItem - FAILED to initialize item links. [ItemLabel=%s][ItemPath=%s] (bma)",m_item.GetLabel().c_str(),m_item.m_strPath.c_str());
        return;
      }

      // there are links -> reset the item m_strPath (it isn't relevant and so it won't be pass in share)
      m_item.m_strPath = "";
      m_item.SetContentType("");

      if (m_linksFileItemList.size() > 0)
      {
        // add links to the button list
        for (size_t i=0; i<m_linksFileItemList.size(); i++)
        {
          CFileItemPtr linkFileItem = m_linksFileItemList[i];

          if (CFileItem::GetLinkBoxeeTypeAsEnum(linkFileItem->GetProperty("link-boxeetype")) == CLinkBoxeeType::LOCAL)
          {
            linkFileItem->SetLabel(g_localizeStrings.Get(53751));

            if (i == 0)
            {
              focusedLinkPathToShow = linkFileItem->m_strPath;
            }
          }

          CFileItemPtr linkFileItemToAdd(new CFileItem(*(linkFileItem.get())));
          CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::InitNotAppItem - [%zu/%zu] - Going to add item [label=%s][label2=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d] (bma)",i+1,m_linksFileItemList.size(),linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->GetLabel2().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItem->GetPropertyBOOL("link-countryrel"));

          CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), ADDITIONAL_LINKS_LIST, 0, 0, linkFileItemToAdd);
          OnMessage(winmsg);

          linksAddedCounter++;
        }
      }

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::InitNotAppItem - After adding [%d] items to the dialog list (bma)",linksAddedCounter);

      SetupTrailer(linksAddedCounter);
    }
    else if (!m_item.m_strPath.IsEmpty())
    {
      CFileItemPtr fileItemToAdd(new CFileItem(m_item));
      fileItemToAdd->SetLabel("");

      if (!fileItemToAdd->IsPlayableFolder() && (fileItemToAdd->GetPropertyBOOL("IsFolder") && !fileItemToAdd->IsPlayList()))
      {
        fileItemToAdd->SetLabel2(g_localizeStrings.Get(53755));
      }
      else
      {
        //////////////////////////////////////////////////////
        // in case of provider -> set it in the PLAY button //
        //////////////////////////////////////////////////////

        CStdString provider = "";
        if (!fileItemToAdd->GetProperty("provider").IsEmpty())
        {
          provider = fileItemToAdd->GetProperty("provider");
        }
        else if (!fileItemToAdd->GetProperty("provider_source").IsEmpty())
        {
          provider = fileItemToAdd->GetProperty("provider_source");
        }
        else if (fileItemToAdd->m_strPath.Left(6) == "http:/" || fileItemToAdd->m_strPath.Left(7) == "flash:/" || fileItemToAdd->m_strPath.Left(5) == "app:/")
        {
          CURI url(fileItemToAdd->m_strPath);
          provider = url.GetHostName();
        }

        if (!provider.IsEmpty())
        {
          CStdString providerLabel = g_localizeStrings.Get(53752) + provider;
          fileItemToAdd->SetLabel(providerLabel);
        }
      }

      if (fileItemToAdd->GetLabel2().IsEmpty())
      {
        fileItemToAdd->SetLabel2(g_localizeStrings.Get(53750));
      }

      m_linksFileItemList.push_back(fileItemToAdd);

      focusedLinkPathToShow = GetLinkPathToShowInDialog(m_item);

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::InitNotAppItem - There are no LinksFileItem -> Going to add the original item [label=%s][label2=%s][path=%s][thumb=%s] (bma)",fileItemToAdd->GetLabel().c_str(),fileItemToAdd->GetLabel2().c_str(),fileItemToAdd->m_strPath.c_str(),fileItemToAdd->GetThumbnailImage().c_str());

      CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), ADDITIONAL_LINKS_LIST, 0, 0, fileItemToAdd);
      OnMessage(winmsg);

      linksAddedCounter++;
    }
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::InitNotAppItem - After adding links [linksAddedCounter=%d]. [HasTrailerLink=%d] (bma)",linksAddedCounter,m_trailerLinkItem.get()?true:false);

  m_numOfButtons = FillDialogButtons((linksAddedCounter > 0) ? true : false);

  if (linksAddedCounter > 0)
  {
    /////////////////////
    // links was added //
    /////////////////////

    SET_CONTROL_HIDDEN(INFO_GROUP);
    SET_CONTROL_VISIBLE(LINKS_GROUP);
    SET_CONTROL_FOCUS(ADDITIONAL_LINKS_LIST, 0);

    SetLinkPathAsWindowProperty(focusedLinkPathToShow);

    SET_CONTROL_VISIBLE(ADDITIONAL_LINKS_LIST);
  }
  else
  {
    ////////////////////////
    // no links was added //
    ////////////////////////

    SET_CONTROL_HIDDEN(INFO_GROUP);
    SET_CONTROL_HIDDEN(LINKS_GROUP);
    SET_CONTROL_HIDDEN(ADDITIONAL_LINKS_LIST);

    if (m_numOfButtons > 0)
    {
      SET_CONTROL_VISIBLE(BUTTONS_LIST_GROUP);
    }
    else
    {
      SET_CONTROL_HIDDEN(BUTTONS_LIST_GROUP);
    }
  }

  CGUIWindow* activeWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
  if (activeWindow)
  {
    activeWindow->SetProperty("has-playable-links",linksAddedCounter > 0 ? true : false);
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::InitNotAppItem - After set window [id=%d] property [has-playable-links=%d]. [NumOfPlayableLinks=%d] (bma)",activeWindow->GetID(),activeWindow->GetPropertyBOOL("has-playable-links"),linksAddedCounter);
  }

  if (m_tarilerWasAddedToLinkList)
  {
    SET_CONTROL_VISIBLE(TRAILER_LIST);
}
  else
  {
    SET_CONTROL_HIDDEN(TRAILER_LIST);
  }

  FillMovieAdditionalDialogButtons();

  InitCastPanel();
}

bool CGUIDialogBoxeeMediaAction::AddFileItemToList(std::map<CStdString, CFileItemPtr>& addedLinksMap,  std::list<CFileItemPtr>& pathsList)
{
  int addedPathsCounter = addedLinksMap.size();

  std::list<CFileItemPtr>::iterator itFreelist = pathsList.begin();
  std::map<CStdString, CFileItemPtr>::iterator mapIt;
  while (itFreelist != pathsList.end())
  {
    CFileItemPtr linkFileItemToAdd = *(itFreelist);
    CStdString strIdentifier;
    bool isTrailer = (CFileItem::GetLinkBoxeeTypeAsEnum(linkFileItemToAdd->GetProperty("link-boxeetype")) == CLinkBoxeeType::TRAILER);
    if (isTrailer)
    {
      strIdentifier = linkFileItemToAdd->GetProperty("link-title") + linkFileItemToAdd->GetProperty("link-provider");
    }
    else
    {
      strIdentifier = linkFileItemToAdd->GetProperty("link-provider");
    }

    mapIt = addedLinksMap.find(strIdentifier);
    if (mapIt == addedLinksMap.end())
    {
      CFileItemPtr newItem(new CFileItem(m_item));
      newItem->ClearLinksList();
      // save some of the properties.
      newItem->m_strPath = linkFileItemToAdd->m_strPath;

      if (isTrailer)
      {
        newItem->SetLabel2(linkFileItemToAdd->GetProperty("link-title"));
        newItem->SetLabel("");
      }
      else
      {
        newItem->SetLabel2(linkFileItemToAdd->GetProperty("link-providername"));
        newItem->SetLabel(linkFileItemToAdd->GetLabel());
      }

      newItem->SetProperty("link-boxeetype", linkFileItemToAdd->GetProperty("link-boxeetype"));
      newItem->SetProperty("link-boxeeoffer", linkFileItemToAdd->GetProperty("link-boxeeoffer"));
      newItem->SetProperty("link-provider", linkFileItemToAdd->GetProperty("link-provider"));
      newItem->SetProperty("link-providername", linkFileItemToAdd->GetProperty("link-providername"));
      newItem->SetProperty("link-providerthumb", linkFileItemToAdd->GetProperty("link-providerthumb"));
      newItem->SetProperty("link-productslist", linkFileItemToAdd->GetProperty("link-productslist"));
      newItem->AddLink(linkFileItemToAdd->GetProperty("link-title"), linkFileItemToAdd->m_strPath,
                       linkFileItemToAdd->GetContentType(true), CFileItem::GetLinkBoxeeTypeAsEnum(linkFileItemToAdd->GetProperty("link-boxeetype")),
                       linkFileItemToAdd->GetProperty("link-provider"), linkFileItemToAdd->GetProperty("link-providername"), linkFileItemToAdd->GetProperty("link-providerthumb"),
                       linkFileItemToAdd->GetProperty("link-countrycodes"), linkFileItemToAdd->GetPropertyBOOL("link-countryrel"),
                       linkFileItemToAdd->GetProperty("quality-lbl"), linkFileItemToAdd->GetPropertyInt("quality"),
                       CFileItem::GetLinkBoxeeOfferAsEnum(linkFileItemToAdd->GetProperty("link-boxeeoffer")), linkFileItemToAdd->GetProperty("link-productslist"));

      if (linkFileItemToAdd->GetPropertyInt("quality") >= 720)
      {
        newItem->SetProperty("is-hd","true");
      }

      if (CFileItem::GetLinkBoxeeOfferAsEnum(newItem->GetProperty("link-boxeeoffer")) == CLinkBoxeeOffer::SUBSCRIPTION)
      {
        bool isEntitle = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().IsInEntitlements(newItem->GetProperty("link-productslist"));

        if (!isEntitle)
        {
          newItem->SetLabel(g_localizeStrings.Get(53774));
          newItem->SetProperty("NeedToSubscribe",true);
        }
      }

      // local files are special - we will treat it as different provider
      CLinkBoxeeType::LinkBoxeeTypeEnums linkBoxeeTypeEnum = CFileItem::GetLinkBoxeeTypeAsEnum(linkFileItemToAdd->GetProperty("link-boxeetype"));

      if (linkBoxeeTypeEnum != CLinkBoxeeType::LOCAL)
      {
        addedLinksMap[strIdentifier] =  newItem;
      }
      else
      {
        newItem->m_strPath = linkFileItemToAdd->m_strPath;
        newItem->SetLabel2(g_localizeStrings.Get(53750));
        CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::AddFileItemToList Local file - push it to the linked list");
      }

      m_linksFileItemList.push_back(newItem);

      addedPathsCounter++;
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::AddFileItemToList - [%d] - Add link item [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-boxeeoffer=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d][quality-lbl=%s][quality=%d][is-hd=%s][productsList=%s] (bbma)",addedPathsCounter,newItem->GetLabel().c_str(),newItem->m_strPath.c_str(),newItem->GetThumbnailImage().c_str(),newItem->GetProperty("link-title").c_str(),newItem->GetProperty("link-url").c_str(),newItem->GetProperty("link-boxeetype").c_str(),newItem->GetProperty("link-boxeeoffer").c_str(),newItem->GetProperty("link-type").c_str(),newItem->GetProperty("link-provider").c_str(),newItem->GetProperty("link-providername").c_str(),newItem->GetProperty("link-providerthumb").c_str(),newItem->GetProperty("link-countrycodes").c_str(),newItem->GetPropertyBOOL("link-countryrel"),newItem->GetProperty("quality-lbl").c_str(),newItem->GetPropertyInt("quality"),newItem->GetProperty("is-hd").c_str(),newItem->GetProperty("link-productslist").c_str());
    }
    else
    {
      CFileItemPtr Item = (mapIt)->second;

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::AddFileItemToList - Add link to existing item [%d]  [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-boxeeoffer=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d][quality-lbl=%s][quality=%d][is-hd=%s][productsList=%s] (bbma)",
                           addedPathsCounter,linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-boxeeoffer").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItemToAdd->GetPropertyBOOL("link-countryrel"),linkFileItemToAdd->GetProperty("quality-lbl").c_str(),linkFileItemToAdd->GetPropertyInt("quality"),linkFileItemToAdd->GetProperty("is-hd").c_str(),linkFileItemToAdd->GetProperty("link-productslist").c_str());

      Item->AddLink(linkFileItemToAdd->GetProperty("link-title"), linkFileItemToAdd->m_strPath,
          linkFileItemToAdd->GetContentType(true), CFileItem::GetLinkBoxeeTypeAsEnum(linkFileItemToAdd->GetProperty("link-boxeetype")),
          linkFileItemToAdd->GetProperty("link-provider"), linkFileItemToAdd->GetProperty("link-providername"), linkFileItemToAdd->GetProperty("link-providerthumb"),
          linkFileItemToAdd->GetProperty("link-countrycodes"), linkFileItemToAdd->GetPropertyBOOL("link-countryrel"),
          linkFileItemToAdd->GetProperty("quality-lbl"), linkFileItemToAdd->GetPropertyInt("quality"),
          CFileItem::GetLinkBoxeeOfferAsEnum(linkFileItemToAdd->GetProperty("link-boxeeoffer")), linkFileItemToAdd->GetProperty("link-productslist"));

      if (linkFileItemToAdd->GetPropertyInt("quality") >= 720)
      {
        Item->SetProperty("is-hd","true");
      }

    }
    itFreelist++;
  }

  return true;
}

bool CGUIDialogBoxeeMediaAction::BuildLinksFileItemList()
{
  const CFileItemList* linksFileItemList = m_item.GetLinksList();

  if (!linksFileItemList)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - FAILED to get the LinksFileItemList (bma)");
    return false;
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - Enter function. Item [label=%s] has [%d] links (bma)(bbma)", m_item.GetLabel().c_str(), linksFileItemList->Size());

  std::list<CFileItemPtr> localPathsList;
  std::list<CFileItemPtr> trailersPathsList;
  std::list<CFileItemPtr> freeWebPathsList;
  std::list<CFileItemPtr> subscribeServicePathsList;
  //CFileItemPtr subscribeServicePath;
  std::list<CFileItemPtr> otherPathsVec;

/*
  bool bIdentifyAsTrailerItem = true;

  for (int i=0; i<linksFileItemList->Size(); i++)
  {
    CFileItemPtr linkFileItemToAdd = linksFileItemList->Get(i);

    CLinkBoxeeType::LinkBoxeeTypeEnums linkBoxeeTypeEnum = CFileItem::GetLinkBoxeeTypeAsEnum(linkFileItemToAdd->GetProperty("link-boxeetype"));

    if (linkBoxeeTypeEnum != CLinkBoxeeType::TRAILER)
    {
      bIdentifyAsTrailerItem = false;
    }
  }

  m_item.SetProperty("IsTrailer",bIdentifyAsTrailerItem);
*/
  for (int i=0; i<linksFileItemList->Size(); i++)
  {
    CFileItemPtr linkFileItemToAdd = linksFileItemList->Get(i);

    bool isAllowed = linkFileItemToAdd->IsAllowed();
    if (!isAllowed)
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - [%d/%d] - Skipping link item [label=%s][path=%s][thumb=%s] because [MyCountryCode=%s][LinkCountryCodes=%s]->[isAllowed=%d]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][country-codes=%s][link-countrycodes=%s][country-rel=%d][link-countryrel=%d][quality-lbl=%s][quality=%d][is-hd=%s][productsList=%s] (bbma)",i+1,linksFileItemList->Size(),linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),g_application.GetCountryCode().c_str(),linkFileItemToAdd->GetProperty("country-codes").c_str(),isAllowed,linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("country-codes").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItemToAdd->GetPropertyBOOL("country-rel"),linkFileItemToAdd->GetPropertyBOOL("link-countryrel"),linkFileItemToAdd->GetProperty("quality-lbl").c_str(),linkFileItemToAdd->GetPropertyInt("quality"),linkFileItemToAdd->GetProperty("is-hd").c_str(),linkFileItemToAdd->GetProperty("link-productslist").c_str());
      continue;
    }

    CLinkBoxeeType::LinkBoxeeTypeEnums linkBoxeeTypeEnum = CFileItem::GetLinkBoxeeTypeAsEnum(linkFileItemToAdd->GetProperty("link-boxeetype"));

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - [%d/%d] - Handling link item [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-boxeeoffer=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d][quality-lbl=%s][quality=%d][is-hd=%s][productsList=%s] (bbma)",i+1,linksFileItemList->Size(),linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-boxeeoffer").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItemToAdd->GetPropertyBOOL("link-countryrel"),linkFileItemToAdd->GetProperty("quality-lbl").c_str(),linkFileItemToAdd->GetPropertyInt("quality"),linkFileItemToAdd->GetProperty("is-hd").c_str(),linkFileItemToAdd->GetProperty("link-productslist").c_str());

    switch (linkBoxeeTypeEnum)
    {
    case CLinkBoxeeType::LOCAL:
    {
      linkFileItemToAdd->SetLabel2(g_localizeStrings.Get(53750));

      // there is local path -> push to front of the list in order to show it first
      localPathsList.push_back(linkFileItemToAdd);

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - LOCAL - Add link item to localPaths [size=%zu]. [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-boxeeoffer=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d][quality-lbl=%s][quality=%d][is-hd=%s][productsList=%s] (bbma)",localPathsList.size(),linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-boxeeoffer").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItemToAdd->GetPropertyBOOL("link-countryrel"),linkFileItemToAdd->GetProperty("quality-lbl").c_str(),linkFileItemToAdd->GetPropertyInt("quality"),linkFileItemToAdd->GetProperty("is-hd").c_str(),linkFileItemToAdd->GetProperty("link-productslist").c_str());
    }
    break;
    case CLinkBoxeeType::FEATURE:
    {
      // there is "free with adds" path -> push to back of the list in in case there is local path
      freeWebPathsList.push_back(linkFileItemToAdd);

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - FEATURE - Add link item to freeWebPaths [size=%zu]. [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-boxeeoffer=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d][quality-lbl=%s][quality=%d][is-hd=%s][productsList=%s] (bbma)",freeWebPathsList.size(),linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-boxeeoffer").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItemToAdd->GetPropertyBOOL("link-countryrel"),linkFileItemToAdd->GetProperty("quality-lbl").c_str(),linkFileItemToAdd->GetPropertyInt("quality"),linkFileItemToAdd->GetProperty("is-hd").c_str(),linkFileItemToAdd->GetProperty("link-productslist").c_str());
    }
    break;
    case CLinkBoxeeType::TRAILER:
    {
      trailersPathsList.push_back(linkFileItemToAdd);
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - TRAILER - Add link item to trailersPathsList [size=%zu]. [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-boxeeoffer=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d][quality-lbl=%s][quality=%d][is-hd=%s][productsList=%s] (bbma)",trailersPathsList.size(),linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-boxeeoffer").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItemToAdd->GetPropertyBOOL("link-countryrel"),linkFileItemToAdd->GetProperty("quality-lbl").c_str(),linkFileItemToAdd->GetPropertyInt("quality"),linkFileItemToAdd->GetProperty("is-hd").c_str(),linkFileItemToAdd->GetProperty("link-productslist").c_str());

      /*
      if (m_item.GetPropertyBOOL("IsTrailer"))
      {
        trailersPathsList.push_back(linkFileItemToAdd);
        CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - TRAILER - Add link item to trailersPathsList [size=%zu]. [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-boxeeoffer=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d][quality-lbl=%s][quality=%d][is-hd=%s][productsList=%s] (bbma)",trailersPathsList.size(),linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-boxeeoffer").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItemToAdd->GetPropertyBOOL("link-countryrel"),linkFileItemToAdd->GetProperty("quality-lbl").c_str(),linkFileItemToAdd->GetPropertyInt("quality"),linkFileItemToAdd->GetProperty("is-hd").c_str(),linkFileItemToAdd->GetProperty("link-productslist").c_str());
      }
      else if (!m_trailerLinkItem.get())
      {
        m_trailerLinkItem = linkFileItemToAdd;
        CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - TRAILER - Save link item as first trailerPath [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-boxeeoffer=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d][quality-lbl=%s][quality=%d][is-hd=%s][productsList=%s] (bbma)",linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-boxeeoffer").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItemToAdd->GetPropertyBOOL("link-countryrel"),linkFileItemToAdd->GetProperty("quality-lbl").c_str(),linkFileItemToAdd->GetPropertyInt("quality"),linkFileItemToAdd->GetProperty("is-hd").c_str(),linkFileItemToAdd->GetProperty("link-productslist").c_str());
      }
      */
    }
    break;
    /*
    case CLinkBoxeeType::SUBSCRIPTION:
    {
      if (Boxee::GetInstance().GetBoxeeClientServerComManager().IsRegisterToServices(linkFileItemToAdd->GetProperty("link-provider"),CServiceIdentifierType::NAME))
      {
        // a subscribe service that the user is subscribe to

        subscribeServicePathsList.push_back(linkFileItemToAdd);

        CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - SUBSCRIPTION - Add link item to subscribeServicePaths [size=%d]. [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d][quality-lbl=%s][quality=%d][is-hd=%s][productsList=%s] (bbma)",(int)subscribeServicePathsList.size(),linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItemToAdd->GetPropertyBOOL("link-countryrel"),linkFileItemToAdd->GetProperty("quality-lbl").c_str(),linkFileItemToAdd->GetPropertyInt("quality"),linkFileItemToAdd->GetProperty("is-hd").c_str(),linkFileItemToAdd->GetProperty("link-productslist").c_str());
      }
      else
        {
        otherPathsVec.push_back(linkFileItemToAdd);

        CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - SUBSCRIPTION - Add link item to otherPaths [size=%d]. [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d][quality-lbl=%s][quality=%d][is-hd=%s][productsList=%s] (bbma)",(int)otherPathsVec.size(),linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItemToAdd->GetPropertyBOOL("link-countryrel"),linkFileItemToAdd->GetProperty("quality-lbl").c_str(),linkFileItemToAdd->GetPropertyInt("quality"),linkFileItemToAdd->GetProperty("is-hd").c_str(),linkFileItemToAdd->GetProperty("link-productslist").c_str());
        }
      }
    break;
    */
    default:
    {
      otherPathsVec.push_back(linkFileItemToAdd);

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - OTHER - Add link item to otherPaths [size=%zu]. [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-boxeeoffer=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d][quality-lbl=%s][quality=%d][is-hd=%s][productsList=%s] (bbma)",otherPathsVec.size(),linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-boxeeoffer").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItemToAdd->GetPropertyBOOL("link-countryrel"),linkFileItemToAdd->GetProperty("quality-lbl").c_str(),linkFileItemToAdd->GetPropertyInt("quality"),linkFileItemToAdd->GetProperty("is-hd").c_str(),linkFileItemToAdd->GetProperty("link-productslist").c_str());
    }
    break;
    }
  }

  std::map<CStdString, CFileItemPtr> addedLinksMap;

  //Here we define the sequence of the links showed in the media action dialog

  if (m_item.GetPropertyBOOL("IsTrailer"))
  {
    // if trailer item -> trailer link should appear first
    AddFileItemToList(addedLinksMap, trailersPathsList);
  }

  ///////////////////////////
  // Enter all local paths //
  ///////////////////////////
  AddFileItemToList(addedLinksMap, localPathsList);

  ///////////////////////////////////////
  // Enter all user subscribe services //
  ///////////////////////////////////////
  AddFileItemToList(addedLinksMap, subscribeServicePathsList);

  //////////////////////////////
  // Enter all web free paths //
  //////////////////////////////
  AddFileItemToList(addedLinksMap, freeWebPathsList);

  /////////////////////////////
  // Enter all trailer paths //
  /////////////////////////////

  if (!m_item.GetPropertyBOOL("IsTrailer"))
  {
    AddFileItemToList(addedLinksMap, trailersPathsList);
  }

  ///////////////////////////
  // Enter all other paths //
  ///////////////////////////
  AddFileItemToList(addedLinksMap, otherPathsVec);

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - Exit function. [LinksFileItemListSize=%d] (bbma)",(int)m_linksFileItemList.size());

  return true;
}

bool CGUIDialogBoxeeMediaAction::OnAction(const CAction& action)
{
  switch (action.id)
  {
  case ACTION_PARENT_DIR:
  case ACTION_PREVIOUS_MENU:
  {
    CGUIBaseContainer* pControl = (CGUIBaseContainer*)GetControl(INFO_GROUP);
    if (pControl && pControl->IsVisible())
    {
      SET_CONTROL_HIDDEN(INFO_GROUP);
      SET_CONTROL_VISIBLE(BUTTONS_LIST_GROUP);
    }
    else
    {
      m_bConfirmed = false;
      Close();
    }

    return true;
  }
  break;
  case ACTION_MOUSE:
  case ACTION_MOVE_LEFT:
  case ACTION_MOVE_RIGHT:
  case ACTION_MOVE_UP:
  case ACTION_MOVE_DOWN:
  {
    bool retVal = CGUIDialog::OnAction(action);

    CFileItem* linkItem = NULL;

    int focusedControl = GetFocusedControlID();

    if (focusedControl == ADDITIONAL_LINKS_LIST)
    {
      CGUIBaseContainer* pLinkListControl = (CGUIBaseContainer*)GetControl(ADDITIONAL_LINKS_LIST);

      if (pLinkListControl)
      {
        int focusedLinkIndex = pLinkListControl->GetSelectedItem();
        std::vector< CGUIListItemPtr >& listItems = pLinkListControl->GetItemsByRef();

        if (focusedLinkIndex <= (int)listItems.size()-1)
        {
          CGUIListItemPtr focusedLinkItem = listItems[focusedLinkIndex];

          if (focusedLinkItem.get())
          {
            linkItem = ((CFileItem*)focusedLinkItem.get());
          }
        }
      }
    }

    if (linkItem)
    {
      SetLinkPathAsWindowProperty(linkItem);
    }
    else
    {
      SetLinkPathAsWindowProperty("");
    }

    return retVal;
  }
  break;
  default:
  {
    // do nothing
  }
  break;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeeMediaAction::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    return OnClick(message);
  }
  break;
  case GUI_MSG_ITEM_LOADED:
  {
    CFileItem* pItem = (CFileItem *)message.GetPointer();
    message.SetPointer(NULL);
    if (pItem)
    {
      m_item = *pItem;
      delete pItem;

      OnInitWindow();
    }
  }
  break;
  case GUI_MSG_PLAYLIST_CHANGED:
  {
    // 171110 - ugly hack - temp fix for box getting hang when try to play playlist
    return true;
  }
  break;
  default:
  {
    // do nothing
  }
  break;
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogBoxeeMediaAction::OnClick(CGUIMessage& message)
{
  bool succeeded = false;

  int iControl = message.GetSenderId();

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnClick - Enter function with [iControl=%d]. [ListContainServerLinks=%d] (bma)",iControl,m_listContainServerLinks);

  switch(iControl)
  {
  case ADDITIONAL_LINKS_LIST:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnClick - Handling click on [%d=LINK_LIST] (bma)",iControl);

    succeeded = HandleClickOnItemList();

    if (succeeded)
    {
      Close(true);
    }
  }
  break;
  case BUTTONS_LIST_MOVIE:
  case BUTTONS_LIST_NOT_MOVIE:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnClick - Handling click on [%d=BUTTONS_LIST]. [visibleButtonsListId=%d] (bma)",iControl,m_visibleButtonsListId);

    bool needToCloseDialog = false;

    succeeded = HandleClickOnButtonList(needToCloseDialog);

    if (needToCloseDialog)
    {
      Close(true);
    }
  }
  break;
  case ADDITIONAL_MOVIE_BUTTONS_LIST:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnClick - Handling click on [%d=ADDITIONAL_MOVIE_BUTTONS_LIST] (bma)",iControl);

    return HandleClickOnMovieAdditinalButtonList();
  }
  break;
  case TRAILER_LIST:
  case TRAILER_BUTTON:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnClick - Handling click on trailer control [%d] (bma)",iControl);

    succeeded = HandleClickOnTrailer();

    if (succeeded)
    {
      Close(true);
    }
  }
  break;
  case BACK_GROUP:
  {
    SET_CONTROL_HIDDEN(INFO_GROUP);
    SET_CONTROL_VISIBLE(BUTTONS_LIST_GROUP);
  }
  break;
  /*
  case PLAY_BUTTON:
  {
    bool succeeded = false;

    CGUILabelControl* pControl = (CGUILabelControl*) GetControl(PLAY_BUTTON_LABEL_FOCUS);
    CStdString playButtonLabel = pControl->GetLabel();

    if (playButtonLabel == g_localizeStrings.Get(53755))
    {
      succeeded = OnBrowse(m_item);
    }
    else if (m_item.IsApp())
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnClick - Handling click on [%d=PLAY_BUTTON] with [IsApp=%d] item (bma)",iControl,m_item.IsApp());
      succeeded = HandleClickOnLuanchApp();
    }
    else
    {
    CFileItemPtr clickedLinkFileItem;

    if (m_playButtonItemIndex == PLAY_BUTTON_INIT_WITH_TAILER)
    {
      clickedLinkFileItem = m_trailerLinkItem;
    }
    else if (m_playButtonItemIndex >= 0)
    {
      clickedLinkFileItem = m_linksFileItemList[m_playButtonItemIndex];
    }
    else
    {
      clickedLinkFileItem = CFileItemPtr(new CFileItem(m_item));
    }

    // update item for play
    UpdateItemWithLinkData(clickedLinkFileItem);

    if (BoxeeUtils::CanResume(m_item))
    {
      succeeded = OnResume(m_item);
    }
    else
    {
      succeeded = OnPlay(m_item);
    }
    }

    if (succeeded)
    {
      Close(true);
    }
  }
  break;
  */
  case APP_LUANCH_BUTTON:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnClick - Handling click on [%d=APP_LUANCH_BUTTON] (bma)",iControl);

    Close(true);

    succeeded = HandleClickOnLuanchApp();
  }
  break;
  case APP_ADD_BUTTON:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnClick - Handling click on [%d=APP_ADD_BUTTON] (bma)",iControl);

    Close(true);

    succeeded = HandleClickOnAddApp();
  }
  break;
  case APP_REMOVE_BUTTON:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnClick - Handling click on [%d=APP_REMOVE_BUTTON] (bma)",iControl);

    Close(true);

    succeeded = HandleClickOnRemoveApp();
  }
  break;
  case APP_SHORTCUT_BUTTON:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnClick - APP_ADD_SHORTCUT_BUTTON - Handling click on [%d=APP_ADD_SHORTCUT_BUTTON] (bma)",iControl);

    CGUIButtonControl* pControl = (CGUIButtonControl*) GetControl(APP_SHORTCUT_BUTTON);

    if (pControl)
    {
      CStdString shortcutButtonLabel = pControl->GetLabel();

      bool succeeded = false;

      if (shortcutButtonLabel == g_localizeStrings.Get(53715))
      {
        ///////////////////////////
        // click on ADD_SHORTCUT //
        ///////////////////////////

        CBoxeeShortcut shortcut(m_item);

        succeeded = AddShortcut(shortcut);

        if (succeeded)
        {
          SET_CONTROL_LABEL(APP_SHORTCUT_BUTTON,g_localizeStrings.Get(53716));
        }
      }
      else if (shortcutButtonLabel == g_localizeStrings.Get(53716))
      {
        //////////////////////////////
        // click on REMOVE_SHORTCUT //
        //////////////////////////////

        CBoxeeShortcut shortcut(m_item);

        succeeded = RemoveShortcut(shortcut);

        if (succeeded)
        {
          SET_CONTROL_LABEL(APP_SHORTCUT_BUTTON,g_localizeStrings.Get(53715));
        }
      }
      else
      {
        CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::OnClick - APP_SHORTCUT_BUTTON - Button [%d] has an unknown label [%s] (bma)",iControl,shortcutButtonLabel.c_str());
      }
    }
    else
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::OnClick - APP_SHORTCUT_BUTTON - FAILED to get control [%d] (bma)",iControl);
    }
  }
  break;
  default:
  {
    CLog::Log(LOGWARNING,"CGUIDialogBoxeeMediaAction::OnClick - UNKNOWN control [%d] was click (bma)",iControl);
  }
  break;
  }

  return succeeded;
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnLuanchApp()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnLuanchApp - Going to call CBoxeeItemLauncher::Launch with item [label=%s][path=%s] (bma)",m_item.GetLabel().c_str(),m_item.m_strPath.c_str());

  if (!g_application.IsConnectedToInternet())
  {
    CStdString errorStr = g_localizeStrings.Get(53766);
    CStdString message;
    message.Format(errorStr.c_str(), m_item.GetLabel());

    CGUIDialogBoxeeNetworkNotification::ShowAndGetInput(g_localizeStrings.Get(53743),message);
    return true;
  }

  return CAppManager::GetInstance().Launch(m_item, false);
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnAddApp()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnAddApp - Enter function. item is [label=%s][path=%s] (bma)",m_item.GetLabel().c_str(),m_item.m_strPath.c_str());

    bool succeeded = false;
    CStdString message;
    CURI url(m_item.m_strPath);
    if (url.GetProtocol() == "app")
    {
      InstallOrUpgradeAppBG* job = new InstallOrUpgradeAppBG(url.GetHostName(), true, false);
      if (CUtil::RunInBG(job) == JOB_SUCCEEDED)
      {
        message = g_localizeStrings.Get(52016);
        succeeded = true;
      }
      else
      {
        CStdString errorStr = g_localizeStrings.Get(52017);
        message.Format(errorStr.c_str(), m_item.GetLabel());
      }
    }
    else
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::HandleClickOnItemList - FAILED to install app [label=%s]. Invalid path [path=%s] (bapps)",m_item.GetLabel().c_str(),m_item.m_strPath.c_str());

      CStdString errorStr = g_localizeStrings.Get(52017);
      message.Format(errorStr.c_str(), m_item.GetLabel());
    }

    if (succeeded)
    {
      //g_application.m_guiDialogKaiToast.QueueNotification("", "",g_localizeStrings.Get(53824), 5000);

      CGUIMessage refreshAppsWinMsg(GUI_MSG_UPDATE, WINDOW_BOXEE_BROWSE_APPS, 0);
      g_windowManager.SendThreadMessage(refreshAppsWinMsg);
    }
    else
    {
      CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(52039), message);
    }

    return succeeded;
  }

bool CGUIDialogBoxeeMediaAction::HandleClickOnRemoveApp()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnRemoveApp - Enter function. item is [label=%s][path=%s] (bma)",m_item.GetLabel().c_str(),m_item.m_strPath.c_str());

  if (m_item.GetPropertyBOOL("testapp"))
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseApps::HandleClickOnRemoveApp - Clicked app [label=%s][path=%s] is a TestApp -> Need to remove it manually (bapps)",m_item.GetLabel().c_str(),m_item.m_strPath.c_str());

    CStdString heading = g_localizeStrings.Get(53820);
    CStdString line;
    CStdString str = g_localizeStrings.Get(53821);
    line.Format(str.c_str(), m_item.GetLabel());

    CGUIDialogOK2::ShowAndGetInput(heading, line);
    return true;
  }
  else
  {
    CStdString heading = g_localizeStrings.Get(53914);
    CStdString line;
    CStdString str = g_localizeStrings.Get(53822);
    line.Format(str.c_str(), m_item.GetLabel());

    if (CGUIDialogYesNo2::ShowAndGetInput(heading, line))
    {
      CStdString appId = m_item.GetProperty("appid");

      if (appId.IsEmpty())
      {
        CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseApps::HandleClickOnRemoveApp - FAILED to get appId for AppItem [label=%s] (bapps)",m_item.GetLabel().c_str());
        return false;
      }

      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseApps::HandleClickOnRemoveApp - Going to remove application with [appid=%s]. [ItemLabel=%s] (bapps)",appId.c_str(),m_item.GetLabel().c_str());

      CGUIDialogProgress* progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
      if (progress)
      {
        progress->StartModal();
        progress->Progress();
      }

      // report to the server about the removed app
      bool succeeded = BoxeeUtils::ReportRemoveApp(appId);

      if (progress)
      {
        progress->Close();
      }

      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseApps::HandleClickOnRemoveApp - Call to ReportRemoveApp for app [label=%s] returned [%d] (bapps)",m_item.GetLabel().c_str(),succeeded);

      if (!succeeded)
      {
        heading = g_localizeStrings.Get(53701);
        line = "";
        str = g_localizeStrings.Get(53823);
        line.Format(str.c_str(), m_item.GetLabel());

        CGUIDialogOK2::ShowAndGetInput(heading, line);
      }
      else
      {
        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseApps::HandleClickOnRemoveApp - App was removed. Going to refresh the screen (bapps)");

        CGUIMessage refreshAppsWinMsg(GUI_MSG_UPDATE, WINDOW_BOXEE_BROWSE_APPS, 0);
        g_windowManager.SendThreadMessage(refreshAppsWinMsg);
      }

      return succeeded;
    }
    else
    {
      return true;
    }
  }
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnItemList()
{
  bool retVal;

  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), ADDITIONAL_LINKS_LIST);
  g_windowManager.SendMessage(msg);
  int itemIndex = msg.GetParam1();

  if ((itemIndex < 0) || (itemIndex >= (int)m_linksFileItemList.size()))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::HandleClickOnItemList - Index of item clicked is [%d] while [LinksFileItemListSize=%zu] (bma)",itemIndex,m_linksFileItemList.size());
    return false;
  }

  CFileItemPtr clickedLinkFileItem = m_linksFileItemList[itemIndex];

  clickedLinkFileItem->Dump();

  if (clickedLinkFileItem->GetLabel2().CompareNoCase(g_localizeStrings.Get(53755)) == 0)
  {
    retVal = OnBrowse(m_item);
  }
  else
  {
    // update item for play
    UpdateItemWithLinkData(clickedLinkFileItem);

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnItemList - Click on item [label=%s][path=%s] so MainItem path was set to [%s]. [contenttype=%s][link-boxeetype=%s] (bma)",clickedLinkFileItem->GetLabel().c_str(),clickedLinkFileItem->m_strPath.c_str(),m_item.m_strPath.c_str(),m_item.GetProperty("contenttype").c_str(),m_item.GetProperty("link-boxeetype").c_str());

    if (NeedToSubscribe())
    {
      // in case the user clicked on a link that need Subscription -> go to Subscription dialog
      if (!OnSubscription())
      {
        CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::HandleClickOnItemList - FAILED to subscribe (bma)(pay)");
        return false;
      }

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnItemList - Succeeded to subscribe. Continue... (bma)(pay)");
    }

    if (BoxeeUtils::CanResume(m_item))
    {
      retVal = OnResume(m_item);
    }
    else
    {
      retVal = OnPlay(m_item);
    }
  }

  return retVal;
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnMovieAdditinalButtonList()
{
  CGUIBaseContainer* pContainer = (CGUIBaseContainer*)GetControl(ADDITIONAL_MOVIE_BUTTONS_LIST);

  if (!pContainer)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::HandleClickOnMovieAdditinalButtonList - FAILED to get container [%d=ADDITIONAL_MOVIE_BUTTONS_LIST] container (bma)",ADDITIONAL_MOVIE_BUTTONS_LIST);
    return false;
  }

  CGUIListItemPtr selectedButton = pContainer->GetSelectedItemPtr();
  if (!selectedButton.get())
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::HandleClickOnMovieAdditinalButtonList - FAILED to get the SelectedItem from container [%d=ADDITIONAL_MOVIE_BUTTONS_LIST] container (bma)",ADDITIONAL_MOVIE_BUTTONS_LIST);
    return false;
  }

  CStdString selectedButtonLabel = selectedButton->GetLabel();

  if (selectedButtonLabel == g_localizeStrings.Get(90452))
  {
    g_settings.SetSkinString(g_settings.TranslateSkinString("MediaAction"),"Overview");
  }
  else if (selectedButtonLabel == g_localizeStrings.Get(90453))
  {
    g_settings.SetSkinString(g_settings.TranslateSkinString("MediaAction"),"CastCrew");
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::HandleClickOnMovieAdditinalButtonList - FAILED to handle SelectedItem [label=%s] (bma)",selectedButtonLabel.c_str());
    return false;
  }

  CGUIMessage winmsg(GUI_MSG_UNMARK_ALL_ITEMS, GetID(), ADDITIONAL_MOVIE_BUTTONS_LIST);
  OnMessage(winmsg);

  pContainer->SetSingleSelectedItem();

  return true;
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnButtonList(bool& needToCloseDialog)
{
  m_visibleButtonsListId = GetVisibleButtonListControlId();

  if (m_visibleButtonsListId == 0)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::FillDialogButtons - FAILED to get visible ButtonList id (bma)");
    return false;
  }

  CGUIBaseContainer* pContainer = (CGUIBaseContainer*)GetControl(m_visibleButtonsListId);

  if (!pContainer)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - FAILED to get container [visibleButtonsListId=%d] (bma)",m_visibleButtonsListId);
    return false;
  }

  CGUIListItemPtr selectedButton = pContainer->GetSelectedItemPtr();
  if (!selectedButton.get())
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - FAILED to get the SelectedItem from container [visibleButtonsListId=%d] container (bma)",m_visibleButtonsListId);
    return false;
  }

  //CStdString selectedButtonLabel = selectedButton->GetLabel();
  //CDialogButtons::DialogButtonsEnums selectedButtonEnum = GetButtonAsEnum(selectedButtonLabel);

  CStdString selectedButtonLabel = selectedButton->GetProperty(BUTTON_ACTION_PROPERTY_NAME);
  CDialogButtons::DialogButtonsEnums selectedButtonEnum = GetButtonActionPropertyAsEnum(selectedButtonLabel);

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - Enter function with [selectedButtonLabel=%s][selectedButtonEnum=%d] (bma)",selectedButtonLabel.c_str(),selectedButtonEnum);

  CStdString newLabelToUpdate = "";
  CStdString newThumbToUpdate = "";

  bool succeeded = false;

  switch(selectedButtonEnum)
  {
  case CDialogButtons::BTN_READ_MORE:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - Handling click on [%s=%d=BTN_READ_MORE] (bma)",selectedButtonLabel.c_str(),selectedButtonEnum);

    succeeded = HandleClickOnReadMore();
  }
  break;
  case CDialogButtons::BTN_TRAILER:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - Handling click on [%s=%d=BTN_TRAILER] (bma)",selectedButtonLabel.c_str(),selectedButtonEnum);

    succeeded = HandleClickOnTrailer();

    if (succeeded)
    {
    needToCloseDialog = true;
  }
  }
  break;
  case CDialogButtons::BTN_SHARE:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - Handling click on [%s=%d=BTN_SHARE] (bma)",selectedButtonLabel.c_str(),selectedButtonEnum);

    succeeded = HandleClickOnShare();

    // do not close the action dialog
    needToCloseDialog = false;
  }
  break;
  case CDialogButtons::BTN_EJECT:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - Handling click on [%s=%d=BTN_EJECT] (bma)",selectedButtonLabel.c_str(),selectedButtonEnum);

    HandleClickOnEject();

    needToCloseDialog = true;
  }
  break;
  case CDialogButtons::BTN_BROWSE:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - Handling click on [%s=%d=BTN_BROWSE] (bma)",selectedButtonLabel.c_str(),selectedButtonEnum);

    needToCloseDialog = HandleClickOnBrowse();
  }
  break;
  case CDialogButtons::BTN_RESOLVE:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - Handling click on [%s=%d=BTN_BROWSE] (bma)",selectedButtonLabel.c_str(),selectedButtonEnum);

    needToCloseDialog = HandleClickOnResolve();
  }
  break;
  case CDialogButtons::BTN_ADD_TO_QUEUE:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - Handling click on [%s=%d=BTN_ADD_TO_QUEUE] (bma)",selectedButtonLabel.c_str(),selectedButtonEnum);

    succeeded = HandleClickOnAddToQueue(newLabelToUpdate, newThumbToUpdate);

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - Call to HandleClickOnAddToQueue returned [succeeded=%d]. [newLabelToUpdate=%s][newThumbToUpdate=%s] (bma)",succeeded,newLabelToUpdate.c_str(),newThumbToUpdate.c_str());

    if (succeeded)
    {
      CStdString referral;

      if (BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().IsInQueue(m_item.GetProperty("boxeeid"),m_item.m_strPath,referral))
      {
        CStdString orgButtonAction = selectedButton->GetProperty(BUTTON_ACTION_PROPERTY_NAME);

        m_item.SetProperty("referral",referral);
        selectedButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_REMOVE_FROM_QUEUE);

        CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - After queue item [label=%s], set [button-action=%s] and added [referral=%s] for future dequeue. [orgButtonAction=%s] (queue)(bma)",m_item.GetLabel().c_str(),selectedButton->GetProperty(BUTTON_ACTION_PROPERTY_NAME).c_str(),m_item.GetProperty("referral").c_str(),orgButtonAction.c_str());
      }
      else
      {
        CLog::Log(LOGWARNING,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - After queue item [label=%s], FAILED to find it in the queue for getting referral (queue)(bma)",m_item.GetLabel().c_str());
      }
    }
  }
  break;
  case CDialogButtons::BTN_REMOVE_FROM_QUEUE:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - Handling click on [%s=%d=BTN_REMOVE_FROM_QUEUE] (bma)",selectedButtonLabel.c_str(),selectedButtonEnum);

    succeeded = HandleClickOnRemoveFromQueue(newLabelToUpdate, newThumbToUpdate);

    if (succeeded)
    {
      CStdString orgButtonAction = selectedButton->GetProperty(BUTTON_ACTION_PROPERTY_NAME);

      selectedButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_ADD_TO_QUEUE);

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - After dequeue item [label=%s], set [button-action=%s]. [orgButtonAction=%s] (queue)(bma)",m_item.GetLabel().c_str(),selectedButton->GetProperty(BUTTON_ACTION_PROPERTY_NAME).c_str(),orgButtonAction.c_str());

      int activeWindow = g_windowManager.GetActiveWindow();

      if (activeWindow == WINDOW_BOXEE_BROWSE_QUEUE)
      {
        needToCloseDialog = true;
      }
    }
  }
  break;
  case CDialogButtons::BTN_MARK_AS_SEEN:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - Handling click on [%s=%d=BTN_MARK_AS_SEEN] (bma)",selectedButtonLabel.c_str(),selectedButtonEnum);

    succeeded = HandleClickOnMarkAsSeen(newLabelToUpdate, newThumbToUpdate);

    if (succeeded)
    {
      selectedButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_MARK_AS_UNSEEN);
  }
  }
  break;
  case CDialogButtons::BTN_MARK_AS_UNSEEN:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - Handling click on [%s=%d=BTN_MARK_AS_UNSEEN] (bma)",selectedButtonLabel.c_str(),selectedButtonEnum);

    succeeded = HandleClickOnMarkAsUnseen(newLabelToUpdate, newThumbToUpdate);

    if (succeeded)
    {
      selectedButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_MARK_AS_SEEN);
    }
  }
  break;
  case CDialogButtons::BTN_ADD_SHORTCUT:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - Handling click on [%s=%d=BTN_ADD_SHORTCUT] (bma)",selectedButtonLabel.c_str(),selectedButtonEnum);

    succeeded = HandleClickOnAddShortcut(newLabelToUpdate, newThumbToUpdate);

    if (succeeded)
    {
      selectedButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_REMOVE_FROM_SHORTCUT);
  }
  }
  break;
  case CDialogButtons::BTN_REMOVE_FROM_SHORTCUT:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - Handling click on [%s=%d=BTN_REMOVE_FROM_SHORTCUT] (bma)",selectedButtonLabel.c_str(),selectedButtonEnum);

    succeeded = HandleClickOnRemoveFromShortcut(newLabelToUpdate, newThumbToUpdate);

    if (succeeded)
    {
      selectedButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_ADD_AS_SHORTCUT);
    }
  }
  break;
  case CDialogButtons::BTN_GOTO_SHOW:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - Handling click on [%s=%d=BTN_GOTO_SHOW] (bma)",selectedButtonLabel.c_str(),selectedButtonEnum);

    succeeded = HandleClickOnGotoShow();

    if (succeeded)
    {
      needToCloseDialog = true;
    }
  }
  break;
  case CDialogButtons::BTN_REMOVE_FROM_HISTORY:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - Handling click on [%s=%d=BTN_REMOVE_FROM_HISTORY] (bma)",selectedButtonLabel.c_str(),selectedButtonEnum);

    succeeded = HandleClickOnRemoveFromHistory();

    if (succeeded)
    {
      needToCloseDialog = true;
    }
  }
  break;
  case CDialogButtons::BTN_ADD_TO_FAVORITE:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - Handling click on [%s=%d=BTN_ADD_TO_FAVORITE] (bma)",selectedButtonLabel.c_str(),selectedButtonEnum);

    succeeded = HandleClickOnAddToFavorite(newLabelToUpdate, newThumbToUpdate);

    if (succeeded)
    {
      selectedButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_REMOVE_FROM_FAVORITE);
    }
  }
  break;
  case CDialogButtons::BTN_REMOVE_FROM_FAVORITE:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - Handling click on [%s=%d=BTN_REMOVE_FROM_FAVORITE] (bma)",selectedButtonLabel.c_str(),selectedButtonEnum);

    succeeded = HandleClickOnRemoveFromFavorite(newLabelToUpdate, newThumbToUpdate);

    if (succeeded)
    {
      selectedButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_ADD_TO_FAVORITE);
    }
  }
  break;
  default:
  {
    CLog::Log(LOGWARNING,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - UNKNOWN button was click. [%s=%d] (bma)",selectedButtonLabel.c_str(),selectedButtonEnum);
  }
  break;
  }

  if (!newLabelToUpdate.IsEmpty())
  {
    CStdString orgLabel = selectedButton->GetLabel();

    selectedButton->SetLabel(newLabelToUpdate);

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - Button label was updated to [newLabel=%s]. [orgLabel=%s] (bma)",selectedButton->GetLabel().c_str(),orgLabel.c_str());
  }

  if (!newThumbToUpdate.IsEmpty())
  {
    CStdString orgThumb = selectedButton->GetThumbnailImage();

    selectedButton->SetThumbnailImage(newThumbToUpdate);

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - Button thumb was updated to [newThumb=%s]. [orgThumb=%s] (bma)",selectedButton->GetThumbnailImage().c_str(),orgThumb.c_str());
  }

  return succeeded;
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnReadMore()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnReadMore - Enter function (bma)");

  SET_CONTROL_VISIBLE(INFO_GROUP);
  SET_CONTROL_FOCUS(MORE_INFO_SCROLLBAR,0);

  return true;
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnTrailer()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnTrailer - Enter function (bma)");

  bool succeeded = false;
/*
  CFileItemList trailersList;

  for (int i = 0 ; i < m_item.GetLinksList()->Size() ; i++)
  {
    CFileItemPtr currentItem = m_item.GetLinksList()->Get(i);
    if (CFileItemList::GetLinkBoxeeTypeAsEnum(currentItem->GetProperty("link-boxeetype")) == CLinkBoxeeType::TRAILER)
    {
      trailersList.Add(currentItem);
    }
  }

  if (trailersList.Size() > 1)
  {
    CGUIDialogBoxeeSelectionList *pDlgSelectionList = (CGUIDialogBoxeeSelectionList*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_SELECTION_LIST);
    pDlgSelectionList->Reset();
    pDlgSelectionList->SetTitle(g_localizeStrings.Get(12034));
    pDlgSelectionList->Set(trailersList);
    pDlgSelectionList->DoModal();

    if (pDlgSelectionList->IsCanceled())
    {
      return false;
    }

    int chosenItem = pDlgSelectionList->GetSelectedItemPos();

    if (chosenItem >= 0 && chosenItem < trailersList.Size())
      m_trailerLinkItem = trailersList.Get(chosenItem);
  }*/

  if (m_trailerLinkItem.get())
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnTrailer - going to launch following trailer item [path=%s] (bma)",m_trailerLinkItem->m_strPath.c_str());
    m_trailerLinkItem->Dump();

    if (m_trailerLinkItem->IsInternetStream() && !g_application.IsConnectedToInternet())
    {
      CGUIDialogBoxeeNetworkNotification::ShowAndGetInput(53743,53746);
      return false;
    }

    CStdString trailetLabel = m_item.GetLabel();
    trailetLabel += " (";
    trailetLabel += g_localizeStrings.Get(53777);
    trailetLabel += ")";
    CFileItem trailerItem(trailetLabel);
    trailerItem.SetThumbnailImage(m_item.GetThumbnailImage());
    trailerItem.SetProperty("OriginalThumb",m_item.GetProperty("OriginalThumb"));
    trailerItem.SetProperty("description",m_item.GetProperty("description"));
    trailerItem.SetProperty("isVideo", true);
    trailerItem.m_strPath = m_trailerLinkItem->m_strPath;
    trailerItem.SetProperty("isinternetstream", m_trailerLinkItem->IsInternetStream());
    trailerItem.SetContentType(m_trailerLinkItem->GetContentType(true));

    succeeded = OnPlay(trailerItem);
  }
  else
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnTrailer - Trailer item is empty (bma)");
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnTrailer - Exit function and return [succeeded=%d] (bma)",succeeded);

  return succeeded;
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnShare()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnShare - Enter function (bma)");

  if (!g_application.IsConnectedToInternet())
  {
    CGUIDialogBoxeeNetworkNotification::ShowAndGetInput(53743,53747);
    return true;
  }

  bool succeeded = false;

  CGUIDialogBoxeeShare* pShare = (CGUIDialogBoxeeShare *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_SHARE);

  if (pShare)
  {
    m_item.Dump();

    pShare->SetItem(&m_item);
    pShare->DoModal();
    succeeded = true;
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::HandleClickOnShare - FAILED to get WINDOW_DIALOG_BOXEE_SHARE (bma)");
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnShare - Exit function and return [succeeded=%d] (bma)",succeeded);

  return succeeded;
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnAddToQueue(CStdString& newLabelToUpdate, CStdString& newThumbToUpdate)
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnAddToQueue - Enter function. Going to QUEUE item (bma)(queue)");

  if (!g_application.IsConnectedToInternet())
  {
    CGUIDialogBoxeeNetworkNotification::ShowAndGetInput(53743,53748);
    return false;
  }

  bool succeeded = BoxeeUtils::Queue(&m_item, true);

  if (succeeded)
  {
    //g_application.m_guiDialogKaiToast.QueueNotification(g_localizeStrings.Get(53780), "", g_localizeStrings.Get(53731), 2000);

    if (g_windowManager.GetActiveWindow() == WINDOW_BOXEE_BROWSE_QUEUE)
    {
      m_refreshActiveWindow = true;
    }

    newLabelToUpdate = g_localizeStrings.Get(53712);
    newThumbToUpdate = g_localizeStrings.Get(53796);
  }
  else
  {
    CGUIDialogOK2::ShowAndGetInput(53701, 53735);
  }

  return succeeded;
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnRemoveFromQueue(CStdString& newLabelToUpdate, CStdString& newThumbToUpdate)
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnRemoveFromQueue - Enter function. Going to DEQUEUE item (bma)(queue)");

  if (!g_application.IsConnectedToInternet())
  {
    CGUIDialogBoxeeNetworkNotification::ShowAndGetInput(53743,53749);
    return false;
  }

  bool succeeded = BoxeeUtils::Dequeue(&m_item, true);

  if (succeeded)
  {
    //g_application.m_guiDialogKaiToast.QueueNotification(g_localizeStrings.Get(53781), "", g_localizeStrings.Get(53732), 3000);

    if (g_windowManager.GetActiveWindow() == WINDOW_BOXEE_BROWSE_QUEUE)
    {
      m_refreshActiveWindow = true;
    }

    newLabelToUpdate = g_localizeStrings.Get(53711);
    newThumbToUpdate = g_localizeStrings.Get(53791);
  }
  else
  {
    CGUIDialogOK2::ShowAndGetInput(53701, 53736);
  }

  return succeeded;
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnMarkAsSeen(CStdString& newLabelToUpdate, CStdString& newThumbToUpdate)
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnMarkAsSeen - Enter function (bma)");

  bool succeeded = true;

  if (!m_item.HasLinksList())
  {
    BOXEE::Boxee::GetInstance().GetMetadataEngine().MarkAsWatched(m_item.m_strPath, m_item.GetProperty("boxeeid"), 0);
  }
  else
  {
    const CFileItemList* fileLinksList = m_item.GetLinksList();
    for (int i=0; i<fileLinksList->Size(); i++)
    {
      CFileItemPtr linkItem = fileLinksList->Get(i);
      CLinkBoxeeType::LinkBoxeeTypeEnums linkType = CFileItemList::GetLinkBoxeeTypeAsEnum(linkItem->GetProperty("link-boxeetype"));
      if (linkType == CLinkBoxeeType::FEATURE || linkType == CLinkBoxeeType::LOCAL)
      {
        BOXEE::Boxee::GetInstance().GetMetadataEngine().MarkAsWatched(linkItem->m_strPath, m_item.GetProperty("boxeeid"), 0);
      }
    }
  }

  m_item.SetProperty("watched", true);

  newLabelToUpdate = g_localizeStrings.Get(53714);
  newThumbToUpdate = g_localizeStrings.Get(53799);

  g_directoryCache.ClearSubPaths("boxeedb://movies/");
  g_directoryCache.ClearSubPaths("boxee://movies/");
  g_directoryCache.ClearSubPaths("boxeedb://episodes/");
  g_directoryCache.ClearSubPaths("boxee://tvshows/episodes/");

  m_refreshActiveWindow = true;

  return succeeded;
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnMarkAsUnseen(CStdString& newLabelToUpdate, CStdString& newThumbToUpdate)
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnMarkAsUnseen - Enter function (bma)");

  bool succeeded = true;

  if (!m_item.HasLinksList())
  {
    BOXEE::Boxee::GetInstance().GetMetadataEngine().MarkAsUnWatched(m_item.m_strPath, m_item.GetProperty("boxeeid"));
  }
  else
  {
    const CFileItemList* fileLinksList = m_item.GetLinksList();
    for (int i=0; i<fileLinksList->Size(); i++)
    {
      CFileItemPtr linkItem = fileLinksList->Get(i);
      BOXEE::Boxee::GetInstance().GetMetadataEngine().MarkAsUnWatched(linkItem->m_strPath, m_item.GetProperty("boxeeid"));
    }
  }

  m_item.SetProperty("watched", false);

  newLabelToUpdate = g_localizeStrings.Get(53713);
  newThumbToUpdate = g_localizeStrings.Get(53792);

  g_directoryCache.ClearSubPaths("boxeedb://movies/");
  g_directoryCache.ClearSubPaths("boxee://movies/");
  g_directoryCache.ClearSubPaths("boxeedb://episodes/");
  g_directoryCache.ClearSubPaths("boxee://tvshows/episodes/");

  m_refreshActiveWindow = true;

  return succeeded;
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnAddShortcut(CStdString& newLabelToUpdate, CStdString& newThumbToUpdate)
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnAddShortcut - Enter function (bma)");

  CBoxeeShortcut shortcut(m_item);

  bool succeeded = AddShortcut(shortcut);

  if (succeeded)
  {
    newLabelToUpdate = g_localizeStrings.Get(53716);
    newThumbToUpdate = g_localizeStrings.Get(53795);

    /*
    ////////////////////////////////////////////////////////////////////
    // remove the [Add Shortcut - 53715] button from the buttons list //
    ////////////////////////////////////////////////////////////////////

    CGUIBaseContainer* pControl = (CGUIBaseContainer*)GetControl(BUTTONS_LIST);

    if (pControl)
    {
      std::vector< CGUIListItemPtr >& buttonList = pControl->GetItemsByRef();

      for (size_t i=0; i<buttonList.size(); i++)
      {
        CGUIListItemPtr buttonItem = buttonList[i];

        if (buttonItem->GetLabel() == g_localizeStrings.Get(53715))
        {
          SET_CONTROL_FOCUS(LINK_LIST, 1);

          buttonList.erase(buttonList.begin() + i);
        }
      }
    }
    */
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::HandleClickOnAddShortcut - FAILED to add item [label=%s][path=%s] as shortcut (bma)",m_item.GetLabel().c_str(),m_item.m_strPath.c_str());

    // Error dialog is open from BoxeeShortcut::AddShortcut()
    //CGUIDialogOK2::ShowAndGetInput(53701, 53738);
  }

  return succeeded;
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnRemoveFromShortcut(CStdString& newLabelToUpdate, CStdString& newThumbToUpdate)
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnRemoveFromShortcut - Enter function (bma)");

  CBoxeeShortcut shortcut(m_item);

  bool succeeded = RemoveShortcut(shortcut);

  if (succeeded)
  {
    newLabelToUpdate = g_localizeStrings.Get(53715);
    newThumbToUpdate = g_localizeStrings.Get(53793);
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::HandleClickOnRemoveFromShortcut - FAILED to remove item [label=%s][path=%s] as shortcut (bma)",m_item.GetLabel().c_str(),m_item.m_strPath.c_str());
  }

  return succeeded;
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnGotoShow()
{
  CStdString showId = m_item.GetProperty("showid");
  CStdString showLabel = m_item.GetVideoInfoTag()->m_strShowTitle;

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnGotoShow - Enter function. Item ShowId is [%s] (bma)",showId.c_str());

  if (g_application.IsPlayingVideo())
  {
    g_application.StopPlaying();
  }

  CStdString urlStr;
  CStdString encodedProperty = showLabel;
  CUtil::URLEncode(encodedProperty);

  urlStr.Format("boxeeui://tvshows/?seriesId=%s&title=%s",showId,encodedProperty);
  g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_TVEPISODES, urlStr);

  return true;
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnRemoveFromHistory()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnRemoveFromHistory - Enter function. [ItemLabel=%s] (bma)",m_item.GetLabel().c_str());

  bool succeeded = g_application.GetBoxeeItemsHistoryList().RemoveItemFromHistory(m_item);

  if (succeeded)
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnRemoveFromHistory - Successfully removed item [label=%s] from history. Going to refresh history screen (bma)",m_item.GetLabel().c_str());

    CGUIMessage refreshHistoryWinMsg(GUI_MSG_UPDATE, WINDOW_BOXEE_BROWSE_HISTORY, 0);
    g_windowManager.SendThreadMessage(refreshHistoryWinMsg);

    return true;
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::HandleClickOnRemoveFromHistory - FAILED to removed item [label=%s] from history (bma)",m_item.GetLabel().c_str());

    CGUIDialogOK2::ShowAndGetInput(53701, 53740);

    return false;
  }
}
bool CGUIDialogBoxeeMediaAction::HandleClickOnEject()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnEject - Enter function (bma)");

  bool succeeded = true;

#ifdef _WIN32
    CWIN32Util::ToggleTray() ;
#elif defined(_LINUX) && !defined(__APPLE__)
    system("eject");
#else
    CIoSupport::ToggleTray();
#endif

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnEject - Exit function and return [succeeded=%d] (bma)",succeeded);

  return succeeded;
}

#ifdef HAVE_LIBBLURAY
struct VectorSortByDurationFunctor {
    bool operator()(const DemuxTitleInfo &a, const DemuxTitleInfo &b) const {
        return a.duration > b.duration;
    }
};
#endif

bool CGUIDialogBoxeeMediaAction::HandleClickOnBrowse()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnBrowse - Enter function (bma)");

  bool succeeded = true;
  CStdString strPath = m_item.m_strPath;

  if(strPath.empty())
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), ADDITIONAL_LINKS_LIST);
    g_windowManager.SendMessage(msg);
    int itemIndex = msg.GetParam1();

    if ((itemIndex < 0) || (itemIndex >= (int)m_linksFileItemList.size()))
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::HandleClickOnBrowse - Index of item clicked foo browsw is [%d] while [LinksFileItemListSize=%zu] (bma)",itemIndex,m_linksFileItemList.size());
      return false;
    }

    strPath = m_linksFileItemList[itemIndex]->m_strPath;
  }
  
#ifdef HAVE_LIBBLURAY
  if( m_item.GetPropertyBOOL("isBlurayFolder") || m_item.IsType(".iso") )
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnBrowse - Running bluray title selection " );

    /////// LOAD PROGRESS
    // pulling title info through libbluray over a network interface may take a bit.
    // unfortunately we are doing this on the main thread...
    CGUIDialogProgress* progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      progress->StartModal();
      progress->Progress();
    }

    
    CDVDInputStreamBluray bluray;
    if( !bluray.Open( strPath + "/BDMV/index.bdmv", "bluray/bluray" ) ) // content type is ignored
    {
      CLog::Log(LOGERROR, "CGUIDialogBoxeeMediaAction::HandleClickOnBrowse - failed to open br path [%s]", strPath.c_str());
      return false;
    }
    
    if( progress )  progress->Progress();

    std::vector<DemuxTitleInfo> titles;
    int title_count = bluray.GetTitleCount();
    int selectedTitle=1;

    CLog::Log(LOGDEBUG, "CGUiDialogBoxeeMediaAction::HandleClickOnBrowse - found %d titles", title_count);
    
    for( int i = 1; i <= title_count; i++ )
    {
      DemuxTitleInfo info;
      bluray.GetTitleInfo(i, info.duration, info.title);
      info.id = i;
      info.duration /= 1000;
      
      if( info.duration > 60 )
      {
        titles.push_back(info);
      }
	  }

	  // Now sort, based on duration:
	  std::sort(titles.begin(), titles.end(), VectorSortByDurationFunctor());

    //////// UNLOAD PROGRESS
    if( progress )
    {
      progress->Close();
    }
    
    bool selection = false;
    if( title_count > 0 )
    {
      selection = CGUIDialogBoxeeChapters::Show(titles, selectedTitle, !m_item.IsType(".iso"));
      if( selection && selectedTitle != -1 )
      {
        // trigger playback with the selected title
        CFileItem item(m_item);
        item.m_strPath = strPath;
        CLog::Log(LOGDEBUG, "CGUiDialogBoxeeMediaAction::HandleClickOnBrowse - playing with selected title == %d\n", selectedTitle);
        item.SetProperty("BlurayStartingTitle", selectedTitle);
        item.ClearLinksList(); //clear the link list if its a bluray playable folder, so we don't show the quality selection dialog
        return OnPlay(item);
      }
      else if( !selection )
        return false;
    }
  }
  // if we get here, the user selected the browse option (selectedTitle == -1)
  
#endif

  CStdString pathEncoded = strPath;
  CUtil::URLEncode(pathEncoded);

  CStdString pathToBrowse = "boxeeui://files/?path=";
  pathToBrowse += pathEncoded;

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnBrowse - Call ActivateWindow with [pathToBrowse=%s] (bma)",pathToBrowse.c_str());

  g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_LOCAL, pathToBrowse);

  g_windowManager.CloseDialogs(true);

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnBrowse - Exit function (bma)");

  return succeeded;
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnResolve()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnResolve - Enter function (bma)");
  bool succeeded = true;

  m_item.Dump();

  CStdString strPath = m_item.m_strPath;
  CFileItemPtr unresolvedItem(new CFileItem(m_item));

  if (strPath.IsEmpty())
  {
      // Go over all links of the item and find only local ones
    const CFileItemList* linksFileItemList = m_item.GetLinksList();

    std::vector<CFileItemPtr> localPathsList;

    for (int i=0; i<linksFileItemList->Size(); i++)
    {
      CFileItemPtr linkFileItemToAdd = linksFileItemList->Get(i);

      CLinkBoxeeType::LinkBoxeeTypeEnums linkBoxeeTypeEnum = CFileItem::GetLinkBoxeeTypeAsEnum(linkFileItemToAdd->GetProperty("link-boxeetype"));

      if (linkBoxeeTypeEnum == CLinkBoxeeType::LOCAL)
      {
        localPathsList.push_back(linkFileItemToAdd);
      }
    }

    if ((int)localPathsList.size() > 1)
    {
      // allow the user to choose which path
      CGUIDialogSelect *pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);

      pDlgSelect->SetHeading(396); //"Select Location"
      pDlgSelect->Reset();

      for (int i = 0; i < (int)localPathsList.size(); i++)
      {
        CStdString strPathNoPassword(localPathsList[i]->m_strPath);
        CUtil::RemovePasswordFromPath(strPathNoPassword);
        pDlgSelect->Add(strPathNoPassword);
      }

      pDlgSelect->EnableButton(TRUE);
      pDlgSelect->SetButtonLabel(222); //'Cancel' button returns to weather settings
      pDlgSelect->DoModal();

      int selectedIndex = pDlgSelect->GetSelectedLabel();
      if (selectedIndex >= 0)
      {
        strPath = localPathsList[selectedIndex]->m_strPath;
      }
      else
      {
        return false;
      }
    }
    else
    {
      strPath = localPathsList[0]->m_strPath;
    }
  }

  unresolvedItem->m_strPath = strPath;

  if ((succeeded = CGUIDialogBoxeeManualResolve::Show(unresolvedItem)))
  {
    CGUIMessage winmsg(GUI_MSG_UPDATE, g_windowManager.GetActiveWindow(), 0);
    g_windowManager.SendThreadMessage(winmsg);
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnResolve - Exit function and return [succeeded=%d] (bma)",succeeded);

  return succeeded;
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnAddToFavorite(CStdString& newLabelToUpdate, CStdString& newThumbToUpdate)
{
  bool succeeded = false;

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnAddToFavorite - Enter function. (bma)");

  if (m_item.GetProperty("showid").IsEmpty() || !m_item.GetVideoInfoTag() || m_item.GetVideoInfoTag()->m_strShowTitle.IsEmpty())
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::HandleClickOnAddToFavorite - FAILED adding [showid=%s] to favorite (bma)",m_item.GetProperty("showid").c_str());
    return succeeded;
  }

  SubscribeJob* job = new SubscribeJob(CSubscriptionType::TVSHOW_SUBSCRIPTION, m_item.GetProperty("showid"), m_item.GetVideoInfoTag()->m_strShowTitle, true);

  succeeded = (CUtil::RunInBG(job) == JOB_SUCCEEDED);

  if (succeeded)
  {
    int activeWindow = g_windowManager.GetActiveWindow();
    if (activeWindow == WINDOW_BOXEE_BROWSE_TVEPISODES)
    {
      CGUIMessage refreshTvEpisodesWinMsg(GUI_MSG_UPDATE_ITEM, WINDOW_BOXEE_BROWSE_TVEPISODES,0);
      refreshTvEpisodesWinMsg.SetStringParam("favorite update");
      g_windowManager.SendThreadMessage(refreshTvEpisodesWinMsg);
   }

    newLabelToUpdate = g_localizeStrings.Get(53730);
    newThumbToUpdate = g_localizeStrings.Get(53785);
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnAddToFavorite - Exit function. [succeeded=%d] (bma)",succeeded);

  return succeeded;
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnRemoveFromFavorite(CStdString& newLabelToUpdate, CStdString& newThumbToUpdate)
{
  bool succeeded = false;

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnRemoveFromFavorite - Enter function. (bma)");

  if (m_item.GetProperty("showid").IsEmpty() || !m_item.GetVideoInfoTag() || m_item.GetVideoInfoTag()->m_strShowTitle.IsEmpty())
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::HandleClickOnRemoveFromFavorite - FAILED removing [showid=%s] from favorite (bma)",m_item.GetProperty("showid").c_str());
    return succeeded;
  }

  SubscribeJob* job = new SubscribeJob(CSubscriptionType::TVSHOW_SUBSCRIPTION, m_item.GetProperty("showid"), m_item.GetVideoInfoTag()->m_strShowTitle, false);

  succeeded = (CUtil::RunInBG(job) == JOB_SUCCEEDED);

  if (succeeded)
  {
    int activeWindow = g_windowManager.GetActiveWindow();
    if (activeWindow == WINDOW_BOXEE_BROWSE_TVEPISODES)
    {
      CGUIMessage refreshTvEpisodesWinMsg(GUI_MSG_UPDATE_ITEM, WINDOW_BOXEE_BROWSE_TVEPISODES,0);
      refreshTvEpisodesWinMsg.SetStringParam("favorite update");
      g_windowManager.SendThreadMessage(refreshTvEpisodesWinMsg);
    }

    newLabelToUpdate = g_localizeStrings.Get(53729);
    newThumbToUpdate = g_localizeStrings.Get(53784);
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnRemoveFromFavorite - Exit function. [succeeded=%d] (bma)",succeeded);

  return succeeded;
}

int CGUIDialogBoxeeMediaAction::FillDialogButtons(bool itemHasLinks)
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), BUTTONS_LIST_MOVIE, 0);
  OnMessage(msg);

  CGUIMessage msg1(GUI_MSG_LABEL_RESET, GetID(), BUTTONS_LIST_NOT_MOVIE, 0);
  OnMessage(msg1);

  //CGUIMessage msg2(GUI_MSG_LABEL_RESET, GetID(), TRAILER_LIST, 0);
  //OnMessage(msg2);

  //m_item.Dump();

  CFileItemList buttonsList;

  /////////////////////
  // Trailer button //
  /////////////////////

  /*
  if (m_trailerLinkItem.get() && !m_tarilerWasAddedToLinkList && !m_item.GetPropertyBOOL("IsTrailer"))
  {
    CFileItemPtr trailerButton(new CFileItem(g_localizeStrings.Get(53718)));
    trailerButton->SetThumbnailImage(g_localizeStrings.Get(53794));
    trailerButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_PLAY_TRAILER);
	  buttonsList.Add(trailerButton);

    //CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), TRAILER_LIST, 0, 0, trailerButton);
    //OnMessage(winmsg);

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Add TRAILER button. (bma)");
  }
  */

  /////////////////////
  // ReadMore button //
  /////////////////////

  /*
  if (BoxeeUtils::HasDescription(m_item))
  {
    CFileItemPtr moreInfoButton(new CFileItem(g_localizeStrings.Get(53710)));
    moreInfoButton->SetThumbnailImage(g_localizeStrings.Get(53790));
    moreInfoButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_MORE_INFO);
    buttonsList.Add(moreInfoButton);

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Add MORE_INFO button. [ButtonsListSize=%d] (bma)",buttonsList.Size());
  }
  */

  //////////////////////////
  // Queue/Dequeue button //
  //////////////////////////

  int activeWindow = g_windowManager.GetActiveWindow();

  // DVD Media Action Dialog doesn't need Queue/Dequeue button
  if (!m_item.IsDVD())
  {
    CGUIWindowBoxeeMain* pHomeWindow = (CGUIWindowBoxeeMain*)g_windowManager.GetWindow(WINDOW_HOME);
    if (!pHomeWindow)
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::FillDialogButtons - FAILED to get WINDOW_HOME. item [label=%s][path=%s][boxeeId=%s] (bma)",m_item.GetLabel().c_str(),m_item.m_strPath.c_str(),m_item.GetProperty("boxeeid").c_str());
    }

    CStdString referral;

    bool canQueue = BoxeeUtils::CanQueue(m_item,referral);
    bool isInQueue = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().IsInQueue(m_item.GetProperty("boxeeid"),m_item.m_strPath,referral);

    if ((activeWindow == WINDOW_BOXEE_BROWSE_QUEUE))
    {
      /////////////////////////////////////////////////////
      // in QueueWindow -> need to add REMOVE_FROM_QUEUE //
      /////////////////////////////////////////////////////

      if (canQueue)
      {
        CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::FillDialogButtons - In QueueWindow and [canQueue=%d] for item [label=%s][path=%s][boxeeId=%s] (bma)",canQueue,m_item.GetLabel().c_str(),m_item.m_strPath.c_str(),m_item.GetProperty("boxeeid").c_str());
      }

      AddDequeueButton(buttonsList, referral);

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - In QueueScreen -> Add REMOVE_FROM_QUEUE button and set property [referral=%s]. [ButtonsListSize=%d] (bma)",m_item.GetProperty("referral").c_str(),buttonsList.Size());
    }
    else if (itemHasLinks)
    {
      if (isInQueue)
      {
        AddDequeueButton(buttonsList, referral);
        CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Add REMOVE_FROM_QUEUE button. [ButtonsListSize=%d] (bma)",buttonsList.Size());
      }
      else
      {
        if (canQueue)
        {
          CFileItemPtr queueButton(new CFileItem(g_localizeStrings.Get(53711)));
          queueButton->SetThumbnailImage(g_localizeStrings.Get(53791));
          queueButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_ADD_TO_QUEUE);
          buttonsList.Add(queueButton);

          CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Add ADD_TO_QUEUE button. [ButtonsListSize=%d] (bma)",buttonsList.Size());
        }
        else
        {
          CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Not adding queue/dequeue button. [activeWindow=%d][canQueue=%d][isInQueue=%d][referral=%s]. [itemHasLinks=%d][label=%s][path=%s][boxeeId=%s] (bma)",activeWindow,canQueue,isInQueue,referral.c_str(),itemHasLinks,m_item.GetLabel().c_str(),m_item.m_strPath.c_str(),m_item.GetProperty("boxeeid").c_str());
        }
      }
    }
    else
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Not adding queue/dequeue button. [activeWindow=%d][canQueue=%d][isInQueue=%d][referral=%s]. [itemHasLinks=%d] for item [label=%s][path=%s][boxeeId=%s] (bma)",activeWindow,canQueue,isInQueue,referral.c_str(),itemHasLinks,m_item.GetLabel().c_str(),m_item.m_strPath.c_str(),m_item.GetProperty("boxeeid").c_str());
    }
  }

  ////////////////////////
  // Seen/Unseen button //
  ////////////////////////

  if (itemHasLinks && !m_item.GetPropertyBOOL("IsTrailer"))
  {
    CFileMarkOptions::FileMarkOptionsEnums fileMarkStatus = GetFileMarkStatus(m_item);
    if (fileMarkStatus == CFileMarkOptions::MARKED_AS_SEEN)
    {
      // file is marked as SEEN -> Add button for MarkAsUnseen

      CFileItemPtr markAsUnseenButton(new CFileItem(g_localizeStrings.Get(53714)));
      markAsUnseenButton->SetThumbnailImage(g_localizeStrings.Get(53799));
      markAsUnseenButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_MARK_AS_UNSEEN);
      buttonsList.Add(markAsUnseenButton);

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Add MARK_AS_UNSEEN button. [ButtonsListSize=%d] (bma)",buttonsList.Size());
    }
    else if (fileMarkStatus == CFileMarkOptions::MARKED_AS_UNSEEN)
    {
      // file is marked as UNSEEN -> Add button for MarkAsSeen

      CFileItemPtr markAsSeenButton(new CFileItem(g_localizeStrings.Get(53713)));
      markAsSeenButton->SetThumbnailImage(g_localizeStrings.Get(53792));
      markAsSeenButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_MARK_AS_SEEN);
      buttonsList.Add(markAsSeenButton);

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Add MARK_AS_SEEN button. [ButtonsListSize=%d] (bma)",buttonsList.Size());
    }
  }

  /*
  /////////////////////
  // Shortcut button //
  /////////////////////

  if (itemHasLinks && !m_item.IsDVD())
  {
    if ((g_windowManager.GetActiveWindow() != WINDOW_BOXEE_BROWSE_SHORTCUTS))
    {
      if (CanAddAsShortcut(m_item))
      {
        CFileItemPtr shortcutButton(new CFileItem(g_localizeStrings.Get(53715)));
        shortcutButton->SetThumbnailImage(g_localizeStrings.Get(53793));
        shortcutButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_ADD_AS_SHORTCUT);
        buttonsList.Add(shortcutButton);

        CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Add BTN_ADD_SHORTCUT button. [ButtonsListSize=%d] (bma)",buttonsList.Size());
      }
      else
      {
        CFileItemPtr shortcutButton(new CFileItem(g_localizeStrings.Get(53716)));
        shortcutButton->SetThumbnailImage(g_localizeStrings.Get(53795));
        shortcutButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_REMOVE_FROM_SHORTCUT);
        buttonsList.Add(shortcutButton);

        CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Add BTN_REMOVE_FROM_SHORTCUT button. [ButtonsListSize=%d] (bma)",buttonsList.Size());
      }
    }
  }
  */

  //////////////////
  // Share screen //
  //////////////////

  if (BoxeeUtils::CanShare(m_item))
  {
    CFileItemPtr shareButton(new CFileItem(g_localizeStrings.Get(53719)));
    shareButton->SetThumbnailImage(g_localizeStrings.Get(53798));
    shareButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_SAHRE_ITEM);
    buttonsList.Add(shareButton);

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Add BTN_SHARE button. [ButtonsListSize=%d] (bma)",buttonsList.Size());
  }

  /////////////////////////
  /// DVD Dialog Buttons //
  /////////////////////////

  if ( BoxeeUtils::CanEject(m_item))
  {
	    CFileItemPtr ejectButton(new CFileItem(g_localizeStrings.Get(53721)));
	    ejectButton->SetThumbnailImage(g_localizeStrings.Get(53786));
	    ejectButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_EJECT);
	    buttonsList.Add(ejectButton);
	    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Add BTN_EJECT button. [ButtonsListSize=%d] (bma)",buttonsList.Size());

  }

  // support for browse in br iso for title lookup
  bool bBlurayISO = false;

#ifdef HAVE_LIBBLURAY
  if( m_item.IsType(".iso") )
  {
    CStdString type;
    
    // this op can be expensive over a slow network connection
    if( CUtil::GetIsoDiskType( m_item.m_strPath, type) && type == "BD")
    {
      // should probably store as a property of the fileitem/persist this
      bBlurayISO = true;
    }
  }
#endif
  
  if ( m_item.IsDVD() || m_item.IsPlayableFolder() || bBlurayISO )
  {
	    CFileItemPtr browseButton(new CFileItem(g_localizeStrings.Get(53722)));
	    browseButton->SetThumbnailImage(g_localizeStrings.Get(53787));
	    browseButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_BROWSE);
	    buttonsList.Add(browseButton);
	    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Add BTN_BROWSE button. [ButtonsListSize=%d] (bma)",buttonsList.Size());
  }

  /*
  /////////////////////
  // Favorite button //
  /////////////////////

  CStdString showId = m_item.GetProperty("showid");

  if (!showId.IsEmpty())
  {
    bool isSubscribe = BoxeeUtils::IsSubscribe(showId);
    if (isSubscribe)
    {
      // already subscribe -> Add button for unSubscribe

      CFileItemPtr unFavoriteButton(new CFileItem(g_localizeStrings.Get(53730)));
      unFavoriteButton->SetThumbnailImage(g_localizeStrings.Get(53785));
      unFavoriteButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_REMOVE_FROM_FAVORITE);
      buttonsList.Add(unFavoriteButton);

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Add ACTION_REMOVE_FAVORITE button. [ButtonsListSize=%d] (bma)",buttonsList.Size());
    }
    else
    {
      // not subscribe -> Add button for subscribe

      CFileItemPtr favoriteButton(new CFileItem(g_localizeStrings.Get(53729)));
      favoriteButton->SetThumbnailImage(g_localizeStrings.Get(53784));
      favoriteButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_ADD_TO_FAVORITE);
      buttonsList.Add(favoriteButton);

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Add ACTION_ADD_FAVORITE button. [ButtonsListSize=%d] (bma)",buttonsList.Size());
    }
  }
  */

  //////////////////////
  // Goto show screen //
  //////////////////////

  if (CanGotoShowScreen(m_item))
  {
    CFileItemPtr gotoShowButton(new CFileItem(g_localizeStrings.Get(53717)));
    gotoShowButton->SetThumbnailImage(g_localizeStrings.Get(53797));
    gotoShowButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_GOTO_SHOW);
    buttonsList.Add(gotoShowButton);

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Add BTN_GOTO_SHOW button. [ButtonsListSize=%d] (bma)",buttonsList.Size());
  }

  /*
  /////////////////////////
  // remove from history //
  /////////////////////////

  if (activeWindow == WINDOW_BOXEE_BROWSE_HISTORY)
  {
    CFileItemPtr removeFromButton(new CFileItem(g_localizeStrings.Get(53720)));
    removeFromButton->SetThumbnailImage(g_localizeStrings.Get(53789));
    removeFromButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_REMOVE_FROM_HISTORY);
    buttonsList.Add(removeFromButton);

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Add BTN_GOTO_SHOW button. [ButtonsListSize=%d] (bma)",buttonsList.Size());
  }
  */

  //////////////////////
  // manually resolve //
  //////////////////////

  if ((activeWindow != WINDOW_BOXEE_BROWSE_HISTORY) && BoxeeUtils::CanRecognize(m_item))
  {
    CStdString strLabel;
    CStdString strThumb;

    if (m_item.GetPropertyBOOL("isFolderItem"))
    {
      strLabel = g_localizeStrings.Get(52126);
      strThumb = g_localizeStrings.Get(53788);
    }
    else
    {
      strLabel = g_localizeStrings.Get(52130);
      strThumb = g_localizeStrings.Get(53779);
    }

    CFileItemPtr resolveButton(new CFileItem(strLabel));
    resolveButton->SetThumbnailImage(strThumb);
    resolveButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_RESOLVE);
    buttonsList.Add(resolveButton);
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Add BTN_RESOLVE button. [ButtonsListSize=%d] (bma)",buttonsList.Size());
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Going to add [ButtonsListSize=%d] to dialog (bma)",buttonsList.Size());

  for (int i=0; i< buttonsList.Size(); i++)
  {
    CFileItemPtr item = buttonsList.Get(i);

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - [%d/%d] - Going to add button [label=%s] to ButtonsListSize (bma)",i+1,buttonsList.Size(),item->GetLabel().c_str());

    CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), BUTTONS_LIST_MOVIE, 0, 0, item);
    OnMessage(winmsg);

    CGUIMessage winmsg1(GUI_MSG_LABEL_ADD, GetID(), BUTTONS_LIST_NOT_MOVIE, 0, 0, item);
    OnMessage(winmsg1);
  }

  return buttonsList.Size();
}

int CGUIDialogBoxeeMediaAction::FillMovieAdditionalDialogButtons()
{
  CFileItemList buttonsList;

  const CGUIControl* list = GetControl(ADDITIONAL_MOVIE_BUTTONS_LIST);

  if (!list || !list->IsVisible())
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillMovieAdditionalDialogButtons - NOT adding movie additional buttons since [list=%p][listIsVisible=%d] (bma)",list,(list ? list->IsVisible() : false));
    return buttonsList.Size();
  }

  CGUIMessage msg2(GUI_MSG_LABEL_RESET, GetID(), ADDITIONAL_MOVIE_BUTTONS_LIST, 0);
  OnMessage(msg2);

  CFileItemPtr overviewButton(new CFileItem(g_localizeStrings.Get(90452)));
  overviewButton->Select(true);
  buttonsList.Add(overviewButton);

  CFileItemPtr castAndCrewButton(new CFileItem(g_localizeStrings.Get(90453)));
  buttonsList.Add(castAndCrewButton);

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), ADDITIONAL_MOVIE_BUTTONS_LIST, 0, 0, &buttonsList);
  OnMessage(msg);

  CONTROL_SELECT_ITEM(ADDITIONAL_MOVIE_BUTTONS_LIST,0);

  return buttonsList.Size();
}

void CGUIDialogBoxeeMediaAction::AddDequeueButton(CFileItemList& buttonsList, const CStdString& referral)
{
  CStdString orgItemReferral = m_item.GetProperty("referral");

  if (!referral.IsEmpty() && (orgItemReferral != referral))
  {
    m_item.SetProperty("referral",referral);
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::AddDequeueButton - After setting item property [referral=%s]. Original referral was [orgItemReferral=%s] (bma)",m_item.GetProperty("referral").c_str(),orgItemReferral.c_str());
  }

  CFileItemPtr dequeueButton(new CFileItem(g_localizeStrings.Get(53712)));
  dequeueButton->SetThumbnailImage(g_localizeStrings.Get(53796));
  dequeueButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_REMOVE_FROM_QUEUE);
  buttonsList.Add(dequeueButton);
}

//////////////////////////////

void CGUIDialogBoxeeMediaAction::OnDeinitWindow(int nextWindowID)
{
  if (m_refreshActiveWindow)
  {
    CGUIMessage winmsg(GUI_MSG_UPDATE, g_windowManager.GetActiveWindow(), 0);
    g_windowManager.SendThreadMessage(winmsg);
    m_refreshActiveWindow = false;
  }

  CGUIMessage winmsg1(GUI_MSG_LABEL_RESET, GetID(), 5000);
  g_windowManager.SendMessage(winmsg1);

  // reset the window property "link-path-to-show"
  SetLinkPathAsWindowProperty("");

  m_trailerLinkItem.reset();
}

CStdString CGUIDialogBoxeeMediaAction::GetLabel(const CFileItem& item)
{
  //  if (item.GetPropertyBOOL("isfeeditem"))
  //  {
  //    if (item.HasProperty("title") && !item.GetProperty("title").Empty())
  //    {
  //      return item.GetProperty("title");
  //    }
  //    else
  //    {
  //      return item.GetLabel();
  //    }
  //  }

  if (item.GetPropertyBOOL("isinternetstream"))
    return item.GetLabel();

  if ((item.GetPropertyBOOL("isvideo") || (item.GetProperty("isvideo") == "true")) && item.HasVideoInfoTag()) 
  {
    const CVideoInfoTag* pInfoTag = item.GetVideoInfoTag();
    return pInfoTag->m_strTitle;
  }

  if(item.GetPropertyBOOL("isalbum") && item.HasMusicInfoTag()) 
  {
    const CMusicInfoTag* pInfoTag = item.GetMusicInfoTag();
    CStdString strAlbumLabel = pInfoTag->GetArtist() + " " + pInfoTag->GetAlbum();
    return strAlbumLabel;
  }

  return m_item.GetLabel();
}

CFileMarkOptions::FileMarkOptionsEnums CGUIDialogBoxeeMediaAction::GetFileMarkStatus(const CFileItem& item)
{
  if (BoxeeUtils::CanMarkWatched(item))
  {
    if (item.GetPropertyBOOL("watched"))
    {
      return CFileMarkOptions::MARKED_AS_SEEN;
    }
    else
    {
      return CFileMarkOptions::MARKED_AS_UNSEEN;
    }
  }
  else
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::GetFileMarkStatus - Going to return CANNOT_BE_MARKED for item [label=%s]. [HasVideoInfoTag=%d][HasProperty(watched)=%d] (bma)",item.GetLabel().c_str(),item.HasVideoInfoTag(),item.HasProperty("watched"));
    return CFileMarkOptions::CANNOT_BE_MARKED;
  }
}

bool CGUIDialogBoxeeMediaAction::CanAddAsShortcut(const CFileItem& item)
{
  CBoxeeShortcut shortcut(item);

  if (g_settings.GetShortcuts().IsInShortcut(shortcut))
  {
    return false;
  }
  else
  {
    return true;
  }
}

bool CGUIDialogBoxeeMediaAction::CanGotoShowScreen(const CFileItem& item)
{
  if ((g_windowManager.GetActiveWindow() != WINDOW_BOXEE_BROWSE_TVEPISODES) && (item.HasProperty("showid")) && (!item.GetProperty("showid").IsEmpty()))
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool CGUIDialogBoxeeMediaAction::ShowAndGetInput(const CFileItem* pItem)
{
  CGUIDialogBoxeeMediaAction *dialog = (CGUIDialogBoxeeMediaAction *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_MEDIA_ACTION);
  if (!pItem || !dialog)
  {
    return false;
  }

  CStdString itemId = pItem->GetProperty("itemid");

  g_application.GetItemLoader().AddItemLoadedObserver(itemId,dialog->GetID());

  dialog->m_item = *pItem;

  dialog->DoModal();

  g_application.GetItemLoader().RemoveItemLoadedObserver(itemId,dialog->GetID());

  return dialog->m_bConfirmed;
}

bool CGUIDialogBoxeeMediaAction::OnBrowse(const CFileItem& item)
{
  if (item.m_bIsFolder)
  {
    AddItemToHistory(item);

    CStdString path;
    CStdString encodedProperty= item.m_strPath;
    CUtil::URLEncode(encodedProperty);
    path = "boxeeui://files/?path=";
    path += encodedProperty;

    if (item.GetPropertyBOOL("ispicturefolder") || item.HasPictureInfoTag())
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnBrowse - Going to open WINDOW_BOXEE_BROWSE_PHOTOS with [path=%s] (bma)(browse)",path.c_str());
      g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_PHOTOS, path);
    }
    else if (item.GetPropertyBOOL("IsAlbum") && !item.GetProperty("BoxeeDBAlbumId").IsEmpty())
    {
      CStdString albumId = item.GetProperty("BoxeeDBAlbumId");
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnBrowse - Going to open WINDOW_BOXEE_BROWSE_TRACKS with [albumId=%s] (bma)(browse)",albumId.c_str());
      g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_TRACKS, albumId);
    }
    else if (item.GetPropertyBOOL("isMusicFolder") || item.HasMusicInfoTag())
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnBrowse - Going to open WINDOW_BOXEE_BROWSE_LOCAL with [path=%s]. [isMusicFolder=%d][HasMusicInfoTag=%d] (bma)(browse)",path.c_str(),item.GetPropertyBOOL("isMusicFolder"),item.HasMusicInfoTag());
      g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_LOCAL, path);
    }
    else
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnBrowse - Going to open WINDOW_BOXEE_BROWSE_LOCAL with [path=%s] (bma)(browse)",path.c_str());
      g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_LOCAL, path);
    }
  }
  else
  {
    CLog::Log(LOGWARNING,"CGUIDialogBoxeeMediaAction::OnBrowse - Could not find a match option for item [label=%s][path=%s][IsFolder=%d][ishistory=%d] (bma)",item.GetLabel().c_str(),item.m_strPath.c_str(),item.m_bIsFolder,item.GetPropertyBOOL("ishistory"));
  }

  return true;

  /*
  if (item.GetPropertyBOOL("isdvd"))
  {
    CGUIWindowBoxeeBrowse::Show(item.m_strPath, "dvd", item.GetLabel());
  }
  if (item.GetPropertyBOOL("isDvdFolder") && item.m_bIsFolder)
  {
    CGUIWindowBoxeeBrowse::Show(item.m_strPath, "video", item.m_strPath);
  }
  if (item.GetPropertyBOOL("istvshowfolder") && item.m_bIsFolder)
  {
    // If case of a TV series
    CGUIWindowBoxeeBrowse::Show(item.m_strPath, "video", item.GetLabel());
  }
  else if (item.GetPropertyBOOL("ishistory") && item.HasProperty("parentfolder")) 
  {
    CStdString strType;
	  if (item.IsVideo()) 
	  {
      strType = "video";
	  }
	  else if (item.IsAudio()) 
	  {
      strType = "music";
	  }
	  else
	  {
      strType = "pictures";
	  }
    
    CStdString strLabel;
    if (item.HasProperty("rsschanneltitle"))
    {
      strLabel = item.GetProperty("rsschanneltitle");
    }
    else
    {
      strLabel = item.GetProperty("parentfolder");
    }      
    CGUIWindowBoxeeBrowse::Show(item.GetProperty("parentfolder"), strType, strLabel);
  }
  else if (item.GetPropertyBOOL("ishistory") && item.m_bIsFolder && item.GetPropertyBOOL("ispicturefolder")) 
  {
    CGUIWindowBoxeeBrowse::Show(item.m_strPath, "pictures", item.m_strPath);
  }
  else if (item.GetPropertyBOOL("ishistory") && item.m_bIsFolder && item.GetPropertyBOOL("ismusicfolder")) 
  {
    CGUIWindowBoxeeBrowse::Show(item.m_strPath, "music", item.m_strPath);
  }
  else if (item.GetPropertyBOOL("isGroup") && item.m_bIsFolder) 
  {
    CStdString groupType = item.GetProperty("GroupType");

    CStdString strType;

    if(groupType.Equals("video"))
    {
      strType = "video";
    }
    else if(groupType.Equals("music"))
    {
      strType = "music";
    }
    else
    {
      strType = "other";
    }

    // Check if current window is already a browse window
    if (g_windowManager.GetActiveWindow() == WINDOW_BOXEE_BROWSE)
      CGUIWindowBoxeeBrowse::Show(item.m_strPath, strType, item.GetLabel(), "", false);
    else
      CGUIWindowBoxeeBrowse::Show(item.m_strPath, strType, item.GetLabel());
  }
  else if (item.m_bIsFolder)
  {
    CGUIWindowBoxeeBrowse*pWindow = (CGUIWindowBoxeeBrowse*)g_windowManager.GetWindow(WINDOW_BOXEE_BROWSE);
    CGUIWindowBoxeeBrowse::Show(item.m_strPath, pWindow->GetWindowStateType(), item.GetLabel());
  }
  else
  {
    CLog::Log(LOGWARNING,"CGUIDialogBoxeeMediaAction::OnMessage - Could not find a match option for item [%s] in BTN_BROWSE",(item.m_strPath).c_str());
  }

  return true;
  */
}

bool CGUIDialogBoxeeMediaAction::OnPlay(const CFileItem& item)
{ 
  if (item.IsAudio() && item.HasProperty("isFolderItem"))
  {
    //CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnPlay, playing audio folder, path = %s", item.m_strPath.c_str());
    OnPlayAudioFromFolder(item);
  }
  else if (item.GetPropertyBOOL("isdvd"))
  {
#ifdef HAS_DVD_DRIVE
    if (IsDVDPlaying())
      g_application.StopPlaying();

    // Always start playing
    CAutorun::PlayDisc();
#endif
    g_windowManager.CloseDialogs(true);
  }
  else if (item.GetPropertyBOOL("IsAlbum") && item.HasMusicInfoTag())
  {
    AddItemToHistory(item);

    g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_TRACKS, item.GetProperty("BoxeeDBAlbumId"));
    return true;
  }
  else if (item.IsPicture()) 
  {
    AddItemToHistory(item);

    CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    if (!pSlideShow)
      return false;

    if (g_application.IsPlayingVideo())
      g_application.StopPlaying();

    pSlideShow->Reset();
    pSlideShow->Add(&item);
    pSlideShow->PauseSlideShow();
    g_windowManager.ActivateWindow(WINDOW_SLIDESHOW);
    return true;
  }
  else if (item.IsPlugin()) 
  {
    AddItemToHistory(item);

    return DIRECTORY::CPluginDirectory::RunScriptWithParams(item.m_strPath);
  }
#ifdef HAS_LASTFM
  else if (item.IsLastFM()) 
  {
    AddItemToHistory(item);

    CLastFmManager::GetInstance()->StopRadio();
    CLastFmManager::GetInstance()->ChangeStation(CURI(item.m_strPath));
  }
#endif
  else if ((item.m_bIsFolder) && (item.GetPropertyBOOL("isDvdFolder")))
  {
    CLog::Log(LOGDEBUG,"In CGUIDialogBoxeeMediaAction::OnPlay - Going to build a playlist for dvd folder [item=%s] and then play it (dvdfolder)",(item.m_strPath).c_str());

    AddItemToHistory(item);

    CreatePlaylistAndPlay(item,CPlaylistSourceType::DVD);
  }
  else if (item.IsStack())
  {
    CLog::Log(LOGDEBUG,"In CGUIDialogBoxeeMediaAction::OnPlay - Going to build a playlist for stack item [item=%s] and then play it (stack)",(item.m_strPath).c_str());

    AddItemToHistory(item);

    //CreatePlaylistAndPlay(item,CPlaylistSourceType::STACK);
	  OnPlayMedia(item);
  }
  else
  {
    if(item.GetPropertyBOOL("istrailer") == true)
    {
      // If item is trailer -> Need to add (trailer) to its label
      
      CFileItemPtr trailerItem(new CFileItem(item));
      
      AddItemToHistory(*((CFileItem*)trailerItem.get()));

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnPlay - Going to play trailer file from folder mode. [path=%s][label=%s][thumb=%s] (tr)", (trailerItem->m_strPath).c_str(),(trailerItem->GetLabel()).c_str(),(trailerItem->GetThumbnailImage()).c_str());

      return OnPlayMedia(*((CFileItem*)trailerItem.get()));
    }
    else
    {
      AddItemToHistory(item);

      CLog::Log(LOGDEBUG,"In CGUIDialogBoxeeMediaAction::OnPlay - Going to play file from folder mode, path = %s", item.m_strPath.c_str());

      return OnPlayMedia(item);
    }
  }

  return true;
}

bool CGUIDialogBoxeeMediaAction::OnResume(const CFileItem& item)
{
  bool resumeItem = g_guiSettings.GetInt("myvideos.resumeautomatically") != RESUME_ASK;

  if ((item.IsPlayableFolder() || !item.m_bIsFolder) && !resumeItem)
  {
    // check to see whether we have a resume offset available
    BXUserProfileDatabase db;
    if (db.IsOpen())
    {
      double timeInSeconds;
      CStdString itemPath(item.m_strPath);

      if ( db.GetTimeWatchedByPath(itemPath , timeInSeconds) )
      {
        // prompt user whether they wish to resume
        std::vector<CStdString> choices;
        CStdString resumeString, time;
        StringUtils::SecondsToTimeString(lrint(timeInSeconds), time);
        CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnResume - setting Resume Dialog with [timeInSeconds=%f=%s]. [path=%s] (bma)",timeInSeconds,time.c_str(),itemPath.c_str());
        resumeString.Format(g_localizeStrings.Get(12022).c_str(), time.c_str());
        choices.push_back(resumeString);
        choices.push_back(g_localizeStrings.Get(12021)); // start from the beginning

        int choiceIndex = CGUIDialogBoxeeVideoResume::ShowAndGetInput(choices);

        CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnResume - Resume dialog returned [choiceIndex=%d] (bma)",choiceIndex);

        if (choiceIndex == -1)
        {
          // don't do anything
          return false;
        }

        resumeItem = (choices[choiceIndex] == resumeString);
      }

      db.Close();
    }
  }
  
  AddItemToHistory(item);

  // We have to copy the item here because it was passed as const
  CFileItem newItem(item);
  if (resumeItem)
  {
    newItem.m_lStartOffset = STARTOFFSET_RESUME;
  }
  
  OnPlayMedia(newItem);
    
  return true;
}

bool CGUIDialogBoxeeMediaAction::GetPreferredQuality(const CFileItem& item, int &chosenItem)
{
  int quality = BOXEE::Boxee::GetInstance().GetMetadataEngine().GetProviderPerfQuality(item.GetProperty("link-provider"));
  // get the previous played quality from the database
  if (quality == MEDIA_DATABASE_ERROR)
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::GetPreferredQuality - [provider=%s][providerName=%s] doesn't have preferred quality (bma)",item.GetProperty("link-provider").c_str(),item.GetProperty("link-providername").c_str());
    return false;
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::GetPreferredQuality - [provider=%s][providerName=%s] played before with quality [%d] (bma)",item.GetProperty("link-provider").c_str(),item.GetProperty("link-providername").c_str(),quality);

  // the list is sorted in desc order - so in case the we are looking for an item with the same quality or the item with the closest quality that (less then the previous)
  const CFileItemList* linksFileItemList = item.GetLinksList();
  for (int i=0; i<linksFileItemList->Size(); i++)
  {
    CFileItemPtr linkFileItemToAdd = linksFileItemList->Get(i);
    if  (linkFileItemToAdd->GetPropertyInt("quality") <= quality)
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::GetPreferredQuality - found link with quality [%d] (bma)",linkFileItemToAdd->GetPropertyInt("quality"));
      chosenItem = i;
      return true;

    }
  }

  return false;
}

bool CGUIDialogBoxeeMediaAction::ChooseVideoQuality(const CFileItem& item, int  &chosenItem)
{

  const CFileItemList* linksFileItemList = item.GetLinksList();

  if (!linksFileItemList)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::ChooseVideoQuality - FAILED to get the LinksFileItemList (bma)");
    return false;
  }

  if (linksFileItemList->Size() == 1)
  {
    chosenItem = 0;
    return true;
  }

  CGUIDialogBoxeeVideoQuality *pDlgVideoQuality = (CGUIDialogBoxeeVideoQuality*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_VIDEO_QUALITY);
  pDlgVideoQuality->Reset();
  for (int i=0; i<linksFileItemList->Size(); i++)
  {
    CFileItemPtr linkFileItemToAdd = linksFileItemList->Get(i);
    pDlgVideoQuality->Add((*linkFileItemToAdd));
  }

  pDlgVideoQuality->ChangeDialogType(FULL_CVQ_DIALOG);
  pDlgVideoQuality->DoModal();

  if (pDlgVideoQuality->IsCanceled())
  {
    return false;
  }

  chosenItem = pDlgVideoQuality->GetSelectedItemPos();

  if( pDlgVideoQuality->GetSavePerference())
  {
    CFileItemPtr chosenFileItem = linksFileItemList->Get(chosenItem);
    BOXEE::Boxee::GetInstance().GetMetadataEngine().AddProviderPerf(chosenFileItem->GetProperty("link-provider"),chosenFileItem->GetPropertyInt("quality"));
  }

  return true;
}

bool CGUIDialogBoxeeMediaAction::OnPlayMedia(const CFileItem& item)
{
  CFileItem PlayableItem(item);

  // Reset Playlistplayer, playback started now does
  // not use the playlistplayer.
  g_playlistPlayer.Reset();
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);

  if (PlayableItem.HasLinksList() && PlayableItem.GetLinksList()->Size() > 0)
  {
    CBoxeeSort qualitySort("quality", SORT_METHOD_VIDEO_QUALITY, SORT_ORDER_DESC, "Best Quality", "");
    PlayableItem.SortLinkList(qualitySort);
    int chooseQuality = 0;

    // check if the have a previous link
    if (!GetPreferredQuality(PlayableItem, chooseQuality))
    {
      // we dont have a previous link - let the user choose
      if (!ChooseVideoQuality(PlayableItem, chooseQuality))
      {
        return false;
      }
    }

    CFileItemPtr chosenItem = PlayableItem.GetLinksList()->Get(chooseQuality);
    chosenItem->Dump();
    if (chosenItem.get() == NULL)
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::OnPlayMedia choose NULL item");
      return false;
    }

    // copy the relevant properties from the chosen item
    CopyItemContnetProperties(PlayableItem, chosenItem);
  }

  if (PlayableItem.IsInternetStream() && !g_application.IsConnectedToInternet())
  {
    CGUIDialogBoxeeNetworkNotification::ShowAndGetInput(53743,53745);
    return false;
  }

  return g_application.PlayMedia(PlayableItem);
}

bool CGUIDialogBoxeeMediaAction::OnPlayAudioFromFolder(const CFileItem& item)
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnPlayAudioFromFolder - Enter function with item [path=%s] (apl)",(item.m_strPath).c_str());

  CDirectory dir;
  CFileItemList fileList;
  CFileItemList audioFileList;
  CStdString playlistFileSource;
  
  bool isItemPlayList = item.IsPlayList();

  if(isItemPlayList)
  {
    playlistFileSource = item.m_strPath;
  }
  else
  {
    // in some cases we couldn't retrieve parent path (e.q UPnP path)
    // so we use the parentPath prop which stored in DIRECTORY::GetDirectory 
    if(item.HasProperty("parentPath"))
      playlistFileSource = item.GetProperty("parentPath");
    else
      playlistFileSource = BXUtils::GetParentPath(item.m_strPath);
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnPlayAudioFromFolder - playlistFileSource was set to [%s]. [isItemPlayList=%d] (apl)",playlistFileSource.c_str(),isItemPlayList);

  // Get all of the files for the playlist from the playlistFileSource
  dir.GetDirectory(playlistFileSource,fileList);

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnPlayAudioFromFolder - Call to GetDirectory with [playlistFileSource=%s] returned [%d] items (apl)",playlistFileSource.c_str(),fileList.Size());

  int numOfAudioFileThatWasAddedToTheList = 0;

  // Build a list from of the audio files in the directory
  for(int i=0;i<fileList.Size();i++)
  {
    CFileItemPtr fileItem = fileList.Get(i);

    if((fileItem->IsAudio()) && (!fileItem->IsPlayList()))
    {
      CFileItemPtr item(new CFileItem(*fileItem));

      // Find whether we have information on this track from library
      //CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnPlayAudioFromFolder, Looking for album for track, path = %s (apl)",(fileItem->m_strPath).c_str());
      BXMetadata metadata(MEDIA_ITEM_TYPE_AUDIO);
      if (BOXEE::Boxee::GetInstance().GetMetadataEngine().GetAudioByPath(_P(item->m_strPath), &metadata) == MEDIA_DATABASE_OK)
      {
        DIRECTORY::CBoxeeDatabaseDirectory::CreateAudioItem(&metadata, item.get());
        item->SetProperty("isloaded", true);
      }

      item->SetProperty("isFolderItem", true);

      audioFileList.Add(item);
      numOfAudioFileThatWasAddedToTheList++;
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnPlayAudioFromFolder - File [%s] was added to the audio playlist [index=%d] (apl)",(fileItem->m_strPath).c_str(),numOfAudioFileThatWasAddedToTheList);
    }
  } // for

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnPlayAudioFromFolder - Audio playlist that was build contains [%d] files which were taken from [playlistFileSource=%s] (apl)",audioFileList.Size(),playlistFileSource.c_str());

  if(!isItemPlayList)
  {
    audioFileList.Sort(SORT_METHOD_FILE,SORT_ORDER_ASC);
    
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnPlayAudioFromFolder - playlistFileSource [%s] is NOT a playlist [isItemPlayList=%d] and therefore it was sort by SORT_METHOD_FILE (apl)",playlistFileSource.c_str(),isItemPlayList);
  }

  int songIndexToStartFrom = 0;

  for(int i=0;i<audioFileList.Size();i++)
  {
    CFileItemPtr fileItem = audioFileList.Get(i);

    if((fileItem->m_strPath).Equals(item.m_strPath))
    {
      songIndexToStartFrom = i;
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnPlayAudioFromFolder - Song index to start from was set to [%d] (apl)",songIndexToStartFrom);
    }
  }

  if(audioFileList.Size() > 0)
  {
    AddItemToHistory(item);

    g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
    g_playlistPlayer.Reset();
    g_playlistPlayer.Add(PLAYLIST_MUSIC, audioFileList);
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
    g_playlistPlayer.Play(songIndexToStartFrom);
    g_application.getApplicationMessenger().SwitchToFullscreen();
  }
  else
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnPlayAudioFromFolder - Audio playlist that was build from [playlistFileSource=%s] contains [%d<=0]. There are no files to play (apl)",playlistFileSource.c_str(),audioFileList.Size());
  }

  return true;
}

void CGUIDialogBoxeeMediaAction::PlayTrailer(const CFileItem& _item)
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::PlayTrailer, NEWUI, play trailer, path = %s", _item.GetVideoInfoTag()->m_strTrailer.c_str());
  CFileItem item;
  item.m_strPath = _item.GetVideoInfoTag()->m_strTrailer;
  *item.GetVideoInfoTag() = *_item.GetVideoInfoTag();
  item.GetVideoInfoTag()->m_strTitle.Format("%s (%s)",_item.GetVideoInfoTag()->m_strTitle.c_str(),g_localizeStrings.Get(20410));
  item.SetThumbnailImage(_item.GetThumbnailImage());
  item.SetLabel(item.GetVideoInfoTag()->m_strTitle);
  item.SetProperty("OriginalThumb", _item.GetProperty("OriginalThumb"));
  item.SetProperty("isvideo", true);
  item.SetProperty("istrailer", true);

  // Because we add the "(trailer)" to the VideoInfoTag title parameter,
  // we set the label parameter of item (the temporary item)
  // from its own VideoInfoTag title parameter.
  item.SetLabel(item.GetVideoInfoTag()->m_strTitle);

  g_application.GetBoxeeItemsHistoryList().AddItemToHistory(item);

  g_application.getApplicationMessenger().PlayFile(item);
}

void CGUIDialogBoxeeMediaAction::RunSlideshow(const CStdString& folderPath, const CStdString& itemPath)
{
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
  if (!pSlideShow)
    return;

  if (g_application.IsPlayingVideo())
    g_application.StopPlaying();

  pSlideShow->Reset();
  pSlideShow->RunSlideShow(folderPath,false,false,false,itemPath,true);
}

bool CGUIDialogBoxeeMediaAction::CreatePlaylistAndPlay(const CFileItem& FolderItem,CPlaylistSourceType::PlaylistSourceTypeEnums PlaylistSourceType)
{
  CLog::Log(LOGDEBUG,"In CGUIDialogBoxeeMediaAction::CreatePlaylistAndPlay - Enter function with [FolderItemPath=%s] and [PlaylistSourceType=%s] (dvdfolder)(stack)",(FolderItem.m_strPath).c_str(),GetPlaylistSourceTypeEnumAsString(PlaylistSourceType));

  // Going to build a playlist from the DVD files that in the folder and play them

  CFileItemList folderFilesList;
  CFileItemList filesList;

  DIRECTORY::CDirectory::GetDirectory(FolderItem.m_strPath, folderFilesList);

  CLog::Log(LOGDEBUG,"In CGUIDialogBoxeeMediaAction::CreatePlaylistAndPlay - Directory [item=%s] contains [%d] files (dvdfolder)(stack)",(FolderItem.m_strPath).c_str(), folderFilesList.Size());

  for ( int i = 0; i < folderFilesList.Size(); i++ )
  {
    CFileItemPtr file = folderFilesList.Get(i);

    switch(PlaylistSourceType)
    {
    case CPlaylistSourceType::DVD:
    {
      CStdString fileExt = CUtil::GetExtension(file->m_strPath);
      fileExt.ToLower();

      if(fileExt != ".bup")
      {
        // We need to ignore ".BUP" files because although they are DVD files we don't want to enter them to the playlist

        CLog::Log(LOGDEBUG,"In CGUIDialogBoxeeMediaAction::CreatePlaylistAndPlay - [%d] item [%s] has NO [.bup] extension (dvdfolder)",i, (file->m_strPath).c_str());

        if(CMetadataResolverVideo::IsDVDFilename(file->m_strPath))
        {
          // Note: We copy the FolderItem object for the VideoTag and the properties
          CFileItemPtr matchItem ( new CFileItem(FolderItem) );
          matchItem->m_strPath = file->m_strPath;

          filesList.Add(matchItem);

          CLog::Log(LOGDEBUG,"In CGUIDialogBoxeeMediaAction::CreatePlaylistAndPlay - [%d] item [%s] was added to the playlist (dvdfolder)",i, (file->m_strPath).c_str());
        }
      }
    }
    break;
    case CPlaylistSourceType::STACK:
    {
      // Note: We copy the FolderItem object for the VideoTag
      CFileItemPtr matchItem ( new CFileItem(FolderItem) );
      matchItem->m_strPath = file->m_strPath;

      filesList.Add(matchItem);

      CLog::Log(LOGDEBUG,"In CGUIDialogBoxeeMediaAction::CreatePlaylistAndPlay - [%d] item [%s] was added to the playlist (stack)",i, (file->m_strPath).c_str());
    }
    break;
    default:
    {
      CLog::Log(LOGERROR,"In CGUIDialogBoxeeMediaAction::CreatePlaylistAndPlay - Invalid type was received [%s] (dvdfolder)(stack)",GetPlaylistSourceTypeEnumAsString(PlaylistSourceType));
    }
    break;
    }
  }

  CLog::Log(LOGDEBUG,"In CGUIDialogBoxeeMediaAction::CreatePlaylistAndPlay - After building, the playlist contains [%d] items (dvdfolder)(stack)",filesList.Size());

  filesList.Sort(SORT_METHOD_FILE,SORT_ORDER_ASC);

  g_playlistPlayer.ClearPlaylist(PLAYLIST_VIDEO);
  g_playlistPlayer.Reset();
  g_playlistPlayer.Add(PLAYLIST_VIDEO, filesList);

  //filesList.ClearKeepPointer();

  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
  g_playlistPlayer.Play();

  g_application.getApplicationMessenger().SwitchToFullscreen();

  return true;
}

const char* CGUIDialogBoxeeMediaAction::GetPlaylistSourceTypeEnumAsString(CPlaylistSourceType::PlaylistSourceTypeEnums PlaylistSourceTypeEnum)
{
  switch(PlaylistSourceTypeEnum)
  {
  case CPlaylistSourceType::DVD:
    return "DVD";
  case CPlaylistSourceType::STACK:
    return "STACK";
  case CPlaylistSourceType::NONE:
    return "NONE";
  default:
    CLog::Log(LOGERROR,"Failed to convert enum [%d] to string of PlaylistSourceTypeEnum. Return NONE - (bh)",(int)PlaylistSourceTypeEnum);
    return "NONE";
  }
}

bool CGUIDialogBoxeeMediaAction::IsDVDPlaying()
{
  bool bIsPlaying = false;
#ifdef HAS_DVD_DRIVE
  // Determine type of the inserted DVD
  CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();
  if (!pInfo) 
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::IsDVDPlaying, DVD, could not get DVD info");
    return true;
  }

  bool bIsDVD = (pInfo->IsISOUDF(1) || pInfo->IsISOHFS(1) || pInfo->IsIso9660(1) || pInfo->IsIso9660Interactive(1));
  bool bIsMusic = pInfo->IsAudio(1);

  if (bIsDVD)
  {
    if (g_application.IsPlaying() && (g_application.CurrentFileItem().IsDVD() || g_application.CurrentFileItem().IsISO9660()))
    {
      bIsPlaying = true;
    }
  }
  else if (bIsMusic)
  {
    if (g_application.IsPlaying() && g_application.CurrentFileItem().IsCDDA())
    {
      bIsPlaying = true;
    }
  }
#endif
  return bIsPlaying;
}

void CGUIDialogBoxeeMediaAction::AddItemToHistory(const CFileItem& item)
{
  if (item.HasExternlFileItem())
  {
    g_application.GetBoxeeItemsHistoryList().AddItemToHistory(*(item.GetExternalFileItem().get()));
  }
  else
  {    
    g_application.GetBoxeeItemsHistoryList().AddItemToHistory(item);
  }
}

CDialogButtons::DialogButtonsEnums CGUIDialogBoxeeMediaAction::GetButtonAsEnum(const CStdString& buttonLabel)
{
  if (buttonLabel == g_localizeStrings.Get(53710))
  {
    return CDialogButtons::BTN_READ_MORE;
  }
  else if (buttonLabel == g_localizeStrings.Get(53711))
  {
    return CDialogButtons::BTN_ADD_TO_QUEUE;
  }
  else if (buttonLabel == g_localizeStrings.Get(53712))
  {
    return CDialogButtons::BTN_REMOVE_FROM_QUEUE;
  }
  else if (buttonLabel == g_localizeStrings.Get(53713))
  {
    return CDialogButtons::BTN_MARK_AS_SEEN;
  }
  else if (buttonLabel == g_localizeStrings.Get(53714))
  {
    return CDialogButtons::BTN_MARK_AS_UNSEEN;
  }
  else if (buttonLabel == g_localizeStrings.Get(53715))
  {
    return CDialogButtons::BTN_ADD_SHORTCUT;
  }
  else if (buttonLabel == g_localizeStrings.Get(53716))
  {
    return CDialogButtons::BTN_REMOVE_FROM_SHORTCUT;
  }
  else if (buttonLabel == g_localizeStrings.Get(53717))
  {
    return CDialogButtons::BTN_GOTO_SHOW;
  }
  else if (buttonLabel == g_localizeStrings.Get(53718))
  {
    return CDialogButtons::BTN_TRAILER;
  }
  else if (buttonLabel == g_localizeStrings.Get(53719))
  {
    return CDialogButtons::BTN_SHARE;
  }
  else if (buttonLabel == g_localizeStrings.Get(53721))
  {
    return CDialogButtons::BTN_EJECT;
  }
  else if (buttonLabel == g_localizeStrings.Get(53722))
  {
    return CDialogButtons::BTN_BROWSE;
  }
  else if (buttonLabel == g_localizeStrings.Get(53729))
  {
    return CDialogButtons::BTN_ADD_TO_FAVORITE;
  }
  else if (buttonLabel == g_localizeStrings.Get(53730))
  {
    return CDialogButtons::BTN_REMOVE_FROM_FAVORITE;
  }
  else
  {
    return CDialogButtons::BTN_UNKNOWN;
  }
}

CDialogButtons::DialogButtonsEnums CGUIDialogBoxeeMediaAction::GetButtonActionPropertyAsEnum(const CStdString& buttonActionProperty)
{
  if (buttonActionProperty == ACTION_MORE_INFO)
  {
    return CDialogButtons::BTN_READ_MORE;
  }
  else if (buttonActionProperty == ACTION_ADD_TO_QUEUE)
  {
    return CDialogButtons::BTN_ADD_TO_QUEUE;
  }
  else if (buttonActionProperty == ACTION_REMOVE_FROM_QUEUE)
  {
    return CDialogButtons::BTN_REMOVE_FROM_QUEUE;
  }
  else if (buttonActionProperty == ACTION_MARK_AS_SEEN)
  {
    return CDialogButtons::BTN_MARK_AS_SEEN;
  }
  else if (buttonActionProperty == ACTION_MARK_AS_UNSEEN)
  {
    return CDialogButtons::BTN_MARK_AS_UNSEEN;
  }
  else if (buttonActionProperty == ACTION_ADD_TO_FAVORITE)
  {
    return CDialogButtons::BTN_ADD_TO_FAVORITE;
  }
  else if (buttonActionProperty == ACTION_REMOVE_FROM_FAVORITE)
  {
    return CDialogButtons::BTN_REMOVE_FROM_FAVORITE;
  }
  else if (buttonActionProperty == ACTION_ADD_AS_SHORTCUT)
  {
    return CDialogButtons::BTN_ADD_SHORTCUT;
  }
  else if (buttonActionProperty == ACTION_REMOVE_FROM_SHORTCUT)
  {
    return CDialogButtons::BTN_REMOVE_FROM_SHORTCUT;
  }
  else if (buttonActionProperty == ACTION_GOTO_SHOW)
  {
    return CDialogButtons::BTN_GOTO_SHOW;
  }
  else if (buttonActionProperty == ACTION_REMOVE_FROM_HISTORY)
  {
    return CDialogButtons::BTN_REMOVE_FROM_HISTORY;
  }
  else if (buttonActionProperty == ACTION_PLAY_TRAILER)
  {
    return CDialogButtons::BTN_TRAILER;
  }
  else if (buttonActionProperty == ACTION_SAHRE_ITEM)
  {
    return CDialogButtons::BTN_SHARE;
  }
  else if (buttonActionProperty == ACTION_EJECT)
  {
    return CDialogButtons::BTN_EJECT;
  }
  else if (buttonActionProperty == ACTION_BROWSE)
  {
    return CDialogButtons::BTN_BROWSE;
  }
  else if (buttonActionProperty == ACTION_RESOLVE)
  {
    return CDialogButtons::BTN_RESOLVE;
  }
  else
  {
    return CDialogButtons::BTN_UNKNOWN;
  }
}

CStdString CGUIDialogBoxeeMediaAction::GetLinkPathToShowInDialog(const CFileItem& linkItem)
{
  CStdString linkPathToShow = "";

  // Case of Album
  if (linkItem.HasProperty("IsAlbum") && !(linkItem.GetProperty("AlbumFolderPath").IsEmpty()))
  {
    linkPathToShow = linkItem.GetProperty("AlbumFolderPath");
  }
  else
  {
    linkPathToShow = linkItem.m_strPath;
  }

  return linkPathToShow;
}

void CGUIDialogBoxeeMediaAction::SetLinkPathAsWindowProperty(const CFileItem* linkItem, bool shortenPath)
{
  if (!linkItem)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::SetLinkPathAsWindowProperty - Enter function with a NULL linkItem (bma)");
    return;
  }

  CStdString linkPath = GetLinkPathToShowInDialog(*linkItem);

  return SetLinkPathAsWindowProperty(linkPath, shortenPath);
}

void CGUIDialogBoxeeMediaAction::SetLinkPathAsWindowProperty(const CStdString& _path, bool shortenPath)
{
  CStdString path = _path;

  if (!CUtil::IsHD(path) && !CUtil::IsSmb(path) && !CUtil::IsUPnP(path))
  {
    // don't show this path
    path = "";
  }

  CStdString translatedPath = "";
  CStdString pathLabel = "";

  if(!path.IsEmpty())
  {
    // Translate the path
    translatedPath = _P(path);

    if (CUtil::IsHD(translatedPath))
      CUtil::HideExternalHDPath(translatedPath, translatedPath);

    if (shortenPath)
    {
      // Shorten the path
      CStdString shortPath = translatedPath;
      CUtil::MakeShortenPath(translatedPath,shortPath,80);
      translatedPath = shortPath;
    }

    if(!translatedPath.IsEmpty())
    {
      // Build path label
      pathLabel += translatedPath;
      CUtil::RemovePasswordFromPath(pathLabel);
    }
  }

  SET_CONTROL_LABEL(ITEM_PATH_LABEL,pathLabel);
}

bool CGUIDialogBoxeeMediaAction::AddShortcut(const CBoxeeShortcut& shortcut)
{
  bool succeeded = g_settings.GetShortcuts().AddShortcut(shortcut);

  if (succeeded)
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::AddShortcut - ShortcutItem [name=%s][path=%s] was added (bma)",shortcut.GetName().c_str(),shortcut.GetPath().c_str());

    //g_application.m_guiDialogKaiToast.QueueNotification(g_localizeStrings.Get(53782), "", g_localizeStrings.Get(53737), 3000);
  }

  return succeeded;
}

bool CGUIDialogBoxeeMediaAction::RemoveShortcut(const CBoxeeShortcut& shortcut)
{
  bool succeeded = g_settings.GetShortcuts().RemoveShortcut(shortcut);

  if (succeeded)
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::RemoveShortcut - ShortcutItem [name=%s][path=%s] was removed (bma)",shortcut.GetName().c_str(),shortcut.GetPath().c_str());

    //g_application.m_guiDialogKaiToast.QueueNotification(g_localizeStrings.Get(53783), "", g_localizeStrings.Get(53739), 3000);

    CGUIMessage winmsg(GUI_MSG_UPDATE, WINDOW_BOXEE_BROWSE_SHORTCUTS, 0);
    g_windowManager.SendThreadMessage(winmsg);
  }

  return succeeded;
}

void CGUIDialogBoxeeMediaAction::UpdateItemWithLinkData(const CFileItemPtr linkItem)
{
  if (linkItem->HasLinksList())
  {
    m_item.SetLinksList(linkItem->GetLinksList());
  }

  // save item's labels in order to restore after copy link item
  CStdString orgLabel = m_item.GetLabel();
  CStdString orgLabel2 = m_item.GetLabel2();

  m_item = *(linkItem.get());

  // restore item's original labels
  m_item.SetLabel(orgLabel);
  m_item.SetLabel2(orgLabel2);
}


void CGUIDialogBoxeeMediaAction::CopyItemContnetProperties(CFileItem &dstItem, const CFileItemPtr linkItem)
{

  dstItem.m_strPath = linkItem->m_strPath;
  dstItem.SetProperty("contenttype",linkItem->GetProperty("contenttype"));
  dstItem.SetContentType(linkItem->GetContentType());
  dstItem.SetProperty("link-type",linkItem->GetProperty("link-type"));
  dstItem.SetProperty("link-boxeetype",linkItem->GetProperty("link-boxeetype"));
  dstItem.SetProperty("link-boxeeoffer",linkItem->GetProperty("link-boxeeoffer"));
  dstItem.SetProperty("link-title",linkItem->GetProperty("link-title"));

  if (!linkItem->GetProperty("isinternetstream").IsEmpty())
  {
	  dstItem.SetProperty("isinternetstream",linkItem->GetProperty("isinternetstream"));
  }

  if (!linkItem->GetProperty("NeedVerify").IsEmpty())
  {
	  dstItem.SetProperty("NeedVerify",linkItem->GetProperty("NeedVerify"));
  }

  if (!linkItem->GetProperty("link-provider").IsEmpty())
  {
	  dstItem.SetProperty("link-provider",linkItem->GetProperty("link-provider"));
  }

  if (!linkItem->GetProperty("link-providername").IsEmpty())
  {
	  dstItem.SetProperty("link-providername",linkItem->GetProperty("link-providername"));
  }

  if (!linkItem->GetProperty("link-providerthumb").IsEmpty())
  {
	  dstItem.SetProperty("link-providerthumb",linkItem->GetProperty("link-providerthumb"));
  }

  if (!linkItem->GetProperty("link-countrycodes").IsEmpty())
  {
	  dstItem.SetProperty("link-countrycodes",linkItem->GetProperty("link-countrycodes"));
  }

  if (linkItem->HasProperty("link-countryrel"))
  {
	  dstItem.SetProperty("link-countryrel",linkItem->GetPropertyBOOL("link-countryrel"));
  }

  if (!linkItem->GetProperty("quality-lbl").IsEmpty())
  {
	  dstItem.SetProperty("quality-lbl",linkItem->GetProperty("quality-lbl"));
  }

  dstItem.SetProperty("quality",linkItem->GetPropertyInt("quality"));

  dstItem.SetProperty("link-productslist",linkItem->GetPropertyInt("link-productslist"));

  if (dstItem.GetPropertyBOOL("istrailer"))
    BoxeeUtils::AddTrailerStrToItemLabel(dstItem);

  dstItem.Dump();
}

bool CGUIDialogBoxeeMediaAction::NeedToSubscribe()
{
  // TODO: temp for commit. Need to remove //
  //return false;
  ///////////////////////////////////////////

  if (!m_item.HasProperty("NeedToSubscribe"))
  {
    return false;
  }

  if (!m_item.GetPropertyBOOL("NeedToSubscribe"))
  {
    return false;
  }

  return true;
}

bool CGUIDialogBoxeeMediaAction::OnSubscription()
{
  CFileItemPtr itemPtr(new CFileItem(m_item));
  itemPtr->SetProperty("link-medialabel",m_item.GetLabel());

  bool retVal = false;
  if (CGUIDialogBoxeePaymentProducts::Show(itemPtr))
  {
    retVal = CGUIDialogBoxeePaymentOkPlay::Show();
  }

  return retVal;
}

void CGUIDialogBoxeeMediaAction::OpenDvdDialog()
{
#ifdef HAS_DVD_DRIVE
  CFileItem dvdItem;
  
#ifdef __APPLE__
  CStdString strDev = CLibcdio::GetInstance()->GetDeviceFileName();
  strDev.Replace("/dev/rdisk","/dev/disk");

  // figure out the mounted folder name from the output of "mount". yack.
  CStdString cmd = "mount | grep '^"+strDev+"' | cut -d' ' -f3 | head -1";
  FILE *p_file;
  if( ( p_file = popen( cmd.c_str(), "r" ) ) != NULL )
  {
    char result[2048];
    memset(result,0,2048);
    fread( result, 1, sizeof(result) - 1, p_file );
    pclose( p_file );
    dvdItem.m_strPath = result;
    dvdItem.m_strPath.Trim();
  }
  else
    dvdItem.m_strPath = "iso9660://";  
#else
  dvdItem.m_strPath = "iso9660://";
#endif
  dvdItem.SetProperty("isdvd", true);
  dvdItem.SetThumbnailImage("defaultmusicalbum.png");
  dvdItem.m_iDriveType = CMediaSource::SOURCE_TYPE_DVD;
  
  CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();
  
  CStdString strDiscLabel = g_application.CurrentFileItem().GetLabel();
  if (pInfo)
    strDiscLabel = pInfo->GetDiscLabel();
  
  strDiscLabel.Trim();
  
  if (strDiscLabel == "")
    strDiscLabel = "DVD";
  
  dvdItem.SetLabel(strDiscLabel);
  
  CGUIDialogBoxeeMediaAction::ShowAndGetInput(&dvdItem);
#endif
}

bool CGUIDialogBoxeeMediaAction::SetupTrailer(int& linksAddedCounter)
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::SetupTrailer - enter function with [linksAddedCounter=%d]. [trailerExist=%d] (bma)",linksAddedCounter,(m_trailerLinkItem.get() != NULL));

  if (!m_trailerLinkItem.get())
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::SetupTrailer - No trailer to set (bma)");
    SET_CONTROL_HIDDEN(TRAILER_BUTTON);
    return true;
  }

  SET_CONTROL_VISIBLE(TRAILER_BUTTON);

  if (linksAddedCounter == 0)
  {
    // in case no link was added and there is a trailer item (can happen in feed) -> add the trailer to the ADDITIONAL_LINKS_LIST

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::InitNotAppItem - No links was added and there is a TRAILER item -> add the trailer to the ADDITIONAL_LINKS_LIST (bma)");

    m_linksFileItemList.push_back(m_trailerLinkItem);

    CFileItemPtr trailerLinkItem = m_trailerLinkItem;
    CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), ADDITIONAL_LINKS_LIST, 0, 0, trailerLinkItem);
    OnMessage(winmsg);

    linksAddedCounter++;
    m_tarilerWasAddedToLinkList = true;

    SET_CONTROL_HIDDEN(TRAILER_BUTTON);
  }

  return true;
}

void CGUIDialogBoxeeMediaAction::InitCastPanel()
{
  CVideoInfoTag* videoInfoTag = m_item.GetVideoInfoTag();
  if (!videoInfoTag || videoInfoTag->m_cast.empty())
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::InitCastPanel - there is NO VideoInfoTag [%p] or cast container is empty. [numOfCast=%zu] (bma)",videoInfoTag,videoInfoTag->m_cast.size());
    return;
  }

  CFileItemList castList;
  for (size_t i=0; (i<videoInfoTag->m_cast.size() && i<MAX_NUM_OF_CAST_MEMBER_TO_SHOW); i++)
  {
    CFileItemPtr castItem(new CFileItem());
    castItem->SetLabel(videoInfoTag->m_cast[i].strName);
    castItem->SetLabel2(videoInfoTag->m_cast[i].strRole);

    castList.Add(castItem);
  }

  CGUIMessage msgBind(GUI_MSG_LABEL_BIND, GetID(), CAST_PANEL_ID, 0,0, &castList);
  OnMessage(msgBind);
}

int CGUIDialogBoxeeMediaAction::GetVisibleButtonListControlId()
{
  if (m_visibleButtonsListId == 0)
  {
    const CGUIControl* pButtonListCtrl = GetControl(BUTTONS_LIST_MOVIE);
    if (pButtonListCtrl && pButtonListCtrl->IsVisible())
    {
      m_visibleButtonsListId = pButtonListCtrl->GetID();
    }
    else
    {
      pButtonListCtrl = GetControl(BUTTONS_LIST_NOT_MOVIE);
      if (pButtonListCtrl && pButtonListCtrl->IsVisible())
      {
        m_visibleButtonsListId = pButtonListCtrl->GetID();
      }
    }
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::GetVisibleButtonListControlId - visible ButtonList is [id=%d] (bma)",m_visibleButtonsListId);

  return m_visibleButtonsListId;
}

