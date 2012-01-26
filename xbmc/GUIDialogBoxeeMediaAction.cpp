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
#include "BoxeeUtils.h"
#include "URL.h"
#include "FileSystem/Directory.h"
#include "Application.h"
#include "MusicInfoTag.h"
#include "FileSystem/PluginDirectory.h"
#include "FileSystem/Directory.h"
#include "FileSystem/BoxeeDatabaseDirectory.h"
#include "GUIImage.h"
#include "VideoInfoTag.h"
#include "LastFmManager.h"
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
#include "GUIDialogContextMenu.h"
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
#include "GUIDialogBoxeeNetworkNotification.h"

#ifdef _WIN32
#include "WIN32Util.h"
#endif

using namespace DIRECTORY;
using namespace BOXEE;
using namespace XFILE;

#define LINKS_GROUP                            6000

#define PLAY_BUTTON                            6010
#define PLAY_BUTTON_LABEL_FOCUS                6011
#define PLAY_BUTTON_LABEL_NOT_FOCUS            6012
#define PLAY_BUTTON_ADDITION_LABEL_FOCUS       6013
#define PLAY_BUTTON_ADDITION_LABEL_NOT_FOCUS   6014
#define PLAY_BUTTON_ADDITION_LABEL_FOCUS_1     6015
#define PLAY_BUTTON_ADDITION_LABEL_NOT_FOCUS_1 6016

#define ADDITIONAL_LINKS_LIST                  6020
#define ADDITIONAL_LINKS_LIST_LABEL            6021

#define BUTTONS_GROUP                          6100
#define TRAILER_LIST                           6110
#define BUTTONS_LIST                           6120

#define INFO_GROUP                             6200
#define BACK_GROUP                             6210

#define APP_GROUP                              6300
#define APP_LUANCH_BUTTON                      6310
#define APP_ADD_BUTTON                         6320
#define APP_REMOVE_BUTTON                      6330
#define APP_SHORTCUT_BUTTON                    6340

#define ITEM_PATH_LABEL                        6742


//#define LINK_LIST                       6000
//#define LINK_LIST_LABEL                 6001

//#define TRAILER_LIST                    6500
//#define BUTTONS_LIST                    6600

//#define PLAY_BUTTON                     6010
//#define PLAY_LABEL_FOCUS                6011
//#define PLAY_LABEL_NOT_FOCUS            6012
//#define PLAY_ADDITION_LABEL_FOCUS       6013
//#define PLAY_ADDITION_LABEL_NOT_FOCUS   6014
//#define SOURCES_BUTTON                  6210

//#define SOURCES_GROUP                   6100
//#define INFO_GROUP                      6200

//#define ITEM_PATH_LABEL                 6500

#define HIDDEN_CONTAINER                5000
#define NOTIFICATION_APPEARANCE_IN_SEC  5000

#define BUTTON_ACTION_PROPERTY_NAME      "button-action"
#define ACTION_MORE_INFO                 "more-info"
#define ACTION_ADD_TO_QUEUE              "add-to-queue"
#define ACTION_REMOVE_FROM_QUEUE         "remove-from-queue"
#define ACTION_MARK_AS_SEEN              "mark-as-seen"
#define ACTION_MARK_AS_UNSEEN            "mark-as-unseen"
#define ACTION_ADD_AS_SHORTCUT           "add-as-shortcut"
#define ACTION_REMOVE_FROM_SHORTCUT      "remove-from-shortcut"
#define ACTION_GOTO_SHOW                 "goto-show"
#define ACTION_PLAY_TRAILER              "play-trailer"
#define ACTION_SAHRE_ITEM                "share-item"
#define ACTION_REMOVE_FROM_HISTORY       "remove-from-history"
#define ACTION_EJECT                     "eject"
#define ACTION_BROWSE                    "browse"
#define ACTION_RESOLVE                   "resolve"

#define PLAY_BUTTON_NOT_INIT_INDEX       -2
#define PLAY_BUTTON_INIT_WITH_TAILER     -1
#define PLAY_BUTTON_ISHD                 "is-hd"
#define PLAY_BUTTON_THUMB                "pb_provider_thumb"

CGUIDialogBoxeeMediaAction::CGUIDialogBoxeeMediaAction(void) : CGUIDialog(WINDOW_DIALOG_BOXEE_MEDIA_ACTION, "boxee_media_action.xml")
{
  m_bConfirmed = true;
  m_listContainServerLinks = false;
  m_numOfButtons = 0;
}

CGUIDialogBoxeeMediaAction::~CGUIDialogBoxeeMediaAction()
{

}

void CGUIDialogBoxeeMediaAction::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  int activeWindow = g_windowManager.GetActiveWindow();

  if (m_item.IsApp() && !m_item.GetProperty("appid").IsEmpty() && (activeWindow == WINDOW_BOXEE_BROWSE_APPS || activeWindow == WINDOW_BOXEE_BROWSE_REPOSITORIES))
  {
    InitAppItem();
    return;
  }

  if (m_item.IsVideo() && m_item.GetPropertyBOOL("isFolderItem"))
  {
    BoxeeUtils::GetLocalVideoMetadata(m_item);
  }

  InitNotAppItem();
}

