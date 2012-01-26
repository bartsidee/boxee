/*
 * GUIWindowBoxeeMediaInfo.h
 *
 *  Created on: May 28, 2008
 *      Author: alex
 */

#ifndef GUIWINDOWBOXEEMEDIAINFO_H_
#define GUIWINDOWBOXEEMEDIAINFO_H_

#include "GUIDialog.h"
#include "FileItem.h"

class CFileItem;

class CGUIWindowBoxeeMediaInfo: public CGUIDialog
{
public:
  CGUIWindowBoxeeMediaInfo();
  virtual ~CGUIWindowBoxeeMediaInfo();

  void OnInitWindow();
  bool OnAction(const CAction &action);
  bool OnMessage(CGUIMessage& message);

  static void Show(CFileItem* pItem);

protected:

  CFileItem m_item;
};

#endif /* GUIWINDOWBOXEEMEDIAINFO_H_ */
