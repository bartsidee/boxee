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
 

#include "FlashVideoPlayer.h"
#include "GUIFontManager.h"
#include "GUITextLayout.h"
#include "Application.h"
#include "Settings.h"
#include "VideoRenderers/RenderManager.h"
#include "VideoRenderers/RenderFlags.h"
#include "URL.h"
#include "Util.h"
#include "VideoInfoTag.h"
#include "utils/SystemInfo.h"
#include "AppManager.h"
#include "FileSystem/SpecialProtocol.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "GUISettings.h"
#include "utils/TimeUtils.h"
#include "../../lib/libBoxee/bxutils.h"
#include "../../lib/libBoxee/bxcurl.h"
#include "GUIDialogKeyboard.h"
#include "GUIWindowManager.h"
#include "utils/SingleLock.h"
#include "GUIDialogProgress.h"

#ifdef _LINUX
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#include <vector>
#include <set>

#define HEIGHT 480
#define WIDTH 640

CFlashVideoPlayer::CFlashVideoPlayer(IPlayerCallback& callback)
    : IPlayer(callback),
      CThread()
{
  m_paused = false;
  m_playing = false;
  m_clock = 0;
  m_lastTime = 0;
  m_speed = 1;
  m_handle = NULL;
  m_height = HEIGHT;
  m_width = WIDTH;
  m_cropTop = 0;
  m_cropBottom = 0;
  m_cropLeft = 0;
  m_cropRight = 0;
  m_isFullscreen = true;
  m_totalTime = 0;
  m_pDlgCache = NULL;
  
  m_canPause = false;
  m_canSkip = false;
  m_canSetVolume = false;
  
  m_userStopped = true;
  
  m_bBrowserMode  = false;
  m_bShowKeyboard = false;
  m_bCloseKeyboard = false;
  
  m_bConfigChanged = false;
  m_bImageLocked = false;
  m_bDirty = false;

#ifdef HAS_XCOMPOSITE
  setenv("BOXEE_USE_XCOMPOSITE", "1", 1);
#endif
}

CFlashVideoPlayer::~CFlashVideoPlayer()
{
  CloseFile();
}

void CFlashVideoPlayer::FlashSetMode(FlashLibMode nMode)
{
  if (nMode == FLASHLIB_MODE_BROWSER)
    m_bBrowserMode = true;
  else
    m_bBrowserMode = false;
}

void CFlashVideoPlayer::OSDExtensionClicked(int nId)
{
  if (m_handle)
    m_dll.FlashActivateExt(m_handle, nId);
}

bool CFlashVideoPlayer::OnAction(const CAction &action)
{
  if (m_bCloseKeyboard)
  {
    CGUIDialogKeyboard *dlg = (CGUIDialogKeyboard *)g_windowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);
    if (dlg) dlg->Close();
    m_bCloseKeyboard = false;
  }
  
  if (!m_bBrowserMode)
    return false;
  
  if (action.id == ACTION_MOUSE)
  { 
    return true;
  }
  else if (action.id == ACTION_BACKSPACE)
  { 
    m_dll.FlashPause(m_handle); 
    return true;
  }
  else if (action.id == ACTION_SHOW_OSD)
  { 
    return true;
  }
  else if (action.id == ACTION_SHOW_INFO)
  { 
    m_dll.FlashPlay(m_handle); 
    return true;
  }
  else if (action.id == ACTION_STEP_BACK)
  { 
    m_dll.FlashSmallStep(m_handle, true); // emulate seek (left)
    return true;
  }
  else if (action.id == ACTION_STEP_FORWARD)
  { 
    m_dll.FlashSmallStep(m_handle, false); // emulate seek (right)
    return true;
  }
  else if (action.id == ACTION_VOLUME_UP)
  { 
    m_dll.FlashBigStep(m_handle, false); // emulate seek (up)
    return true;
  }
  else if (action.id == ACTION_VOLUME_DOWN)
  { 
    m_dll.FlashBigStep(m_handle, true); // emulate seek (down)
    return true;
  }
  
  return false;
}

