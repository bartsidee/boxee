#include "GUIDialogBoxeeSearch.h"
#include "GUIWindowManager.h"
#include "Application.h"
#include "GUIUserMessages.h"
#include "utils/log.h"
#include "FileSystem/Directory.h"
#include "Util.h"
#include "TimeUtils.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "Directory.h"
#include "LocalizeStrings.h"
#include "BoxeeUtils.h"
#include "FileSystem/BoxeeServerDirectory.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIToggleButtonControl.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIControlFactory.h"
#include "guilib/GUIMultiImage.h"
#include "GUIFontManager.h"
#include "BoxeeItemLauncher.h"
#include "GUIFixedListContainer.h"
#include "GUIDialogBoxeeBrowseMenu.h"

#define GLOBAL_SEARCH_SUGGESTIONS_GROUP 4000


#define GLOBAL_SEARCH_RESULTS_LIST      5200

#define SUGGESTION_LIST 4100
#define NUMBER_BUTTON   4200
#define LANGUAGE_BUTTON   33

#define CTL_LABEL_EDIT  310
#define CTL_IMG_LOADING 320

#define DEFAULT_CATEGORY_BUTTON_HEIGHT 64

#define KEYBOARD_DELAY 200

#define HIGHLIGHT_COLOR_GREEN   "green"
#define HIGHLIGHT_COLOR_RED     "red"
#define HIGHLIGHT_COLOR_YELLOW  "yellow"

#define SYMBOLS "0123456789!@,.;:\'\"?/\\#$&-_=+%^*[]()|"

#define SEARCH_ICON   "icons/icon_browser_no_favorite.png"
#define MOVIES_ICON   "icons/icon_browse_menu_video.png"
#define CLIPS_ICON    "icons/icon_browse_menu_clip.png"
#define SHOWS_ICON    "icons/icon_browse_menu_shows.png"
#define APPS_ICON     "icons/icon_browse_menu_apps.png"

using namespace XFILE;


// //////////////////////////////////////////////////////////////////////////////////////////

CStdString CSuggestionSource::GetUrl(const CStdString& strTerm)
{
  CStdString strLink = m_strBaseUrl;
  m_mapOptions["term"] = strTerm;

  strLink += BoxeeUtils::BuildParameterString(m_mapOptions);

  return strLink;
}

void CSuggestionSource::ApplyIcon(CFileItemList& list)
{
  if (!list.IsEmpty())
  {
    for (unsigned int i = 0 ; i < (unsigned int)list.Size(); ++i)
    {
      CFileItemPtr item = list.Get(i);
      item->SetProperty("icon",m_mediaIcon);
    }
  }
}

void CSuggestionMediaSource::ApplyIcon(CFileItemList& list)
{
  if (list.IsEmpty())
    return;

  for (unsigned int i = 0 ; i < (unsigned int)list.Size(); ++i)
  {
    CFileItemPtr item = list.Get(i);

    CStdString type = item->GetProperty("boxee-mediatype");

    if (type == "tv")
    {
      item->SetProperty("icon", SHOWS_ICON);
    }
    else if (type == "movie")
    {
      item->SetProperty("icon", MOVIES_ICON);
    }
    else if (type == "clip")
    {
      item->SetProperty("icon", CLIPS_ICON);
    }
  }
}

// START OF CSuggestionManager IMPLEMENTATION

CSuggestionManager::CSuggestionManager(ISuggestionCallback* callback) : m_suggestionProcessor(10000)
{
  m_callback = callback;
}

CSuggestionManager::~CSuggestionManager()
{

}

void CSuggestionManager::Start()
{
  m_vecSources.clear();
  m_suggestionProcessor.Start(1);
}

void CSuggestionManager::Stop()
{
  m_suggestionProcessor.SignalStop();
  m_http.Close();
}

void CSuggestionManager::AddSource(const CSuggestionSourcePtr& source)
{
  m_vecSources.push_back(source);
}

void CSuggestionManager::GetSuggestions(const CStdString& strTerm)
{
  m_suggestionProcessor.ClearQueue();
  m_suggestionProcessor.QueueJob(new CSuggestionManager::BXSuggestionJob(strTerm, this));
}

