#include "stdafx.h"
#include "GUIWindowBoxeeWizardNetworkManual.h"
#include "Application.h"
#include "GUIListContainer.h"

CGUIWindowBoxeeWizardNetworkManual::CGUIWindowBoxeeWizardNetworkManual(void)
    : CGUIDialog(WINDOW_BOXEE_WIZARD_NETWORK_MANUAL, "boxee_wizard_network_manual.xml")
{
}

CGUIWindowBoxeeWizardNetworkManual::~CGUIWindowBoxeeWizardNetworkManual(void)
{}


bool CGUIWindowBoxeeWizardNetworkManual::OnAction(const CAction &action)
{

   if (action.wID == ACTION_PREVIOUS_MENU)
   {
      Close();
   }
   
   return CGUIWindow::OnAction(action);
}

void CGUIWindowBoxeeWizardNetworkManual::OnInitWindow()
{
  CGUIWindow::OnInitWindow();
}  
