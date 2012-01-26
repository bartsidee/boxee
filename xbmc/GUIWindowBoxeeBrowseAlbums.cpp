
#include "GUIWindowBoxeeBrowseAlbums.h"
#include "FileSystem/BoxeeServerDirectory.h"
#include "FileSystem/BoxeeDatabaseDirectory.h"
#include "GUIWindowManager.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "URL.h"
#include "Util.h"
#include "BoxeeUtils.h"
#include "GUIWindowBoxeeBrowseTracks.h"
#include "GUIDialogBoxeeDropdown.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIWindowManager.h"
#include "GUIDialogBoxeeMainMenu.h"
#include "MusicInfoTag.h"
#include "FileSystem/Directory.h"
#include "Application.h"
#include "GUIDialogSelect.h"
#include "lib/libBoxee/bxutils.h"
#include "bxaudiodatabase.h"
#include "lib/libBoxee/boxee.h"
#include "SpecialProtocol.h"
#include "BoxeeDatabaseDirectory.h"
#include "GUISettings.h"
#include "GUIDialogBoxeeBrowseMenu.h"

using namespace std;
using namespace BOXEE;

#define BUTTON_ARTISTS  120
#define BUTTON_ALBUMS  130
#define BUTTON_GENRES  150

// This button is only shown in case no albums present
// and user is suggested to browse apps instead
#define BUTTON_SHOW_APPS  131
#define BUTTON_BROWSE_SOURCES 7002

#define STATE_SHOW_ALL_ALBUMS    "albums"
#define STATE_SHOW_ALL_ARTISTS   "artists"
#define STATE_SHOW_ARTIST_ALBUMS "artistsalbum"

#define SWITCH_VIEW_THUMBS   8001
#define SWITCH_VIEW_LIST   8002
#define SWITCH_VIEW_FLAG "show-thumbnails"

#define GENRE_FILTER_ID   600
#define GENRE_FILTER_NAME "genre"

#define ALBUMS_ARTIST_FLAG        "albums-or-artists"

#define SHOW_FILTERS_AND_SORT     9014
#define SHOW_FILTERS_AND_SORT_FLAG "filters-and-sort"

#define ITEM_SUMMARY    9018
#define ITEM_SUMMARY_FLAG "item-summary"

#define INIT_SELECT_POS_IN_BROWSE_MENU 1

// STATE IMPLEMENTATION
CAlbumsSource::CAlbumsSource(int iWindowID) : CBrowseWindowSource("albumsource", "boxeedb://albums/",iWindowID)
{
}

CAlbumsSource::~CAlbumsSource()
{

}

void CAlbumsSource::BindItems(CFileItemList& items)
{
  CStdString strGenre;
  CBrowseWindowAlbumGenreFilter* filter;
  std::map<CStdString, CStdString>::iterator it = m_mapFilters.find("genre");

  if (it != m_mapFilters.end())
  {
    strGenre = it->second;

    if (strGenre != "")
    {
      filter = new CBrowseWindowAlbumGenreFilter(GENRE_FILTER_ID, GENRE_FILTER_NAME, strGenre);

      int i = 0;

      while (i < items.Size())
      {
        if (filter->Apply(&*items[i]) != true)
        {//need to remove this item
          items.Remove(i);
        }
        else
        {
          i++;
        }
      }

      delete filter;
    }

  }

  return CBrowseWindowSource::BindItems(items);
}

CArtistsSource::CArtistsSource(int iWindowID) : CBrowseWindowSource("artistsource", "boxeedb://artists/",iWindowID)
{
}

CArtistsSource::~CArtistsSource()
{

}

CArtistSource::CArtistSource(const CStdString& path ,int iWindowID) : CBrowseWindowSource("selectedartistsource", path ,iWindowID)
{
}

CArtistSource::~CArtistSource()
{

}

void CAlbumsWindowState::SetDefaultCategory()
{
  SetCategory(STATE_SHOW_ALL_ARTISTS);
}

void CAlbumsWindowState::SetCategory(const CStdString &strCategory)
{
  m_sourceController.ActivateAllSources(false,true);

  if (strCategory.CompareNoCase(STATE_SHOW_ALL_ALBUMS) == 0 )
  {
    OnAlbums();
  }
  else if (strCategory.CompareNoCase(STATE_SHOW_ALL_ARTISTS) == 0)
  {
    OnArtists();
  }

  CBrowseWindowState::SetCategory(strCategory);
}

