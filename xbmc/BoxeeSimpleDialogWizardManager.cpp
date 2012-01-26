#include "BoxeeSimpleDialogWizardManager.h"
#include "GUIWindowManager.h"
#include "GUIDialogBoxeeWizardBase.h"
#include "utils/log.h"

CBoxeeSimpleDialogWizardManager::CBoxeeSimpleDialogWizardManager()
{
  m_isWizardComplited = false;
  m_shouldEndWizardOnEmptyStack = false;
}

CBoxeeSimpleDialogWizardManager::~CBoxeeSimpleDialogWizardManager()
{

}

bool CBoxeeSimpleDialogWizardManager::Run(int startDialogId, bool shouldEndWizardOnEmptyStack)
{
  m_wizardCancelled = false;

  CLog::Log(LOGDEBUG,"CBoxeeSimpleDialogWizardManager::Run - ENTER wizard (digwiz)");

  CGUIDialogBoxeeWizardBase* pDialog = (CGUIDialogBoxeeWizardBase*)g_windowManager.GetWindow(startDialogId);
  if (!pDialog)
  {
    CLog::Log(LOGERROR,"CBoxeeSimpleDialogWizardManager::Run - FAILED to get start dialog (digwiz)");
    return false;
  }

  ClearStack();

  m_shouldEndWizardOnEmptyStack = shouldEndWizardOnEmptyStack;
  m_isWizardComplited = false;

  CLog::Log(LOGDEBUG,"CBoxeeSimpleDialogWizardManager::Run - going to START wizard. [EndWizardOnEmptyStack=%d][isWizardComplited=%d][StackSize=%zu] (digwiz)",m_shouldEndWizardOnEmptyStack,m_isWizardComplited,m_dialogStack.size());

  while (!m_isWizardComplited)
  {
    if (!pDialog)
    {
      CLog::Log(LOGERROR,"CBoxeeSimpleDialogWizardManager::Run - FAILED to get next dialog -> Break (digwiz)");
      break;
    }

    pDialog->DoModal();

    pDialog = GetNextDialog(pDialog);
  }

  CLog::Log(LOGDEBUG,"CBoxeeSimpleDialogWizardManager::Run - going to STOP wizard. [EndWizardOnEmptyStack=%d][isWizardComplited=%d][StackSize=%zu] (digwiz)",m_shouldEndWizardOnEmptyStack,m_isWizardComplited,m_dialogStack.size());

  if (m_isWizardComplited)
  {
    CLog::Log(LOGDEBUG,"CBoxeeSimpleDialogWizardManager::Run - since [isWizardComplited=%d=TRUE] going to call OnWizardComplete (digwiz)",m_isWizardComplited);
    OnWizardComplete();
  }

  CLog::Log(LOGDEBUG,"CBoxeeSimpleDialogWizardManager::Run - EXIT wizard (digwiz)");

  //g_windowManager.PreviousWindow();

  return !m_wizardCancelled;
}

CGUIDialogBoxeeWizardBase* CBoxeeSimpleDialogWizardManager::GetNextDialog(CGUIDialogBoxeeWizardBase* pDialog)
{
  if (!pDialog)
  {
    CLog::Log(LOGERROR,"CBoxeeSimpleDialogWizardManager::GetNextDialog - Enter function with a NULL pointer (digwiz)");
    return NULL;
  }

  int id = pDialog->GetID();

  CActionChoose::ActionChooseEnums actionChose = pDialog->GetActionChose();
  if (actionChose == CActionChoose::ACTION_ERROR)
  {
    CLog::Log(LOGERROR,"CBoxeeSimpleDialogWizardManager::GetNextDialog - Action chose enum is ERROR (digwiz)");
    return NULL;
  }

  CGUIDialogBoxeeWizardBase* pNextWindow = NULL;
  bool addCurrentDlgToStack = true;

  switch(actionChose)
  {
  case CActionChoose::NEXT:
  {
    CLog::Log(LOGDEBUG,"CBoxeeSimpleDialogWizardManager::GetNextDialog - For [windowId=%d] handling [ActionChose=%d=NEXT] (digwiz)",pDialog->GetID(),actionChose);

    pNextWindow = HandleNextAction(pDialog,addCurrentDlgToStack);

    if (addCurrentDlgToStack && (m_dialogStack.empty() || m_dialogStack.top() != id))
    {
      m_dialogStack.push(id);
    }
    else
    {
      CLog::Log(LOGDEBUG,"CBoxeeSimpleDialogWizardManager::GetNextDialog - Not adding [id=%d] to stack. [addCurrentDlgToStack=%d][DialogStackSize=%d] (digwiz)",id,addCurrentDlgToStack,(int)m_dialogStack.size());
    }
  }
  break;
  case CActionChoose::BACK:
  {
    CLog::Log(LOGDEBUG,"CBoxeeSimpleDialogWizardManager::GetNextDialog - For [windowId=%d] handling [ActionChose=%d=BACK] (digwiz)",pDialog->GetID(),actionChose);

    pNextWindow = HandleBackAction(pDialog);
  }
  break;
  default:
  {
    CLog::Log(LOGERROR,"CBoxeeSimpleDialogWizardManager::GetNextDialog - FAILED to handle ActionChoseEnum [%d] (digwiz)",actionChose);
  }
  break;
  }

  return pNextWindow;
}

