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

#include "system.h"
#include "GUIDialogBoxeeBrowseLocalSubtitleSettings.h"
#include "GUIDialogBoxeeBrowseSubtitleSettings.h"
#include "GUIDialogFileBrowser.h"
#include "GUIPassword.h"
#include "Util.h"
#include "Application.h"
#include "VideoDatabase.h"
#include "XBAudioConfig.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogSelect.h"
#include "GUIWindowManager.h"
#include "GUILabelControl.h"
#include "FileSystem/Directory.h"
#include "FileSystem/File.h"
#include "URL.h"
#include "FileItem.h"
#include "SkinInfo.h"
#include "Settings.h"
#include "AdvancedSettings.h"
#include "GUISettings.h"
#include "LocalizeStrings.h"
#include "LangCodeExpander.h"

using namespace std;
using namespace XFILE;
using namespace DIRECTORY;

#define SOURCES_CONTROL_LIST    450
#define CURRENT_LOCATION_LABEL  412

CGUIDialogBoxeeBrowseLocalSubtitleSettings::CGUIDialogBoxeeBrowseLocalSubtitleSettings(void)
    : CGUIDialogFileBrowser(WINDOW_DIALOG_BOXEE_BROWSE_LOCAL_SUBTITLES_SETTINGS, "boxee_osd_browse_local_subtitles_settings.xml")
{
}

CGUIDialogBoxeeBrowseLocalSubtitleSettings::~CGUIDialogBoxeeBrowseLocalSubtitleSettings(void)
{
}

bool CGUIDialogBoxeeBrowseLocalSubtitleSettings::ShowAndGetFile(const VECSOURCES &shares, const CStdString &mask, const CStdString &heading, CStdString &path, bool useThumbs /* = false */, bool useFileDirectories /* = false */, bool allowNonLocalSources /* = true */)
{
  CGUIDialogBoxeeBrowseLocalSubtitleSettings *browser = new CGUIDialogBoxeeBrowseLocalSubtitleSettings();
  if (!browser)
    return false;
  g_windowManager.AddUniqueInstance(browser);

  browser->m_useFileDirectories = useFileDirectories;

  browser->m_browsingForImages = useThumbs;
  browser->SetHeading(heading);
  browser->SetSources(shares);
  CStdString strMask = mask;
  if (mask == "/")
    browser->m_browsingForFolders=1;
  else
  if (mask == "/w")
  {
    browser->m_browsingForFolders=2;
    strMask = "/";
  }
  else
    browser->m_browsingForFolders = 0;

  browser->m_rootDir.SetMask(strMask);
  browser->m_selectedPath = path;
  browser->m_addNetworkShareEnabled = false;
  browser->m_rootDir.AllowNonLocalSources(allowNonLocalSources);
  browser->DoModal();
  bool confirmed(browser->IsConfirmed());
  if (confirmed)
    path = browser->m_selectedPath;
  g_windowManager.Remove(browser->GetID());
  delete browser;
  return confirmed;
}


bool CGUIDialogBoxeeBrowseLocalSubtitleSettings::OnMessage(CGUIMessage& message)
{
  return CGUIDialogFileBrowser::OnMessage(message);
}

bool CGUIDialogBoxeeBrowseLocalSubtitleSettings::OnAction(const CAction& action)
{
  if(action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    CGUIDialogBoxeeBrowseSubtitleSettings* pDlg = (CGUIDialogBoxeeBrowseSubtitleSettings*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_BROWSE_SUBTITLES_SETTINGS);
    if(pDlg)
    {
      Close();
      pDlg->DoModal();
    }
  }
  return CGUIDialogFileBrowser::OnAction(action);
}
