#pragma once

#include "GUIDialogKeyboard.h"
#include "FileSystem/FileCurl.h"
#include "lib/libBoxee/bxbgprocess.h"

class ISuggestionCallback
{
public:
  virtual void OnSuggestions(CFileItemList& suggestionList) = 0;
  virtual ~ISuggestionCallback() {} // to avoid warnings
};

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

  void AddSource(const CStdString& strSource);

  void GetSuggestions(const CStdString& strTerm);
  void GetSuggestionsBG(const CStdString& strTerm);

  bool ParseSuggestionXml(const CStdString& strHtml, CFileItemList& items);
  void MergeByLabel(CFileItemList& left, CFileItemList& right);

  void Close();

private:

  bool BuildColorHighlight(CStdString& highlightColorPrefix, CStdString& highlightColorPostfix);
  void MarkSearchStringInSuggestion(CStdString& suggestion, const CStdString& strTerm, const CStdString& highlightColorPrefix, const CStdString& highlightColorPostfix);

  bool m_bSessionStarted;

  XFILE::CFileCurl m_http;

  BOXEE::BXBGProcess m_suggestionProcessor;

  ISuggestionCallback* m_callback;
  std::vector<CStdString> m_vecSources;
};

class CSuggestionTip
{
public:
  CSuggestionTip(const std::string& strLabel, const std::string& strTerm, const std::string& strYear, const std::string& strCountries, bool countriesRel, bool isAdult)
  {
    m_strLabel = strLabel;
    m_strTerm = strTerm;
    m_strYear = strYear;
    m_strCountries = strCountries;
    m_countriesRel = countriesRel;
    m_isAdult = isAdult;
  }

  std::string m_strLabel;
  std::string m_strTerm;
  std::string m_strYear;
  std::string m_strCountries;
  bool m_countriesRel;
  bool m_isAdult;

  bool operator<(const CSuggestionTip& t) const
  {
    return m_strLabel < t.m_strLabel;
  }
};

class CGUIDialogBoxeeSearch: public CGUIDialogKeyboard, public ISuggestionCallback
{
public:

  CGUIDialogBoxeeSearch(void);
  virtual ~CGUIDialogBoxeeSearch(void);

  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  virtual void Render();

  bool OnMessage(CGUIMessage& message);

  void SetAutoCompleteEnabled(bool bEnabled);
  void SetType(const CStdString& strType);
  virtual CStdString GetText() const;

  static bool ShowAndGetInput(CStdString& aTextString, const CStdString& strType, const CStdString &strHeading, bool allowEmptyResult, bool hiddenInput = false);

  // Implementation of ISuggestionCallback
  virtual void OnSuggestions(CFileItemList& suggestionList);

protected:
  virtual void UpdateButtons();

private:
  bool m_bAutoCompleteEnabled;
  CStdString m_strSearchTerm;
  CStdString m_strSuggestion;
  CStdString m_strType;  // type determines from which sources should the suggestions be retrieved
  int m_delayCounter;
  CSuggestionManager m_suggestionManager;
};
