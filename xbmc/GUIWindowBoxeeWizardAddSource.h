#ifndef GUI_WINDOW_BOXEE_WIZARD_ADD_SOURCE
#define GUI_WINDOW_BOXEE_WIZARD_ADD_SOURCE

#pragma once

#include "GUIDialog.h"
#include "FileItem.h"
#include <vector>

struct ShareData
{
  CStdString strLabel;
  CStdString strPath;
  CStdString strThumb;
  ShareData(const CStdString &label, const CStdString &path, const CStdString &thumb): strLabel(label), strPath(path), strThumb(thumb) { }
};

class CGUIWindowBoxeeWizardAddSource : public CGUIDialog
{
public:
  CGUIWindowBoxeeWizardAddSource(void);
  virtual ~CGUIWindowBoxeeWizardAddSource(void);
  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);
  void SetCategory(CStdString category);
  
private:
  void AddPlugins(const CStdString &strType, std::vector<ShareData> &sharesVec);

  void ShowLocalDevices();
  void ShowInternetApps();
  void ShowNetworkDevices();
  void BrowseDirectory();
  void InitBrowseDirectory();

  CFileItemList m_devices;  
  CFileItemList m_folders;
  
  CStdString m_rootFolder;
  CStdString m_currentFolder;  
  CStdString m_upFolder;  
  
  CStdString m_category;
};

#endif
