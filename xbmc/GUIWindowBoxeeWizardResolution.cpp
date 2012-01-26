#include "stdafx.h"
#include "GUIWindowManager.h"
#include "GUIDialogYesNo.h"
#include "GraphicContext.h"
#include "GUIWindowBoxeeWizardResolution.h"
#include "Application.h"
#include "XRandR.h"
#include "GUIDialogOK.h"
#include "GUIListContainer.h"
#include "GUIFontManager.h"

#define CONTROL_SD_HD       50
#define CONTROL_RESOLUTIONS 51
//#define CONTROL_CALIBRATE   97
#define CONTROL_BACK        98
#define CONTROL_NEXT        99

#define VALUE_SD 0
#define VALUE_HD 1

using namespace std;

CGUIWindowBoxeeWizardResolution::CGUIWindowBoxeeWizardResolution(void)
    : CGUIDialog(WINDOW_BOXEE_WIZARD_RESOLUTION, "boxee_wizard_resolution.xml")
{
    m_afterResolutionChange = false;
    m_wizardCompletedOnce = false; 
    m_resolutionChangedOnce = false;
    m_xrandr = new CXRandR();
}

CGUIWindowBoxeeWizardResolution::~CGUIWindowBoxeeWizardResolution(void)
{}

void CGUIWindowBoxeeWizardResolution::OnInitWindow()
{
   CGUIWindow::OnInitWindow();
   
   delete m_xrandr;
   m_xrandr = new CXRandR();
   
   if (!m_wizardCompletedOnce)
   {             
      CONTROL_DISABLE(CONTROL_NEXT);
   }
   
   if (m_afterResolutionChange)
   {
       CGUIListContainer *pList = (CGUIListContainer *)GetControl(CONTROL_SD_HD);
       if (pList)
         pList->SetSingleSelectedItem();

      ShowResolutionsList(IsResolutionHD(m_newResolution), &m_newResolution);     
      
      CStdString origHz;
      origHz.Format("%.2f", m_originalResolution.hz);
      CStdString newHz;
      newHz.Format("%.2f", m_newResolution.hz);

      if (m_newResolution.name != m_originalResolution.name || origHz != newHz)
      {
         CGUIDialogYesNo *pDlgYesNo = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
         pDlgYesNo->SetHeading("Resolution Change");
         pDlgYesNo->SetLine(0, "Do you want to keep this resolution");
         pDlgYesNo->SetLine(1, "(will revert automatically in 6 seconds)");
         pDlgYesNo->SetLine(2, "");
         pDlgYesNo->SetLine(3, "");
         pDlgYesNo->SetChoice(0, "Revert");
         pDlgYesNo->SetChoice(1, "Keep");
         pDlgYesNo->SetDefaultChoice(0);
         pDlgYesNo->SetAutoClose(6000);
         pDlgYesNo->DoModal();
         
         if (!pDlgYesNo->IsConfirmed())
         {
           m_afterResolutionChange = false;   
           // Set the requested resolution
           g_graphicsContext.SetVideoResolution(m_originalResolutionId, TRUE);
           // Reload the fonts to they will scale correctly
           g_fontManager.ReloadTTFFonts();
           // Close the dialog and restart it so it will re-set the size of the labels to 
           // fit for the new resolutions
           Close();
                    
           m_gWindowManager.ActivateWindow(WINDOW_BOXEE_WIZARD_RESOLUTION);
         }
         else
         {
            int xbmcResolutionId = GetXBMCResolutionId();
            if (xbmcResolutionId != -1)
            {
               g_guiSettings.SetInt("videoscreen.resolution", xbmcResolutionId);
               g_guiSettings.SetInt("videoplayer.displayresolution", xbmcResolutionId);
               g_guiSettings.SetInt("pictures.displayresolution", xbmcResolutionId);
               g_settings.Save();
               delete m_xrandr;
               m_xrandr = new CXRandR();
            }
         
            m_wizardCompletedOnce = true;
            CONTROL_ENABLE(CONTROL_NEXT);
            SET_CONTROL_FOCUS(CONTROL_NEXT, 0);            
         }
      }
      else
      {
         m_wizardCompletedOnce = true;
         CONTROL_ENABLE(CONTROL_NEXT);
         SET_CONTROL_FOCUS(CONTROL_NEXT, 0);         
      }
   }
   else if (m_resolutionChangedOnce || m_wizardCompletedOnce)
   {
      XOutput output = GetCurrentOutput();
      XMode currentResolution =  m_xrandr->GetCurrentMode(output.name);
      ShowResolutionsList(IsResolutionHD(currentResolution), &currentResolution);      
   }
   else
   {
      XOutput output = GetCurrentOutput();
      XMode currentResolution =  m_xrandr->GetCurrentMode(output.name);
      CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_SD_HD, IsResolutionHD(currentResolution) ? VALUE_HD : VALUE_SD);
      OnMessage(msg); 
   }
}

