#include "GUIDialogBoxeeSeekBar.h"
#include "GUIWindowManager.h"
#include "log.h"
#include "Application.h"
#include "utils/GUIInfoManager.h"
#include "GUIButtonControl.h"
#include "GUIImage.h"
#include "TimeUtils.h"

#define CONTROL_SEEK_BUTTON 8100
#define CONTROL_SEEK_ARROW_IMAGE 8200

#define MIN_SEEK_OFFSET_FARWARD 10
#define MIN_SEEK_OFFSET_BACKWARD 5
#define MIN_PROGRESS_POS_X 0
#define MAX_PROGRESS_POS_X 1280

CGUIDialogBoxeeSeekBar::CGUIDialogBoxeeSeekBar() : CGUIDialog(WINDOW_DIALOG_BOXEE_SEEK_BAR, "boxee_seek_bar.xml")
{

}

CGUIDialogBoxeeSeekBar::~CGUIDialogBoxeeSeekBar()
{

}

void CGUIDialogBoxeeSeekBar::OnInitWindow()
{
  if (!g_application.m_pPlayer->CanSeek())
  {
    CLog::Log(LOGWARNING,"CGUIDialogBoxeeSeekBar::OnInitWindow - player can't seek. Close dialog (osds)");
    Close();
    return;
  }

  CGUIDialog::OnInitWindow();

  m_buttonCtrl = (CGUIButtonControl*)GetControl(CONTROL_SEEK_BUTTON);
  if (!m_buttonCtrl)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeSeekBar::OnInitWindow - FAILED to get CGUIButtonControl (osds)");
    Close();
    return;
  }

  m_arrowImageCtrl = (CGUIImage*)GetControl(CONTROL_SEEK_ARROW_IMAGE);
  if (!m_arrowImageCtrl)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeSeekBar::OnInitWindow - FAILED to get CGUIImage (osds)");
    Close();
    return;
  }

  m_onUserMove = false;
  m_totalTime = g_application.GetTotalTime();

  if (m_totalTime < 3600)
  {
    m_labelTimeFormt = TIME_FORMAT_MM_SS;
  }
  else
  {
    m_labelTimeFormt = TIME_FORMAT_HH_MM_SS;
  }

  m_currentTime = g_application.GetTime();
  m_seekTimeLabel = g_infoManager.GetCurrentPlayTime(m_labelTimeFormt);
  m_seekHoldTime = 0;
  m_lastSeekEvent = 0;
  m_seekForward = true;

  float playbackPosX = (MAX_PROGRESS_POS_X) * m_currentTime / m_totalTime;

  if (!SetSeekControlPos(playbackPosX))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeSeekBar::OnInitWindow - FAILED to set SeekControlPos (osds)");
    Close();
    return;
  }

  SET_CONTROL_VISIBLE(CONTROL_SEEK_BUTTON);
  SET_CONTROL_VISIBLE(CONTROL_SEEK_ARROW_IMAGE);
  SET_CONTROL_LABEL(CONTROL_SEEK_BUTTON,m_seekTimeLabel);
  SET_CONTROL_FOCUS(CONTROL_SEEK_BUTTON, 0);
}

bool CGUIDialogBoxeeSeekBar::OnAction(const CAction &action)
{
  switch (action.id)
  {
  case ACTION_PARENT_DIR:
  case ACTION_PREVIOUS_MENU:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeSeekBar::OnAction - ACTION_PARENT_DIR - ACTION_PREVIOUS_MENU (osds)");
    Close();
    return true;
  }
  case ACTION_MOVE_LEFT:
  case ACTION_MOVE_RIGHT:
  {
    m_onUserMove = true;
    if (m_autoClosing) SetAutoClose(3000);
    m_seekHoldTime = action.holdTime;
    ChangeSeek(action.id == ACTION_MOVE_LEFT);
    g_infoManager.m_performingSeek = true;
    g_application.SeekTime(m_currentTime);
    m_lastSeekEvent = CTimeUtils::GetTimeMS();
    return true;
  }
  default:
    break;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeeSeekBar::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    //CLog::Log(LOGDEBUG,"CGUIDialogBoxeeSeekBar::OnMessage - GUI_MSG_CLICKED - seek to [seekPercentage=%f] (osds)",m_seekPercentage);

    //g_application.SeekPercentage(m_seekPercentage);
    m_onUserMove = true;
    Close();
    return true;
  }
  break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogBoxeeSeekBar::Render()
{
  if (!m_onUserMove && !g_infoManager.m_performingSeek)
  {
    m_seekTimeLabel = g_infoManager.GetCurrentPlayTime(m_labelTimeFormt);
    m_currentTime = g_application.GetTime();
  }
  else
  {
    StringUtils::SecondsToTimeString((int)m_currentTime, m_seekTimeLabel, m_labelTimeFormt);
  }

  if (!SetSeekControlPos((MAX_PROGRESS_POS_X)*m_currentTime/m_totalTime))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeSeekBar::Render - FAILED to set SeekControlPos (osds)");
    Close();
  }

  SET_CONTROL_LABEL(CONTROL_SEEK_BUTTON,m_seekTimeLabel);

  if (m_lastSeekEvent && (CTimeUtils::GetTimeMS() - m_lastSeekEvent > 500))
  {
    m_onUserMove = false;
    m_lastSeekEvent = 0;
  }

  CGUIDialog::Render();
}