CAlbumsWindowState::CAlbumsWindowState(CGUIWindowBoxeeBrowse* pWindow) : CBrowseWindowState(pWindow)
{
  m_sourceController.RemoveAllSources();

  m_sourceController.AddSource(new CArtistsSource(m_pWindow->GetID()));
  m_sourceController.AddSource(new CAlbumsSource(m_pWindow->GetID()));
  m_sourceController.AddSource(new CArtistSource("", m_pWindow->GetID()));
  
  m_strGenre = "all";
  bool IgnorePrefix = g_guiSettings.GetBool("sort.showstarter");

  // Initialize sort vector
  m_vecSortMethods.clear();
  m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_ATOZ, IgnorePrefix?SORT_METHOD_LABEL_IGNORE_THE:SORT_METHOD_LABEL, SORT_ORDER_ASC , g_localizeStrings.Get(53505), ""));
  m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_RELEASE, SORT_METHOD_DATE_ADDED, SORT_ORDER_DESC, g_localizeStrings.Get(53539), ""));

  // Reset selected item
  m_iSelectedArtist = -1;
  m_iSavedView = MUSIC_THUMB_VIEW;
}

bool CAlbumsWindowState::OnBack()
{
  CStdString category = GetCategory();

  if (category.CompareNoCase(STATE_SHOW_ARTIST_ALBUMS) == 0 )
  {
    SetCategory(STATE_SHOW_ALL_ARTISTS);
    SetSelectedItem(m_iSelectedArtist);
    SetCurrentView(m_iSavedView);
    Refresh(false);
  }
  else
  {
    CGUIDialogBoxeeBrowseMenu* pMenu = (CGUIDialogBoxeeBrowseMenu*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_BROWSE_MENU);
    pMenu->DoModal();
  }

  return true;
}

bool CAlbumsWindowState::OnAlbums()
{
  m_sourceController.ActivateSource("albumsource",true,true);
  m_pWindow->SetProperty(ALBUMS_ARTIST_FLAG, 0);
  return true;
}

bool CAlbumsWindowState::OnArtists()
{
  m_iSelectedItem = m_iSelectedArtist;
  m_sourceController.ActivateSource("artistsource",true,true);
  m_pWindow->SetProperty(ALBUMS_ARTIST_FLAG, 1);
  return true;
}

bool CAlbumsWindowState::OnArtist(CFileItem& artistItem)
{

  bool bRetVal = false;
  m_strArtist = artistItem.GetLabel();
  CBrowseWindowSource* source = NULL;

  source = m_sourceController.GetSourceById("selectedartistsource");

  if (source != NULL)
  {
    m_iSavedView = GetCurrentView(); //save the current view before applying the new one in SetCategory

    SetCategory(STATE_SHOW_ARTIST_ALBUMS); //once its done, previous view and sort is loaded
    source->SetBasePath( artistItem.m_strPath );
    source->Activate(true);
    source->Reset();
    bRetVal = true;
    m_pWindow->SetProperty(ALBUMS_ARTIST_FLAG, 2);

    //SetDefaultView(); //should set MUSIC_THUMB_VIEW
  }
  
  return bRetVal;
}

void CAlbumsWindowState::SetDefaultView()
{
  m_iCurrentView = MUSIC_THUMB_VIEW;
}

void CAlbumsWindowState::SetArtist(const CStdString& strArtist)
{
  m_strArtist = strArtist;
}

void CAlbumsWindowState::SetGenre(const CStdString& strGenre)
{
  m_strGenre = strGenre;

  //RemoveLocalFilter(GENRE_FILTER_NAME);

  if (m_strGenre.CompareNoCase("all") != 0)
  {
    m_pWindow->SetProperty("genre-label", m_strGenre);
    //AddLocalFilter(new CBrowseWindowAlbumGenreFilter(GENRE_FILTER_ID, GENRE_FILTER_NAME, m_strGenre));
    m_sourceController.SetFilter("genre", m_strGenre);
  }
  else
  {
    m_pWindow->SetProperty("genre-label", ""); // reset genre
    m_sourceController.ClearFilter("genre");
  }
}


void CAlbumsWindowState::Refresh(bool bResetSelected)
{
  CBrowseWindowState::Refresh(bResetSelected);

  m_pWindow->SetProperty(ITEM_SUMMARY_FLAG, GetItemSummary());
}

