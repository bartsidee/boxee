#ifndef GUIWINDOWBOXEE_BROWSE_APPS_H_
#define GUIWINDOWBOXEE_BROWSE_APPS_H_

#include "GUIWindowBoxeeBrowse.h"

class CMyAppsSource : public CBrowseWindowSource
{
public:
  CMyAppsSource(int iWindowID);
  virtual ~CMyAppsSource();
  void BindItems(CFileItemList& items);
};

class CAppStoreSource : public CBrowseWindowSource
{
public:
  CAppStoreSource(int iWindowID);
  virtual ~CAppStoreSource();
  void BindItems(CFileItemList& items);
  void AddStateParameters(std::map <CStdString, CStdString>& mapOptions);

  CStdString m_strFilterCategory;
};

class CRepositorySource : public CBrowseWindowSource
{
public:
  CRepositorySource(int iWindowID);
  virtual ~CRepositorySource();

  void AddStateParameters(std::map <CStdString, CStdString>& mapOptions);

  CStdString m_strRepositoryId;
  CStdString m_strLabel;
};


class CAppsWindowState : public CBrowseWindowState
{
public:
  CAppsWindowState(CGUIWindowBoxeeBrowse* pWindow);

  virtual void SetApplicationType(const CStdString& strType);
  void SetCategoryFilter(const CStdString& filter);
  virtual bool OnBack();
  bool OnMyApps();
  bool OnAllApps();
  bool OnRepositories();
  void OnBind(CFileItemList& items);

  void SetState(int state);
  int GetState();

  void SetCategory(const CStdString& strCategory);
  void SetDefaultCategory();

  void SetRepository(const CStdString& strRepositoryId, const CStdString& strLabel);

  void SetDefaultView();

  virtual void Refresh(bool bResetSelected=false);
  CStdString GetItemSummary();

  CStdString GetFilterCategory();

protected:
  CStdString m_strApplicationType;
  CStdString m_strFilterCategory;

  CStdString m_iPreviousCategory;
  CBoxeeSort m_savedSort;

private:
  CStdString m_strCurrentRepository;
};


class CGUIWindowBoxeeBrowseApps : public CGUIWindowBoxeeBrowse
{
public:

  CGUIWindowBoxeeBrowseApps();
  virtual ~CGUIWindowBoxeeBrowseApps();

  virtual bool OnMessage(CGUIMessage& message);

  //virtual bool ProcessPanelMessages(CGUIMessage& message);

  void SetWindowLabel(int controlId, const CStdString windowLabel);

  bool IsInMyAppsState();

  void FromRepositories();

  void ShowItems(CFileItemList& list, bool append);

  virtual bool HandleEmptyState();

protected:

  virtual void ConfigureState(const CStdString& param);
  virtual void GetStartMenusStructure(std::list<CFileItemList>& browseMenuLevelList);

  virtual bool OnClick(int iItem);

private:

  bool HandleBtnApplicationType();
  bool FillDropdownWithApplicationTypes(CFileItemList& applicationTypes);

  bool m_isRepository;

};

#endif /*GUIWINDOWBOXEE_BROWSE_APPS_H_*/
