#ifndef GUI_WINDOW_BOXEE_WIZARD_RESOLUTION
#define GUI_WINDOW_BOXEE_WIZARD_RESOLUTION

#pragma once

#include <vector>
#include "GUIDialog.h"
#include "linux/XRandR.h"

class CGUIWindowBoxeeWizardResolution : public CGUIDialog
{
public:
   CGUIWindowBoxeeWizardResolution(void);
   virtual ~CGUIWindowBoxeeWizardResolution(void);
   virtual void OnInitWindow();
   virtual bool OnAction(const CAction &action);
  
private:
   bool IsResolutionValid(XMode mode);
   bool IsResolutionHD(XMode mode); 
   void ShowResolutionsList(bool HD, XMode* selectedMode);
   XOutput GetCurrentOutput();
   int GetXBMCResolutionId();   

   CXRandR* m_xrandr;  
   bool m_afterResolutionChange;
   bool m_wizardCompletedOnce;
   bool m_resolutionChangedOnce;
   XMode m_originalResolution;
   RESOLUTION m_originalResolutionId; 
   XMode m_newResolution;
   std::vector<CFileItem*> m_resolutions;
   std::vector<CStdString> m_resolutionNames;
};

#endif