bool CGUIWindowBoxeeWizardResolution::OnAction(const CAction &action)
{
   int iControl = GetFocusedControlID();

   if (action.wID == ACTION_PREVIOUS_MENU || (action.wID == ACTION_SELECT_ITEM && iControl == CONTROL_BACK))
   {
      Close();
      return true;     
   }
   else if (action.wID == ACTION_MOVE_LEFT && iControl == CONTROL_RESOLUTIONS)
   {
      SET_CONTROL_HIDDEN(CONTROL_RESOLUTIONS);
      SET_CONTROL_FOCUS(CONTROL_SD_HD, 0);
      return true;      
   }   
   else if (action.wID == ACTION_SELECT_ITEM && iControl == CONTROL_SD_HD)
   {
      SET_CONTROL_VISIBLE(CONTROL_RESOLUTIONS);
      
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_SD_HD);
      OnMessage(msg);
      int iItem = msg.GetParam1();

      CGUIListContainer *pList = (CGUIListContainer *)GetControl(CONTROL_SD_HD);
      if (pList)
        pList->SetSingleSelectedItem();

      XOutput output = GetCurrentOutput();
      XMode currentResolution =  m_xrandr->GetCurrentMode(output.name);
      ShowResolutionsList(iItem == VALUE_HD, &currentResolution);
      
      return true;      
   }      
   else if (action.wID == ACTION_SELECT_ITEM && iControl == CONTROL_RESOLUTIONS)
   {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_RESOLUTIONS);
      OnMessage(msg);
      int iItem = msg.GetParam1();

      CGUIListContainer *pList = (CGUIListContainer *)GetControl(CONTROL_RESOLUTIONS);
      if (pList)
        pList->SetSingleSelectedItem();
   
      XOutput output = GetCurrentOutput();
      m_originalResolution =  m_xrandr->GetCurrentMode(output.name);
      m_originalResolutionId = g_graphicsContext.GetVideoResolution();
      
      for (unsigned int i = 0; i < output.modes.size(); i++)
      {
         if (output.modes[i].name == m_resolutions[iItem]->GetProperty("resolutionName") &&
             (int)output.modes[i].hz == m_resolutions[iItem]->GetPropertyInt("resolutionHz"))
         {
            m_newResolution = output.modes[i];

            CStdString origHz;
            origHz.Format("%.2f", m_originalResolution.hz);
            CStdString newHz;
            newHz.Format("%.2f", m_newResolution.hz);
            
            if (m_newResolution.name != m_originalResolution.name || newHz != origHz)
            {
               // Set the requested resolution
               RESOLUTION res = (RESOLUTION) GetXBMCResolutionId();
               g_graphicsContext.SetVideoResolution(res, TRUE);
   
               // Reload the fonts to they will scale correctly
               g_fontManager.ReloadTTFFonts();
            }
            
            m_afterResolutionChange = true;
            m_resolutionChangedOnce = true;
            
            // Close the dialog and restart it so it will re-set the size of the labels to 
            // fit for the new resolutions
            Close();
            m_gWindowManager.ActivateWindow(WINDOW_BOXEE_WIZARD_RESOLUTION);
                                  
            break;
         }
      }
      
      return true;      
   }
   else if (action.wID == ACTION_MOVE_UP && (iControl == CONTROL_NEXT || iControl == CONTROL_BACK))
   {
      if (GetControl(CONTROL_RESOLUTIONS)->IsVisible())
      {
         SET_CONTROL_FOCUS(CONTROL_RESOLUTIONS, 0);
      }
      else
      {
         SET_CONTROL_FOCUS(CONTROL_SD_HD, 0);
      }
      
      return true;
   }
  
   return CGUIWindow::OnAction(action);
}

