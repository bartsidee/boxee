
#include "GUIDialogBoxeeManualResolveMovie.h"
#include "GUIDialogBoxeeManualResolve.h"
#include "GUIDialogBoxeeManualResolveResults.h"
#include "GUIDialogBoxeeManualResolveAddFiles.h"
#include "MetadataResolverVideo.h"
#include "GUIWindowManager.h"
#include "Util.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "GUIEditControl.h"
#include "GUIImage.h"

using namespace BOXEE;

#define BUTTON_DONE 9010
#define BUTTON_PREV 9020
#define BUTTON_ADD_FILES  9030

#define MOVIE_TITLE_EDIT 5010
#define MOVIE_COVER 5015

CGUIDialogBoxeeManualResolveMovie::CGUIDialogBoxeeManualResolveMovie() :
  CGUIDialog(WINDOW_DIALOG_BOXEE_MANUAL_MOVIE, "boxee_manual_resolve_movie.xml")
  {
    m_bConfirmed = false;
  }

CGUIDialogBoxeeManualResolveMovie::~CGUIDialogBoxeeManualResolveMovie() {
}

bool CGUIDialogBoxeeManualResolveMovie::Show(CFileItemPtr pItem, CFileItemList& videoParts)
{
  CGUIDialogBoxeeManualResolveMovie *pDialog = (CGUIDialogBoxeeManualResolveMovie*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_MANUAL_MOVIE);
  if (!pDialog)
  {
    return false;
  }

    // Copy the item into the dialog
  pDialog->m_videoItem = pItem;
  pDialog->m_videoParts = videoParts;
  pDialog->m_bConfirmed = false;

  pDialog->DoModal();

  videoParts = pDialog->m_videoParts;

  return pDialog->m_bConfirmed;
}

void CGUIDialogBoxeeManualResolveMovie::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  // Send the item to the special container to allow skin acceess 
  CFileItemPtr itemPtr(new CFileItem(*m_videoItem.get()));
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), 5000, 0, 0, itemPtr);
  g_windowManager.SendThreadMessage(winmsg);

  m_videoItem->Dump();
  m_bConfirmed = false;

  CLog::Log(LOGDEBUG, "CGUIDialogBoxeeManualResolveMovie::OnInitWindow, confirm movie, thumb path = %s (manual)", m_videoItem->GetThumbnailImage().c_str());
}

void CGUIDialogBoxeeManualResolveMovie::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);

  CGUIImage* pImage = (CGUIImage*)GetControl(MOVIE_COVER);
  pImage->FreeResources();
}

bool CGUIDialogBoxeeManualResolveMovie::OnMessage(CGUIMessage& message)
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
    }
    else if (iControl == BUTTON_DONE)
    {
      m_bConfirmed = true;
      Close();
    }
    else if (iControl == BUTTON_ADD_FILES)
    {
      // Open the Add Files dialog to add more parts to the movie
      CGUIDialogBoxeeManualResolveAddFiles::Show(m_videoItem, m_videoParts);
    }
  }
  break;
  } // switch
  return CGUIDialog::OnMessage(message);
}
