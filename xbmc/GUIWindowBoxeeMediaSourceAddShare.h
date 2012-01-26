#ifndef GUI_WINDOW_BOXEE_MEDIA_ADD_SHARE
#define GUI_WINDOW_BOXEE_MEDIA_ADD_SHARE

#pragma once

#include "GUIDialog.h"
#include "FileItem.h"
#include "utils/Thread.h"

class CGUIWindowBoxeeMediaSourceAddShare : public CGUIDialog
{
  class BrowseWorkgroupsJob : public IRunnable
  {
  public:
    BrowseWorkgroupsJob(const CStdString& sharePath, CFileItemList& folders) :
      m_folders(folders)
    {
      m_sharePath = sharePath;
    }

    virtual void Run();
    CStdString m_sharePath;
    CFileItemList& m_folders;
  };
  
  class BrowseComputersJob : public IRunnable
  {
  public:
    BrowseComputersJob(const CStdString& sharePath, CFileItemList& folders) :
      m_folders(folders)
      {
      m_sharePath = sharePath;
      }

    virtual void Run();
    CStdString m_sharePath;
    CFileItemList& m_folders;
  };
  
public:
  CGUIWindowBoxeeMediaSourceAddShare(void);
  virtual ~CGUIWindowBoxeeMediaSourceAddShare(void);
  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  CFileItemPtr GetSelectedFileItem();
  void Refresh();

private:
  void ClearDevicesList();
  void ShowNetworkDevices();
  void ShowLocalDevices();
  
  void ProccessItemSelectedInControlShareList();
  void ProccessHitOnRefreshButton();

  int m_selectedSource;
  CFileItemList m_devices;
  bool m_devicesFound;    
};

#endif
