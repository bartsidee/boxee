#pragma once

#include "GUIDialogFirstTimeUseBase.h"

#ifdef HAS_EMBEDDED

#include "BoxeeVersionUpdateManager.h"

class CGUIDialogFirstTimeUseUpdateProgress : public CGUIDialogFirstTimeUseBase
{
public:
  
  CGUIDialogFirstTimeUseUpdateProgress();
  virtual ~CGUIDialogFirstTimeUseUpdateProgress();

  virtual bool OnAction(const CAction &action);
  virtual void Render();

  VERSION_UPDATE_DOWNLOAD_STATUS GetDownloadStatus();

protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

private:

  void UpdateDialog();

  VERSION_UPDATE_DOWNLOAD_STATUS m_Status;
  unsigned long m_downloadCounter;

};

#endif
