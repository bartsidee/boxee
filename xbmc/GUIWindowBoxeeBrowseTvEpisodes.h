#ifndef GUIWINDOWBOXEEBROWSETVEPISODES_H_
#define GUIWINDOWBOXEEBROWSETVEPISODES_H_

#include "GUIWindowBoxeeBrowseWithPanel.h"
#include "Thread.h"
#include "BoxeeUtils.h"
#include "Util.h"
#include "lib/libBoxee/bxsubscriptionsmanager.h"
#include <set>

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

class CEpisodesWindowState : public CBrowseWindowState
{
public:
  CEpisodesWindowState(CGUIWindow* pWindow);
  void InitState(const CStdString& strPath);

  virtual void SortItems(CFileItemList &items);

  CStdString CreatePath();

  bool OnSeasons(std::set<int>& m_setSeasons);
  bool OnSubscribe();
  bool OnShortcut();
  void OnFree();

  void SetFree(bool bFree);

  bool HasShortcut();

  virtual void Reset();
  void SetPath(const CStdString& strPath);
  void SetShowId(const CStdString& strShowId);
  void SetShowTitle(const CStdString& strShowTitle);
  void SetShowThumb(const CStdString& strShowThumb);

protected:

  virtual void UpdateFilters(const CStdString& strPath) {}

  int m_iSort;
  int m_iSeason;
  bool m_bSubscribed;
  bool m_bFreeOnly;
  bool m_bHasShortcut;
  bool m_bLocal;
  bool m_bRemote;


  CStdString m_strPath;
  CStdString m_strShowId;
  CStdString m_strShowTitle;
  CStdString m_strShowThumb;

};


class CGUIWindowBoxeeBrowseTvEpisodes : public CGUIWindowBoxeeBrowseWithPanel
{
public:
  CGUIWindowBoxeeBrowseTvEpisodes();
	virtual ~CGUIWindowBoxeeBrowseTvEpisodes();
	
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  bool OnMessage(CGUIMessage& message);
	virtual bool ProcessPanelMessages(CGUIMessage& message);
  virtual bool OnBind(CGUIMessage& message);
  virtual void OnBack();

protected:
  void SetFreeOnlyFilter(bool bOn);
  void ExtractSeasons(CFileItemList* items, std::set<int>& setSeasons);

  std::set<int> m_setSeasons;


};

#endif /*GUIWINDOWBOXEEBROWSETVEPISODES_H_*/
