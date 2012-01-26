#include "GUIDialogFirstTimeUseWireless.h"

#ifdef HAS_EMBEDDED

#include "GUIDialogAccessPoints.h"
#include "GUIWindowManager.h"
#include "Settings.h"
#include "GUIDialogYesNo2.h"
#include "GUIDialogOK2.h"
#include "LocalizeStrings.h"
#include "Util.h"
#include "log.h"
#include "SingleLock.h"

#define CHECK_WIRELESS_NETWORKS_INTERVAL_IN_MS 5000    // 5 sec
#define SHOW_SEARCHING_LABEL_MIN_TIME_IN_SEC 2    // 2 sec

CGUIDialogFirstTimeUseWireless::CGUIDialogFirstTimeUseWireless() : CGUIDialogFirstTimeUseWithList(WINDOW_DIALOG_FTU_WIRELESS,"ftu_wireless.xml","CGUIDialogFirstTimeUseWireless")
{
  m_bgprocess.SetName("ftu_searchwireless");
  m_bgprocess.Start(1);
  m_wirelessNetworksVec.clear();
  m_currentNetworksLabelsSet.clear();
  m_needToLoadWirelessNetworksVec = false;
  m_buttonSelected = 0;
}

CGUIDialogFirstTimeUseWireless::~CGUIDialogFirstTimeUseWireless()
{
  m_bgprocess.Stop();

  // note: scanBgJob is deleted by m_bgprocess
}

void CGUIDialogFirstTimeUseWireless::OnInitWindow()
{
  SetProperty("show-list",true);

  CGUIDialogFirstTimeUseWithList::OnInitWindow();

  // run background job for search wireless networks
  scanBgJob = new CScanWirelessNetworksBG(this);
  m_bgprocess.QueueJob(scanBgJob);

  m_buttonSelected = 0;
}

void CGUIDialogFirstTimeUseWireless::OnDeinitWindow(int nextWindowID)
{
  if (scanBgJob)
  {
    scanBgJob->SignalStop();
    scanBgJob = NULL;
  }
}

bool CGUIDialogFirstTimeUseWireless::OnAction(const CAction &action)
{
  if (action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    if (scanBgJob)
    {
      scanBgJob->SignalStop();
      scanBgJob = NULL;
    }
  }

  return CGUIDialogFirstTimeUseWithList::OnAction(action);
}

bool CGUIDialogFirstTimeUseWireless::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    int senderId = message.GetSenderId();

    CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseWireless::OnMessage - GUI_MSG_CLICKED - [buttonId=%d] (initbox)",senderId);

    switch (senderId)
    {
    case JOIN_OTHER_NETWORKS_BUTTON:
    {
      m_actionChoseEnum = CActionChose::NEXT;
      m_buttonSelected = JOIN_OTHER_NETWORKS_BUTTON;
      Close();
      return true;
    }
    break;
    case SWITCH_TO_ETHERNET_BUTTON:
    {
      m_actionChoseEnum = CActionChose::NEXT;
      m_buttonSelected = SWITCH_TO_ETHERNET_BUTTON;
      Close();
      return true;
  }
    break;
  }
  }
  }

  return CGUIDialogFirstTimeUseWithList::OnMessage(message);
}

int CGUIDialogFirstTimeUseWireless::GetChoiceSelected()
{
  return m_buttonSelected;
}

void CGUIDialogFirstTimeUseWireless::Render()
{
  if (m_needToLoadWirelessNetworksVec)
  {
    m_needToLoadWirelessNetworksVec = false;
    LoadList();
  }

  CGUIDialogFirstTimeUseWithList::Render();
}

CStdString CGUIDialogFirstTimeUseWireless::GetSelectedWirelessName()
{
  CStdString SelectedWirelessName = "";

  if (m_selectedItem.get())
  {
    SelectedWirelessName = m_selectedItem->GetLabel();
  }
  else
  {
    CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseWireless::GetSelectedWirelessName - SelectedItem is EMPTY (initbox)");
  }

  return SelectedWirelessName;
}

bool CGUIDialogFirstTimeUseWireless::HandleClickNext()
{
  // nothing to do

  return true;
}

