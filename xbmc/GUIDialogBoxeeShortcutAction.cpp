/*
 * GUIWindowBoxeeShortcutDialog.cpp
 *
 *  Created on: Jul 14, 2009
 *      Author: yuvalt
 */


#include "GUIDialogBoxeeShortcutAction.h"

#define CONTROL_MOVE_SHORTCUT   10
#define CONTROL_REMOVE_SHORTCUT 20
#define CONTROL_EXIT            30

CGUIDialogBoxeeShortcutAction::CGUIDialogBoxeeShortcutAction() : 
  CGUIDialog(WINDOW_BOXEE_SHORTCUT_DIALOG, "boxee_shortcut_dialog.xml")
{
  m_allowRemove = true;
}

CGUIDialogBoxeeShortcutAction::~CGUIDialogBoxeeShortcutAction()
{
}

void CGUIDialogBoxeeShortcutAction::SetAllowRemove(bool allowRemove)
{
  m_allowRemove = allowRemove;
}

bool CGUIDialogBoxeeShortcutAction::OnAction(const CAction & action)
{
  if (action.id == ACTION_PREVIOUS_MENU)
  {
    CancelDialog();
    return true;
  }

  return CGUIDialog::OnAction(action);  
}

void CGUIDialogBoxeeShortcutAction::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  CONTROL_SELECT(CONTROL_MOVE_SHORTCUT);
}

bool CGUIDialogBoxeeShortcutAction::OnMessage(CGUIMessage & message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      m_isCancelled = false;
      m_isActionMove = false;
      m_isActionRemove = false;
      
      bool result = CGUIDialog::OnMessage(message);
      
      if (!m_allowRemove)
      {
        SET_CONTROL_HIDDEN(CONTROL_REMOVE_SHORTCUT);
      }
      else
      {
        SET_CONTROL_VISIBLE(CONTROL_REMOVE_SHORTCUT);
      }
      
      return result;
      
      break;
    } 
    case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      
      if (iControl == CONTROL_MOVE_SHORTCUT)
      {
        m_isActionMove = true;        
        Close();
        return true;
      }
      else if (iControl == CONTROL_REMOVE_SHORTCUT)
      {
        m_isActionRemove = true;
        Close();
        return true;
      }
      else if (iControl == CONTROL_EXIT)
      {
        CancelDialog();
        return true;
      }
    }
    break;      
  }
  
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogBoxeeShortcutAction::CancelDialog()
{
  m_isCancelled = true;
  Close();
}

bool CGUIDialogBoxeeShortcutAction::IsCancelled()
{
  return m_isCancelled;
}

bool CGUIDialogBoxeeShortcutAction::IsActionMove()
{
  return m_isActionMove;
}

bool CGUIDialogBoxeeShortcutAction::IsActionRemove()
{
  return m_isActionRemove;
}
