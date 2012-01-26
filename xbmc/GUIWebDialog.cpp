/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */


#include "GUIWebDialog.h"
#include "GUIWindowManager.h"
#include "guilib/GUIWebControl.h"

#define WEB_CONTROL 1000

CGUIWebDialog::CGUIWebDialog(void)
    : CGUIDialogBoxBase(WINDOW_DIALOG_WEB, "DialogWeb.xml")
{
}

CGUIWebDialog::~CGUIWebDialog(void)
{}

bool CGUIWebDialog::OnAction(const CAction &action)
{
  if (action.id == ACTION_CLOSE_DIALOG || action.id == ACTION_PREVIOUS_MENU)
  {
    m_bConfirmed = false;
    Close();
    return true;
  }

  if (m_webControl->OnAction(action))
      return true;

  return CGUIDialogBoxBase::OnAction(action);
}

void CGUIWebDialog::OnInitWindow()
{
  CGUIDialogBoxBase::OnInitWindow();

  SetProperty("PassthroughKeys",true);

  m_webControl = (CGUIWebControl*) GetControl(WEB_CONTROL);

  if (m_webControl)
  {
    m_bLoading = true;
    SetProperty("loading",m_bLoading);
    m_webControl->LoadUrl(m_url);
    m_webControl->SetVisible(false);
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIWebDialog::OnInitWindow, there's no webcontrol [id=%d] defined in the skin. closing.",WEB_CONTROL);
    Close();
  }
}

void CGUIWebDialog::OnDeinitWindow(int nextWindowID)
{
  CGUIWindow::OnDeinitWindow(nextWindowID);
}

void CGUIWebDialog::Render()
{
  if (m_webControl && m_webControl->isLoading() != m_bLoading)
  {
    m_bLoading = m_webControl->isLoading();
    SetProperty("loading",m_bLoading);

    if (!m_bLoading)
    {
      m_webControl->SetVisible(true);
    }
  }

  CGUIDialog::Render();
}

bool CGUIWebDialog::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      if (message.GetSenderId() == WEB_CONTROL && message.GetParam1() == 0)
      {
        m_bConfirmed = true;
        Close();
        return true;
      }
    }
    break;
    case GUI_MSG_SET_FILENAME:
    {
      SetURL(message.GetStringParam());
      return true;
    }
    break;
  }
  return CGUIDialogBoxBase::OnMessage(message);
}

void CGUIWebDialog::SetURL(const CStdString& url)
{
  m_url = url;
}

bool CGUIWebDialog::ShowAndGetInput(const CStdString& url)
{
  CLog::Log(LOGDEBUG,"CGUIWebDialog::ShowAndGetInput, going to open up the browsers at %s (wip)",url.c_str());
  CGUIWebDialog *dialog = (CGUIWebDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_WEB);
  if (!dialog)
    return false;

  dialog->SetURL(url);
  dialog->DoModal();

  return dialog->IsConfirmed();
}
