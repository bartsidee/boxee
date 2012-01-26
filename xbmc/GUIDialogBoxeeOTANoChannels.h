#pragma once

#include "GUIDialog.h"

class CGUIDialogBoxeeOTANoChannels : public CGUIDialog
{
public:
  CGUIDialogBoxeeOTANoChannels();
  virtual ~CGUIDialogBoxeeOTANoChannels();
  virtual void OnInitWindow();
  virtual bool OnAction(const CAction& action);

  bool IsSwitchConnection() { return m_isSwitchConnection; }
  bool IsRescan() { return m_isRescan; }

private:
  bool m_isSwitchConnection;
  bool m_isRescan;
};
