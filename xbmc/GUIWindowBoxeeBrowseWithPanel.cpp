
#include "GUIWindowBoxeeBrowseWithPanel.h"
#include "GUIWindowManager.h"
#include "utils/log.h"
#include "LocalizeStrings.h"

using namespace std;

#define GROUP_MENU 8000

#define BUTTON_SORT 110

CGUIWindowBoxeeBrowseWithPanel::CGUIWindowBoxeeBrowseWithPanel(DWORD dwID, const CStdString &xmlFile)
: CGUIWindowBoxeeBrowse(dwID, xmlFile)
{

}

CGUIWindowBoxeeBrowseWithPanel::~CGUIWindowBoxeeBrowseWithPanel()
{
}

void CGUIWindowBoxeeBrowseWithPanel::OnInitWindow()
{
  CGUIWindowBoxeeBrowse::OnInitWindow();

  //  Check whether the window was activated from main menu or home screen
  if (m_bShowPanel || g_windowManager.GetPreviousWindow() == WINDOW_HOME)
    SET_CONTROL_FOCUS(GROUP_MENU, 0);
}

void CGUIWindowBoxeeBrowseWithPanel::OnDeinitWindow(int nextWindowID)
{
  m_bShowPanel = false;
  m_iLastControl = 9000;
  CGUIWindowBoxeeBrowse::OnDeinitWindow(nextWindowID);
}

bool CGUIWindowBoxeeBrowseWithPanel::OnMessage(CGUIMessage& message)
{
  if (ProcessPanelMessages(message))
  {
    return true;
  }

  return CGUIWindowBoxeeBrowse::OnMessage(message);
}

bool CGUIWindowBoxeeBrowseWithPanel::ProcessPanelMessages(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();

    if (iControl == BUTTON_SORT)
    {
      if (m_windowState->OnSort())
        Refresh(true);
      return true;
    }
  }
  }

  return false;
}

void CGUIWindowBoxeeBrowseWithPanel::ShowPanel()
{
  m_bShowPanel = true;
}