bool CGUIDialogBoxeeSeekBar::SetSeekControlPos(float playbackPosX)
{
  if (!m_buttonCtrl)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeSeekBar::SetSeekButtonPos - FAILED to get CGUIButtonControl (osds)");
    return false;
  }

  float seekButtonWidth = m_buttonCtrl->GetWidth();
  float seekArrowWidth = m_arrowImageCtrl->GetWidth();

  float seekButtonPosX = 0;
  float seekArrowPosX = 0;
  bool needToFindArrowPosX = true;

  if (playbackPosX < (seekButtonWidth/2))
  {
    seekButtonPosX = 0;
  }
  else if (playbackPosX > (MAX_PROGRESS_POS_X - (seekButtonWidth/2)))
  {
    seekButtonPosX = (MAX_PROGRESS_POS_X - seekButtonWidth);
  }
  else
  {
    seekButtonPosX = (playbackPosX - (seekButtonWidth/2));
    seekArrowPosX = (playbackPosX - (seekArrowWidth/2));
    needToFindArrowPosX = false;
  }

  float seekButtonPosY = m_buttonCtrl->GetYPosition();

  m_buttonCtrl->SetPosition(seekButtonPosX,seekButtonPosY);

  if (!m_arrowImageCtrl)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeSeekBar::SetSeekControlPos - FAILED to get CGUIImage (osds)");
    return false;
  }

  if (needToFindArrowPosX)
  {
    if (seekButtonPosX <= MIN_PROGRESS_POS_X)
    {
      seekArrowPosX = ((playbackPosX - (seekArrowWidth/2)) > 0) ? (playbackPosX - (seekArrowWidth/2)) : 0 ;
    }
    else if (seekButtonPosX >= (MAX_PROGRESS_POS_X - seekButtonWidth))
    {
      seekArrowPosX = ((playbackPosX - (seekArrowWidth/2)) < (MAX_PROGRESS_POS_X - (seekArrowWidth))) ? (playbackPosX - (seekArrowWidth/2)) : (MAX_PROGRESS_POS_X - (seekArrowWidth)) ;
    }
  }

  float seekArrowPosY = m_arrowImageCtrl->GetYPosition();
  m_arrowImageCtrl->SetPosition(seekArrowPosX,seekArrowPosY);

  return true;
}

void CGUIDialogBoxeeSeekBar::ChangeSeek(bool back)
{
  unsigned int seekDelta = 0;
  unsigned int seekSpeed = 0;

  if(back)
    seekDelta = MIN_SEEK_OFFSET_BACKWARD;
  else
    seekDelta = MIN_SEEK_OFFSET_FARWARD;

  seekSpeed = seekDelta;

  if(m_seekHoldTime > 500)
  {
    float ratio;
    ratio = (m_seekHoldTime / 3000.0);
    if(ratio > 1.5) ratio = 1.5;
    seekSpeed = (unsigned int) (m_totalTime * (ratio / 100));
  }

  if(seekSpeed < seekDelta)
    seekSpeed = seekDelta;

  m_seekForward = !back;

  if(!back)
    m_currentTime += seekSpeed;
  else
    m_currentTime -= seekSpeed;

  if(m_currentTime < 0)
    m_currentTime = 0;

  if (m_currentTime > m_totalTime)
    m_currentTime = m_totalTime;
}

