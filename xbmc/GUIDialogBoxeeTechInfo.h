/*
 * GUIDialogBoxeeTechInfo.h
 *
 *  Created on: Jan 17, 2011
 *      Author: shayyizhak
 */

#ifndef GUIDIALOGBOXEETECHINFO_H_
#define GUIDIALOGBOXEETECHINFO_H_

#include "GUIDialog.h"
#include "Key.h"
#include "GUIBaseContainer.h"
#include "FileItem.h"
#include "GUIDialogBoxeeDropdown.h"


class CGUIDialogBoxeeTechInfo : public CGUIDialog
{
public:
  CGUIDialogBoxeeTechInfo();
  virtual ~CGUIDialogBoxeeTechInfo();

  static void Show();

  virtual void OnInitWindow();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);

  virtual void Render();

private:

  bool m_bShowViewModeInfo;
  bool m_timeCodeShow;
  int m_timeCodePosition;
  unsigned int m_timeCodeTimeout;
  int m_timeCodeStamp[5];
  unsigned int m_dwShowViewModeTimeout;

  CFileItemList m_items;
  CStdString m_value;
  int m_type;
  bool LoadItems();

  ICustomDropdownLabelCallback* m_getDropdownLabelCallback;

  CStdString FormatAudioInfo(CStdString strAudio);
  CStdString FormatVideoInfo(CStdString strVideo);
  CStdString FormatGeneralInfo(CStdString strGeneral);
  CStdString removeNItem(int n, CStdString str);
};



#endif /* GUIDIALOGBOXEETECHINFO_H_ */
