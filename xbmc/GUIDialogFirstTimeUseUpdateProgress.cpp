#include "GUIDialogFirstTimeUseUpdateProgress.h"

#ifdef HAS_EMBEDDED

#include "GUILabelControl.h"
#include "GUIProgressControl.h"
#include "LocalizeStrings.h"
#include "log.h"

#define CONTROL_TIME_LABEL 267
#define CONTROL_PROGRESS   268
#define CONTROL_PROGRESS_LABEL   269

CGUIDialogFirstTimeUseUpdateProgress::CGUIDialogFirstTimeUseUpdateProgress() : CGUIDialogFirstTimeUseBase(WINDOW_DIALOG_FTU_UPDATE_PROGRESS,"ftu_update_progress.xml","CGUIDialogFirstTimeUseUpdateProgress")
{
  m_Status = VUDS_IDLE;
  m_downloadCounter = 0;
}

CGUIDialogFirstTimeUseUpdateProgress::~CGUIDialogFirstTimeUseUpdateProgress()
{

}

void CGUIDialogFirstTimeUseUpdateProgress::OnInitWindow()
{
  CGUIDialogFirstTimeUseBase::OnInitWindow();

  m_Status = VUDS_IDLE;
  UpdateDialog();

  m_downloadCounter = 0;
}

bool CGUIDialogFirstTimeUseUpdateProgress::OnAction(const CAction &action)
{
  if (action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    // don't allow back in this dialog

    return true;
  }

  return CGUIDialogFirstTimeUseBase::OnAction(action);
}

void CGUIDialogFirstTimeUseUpdateProgress::Render()
{
  m_downloadCounter++;
  if (m_downloadCounter % 60 == 0)
  {
    m_downloadCounter = 0;

    ////////////////////////////////////////
    // check download status every second //
    ////////////////////////////////////////

    UpdateDialog();
  }

  CGUIDialogFirstTimeUseBase::Render();
}

void CGUIDialogFirstTimeUseUpdateProgress::UpdateDialog()
{
  CDownloadInfo downloadnfo;
  g_boxeeVersionUpdateManager.GetDownloadInfo(downloadnfo);

  CStdString label;
  double percent = downloadnfo.m_CurrentDownloadProgress;

  m_Status = downloadnfo.m_Status;
  switch(m_Status)
  {
  case VUDS_IDLE:
  case VUDS_PRE_DOWNLOADING:
  {
    label = g_localizeStrings.Get(54735);
  }
  break;
  case VUDS_DOWNLOADING:
  {
    if ((int)percent == 0)
    {
      label = g_localizeStrings.Get(54735);
    }
    else if ((downloadnfo.m_EstimatedTimeLeftMS / (60*1000)) > 1)
    {
      // more then a minutes

      CStdString str = g_localizeStrings.Get(54732);
      label.Format(str.c_str(), downloadnfo.m_EstimatedTimeLeftMS / (60*1000));
    }
    else
    {
      // less then a minutes
      label = g_localizeStrings.Get(54733);
    }
  }
  break;
  case VUDS_POST_DOWNLOADING:
  {
    label = g_localizeStrings.Get(54654);
  }
  break;
  case VUDS_FINISHED:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseUpdateProgress::UpdateDialog - [downloadCounter=%ld] - [status=%d=VUDS_FINISHED] -> Close (initbox)",m_downloadCounter,(int)m_Status);
    CGUIProgressControl* progressCtrl = (CGUIProgressControl*)GetControl(CONTROL_PROGRESS);
    if (progressCtrl)
    {
      progressCtrl->SetPercentage(percent);
    }
    else
    {
      CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseUpdateProgress::UpdateDialog - [downloadCounter=%ld] - [status=%d=VUDS_FINISHED] - FAILED to get CONTROL_PROGRESS in order to update [percent=%f%%] (initbox)",m_downloadCounter,(int)m_Status,percent);
    }

    m_actionChoseEnum = CActionChose::NEXT;
    Close();
  }
  break;
  case VUDS_FAILED:
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseUpdateProgress::UpdateDialog - [downloadCounter=%ld] - [status=%d=VUDS_FAILED] -> Close (initbox)",m_downloadCounter,(int)m_Status);
    m_actionChoseEnum = CActionChose::NEXT;
    Close();
  }
  break;
  default:
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseUpdateProgress::UpdateDialog - [downloadCounter=%ld] - UNKNOWN [status=%d] -> not handling (initbox)",m_downloadCounter,(int)m_Status);
  }
  break;
  }

  CGUILabelControl* timeLabelCtrl = (CGUILabelControl*)GetControl(CONTROL_TIME_LABEL);
  if (timeLabelCtrl)
  {
    timeLabelCtrl->SetLabel(label);
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseUpdateProgress::UpdateDialog - FAILED to get CONTROL_TIME_LABEL in order to update [label=%s] (initbox)",label.c_str());
  }

  CGUIProgressControl* progressCtrl = (CGUIProgressControl*)GetControl(CONTROL_PROGRESS);
  if (progressCtrl)
  {
    progressCtrl->SetPercentage(percent);
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseUpdateProgress::UpdateDialog - FAILED to get CONTROL_PROGRESS in order to update [percent=%f%%] (initbox)",percent);
  }

  CStdString percentStr = "%d%%";
  CStdString percentLabel;
  percentLabel.Format(percentStr.c_str(),(int)percent);
  CGUILabelControl* percentLabelCtrl = (CGUILabelControl*)GetControl(CONTROL_PROGRESS_LABEL);
  if (percentLabelCtrl)
  {
    percentLabelCtrl->SetLabel(percentLabel);
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseUpdateProgress::UpdateDialog - FAILED to get CONTROL_PROGRESS_LABEL in order to update [percent=%s] (initbox)",percentLabel.c_str());
  }
}

bool CGUIDialogFirstTimeUseUpdateProgress::HandleClickNext()
{
  // nothing to do

  return true;
}

bool CGUIDialogFirstTimeUseUpdateProgress::HandleClickBack()
{
  // nothing to do

  return true;
}

VERSION_UPDATE_DOWNLOAD_STATUS CGUIDialogFirstTimeUseUpdateProgress::GetDownloadStatus()
{
  return m_Status;
}

#endif