void CFlashVideoPlayer::ParseFlashURL(const CStdString &strUrl, std::vector<CStdString> &vars, std::vector<CStdString> &values)
{
  CURL url(strUrl);
  CLog::Log(LOGDEBUG,"%s - parsing <%s>", __FUNCTION__, strUrl.c_str());
    
  std::set<CStdString> keys;
  if (url.GetProtocol().Equals("flash", false))
  {
    CStdString strFileName = url.GetFileName();
    if (strFileName.size() && strFileName[0] == '?')
      strFileName = strFileName.Mid(1);
    
    std::vector<CStdString> params;
    CUtil::Tokenize(strFileName, params, "&");
    
    for (size_t nParam = 0; nParam < params.size(); nParam++)
    {
      CStdString strParam = params[nParam];
      CStdString strKey,strVal;
      int nPos = strParam.Find("=");
      if (nPos > 0)
      {
        strKey = strParam.Mid(0, nPos);
        strVal = strParam.Mid(nPos+1);
      }
      else 
      {
        strKey = "dummy";
        strVal = strParam;
      }
      
      if (strKey == "src")
      {
        vars.push_back(strKey);
        CStdString strToPush = strVal; 
        values.push_back(strToPush);       
      }
      else
      {
        if (strKey == "bx-croptop")
          m_cropTop = atoi(strVal.c_str());
        if (strKey == "bx-cropbottom")
          m_cropBottom = atoi(strVal.c_str());
        if (strKey == "bx-cropleft")
          m_cropLeft = atoi(strVal.c_str());
        if (strKey == "bx-cropright")
          m_cropRight = atoi(strVal.c_str());
        vars.push_back(strKey);
        values.push_back(strVal);        
      }
      
      keys.insert(strKey);
    }
  }

  if (keys.find("src") == keys.end() && keys.find("source") == keys.end())
  {
    vars.push_back("src");
    values.push_back(strUrl.c_str());    
  }
}

bool CFlashVideoPlayer::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  if (!m_dll.Load())
  {
      CLog::Log(LOGERROR,"failed to load flashlib");
      return false;
  }

  if (m_pDlgCache)
    m_pDlgCache->Close();

  m_bShowKeyboard = false;

  CGUIDialogProgress *dlg = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (dlg && dlg->IsCanceled())
  {
    CloseFile();
    return false;
  }

  m_pDlgCache = new CDlgCache(0, g_localizeStrings.Get(10214), file.GetLabel());
  
  std::vector<CStdString> params;
  std::vector<CStdString> values;
  
  m_path = file.m_strPath;
  
  //
  // if we are getting an app:// link - we let the app manager launch it.
  // at this point - the player's state is "loading" with progress indication so whatever the app manager does it needs
  // to update the player (probably via python callback)
  //
  if (file.IsApp() && !file.HasProperty("ActualPlaybackPath"))
  {
    CLog::Log(LOGDEBUG,"%s - launching app (path= <%s>)", __FUNCTION__, file.m_strPath.c_str());
    g_application.CurrentFileItem().SetProperty("WaitPlaybackPath", true);
    ThreadMessage tMsg = {TMSG_LAUNCH_APP};
    tMsg.strParam = file.m_strPath;
    g_application.getApplicationMessenger().SendMessage(tMsg, false);  
    return true;
  }
  
  g_application.CurrentFileItem().SetProperty("WaitPlaybackPath", false);
  
  if (file.IsApp() && file.GetPropertyBOOL("AddToHistory"))
  {
    CFileItem copy = file;
    copy.ClearProperty("actualplaybackpath"); // this is a temp path and should not be set in order for history playback to work correctly (go through the whole playback procedure)
    g_application.GetBoxeeItemsHistoryList().AddItemToHistory(copy);
  }

  CStdString strActualPath = file.m_strPath;
  if (file.HasProperty("ActualPlaybackPath"))
    strActualPath = file.GetProperty("ActualPlaybackPath");
  
  CLog::Log(LOGINFO,"about to play url %s", strActualPath.c_str());
  
  ParseFlashURL(strActualPath, params, values);
  params.push_back("bx-title");
  values.push_back(file.GetLabel().c_str());
  params.push_back("bx-lang");
  values.push_back(g_guiSettings.GetString("locale.language"));
  if (file.HasVideoInfoTag())
  {
    CStdString strShow = file.GetVideoInfoTag()->m_strShowTitle;
    if (strShow.IsEmpty())
      strShow = file.GetVideoInfoTag()->m_strTitle;
    int nSeason = file.GetVideoInfoTag()->m_iSeason;
    int nEpisode = file.GetVideoInfoTag()->m_iEpisode;
    CStdString strSeason, strEpisode;
    strSeason.Format("%d",nSeason);
    strEpisode.Format("%d",nEpisode);
    
    params.push_back("bx-show");
    values.push_back(strShow);
    params.push_back("bx-season");
    values.push_back(strSeason);
    params.push_back("bx-episode");
    values.push_back(strEpisode);
    params.push_back("bx-plot");
    values.push_back(file.GetVideoInfoTag()->m_strPlot);
    params.push_back("bx-plot2");
    values.push_back(file.GetVideoInfoTag()->m_strPlotOutline);
    params.push_back("bx-studio");
    values.push_back(file.GetVideoInfoTag()->m_strStudio);
    params.push_back("bx-cookiejar");
    values.push_back(BOXEE::BXCurl::GetCookieJar());    
  }
  
  params.push_back("bx-thumb");
  values.push_back(_P(file.GetThumbnailImage()));
  char **argn = new char* [params.size()];
  char **argv = new char* [params.size()];
  
  for (size_t i=0; i<params.size(); i++)
  {
    argn[i] = strdup(params[i].c_str());

    if (values[i].size() > 4096)
      values[i] = values[i].substr(0,4096);

    argv[i] = strdup(values[i].c_str());    

    if (strcasecmp(argn[i], "height") == 0)
    {
      m_height = atoi(argv[i]);
    }
    
    if (strcasecmp(argn[i], "width") == 0)
    {
      m_width = atoi(argv[i]);
    }
  }

  m_isFullscreen = options.fullscreen;

  m_bConfigChanged = false;

  // setup cache folder for persistent browser info
