#pragma once

#include "GUIDialogFirstTimeUseWithList.h"

#ifdef HAS_EMBEDDED

#include "HalServices.h"
#include "lib/libBoxee/bxbgprocess.h"

#include <set>

#define JOIN_OTHER_NETWORKS_BUTTON     255
#define SWITCH_TO_ETHERNET_BUTTON      275

class CGUIDialogFirstTimeUseWireless : public CGUIDialogFirstTimeUseWithList
{
public:
  
  CGUIDialogFirstTimeUseWireless();
  virtual ~CGUIDialogFirstTimeUseWireless();

  virtual bool OnMessage(CGUIMessage &message);

  virtual void Render();

  CStdString GetSelectedWirelessName();

  int GetChoiceSelected();

protected:

  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

  virtual bool OnAction(const CAction &action);

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

  virtual bool FillListOnInit();
  virtual bool LoadList();
  virtual bool HandleListChoice();

  void SetWirelessNetworksVec(const std::vector<CHalWirelessNetwork>& wirelessNetworksVec);
  void GetWirelessNetworksVec(std::vector<CHalWirelessNetwork>& wirelessNetworksVec);

private:

  class CScanWirelessNetworksBG : public BOXEE::BXBGJob
  {
  public:
    CScanWirelessNetworksBG(CGUIDialogFirstTimeUseWireless* pHandler);
    virtual ~CScanWirelessNetworksBG();
    virtual void DoWork();

    void SignalStop();

  private:

    SDL_cond* m_pSleepCond;
    SDL_mutex* m_pLock;

    bool m_shouldStop;
    CGUIDialogFirstTimeUseWireless* m_pHandler;
  };

  int m_buttonSelected;

  CCriticalSection m_lock;
  CScanWirelessNetworksBG* scanBgJob;

  std::vector<CHalWirelessNetwork> m_wirelessNetworksVec;
  std::set<CStdString> m_currentNetworksLabelsSet;
  bool m_needToLoadWirelessNetworksVec;
  BOXEE::BXBGProcess m_bgprocess;
};

#endif