void CSuggestionManager::GetSuggestionsBG(const CStdString& _strTerm)
{
  CFileItemList suggestionList;

  CStdString strTerm = _strTerm;

  CUtil::URLEncode(strTerm);

  if (strTerm.IsEmpty())
  {
    // Send empty list to the callback
    suggestionList.SetProperty("term", strTerm);
    m_callback->OnSuggestions(suggestionList);
    return;
  }

  // Go over all sources
  for (int i = 0; i < (int)m_vecSources.size(); i++)
  {
    CFileItemList items;
    CSuggestionSourcePtr source = m_vecSources[i];
    CStdString strLink = source->GetUrl(strTerm);

    // Use get directory API
    CLog::Log(LOGDEBUG, "CSuggestionManager::GetSuggestionsBG, get suggestions from %s (search)", strLink.c_str());
    if (DIRECTORY::CDirectory::GetDirectory(strLink, items))
    {
      CLog::Log(LOGDEBUG, "CSuggestionManager::GetSuggestionsBG, got %d suggestions from %s (search)", items.Size(), strLink.c_str());
    }
    else
    {
      CLog::Log(LOGDEBUG, "CSuggestionManager::GetSuggestionsBG, could not get suggestions from %s (search)", strLink.c_str());
    }

    // Merge the retrieved result into the suggestion list
    source->ApplyIcon(items);
    MergeByLabel(items, suggestionList);
    items.Clear();
  }

  //suggestionList.Sort(SORT_METHOD_SEARCH_COUNT,SORT_ORDER_DESC);

  CStdString highlightColorPrefix = g_localizeStrings.Get(53954);
  CStdString highlightColorPostfix = g_localizeStrings.Get(53955);

  if (!highlightColorPrefix.IsEmpty() && !highlightColorPostfix.IsEmpty())
  {
    for (unsigned int i = 0 ; i < (unsigned int)suggestionList.Size() ; ++i)
    {
      CFileItemPtr highlightItem = suggestionList.Get(i);
      CStdString label = highlightItem->GetLabel();
      MarkSearchStringInSuggestion(label, _strTerm, highlightColorPrefix, highlightColorPostfix);
      highlightItem->SetProperty("highlightedLabel",label);
    }
  }

  // Pass the result to the callback
  suggestionList.SetProperty("term", _strTerm);

  if (suggestionList.IsEmpty())
  {
    //append "No Results Found." item
    CFileItemPtr emptyList (new CFileItem(g_localizeStrings.Get(51929)));
    emptyList->SetProperty("highlightedLabel",emptyList->GetLabel());
    emptyList->SetProperty("isseparator",true);
    suggestionList.Add(emptyList);
  }

  //append the search engine url item
  CStdString strSearchEngineURL = g_localizeStrings.Get(53956);
  CStdString strSearchEngineLabel = g_localizeStrings.Get(53957);

  strSearchEngineURL += _strTerm;
  strSearchEngineLabel.Format(strSearchEngineLabel,_strTerm);

  CFileItemPtr searchItem (new CFileItem(strSearchEngineURL));

  searchItem->SetProperty("type", "url");
  searchItem->SetProperty("link", strSearchEngineURL);
  searchItem->SetProperty("highlightedLabel",strSearchEngineLabel);
  searchItem->SetProperty("icon",SEARCH_ICON);

  searchItem->m_strPath = strSearchEngineURL;

  suggestionList.Add(searchItem);

  CLog::Log(LOGDEBUG, "CSuggestionManager::GetSuggestionsBG, total of %d suggestions  (search)", suggestionList.Size());

  m_callback->OnSuggestions(suggestionList);
}
/*
bool CSuggestionManager::ParseSuggestionXml(const CStdString& strHtml, CFileItemList& items)
{
  TiXmlDocument xmlDoc;
  xmlDoc.Parse(strHtml);

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (!pRootElement)
    return false;

  if (strcmpi(pRootElement->Value(), "tips") != 0)
  {
    CLog::Log(LOGERROR, "CSuggestionManager::ParseSuggestionXml, could not parse tips (bsd) (browse)");
    return false;
  }

  const TiXmlNode* pTag = 0;
  while ((pTag = pRootElement->IterateChildren(pTag)))
  {
    if (pTag->ValueStr() == "tip")
    {
      const TiXmlNode *pValue = pTag->FirstChild();

      CStdString strValue = pValue->ValueStr();
      CStdString strYear = ((TiXmlElement*)pTag)->Attribute("year");
      CStdString strType = ((TiXmlElement*)pTag)->Attribute("boxee_type");
      CStdString strSearchCount = ((TiXmlElement*)pTag)->Attribute("search_count");
      CStdString strBoxeeId = ((TiXmlElement*)pTag)->Attribute("boxee_id");
      CStdString strLabel;

      if (strYear.IsEmpty())
        strLabel.Format("%s", strValue.c_str());
      else
        strLabel.Format("%s (%s)", strValue.c_str(), strYear.c_str());

      CFileItemPtr tipItem (new CFileItem(strLabel));
      tipItem->SetProperty("type", strType);
      tipItem->SetProperty("year", strYear);
      tipItem->SetProperty("value", strValue);
      tipItem->SetProperty("searchCount", strSearchCount);
      // currently the server doesn't pass IsAdult -> set it to FALSE
      tipItem->SetAdult(false);

      tipItem->SetProperty("boxeeid",strBoxeeId);

      if (strType == "movie")
      {
        tipItem->SetProperty("ismovie",true);
      }
      else if (strType == "tv")
      {
        tipItem->SetProperty("showid",strBoxeeId);
        tipItem->SetProperty("ismovie",false);
        tipItem->SetProperty("istvshowfolder",true);
      }

      items.Add(tipItem);
    }
    else if (pTag->ValueStr() == "app")
    {
      const TiXmlNode *pValue = pTag->FirstChild();

      CStdString strValue = pValue->ValueStr();
      CStdString strSearchCount = ((TiXmlElement*)pTag)->Attribute("search_count");
      CStdString strAppId = ((TiXmlElement*)pTag)->Attribute("app_id");
      CStdString strLink;

      strLink.Format("app://%s/",strAppId.c_str());

      CFileItemPtr tipItem (new CFileItem(strValue));
      tipItem->SetProperty("appid", strAppId);
      tipItem->SetProperty("type","app");
      tipItem->SetProperty("isApp",true);
      tipItem->m_strPath = strLink;

      items.Add(tipItem);
    }
    else if (pTag->ValueStr() == "url")
    {
      const TiXmlNode *pValue = pTag->FirstChild();

      CStdString strLink = ((TiXmlElement*)pTag)->Attribute("link");
      CStdString strType = ((TiXmlElement*)pTag)->Attribute("boxee_type");
      CStdString strSearchCount = ((TiXmlElement*)pTag)->Attribute("search_count");

      CFileItemPtr tipItem (new CFileItem(pValue->ValueStr()));

      if (strType.IsEmpty())
      {
        strType = "link";
      }

      tipItem->SetProperty("type", strType);
      tipItem->SetProperty("link", strLink);

      tipItem->SetProperty("searchCount", strSearchCount);
      tipItem->m_strPath = strLink;

      if (strType == "clip")
      {
        tipItem->SetProperty("isClip",true);
      }
      else
      {
        tipItem->SetProperty("isLink",true);
      }

      // currently the server doesn't pass IsAdult -> set it to FALSE
      tipItem->SetAdult(false);

      items.Add(tipItem);
    }
  }

  return true;
}
*/
void CSuggestionManager::MergeByLabel(CFileItemList& left, CFileItemList& right)
{
  if (left.Size() == 0)
    return;

  if (right.Size() == 0)
  {
    right.Append(left);
    return;
  }

  for (int i = 0; i < left.Size(); i++)
  {
    CFileItemPtr leftItem = left[i];

    leftItem->SetProperty("type",left.GetPropertyBOOL("type"));
    leftItem->SetProperty("IsLocal",left.GetPropertyBOOL("IsLocal"));

    for (int j = 0; j < right.Size();j++)
    {
      CFileItemPtr rightItem = right[j];

      //merge if the content is from the same type, avoid pandora app merge with pandora movie
      if (rightItem->GetLabel() == leftItem->GetLabel() &&
          rightItem->GetProperty("boxeeid") == leftItem->GetProperty("boxeeid"))
      {
        // we found a match
        CLog::Log(LOGDEBUG, "CSuggestionManager::MergeByLabel, match found = %s (browse)", leftItem->GetLabel().c_str());

        // Get the links from the left item and add them to the right item
        const CFileItemList* linksFileItemList = leftItem->GetLinksList();

        if (linksFileItemList)
        {
          // Copy all links from local to the remote item
          for (int j = 0; j < linksFileItemList->Size(); j++)
          {
            CFileItemPtr link = linksFileItemList->Get(j);
            rightItem->AddLink(link->GetLabel(), link->m_strPath, link->GetContentType(true), CFileItem::GetLinkBoxeeTypeAsEnum(link->GetProperty("link-boxeetype")), "", "", "", "all", true, "", 0, CFileItem::GetLinkBoxeeOfferAsEnum(link->GetProperty("link-boxeeoffer")), link->GetProperty("link-productslist"));
          }
        }

        left.Remove(i); //remove the duplicate item
        i--; // set the loop back
        break; // from the right loop
      }
    }

    //we didn't find anything we can merge it with, just push it last in the list but before the urls for searching the web
    /*if (right.Size() > 2)
    {
      right.AddFront(leftItem, right.Size()-2);
      left.Remove(i);
      i--;
    }*/
  }

  right.Append(left);
}
/*
bool CSuggestionManager::BuildColorHighlight(CStdString& highlightColorPrefix, CStdString& highlightColorPostfix)
{
  CStdString color;
  CStdString colorOption = g_localizeStrings.Get(53950);

  if (colorOption == "0")
  {
    // no color needed
    CLog::Log(LOGDEBUG,"CSuggestionManager::BuildColorHighlight - [option=%s] so color won't be set (search)",colorOption.c_str());
    return false;
  }
  else if (colorOption == "1")
  {
    color = HIGHLIGHT_COLOR_GREEN;
  }
  else if (colorOption == "2")
  {
    color = HIGHLIGHT_COLOR_RED;
  }
  else if (colorOption == "3")
  {
    color = HIGHLIGHT_COLOR_YELLOW;
  }
  else
  {
    // not a valid option
    CLog::Log(LOGDEBUG,"CSuggestionManager::BuildColorHighlight - Color won't be set because [option=%s] isn't valid (search)",colorOption.c_str());
    return false;
  }

  highlightColorPrefix = "[B][COLOR ";

  highlightColorPrefix += color;
  highlightColorPrefix += "]";

  highlightColorPostfix = "[/COLOR][/B]";

  return true;
}
*/
void CSuggestionManager::MarkSearchStringInSuggestion(CStdString& suggestion, const CStdString& strTerm, const CStdString& highlightColorPrefix, const CStdString& highlightColorPostfix)
{
  CStdString strTermLower = strTerm;
  strTermLower.ToLower();

  CStdString suggestionLower = suggestion;
  suggestionLower.ToLower();

  int pos = suggestionLower.Find(strTermLower);

  while (pos != (-1))
  {
    suggestion.Insert(pos,highlightColorPrefix);
    suggestion.Insert((pos+(int)highlightColorPrefix.length()+(int)strTerm.length()),highlightColorPostfix);

    suggestionLower.Insert(pos,highlightColorPrefix);
    suggestionLower.Insert((pos+(int)highlightColorPrefix.length()+(int)strTerm.length()),highlightColorPostfix);

    pos += ((int)highlightColorPrefix.length()+(int)strTerm.length()+(int)highlightColorPostfix.length());

    pos = suggestionLower.Find(strTermLower,pos);
  }
}

