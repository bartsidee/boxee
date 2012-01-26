
#include "GUIWindowBoxeeMediaSourceInfo.h"
#include "GUIWindowBoxeeMediaSources.h"
#include "GUIWindowBoxeeMediaSourceList.h"
#include "Application.h"
#include "BoxeeMediaSourceList.h"
#include "GUIWindowManager.h"
#include "GUILabelControl.h"
#include "GUIRadioButtonControl.h"
#include "GUIButtonControl.h"
#include "GUIDialogYesNo2.h"
#include "GUIDialogOK2.h"
#include "GUIDialogKeyboard.h"
#include "GUIEditControl.h"
#include "URL.h"
#include "Util.h"
#include "AppManager.h"
#include "GUIWindowBoxeeMediaSourceAddShare.h"
#include "bxfeedreader.h"
#include "bxxmldocument.h"
#include "bxfeedfactory.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "utils/log.h"
#include "SpecialProtocol.h"
#include "lib/libBoxee/boxee.h"
#include "BoxeeUtils.h"
#ifdef HAS_EMBEDDED
#include "HalServices.h"
#endif

#define CONTROL_NAME_EDIT             51
#define CONTROL_VIDEO_BUTTON          52
#define CONTROL_MUSIC_BUTTON          53
#define CONTROL_PICTURE_BUTTON        54
#define CONTROL_REMOVE_BUTTON         62
#define CONTROL_ADD_UPDATE_BUTTON     57
#define CONTROL_LOCATION_LABEL        58
#define CONTROL_LOCATION_EDIT         59
#define CONTROL_SOURCE_TYPES_GROUP    60
#define CONTROL_SCAN_BUTTON           61
#define CONTROL_SCAN_TYPES_GROUP      80
#define CONTROL_SCAN_PRIVATE_BUTTON   81
#define CONTROL_SCAN_ONCE_BUTTON      82
#define CONTROL_SCAN_DAILY_BUTTON     83
#define CONTROL_SCAN_HOURLY_BUTTON    84
#define CONTROL_SCAN_MONITORED_BUTTON 85

#define CONTROL_MANUALLY_ADD_SOURCES 54

CGUIWindowBoxeeMediaSourceInfo::CGUIWindowBoxeeMediaSourceInfo(void)
: CGUIDialog(WINDOW_BOXEE_MEDIA_SOURCE_INFO, "boxee_media_source_info.xml")
{
  m_enableLocationEdit = false;
}

CGUIWindowBoxeeMediaSourceInfo::~CGUIWindowBoxeeMediaSourceInfo(void)
{}

bool CGUIWindowBoxeeMediaSourceInfo::OnAction(const CAction &action)
{
  if (action.id == ACTION_PREVIOUS_MENU)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceInfo::OnAction - Enter function with [action.id=ACTION_PREVIOUS_MENU] (msmk)");

    Close();

    return true;
  }
  else if (action.id == ACTION_PARENT_DIR)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceInfo::OnAction - Enter function with [action.id=ACTION_PARENT_DIR] (msmk)");

    Close();
    return true;
  }
  // don't allow any built in actions to act here.
  // this forces only navigation type actions to be performed.
  else if (action.id == ACTION_BUILT_IN_FUNCTION)
  {
    return true;  // pretend we handled it
  }

  return CGUIWindow::OnAction(action);
}

void GetRSSInfoBG::Run()
{
  m_url.Replace("rss://", "http://");

  BOXEE::BXXMLDocument document;
  if (!document.LoadFromURL(m_url))
  {
    return;
  }

  BOXEE::IBXFeedReader* reader = BOXEE::BXFeedFactory::Create(document);
  if (reader == NULL)
  {
    return;
  }

  if (!reader->Parse())
  {
    delete reader;
    return;
  }

  m_title = reader->GetChannelTitle();
  if (reader->GetChannelImageLink() != "") 
  {
    m_thumbnail = reader->GetChannelImageLink();
  }
  else if (reader->QueryChannelAttribute("media:content/media:thumbnail", "url") != "") 
  {
    m_thumbnail = (reader->QueryChannelAttribute("media:content/media:thumbnail", "url"));
  }
  else if (reader->QueryChannel("boxee:image") != "")
  {
    m_thumbnail = (reader->QueryChannel("boxee:image"));
  }
  else if (reader->QueryChannel("itunes:image") != "") 
  {
    m_thumbnail = (reader->QueryChannel("itunes:image"));
  }  
  else if (reader->QueryChannelAttribute("itunes:image", "href") != "") 
  {
    m_thumbnail = (reader->QueryChannelAttribute("itunes:image", "href"));
  }

  delete reader;

  m_bResult = true;
}

bool CGUIWindowBoxeeMediaSourceInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    DWORD senderId = message.GetSenderId();

    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceInfo::OnMessage - Enter GUI_MSG_CLICKED case (msmk)");

    if (senderId == CONTROL_REMOVE_BUTTON)
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceInfo::OnMessage - Enter GUI_MSG_CLICKED case with [SenderId=CONTROL_REMOVE_BUTTON] (msmk)");

      CGUIDialogYesNo2 *pDlgYesNo = (CGUIDialogYesNo2*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO_2);
      pDlgYesNo->SetHeading(51020);
      pDlgYesNo->SetLine(0, 51025);
      pDlgYesNo->SetChoice(0, 222);
      pDlgYesNo->SetChoice(1, 186);
      pDlgYesNo->SetDefaultChoice(0);
      pDlgYesNo->DoModal();

      if (!pDlgYesNo->IsConfirmed())
        return true;
      // first inform the scanner
      g_application.GetBoxeeFileScanner().InformRemoveShare(_P(m_editedSource.path));

      CBoxeeMediaSourceList sourceList;
      sourceList.deleteSource(m_editedSourceName);

      g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::ICON_MINUS, "", g_localizeStrings.Get(51039), 5000, KAI_GREEN_COLOR, KAI_GREEN_COLOR);

      CGUIWindowBoxeeMediaSources *pDlgSourceList = (CGUIWindowBoxeeMediaSources*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_SOURCES);
      if (pDlgSourceList)
      {
        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceInfo::OnMessage - In GUI_MSG_CLICKED case with [SenderId=CONTROL_REMOVE_BUTTON]. Going to call CGUIWindowBoxeeMediaSources::Refresh() (msmk)");

        pDlgSourceList->Refresh();
      }

      Close();

      return true;
    }
    else if (senderId == CONTROL_ADD_UPDATE_BUTTON)
    {
      bool updateScanType = false;
      bool updateMediaType = false;

      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceInfo::OnMessage - Enter GUI_MSG_CLICKED case with [SenderId=CONTROL_ADD_UPDATE_BUTTON] (msmk)");

      CBoxeeMediaSourceList sourceList;

      CBoxeeMediaSource source;
      source.name = ((CGUIEditControl*) GetControl(CONTROL_NAME_EDIT))->GetLabel2();

      if(m_enableLocationEdit)
      {
        source.path = ((CGUIEditControl*) GetControl(CONTROL_LOCATION_EDIT))->GetLabel2();
      }
      else
      {
        source.path = m_sourcePath; // Can't get the label value so we had to store it
      }

      source.isVideo = ((CGUIRadioButtonControl*) GetControl(CONTROL_VIDEO_BUTTON))->IsSelected();
      source.isMusic = ((CGUIRadioButtonControl*) GetControl(CONTROL_MUSIC_BUTTON))->IsSelected();
      source.isPicture = ((CGUIRadioButtonControl*) GetControl(CONTROL_PICTURE_BUTTON))->IsSelected();
      source.thumbPath = m_sourceThumbPath;

      updateMediaType = (source.isVideo != m_editedSource.isVideo) ||
                        (source.isMusic != m_editedSource.isMusic) ||
                        (source.isPicture != m_editedSource.isPicture);

      if ((GetControl(CONTROL_SCAN_PRIVATE_BUTTON) != NULL) && ((CGUIRadioButtonControl*) GetControl(CONTROL_SCAN_PRIVATE_BUTTON))->IsSelected())
      {
        source.scanType = CMediaSource::SCAN_TYPE_PRIVATE;
      }
      else if ((GetControl(CONTROL_SCAN_ONCE_BUTTON)  != NULL) && ((CGUIRadioButtonControl*) GetControl(CONTROL_SCAN_ONCE_BUTTON))->IsSelected())
      {
        source.scanType = CMediaSource::SCAN_TYPE_ONCE;
      }
      else if ((GetControl(CONTROL_SCAN_DAILY_BUTTON)  != NULL) && ((CGUIRadioButtonControl*) GetControl(CONTROL_SCAN_DAILY_BUTTON))->IsSelected())
      {
        source.scanType = CMediaSource::SCAN_TYPE_DAILY;
      }
      else if ((GetControl(CONTROL_SCAN_HOURLY_BUTTON)  != NULL) && ((CGUIRadioButtonControl*) GetControl(CONTROL_SCAN_HOURLY_BUTTON))->IsSelected())
      {
        source.scanType = CMediaSource::SCAN_TYPE_HOURLY;
      }
      else if ((GetControl(CONTROL_SCAN_MONITORED_BUTTON)  != NULL) && ((CGUIRadioButtonControl*) GetControl(CONTROL_SCAN_MONITORED_BUTTON))->IsSelected())
      {
        source.scanType = CMediaSource::SCAN_TYPE_MONITORED;
      }
      updateScanType = (source.scanType != m_editedSource.scanType);

      if(m_enableLocationEdit)
      {
        if (source.path == "")
        {
          CGUIDialogOK2::ShowAndGetInput(257, 51041);
          return true;
        }      
      }

      // Make sure that a source with the same name does not exist already
      if (m_editedSourceName != source.name && sourceList.sourceNameExists(source.name))
      {
        CGUIDialogOK2::ShowAndGetInput(257, 51035);
        return true;
      }

      // Make sure at least one content has been selected
      if (!source.isVideo && !source.isMusic && !source.isPicture)
      {
        CGUIDialogOK2::ShowAndGetInput(257, 51040);
        return true;
      }

      if (source.scanType !=  CMediaSource::SCAN_TYPE_PRIVATE && source.scanType !=  CMediaSource::SCAN_TYPE_ONCE &&
          source.scanType !=  CMediaSource::SCAN_TYPE_DAILY && source.scanType !=  CMediaSource::SCAN_TYPE_HOURLY &&
          source.scanType !=  CMediaSource::SCAN_TYPE_MONITORED)
      {
        CGUIDialogOK2::ShowAndGetInput(257, 51042);
        return true;
      }

      CURI url(source.path);
      if (url.GetProtocol() == "rss" || url.GetProtocol() == "http")
      {
        GetRSSInfoBG* job = new GetRSSInfoBG(source.path);
        if (CUtil::RunInBG(job,false) == JOB_SUCCEEDED )
        {
          if (source.name == "")
          {
            source.name = job->m_title;
          }

          source.thumbPath = job->m_thumbnail;
          source.path.Replace("http://", "rss://");

          delete job;
        }        
      }

      // Make sure that a source not empty
      if (source.name == "")
      {
        CGUIDialogOK2::ShowAndGetInput(257, 51036);
        return true;
      }

      // Edit source
      if (m_editedSourceName != "")
      {
        // if the media type had change we should create the new types and delete the old types
        if (updateMediaType)
        {
          CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceInfo::OnMessage - In GUI_MSG_CLICKED case with [SenderId=CONTROL_ADD_UPDATE_BUTTON]. update media Type or scan type call addOrEditSource (msmk)");
          sourceList.addOrEditSource( m_editedSource, source);
        }
        if (updateScanType)
        {
          CStdString scanStr;
          scanStr.AppendFormat("%d",source.scanType);
          sourceList.updateSource(m_editedSourceName, "scanType", scanStr);
        }
        if (m_editedSourceName != source.name)
        {
          CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceInfo::OnMessage - In GUI_MSG_CLICKED case with [SenderId=CONTROL_ADD_UPDATE_BUTTON]. Going to call CGUIWindowBoxeeMediaSourceList::updateSource() (msmk)");
          sourceList.updateSource(m_editedSourceName, "name", source.name);
        }

        g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::ICON_CHECK, "", g_localizeStrings.Get(51038), 5000 , KAI_GREEN_COLOR, KAI_GREEN_COLOR);
      }
      else
      {
        sourceList.addSource(source);

        if (source.scanType != CMediaSource::SCAN_TYPE_PRIVATE)
        {
          CFileScanner::ShowSourcesStatusKaiDialog(51037,source.name);
        }
      }

      // Install the application (if it's an application)
      if (url.GetProtocol() == "app")
      {
        CAppManager::GetInstance().InstallOrUpgradeAppIfNeeded(url.GetHostName());
      }

      CGUIWindowBoxeeMediaSourceList* pDlgSourceList = (CGUIWindowBoxeeMediaSourceList*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_SOURCE_LIST);
      if (pDlgSourceList)
      {
        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceInfo::OnMessage - In GUI_MSG_CLICKED case with [SenderId=CONTROL_ADD_UPDATE_BUTTON]. Going to call CGUIWindowBoxeeMediaSourceList::Refresh() (msmk)");
        pDlgSourceList->Refresh();
      }

      CGUIWindowBoxeeMediaSources* pDlgSourceSource = (CGUIWindowBoxeeMediaSources*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_SOURCES);
      if (pDlgSourceSource)
      {
        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceInfo::OnMessage - In GUI_MSG_CLICKED case with [SenderId=CONTROL_ADD_UPDATE_BUTTON]. Going to call CGUIWindowBoxeeMediaSources::Refresh() (msmk)");
        pDlgSourceSource->Refresh();
      }

      Close();

      return true;
    }
    else if ((senderId == CONTROL_VIDEO_BUTTON || senderId == CONTROL_MUSIC_BUTTON || senderId == CONTROL_PICTURE_BUTTON) && IsOnlyPictureSourceSelected())
    {
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_PRIVATE_BUTTON, true);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_ONCE_BUTTON, false);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_MONITORED_BUTTON, false);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_DAILY_BUTTON, false);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_HOURLY_BUTTON, false);
    }
    else if (senderId == CONTROL_SCAN_PRIVATE_BUTTON)
    {
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_PRIVATE_BUTTON, true);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_ONCE_BUTTON, false);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_MONITORED_BUTTON, false);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_DAILY_BUTTON, false);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_HOURLY_BUTTON, false);
    }
    else if (senderId == CONTROL_SCAN_ONCE_BUTTON)
    {
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_PRIVATE_BUTTON, false);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_ONCE_BUTTON, true);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_MONITORED_BUTTON, false);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_DAILY_BUTTON, false);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_HOURLY_BUTTON, false);
    }
    else if (senderId == CONTROL_SCAN_MONITORED_BUTTON)
    {
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_PRIVATE_BUTTON, false);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_ONCE_BUTTON, false);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_MONITORED_BUTTON, true);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_DAILY_BUTTON, false);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_HOURLY_BUTTON, false);
    }
    else if (senderId == CONTROL_SCAN_DAILY_BUTTON)
    {
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_PRIVATE_BUTTON, false);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_ONCE_BUTTON, false);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_MONITORED_BUTTON, false);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_DAILY_BUTTON, true);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_HOURLY_BUTTON, false);
    }
    else if (senderId == CONTROL_SCAN_HOURLY_BUTTON)
    {
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_PRIVATE_BUTTON, false);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_ONCE_BUTTON, false);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_MONITORED_BUTTON, false);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_DAILY_BUTTON, false);
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_HOURLY_BUTTON, true);
    }
    else if (senderId == CONTROL_SCAN_BUTTON)
    {
      if (BoxeeUtils::DoYouWantToScan(m_sourcePath))
      {
        g_application.GetBoxeeFileScanner().AddUserPath(_P(m_sourcePath));
        //BOXEE::Boxee::GetInstance().GetMetadataEngine().MarkFolderTreeNew(_P(m_sourcePath));

        g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::ICON_STAR, "", g_localizeStrings.Get(51043), 5000 , KAI_YELLOW_COLOR, KAI_GREY_COLOR);
      }
    }
  }
  break;
  }

  return CGUIDialog::OnMessage(message);
}