CStdString CAlbumsWindowState::GetItemSummary()
{
  CStdString strSummary = "";
  std::map<CStdString , CStdString> mapTitleItemValue;

  if (!m_sort.m_sortName.empty() && m_sort.m_id != VIEW_SORT_METHOD_ATOZ)
  {
    mapTitleItemValue["sort"] = g_localizeStrings.Get(90006) + " " + m_sort.m_sortName; //prefix + " " + sort name
  }

  if (m_strGenre.CompareNoCase("all") != 0 && GetCategory() == STATE_SHOW_ALL_ALBUMS)
  {
    CStdString strGenre = m_strGenre.ToLower();

    strGenre[0] = toupper(strGenre[0]);
    strGenre.Replace("_"," ");

    mapTitleItemValue["filter"] = strGenre;
  }

  if (GetCategory() == STATE_SHOW_ALL_ALBUMS)
  {
    mapTitleItemValue["media"] = g_localizeStrings.Get(82006);
  }
  else if (GetCategory() == STATE_SHOW_ALL_ARTISTS)
  {
    mapTitleItemValue["media"] = g_localizeStrings.Get(82005);
  }
  else if (GetCategory() == STATE_SHOW_ARTIST_ALBUMS)
  {
    mapTitleItemValue["media"] = g_localizeStrings.Get(82003) + m_strArtist;
  }

  if (!CUtil::ConstructStringFromTemplate(g_localizeStrings.Get(90003), mapTitleItemValue,strSummary))
  {
    strSummary = g_localizeStrings.Get(2);
    CLog::Log(LOGERROR,"CAlbumsWindowState::GetItemSummary, Error in Strings.xml for the current language [id=90003], the template is bad. (browse)");
  }

  return strSummary;
}

// WINDOW IMPLEMENTATION

CGUIWindowBoxeeBrowseAlbums::CGUIWindowBoxeeBrowseAlbums() : CGUIWindowBoxeeBrowse(WINDOW_BOXEE_BROWSE_ALBUMS, "boxee_browse_music.xml")
{
  SetWindowState(new CAlbumsWindowState(this));
  
  SetProperty(SHOW_FILTERS_AND_SORT_FLAG, false);
}

CGUIWindowBoxeeBrowseAlbums::~CGUIWindowBoxeeBrowseAlbums()
{

}

void CGUIWindowBoxeeBrowseAlbums::ShowItems(CFileItemList& list, bool append)
{
  CGUIWindowBoxeeBrowse::ShowItems(list,append);

  SetProperty(SWITCH_VIEW_FLAG, (m_windowState->GetCurrentView() != MUSIC_THUMB_VIEW));
}

bool CGUIWindowBoxeeBrowseAlbums::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == SWITCH_VIEW_THUMBS || iControl == SWITCH_VIEW_LIST)
      {
        SetProperty(SWITCH_VIEW_FLAG, !GetPropertyBOOL(SWITCH_VIEW_FLAG));
        return true;
      }
      if (iControl == BUTTON_SHOW_APPS)
      {
        g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_APPS , "boxeeui://apps/?category=all&categoryfilter=9");
        return true;
      }
    }
  }
  return CGUIWindowBoxeeBrowse::OnMessage(message);
}

void CGUIWindowBoxeeBrowseAlbums::ConfigureState(const CStdString& param)
{
  CGUIWindowBoxeeBrowse::ConfigureState(param);

   std::map<CStdString, CStdString> optionsMap;
   CURI properties(param);


   if (properties.GetProtocol().compare("boxeeui") == 0)
   {
     optionsMap = properties.GetOptionsAsMap();

     CStdString strGenre = "all";
     if (optionsMap.find("genre") != optionsMap.end())
     {
       strGenre = optionsMap["genre"];
     }

     ((CAlbumsWindowState*)m_windowState)->SetGenre(strGenre);
   }
}

