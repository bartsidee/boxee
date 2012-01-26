#pragma once

#include "system.h"

#ifdef HAS_DVB

#include "GUIDialog.h"

class CGUIDialogBoxeeLiveTvScan : public CGUIDialog
{
public:
  CGUIDialogBoxeeLiveTvScan();
  virtual ~CGUIDialogBoxeeLiveTvScan();
  virtual void OnInitWindow();
  virtual bool OnAction(const CAction& action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();

  bool RequestedGoToLiveTv() { return m_requestGoToLiveTv; }
  bool NoChannelsFound() { return m_noChannelsFound; }


private:
  void HideAll();
  void StartScan();
  void CancelScan();
  void ShowScanning();
  void ShowNoChannels();
  void ShowNoInternet();
  void ShowDone();
  void ConfirmCancel();

  bool m_requestGoToLiveTv;
  bool m_noChannelsFound;
  bool m_scanStarted;
};

#endif