void CGUIDialogBoxeeMediaAction::InitAppItem()
{
  // Send the item to the special container to allow skin access
  CFileItemPtr itemPtr(new CFileItem(m_item));
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), HIDDEN_CONTAINER, 0, 0, itemPtr);
  g_windowManager.SendThreadMessage(winmsg);

  SET_CONTROL_HIDDEN(LINKS_GROUP);
  SET_CONTROL_HIDDEN(PLAY_BUTTON);
  SET_CONTROL_HIDDEN(ADDITIONAL_LINKS_LIST);
  SET_CONTROL_HIDDEN(BUTTONS_LIST);

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
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::InitAppItem - Enter function with [NUmOfLinks=%d] < 3 (bma)",(int)m_linksFileItemList.size());
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

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildAppFileItemLinksList - After add LAUNCH [label=%s][path=%s] for item [label=%s][appId=%s]. [linksFileItemList=%d] (bbma)",launchAppFileItem->GetLabel().c_str(),launchAppFileItem->m_strPath.c_str(),m_item.GetLabel().c_str(),m_item.GetProperty("appid").c_str(),(int)m_linksFileItemList.size());

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

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildAppFileItemLinksList - After add INSTALL [label=%s] for item [label=%s][appId=%s]. [linksFileItemList=%d] (bbma)",installAppFileItem->GetLabel().c_str(),m_item.GetLabel().c_str(),m_item.GetProperty("appid").c_str(),(int)m_linksFileItemList.size());
  }
  else
  {
    CFileItemPtr uninstallAppFileItem(new CFileItem(g_localizeStrings.Get(53762)));
    m_linksFileItemList.push_back(uninstallAppFileItem);

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildAppFileItemLinksList - After add REMOVE [label=%s] for item [label=%s][appId=%s]. [linksFileItemList=%d] (bbma)",uninstallAppFileItem->GetLabel().c_str(),m_item.GetLabel().c_str(),m_item.GetProperty("appid").c_str(),(int)m_linksFileItemList.size());
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

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildAppFileItemLinksList - After add SORTCUT [label=%s][type=%s] for item [label=%s][appId=%s]. [linksFileItemList=%d] (bma)",shortcutButton->GetLabel().c_str(),buttonType.c_str(),m_item.GetLabel().c_str(),m_item.GetProperty("appid").c_str(),(int)m_linksFileItemList.size());

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
  m_listContainServerLinks = false;

  m_playButtonItemIndex = PLAY_BUTTON_NOT_INIT_INDEX;

  m_numOfButtons = 0;

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnInitWindow - Main item has properties [free-play=%s][clip-play=%s][trailer-play=%s][rent-play=%s][buy-play=%s][sub-play=%s] (bma)",m_item.GetProperty("free-play").c_str(),m_item.GetProperty("clip-play").c_str(),m_item.GetProperty("trailer-play").c_str(),m_item.GetProperty("rent-play").c_str(),m_item.GetProperty("buy-play").c_str(),m_item.GetProperty("sub-play").c_str());

  // Send the item to the special container to allow skin access
  CFileItemPtr itemPtr(new CFileItem(m_item));
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), HIDDEN_CONTAINER, 0, 0, itemPtr);
  g_windowManager.SendThreadMessage(winmsg);

  int linksAddedCounter = 0;

  if (BoxeeUtils::CanPlay(m_item) || m_item.GetPropertyBOOL("ishistory"))
  {
    if (m_item.HasLinksList())
    {
      /////////////////////////////////////////////////////////////////////////////////////////////
      // in case there are links in the FileItem -> build a list and add them to the dialog list //
      /////////////////////////////////////////////////////////////////////////////////////////////

      bool succeeded = BuildLinksFileItemList();

      if (!succeeded)
      {
        CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::OnInitWindow - FAILED to initialize item links. [ItemLabel=%s][ItemPath=%s] (bma)",m_item.GetLabel().c_str(),m_item.m_strPath.c_str());
        return;
      }

      // there are links -> reset the item m_strPath (it isn't relevant and so it won't be pass in share)
      m_item.m_strPath = "";
      m_item.SetContentType("");

      // Put first link labels in play button
      if (m_linksFileItemList.size() > 0)
      {
        m_playButtonItemIndex = 0;
        bool playButtonWasSet = false;

        while (!playButtonWasSet && (m_playButtonItemIndex < (int)m_linksFileItemList.size()))
        {
          CFileItemPtr linkFileItem = m_linksFileItemList[m_playButtonItemIndex];

          if (linkFileItem->HasProperty("is-hd"))
          {
            SetProperty("is-hd", 1);
          }
          else
          {
            SetProperty("is-hd", 0);
          }

          itemPtr->SetProperty(PLAY_BUTTON_THUMB, linkFileItem->GetProperty("link-providerthumb"));

          CStdString label = linkFileItem->GetLabel();
          CStdString label2 = linkFileItem->GetLabel2();

          if (CFileItem::GetLinkBoxeeTypeAsEnum(linkFileItem->GetProperty("link-boxeetype")) == CLinkBoxeeType::LOCAL)
          {
            SET_CONTROL_LABEL(PLAY_BUTTON_LABEL_FOCUS, g_localizeStrings.Get(53751));
            SET_CONTROL_LABEL(PLAY_BUTTON_LABEL_NOT_FOCUS, g_localizeStrings.Get(53751));
            SET_CONTROL_LABEL(PLAY_BUTTON_ADDITION_LABEL_FOCUS, "");
            SET_CONTROL_LABEL(PLAY_BUTTON_ADDITION_LABEL_NOT_FOCUS, "");
			SET_CONTROL_LABEL(PLAY_BUTTON_ADDITION_LABEL_FOCUS_1, "");
            SET_CONTROL_LABEL(PLAY_BUTTON_ADDITION_LABEL_NOT_FOCUS_1, "");
          }
          else
          {
            CStdString playButtonLabel = label2;
            playButtonLabel.ToUpper();

            SET_CONTROL_LABEL(PLAY_BUTTON_LABEL_FOCUS, playButtonLabel);
            SET_CONTROL_LABEL(PLAY_BUTTON_LABEL_NOT_FOCUS, playButtonLabel);
            SET_CONTROL_LABEL(PLAY_BUTTON_ADDITION_LABEL_FOCUS, label);
            SET_CONTROL_LABEL(PLAY_BUTTON_ADDITION_LABEL_NOT_FOCUS, label);
			SET_CONTROL_LABEL(PLAY_BUTTON_ADDITION_LABEL_FOCUS_1, label);
            SET_CONTROL_LABEL(PLAY_BUTTON_ADDITION_LABEL_NOT_FOCUS_1, label);
          }

          focusedLinkPathToShow = linkFileItem->m_strPath;

          linksAddedCounter++;

          playButtonWasSet = true;

          CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnInitWindow - [%d/%d] - Adding item [label=%s][label2=%s][path=%s][thumb=%s] to PLAY_BUTTON. [link-title=%s][link-url=%s][link-boxeetype=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d] (bma)",m_playButtonItemIndex+1,(int)m_linksFileItemList.size(),linkFileItem->GetLabel().c_str(),linkFileItem->GetLabel2().c_str(),linkFileItem->m_strPath.c_str(),linkFileItem->GetThumbnailImage().c_str(),linkFileItem->GetProperty("link-title").c_str(),linkFileItem->GetProperty("link-url").c_str(),linkFileItem->GetProperty("link-boxeetype").c_str(),linkFileItem->GetProperty("link-type").c_str(),linkFileItem->GetProperty("link-provider").c_str(),linkFileItem->GetProperty("link-providername").c_str(),linkFileItem->GetProperty("link-providerthumb").c_str(),linkFileItem->GetProperty("link-countrycodes").c_str(),linkFileItem->GetPropertyBOOL("link-countryrel"));
        }

        // add the rest of the links
        for (int i=(m_playButtonItemIndex+1);i<(int)m_linksFileItemList.size();i++)
        {
          CFileItemPtr linkFileItem = m_linksFileItemList[i];

          CFileItemPtr linkFileItemToAdd(new CFileItem(*(linkFileItem.get())));
          CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnInitWindow - [%d/%d] - Going to add item [label=%s][label2=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d] (bma)",i+1,(int)m_linksFileItemList.size(),linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->GetLabel2().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItem->GetPropertyBOOL("link-countryrel"));

          CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), ADDITIONAL_LINKS_LIST, 0, 0, linkFileItemToAdd);
          OnMessage(winmsg);

          linksAddedCounter++;
          m_listContainServerLinks = true;
        }
      }

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnInitWindow - After adding [%d] items to the dialog list (bma)",linksAddedCounter);

      if (linksAddedCounter == 0)
      {
        m_playButtonItemIndex = PLAY_BUTTON_NOT_INIT_INDEX;

        if (m_trailerItem.get())
        {
          // in case no link was added and there is a trailer item (can happen in feed) -> add the trailer to the PLAY BUTTON

          CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnInitWindow - No links was added and there is a TRAILER item -> add the trailer to the PLAY_BUTTON (bma)");

          CFileItemPtr trailerItem = m_trailerItem;
          SET_CONTROL_LABEL(PLAY_BUTTON_LABEL_FOCUS, trailerItem->GetProperty("link-providername"));
          SET_CONTROL_LABEL(PLAY_BUTTON_LABEL_NOT_FOCUS, trailerItem->GetProperty("link-providername"));
          SET_CONTROL_LABEL(PLAY_BUTTON_ADDITION_LABEL_FOCUS, trailerItem->GetProperty("link-boxeetype"));
          SET_CONTROL_LABEL(PLAY_BUTTON_ADDITION_LABEL_NOT_FOCUS, trailerItem->GetProperty("link-boxeetype"));
		  SET_CONTROL_LABEL(PLAY_BUTTON_ADDITION_LABEL_FOCUS_1, trailerItem->GetProperty("link-boxeetype"));
          SET_CONTROL_LABEL(PLAY_BUTTON_ADDITION_LABEL_NOT_FOCUS_1, trailerItem->GetProperty("link-boxeetype"));

          m_playButtonItemIndex = PLAY_BUTTON_INIT_WITH_TAILER;

          linksAddedCounter++;
        }
      }
    }
    else if (!m_item.m_strPath.IsEmpty())
    {
      CFileItemPtr fileItemToAdd(new CFileItem(m_item));

      //fileItemToAdd->SetLabel2(g_localizeStrings.Get(53750));
      focusedLinkPathToShow = GetLinkPathToShowInDialog(m_item);

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnInitWindow - There are no LinksFileItem -> Going to add the original item [label=%s][label2=%s]][path=%s][thumb=%s] (bma)",fileItemToAdd->GetLabel().c_str(),fileItemToAdd->GetLabel2().c_str(),fileItemToAdd->m_strPath.c_str(),fileItemToAdd->GetThumbnailImage().c_str());

      CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), ADDITIONAL_LINKS_LIST, 0, 0, fileItemToAdd);
      OnMessage(winmsg);

      linksAddedCounter++;

      if (fileItemToAdd->GetPropertyBOOL("IsFolder") && !fileItemToAdd->IsPlayList())
      {
        SET_CONTROL_LABEL(PLAY_BUTTON_LABEL_FOCUS, g_localizeStrings.Get(53755));
        SET_CONTROL_LABEL(PLAY_BUTTON_LABEL_NOT_FOCUS, g_localizeStrings.Get(53755));
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
        else if (fileItemToAdd->m_strPath.Left(6) == "http:/" || fileItemToAdd->m_strPath.Left(7) == "flash:/")
        {
          CURL url(fileItemToAdd->m_strPath);
          provider = url.GetHostName();
        }

        if (!provider.IsEmpty())
        {
          CStdString providerLabel = g_localizeStrings.Get(53752) + provider;
          SET_CONTROL_LABEL(PLAY_BUTTON_ADDITION_LABEL_FOCUS, providerLabel);
          SET_CONTROL_LABEL(PLAY_BUTTON_ADDITION_LABEL_NOT_FOCUS, providerLabel);
		  SET_CONTROL_LABEL(PLAY_BUTTON_ADDITION_LABEL_FOCUS_1, providerLabel);
          SET_CONTROL_LABEL(PLAY_BUTTON_ADDITION_LABEL_NOT_FOCUS_1, providerLabel);
        }
      }
    }
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnInitWindow - After adding links [linksAddedCounter=%d]. [PlayButtonItemIndex=%d][HasTrailer=%d] (bma)",linksAddedCounter,m_playButtonItemIndex,m_trailerItem.get()?true:false);

  m_numOfButtons = FillDialogButtons((linksAddedCounter>0)?true:false);

  if (linksAddedCounter > 0)
  {
    /////////////////////
    // links was added //
    /////////////////////

    SET_CONTROL_HIDDEN(INFO_GROUP);
    SET_CONTROL_VISIBLE(LINKS_GROUP);
    SET_CONTROL_VISIBLE(PLAY_BUTTON);
    SET_CONTROL_FOCUS(PLAY_BUTTON, 0);

    SetLinkPathAsWindowProperty(focusedLinkPathToShow);

    if (linksAddedCounter < 2)
    {
      /////////////////////////////
      // only one link was added //
      /////////////////////////////

      //CONTROL_DISABLE(ADDITIONAL_LINKS_LIST);
      SET_CONTROL_HIDDEN(ADDITIONAL_LINKS_LIST);
      SET_CONTROL_HIDDEN(ADDITIONAL_LINKS_LIST_LABEL);
    }
    else
    {
      //////////////////////////////////
      // more then one link was added //
      //////////////////////////////////

      SET_CONTROL_VISIBLE(ADDITIONAL_LINKS_LIST);
      SET_CONTROL_VISIBLE(ADDITIONAL_LINKS_LIST_LABEL);
    }
  }
  else
  {
    ////////////////////////
    // no links was added //
    ////////////////////////

    SET_CONTROL_HIDDEN(INFO_GROUP);
    SET_CONTROL_HIDDEN(LINKS_GROUP);
    SET_CONTROL_HIDDEN(PLAY_BUTTON);
    SET_CONTROL_HIDDEN(ADDITIONAL_LINKS_LIST);

    if (m_numOfButtons > 0)
    {
      SET_CONTROL_VISIBLE(BUTTONS_LIST);
      SET_CONTROL_FOCUS(BUTTONS_LIST,0);
    }
    else
    {
      SET_CONTROL_HIDDEN(BUTTONS_LIST);
    }
  }

  if (m_trailerItem.get() && (m_playButtonItemIndex != PLAY_BUTTON_INIT_WITH_TAILER))
  {
    SET_CONTROL_VISIBLE(TRAILER_LIST);
  }
  else
  {
    SET_CONTROL_HIDDEN(TRAILER_LIST);
  }
}

