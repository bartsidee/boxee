#pragma once

#include "GUIDialogKeyboard.h"
#include "FileSystem/FileCurl.h"
#include "lib/libBoxee/bxbgprocess.h"
#include "BrowseWindowState.h"
#include "Thread.h"

#include "boost/shared_ptr.hpp"

class CGUIToggleButtonControl;

// //////////////////////////////////////////////////////////////////////////////////////////
// Suggestion related classes

class ISuggestionCallback
{
public:
  virtual void OnSuggestions(CFileItemList& suggestionList) = 0;
  virtual ~ISuggestionCallback() {} // to avoid warnings
};

class CSuggestionSource
{
public:
  virtual ~CSuggestionSource(){};
  bool m_bRemote;
  CStdString m_strBaseUrl;
  CStdString m_contentType;
  CStdString m_mediaIcon;
  std::map<CStdString, CStdString> m_mapOptions;
  CStdString GetUrl(const CStdString& strTerm);
  virtual void ApplyIcon(CFileItemList& list);
};

class CSuggestionMediaSource : public CSuggestionSource
{
public:
  virtual ~CSuggestionMediaSource(){};
  CStdString m_moviesIcon;
  CStdString m_showsIcon;
  CStdString m_clipsIcon;

  virtual void ApplyIcon(CFileItemList& list);
};

typedef boost::shared_ptr<CSuggestionSource> CSuggestionSourcePtr;

class CSuggestionManager
{
  class BXSuggestionJob : public BOXEE::BXBGJob
  {
public:
    BXSuggestionJob(const CStdString& strTerm, CSuggestionManager* manager);
    virtual ~BXSuggestionJob();
    virtual void DoWork();

  private:
    CStdString m_strTerm;
    CSuggestionManager* m_manager;
  };

public:
  CSuggestionManager(ISuggestionCallback* callback);
  virtual ~CSuggestionManager();

  void Start();
  void Stop();

  void AddSource(const CSuggestionSourcePtr& source);

  void GetSuggestions(const CStdString& strTerm);
  void GetSuggestionsBG(const CStdString& strTerm);

  //bool ParseSuggestionXml(const CStdString& strHtml, CFileItemList& items);
  void MergeByLabel(CFileItemList& left, CFileItemList& right);

  void Close();

private:

  //bool BuildColorHighlight(CStdString& highlightColorPrefix, CStdString& highlightColorPostfix);
  void MarkSearchStringInSuggestion(CStdString& suggestion, const CStdString& strTerm, const CStdString& highlightColorPrefix, const CStdString& highlightColorPostfix);

  bool m_bSessionStarted;

  XFILE::CFileCurl m_http;

  BOXEE::BXBGProcess m_suggestionProcessor;

  ISuggestionCallback* m_callback;
  std::vector<CSuggestionSourcePtr> m_vecSources;
};

class CSuggestionTip
{
public:
  CSuggestionTip() { m_isAdult = false; m_countriesRel = false;}
  CSuggestionTip(const CStdString& strLabel, const CStdString& strTerm, const CStdString& strYear, const CStdString& strCountries, bool countriesRel, bool isAdult)
  {
    m_strLabel = strLabel;
    m_strTerm = strTerm;
    m_strYear = strYear;
    m_strCountries = strCountries;
    m_countriesRel = countriesRel;
    m_isAdult = isAdult;
  }

  CStdString m_strType;
  CStdString m_strLink;
  CStdString m_strLabel;
  CStdString m_strTerm;
  CStdString m_strYear;
  CStdString m_strCountries;
  bool m_countriesRel;
  bool m_isAdult;

  bool operator<(const CSuggestionTip& t) const
  {
      return m_strLabel < t.m_strLabel;
  }
};

class CAllCategoriesLoadedJob : public IRunnable
{
public:
  CAllCategoriesLoadedJob() { m_bDone = false; }
  virtual void Run();
  bool m_bDone;
};

// //////////////////////////////////////////////////////////////////////////////////////////

class CGUIDialogBoxeeSearch: public CGUIDialogKeyboard, public ISuggestionCallback
{
public:

  CGUIDialogBoxeeSearch(void);
  virtual ~CGUIDialogBoxeeSearch(void);

  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  virtual void Render();

  virtual void Close(bool forceClose = false);

  bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);

  void InitSuggestions();

  // OBSOLETE FUCTIONS? REMOVE
  void SetAutoCompleteEnabled(bool bEnabled);
  void SetType(const CStdString& strType);
  virtual CStdString GetText() const;

  CStdString GetSearchTerm() { return m_strSearchTerm; }

  //void LoadSearchResults(const CStdString& strTerm);
  virtual void OnOK();

  // Implementation of ISuggestionCallback
  virtual void OnSuggestions(CFileItemList& suggestionList);

  bool ClosedByMovingRightFromTextBox() {return m_bClosedByMovingRightFromTextBox; }
  void SetSearchBoxWidth(float width);

private:

  bool HandleSearchingUrl(const CStdString& url);

  bool m_bAutoCompleteEnabled;
  CStdString m_strSearchTerm;
  CStdString m_strSuggestion;
  CStdString m_strType;
  int m_iContext;
  int m_delayCounter;
  CSuggestionManager m_suggestionManager;

  bool m_canSendQueryToServer;

  bool m_bClosedByMovingRightFromTextBox;

};
