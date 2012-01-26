#include "GUIDialogBoxeeLiveTvScan.h"

#ifdef HAS_DVB

#include "GUIWindowManager.h"
#include "GUIDialogProgress.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "dvbmanager.h"
#include "GUISettings.h"
#include "Settings.h"
#include "Application.h"

#define CONTROL_GROUP_SCANNING          50
#define CONTROL_LABEL_SCANNING          51
#define CONTROL_PROGRESS_SCANNING       52
#define CONTROL_GROUP_SCAN_OK           60
#define CONTROL_LABEL_CHANNEL_COUNT     61
#define CONTROL_BUTTON_GO_TO_LIVETV     62
#define CONTROL_GROUP_SCAN_NO_CHANNELS  70
#define CONTROL_BUTTON_TRY_AGAIN        71
#define CONTROL_GROUP_CANCEL_SCAN       80
#define CONTROL_BUTTON_CONFIRM_CANCEL   81
#define CONTROL_GROUP_NO_INTERNET       90
#define CONTROL_BUTTON_CLOSE            91

CGUIDialogBoxeeLiveTvScan::CGUIDialogBoxeeLiveTvScan() : CGUIDialog(WINDOW_OTA_SCANNING, "boxee_livetv_scan.xml")
{
}

CGUIDialogBoxeeLiveTvScan::~CGUIDialogBoxeeLiveTvScan()
{
}

void CGUIDialogBoxeeLiveTvScan::OnInitWindow()
{
  g_guiSettings.SetBool("ota.scanned", false);
  g_settings.Save();

  m_requestGoToLiveTv = false;
  m_scanStarted = false;
  m_noChannelsFound = false;

  CGUIDialog::OnInitWindow();

  if (!g_application.IsConnectedToInternet())
  {
    ShowNoInternet();
  }
}

bool CGUIDialogBoxeeLiveTvScan::OnAction(const CAction& action)
{
  if (action.id != ACTION_PREVIOUS_MENU)
  {
    return CGUIDialog::OnAction(action);
  }

  // Cancel scan?
  if (GetControl(CONTROL_GROUP_SCANNING)->IsVisible())
  {
    ConfirmCancel();
    return true;
  }
  // Don't cancel, show scanning progress
  else if (GetControl(CONTROL_GROUP_CANCEL_SCAN)->IsVisible())
  {
    ShowScanning();
    return true;
  }
  else
  {
    Close();
    return true;
  }

  return CGUIDialog::OnAction(action);
}


