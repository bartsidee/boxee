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


#define CONTROL_QUALITY_GROUP_LIST       10
#define CONTROL_QUALITY_LIST             11
#define SAVE_PREF_RADIO_BTN              3
#define CONTROL_WINDOW_POPUP_IMAGE       20

typedef struct
{
	int posX;
	int posY;
	int width;
	int height;
}  rectDefT;

rectDefT   dialogDim[] = {
		/*type = 1 */{440,122,400,472},
		/*type = 2 */ {900,572,240,150}};

using namespace std;

CGUIDialogBoxeeVideoQuality::CGUIDialogBoxeeVideoQuality(void) : CGUIDialog(WINDOW_DIALOG_BOXEE_VIDEO_QUALITY, "boxee_video_quality.xml"), m_dialogType(FULL_CVQ_DIALOG), m_focusedItemPos (0)
{
}

CGUIDialogBoxeeVideoQuality::~CGUIDialogBoxeeVideoQuality(void)
{
}

bool CGUIDialogBoxeeVideoQuality::OnAction(const CAction &action)
{
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeeVideoQuality::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    int iControl = message.GetSenderId();

    if(iControl == CONTROL_QUALITY_LIST)
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
    else if (iControl == SAVE_PREF_RADIO_BTN)
    {
      m_savePreference = ((CGUIRadioButtonControl*) GetControl(SAVE_PREF_RADIO_BTN))->IsSelected();
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

  CGUIDialogBoxeeVideoQuality *dialog = (CGUIDialogBoxeeVideoQuality *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_VIDEO_QUALITY);
  dialog->AllocResources();


  m_dialogType = dialogType;

  SetProperty("dialog-type", m_dialogType);

  CGUIRadioButtonControl*  radioControl =  ((CGUIRadioButtonControl*) GetControl(SAVE_PREF_RADIO_BTN));
  if (!radioControl)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeVideoQuality::ChangeDialogType - can't get radiobutton Control");
  }
  CGUIListContainer*       listControl = (CGUIListContainer *)GetControl(CONTROL_QUALITY_GROUP_LIST);
  if (!listControl)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeVideoQuality::ChangeDialogType - can't get list Control");
  }

  switch (dialogType)
  {
  case FULL_CVQ_DIALOG:
  {
	  dialog->SetPosition(dialogDim[m_dialogType].posX, dialogDim[m_dialogType].posY);
	  radioControl->AllocResources();
	  radioControl->SetPosition(66,190);
	  radioControl->SetWidth(270);
	  radioControl->SetHeight(28);
	  radioControl->SetLabel(g_localizeStrings.Get(12029));

	  listControl->AllocResources();
	  listControl->SetPosition(100,250);
	  listControl->SetWidth(220);
	  listControl->SetHeight(340);
	  break;
  }
  case LIST_CVQ_DIALOG:
  {
	  CGUIImage*   windowImage = ((CGUIImage *) GetControl(CONTROL_WINDOW_POPUP_IMAGE));
	  dialog->AllocResources();
	  int posX = dialogDim[m_dialogType].posX;
	  int posY = dialogDim[m_dialogType].posY  - (m_vecList.Size() * 40);
      windowImage->SetHeight(windowImage->GetHeight() + (m_vecList.Size() * 40));

	  dialog->SetPosition(posX, posY);
	  radioControl->AllocResources();
	  radioControl->SetPosition(10,10);
	  radioControl->SetWidth(100);
	  radioControl->SetHeight(28);
      radioControl->SetLabel(g_localizeStrings.Get(12030));

	  listControl->AllocResources();
	  listControl->SetPosition(10,40);
	  listControl->SetWidth(220);
	  listControl->SetHeight(340);

	  break;
  }
  }

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

  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_QUALITY_LIST);
  OnMessage(msgReset);

  for (int i = 0 ; i <  (int)m_vecList.Size();  i++ )
  {
    CFileItemPtr quality (new CFileItem((*m_vecList[i])));
    quality->SetLabel(quality->GetProperty("quality-lbl"));

    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_QUALITY_LIST, 0, 0, quality);
    OnMessage(msg);
  }

  if (m_focusedItemPos < m_vecList.Size())
  {
    SET_CONTROL_FOCUS(CONTROL_QUALITY_LIST,m_focusedItemPos);
  }
  else
  {
    SET_CONTROL_FOCUS(CONTROL_QUALITY_LIST,0);
  }

}

void CGUIDialogBoxeeVideoQuality::ProccessItemSelected()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeVideoQuality::ProccessItemSelected");

  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_QUALITY_LIST);
  OnMessage(msg);

  m_selectedItemPos = msg.GetParam1();
  m_canceled = false;

}

int CGUIDialogBoxeeVideoQuality::GetSelectedItemPos()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeVideoQuality::GetItemSelected");
  return m_selectedItemPos;
}


