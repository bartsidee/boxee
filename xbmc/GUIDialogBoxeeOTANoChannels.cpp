#include "GUIDialogBoxeeOTANoChannels.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUISettings.h"

#define LEFT_SIDE_LABEL  1
#define RESCAN_BUTTON    2
#define SWITCH_BUTTON    3

CGUIDialogBoxeeOTANoChannels::CGUIDialogBoxeeOTANoChannels()
  : CGUIDialog(WINDOW_OTA_NO_CHANNELS, "boxee_livetv_no_channels.xml")
{
}

CGUIDialogBoxeeOTANoChannels::~CGUIDialogBoxeeOTANoChannels()
{
}

void CGUIDialogBoxeeOTANoChannels::OnInitWindow()
{
  m_isSwitchConnection = false;
  m_isRescan = false;

  CGUIDialog::OnInitWindow();

  bool isCable = g_guiSettings.GetBool("ota.selectedcable");

  if (isCable)
  {
    SET_CONTROL_LABEL(LEFT_SIDE_LABEL, g_localizeStrings.Get(58092));
    SET_CONTROL_LABEL(SWITCH_BUTTON, g_localizeStrings.Get(58094));
  }
}

bool CGUIDialogBoxeeOTANoChannels::OnAction(const CAction& action)
{
  if (action.id == ACTION_SELECT_ITEM)
  {
    int iControl = GetFocusedControlID();

    if (iControl == SWITCH_BUTTON)
    {
      m_isSwitchConnection = true;
      Close();
      return true ;
    }
    else if (iControl == RESCAN_BUTTON)
    {
      m_isRescan = true;
      Close();
      return true;
    }
  }
  else if (action.id == ACTION_PARENT_DIR || action.id == ACTION_PREVIOUS_MENU)
  {
    Close();
    return true;
  }

  return CGUIDialog::OnAction(action);
}
