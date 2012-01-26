#ifndef GUIDIALOGBOXEEWIZARDBASE_H_
#define GUIDIALOGBOXEEWIZARDBASE_H_

#include "GUIDialog.h"

#define DIALOG_WIZARD_BUTTON_NEXT 1
#define DIALOG_WIZARD_BUTTON_BACK 2

class CActionChoose
{
public:
  enum ActionChooseEnums
  {
    ACTION_ERROR=0,
    NEXT=1,
    BACK=2,
    NUM_OF_NEXT_ACTIONS=3
  };
};

class CGUIDialogBoxeeWizardBase : public CGUIDialog
{
public:
  CGUIDialogBoxeeWizardBase(int id, const CStdString& xmlFile, const CStdString& derivedClassStr);
  virtual ~CGUIDialogBoxeeWizardBase();

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage &message);

  virtual CActionChoose::ActionChooseEnums GetActionChose();
  virtual void SetActionChose(CActionChoose::ActionChooseEnums actionChoseEnum);

protected:
  virtual bool HandleClickNext() { return true; }
  virtual bool HandleClickBack() { return true; }

  CActionChoose::ActionChooseEnums m_actionChoseEnum;
  CStdString m_name;
};

#endif // GUIDIALOGBOXEEWIZARDBASE_H_