CSuggestionManager::BXSuggestionJob::BXSuggestionJob(const CStdString& strTerm, CSuggestionManager* manager) : BXBGJob("BXSuggestionJob")
{
  m_manager = manager;
  m_strTerm = strTerm;
}

CSuggestionManager::BXSuggestionJob::~BXSuggestionJob()
{

}

void CSuggestionManager::BXSuggestionJob::DoWork()
{
  m_manager->GetSuggestionsBG(m_strTerm);
}


CGUIDialogBoxeeSearch::CGUIDialogBoxeeSearch(void)
: CGUIDialogKeyboard(WINDOW_DIALOG_BOXEE_SEARCH, "boxee_search.xml"), m_suggestionManager(this)//, m_sourceController(this)
{
  m_bAutoCompleteEnabled = true;
  m_delayCounter = 0;
  m_keyType = CAPS;
}

CGUIDialogBoxeeSearch::~CGUIDialogBoxeeSearch(void)
{

}

void CGUIDialogBoxeeSearch::InitSuggestions()
{
  // Add all suggestion urls to the suggestion manager
  CStdString strTipLink = BOXEE::BXConfiguration::GetInstance().GetStringParam("Boxee.Autocomplete","http://res.boxee.tv");

  std::map<CStdString, CStdString> mapRemoteOptions;
  DIRECTORY::CBoxeeServerDirectory::AddParametersToRemoteRequest(mapRemoteOptions);

  //by order of appearance
  CSuggestionSourcePtr appSource(new CSuggestionSource());
  appSource->m_mapOptions = mapRemoteOptions;
  appSource->m_strBaseUrl = "appbox://all/";//strTipLink + "/titles/searchtip/";
  appSource->m_mediaIcon = APPS_ICON;
  appSource->m_contentType = "apps";
  m_suggestionManager.AddSource(appSource);

  CSuggestionSource* localMoviesSource = new CSuggestionSource;
  localMoviesSource->m_bRemote = false;
  localMoviesSource->m_mediaIcon = MOVIES_ICON;
  localMoviesSource->m_strBaseUrl = "boxeedb://movies/";
  localMoviesSource->m_contentType = "movie";
  m_suggestionManager.AddSource(CSuggestionSourcePtr(localMoviesSource));

  CSuggestionSource* localShowsSource = new CSuggestionSource;
  localShowsSource->m_bRemote = false;
  localShowsSource->m_mediaIcon = SHOWS_ICON;
  localShowsSource->m_strBaseUrl = "boxeedb://tvshows/";
  localShowsSource->m_contentType = "tv";
  m_suggestionManager.AddSource(CSuggestionSourcePtr(localShowsSource));

  CSuggestionMediaSource* serverSource = new CSuggestionMediaSource;
  serverSource->m_bRemote = true;
  serverSource->m_mapOptions = mapRemoteOptions;
  serverSource->m_moviesIcon = MOVIES_ICON;
  serverSource->m_showsIcon = SHOWS_ICON;
  serverSource->m_clipsIcon = CLIPS_ICON;
  serverSource->m_strBaseUrl = "boxee://search/";//strTipLink + "/titles/searchtip/";
  serverSource->m_contentType = "online";
  m_suggestionManager.AddSource(CSuggestionSourcePtr(serverSource));

}

