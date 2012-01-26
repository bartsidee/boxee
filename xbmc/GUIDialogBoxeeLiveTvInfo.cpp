#include "GUIDialogBoxeeLiveTvInfo.h"

#ifdef HAS_DVB

#include <boost/foreach.hpp>
#include "BoxeeUtils.h"
#include "GUIWindowManager.h"
#include "GUIInfoManager.h"
#include "cores/dvb/dvbmanager.h"
#include "cores/dvb/epgstore.h"
#include "GUISettings.h"
#include "utils/log.h"
#include "GUIWindowBoxeeLiveTv.h"
#include "GUIDialogBoxeeLiveTvEditChannels.h"

#define CONTROL_FAKE_LIST                5000
#define CONTROL_BUTTON_LIST              3333
#define CONTROL_TUNE_BUTTON              3330

CGUIDialogBoxeeLiveTvInfo::CGUIDialogBoxeeLiveTvInfo() : CGUIDialog(WINDOW_DIALOG_BOXEE_LIVETV_INFO, "boxee_livetv_info.xml")
{
  m_requestTune = false;
}

CGUIDialogBoxeeLiveTvInfo::~CGUIDialogBoxeeLiveTvInfo()
{
}

void CGUIDialogBoxeeLiveTvInfo::SetProgram(CFileItemPtr item)
{
  m_item = item;
}

void CGUIDialogBoxeeLiveTvInfo::OnInitWindow()
{
  CFileItemList list;
  list.Add(m_item);

  CGUIMessage msgBind(GUI_MSG_LABEL_BIND, GetID(), CONTROL_FAKE_LIST, 0, 0, &list);
  OnMessage(msgBind);

  SET_CONTROL_FOCUS(CONTROL_BUTTON_LIST, 0);
}

bool CGUIDialogBoxeeLiveTvInfo::OnAction(const CAction& action)
{
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeeLiveTvInfo::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_LABEL_BIND:
  {
    if (message.GetPointer() && message.GetControlId() == 0)
    {
      CFileItemList *items = (CFileItemList *)message.GetPointer();
      delete items;
      return true;
    }
  }
  break;

  case GUI_MSG_CLICKED:
  {
    if (message.GetSenderId() == CONTROL_TUNE_BUTTON)
    {
      m_requestTune = true;
      Close();
    }
  }
  break;

  default:
  break;
  }

  return CGUIDialog::OnMessage(message);
}

#endif

