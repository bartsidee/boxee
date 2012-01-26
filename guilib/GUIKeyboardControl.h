/*!
\file GUIKeyboardControl.h
\brief 
*/

#ifndef GUILIB_GUIKeyboardControl_H
#define GUILIB_GUIKeyboardControl_H

#pragma once

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

#include "GUIControlGroup.h"
#include "GUIEditControl.h"

/*!
 \ingroup controls
 \brief 
 */

class CGUIEditControl;
class CGUIPanelContainer;
class CGUIButtonControl;

class CGUIKeyboardControl : public CGUIControlGroup
{
public:

  CGUIKeyboardControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY,
                  float width, float height, const CTextureInfo &textureFocus, const CTextureInfo &textureNoFocus,
                  const CLabelInfo& labelInfo, const std::string &text);
  CGUIKeyboardControl(const CGUIKeyboardControl &keyboard);
  virtual ~CGUIKeyboardControl(void);
  virtual CGUIKeyboardControl *Clone() const { return new CGUIKeyboardControl(*this); };

  // Function that delegate to edit control
  void SetInputType(CGUIEditControl::INPUT_TYPE type, int heading);
  void SetTextChangeActions(const std::vector<CGUIActionDescriptor>& textChangeActions);

protected:

  CGUIEditControl* m_editControl;
  CGUIPanelContainer* m_keyPanel;
  CGUIButtonControl* m_EnterButton;

};
#endif
