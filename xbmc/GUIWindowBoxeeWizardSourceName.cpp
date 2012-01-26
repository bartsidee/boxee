#include "GUIWindowBoxeeWizardSourceName.h"
#include "Application.h"
#include "GUIButtonControl.h"
#include "GUIDialogKeyboard.h"

#include "Util.h"

#define CONTROL_SOURCE_NAME 9001
#define CONTROL_CANCEL      9003
#define CONTROL_OK          9004

#define CONTROL_SHARE_NAME  9200

CGUIWindowBoxeeWizardSourceName::CGUIWindowBoxeeWizardSourceName(void)
    : CGUIDialog(WINDOW_BOXEE_WIZARD_SOURCE_NAME, "boxee_wizard_set_source_name.xml")
{
}

CGUIWindowBoxeeWizardSourceName::~CGUIWindowBoxeeWizardSourceName(void)
{}


void CGUIWindowBoxeeWizardSourceName::SetSourcePath(const CStdString &strPath)
{
  m_sourcePath = strPath;
}

bool CGUIWindowBoxeeWizardSourceName::OnAction(const CAction &action)
{
   int iControl = GetFocusedControlID();

   bool bSelectAction = ((action.id == ACTION_SELECT_ITEM) || (action.id == ACTION_MOUSE_LEFT_CLICK));
   
   if (bSelectAction && iControl == CONTROL_SOURCE_NAME)
   {
      CGUIButtonControl* sourceNameButton = (CGUIButtonControl*) GetControl(iControl);
      CStdString sourceName = sourceNameButton->GetLabel();
      if (CGUIDialogKeyboard::ShowAndGetInput(sourceName, "", false))
      {
         sourceNameButton->SetLabel(sourceName);
         m_sourceName = sourceName;
         CONTROL_ENABLE(CONTROL_OK);
         SET_CONTROL_FOCUS(CONTROL_OK, 0);
      }
      return true;
   }
   else if (bSelectAction && iControl == CONTROL_OK)
   {
      if (GetSourceName() != "")
      {
         m_confirmed = true;
         Close();
      }
      
      return true;
   }
   else if (action.id == ACTION_PREVIOUS_MENU || (bSelectAction && iControl == CONTROL_CANCEL))
   {
      Close();
      return true;
   }
   
   return CGUIWindow::OnAction(action);
}

void CGUIWindowBoxeeWizardSourceName::OnInitWindow()
{
  SET_CONTROL_LABEL(CONTROL_SHARE_NAME, m_sourcePath);
  m_sourceName = CUtil::GetTitleFromPath(m_sourcePath, true);
  SET_CONTROL_LABEL(CONTROL_SOURCE_NAME, m_sourceName);
  CGUIWindow::OnInitWindow();
  m_confirmed = false;
}  

CStdString CGUIWindowBoxeeWizardSourceName::GetSourceName()
{
   return m_sourceName;
}

bool CGUIWindowBoxeeWizardSourceName::IsConfirmed()
{
   return m_confirmed;
}

