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

#include "GUIKeyboardControl.h"
#include "GUIButtonControl.h"
#include "GUIEditControl.h"
#include "GUIPanelContainer.h"


#include "utils/CharsetConverter.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogNumeric.h"
#include "LocalizeStrings.h"
#include "DateTime.h"
#include "Clipboard.h"

#ifdef __APPLE__
#include "CocoaInterface.h"
#endif

using namespace std;

#ifdef WIN32
extern HWND g_hWnd;
#endif

CGUIKeyboardControl::CGUIKeyboardControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY,
                                 float width, float height, const CTextureInfo &textureFocus, const CTextureInfo &textureNoFocus,
                                 const CLabelInfo& labelInfo, const std::string &text)
    : CGUIControlGroup(dwParentID, dwControlId, posX, posY, width, height)
{
  ControlType = GUICONTROL_KEYBOARD;

  m_editControl = new CGUIEditControl(dwParentID, dwControlId, posX, posY, width, height, textureFocus, textureNoFocus, labelInfo, text, labelInfo, "");
}

CGUIKeyboardControl::CGUIKeyboardControl(const CGUIKeyboardControl &keyboard)
    : CGUIControlGroup(keyboard)
{
  ControlType = GUICONTROL_KEYBOARD;

}

CGUIKeyboardControl::~CGUIKeyboardControl(void)
{
}

void CGUIKeyboardControl::SetInputType(CGUIEditControl::INPUT_TYPE type, int heading)
{
  m_editControl->SetInputType(type, heading);
}

void CGUIKeyboardControl::SetTextChangeActions(const std::vector<CGUIActionDescriptor>& textChangeActions)
{
  m_editControl->SetTextChangeActions(textChangeActions);
}
