#ifndef INITIALIZEBOXMANAGER_H_
#define INITIALIZEBOXMANAGER_H_

#include "system.h"

#ifdef HAS_EMBEDDED

#include "HalServices.h"
#include "FileItem.h"
#include "Thread.h"
#include <stack>

class CGUIDialogFirstTimeUseBase;
class CGUIDialogFirstTimeUseResolution;
class CGUIDialogFirstTimeUseEthernet;
class CGUIDialogFirstTimeUseWireless;
class CGUIDialogFirstTimeUseConfWirelessPassword;
class CGUIDialogFirstTimeUseNetworkMessage;
class CGUIDialogProgress;

class CUpdateVersionStatus
{
public:
  enum UpdateVersionStatusEnums
  {
    NO_UPDATE=0,
    HAS_UPDATE=1,
    UPDATING=2,
    UPDATE_SUCCEEDED=3,
    UPDATE_FAILED=4,
    REQUEST_UPGRADE=5,
    NUM_OF_SERVICE_IDENTIFIER_TYPES=6
  };
};

class CInitializeBoxManager
{
public:

  CInitializeBoxManager();
  virtual ~CInitializeBoxManager();

  static CInitializeBoxManager& GetInstance();

  bool Run(bool runFromSettings = false);

  bool IsConnectViaEthernet();

  CUpdateVersionStatus::UpdateVersionStatusEnums GetUpdateStatus();

  void Reboot();
  void RequestUpgrade();

protected:

  CGUIDialogFirstTimeUseBase* GetNextDialog(CGUIDialogFirstTimeUseBase* pDialog);
  CGUIDialogFirstTimeUseBase* HandleNextAction(CGUIDialogFirstTimeUseBase* pDialog);
  CGUIDialogFirstTimeUseBase* HandleBackAction(CGUIDialogFirstTimeUseBase* pDialog);

  // wellcome dialog
  bool HandleNextWellcomeDialog(CGUIDialogFirstTimeUseBase** pNextDialog, CHalAddrType addrType, bool onRetry = false);

  // network message dialog
  bool HandleNextNetworkMessageDialog(CGUIDialogFirstTimeUseBase** pNextDialog);

  // wireless dialog
  bool HandleNextWirelessDialog(CGUIDialogFirstTimeUseBase** pNextDialog);

  // conf wireless dialog
  bool HandleNextConfWirelessPasswordDialog(CGUIDialogFirstTimeUseBase** pNextDialog);

  bool HandleNextConfWirelessSsidDialog(CGUIDialogFirstTimeUseBase** pNextDialog);

  bool HandleNextConfWirelessSecurityDialog(CGUIDialogFirstTimeUseBase** pNextDialog);

  bool HandleNextConfNetworkDialog(CGUIDialogFirstTimeUseBase** pNextDialog);

  // update message dialog
  bool HandleNextUpdateMessageDialog(CGUIDialogFirstTimeUseBase** pNextDialog);

  // update progress dialog
  bool HandleNextUpdateProgressDialog(CGUIDialogFirstTimeUseBase** pNextDialog);

  // simple message dialog
  bool HandleNextSimpleMessageDialog(CGUIDialogFirstTimeUseBase** pNextDialog);

  bool IsEthernetConnected();
  bool ConnectToEthernet(unsigned int instance, CHalAddrType addrType);
  bool ConnectToWireless(unsigned int instance, CHalAddrType addrType, const CStdString& password, CStdString& errorMessage);

  IHalServices* m_hal;
  bool m_isConnectViaEthernet;
  std::stack<int> m_dialogStack;

private:

  bool CheckForNewVersion();

  bool GetWirelessNetworkToConnectItem(CFileItem& wlItem, const CStdString& password);

  bool PostFtuFromSettings();

  bool PostFtuInitialization();
  bool GetDataFromServer();

  enum NetworkConnectionStatus
  {
    NETWORK_DOWN = 0,
    NO_INTERNET_CONNECTION,
    CONNECTED_TO_INTERNET
  };

  NetworkConnectionStatus HasInternetConnection(int connectionType, int numOfIntervals = 0);
  bool StartDialogProgress(CGUIDialogProgress** pDialogProgress);
  bool StopDialogProgress(CGUIDialogProgress* pDialogProgress);

  bool m_loginViaCustomWirelessNetwork;
  bool m_initCompleted;
  bool m_updateCompleted;

  bool m_runFromSettings;

  CUpdateVersionStatus::UpdateVersionStatusEnums m_updateVersionStatus;

  class CReadDataFromServer : public IRunnable
  {
  public:
    CReadDataFromServer(){}
    virtual void Run();
  };
};

#endif

#endif /*INITIALIZEBOXMANAGER_H_*/