bool CGUIDialogBoxeeMediaAction::AddFileItemToList(std::map<CStdString, CFileItemPtr>& addedLinksMap,  std::list<CFileItemPtr>& pathsList)
{
  int addedPathsCounter = addedLinksMap.size();

  std::list<CFileItemPtr>::iterator itFreelist = pathsList.begin();
  std::map<CStdString, CFileItemPtr>::iterator mapIt;
  while (itFreelist != pathsList.end())
  {
    CFileItemPtr linkFileItemToAdd = *(itFreelist);

    mapIt = addedLinksMap.find(linkFileItemToAdd->GetProperty("link-providername"));
    if (mapIt == addedLinksMap.end())
    {
      CFileItemPtr newItem(new CFileItem(m_item));
      newItem->ClearLinksList();
      // save some of the properties.
      newItem->m_strPath = linkFileItemToAdd->m_strPath;
      newItem->SetLabel(linkFileItemToAdd->GetLabel());
      newItem->SetLabel2(linkFileItemToAdd->GetProperty("link-providername"));
      newItem->SetProperty("link-boxeetype", linkFileItemToAdd->GetProperty("link-boxeetype"));
      newItem->SetProperty("link-providername", linkFileItemToAdd->GetProperty("link-providername"));
      newItem->SetProperty("link-providerthumb", linkFileItemToAdd->GetProperty("link-providerthumb"));
      newItem->AddLink(linkFileItemToAdd->GetLabel(), linkFileItemToAdd->m_strPath,
                       linkFileItemToAdd->GetContentType(true), CFileItem::GetLinkBoxeeTypeAsEnum(linkFileItemToAdd->GetProperty("link-boxeetype")),
                       linkFileItemToAdd->GetProperty("link-provider"), linkFileItemToAdd->GetProperty("link-providername"), linkFileItemToAdd->GetProperty("link-providerthumb"),
                       linkFileItemToAdd->GetProperty("link-countrycodes"), linkFileItemToAdd->GetPropertyBOOL("link-countryrel"),
                       linkFileItemToAdd->GetProperty("quality-lbl"), linkFileItemToAdd->GetPropertyInt("quality"));

      if (linkFileItemToAdd->GetPropertyInt("quality") >= 720)
      {
        newItem->SetProperty("is-hd","true");
      }

      // local files are special - we will treat it as different provider
      CLinkBoxeeType::LinkBoxeeTypeEnums linkBoxeeTypeEnum = CFileItem::GetLinkBoxeeTypeAsEnum(linkFileItemToAdd->GetProperty("link-boxeetype"));

      if (linkBoxeeTypeEnum != CLinkBoxeeType::LOCAL)
      {
        addedLinksMap[linkFileItemToAdd->GetProperty("link-providername")] =  newItem;
      }
      else
      {
        newItem->m_strPath = linkFileItemToAdd->m_strPath;
        newItem->SetLabel2(g_localizeStrings.Get(53750));
        CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::AddFileItemToList Local file - push it to the linked list");
      }
      m_linksFileItemList.push_back(newItem);

      addedPathsCounter++;
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::AddFileItemToList - [%d] - Add link item [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d] [quality-lbl=%s] [quality=%d] (bbma)",addedPathsCounter,newItem->GetLabel().c_str(),newItem->m_strPath.c_str(),newItem->GetThumbnailImage().c_str(),newItem->GetProperty("link-title").c_str(),newItem->GetProperty("link-url").c_str(),newItem->GetProperty("link-boxeetype").c_str(),newItem->GetProperty("link-type").c_str(),newItem->GetProperty("link-provider").c_str(),newItem->GetProperty("link-providername").c_str(),newItem->GetProperty("link-providerthumb").c_str(),newItem->GetProperty("link-countrycodes").c_str(),newItem->GetPropertyBOOL("link-countryrel"), linkFileItemToAdd->GetProperty("quality-lbl").c_str(), linkFileItemToAdd->GetPropertyInt("quality"));
    }
    else
    {
      CFileItemPtr Item = (mapIt)->second;

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::AddFileItemToList - Add link to existing item [%d]  [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d] (bbma)",
                           addedPathsCounter,linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItemToAdd->GetPropertyBOOL("link-countryrel"));

      Item->AddLink(linkFileItemToAdd->GetLabel(), linkFileItemToAdd->m_strPath,
          linkFileItemToAdd->GetContentType(true), CFileItem::GetLinkBoxeeTypeAsEnum(linkFileItemToAdd->GetProperty("link-boxeetype")),
          linkFileItemToAdd->GetProperty("link-provider"), linkFileItemToAdd->GetProperty("link-providername"), linkFileItemToAdd->GetProperty("link-providerthumb"),
          linkFileItemToAdd->GetProperty("link-countrycodes"), linkFileItemToAdd->GetPropertyBOOL("link-countryrel"),
          linkFileItemToAdd->GetProperty("quality-lbl"), linkFileItemToAdd->GetPropertyInt("quality"));

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

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - Item [label=%s] has [%d] links (bma)", m_item.GetLabel().c_str(), linksFileItemList->Size());

  std::list<CFileItemPtr> localPathsList;
  std::list<CFileItemPtr> freeWebPathsList;
  std::list<CFileItemPtr> subscribeServicePathsList;
  //CFileItemPtr subscribeServicePath;
  std::list<CFileItemPtr> otherPathsVec;

  for (int i=0; i<linksFileItemList->Size(); i++)
  {
    CFileItemPtr linkFileItemToAdd = linksFileItemList->Get(i);

    /*
    bool isAllowed = linkFileItemToAdd->IsAllowed();
    if (!isAllowed)
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - [%d/%d] - Skipping link item [label=%s][path=%s][thumb=%s] because [MyCountryCode=%s][LinkCountryCodes=%s]->[isAllowed=%d]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][country-codes=%s][link-countrycodes=%s][country-rel=%d][link-countryrel=%d] (bbma)",i+1,linksFileItemList->Size(),linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),g_application.GetCountryCode().c_str(),linkFileItemToAdd->GetProperty("country-codes").c_str(),isAllowed,linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("country-codes").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItemToAdd->GetPropertyBOOL("country-rel"),linkFileItemToAdd->GetPropertyBOOL("link-countryrel"));
      continue;
    }
    */

    //linkFileItemToAdd->Dump();

    CLinkBoxeeType::LinkBoxeeTypeEnums linkBoxeeTypeEnum = CFileItem::GetLinkBoxeeTypeAsEnum(linkFileItemToAdd->GetProperty("link-boxeetype"));

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - [%d/%d] - Handling link item [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d] (bbma)",i+1,linksFileItemList->Size(),linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItemToAdd->GetPropertyBOOL("link-countryrel"));

    switch (linkBoxeeTypeEnum)
    {
    case CLinkBoxeeType::LOCAL:
    {
      linkFileItemToAdd->SetLabel2(g_localizeStrings.Get(53750));

      // there is local path -> push to front of the list in order to show it first
      localPathsList.push_back(linkFileItemToAdd);

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - LOCAL - Add link item to localPaths [size=%d]. [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d] (bbma)",(int)localPathsList.size(),linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItemToAdd->GetPropertyBOOL("link-countryrel"));
    }
    break;
    case CLinkBoxeeType::FEATURE:
    {
      // there is "free with adds" path -> push to back of the list in in case there is local path
      freeWebPathsList.push_back(linkFileItemToAdd);

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - FEATURE - Add link item to freeWebPaths [size=%d]. [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d] (bbma)",(int)freeWebPathsList.size(),linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItemToAdd->GetPropertyBOOL("link-countryrel"));
    }
    break;
    case CLinkBoxeeType::TRAILER:
    {
      // there is at least 1 trailer link
      m_trailerItem = linkFileItemToAdd;
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - TRAILER - Save link item as first trailerPath [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d] (bbma)",linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItemToAdd->GetPropertyBOOL("link-countryrel"));
    }
    break;
    case CLinkBoxeeType::SUBSCRIPTION:
    {
      if (Boxee::GetInstance().GetBoxeeClientServerComManager().IsRegisterToServices(linkFileItemToAdd->GetProperty("link-provider"),CServiceIdentifierType::NAME))
      {
        // a subscribe service that the user is subscribe to

        subscribeServicePathsList.push_back(linkFileItemToAdd);

        CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - SUBSCRIPTION - Add link item to subscribeServicePaths [size=%d]. [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d] (bbma)",(int)subscribeServicePathsList.size(),linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItemToAdd->GetPropertyBOOL("link-countryrel"));
      }
      else
      {
        otherPathsVec.push_back(linkFileItemToAdd);

        CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - SUBSCRIPTION - Add link item to otherPaths [size=%d]. [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d] (bbma)",(int)otherPathsVec.size(),linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItemToAdd->GetPropertyBOOL("link-countryrel"));
      }
    }
    break;
    default:
    {
      otherPathsVec.push_back(linkFileItemToAdd);

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::BuildLinksFileItemList - OTHER - Add link item to otherPaths [size=%d]. [label=%s][path=%s][thumb=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d] (bbma)",(int)otherPathsVec.size(),linkFileItemToAdd->GetLabel().c_str(),linkFileItemToAdd->m_strPath.c_str(),linkFileItemToAdd->GetThumbnailImage().c_str(),linkFileItemToAdd->GetProperty("link-title").c_str(),linkFileItemToAdd->GetProperty("link-url").c_str(),linkFileItemToAdd->GetProperty("link-boxeetype").c_str(),linkFileItemToAdd->GetProperty("link-type").c_str(),linkFileItemToAdd->GetProperty("link-provider").c_str(),linkFileItemToAdd->GetProperty("link-providername").c_str(),linkFileItemToAdd->GetProperty("link-providerthumb").c_str(),linkFileItemToAdd->GetProperty("link-countrycodes").c_str(),linkFileItemToAdd->GetPropertyBOOL("link-countryrel"));
    }
    break;
    }
  }

  std::map<CStdString, CFileItemPtr> addedLinksMap;

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
      SET_CONTROL_FOCUS(BUTTONS_LIST,0);
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

    if (focusedControl == PLAY_BUTTON)
    {
      if (m_playButtonItemIndex >= 0)
      {
        linkItem = (CFileItem*)m_linksFileItemList[m_playButtonItemIndex].get();
      }
      else if (m_playButtonItemIndex == PLAY_BUTTON_INIT_WITH_TAILER)
      {
        linkItem = m_trailerItem.get();
      }
      else
      {
        linkItem = &m_item;
      }
    }
    else if (focusedControl == ADDITIONAL_LINKS_LIST)
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
  switch ( message.GetMessage() )
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

      CGUIMessage winmsg1(GUI_MSG_LABEL_RESET, GetID(), 5000);
      g_windowManager.SendMessage(winmsg1);

      CFileItemPtr itemPtr(new CFileItem(*pItem));
      CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), 5000, 0, 0, itemPtr);
      g_windowManager.SendMessage(winmsg);

      FillDialogButtons(m_linksFileItemList.size() > 0);

      delete pItem;
    }
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
  case BUTTONS_LIST:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnClick - Handling click on [%d=BUTTONS_LIST] (bma)",iControl);

    bool needToCloseDialog = false;

    succeeded = HandleClickOnButtonList(needToCloseDialog);

    if (needToCloseDialog)
    {
      Close(true);
    }
  }
  break;
  case TRAILER_LIST:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnClick - Handling click on [%d=TRAILER_LIST] (bma)",iControl);

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
    SET_CONTROL_FOCUS(BUTTONS_LIST,0);
  }
  break;
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
        clickedLinkFileItem = m_trailerItem;
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
  CURL url(m_item.m_strPath);
  if (url.GetProtocol() == "app")
  {
    InstallOrUpgradeAppBG* job = new InstallOrUpgradeAppBG(url.GetHostName(), true, false);
    if (CUtil::RunInBG(job))
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
    g_application.m_guiDialogKaiToast.QueueNotification("", "",g_localizeStrings.Get(53824), 5000);

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
    CStdString heading = g_localizeStrings.Get(52151);
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

  if (!m_listContainServerLinks)
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnItemList - No LinkList. Going to call OnPlay() with item [label=%s][path=%s] (bma)",m_item.GetLabel().c_str(),m_item.m_strPath.c_str());
  }
  else
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), ADDITIONAL_LINKS_LIST);
    g_windowManager.SendMessage(msg);
    int itemIndex = msg.GetParam1();

    if ((itemIndex < 0) || (itemIndex >= (int)m_linksFileItemList.size()))
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::HandleClickOnItemList - Index of item clicked is [%d] while [LinksFileItemListSize=%d] (bma)",itemIndex,(int)m_linksFileItemList.size());
      return false;
    }

    CFileItemPtr clickedLinkFileItem = m_linksFileItemList[itemIndex + 1];

    // update item for play
    UpdateItemWithLinkData(clickedLinkFileItem);

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnItemList - Click on item [label=%s][path=%s] so MainItem path was set to [%s]. [contenttype=%s][link-boxeetype=%s] (bma)",clickedLinkFileItem->GetLabel().c_str(),clickedLinkFileItem->m_strPath.c_str(),m_item.m_strPath.c_str(),m_item.GetProperty("contenttype").c_str(),m_item.GetProperty("link-boxeetype").c_str());

    //retVal = OnPlay(*(clickedLinkFileItem.get()));
  }

  if (BoxeeUtils::CanResume(m_item))
  {
    retVal = OnResume(m_item);
  }
  else
  {
    retVal = OnPlay(m_item);
  }

  return retVal;
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnButtonList(bool& needToCloseDialog)
{
  CGUIBaseContainer* pContainer = (CGUIBaseContainer*)GetControl(BUTTONS_LIST);

  if (!pContainer)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - FAILED to get container [%d=BUTTONS_LIST] container (bma)",BUTTONS_LIST);
    return false;
  }

  CGUIListItemPtr selectedButton = pContainer->GetSelectedItemPtr();
  if (!selectedButton.get())
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - FAILED to get the SelectedItem from container [%d=BUTTONS_LIST] container (bma)",BUTTONS_LIST);
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

    HandleClickOnBrowse();

    needToCloseDialog = true;
  }
  break;
  case CDialogButtons::BTN_RESOLVE:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - Handling click on [%s=%d=BTN_BROWSE] (bma)",selectedButtonLabel.c_str(),selectedButtonEnum);

    HandleClickOnResolve();

    needToCloseDialog = true;
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
      else if (activeWindow == WINDOW_HOME)
      {
        CGUIWindowBoxeeMain* pHomeWindow = (CGUIWindowBoxeeMain*)g_windowManager.GetWindow(WINDOW_HOME);
        if (!pHomeWindow)
        {
          CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::HandleClickOnButtonList - FAILED to get WINDOW_HOME. item [label=%s][path=%s][boxeeId=%s] (bma)",m_item.GetLabel().c_str(),m_item.m_strPath.c_str(),m_item.GetProperty("boxeeid").c_str());
        }
        else if (pHomeWindow->GetFocusedControlID() == LIST_QUEUE)
        {
          needToCloseDialog = true;
        }
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
  SET_CONTROL_FOCUS(BACK_GROUP,0);

  return true;
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnTrailer()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnTrailer - Enter function (bma)");

  bool succeeded = false;

  if (m_trailerItem.get())
  {
    if (m_trailerItem->IsInternetStream() && !g_application.IsConnectedToInternet())
    {
      CGUIDialogBoxeeNetworkNotification::ShowAndGetInput(53743,53746);
      return false;
    }

    m_trailerItem->Dump();

    succeeded = OnPlay(*m_trailerItem.get());
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
    g_application.m_guiDialogKaiToast.QueueNotification(g_localizeStrings.Get(53780), "", g_localizeStrings.Get(53731), 2000);

    CGUIMessage refreshQueueWinMsg(GUI_MSG_UPDATE, WINDOW_BOXEE_BROWSE_QUEUE, 0);
    g_windowManager.SendThreadMessage(refreshQueueWinMsg);

    CGUIMessage refreshHomeWinMsg(GUI_MSG_UPDATE, WINDOW_HOME, 0);
    g_windowManager.SendThreadMessage(refreshHomeWinMsg);

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
    g_application.m_guiDialogKaiToast.QueueNotification(g_localizeStrings.Get(53781), "", g_localizeStrings.Get(53732), 3000);

    CGUIMessage refreshQueueWinMsg(GUI_MSG_UPDATE, WINDOW_BOXEE_BROWSE_QUEUE, 0);
    g_windowManager.SendThreadMessage(refreshQueueWinMsg);

    CGUIMessage refreshHomeWinMsg(GUI_MSG_UPDATE, WINDOW_HOME, 0);
    g_windowManager.SendThreadMessage(refreshHomeWinMsg);

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

  BOXEE::Boxee::GetInstance().GetMetadataEngine().MarkAsWatched(m_item.m_strPath, m_item.GetProperty("boxeeid"), 0);

  m_item.SetProperty("watched", true);

  newLabelToUpdate = g_localizeStrings.Get(53714);
  newThumbToUpdate = g_localizeStrings.Get(53799);

  CGUIMessage winmsg(GUI_MSG_UPDATE, g_windowManager.GetActiveWindow(), 0);
  g_windowManager.SendThreadMessage(winmsg);

  return succeeded;
}

bool CGUIDialogBoxeeMediaAction::HandleClickOnMarkAsUnseen(CStdString& newLabelToUpdate, CStdString& newThumbToUpdate)
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnMarkAsUnseen - Enter function (bma)");

  bool succeeded = true;

  BOXEE::Boxee::GetInstance().GetMetadataEngine().MarkAsUnWatched(m_item.m_strPath, m_item.GetProperty("boxeeid"));

  m_item.SetProperty("watched", false);

  newLabelToUpdate = g_localizeStrings.Get(53713);
  newThumbToUpdate = g_localizeStrings.Get(53792);

  CGUIMessage winmsg(GUI_MSG_UPDATE, g_windowManager.GetActiveWindow(), 0);
  g_windowManager.SendThreadMessage(winmsg);

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

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnGotoShow - Enter function. Item ShowId is [%s] (bma)",showId.c_str());

  CStdString urlStr = "boxee://tvshows/episodes?local=true&remote=true&seriesId=" + showId;
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
bool CGUIDialogBoxeeMediaAction::HandleClickOnBrowse()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnBrowse - Enter function (bma)");

  bool succeeded = true;
  g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE, m_item.m_strPath);
  g_windowManager.CloseDialogs(true);

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnBrowse - Exit function and return [succeeded=%d] (bma)",succeeded);

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
        pDlgSelect->Add(localPathsList[i]->m_strPath);
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

  if (CGUIDialogBoxeeManualResolve::Show(unresolvedItem))
  {
    CGUIMessage winmsg(GUI_MSG_UPDATE, g_windowManager.GetActiveWindow(), 0);
    g_windowManager.SendThreadMessage(winmsg);
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::HandleClickOnResolve - Exit function and return [succeeded=%d] (bma)",succeeded);

  return succeeded;
}