CStdString CGUIWindowBoxeeMediaSourceInfo::ManipulateSourceDisplayName()
{
  CStdString name = CUtil::GetTitleFromPath(m_sourcePath);
  CURI url(m_sourcePath);
  CStdString hostName = url.GetHostName();

  if(url.GetProtocol() == "nfs" || url.GetProtocol() == "afp")
  {
    if (!name.Equals(hostName))
    {
      name  += " on " + hostName;
    }
  }
  else if(url.GetProtocol() == "bms")
  {
    CStdString bmm = "Boxee Media Manager on";
    if(hostName.Find(bmm) != -1)
    {
      hostName = hostName.substr(bmm.size(), hostName.size()-bmm.size());
    }
    if(name.IsEmpty())
    {
      name =  hostName;
    }
    else
    {
      name += " on " +hostName;
    }
  }
  else if(url.GetProtocol() == "upnp")
  {
    if(name.IsEmpty())
    {
      name =  hostName;
    }
    else
    {
      name += " on " +hostName;
    }
    CUtil::UrlDecode(name);
  }
  else if (url.GetProtocol() == "smb")
  {
    if(hostName.Equals("computers"))
    {
      CStdString strComp;
      CStdString strDevices;
      std::map<std::string, std::string> mapParams;

      if (BoxeeUtils::ParseBoxeeDbUrl(m_sourcePath, strComp, strDevices, mapParams))
      {
        if(!mapParams["name"].empty())
        {
          m_sourcePath = "smb://" + mapParams["name"];
          hostName = mapParams["name"];
        }
        else
        {
          hostName = g_localizeStrings.Get(13205);
        }
      }
    }
    if(name.Find("(SMB)") != -1)
    {
      name = hostName;
    }
    else
    {
      name += " on " + hostName;
    }
  }

  return name;
}

