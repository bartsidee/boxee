#include "GUIWindowManager.h"
#include "GUIWindowBoxeeWizardAudio.h"
#include "Application.h"
#include "Util.h"
#include "GUIListContainer.h"
#include "GUIDialogOK.h"
#include "SpecialProtocol.h"
#include "GUISettings.h"

#define CONTROL_MODES 50
#define CONTROL_ASSIST 97
#define CONTROL_BACK  98
#define CONTROL_NEXT  99

CGUIWindowBoxeeWizardAudio::CGUIWindowBoxeeWizardAudio(void)
    : CGUIDialog(WINDOW_BOXEE_WIZARD_AUDIO, "boxee_wizard_audio.xml"), m_player(*this)
{
}

CGUIWindowBoxeeWizardAudio::~CGUIWindowBoxeeWizardAudio(void)
{}


bool CGUIWindowBoxeeWizardAudio::OnAction(const CAction &action)
{
   int iControl = GetFocusedControlID();

   bool bSelectAction = ((action.id == ACTION_SELECT_ITEM) || (action.id == ACTION_MOUSE_LEFT_CLICK));

   if (action.id == ACTION_PREVIOUS_MENU || (bSelectAction && iControl == CONTROL_BACK))
   {
      StopTestSound();
      Close();
      return true;
   }
   else if (bSelectAction && iControl == CONTROL_NEXT)
   {
      StopTestSound();
   }
   else if (bSelectAction && iControl == CONTROL_ASSIST)
   {
      CGUIDialogOK *pDialogOK = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
      pDialogOK->SetHeading("");
      pDialogOK->SetLine(0, 51015);
      pDialogOK->SetLine(1, 51016);
      pDialogOK->SetLine(2, 51017);
      pDialogOK->DoModal();
   }
   else if (bSelectAction && iControl == CONTROL_MODES)
   {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_MODES, 0);
      OnMessage(msg);
      int iItem = msg.GetParam1();

      if (GetOutputDevice() != iItem || !m_player.IsPlaying())
      {
         StopTestSound();
         SetOutputDevice(iItem);
         PlayTestSound();
      }   

     CGUIListContainer *pList = (CGUIListContainer *)GetControl(CONTROL_MODES);
     if (pList)
       pList->SetSingleSelectedItem();
      
      SET_CONTROL_FOCUS(CONTROL_NEXT, 0);
      CONTROL_ENABLE(CONTROL_NEXT);
      return true;      
   }
   
  return CGUIWindow::OnAction(action);
}

void CGUIWindowBoxeeWizardAudio::OnInitWindow()
{
  CGUIWindow::OnInitWindow();
  
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_MODES, GetOutputDevice());
  OnMessage(msg);
      
  PlayTestSound();
}  

void CGUIWindowBoxeeWizardAudio::PlayTestSound()
{
   CFileItem file;
   file.m_strPath = _P("special://xbmc/media/test_sound.mp3");
   CPlayerOptions options;
   m_player.OpenFile(file, options); 
}

void CGUIWindowBoxeeWizardAudio::OnPlayBackEnded(bool bError, const CStdString& error)
{
}

void CGUIWindowBoxeeWizardAudio::StopTestSound()
{
   m_player.CloseFile();
}

void CGUIWindowBoxeeWizardAudio::SetOutputDevice(int output)
{
   g_guiSettings.SetInt("audiooutput.mode", output == OUTPUT_ANALOG ? 0 : 1);
   if (output == OUTPUT_ANALOG)
   {
      g_guiSettings.SetString("audiooutput.audiodevice", "default");
      g_guiSettings.SetString("audiooutput.passthroughdevice", "default");
   }
   else
   {
      g_guiSettings.SetString("audiooutput.audiodevice", "iec958");
      g_guiSettings.SetString("audiooutput.passthroughdevice", "iec958");
   }

   g_settings.Save();
}

int CGUIWindowBoxeeWizardAudio::GetOutputDevice()
{
   int mode = g_guiSettings.GetInt("audiooutput.mode");
   return (mode == 0 ? OUTPUT_ANALOG : OUTPUT_DIGITAL);
}