int CGUIDialogBoxeeMediaAction::FillDialogButtons(bool itemHasLinks)
{
  CGUIMessage msg1(GUI_MSG_LABEL_RESET, GetID(), BUTTONS_LIST, 0);
  OnMessage(msg1);

  CGUIMessage msg2(GUI_MSG_LABEL_RESET, GetID(), TRAILER_LIST, 0);
  OnMessage(msg2);

  //m_item.Dump();

  /////////////////////
  // Trailer button //
  /////////////////////

  if (m_trailerItem.get() && (m_playButtonItemIndex > (-1)))
  {
    CFileItemPtr trailerButton(new CFileItem(g_localizeStrings.Get(53718)));
    trailerButton->SetThumbnailImage(g_localizeStrings.Get(53794));
    trailerButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_PLAY_TRAILER);

    CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), TRAILER_LIST, 0, 0, trailerButton);
    OnMessage(winmsg);

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Add TRAILER button. (bma)");
  }

  CFileItemList buttonsList;

  /////////////////////
  // ReadMore button //
  /////////////////////

  if (BoxeeUtils::HasDescription(m_item))
  {
    CFileItemPtr moreInfoButton(new CFileItem(g_localizeStrings.Get(53710)));
    moreInfoButton->SetThumbnailImage(g_localizeStrings.Get(53790));
    moreInfoButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_MORE_INFO);
    buttonsList.Add(moreInfoButton);

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Add MORE_INFO button. [ButtonsListSize=%d] (bma)",buttonsList.Size());
  }

  //////////////////////////
  // Queue/Dequeue button //
  //////////////////////////
  int activeWindow = g_windowManager.GetActiveWindow();

  // DVD Media Action Dialog doesnt need Queue/Dequeue button
  if (!m_item.IsDVD()) {
	  CGUIWindowBoxeeMain* pHomeWindow = (CGUIWindowBoxeeMain*)g_windowManager.GetWindow(WINDOW_HOME);
	  if (!pHomeWindow)
	  {
		  CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::FillDialogButtons - FAILED to get WINDOW_HOME. item [label=%s][path=%s][boxeeId=%s] (bma)",m_item.GetLabel().c_str(),m_item.m_strPath.c_str(),m_item.GetProperty("boxeeid").c_str());
	  }

	  CStdString referral;

	  bool canQueue = BoxeeUtils::CanQueue(m_item,referral);

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
	  else if ((activeWindow == WINDOW_HOME) && pHomeWindow && (pHomeWindow->GetFocusedControlID() == LIST_QUEUE))
	  {
		  ///////////////////////////////////////////////////////////////////////////
		  // in HomeWindow and click on QueueList -> need to add REMOVE_FROM_QUEUE //
		  ///////////////////////////////////////////////////////////////////////////

		  if (canQueue)
		  {
			  CLog::Log(LOGERROR,"CGUIDialogBoxeeMediaAction::FillDialogButtons - In HomeWindow, click on QueueItem and [canQueue=%d] for item [label=%s][path=%s][boxeeId=%s] (bma)",canQueue,m_item.GetLabel().c_str(),m_item.m_strPath.c_str(),m_item.GetProperty("boxeeid").c_str());
		  }

		  AddDequeueButton(buttonsList, referral);

		  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - In HomeWindow and click on QueueItem -> Add REMOVE_FROM_QUEUE button and set property [referral=%s]. [ButtonsListSize=%d] (bma)",m_item.GetProperty("referral").c_str(),buttonsList.Size());
	  }
	  else if (itemHasLinks)
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
			  AddDequeueButton(buttonsList, referral);

			  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Add REMOVE_FROM_QUEUE button and set property [referral=%s]. [ButtonsListSize=%d] (bma)",m_item.GetProperty("referral").c_str(),buttonsList.Size());
		  }
	  }
	  else
	  {
		  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Not adding queue/dequeue button. [activeWindow=%d][itemHasLinks=%d] for item [label=%s][path=%s][boxeeId=%s] (bma)",activeWindow,itemHasLinks,m_item.GetLabel().c_str(),m_item.m_strPath.c_str(),m_item.GetProperty("boxeeid").c_str());
	  }
  }

  ////////////////////////
  // Seen/Unseen button //
  ////////////////////////

  if (itemHasLinks)
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
        CFileItemPtr  shortcutButton(new CFileItem(g_localizeStrings.Get(53716)));
        shortcutButton->SetThumbnailImage(g_localizeStrings.Get(53795));
        shortcutButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_REMOVE_FROM_SHORTCUT);
        buttonsList.Add(shortcutButton);

        CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Add BTN_REMOVE_FROM_SHORTCUT button. [ButtonsListSize=%d] (bma)",buttonsList.Size());
      }
    }
  }

  //////////////////
  // Share screen //
  //////////////////

  if (BoxeeUtils::CanShare(m_item) )
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
  if ( m_item.IsDVD())
  {
	    CFileItemPtr browseButton(new CFileItem(g_localizeStrings.Get(53722)));
	    browseButton->SetThumbnailImage(g_localizeStrings.Get(53787));
	    browseButton->SetProperty(BUTTON_ACTION_PROPERTY_NAME,ACTION_BROWSE);
	    buttonsList.Add(browseButton);
	    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::FillDialogButtons - Add BTN_BROWSE button. [ButtonsListSize=%d] (bma)",buttonsList.Size());

  }

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

  /////////////////////////
  // manually    resolve //
  /////////////////////////
  if ( BoxeeUtils::CanRecognize(m_item))
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

    CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), BUTTONS_LIST, 0, 0, item);
    OnMessage(winmsg);
  }

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
  CGUIMessage winmsg1(GUI_MSG_LABEL_RESET, GetID(), 5000);
  g_windowManager.SendMessage(winmsg1);

  // reset the window property "link-path-to-show"
  SetLinkPathAsWindowProperty("");

  m_trailerItem.reset();
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
    return false;

  dialog->m_item = *pItem;
  dialog->DoModal();

  return dialog->m_bConfirmed;
}

