/*
 * GUIWindowBoxeeMediaInfo.h
 *
 *  Created on: May 28, 2008
 *      Author: alex
 */

#ifndef GUIWINDOWBOXEEMEDIAINFO_H_
#define GUIWINDOWBOXEEMEDIAINFO_H_

#include "GUIWindow.h"
#include "FileItem.h"

class CFileItem;

class CGUIWindowBoxeeMediaInfo: public CGUIWindow
{
public:
  CGUIWindowBoxeeMediaInfo();
  virtual ~CGUIWindowBoxeeMediaInfo();

  void OnInitWindow();
  bool OnAction(const CAction &action);
  bool OnMessage(CGUIMessage& message);

  static void Show(CFileItem* pItem);

protected:
  void HandlePlayForVideoItem();
  void HandlePlayForPictureItem();

  CFileItem m_item;
};

#endif /* GUIWINDOWBOXEEMEDIAINFO_H_ */