bool CGUIDialogBoxeeLiveTvScan::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() != GUI_MSG_CLICKED)
    return CGUIDialog::OnMessage(message);

  switch (message.GetSenderId())
  {
  case CONTROL_BUTTON_CONFIRM_CANCEL:
    CancelScan();
    return true;
    break;

  case CONTROL_BUTTON_GO_TO_LIVETV:
    Close();
    m_requestGoToLiveTv = true;
    return true;
    break;

  case CONTROL_BUTTON_TRY_AGAIN:
    StartScan();
    return true;
    break;

  case CONTROL_BUTTON_CLOSE:
    Close();
    return true;
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogBoxeeLiveTvScan::HideAll()
{
  SET_CONTROL_HIDDEN(CONTROL_GROUP_SCANNING);
  SET_CONTROL_HIDDEN(CONTROL_GROUP_CANCEL_SCAN);
  SET_CONTROL_HIDDEN(CONTROL_GROUP_SCAN_NO_CHANNELS);
  SET_CONTROL_HIDDEN(CONTROL_GROUP_SCAN_OK);
  SET_CONTROL_HIDDEN(CONTROL_GROUP_NO_INTERNET);
}

void CGUIDialogBoxeeLiveTvScan::CancelScan()
{
  CGUIDialogProgress* progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  progress->StartModal();
  progress->Progress();

  if (!DVBManager::GetInstance().CancelScan())
  {
    CLog::Log(LOGERROR, "CGUIDialogBoxeeLiveTvScan::CancelScan failed to cancel scan");
    return;
  }
}

void CGUIDialogBoxeeLiveTvScan::StartScan()
{
  ShowScanning();

  if (!DVBManager::GetInstance().StartScan())
  {
    CLog::Log(LOGERROR, "CGUIDialogBoxeeLiveTvScan::StartScan failed to start scan");
    return;
  }
}

void CGUIDialogBoxeeLiveTvScan::ConfirmCancel()
{
  HideAll();
  SET_CONTROL_VISIBLE(CONTROL_GROUP_CANCEL_SCAN);
  SET_CONTROL_FOCUS(CONTROL_BUTTON_CONFIRM_CANCEL, 0);
}

void CGUIDialogBoxeeLiveTvScan::ShowScanning()
{
  HideAll();
  SET_CONTROL_VISIBLE(CONTROL_GROUP_SCANNING);
  SET_CONTROL_FOCUS(CONTROL_PROGRESS_SCANNING, 0);
}

void CGUIDialogBoxeeLiveTvScan::ShowNoChannels()
{
  m_noChannelsFound = true;
  Close();
}

void CGUIDialogBoxeeLiveTvScan::ShowNoInternet()
{
  HideAll();
  SET_CONTROL_VISIBLE(CONTROL_GROUP_NO_INTERNET);
  SET_CONTROL_FOCUS(CONTROL_BUTTON_CLOSE, 0);
}

void CGUIDialogBoxeeLiveTvScan::ShowDone()
{
  DVBManager::GetInstance().ClearScanner();

  int channelCount = DVBManager::GetInstance().GetChannels().Size();

  if (channelCount > 0)
  {
    HideAll();
    SET_CONTROL_VISIBLE(CONTROL_GROUP_SCAN_OK);

    CStdString str;
    str.Format(g_localizeStrings.Get(58003), channelCount);
    SET_CONTROL_LABEL(CONTROL_LABEL_CHANNEL_COUNT, str);

    SET_CONTROL_FOCUS(CONTROL_BUTTON_GO_TO_LIVETV, 0);
  }
  else
  {
    ShowNoChannels();
  }
}

void CGUIDialogBoxeeLiveTvScan::Render()
{
  if (!g_application.IsConnectedToInternet())
  {
    CGUIDialog::Render();
    return;
  }

  // No screen saver should appear when in this dialog
  g_application.ResetScreenSaver();

  static Uint32 lastCheck1 = SDL_GetTicks();
  static Uint32 lastCheck2 = SDL_GetTicks();
  static DvbScanner::ScanState lastScanState = DvbScanner::SCAN_NOT_RUNNING;

  Uint32 now = SDL_GetTicks();

  if (now - lastCheck1 < 500)
  {
    CGUIDialog::Render();
    return;
  }

  lastCheck1 = now;

  // Animate the ellipsis
  static int nofDots = 3;
  static const char* dotsStr[] = { "", ".", "..", "..." };

  if (GetControl(CONTROL_LABEL_SCANNING)->IsVisible())
  {
    CStdString label;
    nofDots = (nofDots + 1) % 4;
    label.Format("[B]%s%s[/B]", g_localizeStrings.Get(58001).c_str(), dotsStr[nofDots]);
    SET_CONTROL_LABEL(CONTROL_LABEL_SCANNING, label);
  }

  if (now - lastCheck2 < 2000)
  {
    CGUIDialog::Render();
    return;
  }

  lastCheck2 = now;

  if (!m_scanStarted)
  {
    m_scanStarted = true;
    StartScan();
  }


  DvbScanner::ScanState scanState = DVBManager::GetInstance().GetScanState();
  if (lastScanState == scanState)
  {
    CGUIDialog::Render();
    return;
  }

  lastScanState = scanState;

  CLog::Log(LOGDEBUG, "CGUIDialogBoxeeLiveTvScan::Render switched to scan state=%d", lastScanState);

  switch (scanState)
  {
  case DvbScanner::SCAN_STARTING:
  case DvbScanner::SCAN_RUNNING:
  {
    if (!GetControl(CONTROL_GROUP_CANCEL_SCAN))
      ShowScanning();
  }
  break;
  case DvbScanner::SCAN_FAILED:
  {
    ShowNoChannels();
  }
  break;
  case DvbScanner::SCAN_CANCELED:
  {
    DVBManager::GetInstance().ClearScanner();
    g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS)->Close();
    Close();
    return;
  }
  break;
  case DvbScanner::SCAN_DONE:
  {
    ShowDone();
  }
  break;
  default:
  {
    // do nothing
  }
  break;
  }

  CGUIDialog::Render();
}

#endif