bool CGUIDialogBoxeeMediaAction::OnBrowse(const CFileItem& item)
{
  if (item.m_bIsFolder)
  {
    if (item.GetPropertyBOOL("ispicturefolder") || item.HasPictureInfoTag())
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnBrowse - Going to open WINDOW_BOXEE_BROWSE_PHOTOS with [path=%s] (bma)(browse)",item.m_strPath.c_str());
      g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_PHOTOS, item.m_strPath);
    }
    else if (item.GetPropertyBOOL("IsAlbum"))
    {
      CStdString strPath;

      if (item.m_strPath.Left(17) == "boxeedb://tracks/" && item.m_strPath.length() > 17)
      {
        strPath = item.m_strPath;
      }
      else if (!item.GetProperty("BoxeeDBAlbumId").IsEmpty())
      {
        strPath = "boxeedb://tracks/";
        strPath += item.GetProperty("BoxeeDBAlbumId");
      }

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnBrowse - Going to open WINDOW_BOXEE_BROWSE_TRACKS with [path=%s] (bma)(browse)",strPath.c_str());
      g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_TRACKS, strPath);
    }
    else if (item.GetPropertyBOOL("isMusicFolder") || item.HasMusicInfoTag())
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnBrowse - Going to open WINDOW_BOXEE_BROWSE_LOCAL with [path=%s] (bma)(browse)",item.m_strPath.c_str());
      g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_LOCAL, item.m_strPath);
    }
    else
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnBrowse - Going to open WINDOW_BOXEE_BROWSE with [path=%s] (bma)(browse)",item.m_strPath.c_str());
      g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE, item.m_strPath);
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

    CStdString strPath = "boxeedb://tracks/";
    strPath += item.GetProperty("BoxeeDBAlbumId");
    g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_TRACKS, strPath);
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
  else if (item.IsLastFM()) 
  {
    AddItemToHistory(item);

    CLastFmManager::GetInstance()->StopRadio();
    CLastFmManager::GetInstance()->ChangeStation(CURL(item.m_strPath));
  }
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

      BoxeeUtils::AddTrailerStrToItemLabel(*((CFileItem*)trailerItem.get()));

      AddItemToHistory(*((CFileItem*)trailerItem.get()));

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::OnPlay - Going to play trailer file from folder mode. [path=%s][label=%s][thumb=%s] (tr)", (trailerItem->m_strPath).c_str(),(trailerItem->GetLabel()).c_str(),(trailerItem->GetThumbnailImage()).c_str());

      OnPlayMedia(*((CFileItem*)trailerItem.get()));
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

  if (!item.m_bIsFolder && !resumeItem)
  {
    // check to see whether we have a resume offset available
    BXUserProfileDatabase db;
    if (db.IsOpen())
    {
      double timeInSeconds;
      CStdString itemPath(item.m_strPath);

      if ( db.GetTimeWatchedByPath(itemPath , timeInSeconds) )
      { // prompt user whether they wish to resume
        std::vector<CStdString> choices;
        CStdString resumeString, time;
        StringUtils::SecondsToTimeString(lrint(timeInSeconds), time);
        resumeString.Format(g_localizeStrings.Get(12022).c_str(), time.c_str());
        choices.push_back(resumeString);
        choices.push_back(g_localizeStrings.Get(12021)); // start from the beginning
        int retVal = CGUIDialogContextMenu::ShowAndGetChoice(choices);

        if (!retVal)
        return false; // don't do anything

        resumeItem = (retVal == 1);
      }

    db.Close();
    }
  }

  AddItemToHistory(item);

  // We have to copy the item here because it was passed as const
  CFileItem newItem(item);
  if (resumeItem)
    newItem.m_lStartOffset = STARTOFFSET_RESUME;

  OnPlayMedia(newItem);

  return true;
}

