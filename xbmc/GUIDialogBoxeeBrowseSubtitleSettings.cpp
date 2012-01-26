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
#include "GUIDialogBoxeeBrowseSubtitleSettings.h"
#include "GUIDialogAudioSubtitleSettings.h"
#include "GUIDialogBoxeeBrowseLocalSubtitleSettings.h"
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
#include "ItemLoader.h"

using namespace std;
using namespace XFILE;
using namespace DIRECTORY;

CGUIDialogBoxeeBrowseSubtitleSettings::CGUIDialogBoxeeBrowseSubtitleSettings(void)
    : CGUIDialog(WINDOW_DIALOG_BOXEE_BROWSE_SUBTITLES_SETTINGS, "boxee_browse_subtitles_settings.xml")
{
}

CGUIDialogBoxeeBrowseSubtitleSettings::~CGUIDialogBoxeeBrowseSubtitleSettings(void)
{
}

#define BROWSE_LOCAL_BUTTON             51
#define BROWSE_ONLINE_BUTTON            52
#define BROWSE_ONLINE_LIST              5000

void CGUIDialogBoxeeBrowseSubtitleSettings::OnInitWindow()
{
  CFileItem item = g_application.CurrentFileItem();

  if ((item.IsHD() || item.IsSmb()) && !item.m_bIsFolder && !item.m_bIsShareOrDrive &&
      !item.m_strPath.IsEmpty() && !item.GetPropertyBOOL("MetaDataExtracted") &&
      g_application.IsPathAvailable(item.m_strPath), false)
  {
    g_application.GetItemLoader().LoadFileMetadata(GetID(), BROWSE_ONLINE_LIST, &item);
  }
  else
  {
    CFileItemPtr pItem (new CFileItem(item));
    CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), BROWSE_ONLINE_LIST, 0);
    OnMessage(msg);
    CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), BROWSE_ONLINE_LIST, 0, 0, pItem);
    OnMessage(winmsg);
  }
  CGUIDialog::OnInitWindow();
}