#ifdef _LINUX
  setenv("BOXEE_CACHE_FOLDER",_P("special://profile/browser").c_str(),1);
#else
  SetEnvironmentVariable("BOXEE_CACHE_FOLDER",_P("special://profile/browser").c_str());
#endif
  
  m_handle = m_dll.FlashCreate(m_width*4);
  m_dll.FlashSetWorkingPath(m_handle, PTH_IC("special://xbmc/system/players/flashplayer"));
  m_dll.FlashSetCrop(m_handle, m_cropTop, m_cropBottom, m_cropLeft, m_cropRight);
  m_dll.FlashSetCallback(m_handle, this);
  if (!m_dll.FlashOpen(m_handle, params.size(), (const char **)argn, (const char **)argv))
  {
    m_dll.FlashClose(m_handle);
    m_handle = NULL;
  }
  
  for (size_t p=0;p<params.size();p++)
  {
    free(argn[p]);    
    free(argv[p]);    
  }

  delete [] argn;
  delete [] argv;

  if (!m_handle)
  {
    if (m_pDlgCache)
      m_pDlgCache->Close();
    m_pDlgCache = NULL;
    return false;
  }
  
  if (file.HasVideoInfoTag())
  {
    std::vector<CStdString> tokens;
    CUtil::Tokenize(file.GetVideoInfoTag()->m_strRuntime, tokens, ":");
    int nHours = 0;
    int nMinutes = 0;
    int nSeconds = 0;
    
    if (tokens.size() == 2)
    {
      nMinutes = atoi(tokens[0].c_str());
      nSeconds = atoi(tokens[1].c_str());
    }
    else if (tokens.size() == 3)
    {
      nHours = atoi(tokens[0].c_str());
      nMinutes = atoi(tokens[1].c_str());
      nSeconds = atoi(tokens[2].c_str());
    }
    m_totalTime = (nHours * 3600) + (nMinutes * 60) + nSeconds;
  }

  Create();
  if( options.starttime > 0 )
    SeekTime( (__int64)(options.starttime * 1000) );

  return true;
}

