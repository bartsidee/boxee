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

#include "GUIDialog.h"

#include <queue>

#define TOAST_DISPLAY_TIME   5000L  // default 5 seconds
#define TOAST_MESSAGE_TIME   1000L  // minimal message time 1 second

#define DEFAULT_BGCOLOR  "FFFFFFFF"
#define DEFAULT_ICON    "kaidialog/kai-star.png"

#define KAI_GREEN_COLOR   "FF8CC641"
#define KAI_RED_COLOR     "FFDA534A"
#define KAI_ORANGE_COLOR  "FFF9A41A"
#define KAI_GREY_COLOR    "FF6A7279"
#define KAI_YELLOW_COLOR  "FFEADA17"


class CGUIDialogKaiToast: public CGUIDialog
{
public:
  CGUIDialogKaiToast(void);
  virtual ~CGUIDialogKaiToast(void);

  enum PopupIconEnums
  {
    ICON_CHECK,
    ICON_EXCLAMATION,
    ICON_SEARCH,
    ICON_ARROWDOWN,
    ICON_ARROWUP,
    ICON_MINUS,
    ICON_PLUS,
    ICON_STAR,
    ICON_SCAN,
    ICON_HEART
  };

  struct Notification
  {
    CStdString caption;
    CStdString description;
    CStdString imagefile;
    CStdString bgColor;
    CStdString iconColor;
    unsigned int displayTime;
  };

  void QueueNotification(const CStdString& aCaption, const CStdString& aDescription);
  void QueueNotification(const CStdString& aImageFile, const CStdString& aCaption, const CStdString& aDescription, unsigned int displayTime = TOAST_DISPLAY_TIME, const CStdString& iconColor = "", const CStdString& bgColor = "");
  void QueueNotification(const CGUIDialogKaiToast::PopupIconEnums& icon , const CStdString& aCaption, const CStdString& aDescription, unsigned int displayTime = TOAST_DISPLAY_TIME, const CStdString& iconColor = "", const CStdString& bgColor = "");
  bool DoWork();

  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnWindowLoaded();
  virtual void Render();
  void ResetTimer();

protected:

  virtual void OnInitWindow();

  bool IsValid(const CStdString& aImageFile, const CStdString& aCaption, const CStdString& aDescription, unsigned int displayTime /*= TOAST_DISPLAY_TIME*/);

  unsigned int m_timer;

  unsigned int m_toastDisplayTime;

  typedef std::queue<Notification> TOASTQUEUE;
  TOASTQUEUE m_notifications;
  CCriticalSection m_critical;

private:
  std::map<PopupIconEnums, CStdString> m_mapIconToImage;
};
