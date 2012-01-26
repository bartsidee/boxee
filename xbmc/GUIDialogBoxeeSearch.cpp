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

#define SUGGESTION_LIST 4100
#define NUMBER_BUTTON   4200
#define LANGUAGE_BUTTON   33
#define FIRST_LINE 351
#define LAST_LINE  356

#define KEYBOARD_DELAY 200

#define HIGHLIGHT_COLOR_GREEN   "green"
#define HIGHLIGHT_COLOR_RED     "red"
#define HIGHLIGHT_COLOR_YELLOW  "yellow"

#define SYMBOLS "0123456789!@,.;:\'\"?/\\#$&-_=+%^*[]()|"

using namespace XFILE;

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
  m_suggestionProcessor.Stop();
  m_http.Close();
}

void CSuggestionManager::AddSource(const CStdString& strSource)
{
  m_vecSources.push_back(strSource);
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
    CStdString strLink = m_vecSources[i];
    strLink += strTerm;

    CFileItemList items;

    if (CUtil::IsHTTP(strLink))
    {
      CLog::Log(LOGDEBUG, "CSuggestionManager::GetSuggestionsBG, get suggestions from %s (search)", strLink.c_str());
      CStdString strHtml;
      if (m_http.Get(strLink, strHtml, false) )
      {
        CLog::Log(LOGDEBUG, "CSuggestionManager::GetSuggestionsBG, got suggestions from %s (search)", strLink.c_str());
        // Parse retrieved html
        ParseSuggestionXml(strHtml, items);
      }
      else
      {
        CLog::Log(LOGDEBUG, "CSuggestionManager::GetSuggestionsBG, could not get suggestions from %s (search)", strLink.c_str());
      }
    }
    else
    {
      // Use get directory API
      DIRECTORY::CDirectory::GetDirectory(strLink, items);
    }

    // Merge the retrieved result into the suggestion list
    CLog::Log(LOGDEBUG, "CSuggestionManager::GetSuggestionsBG, merge %d suggestions  (search)", items.Size());
    MergeByLabel(items, suggestionList);
  }

  // Remove duplicate labels
  std::vector<CSuggestionTip> vec;
  std::set<CSuggestionTip> s;
  unsigned size = suggestionList.Size();
  for( unsigned i = 0; i < size; ++i )
  {
    s.insert(CSuggestionTip(suggestionList.Get(i)->GetLabel(), suggestionList.Get(i)->GetProperty("value"), suggestionList.Get(i)->GetProperty("year"), suggestionList.Get(i)->GetProperty("country-codes"), suggestionList.Get(i)->GetPropertyBOOL("country-rel"), suggestionList.Get(i)->IsAdult()));
  }
  vec.assign( s.begin(), s.end() );

  suggestionList.Clear();

  CStdString highlightColorPrefix;
  CStdString highlightColorPostfix;
  bool needToAddColor = BuildColorHighlight(highlightColorPrefix, highlightColorPostfix);

  size = vec.size();
  for( unsigned i = 0; i < size; ++i )
  {
    CSuggestionTip suggestion = vec[i];

    if (needToAddColor)
    {
      CStdString label = suggestion.m_strLabel;
      MarkSearchStringInSuggestion(label, _strTerm, highlightColorPrefix, highlightColorPostfix);
      suggestion.m_strLabel = label;
    }

    CFileItem* tipItem = new CFileItem(suggestion.m_strLabel);

    if (!suggestion.m_strTerm.empty())
      tipItem->SetProperty("value", suggestion.m_strTerm);

    if (!suggestion.m_strYear.empty())
      tipItem->SetProperty("year", suggestion.m_strYear);

    tipItem->SetCountryRestriction(suggestion.m_strCountries,suggestion.m_countriesRel);
    tipItem->SetAdult(suggestion.m_isAdult);


    suggestionList.Add(CFileItemPtr(tipItem));

  }

  // Pass the result to the callback
  suggestionList.SetProperty("term", _strTerm);
  m_callback->OnSuggestions(suggestionList);
}

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

      CStdString strYear = ((TiXmlElement*)pTag)->Attribute("year");
      CStdString strValue = pValue->ValueStr();
      CStdString strLabel;
      if (strYear.IsEmpty())
        strLabel.Format("%s", strValue.c_str());
      else
        strLabel.Format("%s (%s)", strValue.c_str(), strYear.c_str());

      CFileItemPtr tipItem (new CFileItem(strLabel));
      tipItem->SetProperty("type", "tip");
      tipItem->SetProperty("value", strValue);
      tipItem->SetProperty("year", strYear);

      // currently the server doesn't pass IsAdult -> set it to FALSE
      tipItem->SetAdult(false);

      items.Add(tipItem);
    }
  }

  return true;
}

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

    for (int j = 0; j < right.Size();j++)
    {

      CFileItemPtr rightItem = right[j];

      if (rightItem->GetLabel() == leftItem->GetLabel())
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
            rightItem->AddLink(link->GetLabel(), link->m_strPath, link->GetContentType(true), CFileItem::GetLinkBoxeeTypeAsEnum(link->GetProperty("link-boxeetype")), "", "", "", "all", true,"",0);
          }
        }

        left.Remove(i);
        i--; // set the loop back
        break; // from the right loop
      }
    }
  }

  // Append the remaining items to the right list
  right.Append(left);
}

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