bool CFlashVideoPlayer::CloseFile()
{
  int locks = ExitCriticalSection(g_graphicsContext);

  m_bStop = true;
  if (m_bCloseKeyboard)
  {
    CGUIDialogKeyboard *dlg = (CGUIDialogKeyboard *)g_windowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);
    if (dlg) dlg->Close();
    m_bCloseKeyboard = false;
  }
  
  CSingleLock lock(m_lock);  
  if (m_bImageLocked)
  {
    m_dll.FlashUnlockImage(m_handle);
    m_bImageLocked = false;
  }  
  lock.Leave();
  
  StopThread();
  if (locks > 0)
    RestoreCriticalSection(g_graphicsContext, locks);
  
  if (m_handle)
    m_dll.FlashClose(m_handle);
  m_handle = NULL;

  g_renderManager.UnInit();
  
  if (m_pDlgCache)
    m_pDlgCache->Close();
  m_pDlgCache = NULL;
  
  return true;
}

void CFlashVideoPlayer::CheckConfig()
{
  CSingleLock lock(m_lock);  
  if (m_bConfigChanged)
  {
    if (m_pDlgCache)
      m_pDlgCache->Close();
    m_pDlgCache = NULL;
    
    if (!m_playing)
    {      
      g_renderManager.PreInit();
      m_playing = true;
    }
    
    unsigned flags = CONF_FLAGS_EXTERN_IMAGE;

    if (m_isFullscreen)
    {
      flags |= CONF_FLAGS_FULLSCREEN;
    }

    g_renderManager.Configure(m_width, m_height, m_width, m_height, 30.0f, flags);

    m_bConfigChanged = false;
  }
}

bool CFlashVideoPlayer::PreRender()
{
  CheckConfig();
  
  if (m_playing)
  {  
    if (!m_bImageLocked)
    {
      m_dll.FlashLockImage(m_handle);
      m_bImageLocked = true;
    }
    
    int nHeight = m_dll.FlashGetHeight(m_handle)  - m_cropTop - m_cropBottom;
    int nWidth = m_dll.FlashGetWidth(m_handle)  - m_cropLeft - m_cropRight;
    
    CSingleLock lock(m_lock);  
    g_renderManager.SetRGB32Image(m_bDirty?(char*) m_dll.FlashGetImage(m_handle):NULL, nHeight, nWidth, m_dll.FlashGetWidth(m_handle) * 4);
    m_bDirty = false;
  }
  
  return m_playing;
}

void CFlashVideoPlayer::PostRender()
{
  CSingleLock lock(m_lock);
  if (m_bImageLocked)
  {
    m_bImageLocked = false;
    m_dll.FlashUnlockImage(m_handle);
  }  
}

void CFlashVideoPlayer::Ping()
{
  if (m_pDlgCache && m_pDlgCache->IsCanceled())
  {
    CloseFile();
    return;    
  }
  
  CheckConfig();

  if (m_bShowKeyboard)
  {
    CStdString strText;
    m_bShowKeyboard = false;
    m_bCloseKeyboard = true;
    bool bConfirmed = CGUIDialogKeyboard::ShowAndGetInput(strText, m_strKeyboardCaption, true); 
    m_bCloseKeyboard = false;
    if (!m_bStop)
      m_dll.FlashUserText(m_handle, BOXEE::BXUtils::URLEncode(strText.c_str()).c_str(), m_strKeyboardCallback.c_str(), bConfirmed);
    m_strKeyboardCaption.clear();
    m_strKeyboardCallback.clear();
  }
}

bool CFlashVideoPlayer::IsPlaying() const
{
  return m_playing;
}

void CFlashVideoPlayer::Process()
{
  m_clock = 0;
  m_lastTime = CTimeUtils::GetTimeMS();

  while (!m_bStop)
  {
    if (m_playing)
    {      
      if (!m_paused)
        m_clock += (CTimeUtils::GetTimeMS() - m_lastTime)*m_speed;
      m_lastTime = CTimeUtils::GetTimeMS();      
    }
	
    if (m_handle) 
      m_dll.FlashUpdate(m_handle, 500000);
  }
  
  m_playing = false;
  if (m_bStop)
  {
    if (m_userStopped)
      m_callback.OnPlayBackStopped();
    else
      m_callback.OnPlayBackEnded();
  }
}

