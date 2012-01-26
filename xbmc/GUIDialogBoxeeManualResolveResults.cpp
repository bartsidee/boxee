
#include "GUIDialogBoxeeManualResolveResults.h"
#include "GUIWindowManager.h"
#include "Util.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "tinyXML/tinyxml.h"
#include "MetadataResolverVideo.h"
#include "BoxeeUtils.h"

using namespace BOXEE;

#define RESULT_LIST 9234

CGUIDialogBoxeeManualResolveResults::CGUIDialogBoxeeManualResolveResults() :
  CGUIDialog(WINDOW_DIALOG_BOXEE_MANUAL_RESULTS, "boxee_manual_resolve_results.xml")
  {
  }

CGUIDialogBoxeeManualResolveResults::~CGUIDialogBoxeeManualResolveResults()
{
}

void CGUIDialogBoxeeManualResolveResults::Show(CFileItemPtr pItem)
{
  CGUIDialogBoxeeManualResolveResults *pDialog = (CGUIDialogBoxeeManualResolveResults*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_MANUAL_RESULTS);
  if (pDialog)
  {
    // Copy the item into the dialog
    pDialog->m_VideoItem = pItem;
    pDialog->DoModal();
  }
}

void CGUIDialogBoxeeManualResolveResults::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  // Send the item to the special container to allow skin acceess 
  CFileItemPtr itemPtr(new CFileItem(*m_VideoItem.get()));
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), 5000, 0, 0, itemPtr);
  g_windowManager.SendThreadMessage(winmsg);

  }

void CGUIDialogBoxeeManualResolveResults::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
}

bool CGUIDialogBoxeeManualResolveResults::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();
  }
  break;
  } // switch
  return CGUIDialog::OnMessage(message);
}

