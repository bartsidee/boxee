#ifndef GUIWINDOWBOXEEBROWSELOCAL_H_
#define GUIWINDOWBOXEEBROWSELOCAL_H_

#include "GUIWindowBoxeeBrowse.h"

class CLocalFilesSource : public CBrowseWindowSource
{
public:
  CLocalFilesSource(int iWindowID);
  virtual ~CLocalFilesSource();
  void BindItems(CFileItemList& items);
  void SetStartingPath(const CStdString& strStartingPath);
  virtual void RequestItems(bool bUseCurPath = false);
};

class CLocalBrowseWindowState : public CBrowseWindowStateWithHistory
{
public:

  CLocalBrowseWindowState(CGUIWindowBoxeeBrowse* pWindow);
  virtual void InitState();

  virtual void OnPathChanged(CStdString strPath, bool bResetSelected=true);
  virtual bool OnBack();
  
  // Used to update the state of scanning button on the left panel
  void SetCurrentPath(const CStdString& path);
  CStdString GetCurrentPath();
  void FromHistory(CBrowseStateHistoryItem* historyItem);
  virtual CStdString GetItemSummary();

  virtual void Refresh(bool bResetSeleted);
private:
  void UpdateProperties();
};

class CGUIWindowBoxeeBrowseLocal : public CGUIWindowBoxeeBrowse
{
public:
  CGUIWindowBoxeeBrowseLocal();
  virtual ~CGUIWindowBoxeeBrowseLocal();

  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);

  virtual void ConfigureState(const CStdString& param);

  //virtual bool ProcessPanelMessages(CGUIMessage& message);

  void UpdateScanningButtonState(int iStatus = 0);
  void UpdateEjectButtonState();
  void UpdateSortButtonState();
  void UpdateEmptyLabelText();

  CStdString GetItemDescription();
  void EjectUserButtonPressed();

  virtual bool HandleEmptyState();

protected:

  void GetStartMenusStructure(std::list<CFileItemList>& browseMenuLevelList);
};

#endif /*GUIWINDOWBOXEEBROWSELOCAL_H_*/
