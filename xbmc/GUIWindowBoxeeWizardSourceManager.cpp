#include "stdafx.h"
#include "FileItem.h"
#include "GUIWindowManager.h"
#include "GUIWindowBoxeeWizardSourceManager.h"
#include "Application.h"
#include "GUIWindowBoxeeWizardAddSource.h"
#include "GUIDialogYesNo2.h"

#define CONTROL_VIDEO_LIST    50
#define CONTROL_VIDEO_ADD     90
#define CONTROL_MUSIC_LIST    60
#define CONTROL_MUSIC_ADD     91
#define CONTROL_PICTURES_LIST 70
#define CONTROL_PICTURES_ADD  92
#define CONTROL_BACK          98
#define CONTROL_NEXT          99

#define VIDEO_LISTITEMS      0
#define MUSIC_LISTITEMS      1
#define PICTURES_LISTITEMS   2

CGUIWindowBoxeeWizardSourceManager::CGUIWindowBoxeeWizardSourceManager(void)
    : CGUIDialog(WINDOW_BOXEE_WIZARD_SOURCE_MANAGER, "boxee_add_source_main.xml")
{
}

CGUIWindowBoxeeWizardSourceManager::~CGUIWindowBoxeeWizardSourceManager(void)
{}


bool CGUIWindowBoxeeWizardSourceManager::OnAction(const CAction &action)
{
   int iControl = GetFocusedControlID();

   bool bSelectAction = ((action.wID == ACTION_SELECT_ITEM) || (action.wID == ACTION_MOUSE_LEFT_CLICK));

   if (bSelectAction && (iControl == CONTROL_VIDEO_ADD || iControl == CONTROL_MUSIC_ADD || iControl == CONTROL_PICTURES_ADD))
   {
      CGUIWindowBoxeeWizardAddSource* pDlgAddSource = (CGUIWindowBoxeeWizardAddSource*)m_gWindowManager.GetWindow(WINDOW_BOXEE_WIZARD_ADD_SOURCE);
      switch (iControl)
      {
         case CONTROL_VIDEO_ADD:
            pDlgAddSource->SetCategory("video");
            break;
         case CONTROL_MUSIC_ADD:
            pDlgAddSource->SetCategory("music");
            break;
         case CONTROL_PICTURES_ADD:
            pDlgAddSource->SetCategory("pictures");
            break;
      }
      pDlgAddSource->DoModal();
      LoadAllShares();
      return true;
   }
   else if (bSelectAction && (iControl == CONTROL_VIDEO_LIST || iControl == CONTROL_MUSIC_LIST || iControl == CONTROL_PICTURES_LIST))
   {
      CGUIDialogYesNo2 *pDlgYesNo = (CGUIDialogYesNo2*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO_2);
      pDlgYesNo->SetHeading(51020);
      pDlgYesNo->SetLine(0, 51025);
      pDlgYesNo->SetChoice(0, 222);
      pDlgYesNo->SetChoice(1, "Ok");
      pDlgYesNo->DoModal();
      
      if (!pDlgYesNo->IsConfirmed())
         return true;
   
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
      OnMessage(msg);
      int iItem = msg.GetParam1();
      
      CFileItemPtr item;
      switch (iControl)
      {
         case CONTROL_VIDEO_LIST:
            item = m_sources[VIDEO_LISTITEMS][iItem];
            g_settings.DeleteSource("video", item->GetLabel(), item->m_strPath);
            g_settings.SaveSources();
            break;
         case CONTROL_MUSIC_LIST:
            item = m_sources[MUSIC_LISTITEMS][iItem];
            g_settings.DeleteSource("music", item->GetLabel(), item->m_strPath);
            g_settings.SaveSources();
            break;
         case CONTROL_PICTURES_LIST:
            item = m_sources[PICTURES_LISTITEMS][iItem];
            g_settings.DeleteSource("pictures", item->GetLabel(), item->m_strPath);
            g_settings.SaveSources();
            break;
      }
      LoadAllShares();
      
      return true;
   }   
   else if (action.wID == ACTION_MOVE_UP && (iControl == CONTROL_VIDEO_ADD || iControl == CONTROL_MUSIC_ADD || iControl == CONTROL_PICTURES_ADD))
   {
     int count = 0;
     switch (iControl)
     {
       case CONTROL_VIDEO_ADD:
          count = m_sources[VIDEO_LISTITEMS].Size();
          break;
       case CONTROL_MUSIC_ADD:
          count = m_sources[MUSIC_LISTITEMS].Size();
          break;
       case CONTROL_PICTURES_ADD:
          count = m_sources[PICTURES_LISTITEMS].Size();
          break;
     }
     
     // If the above lists are empty, don't do anything...stand still
     if (count == 0)
       return true;
   }
   else if (action.wID == ACTION_PREVIOUS_MENU)
   {
      Close();
      
      return true;
   }
   
   return CGUIWindow::OnAction(action);
}

void CGUIWindowBoxeeWizardSourceManager::OnInitWindow()
{
  CGUIWindow::OnInitWindow();
  LoadAllShares();
}

void CGUIWindowBoxeeWizardSourceManager::LoadAllShares()
{
   LoadShares(g_settings.m_videoSources, CONTROL_VIDEO_LIST, VIDEO_LISTITEMS);
   LoadShares(g_settings.m_musicSources, CONTROL_MUSIC_LIST, MUSIC_LISTITEMS);
   LoadShares(g_settings.m_pictureSources, CONTROL_PICTURES_LIST, PICTURES_LISTITEMS);
}

void CGUIWindowBoxeeWizardSourceManager::LoadShares(VECSOURCES& shares, int controlId, int listitemsIndex)
{
   // Clear the list first
   CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), controlId);
   OnMessage(msgReset);
   m_sources[listitemsIndex].Clear();
   
   for (size_t i = 0; i < shares.size(); i++)
   {
      if (!IsPredefinedShare(shares[i]))
      {
         CFileItemPtr share ( new CFileItem(shares[i].strName) );
         share->m_strPath = shares[i].strPath;
         m_sources[listitemsIndex].Add(share);
         CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), controlId, 0, 0, share);
         OnMessage(msg);
      }
   }
}

bool CGUIWindowBoxeeWizardSourceManager::IsPredefinedShare(CMediaSource& share)
{
   if (share.strPath.Left(4) == "smb:")
      return false;
      
   if (share.strPath.Left(7) == "/media/")
      return false;
   
   return true;
}
