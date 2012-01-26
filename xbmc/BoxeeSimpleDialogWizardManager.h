#ifndef BOXEESIMPLEDIALOGWIZARD_H_
#define BOXEESIMPLEDIALOGWIZARD_H_

#include <stack>

class CGUIDialogBoxeeWizardBase;

class CBoxeeSimpleDialogWizardManager
{
public:
  CBoxeeSimpleDialogWizardManager();
  virtual ~CBoxeeSimpleDialogWizardManager();

  bool Run(int startDialogId, bool shouldEndWizardOnEmptyStack = false);

  bool IsWizardComplited();
  void SetWizardComplete(bool isWizardComplited);

  bool IsShouldEndWizardOnEmptyStack();
  void SetIsShouldEndWizardOnEmptyStack(bool isShouldEndWizardOnEmptyStack);

  void ClearStack();
  bool AddToStack(int id);
  int GetStackSize();

  bool IsWizardCancelled() { return m_wizardCancelled; }

protected:
  virtual CGUIDialogBoxeeWizardBase* HandleNextAction(CGUIDialogBoxeeWizardBase* pDialog, bool& addCurrentDlgToStack) = 0;

  virtual bool OnWizardComplete() { return true; }

private:
  CGUIDialogBoxeeWizardBase* GetNextDialog(CGUIDialogBoxeeWizardBase* pDialog);
  CGUIDialogBoxeeWizardBase* HandleBackAction(CGUIDialogBoxeeWizardBase* pDialog);

  std::stack<int> m_dialogStack;
  bool m_shouldEndWizardOnEmptyStack;
  bool m_isWizardComplited;

  bool m_wizardCancelled;
};

#endif /*IBOXEESIMPLEDIALOGWIZARD_H_*/

