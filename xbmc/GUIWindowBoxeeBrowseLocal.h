#ifndef GUIWINDOWBOXEEBROWSELOCAL_H_
#define GUIWINDOWBOXEEBROWSELOCAL_H_

#include "GUIWindowBoxeeBrowseWithPanel.h"

class CLocalBrowseWindowState : public CBrowseWindowState
{
public:

  CLocalBrowseWindowState(CGUIWindow* pWindow);
  //void InitState(const CStdString& strPath);
  virtual CStdString CreatePath();

  virtual void OnPathChanged(CStdString strPath, bool bResetSelected);

  // Handling shortcut button
  bool UpdateHasShortcut(const CStdString& strCommand);
  bool OnShortcut(const CStdString& strCommand);
  void SetHasShortcut(bool isInShortcut);
  bool HasShortcut();

  CStdString m_strPath;

private:

  bool m_bHasShortcut;

};

class CGUIWindowBoxeeBrowseLocal : public CGUIWindowBoxeeBrowseWithPanel
{
public:
  CGUIWindowBoxeeBrowseLocal();
  virtual ~CGUIWindowBoxeeBrowseLocal();

  virtual void OnInitWindow();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool ProcessPanelMessages(CGUIMessage& message);

  virtual bool OnBind(CGUIMessage& message);

  void UpdateButtonState(int iStatus = 0);

private:
  void UpdateShortcutButton();

};

#endif /*GUIWINDOWBOXEEBROWSELOCAL_H_*/