// END OF CSuggestionManager IMPLEMENTATION

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

// END OF CSuggestionManager::BXSuggestionJob IMPLEMENTATION

CGUIDialogBoxeeSearch::CGUIDialogBoxeeSearch(void)
: CGUIDialogKeyboard(WINDOW_DIALOG_BOXEE_SEARCH, "boxee_search.xml"), m_suggestionManager(this)
{
  m_bAutoCompleteEnabled = true;
  m_delayCounter = 0;
  m_keyType = CAPS;
  //m_bShift = true;
}

CGUIDialogBoxeeSearch::~CGUIDialogBoxeeSearch(void)
{}

void CGUIDialogBoxeeSearch::OnInitWindow()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeSearch::OnInitWindow - Enter function with [m_strType=%s] (search)",m_strType.c_str());

  m_delayCounter = 0;
  m_suggestionManager.Start();
  m_strSearchTerm = "";
  m_strSuggestion = "";

  if (m_strType == "tvshows")
  {
    if (g_application.IsConnectedToInternet())
    {
      CStdString strLink = BOXEE::BXConfiguration::GetInstance().GetStringParam("Boxee.Autocomplete","http://res.boxee.tv");
      strLink += "/titles/searchtip/tv/";

      std::map<CStdString, CStdString> mapRemoteOptions;
      DIRECTORY::CBoxeeServerDirectory::AddParametersToRemoteRequest(mapRemoteOptions);
      strLink += BoxeeUtils::BuildParameterString(mapRemoteOptions);

      if (mapRemoteOptions.size() > 0)
      {
        strLink += "&term=";
      }
      else
      {
        strLink += "?term=";
      }

      m_suggestionManager.AddSource(strLink);
    }

    m_suggestionManager.AddSource("boxeedb://tvshows/?search=");
  }
  else if (m_strType == "movies")
  {
    if (g_application.IsConnectedToInternet())
    {
      CStdString strLink = BOXEE::BXConfiguration::GetInstance().GetStringParam("Boxee.Autocomplete","http://res.boxee.tv");
      strLink += "/titles/searchtip/movie/";

      std::map<CStdString, CStdString> mapRemoteOptions;
      DIRECTORY::CBoxeeServerDirectory::AddParametersToRemoteRequest(mapRemoteOptions);
      strLink += BoxeeUtils::BuildParameterString(mapRemoteOptions);

      if (mapRemoteOptions.size() > 0)
      {
        strLink += "&term=";
      }
      else
      {
        strLink += "?term=";
      }

      m_suggestionManager.AddSource(strLink);
    }

    m_suggestionManager.AddSource("boxeedb://movies/?search=");
  }
  else if (m_strType == "apps")
  {
    m_suggestionManager.AddSource("apps://all/?search=");
  }
  else if (m_strType == "appbox")
  {
    m_suggestionManager.AddSource("appbox://all/?search=");
  }
  else if (m_strType == "music")
  {
    m_suggestionManager.AddSource("boxeedb://music/?search=");
  }

  CGUIDialogKeyboard::OnInitWindow();
  UpdateButtons();
}

