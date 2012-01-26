#ifndef GUIWINDOWBOXEEBROWSETVEPISODES_H_
#define GUIWINDOWBOXEEBROWSETVEPISODES_H_

#include "GUIWindowBoxeeBrowse.h"
#include "Thread.h"
#include "BoxeeUtils.h"
#include "Util.h"
#include "lib/libBoxee/bxsubscriptionsmanager.h"
#include <set>

#define THUMB_VIEW_LIST       50
#define LINE_VIEW_LIST        51

class SubscribeJob : public IRunnable
{
public:
  SubscribeJob(BOXEE::CSubscriptionType::SubscriptionTypeEnums type, const CStdString& id, const CStdString& strShowTitle, bool bSubscribe)
  {
    m_type = type;
    m_id = id;
    m_strShowTitle = strShowTitle;
    m_bSubscribe = bSubscribe;
    m_bJobResult = false;
  }

  virtual ~SubscribeJob() { }
  virtual void Run();
  CStdString m_id;
  CStdString m_strShowTitle;
  BOXEE::CSubscriptionType::SubscriptionTypeEnums m_type;
  bool m_bSubscribe;

};


class CLocalEpisodesSource : public CBrowseWindowSource
{
public:
  CLocalEpisodesSource(int iWindowID);
  virtual ~CLocalEpisodesSource();

  void AddStateParameters(std::map <CStdString, CStdString>& mapOptions);
  void BindItems(CFileItemList &items);

  CStdString m_strShowId;

};

class CRemoteEpisodesSource : public CBrowseWindowSource
{
public:
  CRemoteEpisodesSource(int iWindowID);
  virtual ~CRemoteEpisodesSource();

  void AddStateParameters(std::map <CStdString, CStdString>& mapOptions);
  void BindItems(CFileItemList &items);

  void SetAllowProviders(const CStdString& strAllowProviders);

  CStdString m_strShowId;
  CStdString m_allowProviders;
};

class CEpisodesWindowState : public CBrowseWindowState
{
public:
  CEpisodesWindowState(CGUIWindowBoxeeBrowse* pWindow);
	
  bool OnSeasons(std::set<int>& m_setSeasons);
  bool OnSubscribe();
  void OnFree();
  
  void SetDefaultView();

  void SetFree(bool bFree);

  void SetShowId(const CStdString& strShowId);
  void SetShowTitle(const CStdString& strShowTitle);
  void SetShowThumb(const CStdString& strShowThumb);
  void SetSeason(const CStdString& strSeasonId);
  void SetAllowProviders(const CStdString& strAllowProviders);

  bool UpdateSubscription();
  virtual CStdString GetItemSummary();

  void Refresh(bool bResetSelected);

  int GetSeason();

protected:
  
  virtual void UpdateFilters(const CStdString& strPath) {}

  int m_iSort;
  int m_iSeason;
  bool m_bSubscribed;
  bool m_bFreeOnly;
  bool m_bHasShortcut;

  CStdString m_strShowId;
  CStdString m_strShowTitle;
  CStdString m_strShowThumb;
  CStdString m_allowProviders;
};


class CGUIWindowBoxeeBrowseTvEpisodes : public CGUIWindowBoxeeBrowse
{
public:
  CGUIWindowBoxeeBrowseTvEpisodes();
	virtual ~CGUIWindowBoxeeBrowseTvEpisodes();
	
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);

  virtual void OnBack();

  virtual void ShowItems(CFileItemList& list, bool append=false);

  virtual void ConfigureState(const CStdString& param);

  virtual CStdString GetItemDescription();

  void SetTvShowItem(const CFileItem& tvShowItem);

protected:
  void SetFreeOnlyFilter(bool bOn);
  void ExtractSeasons(CFileItemList* episodeList);

  void AddSeparators(CFileItemList &items);

  bool HandleClickOnSeasonList();
  bool HandleClickOnOverviewButton();

  std::set<int> m_setSeasons;

  int m_originalItemCount;

  CFileItem m_tvShowItem;
};

#endif /*GUIWINDOWBOXEEBROWSETVEPISODES_H_*/