void CGUIDialogBoxeeSearch::OnInitWindow()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeSearch::OnInitWindow - Enter function with [m_strType=%s] (search)",m_strType.c_str());

  m_bClosedByMovingRightFromTextBox = false;
  m_delayCounter = 0;

  m_strSearchTerm = "";
  m_strSuggestion = "";
  m_canSendQueryToServer = true;

  m_suggestionManager.Start();
  InitSuggestions();

  CGUIDialogKeyboard::OnInitWindow();
  UpdateButtons();

}

void CGUIDialogBoxeeSearch::OnDeinitWindow(int nextWindowID)
{
  m_suggestionManager.Stop();
  CGUIDialogKeyboard::OnDeinitWindow(nextWindowID);
}

bool CGUIDialogBoxeeSearch::OnAction(const CAction &action)
{
  if (GetFocusedControl() && GetFocusedControl()->GetID() == CTL_LABEL_EDIT && action.id == ACTION_MOVE_RIGHT)
  {
    // Check if we are on the last letter of the textbox
    CGUIEditControl* pEditControl = (CGUIEditControl*)GetControl(CTL_LABEL_EDIT);
    if (pEditControl && (pEditControl->GetCursorPosition() >= (size_t) pEditControl->GetLabel2().GetLength()))
    {
      m_bClosedByMovingRightFromTextBox = true;
      Close();
      return true;
    }
  }

  return CGUIDialogKeyboard::OnAction(action);
}

