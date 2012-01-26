#ifndef GUI_WINDOW_BOXEE_WIZARD_AUDIO
#define GUI_WINDOW_BOXEE_WIZARD_AUDIO

#pragma once

#include <vector>
#include "GUIDialog.h"
#include "IPlayer.h"
#include "cores/paplayer/paplayer.h"

#define OUTPUT_ANALOG    0
#define OUTPUT_DIGITAL   1

class CGUIWindowBoxeeWizardAudio : public CGUIDialog, IPlayerCallback
{
public:
  CGUIWindowBoxeeWizardAudio(void);
  virtual ~CGUIWindowBoxeeWizardAudio(void);
  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);
  
  virtual void OnPlayBackEnded(bool bError = false, const CStdString& error = "");
  virtual void OnPlayBackStarted()  { }
  virtual void OnPlayBackStopped()  { }
  virtual void OnQueueNextItem()    { } 
  
private:
  void PlayTestSound();
  void StopTestSound();
  void SetOutputDevice(int output);
  int GetOutputDevice();
  
  PAPlayer m_player;
};

#endif
