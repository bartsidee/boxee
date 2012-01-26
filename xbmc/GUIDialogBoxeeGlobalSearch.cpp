#include "GUIDialogBoxeeGlobalSearch.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "Application.h"

CGUIDialogBoxeeGlobalSearch::CGUIDialogBoxeeGlobalSearch(void) : CGUIDialogKeyboard(WINDOW_DIALOG_BOXEE_GLOBAL_SEARCH, "boxee_search_global.xml")
{

}

CGUIDialogBoxeeGlobalSearch::~CGUIDialogBoxeeGlobalSearch(void)
{

}

void CGUIDialogBoxeeGlobalSearch::OnInitWindow()
{
  CGUIDialogKeyboard::OnInitWindow();
}

void CGUIDialogBoxeeGlobalSearch::OnDeinitWindow(int nextWindowID)
{
  CGUIDialogKeyboard::OnDeinitWindow(nextWindowID);
}

bool CGUIDialogBoxeeGlobalSearch::ShowAndGetInput(CStdString& aTextString, const CStdString &strHeading, bool allowEmptyResult, bool hiddenInput /* = false */)
{
  CGUIDialogBoxeeGlobalSearch *pKeyboard = (CGUIDialogBoxeeGlobalSearch*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_GLOBAL_SEARCH);

  if (!pKeyboard)
  {
    return false;
  }

  // setup keyboard
  pKeyboard->Initialize();
  pKeyboard->SetHeading(strHeading);
  pKeyboard->SetHiddenInput(hiddenInput);
  pKeyboard->SetText(aTextString);

  // do this using a thread message to avoid render() conflicts
  ThreadMessage tMsg(TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_BOXEE_GLOBAL_SEARCH, g_windowManager.GetActiveWindow());
  g_application.getApplicationMessenger().SendMessage(tMsg, true);
  pKeyboard->Close();

  // If have text - update this.
  if (pKeyboard->IsConfirmed())
  {
    aTextString = pKeyboard->GetText();
    if (!allowEmptyResult && aTextString.IsEmpty())
    {
      return false;
    }

    return true;
  }
  else
  {
    return false;
  }
}

