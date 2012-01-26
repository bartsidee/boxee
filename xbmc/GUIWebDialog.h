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

#include "GUIDialogBoxBase.h"
#include "GUIWebControl.h"

class CGUIWebDialog : public CGUIDialogBoxBase
{
public:
  CGUIWebDialog(void);
  virtual ~CGUIWebDialog(void);
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  static bool ShowAndGetInput(const CStdString& url);
  virtual void Render();
  void SetURL(const CStdString &url);
  virtual void OnInitWindow();

protected:
  virtual void OnDeinitWindow(int nextWindowID);

private:
  CStdString m_url;
  CGUIWebControl* m_webControl;
  bool m_bLoading;
};
