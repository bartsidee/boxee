#ifndef GUI_WINDOW_BOXEE_WIZARD_NETWORK
#define GUI_WINDOW_BOXEE_WIZARD_NETWORK

#pragma once

#include <vector>
#include "GUIDialog.h"

#ifdef _LINUX
#include "NetworkLinux.h"
#endif


class CGUIWindowBoxeeWizardNetwork : public CGUIDialog
{
public:
  CGUIWindowBoxeeWizardNetwork(void);      
  virtual ~CGUIWindowBoxeeWizardNetwork(void);
  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);

private:
  void ResetCurrentNetworkState();
  void ShowInterfaces();
  void ShowWirelessNetworks(CNetworkInterface* interface);
  void ShowWirelessNetworksIfNeeded();
  void ShowPasswordIfNeeded();
  bool NetworkConfigurationChanged();
  bool SaveConfiguration();
  void GetUserConfiguration(CStdString& interfaceName, CStdString& essId, CStdString& key, EncMode& enc);
  bool IsHexString(CStdString& str);
  
  std::vector<CFileItemPtr> m_interfaceItems;
  std::vector<CFileItemPtr> m_networkItems;
  std::vector<CNetworkInterface*> m_interfaces;
  std::vector<NetworkAccessPoint> m_aps;
  
  // Information taken when the wizard is initialized to
  // later check if saving is required
  NetworkAssignment m_assignment;
  CStdString m_ipAddress;
  CStdString m_networkMask;
  CStdString m_defaultGateway;
  CStdString m_essId;
  CStdString m_key;
  EncMode m_encryptionMode;
  CStdString m_interfaceName;
  bool m_foundCurrnetNetwork;
};

#endif