CGUIDialogBoxeeWizardBase* CBoxeeSimpleDialogWizardManager::HandleBackAction(CGUIDialogBoxeeWizardBase* pDialog)
{
  if (!pDialog)
  {
    CLog::Log(LOGERROR,"CBoxeeSimpleDialogWizardManager::HandleBackAction - Enter function with a NULL pointer (digwiz)");
    return NULL;
  }

  CGUIDialogBoxeeWizardBase* pNextDialog = NULL;

  if ((int)m_dialogStack.size() < 1)
  {
    if (m_shouldEndWizardOnEmptyStack)
    {
      CLog::Log(LOGWARNING,"CBoxeeSimpleDialogWizardManager::HandleBackAction - Size of dialogStack is [%d] (initbox)",(int)m_dialogStack.size());
      m_wizardCancelled = true;
      return NULL;
    }
    else
    {
      pNextDialog = pDialog;
    }
  }
  else
  {
    bool foundBackWindow = false;
    int nextDialogId;

    while (!foundBackWindow)
    {
      nextDialogId = m_dialogStack.top();
      m_dialogStack.pop();

      if (nextDialogId != pDialog->GetID() || ((int)m_dialogStack.size()) < 1)
      {
        // not the same id OR no more id's in the stack -> break
        foundBackWindow = true;
      }
    }

    pNextDialog = (CGUIDialogBoxeeWizardBase*)g_windowManager.GetWindow(nextDialogId);

    if (!pNextDialog)
    {
      CLog::Log(LOGERROR,"CBoxeeSimpleDialogWizardManager::HandleBackAction - FAILED to get dialog for [id=%d] (digwiz)",nextDialogId);
    }
  }

  return pNextDialog;
}

bool CBoxeeSimpleDialogWizardManager::IsWizardComplited()
{
  return m_isWizardComplited;
}

void CBoxeeSimpleDialogWizardManager::SetWizardComplete(bool isWizardComplited)
{
  m_isWizardComplited = isWizardComplited;
}

bool CBoxeeSimpleDialogWizardManager::IsShouldEndWizardOnEmptyStack()
{
  return m_shouldEndWizardOnEmptyStack;
}

void CBoxeeSimpleDialogWizardManager::SetIsShouldEndWizardOnEmptyStack(bool isShouldEndWizardOnEmptyStack)
{
  m_shouldEndWizardOnEmptyStack = isShouldEndWizardOnEmptyStack;
}

bool CBoxeeSimpleDialogWizardManager::AddToStack(int id)
{
  CLog::Log(LOGDEBUG,"CBoxeeSimpleDialogWizardManager::AddToStack - enter function [stackSize=%d][idOnTop=%d] (digwiz)",(int)m_dialogStack.size(),m_dialogStack.empty() ? -1 : m_dialogStack.top());

  if (m_dialogStack.empty() || m_dialogStack.top() != id)
  {
    m_dialogStack.push(id);
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeSimpleDialogWizardManager::AddToStack - NOT adding [id=%d] to stack. [stackSize=%d] (digwiz)",id,(int)m_dialogStack.size());
  }

  CLog::Log(LOGDEBUG,"CBoxeeSimpleDialogWizardManager::AddToStack - exit function [stackSize=%d][idOnTop=%d] (digwiz)",(int)m_dialogStack.size(),m_dialogStack.empty() ? -1 : m_dialogStack.top());

  return true;
}

void CBoxeeSimpleDialogWizardManager::ClearStack()
{
  // clear dialog stack
  while(!m_dialogStack.empty())
  {
    m_dialogStack.pop();
  }
}

int CBoxeeSimpleDialogWizardManager::GetStackSize()
{
  return (int)m_dialogStack.size();
}