bool CGUIDialogBoxeeSearch::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_WINDOW_INIT)
  {
    CStdString param = message.GetStringParam();
    m_strSearchTerm = param;

    bool bResult = CGUIDialogKeyboard::OnMessage(message);

    CGUIEditControl* pEdit = ((CGUIEditControl*)GetControl(CTL_LABEL_EDIT));
    if (pEdit)
    {
      CStdString searchTerm = ((CGUIDialogBoxeeBrowseMenu*) g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_BROWSE_MENU))->GetSearchTerm();
      pEdit->SetLabel2(searchTerm);
    }

    return bResult;
  }
  else if (message.GetMessage() == GUI_MSG_LABEL_BIND)
  {
    SetProperty("loading", false);
    if (message.GetPointer() && message.GetControlId() == 0)
    {
      CFileItemList *items = (CFileItemList *)message.GetPointer();
      if (items)
      {
        if (items->HasProperty("searchtips"))
        {
          CGUIMessage message2(GUI_MSG_LABEL_BIND, GetID(), SUGGESTION_LIST, 0, 0, items);
          CGUIDialogKeyboard::OnMessage(message2);
        }/*
        else
        {
          BindSearchResults(items);
        }*/

        delete items;
      }
      return true;
    }
    else
    {
      return CGUIDialogKeyboard::OnMessage(message);
    }
  }
  else if (message.GetMessage() == GUI_MSG_ITEM_LOADED)
  {
    // We don't handle a broadcast message that was sent from this window in order to avoid loop.
    if((message.GetSenderId() == 0) && (message.GetControlId() == 0) && (message.GetParam1() == GetID()))
    {
      return true;
    }

    if (message.GetControlId() == 0)
    {
      CFileItem *pItem = (CFileItem *)message.GetPointer();
      message.SetPointer(NULL);

      if (pItem)
      {
        CLog::Log(LOGDEBUG, "CGUIDialogBoxeeSearch::OnMessage, got update for item %s (search)", pItem->GetLabel().c_str());

        // Forward the item loaded message to the underlying list
        int windowId = GetID();
        CGUIMessage winmsg(GUI_MSG_ITEM_LOADED, windowId, GLOBAL_SEARCH_RESULTS_LIST, 0, 0);
        winmsg.SetPointer(new CFileItem(*pItem));
        g_windowManager.SendThreadMessage(winmsg, windowId);

        delete pItem;
      }
    }
    else
    {
      // Message to a specific container
      return CGUIDialogKeyboard::OnMessage(message);
    }

    return true;
  }

  bool bResult = CGUIDialogKeyboard::OnMessage(message);

  if (bResult && message.GetMessage() == GUI_MSG_CLICKED)
  {
    // Check if the click comes from the suggestion list
    if (message.GetSenderId() == SUGGESTION_LIST)
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeSearch::OnMessage, suggestion list clicked (search)");
      CGUIBaseContainer* subMenuList = (CGUIBaseContainer*)GetControl(SUGGESTION_LIST);
      CGUIListItemPtr selectedListItem = subMenuList->GetSelectedItemPtr();

      if (selectedListItem.get())
      {
        CFileItem* pItem = (CFileItem*)selectedListItem.get();

        if (pItem)
        {
          CStdString strType = selectedListItem->GetProperty("type");
          if (strType == "url")
          {
            Close();
            pItem->SetProperty("url_source", "browser-app");
            g_application.PlayMedia(*pItem);
            return true;
          }
          else
          {
            pItem->Dump();
            if (CBoxeeItemLauncher::Launch(*pItem))
            {
              Close();
              return true;
            }
          }
        }
      }
    }
    else if (message.GetSenderId() == LANGUAGE_BUTTON)
    {
      // When pressing a language button, reset the numeric button state
      CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), NUMBER_BUTTON);
      g_windowManager.SendMessage(msg);
    }
    else if (message.GetSenderId() == NUMBER_BUTTON)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), NUMBER_BUTTON);
      g_windowManager.SendMessage(msg);

      if (msg.GetParam1())
      {
        char szLabel[2];
        szLabel[0] = 32;
        szLabel[1] = 0;
        CStdString aLabel = szLabel;

        // set numerals
        for (int iButton = 65; iButton <= 100; iButton++)
        {
          aLabel[0] = SYMBOLS[iButton - 65];
          SetControlLabel(iButton, aLabel);
          SET_CONTROL_VISIBLE(iButton);
        }

        HideShowButtonLines();
      }
      else
      {
        UpdateButtons();
      }
    }
    else
    {
      m_delayCounter = 0;
      m_strSuggestion.clear();

      // a change was made -> can query the server again
      m_canSendQueryToServer = true;
    }
  }

  return bResult;
}

