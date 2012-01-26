
#include "GUIDialogBoxeeManualResolveResults.h"
#include "GUIWindowManager.h"
#include "Util.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "tinyXML/tinyxml.h"
#include "MetadataResolverVideo.h"
#include "BoxeeUtils.h"

using namespace BOXEE;

#define RESULT_LIST 9234


//CGetDetailsBG::CGetDetailsBG(CFileItemPtr videoItem, bool isMovie)
//{
//  m_VideoItem = videoItem;
//  m_bIsMovie = isMovie;
//}
//
//void CGetDetailsBG::Run()
//{
//  CStdString strLink = BXConfiguration::GetInstance().GetStringParam("Boxee.Resolver.Server","http://res.boxee.tv");
//
//  CVideoFileContext context(m_VideoItem->m_strPath);
//
//  // Load relevant details
//
//  std::map<CStdString, CStdString> mapOptions;
//  context.GetFileOptionsMap(mapOptions);
//
//  // Add the id we have received from the server
//  CStdString idAttributeName = m_VideoItem->GetProperty("manualresolve::idName");
//  CStdString idAttributeValue = m_VideoItem->GetProperty("manualresolve::idValue");
//
//  mapOptions[idAttributeName] = idAttributeValue;
//
//  if (m_bIsMovie)
//  {
//    strLink += "/title/movie/";
//  }
//  else
//  {
//    strLink += "/title/tv/";
//  }
//  strLink += BoxeeUtils::BuildParameterString(mapOptions);
//
//}
//
//CGetResultListBG::CGetResultListBG(const CStdString& strTitle, bool bIsMovie)
//{
//  m_strTitle = strTitle;
//  m_bIsMovie = bIsMovie;
//}
//
//void CGetResultListBG::CGetResultListBG::Run()
//{
//  m_bResult = false;
//
//  // Get the list of the manual resolve results
//  CStdString strLink = BOXEE::BXConfiguration::GetInstance().GetStringParam("Boxee.ManualResolve","http://res.boxee.tv");
//  if (m_bIsMovie)
//  {
//    strLink += "/titles/msearch/movie?name=";
//  }
//  else
//  {
//    strLink += "/titles/msearch/tv?name=";
//  }
//
//  strLink += m_strTitle;
//
//  CStdString strHtml;
//  if (m_http.Get(strLink, strHtml, false))
//  {
//    // Parse retrieved html
//    m_bResult = ParseResultListXml(strHtml, m_resultListItems);
//  }
//  else
//  {
//    CLog::Log(LOGDEBUG, "CSuggestionManager::GetSuggestionsBG, could not get suggestions from %s (search)", strLink.c_str());
//  }
//}
//
//bool CGetResultListBG::ParseResultListXml(const CStdString& strHtml, CFileItemList& items)
//{
//  TiXmlDocument xmlDoc;
//  xmlDoc.Parse(strHtml);
//
//  TiXmlElement *pRootElement = xmlDoc.RootElement();
//  if (!pRootElement)
//    return false;
//
//  if (strcmpi(pRootElement->Value(), "search") != 0)
//  {
//    CLog::Log(LOGERROR, "CGetResultListBG::ParseResultListXml, could not parse manual resolution results (manual)");
//    return false;
//  }
//
//  const TiXmlNode* pTag = 0;
//  while ((pTag = pRootElement->IterateChildren(pTag)))
//  {
//    if (pTag->ValueStr() == "title")
//    {
//      const TiXmlNode *pValue = pTag->FirstChild();
//      CStdString strValue = pValue->ValueStr();
//
//      // Find the id attribute, we do not know its name but we know that there are two attributes and it's not the one called 'year'
//      CStdString idAttributeName;
//      CStdString idAttributeValue;
//
//      TiXmlAttribute* attribute = ((TiXmlElement*)pTag)->FirstAttribute();
//      while (attribute)
//      {
//        if (strcmpi(attribute->Name(), "year") != 0)
//        {
//          idAttributeName = attribute->Name();
//          idAttributeValue = attribute->Value();
//        }
//        attribute = attribute->Next();
//      }
//
//      if (idAttributeName.IsEmpty() || idAttributeValue.IsEmpty())
//      {
//        // this should not happen, each search result should have an id
//        CLog::Log(LOGERROR, "CGetResultListBG::ParseResultListXml, search result without id, value = %s (manual)", strValue.c_str());
//        continue;
//      }
//
//      // Get the year
//      CStdString strYear = ((TiXmlElement*)pTag)->Attribute("year");
//
//      // Format label and create file item
//      CStdString strLabel;
//      if (strYear.IsEmpty())
//        strLabel.Format("%s", strValue.c_str());
//      else
//        strLabel.Format("%s (%s)", strValue.c_str(), strYear.c_str());
//
//      CFileItemPtr resultItem (new CFileItem(strLabel));
//      resultItem->SetProperty("type", "msearch");
//      resultItem->SetProperty("manualresolve::Title", strValue);
//      resultItem->SetProperty("manualresolve::Year", strYear);
//      resultItem->SetProperty("manualresolve::idName", idAttributeName);
//      resultItem->SetProperty("manualresolve::idValue", idAttributeValue);
//      items.Add(resultItem);
//    }
//  }
//
//  return true;
//}