void CGUIWindowBoxeeMediaSourceInfo::OnInitWindow()
{
  CGUIWindow::OnInitWindow();
  CBoxeeMediaSourceList sourceList;

  m_editedSource = BOXEE_NULL_SOURCE;

  if (m_editedSourceName != "")
  {
    SET_CONTROL_VISIBLE(CONTROL_REMOVE_BUTTON);
    SET_CONTROL_LABEL(CONTROL_ADD_UPDATE_BUTTON, "[UPPERCASE][B]" + g_localizeStrings.Get(54200) + "[/B][/UPPERCASE]");

    m_editedSource = sourceList.getBySourceName(m_editedSourceName);

    SET_CONTROL_LABEL2(CONTROL_NAME_EDIT, m_editedSource.name);

    SET_CONTROL_SELECTED(GetID(), CONTROL_VIDEO_BUTTON, m_editedSource.isVideo);
    SET_CONTROL_SELECTED(GetID(), CONTROL_MUSIC_BUTTON, m_editedSource.isMusic);
    SET_CONTROL_SELECTED(GetID(), CONTROL_PICTURE_BUTTON, m_editedSource.isPicture);

    if (m_editedSource.scanType == CMediaSource::SCAN_TYPE_PRIVATE)
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_PRIVATE_BUTTON, true);
    else if ( m_editedSource.scanType == CMediaSource::SCAN_TYPE_ONCE)
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_ONCE_BUTTON, true);
    else if ( m_editedSource.scanType == CMediaSource::SCAN_TYPE_DAILY)
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_DAILY_BUTTON, true);
    else if ( m_editedSource.scanType == CMediaSource::SCAN_TYPE_HOURLY)
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_HOURLY_BUTTON, true);
    else if ( m_editedSource.scanType == CMediaSource::SCAN_TYPE_MONITORED)