bool CGUIWindowBoxeeBrowseAlbums::OnClick(int iItem)
{
  CFileItem item;
  if (!GetClickedItem(iItem, item))
    return true;

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseAlbums::OnClick, item no = %d, label = %s (browse)", iItem, item.GetLabel().c_str());

  item.Dump();

  if (item.GetPropertyBOOL("isalbum"))
  {
    CStdString strAlbum = item.GetMusicInfoTag()->GetAlbum();
    strAlbum = BXUtils::URLEncode(strAlbum);

    CStdString strArtist = item.GetMusicInfoTag()->GetArtist().IsEmpty() ? item.GetMusicInfoTag()->GetAlbumArtist() :item.GetMusicInfoTag()->GetArtist();
    strArtist = BXUtils::URLEncode(strArtist);

    // Construct album path TODO: Check why cant we use the id here
    CStdString strAlbumPath;
    strAlbumPath.Format("boxeedb://album/?title=%s&artist=%s", strAlbum.c_str(), strArtist.c_str());

    // Get all album instances from the database
    CFileItemList albumItems;
    CFileItemList availableAlbums;
    DIRECTORY::CDirectory::GetDirectory(strAlbumPath, albumItems);

    // Create a list of available instances, by checking whether the album folder path is available
    for (int i = 0; i < albumItems.Size(); i++)
    {
      CStdString strPath = albumItems.Get(i)->GetProperty("AlbumFolderPath");
      if (g_application.IsPathAvailable(strPath, true))
      {
        availableAlbums.Add(albumItems.Get(i));
      }
    }

    // Present the user with a list of instances to choose from
    CStdString strAlbumId;
    if (availableAlbums.Size() > 1)
    {
      // with SELECT DIALOG
      CGUIDialogSelect *pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);

      pDlgSelect->SetHeading(396); //"Select Location"
      pDlgSelect->Reset();

      for (int i = 0; i < availableAlbums.Size(); i++)
      {
        CStdString path = availableAlbums.Get(i)->GetProperty("AlbumFolderPath");
        if (!path.IsEmpty())
        {
          CUtil::RemovePasswordFromPath(path);
          CUtil::UrlDecode(path);
          pDlgSelect->Add(path);
        }
        else
        {
          CLog::Log(LOGWARNING,"CGUIWindowBoxeeBrowseAlbums::OnClick - [%d/%d] - received an EMPTY album folder path [%s] (browse)",i+1,availableAlbums.Size(),path.c_str());
        }
      }

      pDlgSelect->EnableButton(TRUE);
      pDlgSelect->SetButtonLabel(222); //'Cancel' button returns to weather settings
      pDlgSelect->DoModal();

      int selectedIndex = pDlgSelect->GetSelectedLabel();
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseAlbums::OnClick - CGUIDialogSelect returned [selectedIndex=%d] (browse)",selectedIndex);

      if (selectedIndex >= 0)
      {
        strAlbumId = availableAlbums.Get(pDlgSelect->GetSelectedLabel())->GetProperty("BoxeeDBAlbumId");
      }

    }
    else if (availableAlbums.Size() == 1)
    {
      strAlbumId = availableAlbums.Get(0)->GetProperty("BoxeeDBAlbumId");
    }
    else
    {
      // this should not happen
      CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseAlbums::OnClick, no available albums to play (browse)");
      return false;
    }

    // If album was selected, activate the tracks window for this album
    if (!strAlbumId.IsEmpty())
    {
      g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_TRACKS, strAlbumId);
    }

    return true;
  }
  else if (item.GetPropertyBOOL("isartist"))
  {
    // Update state in order to be able to return to correct result

    ((CAlbumsWindowState*)m_windowState)->m_iSelectedArtist = iItem;
    ((CAlbumsWindowState*)m_windowState)->OnArtist(item);
    Refresh();

    return true;
  }

  return CGUIWindowBoxeeBrowse::OnClick(iItem);
}

void CGUIWindowBoxeeBrowseAlbums::GetStartMenusStructure(std::list<CFileItemList>& browseMenuLevelList)
{
  CStdString category = m_windowState->GetCategory();

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseAlbums::GetStartMenusStructure - enter function [category=%s] (bm)",category.c_str());

  if (category == STATE_SHOW_ALL_ALBUMS)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvShows::GetStartMenusStructure - handle [category=%s=albums] -> set [m_initSelectPosInBrowseMenu=%d=0] menu (bm)",category.c_str(),m_initSelectPosInBrowseMenu);
    m_initSelectPosInBrowseMenu = 1;
  }
  else if (category == STATE_SHOW_ALL_ARTISTS)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvShows::GetStartMenusStructure - handle [category=%s=artists] -> set [m_initSelectPosInBrowseMenu=%d=1] (bm)",category.c_str(),m_initSelectPosInBrowseMenu);
    m_initSelectPosInBrowseMenu = 0;
  }

  CBoxeeBrowseMenuManager::GetInstance().GetFullMenuStructure("mn_local_music_categories",browseMenuLevelList);

  //m_initSelectPosInBrowseMenu = INIT_SELECT_POS_IN_BROWSE_MENU;

  //CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseAlbums::GetStartMenusStructure - exit function with [browseMenuLevelStackSize=%zu]. [category=%s] (bm)",browseMenuLevelStack.size(),category.c_str());

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseAlbums::GetStartMenusStructure - after set [browseMenuLevelListSize=%zu][m_initSelectPosInBrowseMenu=%d]. [category=%s] (bm)",browseMenuLevelList.size(),m_initSelectPosInBrowseMenu,category.c_str());

  return CGUIWindowBoxeeBrowse::GetStartMenusStructure(browseMenuLevelList);
}

