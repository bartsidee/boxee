
#include "GUIDialogBoxeeManualResolveAlbum.h"
#include "GUIDialogBoxeeManualResolveResults.h"
#include "GUIWindowManager.h"
#include "Util.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIEditControl.h"
#include "GUIImage.h"

using namespace BOXEE;

#define ALBUM_COVER 5015
#define BUTTON_DONE 9010
#define BUTTON_PREV 9020

CGUIDialogBoxeeManualResolveAlbum::CGUIDialogBoxeeManualResolveAlbum() :
  CGUIDialog(WINDOW_DIALOG_BOXEE_MANUAL_RESOLVE_ALBUM, "boxee_manual_resolve_album.xml")
  {
    m_bConfirmed = false;
  }

CGUIDialogBoxeeManualResolveAlbum::~CGUIDialogBoxeeManualResolveAlbum() {
}

bool CGUIDialogBoxeeManualResolveAlbum::Show(CFileItemPtr pItem)
{
  CGUIDialogBoxeeManualResolveAlbum *pDialog = (CGUIDialogBoxeeManualResolveAlbum*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_MANUAL_RESOLVE_ALBUM);
  if (!pDialog)
  {
    return false;
  }

  // Copy the item into the dialog
  pItem->Dump();
  pDialog->m_albumItem = pItem;
  pDialog->DoModal();

  return pDialog->m_bConfirmed;
}

void CGUIDialogBoxeeManualResolveAlbum::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  // Send the item to the special container to allow skin acceess 
  CFileItemPtr itemPtr(new CFileItem(*m_albumItem.get()));
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), 5000, 0, 0, itemPtr);
  g_windowManager.SendThreadMessage(winmsg);

  m_albumItem->Dump();

  CLog::Log(LOGDEBUG, "CGUIDialogBoxeeManualResolveAlbum::OnInitWindow, confirm album, thumb path = %s (manual)", m_albumItem->GetThumbnailImage().c_str());
}

void CGUIDialogBoxeeManualResolveAlbum::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);

  CGUIImage* pImage = (CGUIImage*)GetControl(ALBUM_COVER);
  pImage->FreeResources();
}

bool CGUIDialogBoxeeManualResolveAlbum::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();
    if (iControl == BUTTON_PREV)
    {
      m_bConfirmed = false;
      Close();
      return true;
    }
    else if (iControl == BUTTON_DONE)
    {
      m_bConfirmed = true;
      Close();
      return true;
    }
  }
  break;
  } // switch
  return CGUIDialog::OnMessage(message);
}