bool CGUIDialogBoxeeMediaAction::GetPreferredQuality(const CFileItem& item, int &chosenItem)
{
  int quality = BOXEE::Boxee::GetInstance().GetMetadataEngine().GetProviderPerfQuality(item.GetProperty("link-providername"));
  // get the previous played quality from the database
  if (quality == MEDIA_DATABASE_ERROR)
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::GetPreferredQuality  provider [%s] doesnt have preffered quality ", item.GetProperty("link-providername").c_str());
    return false;
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::GetPreferredQuality provider [%s]  played before with quality %d ", item.GetProperty("link-providername").c_str(), quality);

  // the list is sorted in desc order - so in case the we are looking for an item with the same quality or the item with the closest quality that (less then the previous)
  const CFileItemList* linksFileItemList = item.GetLinksList();
  for (int i=0; i<linksFileItemList->Size(); i++)
  {
    CFileItemPtr linkFileItemToAdd = linksFileItemList->Get(i);
    if  (linkFileItemToAdd->GetPropertyInt("quality") <= quality)
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::found link with quality %d ", linkFileItemToAdd->GetPropertyInt("quality"));
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
    BOXEE::Boxee::GetInstance().GetMetadataEngine().AddProviderPerf(chosenFileItem->GetProperty("link-providername"),
                                                                    chosenFileItem->GetPropertyInt("quality"));
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

  if (PlayableItem.HasLinksList())
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
      pathLabel = g_localizeStrings.Get(15311);
      pathLabel += " ";
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

    g_application.m_guiDialogKaiToast.QueueNotification(g_localizeStrings.Get(53782), "", g_localizeStrings.Get(53737), 3000);
  }

  return succeeded;
}

bool CGUIDialogBoxeeMediaAction::RemoveShortcut(const CBoxeeShortcut& shortcut)
{
  bool succeeded = g_settings.GetShortcuts().RemoveShortcut(shortcut);

  if (succeeded)
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMediaAction::RemoveShortcut - ShortcutItem [name=%s][path=%s] was removed (bma)",shortcut.GetName().c_str(),shortcut.GetPath().c_str());

    g_application.m_guiDialogKaiToast.QueueNotification(g_localizeStrings.Get(53783), "", g_localizeStrings.Get(53739), 3000);

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

  if (!linkItem->GetProperty("link-countryrel").IsEmpty())
  {
	dstItem.SetProperty("link-countryrel",linkItem->GetProperty("link-countryrel"));
  }

  if (!linkItem->GetProperty("quality-lbl").IsEmpty())
  {
	dstItem.SetProperty("quality-lbl",linkItem->GetProperty("quality-lbl"));
  }

  dstItem.SetProperty("quality",linkItem->GetPropertyInt("quality"));
  dstItem.Dump();
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