void CGUIDialogBoxeeSearch::OnOK()
{
  const CGUIEditControl* pEdit = ((const CGUIEditControl*)GetControl(CTL_LABEL_EDIT));
  if (pEdit)
  {
    if (m_strEdit.Equals(pEdit->GetUnicodeLabel()) && !m_canSendQueryToServer)
    {
      // same query and no change was made -> don't query the server
      return;
    }

    m_strEdit = pEdit->GetUnicodeLabel();
    m_strEdit.Trim();
  }

  if (!m_strEdit.IsEmpty())
  {
    if (HandleSearchingUrl(m_strEdit))
    {
      Close();
      return;
    }

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeSearch::OnOK - [strEdit=%ls] ISN'T URL (search)", m_strEdit.c_str());

    // going to query the server -> don't query again unless a change will be made
    m_canSendQueryToServer = false;

  }
  else
  {
    Close();
  }
}

void CGUIDialogBoxeeSearch::Render()
{
  CStdString currentText = GetCurrentText();
  currentText.Trim();

  if (m_delayCounter > 15 && (m_strSearchTerm != currentText || currentText.IsEmpty()))
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeSearch::Render, search for term = %s (search)", currentText.c_str());

    m_delayCounter = 0;

    m_strSearchTerm = currentText;

    if (m_strSearchTerm.IsEmpty())
    {
      CGUIDialogKeyboard::Render();
      Close();
      return;
    }

    SetProperty("loading", true);
    m_suggestionManager.GetSuggestions(m_strSearchTerm);
  }
  else
  {
    m_delayCounter++;
  }

  CGUIDialogKeyboard::Render();
}

