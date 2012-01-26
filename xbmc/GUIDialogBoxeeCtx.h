#pragma once

#include "GUIDialog.h"
#include "FileItem.h"
#include "GUIDialogBoxeeVideoQuality.h"

class CGUIDialogBoxeeCtx : public CGUIDialog
{
public:
  CGUIDialogBoxeeCtx(DWORD dwID, const CStdString &xmlFile);
  virtual ~CGUIDialogBoxeeCtx(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual bool OnAction(const CAction &action);
  virtual void Close(bool forceClose = false);

  virtual void Update();

  virtual void SetItem(const CFileItem &item);

  virtual void OnMoreInfo() = 0;
  virtual void OnPlay();
  bool HandleQualityList();
  
protected:
  virtual void OnInitWindow();
  CStdString GetItemShareType();

  CGUIDialogBoxeeVideoQuality *m_pDlgVideoQuality;


  CFileItem m_item;
};

