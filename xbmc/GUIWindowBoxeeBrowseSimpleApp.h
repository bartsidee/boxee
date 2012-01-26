#ifndef  _GUIWINDOWBOXEEBROWSESIMPLEAPP_H
#define  _GUIWINDOWBOXEEBROWSESIMPLEAPP_H

#include "GUIWindowBoxeeBrowse.h"

class CGUIBrowseSimpleAppState : public CBrowseWindowStateWithHistory
{
public:
  CGUIBrowseSimpleAppState(CGUIWindowBoxeeBrowse* pWindow);
  virtual ~CGUIBrowseSimpleAppState();

  //virtual CStdString CreatePath();
  virtual CBrowseWindowStateWithHistory* Clone();
  virtual bool OnBack();

  // Function is responsible for setting the starting path
  // that is provided during window activation
  void SetStartingPath(const CStdString& strStartingPath);
  virtual CStdString GetItemSummary();
  virtual void Refresh(bool bResetSelected = false);

  void OnPathChanged(CStdString strPath, bool bResetSelected);
};


///////////////////////////////////////////////////////////////////////////////////////


class CGUIWindowBoxeeBrowseSimpleApp : public CGUIWindowBoxeeBrowse
{
public:
  CGUIWindowBoxeeBrowseSimpleApp();
  virtual ~CGUIWindowBoxeeBrowseSimpleApp();

  static void Show(const CStdString &strPath, const CStdString &strLabel = "", const CStdString &strBackgroundImage = "",bool bResetHistory = true, const CStdString &strAppId = "");
  void SetAppData(const CStdString& strAppId, const CStdString& strAppName);
  virtual bool OnMessage(CGUIMessage& message);

  void ResetWindowState(const CStdString &strPath, const CStdString &strLabel, const CStdString &strBackgroundImage, bool bResetHistory);

protected:
  virtual bool OnBind(CGUIMessage& message);

private:

  bool HasAppData();
  void ResetAppData();

  void SetItemWithAppData(CFileItem& item);

  // if m_strAppId is set - then the browse screen was activated from an app. 
  CStdString m_strAppId;
  CStdString m_strAppName;
};

#endif