void CFlashVideoPlayer::Pause()
{
  if(!CanPause())
  {
    g_application.m_guiDialogKaiToast.QueueNotification("pause_not_available.png", "", g_localizeStrings.Get(51671), 2000);
  }
  else
  {
    if (m_paused)
    {
      m_dll.FlashPlay(m_handle);
    }
    else
    {
      m_dll.FlashPause(m_handle);
    }
  
    m_paused = !m_paused;
  }
}

bool CFlashVideoPlayer::IsPaused() const
{
  return m_paused;
}

bool CFlashVideoPlayer::HasVideo() const
{
  return true;
}

bool CFlashVideoPlayer::HasAudio() const
{
  return true;
}

void CFlashVideoPlayer::SwitchToNextLanguage()
{
}

void CFlashVideoPlayer::ToggleSubtitles()
{
}

bool CFlashVideoPlayer::CanSeek()
{
  return CanSkip();
}

void CFlashVideoPlayer::Seek(bool bPlus, bool bLargeStep)
{
  if(CanSeek())
  {
    if (bLargeStep)
    {
      m_dll.FlashBigStep(m_handle, !bPlus);
    }
    else 
    {
      m_dll.FlashSmallStep(m_handle, !bPlus);
    }
  }
}

void CFlashVideoPlayer::ToggleFrameDrop()
{
}

void CFlashVideoPlayer::GetAudioInfo(CStdString& strAudioInfo)
{
  strAudioInfo = "CFlashVideoPlayer - nothing to see here";
}

void CFlashVideoPlayer::GetVideoInfo(CStdString& strVideoInfo)
{
  strVideoInfo.Format("Width: %d, height: %d", m_width, m_height);
}

void CFlashVideoPlayer::GetGeneralInfo(CStdString& strGeneralInfo)
{
  strGeneralInfo = "CFlashVideoPlayer ("+m_path+")";
}

void CFlashVideoPlayer::SwitchToNextAudioLanguage()
{
}

void CFlashVideoPlayer::SetVolume(long nVolume) 
{
  if(!CanSetVolume())
  {
    g_application.m_guiDialogKaiToast.QueueNotification("set_volume_not_available.png","",g_localizeStrings.Get(51673),2000);
  }
  else
  {
    int nPct = (int)fabs(((float)(nVolume - VOLUME_MINIMUM) / (float)(VOLUME_MAXIMUM - VOLUME_MINIMUM)) * 100.0);
    m_dll.FlashSetVolume(m_handle, nPct);    
  }
}

void CFlashVideoPlayer::SeekPercentage(float iPercent)
{
  __int64 iTotalMsec = GetTotalTime() * 1000;
  __int64 iTime = (__int64)(iTotalMsec * iPercent / 100);
  SeekTime(iTime);
}

float CFlashVideoPlayer::GetPercentage()
{
  return (float)m_pct ; 
}

//This is how much audio is delayed to video, we count the oposite in the dvdplayer
void CFlashVideoPlayer::SetAVDelay(float fValue)
{
}

float CFlashVideoPlayer::GetAVDelay()
{
  return 0.0f;
}

void CFlashVideoPlayer::SetSubTitleDelay(float fValue)
{
}

float CFlashVideoPlayer::GetSubTitleDelay()
{
  return 0.0;
}

void CFlashVideoPlayer::SeekTime(__int64 iTime)
{
//  m_clock = iTime;
  CStdString caption = g_localizeStrings.Get(52050); 
  CStdString description = g_localizeStrings.Get(52051); 
  g_application.m_guiDialogKaiToast.QueueNotification(caption, description);
  
  return;
}

// return the time in milliseconds
__int64 CFlashVideoPlayer::GetTime()
{
  return m_clock;
}

// return length in seconds.. this should be changed to return in milleseconds throughout xbmc
int CFlashVideoPlayer::GetTotalTime()
{
  return m_totalTime;
}

