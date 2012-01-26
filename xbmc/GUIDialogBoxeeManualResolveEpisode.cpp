
#include "GUIDialogBoxeeManualResolveEpisode.h"
#include "GUIWindowManager.h"
#include "Util.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIEditControl.h"
#include "GUIRadioButtonControl.h"
#include "lib/libBoxee/bxutils.h"
#include "VideoInfoTag.h"

using namespace BOXEE;

#define BUTTON_PREV 9020
#define BUTTON_DONE 9010

#define SEASON_EDIT 5010
#define EPISODE_EDIT 5020
#define APPLY_TO_FOLDER_RADIO 5030

CGUIDialogBoxeeManualResolveEpisode::CGUIDialogBoxeeManualResolveEpisode() :
  CGUIDialog(WINDOW_DIALOG_BOXEE_MANUAL_EPISODE, "boxee_manual_resolve_episode.xml")
  {
  }

CGUIDialogBoxeeManualResolveEpisode::~CGUIDialogBoxeeManualResolveEpisode() {
}

bool CGUIDialogBoxeeManualResolveEpisode::Show(CFileItemPtr pItem)
{
  CGUIDialogBoxeeManualResolveEpisode *pDialog = (CGUIDialogBoxeeManualResolveEpisode*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_MANUAL_EPISODE);
  if (!pDialog)
  {
    return false;
  }

  // Copy the item into the dialog
  pDialog->m_videoItem = pItem;

  pDialog->DoModal();

  return pDialog->m_bConfirmed;

  }

void CGUIDialogBoxeeManualResolveEpisode::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  // Send the item to the special container to allow skin acceess 
  CFileItemPtr itemPtr(new CFileItem(*m_videoItem.get()));
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), 5000, 0, 0, itemPtr);
  g_windowManager.SendThreadMessage(winmsg);

  // Initialize the edit boxes
  CStdString strSeason;
  strSeason.Format("%d", m_videoItem->GetVideoInfoTag()->m_iSeason >= 0 ? m_videoItem->GetVideoInfoTag()->m_iSeason : 0);
  ((CGUIEditControl*)GetControl(SEASON_EDIT))->SetLabel2(strSeason);

  CStdString strEpisode;
  strEpisode.Format("%d", m_videoItem->GetVideoInfoTag()->m_iEpisode >= 0 ? m_videoItem->GetVideoInfoTag()->m_iEpisode : 0);
  ((CGUIEditControl*)GetControl(EPISODE_EDIT))->SetLabel2(strEpisode);

  m_videoItem->Dump();
}

void CGUIDialogBoxeeManualResolveEpisode::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
}

bool CGUIDialogBoxeeManualResolveEpisode::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();
    if (iControl == BUTTON_DONE)
    {
      // Set contents of the season and episode text boxes into the video item as properties
      CStdString strSeason = ((CGUIEditControl*)GetControl(SEASON_EDIT))->GetLabel2();
      if (!strSeason.IsEmpty())
      {
        m_videoItem->GetVideoInfoTag()->m_iSeason = BOXEE::BXUtils::StringToInt(strSeason);
      }

      CStdString strEpisode = ((CGUIEditControl*)GetControl(EPISODE_EDIT))->GetLabel2();
      if (!strEpisode.IsEmpty())
      {
        m_videoItem->GetVideoInfoTag()->m_iEpisode = BOXEE::BXUtils::StringToInt(strEpisode);
      }

      m_videoItem->SetProperty("manualresolve::ApplyToFolder", ((CGUIRadioButtonControl*)GetControl(APPLY_TO_FOLDER_RADIO))->IsSelected());

      m_bConfirmed = true;
      Close();
    }
    else if (iControl == BUTTON_PREV)
    {
      m_bConfirmed = false;
      Close();
    }
  }
  break;
  } // switch
  return CGUIDialog::OnMessage(message);
}

