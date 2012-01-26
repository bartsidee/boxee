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
 
#include "dlgcache.h"
#include "GUIWindowManager.h"
#include "GUIDialogProgress.h"
#include "LocalizeStrings.h"
#include "utils/log.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "utils/SingleLock.h"
#include "utils/TimeUtils.h"

CDlgCache::CDlgCache(DWORD dwDelay, const CStdString& strHeader, const CStdString& strMsg)
{
  m_pDlg = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

  /* if progress dialog is already running, take it over */
  if( m_pDlg->IsDialogRunning() )
    dwDelay = 0;

  m_strHeader = strHeader;
  m_strLinePrev = strMsg;

  bSentCancel = false;
  if(dwDelay == 0)
    OpenDialog();    
  else
    m_dwTimeStamp = CTimeUtils::GetTimeMS() + dwDelay;

  Create(true);
}

void CDlgCache::Close(bool bForceClose)
{
  bSentCancel = true;

  if (m_pDlg && m_pDlg->IsDialogRunning())
  {
    ThreadMessage tMsg (TMSG_CLOSE_DIALOG);
    tMsg.lpVoid = m_pDlg;
    tMsg.dwParam1 = true;
    g_application.getApplicationMessenger().SendMessage(tMsg,false);
  }

  //Set stop, this will kill this object, when thread stops
  CThread::m_bStop = true;
}

CDlgCache::~CDlgCache()
{
  Close(true);
}

void CDlgCache::OpenDialog()
{  
  if (m_strHeader.IsEmpty())
    m_pDlg->SetHeading(438);
  else
    m_pDlg->SetHeading(m_strHeader);
  m_pDlg->SetLine(2, m_strLinePrev);
  
  g_application.getApplicationMessenger().DialogProgressShow();
  
}

void CDlgCache::SetHeader(const CStdString& strHeader)
{
  m_strHeader = strHeader;
}

void CDlgCache::SetHeader(int nHeader)
{
  SetHeader(g_localizeStrings.Get(nHeader));
}

void CDlgCache::SetMessage(const CStdString& strMessage)
{
  m_pDlg->SetLine(0, m_strLinePrev2);
  m_pDlg->SetLine(1, m_strLinePrev);
  m_pDlg->SetLine(2, strMessage);
  m_strLinePrev2 = m_strLinePrev;
  m_strLinePrev = strMessage; 
}

bool CDlgCache::OnFileCallback(void* pContext, int ipercent, float avgSpeed)
{
  m_pDlg->ShowProgressBar(true);
  m_pDlg->SetPercentage(ipercent); 

  if( IsCanceled() ) 
    return false;
  return true;
}

void CDlgCache::Process()
{
  bSentCancel = false;
  while( !CThread::m_bStop )
  {    
    try 
    {
      // instead of calling the blocking pDlg->Progress, use application messenger to send asynch message
      ThreadMessage tMsg (TMSG_GUI_WIN_MANAGER_PROCESS);
      tMsg.dwParam1 = (DWORD)false;
      g_application.getApplicationMessenger().SendMessage(tMsg, false);

      if( !bSentCancel && m_pDlg->IsCanceled())
      {
        bSentCancel = true;
        break;
      }
        else if( !m_pDlg->IsDialogRunning() && CTimeUtils::GetTimeMS() > m_dwTimeStamp 
              && !g_windowManager.IsWindowActive(WINDOW_DIALOG_YES_NO) )
        OpenDialog();        
    }
    catch(...)
    {
      CLog::Log(LOGERROR, "Exception in CDlgCache::Process()");
    }
    Sleep(200);
  }

  // cant finish untill Close was specifically called (so we dont delete the instance)
  while( !CThread::m_bStop )
    Sleep(500);

}

void CDlgCache::ShowProgressBar(bool bOnOff) 
{ 
  m_pDlg->ShowProgressBar(bOnOff); 
}
void CDlgCache::SetPercentage(int iPercentage) 
{ 
  m_pDlg->SetPercentage(iPercentage); 
}
bool CDlgCache::IsCanceled() const
{
  if (m_pDlg->IsDialogRunning())
    return m_pDlg->IsCanceled();
  return bSentCancel;
}
