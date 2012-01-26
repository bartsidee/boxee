#include "GUIWindowBoxeeWizardTimezone.h"
#include "Application.h"

CGUIWindowBoxeeWizardTimezone::CGUIWindowBoxeeWizardTimezone(void)
    : CGUIDialog(WINDOW_BOXEE_WIZARD_TIMEZONE, "boxee_wizard_timezone.xml")
{
}

CGUIWindowBoxeeWizardTimezone::~CGUIWindowBoxeeWizardTimezone(void)
{}


bool CGUIWindowBoxeeWizardTimezone::OnAction(const CAction &action)
{
   if (action.id == ACTION_PREVIOUS_MENU)
   {
      Close();
   }
   
   return CGUIWindow::OnAction(action);
}

void CGUIWindowBoxeeWizardTimezone::OnInitWindow()
{
  CGUIWindow::OnInitWindow();
}  