#ifndef HAS_EMBEDDED
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_MONITORED_BUTTON, true);
#else
      SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_HOURLY_BUTTON, true);
#endif

    SET_CONTROL_VISIBLE(CONTROL_LOCATION_EDIT);
    SET_CONTROL_LABEL2(CONTROL_LOCATION_EDIT, shortenURL(m_editedSource.path));

    SET_CONTROL_VISIBLE(CONTROL_SCAN_BUTTON);
    // Save the path so we can later access it (can't read label value from the control)
    m_sourcePath = m_editedSource.path;
  }
  else
  {
    SET_CONTROL_HIDDEN(CONTROL_REMOVE_BUTTON);
    SET_CONTROL_HIDDEN(CONTROL_SCAN_BUTTON);
    SET_CONTROL_LABEL(CONTROL_ADD_UPDATE_BUTTON, "[UPPERCASE][B]" + g_localizeStrings.Get(54201) + "[/B][/UPPERCASE]");

    SET_CONTROL_SELECTED(GetID(), CONTROL_VIDEO_BUTTON, false);
    SET_CONTROL_SELECTED(GetID(), CONTROL_MUSIC_BUTTON, false);
    SET_CONTROL_SELECTED(GetID(), CONTROL_PICTURE_BUTTON, false);
    SET_CONTROL_SELECTED(GetID(), CONTROL_SCAN_DAILY_BUTTON, true);

    CStdString name = ManipulateSourceDisplayName();
    SET_CONTROL_LABEL2(CONTROL_NAME_EDIT, name);
  }

  if (m_enableLocationEdit)
  {
    SET_CONTROL_HIDDEN(CONTROL_LOCATION_LABEL);
    SET_CONTROL_VISIBLE(CONTROL_LOCATION_EDIT);
    SET_CONTROL_LABEL2(CONTROL_LOCATION_EDIT, m_sourcePath);
  }
  else
  {
    CStdString pathWithOutPassword = m_sourcePath;
    CUtil::RemovePasswordFromPath(pathWithOutPassword);
    SET_CONTROL_LABEL(CONTROL_LOCATION_LABEL, shortenURL(_P(pathWithOutPassword)));
    SET_CONTROL_HIDDEN(CONTROL_LOCATION_EDIT);
    SET_CONTROL_VISIBLE(CONTROL_LOCATION_LABEL);
  }

  CGUIWindowBoxeeMediaSources *pDlgSources = (CGUIWindowBoxeeMediaSources*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_SOURCES);
  if (!pDlgSources)
  {
    return;
  }

  SET_CONTROL_FOCUS(CONTROL_NAME_EDIT, 0);
}