bool CGUIWindowBoxeeWizardResolution::IsResolutionValid(XMode mode)
{
   // Funky 1080i resolutions are not welcome
   if (mode.w == 1920 && mode.h == 540)
      return false;
      
   // Anything below 640x480 is only good for Atari 2600 gaming
   if (mode.w < 640 || mode.h < 480)
      return false;
      
   return true;
}

bool CGUIWindowBoxeeWizardResolution::IsResolutionHD(XMode mode)
{
   // Anything from 480p and up is HD
   if (mode.h > 480)
      return true;
   else
      return false;
}

void CGUIWindowBoxeeWizardResolution::ShowResolutionsList(bool HD, XMode* selectedMode)
{
   // Set either HD/SD according to the selected mode
   if (selectedMode != NULL)
   {
      CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_SD_HD, IsResolutionHD(*selectedMode) ? VALUE_HD : VALUE_SD);
      OnMessage(msg); 
   }
   
   SET_CONTROL_VISIBLE(CONTROL_RESOLUTIONS);

   // Clear the list first
   CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_RESOLUTIONS);
   OnMessage(msgReset);
   
   for (unsigned int i = 0; i < m_resolutions.size(); i++)
      delete m_resolutions[i];
   m_resolutions.clear();
   m_resolutionNames.clear();

   // Go over all the resolutions and display them
   int iSelectedItem = 0;
   XOutput output = GetCurrentOutput();
   for (unsigned int i = 0; i < output.modes.size(); i++)
   {
      // skip modes with similar names (but different Hz)
      //if (i > 0 && output.modes[i].name == output.modes[i-1].name)
      //   continue;
         
      if (IsResolutionHD(output.modes[i]) && IsResolutionValid(output.modes[i]))
      {
         CStdString name;
         name.Format("%s / %.0fHz", output.modes[i].name.c_str(), output.modes[i].hz); 
         CFileItem *item = new CFileItem(name);
         item->SetProperty("resolutionName", output.modes[i].name);
         item->SetProperty("resolutionHz", (int) output.modes[i].hz);
         CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_RESOLUTIONS, 0, 0, item);
         OnMessage(msg);            

         m_resolutions.push_back(item);
         
         CStdString resolutionName;
         resolutionName.Format("%s: %s @ %.2fHz", output.name.c_str(), output.modes[i].name, output.modes[i].hz);  
         m_resolutionNames.push_back(resolutionName);

         CStdString modeHz;
         modeHz.Format("%.2f", output.modes[i].hz);
         CStdString selectedHz;
         selectedHz.Format("%.2f", selectedMode->hz);
         
         if (selectedMode != NULL && output.modes[i].name == selectedMode->name && modeHz == selectedHz)
           iSelectedItem = m_resolutions.size() - 1;            
      }
   }
   
   CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_RESOLUTIONS, iSelectedItem);
   OnMessage(msg);      

   CGUIListContainer *pList = (CGUIListContainer *)GetControl(CONTROL_RESOLUTIONS);
   if (pList)
     pList->SetSingleSelectedItem();
   
   SET_CONTROL_FOCUS(CONTROL_RESOLUTIONS, 0);
}

int CGUIWindowBoxeeWizardResolution::GetXBMCResolutionId()
{
   // Get the selected resolution name
   CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_RESOLUTIONS );
   OnMessage(msg);
   int iItem = msg.GetParam1();
   CStdString resolutionXbmcName = m_resolutionNames[iItem];
               
   for (int i = 0; i < g_videoConfig.GetNumberOfResolutions(); i++)
   {
      RESOLUTION_INFO info;
      g_videoConfig.GetResolutionInfo(i, info);
      
      if (info.strMode == resolutionXbmcName)
      {   
         return i + CUSTOM;
      }
   }
   
   return -1;
}

// Tom's hack for Intel video where using the apple DVI->VGA adapter, you see two outputs
XOutput CGUIWindowBoxeeWizardResolution::GetCurrentOutput()
{
   std::vector<XOutput> outputs = m_xrandr->GetModes();
   if (outputs.size() == 1)
      return outputs[0];
   if (outputs.size() == 2)
   {
      if (outputs[0].name == "VGA" && outputs[1].name == "TMDS-1")
         return outputs[0];
      if (outputs[0].name == "TMDS-1" && outputs[1].name == "VGA")
         return outputs[1];
   }

   // Fallback!!!
   return outputs[0];
}
