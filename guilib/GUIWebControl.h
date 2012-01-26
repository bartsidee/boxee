#pragma once

/*
 *      Copyright (C) 2005-2011 Team Boxee
 *      http://www.boxee.tv
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

#include "GUIControl.h"
#include "cores/IPlayer.h"
#include "cores/flashplayer/FlashVideoPlayer.h"

class CGUIWebControl : public CGUIControl, public IPlayerCallback
{
public:
  CGUIWebControl(int parentID, int controlID, float posX, float posY, float width, float height, const CStdString& url);
  virtual ~CGUIWebControl(void);
  virtual CGUIWebControl *Clone() const { return new CGUIWebControl(*this); };

  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);

  virtual CStdString GetDescription() const;

  const CStdString &GetURL() const;
  void SetURL(const CStdString& url);

  void LoadUrl(const CStdString& url = "");

  virtual void OnPlayBackEnded(bool bError = false, const CStdString& error = "");
  virtual void OnPlayBackStarted() {}
  virtual void OnPlayBackStopped() {}
  virtual void OnQueueNextItem() {}
  virtual void OnPlayBackPaused() {}
  virtual void OnPlayBackResumed() {}
  virtual void OnPlayBackSeek(int iTime, int seekOffset) {}
  virtual void OnPlayBackSeekChapter(int iChapter) {}
  virtual void OnPlayBackSpeedChanged(int iSpeed) {}

  virtual bool OnMouseOver(const CPoint &point);

  bool isLoading () { return m_browser?m_browser->IsCaching():false; }

protected:

  CGUIInfoLabel m_info;

  CStdString m_url;
  CRect m_rect;

  CFlashVideoPlayer *m_browser;
  bool m_ended;
};
