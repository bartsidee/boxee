#include "GUIDialogBoxeeSeekableCtx.h"
#include "GUIWindowManager.h"
#include "log.h"
#include "Application.h"
#include "utils/GUIInfoManager.h"
#include "GUIButtonControl.h"
#include "GUIImage.h"
#include "TimeUtils.h"

#define MIN_SEEK_OFFSET_FARWARD 10
#define MIN_SEEK_OFFSET_BACKWARD 5
#define MIN_PROGRESS_POS_X 0
#define MAX_PROGRESS_POS_X 1280
#define START_PROGRESS_BUTTON_POS_X 132
#define END_PROGRESS_BUTTON_POS_X (1148 - 104)

CGUIDialogBoxeeSeekableCtx::CGUIDialogBoxeeSeekableCtx(DWORD dwID, const CStdString &xmlFile) : CGUIDialogBoxeeCtx(dwID, xmlFile)
{
  m_seekDirectionOnOpen = CSeekDirection::NONE;
  m_showPlayTimeRemaining = false;
  m_needToAdjustPlaybackPosX = false;
}

CGUIDialogBoxeeSeekableCtx::~CGUIDialogBoxeeSeekableCtx()
{

}

void CGUIDialogBoxeeSeekableCtx::OnInitWindow()
{
  CGUIDialogBoxeeCtx::OnInitWindow();

  if (g_application.m_pPlayer && g_application.m_pPlayer->GetTotalTime() <= 0)
  {
    m_showPlayTimeRemaining = false;
  }

  m_buttonCtrl = (CGUIButtonControl*)GetControl(CONTROL_SEEK_BUTTON);
  if (!m_buttonCtrl)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeSeekableCtx::OnInitWindow - FAILED to get CGUIButtonControl (osds)");
    Close();
    return;
  }

  m_totalTime = g_application.GetTotalTime();

  if (m_totalTime < 3600)
  {
    m_labelTimeFormt = TIME_FORMAT_MM_SS;
  }
  else
  {
    m_labelTimeFormt = TIME_FORMAT_HH_MM_SS;
  }

  m_needToAdjustPlaybackPosX = false;
  if ((MIN_PROGRESS_POS_X != START_PROGRESS_BUTTON_POS_X) || (MAX_PROGRESS_POS_X != END_PROGRESS_BUTTON_POS_X))
  {
    m_needToAdjustPlaybackPosX = true;
  }

  if (g_application.m_pPlayer->CanSeek())
  {
    if (!g_application.m_pPlayer->CanSeekToTime())
    {
      CLog::Log(LOGWARNING,"CGUIDialogBoxeeSeekableCtx::OnInitWindow - player can't seek. Close dialog (osds)");
      //SET_CONTROL_HIDDEN(CONTROL_SEEK_ARROW_IMAGE);

      float seekButtonPosX = (MAX_PROGRESS_POS_X/2) - (m_buttonCtrl->GetWidth()/2);
      float seekButtonPosY = m_buttonCtrl->GetYPosition();
      m_buttonCtrl->SetPosition(seekButtonPosX,seekButtonPosY);
    }
    else
    {
      m_onUserMove = false;
      m_currentTime = g_application.GetTime();
      m_seekTimeLabel = g_infoManager.GetCurrentPlayTime(m_labelTimeFormt);
      m_seekHoldTime = 0;
      m_lastSeekEvent = 0;
      m_seekForward = true;

      float playbackPosX = (MAX_PROGRESS_POS_X) * m_currentTime / m_totalTime;

      if (!SetSeekControlPos(playbackPosX))
      {
        CLog::Log(LOGERROR,"CGUIDialogBoxeeSeekableCtx::OnInitWindow - FAILED to set SeekControlPos (osds)");
        Close();
        return;
      }

      SET_CONTROL_VISIBLE(CONTROL_SEEK_BUTTON);
      //SET_CONTROL_VISIBLE(CONTROL_SEEK_ARROW_IMAGE);
      SET_CONTROL_FOCUS(CONTROL_SEEK_BUTTON, 0);
    }
  }
}

bool CGUIDialogBoxeeSeekableCtx::OnAction(const CAction &action)
{
  switch (action.id)
  {
  case ACTION_MOVE_LEFT:
  case ACTION_MOVE_RIGHT:
  {
    if (GetFocusedControlID() == CONTROL_SEEK_BUTTON)
    {
      if(m_currentTime != m_totalTime)
      {
        if (g_application.m_pPlayer->CanSeek())
        {
          if (m_autoClosing) SetAutoClose(3000);

          if (!g_application.m_pPlayer->CanSeekToTime())
          {
            g_application.m_pPlayer->Seek(action.id == ACTION_MOVE_RIGHT,false);
          }
          else
          {
            m_onUserMove = true;
            m_seekHoldTime = action.holdTime;
            ChangeSeek(action.id == ACTION_MOVE_LEFT);
            g_infoManager.m_performingSeek = true;
            g_application.SeekTime(m_currentTime);
            m_lastSeekEvent = CTimeUtils::GetTimeMS();
          }

          return true;
        }
      }
    }
  }
  break;
  case ACTION_MOVE_DOWN:
  {
    if (GetFocusedControlID() == CONTROL_SEEK_BUTTON)
    {
      if (g_application.m_pPlayer && g_application.m_pPlayer->GetTotalTime() > 0)
      {
        m_showPlayTimeRemaining = !m_showPlayTimeRemaining;
        g_settings.SetSkinString(g_settings.TranslateSkinString("showPlayTimeRemaining"),m_showPlayTimeRemaining ? "1" : "0");
      }

      return true;
    }
  }
  break;
  default:
  {
    // nothing to do
  }
  break;
  }

  return CGUIDialogBoxeeCtx::OnAction(action);
}