void CGUIDialogBoxeeSearch::SetAutoCompleteEnabled(bool bEnabled)
{
  m_bAutoCompleteEnabled = bEnabled;
}

void CGUIDialogBoxeeSearch::SetType(const CStdString& strType)
{
  m_strType = strType;
}

CStdString CGUIDialogBoxeeSearch::GetText() const
{
  if (m_strSuggestion.IsEmpty())
    return CGUIDialogKeyboard::GetText();
  else
    return m_strSuggestion;
}

void CGUIDialogBoxeeSearch::OnSuggestions(CFileItemList& suggestionList)
{
  //SET_CONTROL_HIDDEN(GLOBAL_SEARCH_RESULTS_GROUP);
  SET_CONTROL_VISIBLE(GLOBAL_SEARCH_SUGGESTIONS_GROUP);

  CFileItemList* pNewList = new CFileItemList();
  pNewList->Copy(suggestionList);
  pNewList->SetProperty("term", suggestionList.GetProperty("term"));
  pNewList->SetProperty("searchtips", true);

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), 0, 0, 0, pNewList);
  g_windowManager.SendThreadMessage(msg, GetID());
}

bool CGUIDialogBoxeeSearch::HandleSearchingUrl(const CStdString& url)
{
  CStdString searchText;
  g_charsetConverter.wToUTF8(url, searchText);

  CLog::Log(LOGDEBUG, "CGUIDialogBoxeeSearch::HandleSearchingUrl - enter function with [strEdit=%ls][searchText=%s] (search)",m_strEdit.c_str(),searchText.c_str());

  bool hasDot = (searchText.Find(".") != -1);
  bool hasSpace = (searchText.Find(" ") != -1);

  if (!hasDot || hasSpace)
  {
    CLog::Log(LOGDEBUG, "CGUIDialogBoxeeSearch::HandleSearchingUrl - [searchText=%s] ISN'T url (search)",searchText.c_str());
    return false;
  }

  CLog::Log(LOGDEBUG, "CGUIDialogBoxeeSearch::HandleSearchingUrl - [searchText=%s] is url (search)",searchText.c_str());

  CFileItem urlItem(searchText);
  if (!CUtil::IsHTTP(searchText))
  {
    urlItem.m_strPath = "http://";
  }
  urlItem.m_strPath += searchText;
  urlItem.SetContentType("text/html");

  urlItem.SetProperty("url_source", "browser-app");

  g_application.PlayMedia(urlItem);

  return true;
}

void CGUIDialogBoxeeSearch::Close(bool forceClose)
{
  CGUIMessage message(GUI_MSG_LABEL2_SET, WINDOW_DIALOG_BOXEE_BROWSE_MENU, BROWSE_MENU_BUTTON_SEARCH);
  CStdString searchTerm = GetSearchTerm();
  message.SetLabel(searchTerm);
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeSearch::Close - enter function. sending GUI_MSG_LABEL2_SET with [searchTerm=%s] to BUTTON_SEARCH in BROWSE_MENU (search)",searchTerm.c_str());
  g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_BROWSE_MENU)->OnMessage(message);
  m_canSendQueryToServer = true;

  return CGUIDialogKeyboard::Close(forceClose);
}
