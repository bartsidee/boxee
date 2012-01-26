#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://xbmc.org
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

#include <vector>
#include "StdString.h"

class TiXmlElement;

struct TVShowRegexp
{
  bool byDate;
  CStdString regexp;
  TVShowRegexp(bool d, const CStdString& r)
  {
    byDate = d;
    regexp = r;
  }
};

typedef std::vector<TVShowRegexp> SETTINGS_TVSHOWLIST;

struct RefreshOverride
{
  float fpsmin;
  float fpsmax;

  float refreshmin;
  float refreshmax;

  bool  fallback;
};

class CAdvancedSettings
{
  public:
    CAdvancedSettings();

    bool Load();
    void Clear();

    static void GetCustomTVRegexps(TiXmlElement *pRootElement, SETTINGS_TVSHOWLIST& settings);
    static void GetCustomRegexps(TiXmlElement *pRootElement, CStdStringArray& settings);
    static void GetCustomRegexpReplacers(TiXmlElement *pRootElement, CStdStringArray& settings);
    static void GetCustomExtensions(TiXmlElement *pRootElement, CStdString& extensions);

    // multipath testing
    bool m_useMultipaths;

    int m_audioHeadRoom;
    float m_ac3Gain;
    CStdString m_audioDefaultPlayer;
    float m_audioPlayCountMinimumPercent;

    float m_videoSubsDelayRange;
    float m_videoAudioDelayRange;
    int m_videoSmallStepBackSeconds;
    int m_videoSmallStepBackTries;
    int m_videoSmallStepBackDelay;
    bool m_videoUseTimeSeeking;
    int m_videoTimeSeekForward;
    int m_videoTimeSeekBackward;
    int m_videoTimeSeekForwardBig;
    int m_videoTimeSeekBackwardBig;
    int m_videoPercentSeekForward;
    int m_videoPercentSeekBackward;
    int m_videoPercentSeekForwardBig;
    int m_videoPercentSeekBackwardBig;
    CStdString m_videoPPFFmpegType;
    bool m_musicUseTimeSeeking;
    int m_musicTimeSeekForward;
    int m_musicTimeSeekBackward;
    int m_musicTimeSeekForwardBig;
    int m_musicTimeSeekBackwardBig;
    int m_musicPercentSeekForward;
    int m_musicPercentSeekBackward;
    int m_musicPercentSeekForwardBig;
    int m_musicPercentSeekBackwardBig;
    int m_musicResample;
    int m_videoBlackBarColour;
    int m_videoIgnoreAtStart;
    int m_videoIgnoreAtEnd;
    CStdString m_audioHost;
    bool m_audioApplyDrc;

    bool  m_videoVDPAUScaling;
    float m_videoNonLinStretchRatio;
    bool  m_videoAllowLanczos3;
    float m_videoAutoScaleMaxFps;
    bool  m_videoAllowMpeg4VDPAU;
    std::vector<RefreshOverride> m_videoAdjustRefreshOverrides;

	  bool m_DXVACheckCompatibility;
    bool m_DXVACheckCompatibilityPresent;

    CStdString m_videoDefaultPlayer;
    CStdString m_videoDefaultDVDPlayer;
    float m_videoPlayCountMinimumPercent;

    float m_slideshowBlackBarCompensation;
    float m_slideshowZoomAmount;
    float m_slideshowPanAmount;

    int m_lcdRows;
    int m_lcdColumns;
    int m_lcdAddress1;
    int m_lcdAddress2;
    int m_lcdAddress3;
    int m_lcdAddress4;
    bool m_lcdHeartbeat;
    int m_lcdScrolldelay;

    int m_autoDetectPingTime;

    int m_songInfoDuration;
    int m_busyDialogDelay;
    CStdString m_cddbAddress;
#ifdef HAS_HAL
    bool m_useHalMount;
#endif
    bool m_fullScreenOnMovieStart;
    bool m_noDVDROM;
    CStdString m_cachePath;
    bool m_displayRemoteCodes;
    CStdString m_videoCleanDateTimeRegExp;
    CStdStringArray m_videoCleanStringRegExps;
    CStdStringArray m_videoExcludeFromListingRegExps;
    CStdStringArray m_moviesExcludeFromScanRegExps;
    CStdStringArray m_tvshowExcludeFromScanRegExps;
    CStdStringArray m_audioExcludeFromListingRegExps;
    CStdStringArray m_audioExcludeFromScanRegExps;
    CStdStringArray m_pictureExcludeFromListingRegExps;
    CStdStringArray m_videoStackRegExps;
    SETTINGS_TVSHOWLIST m_tvshowStackRegExps;
    CStdString m_tvshowMultiPartStackRegExp;
    CStdStringArray m_pathSubstitutions;
    int m_remoteDelay;
    float m_controllerDeadzone;

    bool m_playlistAsFolders;
    bool m_detectAsUdf;

    int m_thumbSize;
    bool m_useDDSFanart;

    int m_sambaclienttimeout;
    CStdString m_sambadoscodepage;
    bool m_sambastatfiles;

    bool m_bHTTPDirectoryStatFilesize;

    CStdString m_musicThumbs;
    CStdString m_dvdThumbs;
    CStdString m_fanartImages;

