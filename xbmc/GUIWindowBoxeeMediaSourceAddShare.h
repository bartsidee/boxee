#ifndef GUI_WINDOW_BOXEE_MEDIA_ADD_SHARE
#define GUI_WINDOW_BOXEE_MEDIA_ADD_SHARE

#pragma once

#include "GUIDialog.h"
#include "FileItem.h"
#include "utils/Thread.h"

class CControlListState
{
public:
  enum ControlListStateEnums
  {
    CLS_PROTOCOLS             = 0,
    CLS_PCOMPUTERS            = 1,
    CLS_DEVICES               = 2,
    CLS_PCOMPUTERS_NO_RESULTS = 3,
    CLS_DEVICES_NO_RESULTS    = 4,
    CLS_NUM_OF_STATES         = 5
  };
};

class CProtocols
{
public:
  enum ProtocolsEnums
  {
    PTCL_AFP              = 0,
    PTCL_BMS              = 1,
    PTCL_NFS              = 2,
    PTCL_UPNP             = 3,
    PTCL_SAMBA            = 4,
    PTCL_LOCAL            = 5,
    PTCL_NUM_OF_PROTOCOLS = 6,
    PTCL_NO_PROTOCOL      = 7
  };
};


class CGUIWindowBoxeeMediaSourceAddShare : public CGUIDialog
{
  class BrowseWorkgroupsJob : public IRunnable
  {
  public:
    BrowseWorkgroupsJob(const CStdString& sharePath, CFileItemList* folders) : m_folders(folders)
    {
      m_sharePath = sharePath;
    }

    virtual void Run();
    CStdString m_sharePath;
    CFileItemList* m_folders;
  };
  
  class BrowseComputersJob : public IRunnable
  {
  public:
    BrowseComputersJob(const CStdString& sharePath, CFileItemList* folders) : m_folders(folders)
    {
      m_sharePath = sharePath;
    }

    virtual void Run();
    CStdString m_sharePath;
    CFileItemList* m_folders;
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
  bool ShowLocalDevices();
  bool ShowNetworkDevices();
  bool ShowProtocols();
  bool ShowComputerDevices();

  CStdString GetProtocolFromEnum(CProtocols::ProtocolsEnums protocolEnum);
  bool GetSelectedProtocol();
  bool SetProtocolFromLabel(int selectedLabel);
  bool SetSelectedComputer();
  bool ParentDirAction();
  bool PreviousMenuAction();
  bool ControlListSelectAction();
  CStdString GetItemDescription();
  bool ProccessItemSelectedInControlShareList();
  void ProccessHitOnRefreshButton();
  void ProccessHitOnAddButton();
  void HandleNoResults();

  int m_selectedSource;
  CFileItemList m_devices;
  bool m_devicesFound;
  bool m_addButton;
  CControlListState::ControlListStateEnums m_listState;
  CProtocols::ProtocolsEnums m_currentProtocol;
  CStdString m_currentComputer;

};

#endif