bool CGUIDialogFirstTimeUseWireless::HandleClickBack()
{
  // nothing to do

  return true;
}

bool CGUIDialogFirstTimeUseWireless::HandleListChoice()
{
  CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseWireless::HandleListChoice - [SelectedItem=%s] (initbox)",m_selectedItem->GetLabel().c_str());

  return true;
}

bool CGUIDialogFirstTimeUseWireless::FillListOnInit()
{
  m_wirelessNetworksVec.clear();
  m_currentNetworksLabelsSet.clear();
  m_needToLoadWirelessNetworksVec = false;

  // get the current wireless networks
  // this variable is static because this object could be deleted
  // before the the bg scan job is done
  static std::vector<CHalWirelessNetwork> wirelessNetworksVec;
  wirelessNetworksVec.clear();

  CWirelessScanBG* pJob = new CWirelessScanBG(wirelessNetworksVec);
  CUtil::RunInBG(pJob);

  SetWirelessNetworksVec(wirelessNetworksVec);
  LoadList();
  m_needToLoadWirelessNetworksVec = false;

  return true;
}

bool CGUIDialogFirstTimeUseWireless::LoadList()
{
  CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseWireless::LoadList - Enter function (initbox)");

  // get the current selected item label
  CStdString selectedLabel = "";
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), LIST_CTRL);
  OnMessage(msg);
  int index = msg.GetParam1();

  if (index >= 0 && index < m_listItems.Size())
  {
    selectedLabel = m_listItems.Get(index)->GetLabel();
  }

  // clear list before bind new items
  CGUIMessage message1(GUI_MSG_LABEL_RESET, GetID(), LIST_CTRL);
  OnMessage(message1);
  m_listItems.Clear();
  m_selectedIndex = -1;

  std::vector<CHalWirelessNetwork> wirelessNetworksVec;
  GetWirelessNetworksVec(wirelessNetworksVec);

  if ((int)wirelessNetworksVec.size() < 1)
  {
    SetProperty("show-list",false);
    CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseWireless::LoadList - Got [%d] wireless networks (initbox)",(int)wirelessNetworksVec.size());

    SET_CONTROL_FOCUS(255,0);
  }
  else
  {
    CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseWireless::FillList - Succeeded to find wireless networks. [NumOfWirelessNetworks=%d] (initbox)",(int)wirelessNetworksVec.size());

    for (int i=0; i<(int)wirelessNetworksVec.size(); i++)
    {
      CHalWirelessNetwork wn = wirelessNetworksVec[i];
      CFileItemPtr wnItem(new CFileItem(wn.ssid));

      wnItem->SetProperty("wln-ssid",wn.ssid);
      wnItem->SetProperty("wln-secure",wn.secure);
      wnItem->SetProperty("wln-signal_strength",wn.signal_strength);

      int signalStrength = wn.signal_strength;
      if (signalStrength <= 33)
      {
        wnItem->SetProperty("signalstrength","ap-signal1.png");
        wnItem->SetProperty("signalstrength-on","ap-signal1-on.png");
      }
      else if (signalStrength <= 66)
      {
        wnItem->SetProperty("signalstrength","ap-signal2.png");
        wnItem->SetProperty("signalstrength-on","ap-signal2-on.png");
      }
      else
      {
        wnItem->SetProperty("signalstrength","ap-signal3.png");
        wnItem->SetProperty("signalstrength-on","ap-signal3-on.png");
      }

      if (wn.secure)
      {
        wnItem->SetProperty("secure","ap-lock.png");
        wnItem->SetProperty("secure-on","ap-lock-on.png");
      }

      m_listItems.Add(wnItem);
    }

    SetProperty("show-list",true);

    // bind new items
    CGUIMessage message2(GUI_MSG_LABEL_BIND, GetID(), LIST_CTRL, 0, 0, &m_listItems);
    OnMessage(message2);

    // set last focused item
    if (!selectedLabel.IsEmpty())
    {
      for (int i=0; i<m_listItems.Size(); i++)
      {
        if (selectedLabel == m_listItems.Get(i)->GetLabel())
        {
          CGUIMessage message(GUI_MSG_ITEM_SELECT, GetID(), LIST_CTRL, i);
          OnMessage(message);
          break;
        }
      }
    }
  }

  CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseWireless::LoadList - Exit function. [NumOfWirelessNetworks=%d] (initbox)",m_listItems.Size());

  return true;
}