bool CGUIDialogBoxeeBrowseSubtitleSettings::OnMessage(CGUIMessage& message)
{
  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogBoxeeBrowseSubtitleSettings::OnAction(const CAction& action)
{
  int iControl = GetFocusedControlID();

  if (action.id == ACTION_SELECT_ITEM)
  {
    if(iControl == BROWSE_LOCAL_BUTTON)
    {
      Close();
      OnBrowseLocal();
      return true;
    }
  }
  if(action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    CGUIDialogAudioSubtitleSettings* pDlg = (CGUIDialogAudioSubtitleSettings*)g_windowManager.GetWindow(WINDOW_DIALOG_AUDIO_OSD_SETTINGS);
    if(pDlg)
    {
      Close();
      pDlg->DoModal();
    }
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeeBrowseSubtitleSettings::OnBrowseLocal()
{
  CStdString strPath;// = "sources://all/";

  if (CUtil::IsInRAR(g_application.CurrentFileItem().m_strPath) || CUtil::IsInZIP(g_application.CurrentFileItem().m_strPath))
  {
    CURI url(g_application.CurrentFileItem().m_strPath);
    strPath = url.GetHostName();
  }
  else
  {
    strPath = g_application.CurrentFileItem().m_strPath;
  }

  CStdString strMask = ".utf|.utf8|.utf-8|.sub|.srt|.smi|.rt|.txt|.ssa|.aqt|.jss|.ass|.idx|.rar|.zip";
  if (g_application.GetCurrentPlayer() == EPC_DVDPLAYER)
    strMask = ".srt|.rar|.zip|.ifo|.smi|.sub|.idx|.ass|.ssa|.txt";

  VECSOURCES locations;
  CFileItemList sourceList;
  DIRECTORY::CDirectory::GetDirectory("sources://all/",sourceList);

  CFileItemPtr pNetworkItem(new CFileItem("Network"));
  pNetworkItem->m_strPath = "network://protocols";
  sourceList.Add(pNetworkItem);

  for (int i = 0 ; i < sourceList.Size() ; i++)
  {
    CMediaSource share;
    CURI url(sourceList[i]->m_strPath);

    share.strName = sourceList[i]->GetLabel();
    share.strPath = sourceList[i]->m_strPath;

#ifdef HAS_EMBEDDED
    if (share.strPath.IsEmpty() || share.strPath == "boxeedb://unresolvedVideoFiles")
    {
      continue;
    }
#endif

    locations.push_back(share);
  }
  if (CGUIDialogBoxeeBrowseLocalSubtitleSettings::ShowAndGetFile(locations,strMask,g_localizeStrings.Get(293),strPath,false,true,false))
  {
    int subtitleStream;
    CStdString strExt;
    CUtil::GetExtension(strPath,strExt);
    if (strExt.CompareNoCase(".idx") == 0 || strExt.CompareNoCase(".sub") == 0)
    {
      // Playback could end and delete m_pPlayer while dialog is up so make sure it's valid
      if (g_application.m_pPlayer)
      {
        if (CFile::Cache(strPath,"special://temp/subtitle"+strExt))
        {
          CStdString strPath2;
          CStdString strPath3;
          if (strExt.CompareNoCase(".idx") == 0)
          {
            CUtil::ReplaceExtension(strPath,".sub",strPath2);
            strPath3 = "special://temp/subtitle.sub";
          }
          else
          {
            CUtil::ReplaceExtension(strPath,".idx",strPath2);
            if (!CFile::Exists(strPath2) && (CUtil::IsInRAR(strPath2) || CUtil::IsInZIP(strPath2)))
            {
              CStdString strFileName = CUtil::GetFileName(strPath);
              CUtil::GetDirectory(strPath,strPath3);
              CUtil::GetParentPath(strPath3,strPath2);
              CUtil::AddFileToFolder(strPath2,strFileName,strPath2);
              CUtil::ReplaceExtension(strPath2,".idx",strPath2);
            }
            strPath3 = "special://temp/subtitle.idx";
          }
          if (CFile::Exists(strPath2))
            CFile::Cache(strPath2,strPath3);
          else
          {
            CFileItemList items;
            CStdString strDir,strFileNameNoExtNoCase;
            CUtil::Split(strPath,strDir,strPath3);
            CUtil::ReplaceExtension(strPath3,".",strFileNameNoExtNoCase);
            strFileNameNoExtNoCase.ToLower();
            CUtil::GetDirectory(strPath,strDir);
            CDirectory::GetDirectory(strDir,items,".rar|.zip",false);
            vector<CStdString> vecExts;
            for (int i=0;i<items.Size();++i)
              CUtil::CacheRarSubtitles(vecExts,items[i]->m_strPath,strFileNameNoExtNoCase,"");
          }
          g_stSettings.m_currentVideoSettings.m_SubtitleOn = true;

          if(g_application.m_pPlayer->AddSubtitle("special://temp/subtitle.idx"))
          {
            subtitleStream = g_application.m_pPlayer->GetSubtitleCount() - 1;
            g_application.m_pPlayer->SetSubtitle(subtitleStream);
            g_application.m_pPlayer->SetSubtitleVisible(true);
          }

          Close();
        }
      }
    }
    else
    {
      subtitleStream = g_application.m_pPlayer->GetSubtitleCount();
      CStdString strExt;
      CUtil::GetExtension(strPath,strExt);
      if (CFile::Cache(strPath,"special://temp/subtitle.browsed"+strExt))
      {
        g_stSettings.m_currentVideoSettings.m_SubtitleOn = true;
        g_application.m_pPlayer->SetSubtitleVisible(true);
        g_application.m_pPlayer->AddSubtitle("special://temp/subtitle.browsed"+strExt);
        g_application.m_pPlayer->SetSubtitle(subtitleStream);
      }
    }
    g_stSettings.m_currentVideoSettings.m_SubtitleCached = true;
    return true;
  }

  return false;
}