bool CGUIDialogBoxeeSeekableCtx::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    if (GetFocusedControlID() == CONTROL_SEEK_BUTTON)
    {
      g_application.m_pPlayer->Pause();
      return true;
    }
  }
  break;
  }

  return CGUIDialogBoxeeCtx::OnMessage(message);
}

void CGUIDialogBoxeeSeekableCtx::Close(bool forceClose)
{
  m_seekDirectionOnOpen = CSeekDirection::NONE;
  CGUIDialogBoxeeCtx::Close(forceClose);
}

void CGUIDialogBoxeeSeekableCtx::Render()
{
  if (g_application.m_pPlayer->CanSeek())
  {
    if (!g_application.m_pPlayer->CanSeekToTime())
    {
      m_seekTimeLabel =  m_showPlayTimeRemaining ? g_infoManager.GetCurrentPlayTimeRemaining(m_labelTimeFormt) : g_infoManager.GetCurrentPlayTime(m_labelTimeFormt);
      //SET_CONTROL_LABEL(CONTROL_SEEK_BUTTON,m_seekTimeLabel);
    }
    else
    {
      if (!m_onUserMove && !g_infoManager.m_performingSeek)
      {
        m_seekTimeLabel =  m_showPlayTimeRemaining ? g_infoManager.GetCurrentPlayTimeRemaining(m_labelTimeFormt) : g_infoManager.GetCurrentPlayTime(m_labelTimeFormt);
        m_currentTime = g_application.GetTime();
      }
      else
      {
        double timeToShow = m_showPlayTimeRemaining ? (g_application.GetTotalTime() - m_currentTime) : m_currentTime;
        StringUtils::SecondsToTimeString((int)timeToShow, m_seekTimeLabel, m_labelTimeFormt);
      }

      if (!SetSeekControlPos((MAX_PROGRESS_POS_X)*m_currentTime/m_totalTime))
      {
        CLog::Log(LOGERROR,"CGUIDialogBoxeeSeekableCtx::Render - FAILED to set SeekControlPos (osds)");
        Close();
      }

      //SET_CONTROL_LABEL(CONTROL_SEEK_BUTTON,m_seekTimeLabel);

      if (m_lastSeekEvent && (CTimeUtils::GetTimeMS() - m_lastSeekEvent > 500))
      {
        m_onUserMove = false;
        m_lastSeekEvent = 0;
      }
    }
  }

  CGUIDialogBoxeeCtx::Render();
}

bool CGUIDialogBoxeeSeekableCtx::SetSeekControlPos(float playbackPosX)
{
  if (!m_buttonCtrl)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeSeekableCtx::SetSeekButtonPos - FAILED to get CGUIButtonControl (osds)");
    return false;
  }

  if (m_needToAdjustPlaybackPosX)
  {
    adjustPlaybackPosX(playbackPosX);
  }

  float seekButtonPosX = playbackPosX;
  float seekButtonPosY = m_buttonCtrl->GetYPosition();

  m_buttonCtrl->SetPosition(seekButtonPosX,seekButtonPosY);

  return true;
}

void CGUIDialogBoxeeSeekableCtx::ChangeSeek(bool back)
{
  unsigned int seekDelta = 0;
  unsigned int seekSpeed = 0;

  if (back)
    seekDelta = MIN_SEEK_OFFSET_BACKWARD;
  else
    seekDelta = MIN_SEEK_OFFSET_FARWARD;

  seekSpeed = seekDelta;

  if (m_seekHoldTime > 500)
  {
    float ratio;
    ratio = (m_seekHoldTime / 3000.0);
    if (ratio > 1.5)
      ratio = 1.5;
    seekSpeed = (unsigned int) (m_totalTime * (ratio / 100));
  }

  if (seekSpeed < seekDelta)
    seekSpeed = seekDelta;

  m_seekForward = !back;

  if (!back)
    m_currentTime += seekSpeed;
  else
    m_currentTime -= seekSpeed;

  if (m_currentTime < 0)
    m_currentTime = 0;

  if (m_currentTime > m_totalTime)
    m_currentTime = m_totalTime;

}

void CGUIDialogBoxeeSeekableCtx::SetSeekDirectionOnOpen(CSeekDirection::SeekDirectionEnums seekDirection)
{
  m_seekDirectionOnOpen = seekDirection;
}

void CGUIDialogBoxeeSeekableCtx::adjustPlaybackPosX(float& seekButtonPosX)
{
  seekButtonPosX = ((seekButtonPosX / MAX_PROGRESS_POS_X) * (END_PROGRESS_BUTTON_POS_X - START_PROGRESS_BUTTON_POS_X)) + START_PROGRESS_BUTTON_POS_X;
}