void CGUIWindowBoxeeMediaSourceInfo::OnDeinitWindow(int nextWindowID)
{
  m_enableLocationEdit = false;
  m_sourceThumbPath = "";
  CGUIDialog::OnDeinitWindow(nextWindowID);
}

void CGUIWindowBoxeeMediaSourceInfo::SetEditedSource(CStdString sourceName)
{
  m_editedSourceName = sourceName;
}

void CGUIWindowBoxeeMediaSourceInfo::SetAddSource(CStdString sourcePath)
{
  m_sourcePath = sourcePath;
  m_editedSourceName = "";
}

void CGUIWindowBoxeeMediaSourceInfo::SetSourceThumbPath(CStdString thumbPath)
{
  m_sourceThumbPath = thumbPath;
}

CStdString CGUIWindowBoxeeMediaSourceInfo::shortenURL(CStdString path)
{
  CStdString result = path;

  if (CUtil::IsHD(result))
    CUtil::HideExternalHDPath(result, result);

  CUtil::UrlDecode(result);

  // remove password
  CUtil::RemovePasswordFromPath(result);

  CURI url(result);
  result = url.GetWithoutUserDetails();

  return result;
}

void CGUIWindowBoxeeMediaSourceInfo::SetEnableLocationEdit(bool enableLocationEdit)
{
  m_enableLocationEdit = enableLocationEdit;
}

bool CGUIWindowBoxeeMediaSourceInfo::IsOnlyPictureSourceSelected()
{
  CGUIRadioButtonControl* videoButton = (CGUIRadioButtonControl*)GetControl(CONTROL_VIDEO_BUTTON);
  if (videoButton && videoButton->IsSelected())
  {
    return false;
  }

  CGUIRadioButtonControl* musicButton = (CGUIRadioButtonControl*)GetControl(CONTROL_MUSIC_BUTTON);
  if (musicButton && musicButton->IsSelected())
  {
    return false;
  }

  CGUIRadioButtonControl* pictureButton = (CGUIRadioButtonControl*)GetControl(CONTROL_PICTURE_BUTTON);
  if (pictureButton && pictureButton->IsSelected())
  {
    return true;
  }

  return false;
}
