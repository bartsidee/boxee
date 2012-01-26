/*
 * CGUIDialogBoxeeVideoQuality.cpp
 *
 */
#include "GUIDialogBoxeeVideoQuality.h"
#include "GUIRadioButtonControl.h"
#include "GUIListContainer.h"
#include "GUIWindowManager.h"
#include "GUIImage.h"
#include "utils/log.h"
#include "LocalizeStrings.h"

#define CONTROL_QUALITY_LIST_ACTION_DIALOG     11
#define CONTROL_QUALITY_LIST_OSD_DIALOG        12

CGUIDialogBoxeeVideoQuality::CGUIDialogBoxeeVideoQuality(void) : CGUIDialog(WINDOW_DIALOG_BOXEE_VIDEO_QUALITY, "boxee_video_quality.xml"), m_dialogType(FULL_CVQ_DIALOG), m_focusedItemPos (0)
{
  m_listControlId = CONTROL_QUALITY_LIST_ACTION_DIALOG;
}

CGUIDialogBoxeeVideoQuality::~CGUIDialogBoxeeVideoQuality(void)
{
}

bool CGUIDialogBoxeeVideoQuality::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    int iControl = message.GetSenderId();

    if (iControl == CONTROL_QUALITY_LIST_ACTION_DIALOG || iControl == CONTROL_QUALITY_LIST_OSD_DIALOG)
    {
      if (message.GetParam1() != ACTION_BUILT_IN_FUNCTION)
      {
        // Handle only GUI_MSG_CLICKED on CONTROL_QUALITY_LIST that origin from navigation
        CLog::Log(LOGDEBUG,"CGUIDialogBoxeeVideoQuality::OnMessage - Enter GUI_MSG_CLICKED case with [SenderId=CONTROL_QUALITY_LIST]. Going to call ProccessItemSelected() (cvq)");
        ProccessItemSelected();
        Close();
        return true;
      }
    }
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogBoxeeVideoQuality::OnInitWindow()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeVideoQuality::OnInitWindow - Enter function (cvq)");

  m_canceled = true;
  m_savePreference = false;
  CGUIDialog::OnInitWindow();
  LoadList();
}

void CGUIDialogBoxeeVideoQuality::ChangeDialogType(int dialogType)
{
  if (dialogType >= MAX_CVQ_DIALOG)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeVideoQuality::ChangeDialogType Dialog type %d doesnt exist ", dialogType);
  }

  m_dialogType = dialogType;

  if (m_dialogType == FULL_CVQ_DIALOG)
    m_listControlId = CONTROL_QUALITY_LIST_ACTION_DIALOG;
  else
    m_listControlId = CONTROL_QUALITY_LIST_OSD_DIALOG;

  CGUIDialogBoxeeVideoQuality *dialog = (CGUIDialogBoxeeVideoQuality *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_VIDEO_QUALITY);
  dialog->AllocResources();

  SetProperty("dialog-type", m_dialogType);
}

void CGUIDialogBoxeeVideoQuality::Close(bool forceClose)
{
  CGUIDialog::Close(forceClose);
}

void CGUIDialogBoxeeVideoQuality::Reset()
{
  m_vecList.Clear();
  m_focusedItemPos = 0;
}

void CGUIDialogBoxeeVideoQuality::Add(const CFileItem& item)
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeVideoQuality::Add - Add  [%s] (cvq)",item.GetProperty("quality-lbl").c_str());
  CFileItemPtr pItem(new CFileItem(item));
  m_vecList.Add(pItem);
}

void CGUIDialogBoxeeVideoQuality::LoadList()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeVideoQuality::LoadList");

  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), m_listControlId);
  OnMessage(msgReset);

  for (int i = 0 ; i < m_vecList.Size();  i++ )
  {
    CFileItemPtr quality (new CFileItem((*m_vecList[i])));
    quality->SetLabel(quality->GetProperty("quality-lbl"));

    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), m_listControlId, 0, 0, quality);
    OnMessage(msg);
  }

  if (m_focusedItemPos < m_vecList.Size())
  {
    SET_CONTROL_FOCUS(m_listControlId, m_focusedItemPos);
  }
  else
  {
    SET_CONTROL_FOCUS(m_listControlId,0);
  }
}

void CGUIDialogBoxeeVideoQuality::ProccessItemSelected()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeVideoQuality::ProccessItemSelected");

  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), m_listControlId);
  OnMessage(msg);

  m_selectedItemPos = msg.GetParam1();
  m_canceled = false;
}

int CGUIDialogBoxeeVideoQuality::GetSelectedItemPos()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeVideoQuality::GetItemSelected");
  return m_selectedItemPos;
}