CGUIDialogBoxeeManualResolveResults::CGUIDialogBoxeeManualResolveResults() :
  CGUIDialog(WINDOW_DIALOG_BOXEE_MANUAL_RESULTS, "boxee_manual_resolve_results.xml")
  {
  }

CGUIDialogBoxeeManualResolveResults::~CGUIDialogBoxeeManualResolveResults()
{
}

void CGUIDialogBoxeeManualResolveResults::Show(CFileItemPtr pItem)
{
  CGUIDialogBoxeeManualResolveResults *pDialog = (CGUIDialogBoxeeManualResolveResults*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_MANUAL_RESULTS);
  if (pDialog)
  {
    // Copy the item into the dialog
    pDialog->m_VideoItem = pItem;
    pDialog->DoModal();
  }
}

void CGUIDialogBoxeeManualResolveResults::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  // Send the item to the special container to allow skin acceess 
  CFileItemPtr itemPtr(new CFileItem(*m_VideoItem.get()));
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), 5000, 0, 0, itemPtr);
  g_windowManager.SendThreadMessage(winmsg);

//  // Get the name with which we should resolve
//  bool bMovie = m_VideoItem->GetPropertyBOOL("manualresolve::isMovie");
//  CStdString title = m_VideoItem->GetProperty("manualresolve::Title");
//
//  CGetResultListBG* pJob = new CGetResultListBG(title, bMovie);
//  CUtil::RunInBG(pJob);
//
//  if (!pJob->m_bResult)
//  {
//    CLog::Log(LOGERROR,"CGUIDialogBoxeeManualResolveResults::OnInitWindow, failed to get results, title = %s (manual)", title.c_str());
//  }
//  else
//  {
//    // DEBUG: Print all results to log
//    for (int i = 0; i < pJob->m_resultListItems.Size(); i++)
//    {
//      CLog::Log(LOGERROR, "CGUIDialogBoxeeManualResolveResults::OnInitWindow, result = %s (manual)", pJob->m_resultListItems.Get(i)->GetLabel().c_str());
//    }
//
//    // Populate the list with results
//    CGUIMessage message2(GUI_MSG_LABEL_BIND, GetID(), RESULT_LIST, 0, 0, &pJob->m_resultListItems);
//    OnMessage(message2);
//  }
//
//  delete pJob;
}

void CGUIDialogBoxeeManualResolveResults::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
}

bool CGUIDialogBoxeeManualResolveResults::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();
//    if (iControl == BTN_ISMOVIE)
//    {
//    }
//    else if (iControl == BTN_ISTVSHOW)
//    {
//    }
  }
  break;
  } // switch
  return CGUIDialog::OnMessage(message);
}

