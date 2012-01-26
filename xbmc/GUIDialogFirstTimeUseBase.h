#pragma once

#include "system.h"

#ifdef HAS_EMBEDDED

#include "GUIDialog.h"

#define BUTTON_NEXT 250
#define BUTTON_BACK 251

class CActionChose
{
public:
  enum ActionChoseEnums
  {
    ERROR=0,
    NEXT=1,
    BACK=2,
    NUM_OF_NEXT_ACTIONS=3
  };
};

class CGUIDialogFirstTimeUseBase : public CGUIDialog
{
public:
  
  CGUIDialogFirstTimeUseBase(int id, const CStdString& xmlFile, const CStdString& derivedClassStr);
  virtual ~CGUIDialogFirstTimeUseBase();

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage &message);

  virtual CActionChose::ActionChoseEnums GetActionChose();
  virtual void SetActionChose(CActionChose::ActionChoseEnums actionChoseEnum);

protected:

  virtual bool HandleClickNext() = 0;
  virtual bool HandleClickBack() = 0;

  CActionChose::ActionChoseEnums m_actionChoseEnum;
  CStdString m_name;
};

#endif