    bool m_bMusicLibraryHideAllItems;
    int m_iMusicLibraryRecentlyAddedItems;
    bool m_bMusicLibraryAllItemsOnBottom;
    bool m_bMusicLibraryAlbumsSortByArtistThenYear;
    CStdString m_strMusicLibraryAlbumFormat;
    CStdString m_strMusicLibraryAlbumFormatRight;
    bool m_prioritiseAPEv2tags;
    CStdString m_musicItemSeparator;
    CStdString m_videoItemSeparator;
    std::vector<CStdString> m_musicTagsFromFileFilters;

    bool m_bVideoLibraryHideAllItems;
    bool m_bVideoLibraryAllItemsOnBottom;
    int m_iVideoLibraryRecentlyAddedItems;
    bool m_bVideoLibraryHideRecentlyAddedItems;
    bool m_bVideoLibraryHideEmptySeries;
    bool m_bVideoLibraryCleanOnUpdate;
    bool m_bVideoLibraryExportAutoThumbs;
    bool m_bVideoLibraryMyMoviesCategoriesToGenres;

    bool m_bUseEvilB;
    std::vector<CStdString> m_vecTokens; // cleaning strings tied to language
    //TuxBox
    bool m_bTuxBoxSubMenuSelection;
    int m_iTuxBoxDefaultSubMenu;
    int m_iTuxBoxDefaultRootMenu;
    bool m_bTuxBoxAudioChannelSelection;
    bool m_bTuxBoxPictureIcon;
    int m_iTuxBoxEpgRequestTime;
    int m_iTuxBoxZapWaitTime;
    bool m_bTuxBoxSendAllAPids;

    int m_iMythMovieLength;         // minutes

    // EDL Commercial Break
    bool m_bEdlMergeShortCommBreaks;
    int m_iEdlMaxCommBreakLength;   // seconds
    int m_iEdlMinCommBreakLength;   // seconds
    int m_iEdlMaxCommBreakGap;      // seconds
    int m_iEdlMaxStartGap;          // seconds

    bool m_bFirstLoop;
    int m_curlconnecttimeout;
    int m_curllowspeedtime;
    int m_curlretries;

    bool m_fullScreen;
    bool m_startFullScreen;
    bool m_alwaysOnTop;  /* makes xbmc to run always on top .. osx/win32 only .. */
    int m_playlistRetries;
    int m_playlistTimeout;
    bool m_GLRectangleHack;
    int m_iSkipLoopFilter;
    float m_ForcedSwapTime; /* if nonzero, set's the explicit time in ms to allocate for buffer swap */

    bool m_AllowD3D9Ex;
    bool m_ForceD3D9Ex;
    bool m_AllowDynamicTextures;
    unsigned int m_RestrictCapsMask;

    bool m_osx_GLFullScreen;
    float m_sleepBeforeFlip; ///< if true, XBMC waits for raster to be close to the end of the display before flipping.
    bool m_bVirtualShares;

    float m_karaokeSyncDelayCDG; // seems like different delay is needed for CDG and MP3s
    float m_karaokeSyncDelayLRC;
    bool m_karaokeChangeGenreForKaraokeSongs;
    bool m_karaokeKeepDelay; // store user-changed song delay in the database
    int m_karaokeStartIndex; // auto-assign numbering start from this value
    bool m_karaokeAlwaysEmptyOnCdgs; // always have empty background on CDG files
    bool m_karaokeUseSongSpecificBackground; // use song-specific video or image if available instead of default
    CStdString m_karaokeDefaultBackgroundType; // empty string or "vis", "image" or "video"
    CStdString m_karaokeDefaultBackgroundFilePath; // only for "image" or "video" types above

    CStdString m_cpuTempCmd;
    CStdString m_gpuTempCmd;
    int m_bgInfoLoaderMaxThreads;

    bool m_measureRefreshrate; //when true the videoreferenceclock will measure the refreshrate when direct3d is used
    //otherwise it will use the windows refreshrate
    int m_logFileSize;

    // Boxee
    CStdString m_appsUrl;
    bool m_bWireFrameMode;
    bool m_bCountPixels;

    bool m_bForceVideoHardwareDecoding;
    bool m_bForceAudioHardwarePassthrough;
    bool m_bForceWMV3Flushing;

    // quota limitation
    uint64_t   m_httpCacheMaxSize;   // (in KB)
    uint64_t   m_appsMaxSize;        // (in KB)
    uint64_t   m_thumbsMaxSize;      // (in KB)
    
    // configuration options for dd+ certification testing
    struct __ddplus
    {
      __ddplus() { Clear(); }
      void Clear()
      {
        ltmode = false;
        rfmode = false;
        lfemode = true;
        drc = false;
        monomode = _stereo;
      }
      bool ltmode; // downmix LtRt if true, LoRo if false
      bool rfmode; // RF if true, line out if false
      bool lfemode; // use LFE if true, don't if false
      bool drc; // use DRC if true, don't if false
      enum _monotype
      {
        _stereo,  // default
        _left,
        _right,
        _mixed,
      };
      _monotype monomode;
    } m_ddplus;
    

};

extern CAdvancedSettings g_advancedSettings;