void CFlashVideoPlayer::ToFFRW(int iSpeed)
{
  m_speed = iSpeed;
}

void CFlashVideoPlayer::ShowOSD(bool bOnoff)
{
}

CStdString CFlashVideoPlayer::GetPlayerState()
{
  return "";
}

bool CFlashVideoPlayer::SetPlayerState(CStdString state)
{
  return true;
}

void CFlashVideoPlayer::Render()
{
}

void CFlashVideoPlayer::FlashPlaybackEnded()
{
  m_userStopped = false;
  ThreadMessage tMsg = {TMSG_MEDIA_STOP};
  g_application.getApplicationMessenger().SendMessage(tMsg, false);
}

void CFlashVideoPlayer::FlashPlaybackStarted()
{
  m_callback.OnPlayBackStarted();
  m_bDirty = true;
}

void CFlashVideoPlayer::FlashNewFrame() 
{
  CSingleLock lock(m_lock);
  m_bDirty = true;
  g_application.NewFrame();
}

void CFlashVideoPlayer::FlashPaused()
{
  m_paused = true;
}

void CFlashVideoPlayer::FlashResumed() 
{
  m_paused = false;
}

void CFlashVideoPlayer::FlashProgress(int nPct) 
{
  m_pct = nPct;
}

void CFlashVideoPlayer::FlashTime(int nTime) 
{
  m_clock = nTime * 1000;
}

void CFlashVideoPlayer::FlashDuration(int nTime) 
{
  m_totalTime = nTime;
}

void CFlashVideoPlayer::FlashConfigChange(int nWidth, int nHeight)
{
  CSingleLock lock(m_lock);
  
  int h = nHeight  - m_cropTop - m_cropBottom;
  int w = nWidth  - m_cropLeft - m_cropRight;

  m_width = w;
  m_height = h;  

  m_bConfigChanged = true;
  m_bDirty = true;
}

void CFlashVideoPlayer::FlashSetCanPause(bool canPause)
{
  m_canPause = canPause;
}

void CFlashVideoPlayer::FlashSetCanSkip(bool canSkip)
{
  m_canSkip = canSkip;
}

void CFlashVideoPlayer::FlashSetCanSetVolume(bool canSetVolume)
{
  m_canSetVolume = canSetVolume;
}

bool CFlashVideoPlayer::CanPause() const
{
  return m_canPause;
}

bool CFlashVideoPlayer::CanSkip() const
{
  return m_canSkip;
}

bool CFlashVideoPlayer::CanSetVolume() const
{
  return m_canSetVolume;
}

void CFlashVideoPlayer::FlashGetTextInput(const char *title, const char *callback) 
{ 
  m_bShowKeyboard=true;
  m_strKeyboardCaption=BOXEE::BXUtils::URLDecode(title);
  m_strKeyboardCallback=callback;
}

void CFlashVideoPlayer::FlashNotification(const char *text, const char *thumb, int nTimeout)
{
  if (!text || !thumb)
    return;
  
  CStdString strDecoded = BOXEE::BXUtils::URLDecode(text);
  
  CLog::Log(LOGDEBUG,"notification from flash: <%s> (%s)", text, strDecoded.c_str());
  
  g_application.m_guiDialogKaiToast.QueueNotification(BOXEE::BXUtils::URLDecode(thumb), "", strDecoded, nTimeout*1000);  
}

void CFlashVideoPlayer::FlashEnableExt(int nID, const char *extText, const char *extThumb)
{
  CFileItem& f = g_application.CurrentFileItem();
  CStdString extPref;
  extPref.Format( "osd-ext-%d-", nID );
  f.SetProperty(extPref+"text", BOXEE::BXUtils::URLDecode(extText).c_str());
  f.SetProperty(extPref+"thumb", BOXEE::BXUtils::URLDecode(extThumb).c_str());
  f.SetProperty(extPref+"on", true);
}

void CFlashVideoPlayer::FlashDisableExt(int nID)
{
  CFileItem& f = g_application.CurrentFileItem();
  CStdString extPref;
  extPref.Format( "osd-ext-%d-", nID );
  f.SetProperty(extPref+"on", false);
}

