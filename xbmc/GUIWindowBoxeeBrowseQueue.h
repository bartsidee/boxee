#ifndef GUIWINDOWBOXEEBROWSEQUEUE_H_
#define GUIWINDOWBOXEEBROWSEQUEUE_H_

#include "GUIWindowBoxeeBrowse.h"
#include "lib/libBoxee/bxqueuemanager.h"

class CQueueSource : public CBrowseWindowSource
{
public:
  CQueueSource(int iWindowID);
  virtual ~CQueueSource();

  void AddStateParameters(std::map <CStdString, CStdString>& mapOptions);
};

class CQueueWindowState : public CBrowseWindowState
{
public:
  CQueueWindowState(CGUIWindowBoxeeBrowse* pWindow);
  void SetDefaultView();

  void SetQueueType(const CStdString& queueType);
  CStdString GetQueueType();

  virtual void Refresh(bool bResetSelected=false);

protected:

  CStdString GetItemSummary();

  CStdString m_queueTypeStr;
};

class CGUIWindowBoxeeBrowseQueue : public CGUIWindowBoxeeBrowse
{
public:
  CGUIWindowBoxeeBrowseQueue();
  virtual ~CGUIWindowBoxeeBrowseQueue();

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);

  virtual void ShowItems(CFileItemList& list, bool append=false);

  virtual CStdString GetItemDescription();

  static void LaunchWatchLater();
protected:

  virtual void ConfigureState(const CStdString& param);
  virtual void GetStartMenusStructure(std::list<CFileItemList>& browseMenuLevelList);

private:

  bool OnClick(CGUIMessage& message);
  bool HandleClickOnGetStartedButton();
  static bool WatchLaterGetStarted();
};

#endif /*GUIWINDOWBOXEEBROWSEQUEUE_H_*/