void CGUIDialogFirstTimeUseWireless::SetWirelessNetworksVec(const std::vector<CHalWirelessNetwork>& wirelessNetworksVec)
{
  bool updateNetworks = m_currentNetworksLabelsSet.empty() ? true : false;

  for (int i=0; !updateNetworks && i<(int)wirelessNetworksVec.size(); i++)
  {
    if (m_currentNetworksLabelsSet.find(wirelessNetworksVec[i].ssid) == m_currentNetworksLabelsSet.end())
    {
      updateNetworks = true;
    }
  }

  if (updateNetworks)
  {
    CSingleLock lock(m_lock);
    m_wirelessNetworksVec = wirelessNetworksVec;
    m_needToLoadWirelessNetworksVec = true;

    m_currentNetworksLabelsSet.clear();
    for (int i=0; i<(int)wirelessNetworksVec.size(); i++)
    {
      m_currentNetworksLabelsSet.insert(m_wirelessNetworksVec[i].ssid);
    }
  }
}

void CGUIDialogFirstTimeUseWireless::GetWirelessNetworksVec(std::vector<CHalWirelessNetwork>& wirelessNetworksVec)
{
  CSingleLock lock(m_lock);
  wirelessNetworksVec = m_wirelessNetworksVec;
}

CGUIDialogFirstTimeUseWireless::CScanWirelessNetworksBG::CScanWirelessNetworksBG(CGUIDialogFirstTimeUseWireless* pHandler) : BXBGJob("ScanWirelessNetworksJob")
{
  m_pLock = SDL_CreateMutex();
  m_pSleepCond = SDL_CreateCond();

  m_pHandler = pHandler;
  m_shouldStop = false;
}

CGUIDialogFirstTimeUseWireless::CScanWirelessNetworksBG::~CScanWirelessNetworksBG()
{
  if (m_pSleepCond)
  {
    SDL_DestroyCond(m_pSleepCond);
  }

  if (m_pLock)
  {
    SDL_DestroyMutex(m_pLock);
  }
}

void CGUIDialogFirstTimeUseWireless::CScanWirelessNetworksBG::SignalStop()
{
  m_shouldStop = true;
  SDL_CondSignal(m_pSleepCond);
}

void CGUIDialogFirstTimeUseWireless::CScanWirelessNetworksBG::DoWork()
{
  if (!m_pHandler)
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseWireless::CScanWirelessNetworksBG::Run - FAILED to execute because pHandler is NULL (wireless)");
    m_shouldStop = true;
    return;
  }

  while (1)
  {
    SDL_LockMutex(m_pLock);
    if (SDL_CondWaitTimeout(m_pSleepCond, m_pLock, CHECK_WIRELESS_NETWORKS_INTERVAL_IN_MS) == -1)
    {
      CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseWireless::CScanWirelessNetworksBG::Run - Woke up with an error (wireless)");
    }

    if (m_shouldStop)
    {
      break;
    }

    time_t startScan = time(NULL);

    m_pHandler->SetProperty("is-searching",true);

    std::vector<CHalWirelessNetwork> wirelessNetworksVec;
    bool succeeded = CHalServicesFactory::GetInstance().SearchWireless(0, wirelessNetworksVec);

    time_t stopScan = time(NULL);

    // show the searching label for SHOW_SEARCHING_LABEL_MIN_TIME_IN_SEC
    long diffTime = stopScan - startScan;
    if (diffTime < SHOW_SEARCHING_LABEL_MIN_TIME_IN_SEC)
    {
      sleep(SHOW_SEARCHING_LABEL_MIN_TIME_IN_SEC - diffTime);
    }

    m_pHandler->SetProperty("is-searching",false);

    if (m_shouldStop)
    {
      break;
    }

    if (succeeded)
    {
      m_pHandler->SetWirelessNetworksVec(wirelessNetworksVec);
    }
    else
    {
      CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseWireless::CScanWirelessNetworksBG::Run - FAILED to get wireless networks (wireless)");
    }
  }
}

#endif