void CGUIDialogBoxeeSearch::OnDeinitWindow(int nextWindowID)
{
  m_suggestionManager.Stop();
  CGUIDialogKeyboard::OnDeinitWindow(nextWindowID);
}

bool CGUIDialogBoxeeSearch::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_LABEL_BIND)
  {
    SetProperty("loading", false);
    if (message.GetPointer() && message.GetControlId() == 0)
    {
      CFileItemList *items = (CFileItemList *)message.GetPointer();
      if (items)
      {
        CStdString strTerm = items->GetProperty("term");

        // check that we received items for the current term
        if (m_strSearchTerm == strTerm)
        {
          CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), SUGGESTION_LIST, 0);
          OnMessage(msg);

          if (!m_strSearchTerm.IsEmpty())
          {
            // Add first item for the search term
            CStdString searchTermLabel;
            searchTermLabel.Format("More results for: %s", m_strSearchTerm.c_str());
            CFileItemPtr termItem(new CFileItem(searchTermLabel));
            termItem->SetProperty("value", m_strSearchTerm);

            CGUIMessage winmsg1(GUI_MSG_LABEL_ADD, GetID(), SUGGESTION_LIST, 0, 0, termItem);
            OnMessage(winmsg1);
          }

          for (int i = 0; i < items->Size(); i++)
          {
            CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), SUGGESTION_LIST, 0, 0, items->Get(i));
            OnMessage(winmsg);
          }
        }

        delete items;
      }
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
        if (selectedListItem->HasProperty("value"))
        {
          m_strSuggestion = selectedListItem->GetProperty("value");
        }
        else
        {
          CLog::Log(LOGERROR,"CGUIDialogBoxeeSearch::OnMessage, no value property in suggestion (search)");
        }

        CLog::Log(LOGDEBUG,"CGUIDialogBoxeeSearch::OnMessage, clicked suggestion = %s (search)", m_strSuggestion.c_str());
        OnOK();
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
    }
  }

  return bResult;
}

void CGUIDialogBoxeeSearch::Render()
{
  CStdString currentText = GetCurrentText();
  if (m_delayCounter > 15 && m_strSearchTerm != currentText)
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeSearch::Render, search for term = %s (search)", currentText.c_str());

    m_delayCounter = 0;

    m_strSearchTerm = currentText;
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


bool CGUIDialogBoxeeSearch::ShowAndGetInput(CStdString& aTextString, const CStdString& strType, const CStdString &strHeading, bool allowEmptyResult, bool hiddenInput /* = false */)
{
  CGUIDialogBoxeeSearch *pKeyboard = (CGUIDialogBoxeeSearch*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_SEARCH);

  if (!pKeyboard)
    return false;

  // setup keyboard
  pKeyboard->Initialize();
  //pKeyboard->CenterWindow();
  pKeyboard->SetHeading(strHeading);
  pKeyboard->SetHiddenInput(hiddenInput);
  pKeyboard->SetText(aTextString);
  pKeyboard->SetType(strType);

  // do this using a thread message to avoid render() conflicts
  ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_BOXEE_SEARCH, g_windowManager.GetActiveWindow()};
  g_application.getApplicationMessenger().SendMessage(tMsg, true);
  pKeyboard->Close();

  // If have text - update this.
  if (pKeyboard->IsConfirmed())
  {
    aTextString = pKeyboard->GetText();

    if (!allowEmptyResult && aTextString.IsEmpty())
    {
      return false;
    }

    return true;
  }
  else
  {
    return false;
  }
}

void CGUIDialogBoxeeSearch::OnSuggestions(CFileItemList& suggestionList)
{
  CFileItemList* pNewList = new CFileItemList();
  pNewList->Copy(suggestionList);
  pNewList->SetProperty("term", suggestionList.GetProperty("term"));

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), 0, 0, 0, pNewList);
  g_windowManager.SendThreadMessage(msg, GetID());
}

void CGUIDialogBoxeeSearch::UpdateButtons()
{
  CGUIDialogKeyboard::UpdateButtons();
}

