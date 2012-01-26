INCLUDES+=-I. -I.. -Ilinux -Icores -I../guilib -Iutils -IFileSystem -Icores/dvdplayer -Icores/ffmpeg -Icores/dvdplayer/Codecs -Icores/dvdplayer/Codecs/ffmpeg -Ilib/libjson/include -Icores/dvb

INCLUDES+=-Ilib/libUPnP/Platinum/Source/Core \
          -Ilib/libUPnP/Platinum/Source/Platinum \
          -Ilib/libUPnP/Platinum/Source/Devices/MediaServer \
          -Ilib/libUPnP/Platinum/Source/Devices/MediaConnect \
          -Ilib/libUPnP/Platinum/Source/Devices/MediaRenderer \
          -Ilib/libUPnP/Neptune/Source/System/Posix \
          -Ilib/libUPnP/Neptune/Source/Core

INCLUDES+=-Ilib/libcdio/libcdio/include

INCLUDES+=-Ilib/libBoxee -Ilib/libjson

CXXFLAGS+= -D__STDC_CONSTANT_MACROS

PCH=stdafx.h

SRCS=Application.cpp \
     CueDocument.cpp \
     GUISettings.cpp \
     GUIWindowSettings.cpp \
     GUIWindowSettingsCategory.cpp \
     GUIWindowSettingsProfile.cpp \
     GUIWindowSettingsScreenCalibration.cpp \
     Settings.cpp \
     SettingsControls.cpp \
     GUIDialogMusicScan.cpp \
     GUIViewControl.cpp \
     GUIViewState.cpp \
     GUIViewStateMusic.cpp \
     GUIWindowMusicBase.cpp \
     GUIWindowMusicInfo.cpp \
     GUIWindowMusicOverlay.cpp \
     SmartPlaylist.cpp \
     GUIDialogVideoScan.cpp \
     GUIViewStateVideo.cpp \
     GUIWindowVideoBase.cpp \
     GUIWindowVideoInfo.cpp \
     GUIWindowVideoOverlay.cpp \
     VideoInfoScanner.cpp \
     PlayList.cpp \
     PlayListB4S.cpp \
     PlayListFactory.cpp \
     PlayListM3U.cpp \
     PlayListPlayer.cpp \
     PlayListPLS.cpp \
     PlayListWPL.cpp \
     PlayListURL.cpp \
     PlayListXML.cpp \
     APEv2Tag.cpp \
     FlacTag.cpp \
     Id3Tag.cpp \
     MusicInfoLoader.cpp \
     MusicInfoScanner.cpp \
     MusicInfoTag.cpp \
     MusicInfoTagLoaderAAC.cpp \
     MusicInfoTagLoaderAdplug.cpp \
     MusicInfoTagLoaderApe.cpp \
     MusicInfoTagLoaderCDDA.cpp \
     MusicInfoTagLoaderDatabase.cpp \
     MusicInfoTagLoaderFactory.cpp \
     MusicInfoTagLoaderFlac.cpp \
     MusicInfoTagLoaderGYM.cpp \
     MusicInfoTagLoaderMidi.cpp \
     MusicInfoTagLoaderMod.cpp \
     MusicInfoTagLoaderMP3.cpp \
     MusicInfoTagLoaderMP4.cpp \
     MusicInfoTagLoaderMPC.cpp \
     MusicInfoTagLoaderNSF.cpp \
     MusicInfoTagLoaderOgg.cpp \
     MusicInfoTagLoaderShn.cpp \
     MusicInfoTagLoaderSid.cpp \
     MusicInfoTagLoaderSPC.cpp \
     MusicInfoTagLoaderUPnP.cpp \
     MusicInfoTagLoaderWav.cpp \
     MusicInfoTagLoaderWavPack.cpp \
     MusicInfoTagLoaderWMA.cpp \
     MusicInfoTagLoaderYM.cpp \
     OggTag.cpp \
     VorbisTag.cpp \
     AutoPtrHandle.cpp \
     AutoSwitch.cpp \
     ButtonTranslator.cpp \
     Crc32.cpp \
     DateTime.cpp \
     DetectDVDType.cpp \
     DNSNameCache.cpp \
     DynamicDll.cpp \
     FileItem.cpp \
     GUIPassword.cpp \
     LangCodeExpander.cpp \
     LangInfo.cpp \
     MediaManager.cpp \
     NfoFile.cpp \
     Picture.cpp \
     Profile.cpp \
     SectionLoader.cpp \
     Shortcut.cpp \
     SortFileItem.cpp \
     StringUtils.cpp \
     Temperature.cpp \
     ThumbnailCache.cpp \
     URL.cpp \
     VideoInfoTag.cpp \
     XBAudioConfig.cpp \
     Database.cpp \
     MusicDatabase.cpp \
     ProgramDatabase.cpp \
     Song.cpp \
     VideoDatabase.cpp \
     ViewDatabase.cpp \
     GUIDialogAudioSubtitleSettings.cpp \
     GUIDialogBoxeeBrowseSubtitleSettings.cpp \
     GUIDialogBoxeeBrowseLocalSubtitleSettings.cpp \
     GUIDialogSubtitleSettings.cpp \
     GUIDialogBoxBase.cpp \
     GUIDialogButtonMenu.cpp \
     GUIDialogContentSettings.cpp \
     GUIDialogContextMenu.cpp \
     GUIDialogFileBrowser.cpp \
     GUIDialogFileStacking.cpp \
     GUIDialogGamepad.cpp \
     GUIDialogKeyboard.cpp \
     GUIDialogLockSettings.cpp \
     GUIDialogMediaSource.cpp \
     GUIDialogMusicOSD.cpp \
     GUIDialogMuteBug.cpp \
     GUIDialogNetworkSetup.cpp \
     GUIDialogNumeric.cpp \
     GUIDialogOK.cpp \
     GUIWebDialog.cpp \
     GUIDialogPlayerControls.cpp \
     GUIDialogProfileSettings.cpp \
     GUIDialogProgress.cpp \
     GUIDialogSeekBar.cpp \
     GUIDialogSelect.cpp \
     GUIDialogBoxeeEject.cpp \
     GUIDialogBoxeeGetFacebookExtraCredential.cpp \
     GUIDialogBoxeeWatchLaterGetStarted.cpp \
     GUIDialogBoxeeMakeBoxeeSocial.cpp \
     GUIDialogSettings.cpp \
     GUIDialogSubMenu.cpp \
     GUIDialogTeletext.cpp \
     GUIDialogVideoBookmarks.cpp \
     GUIDialogVideoSettings.cpp \
     GUIDialogVisualisationPresetList.cpp \
     GUIDialogVisualisationSettings.cpp \
     GUIDialogVolumeBar.cpp \
     GUIDialogSlider.cpp \
     GUIDialogYesNo.cpp \
     GUIMediaWindow.cpp \
     GUIWindowFileManager.cpp \
     GUIWindowFullScreen.cpp \
     GUIWindowHome.cpp \
     GUIWindowLoginScreen.cpp \
     GUIWindowOSD.cpp \
     GUIWindowPictures.cpp \
     GUIWindowPointer.cpp \
     GUIWindowPrograms.cpp \
     GUIWindowScreensaver.cpp \
     GUIWindowScripts.cpp \
     GUIWindowScriptsInfo.cpp \
     GUIWindowSystemInfo.cpp \
     GUIWindowVisualisation.cpp \
     BackgroundInfoLoader.cpp \
     PictureThumbLoader.cpp \
     ThumbLoader.cpp \
     ApplicationMessenger.cpp \
     Autorun.cpp \
     Util.cpp \
     GUIWindowSlideShow.cpp \
     XBApplicationEx.cpp \
     xbmc.cpp \
     GUIDialogFavourites.cpp \
     GUIDialogSongInfo.cpp \
     Favourites.cpp \
     GUIDialogSmartPlaylistEditor.cpp \
     GUIDialogSmartPlaylistRule.cpp \
     SlideShowPicture.cpp \
     ApplicationRenderer.cpp \
     GUIDialogBusy.cpp \
     GUIWindowStartup.cpp \
     UPnP.cpp \
     PictureInfoLoader.cpp \
     GUIDialogPictureInfo.cpp \
     LastFmManager.cpp \
     PictureInfoTag.cpp \
     GUILargeTextureManager.cpp \
     GUIDialogKaiToast.cpp \
     KeyboardLayoutConfiguration.cpp \
     GUIDialogPluginSettings.cpp \
     PluginSettings.cpp \
     GUIDialogAccessPoints.cpp \
     ScraperSettings.cpp \
     Artist.cpp \
     Album.cpp \
     MediaSource.cpp \
     MusicInfoTagLoaderASAP.cpp \
     GUIWindowTestPattern.cpp \
     GUIViewStateScripts.cpp \
     GUIViewStatePrograms.cpp \
     GUIViewStatePictures.cpp \
     GUIDialogFullScreenInfo.cpp \
     PowerManager.cpp \
     Zeroconf.cpp \
     GUIWindowBoxeeAlbumInfo.cpp \
     GUIDialogYesNo2.cpp \
     GUIDialogOK2.cpp \
     BoxeeVersionUpdateManager.cpp \
     GUIDialogBoxeeUpdateProgress.cpp \
     GUIDialogBoxeeUpdateMessage.cpp \
     GUIDialogBoxeeApplicationAction.cpp \
     GUIDialogBoxeeRssFeedInfo.cpp \
     ZeroconfBrowser.cpp \
     VideoReferenceClock.cpp \
     DPMSSupport.cpp \
     ScriptSettings.cpp \
     GUIWindowTestPatternGL.cpp \
     GUIWindowTestPatternGLES.cpp \
     RenderSystem.cpp \
     RenderSystemGL.cpp \
     RenderSystemGLES.cpp \
     KeyboardStat.cpp \
     MouseStat.cpp \
     WinSystem.cpp \
     WinSystemX11.cpp \
     WinSystemX11GL.cpp \
     WinSystemGDL.cpp \
     WinSystemGDLGLES.cpp \
     WinBindingEGL.cpp \
     WinSystemEGL.cpp \
     WinSystemEGLGLES.cpp \
     WinEventsSDL.cpp \
     WinEventsX11.cpp \
     WinEventsBoxeeBox.cpp \
     WinEventsIntelCE.cpp \
     AdvancedSettings.cpp \
     Bookmark.cpp \
     InfoPage.cpp \
     GUIDialogBoxeeUserPassword.cpp \
     GUIDialogBoxeeVideoQuality.cpp \
     GUIDialogBoxeeSelectionList.cpp \
     ThumbnailsManager.cpp \
     GUIDialogBoxeeManualResolveAlbum.cpp \
     GUIDialogBoxeeManualResolveAudio.cpp \
     GUIWindowBoxeeBrowseSimpleApp.cpp \
     GUIWindowStateDatabase.cpp \
     AppSecurity.cpp \
     GUIDialogBoxeeOTAFacebookConnect.cpp \
     BoxeeOTAConfigurationData.cpp \
     GUIDialogBoxeeOTALocationConfiguration.cpp \
     BoxeeOTAConfigurationManager.cpp \
     GUIDialogBoxeeOTAConfiguration.cpp \
     GUIWindowBoxeeFeed.cpp \
     GUIDialogBoxeeContext.cpp \
     GUIDialogBoxeeRate.cpp \
     BoxeeUtils.cpp \
     BoxeeObjectCache.cpp \
     GUIDialogBoxeeDropdown.cpp \
     GUIDialogBoxeeSearch.cpp \
     GUIDialogBoxeeLiveTvScan.cpp \
     RssSourceManager.cpp \
     FileScanner.cpp \
     MetadataResolver.cpp \
     MetadataResolverVideo.cpp \
     WatchDog.cpp \
     MetadataResolverMusic.cpp \
     GUIDialogBoxeeLiveTvEditChannels.cpp \
     GUIWindowBoxeeLiveTv.cpp \
     GUIDialogBoxeeLiveTvCtx.cpp \
     GUIDialogBoxeeVideoCtx.cpp \
     GUIDialogBoxeeCtx.cpp \
     GUIDialogBoxeeMusicCtx.cpp \
     GUIDialogBoxeeUserLogin.cpp \
     BoxeeFeedItemsLoader.cpp \
     BoxeeServerItemsLoader.cpp \
     GUIDialogBoxeeBuffering.cpp \
     GUIWindowBoxeeWizardAddSourceManual.cpp \
     LiveTvModel.cpp \
     GUIDialogBoxeeLiveTvInfo.cpp \
     ItemLoader.cpp \
     GUIWindowBoxeeBrowse.cpp \
     BrowseWindowState.cpp \
     GUIWindowBoxeeMain.cpp \
     GUIDialogBoxeeMainMenu.cpp \
     GUIDialogBoxeeBrowseMenu.cpp \
     GUIDialogBoxeeMediaAction.cpp \
     GUIWindowBoxeeMediaInfo.cpp \
     BoxeeSocialUtilsManager.cpp \
     BoxeeSocialUtilsUIManager.cpp

SRCS += GUIDialogBoxeeManualResolve.cpp GUIDialogBoxeeManualResolveMovie.cpp GUIDialogBoxeeManualResolveEpisode.cpp GUIDialogBoxeeManualResolveAddFiles.cpp GUIDialogBoxeeManualResolveDetails.cpp GUIDialogBoxeeManualResolvePartAction.cpp 
SRCS += GUIWindowBoxeeMediaSources.cpp GUIWindowBoxeeMediaSourceInfo.cpp GUIWindowBoxeeMediaSourceList.cpp GUIWindowBoxeeMediaSourceAddFolder.cpp GUIWindowBoxeeMediaSourceAddShare.cpp BoxeeMediaSourceList.cpp
SRCS += GUIDialogBoxeeAppCtx.cpp GUIDialogBoxeePictureCtx.cpp GUIDialogBoxeeLoggingIn.cpp GUIDialogBoxeeCredits.cpp BrowseWindowFilter.cpp BoxeeItemsHistory.cpp GUIWindowBoxeeApplicationSettings.cpp AppDescriptor.cpp AppManager.cpp GUIWindowApp.cpp AppRegistry.cpp GUIBoxeeViewState.cpp BoxeeViewDatabase.cpp AppRepository.cpp AppRepositories.cpp BoxeeAuthenticator.cpp BoxeeItemLauncher.cpp BoxeeShortcut.cpp GUIDialogBoxeeShortcutAction.cpp GUILoaderWindow.cpp GUIWindowSettingsScreenSimpleCalibration.cpp BoxeeLoginManager.cpp
SRCS += HttpCacheManager.cpp GUIWindowBoxeeBrowseLocal.cpp GUIWindowBoxeeBrowseTvShows.cpp GUIWindowBoxeeBrowseMovies.cpp GUIWindowBoxeeBrowseApps.cpp GUIWindowBoxeeBrowseTvEpisodes.cpp GUIWindowBoxeeBrowseAlbums.cpp GUIWindowBoxeeBrowseTracks.cpp GUIWindowBoxeeBrowseRepositories.cpp GUIWindowBoxeeBrowseQueue.cpp GUIWindowBoxeeBrowseDiscover.cpp GUIWindowBoxeeBrowseHistory.cpp GUIWindowBoxeeBrowsePhotos.cpp GUIDialogBoxeeShare.cpp GUIDialogBoxeePostPlay.cpp GUIWindowBoxeeBrowseProduct.cpp
SRCS += ThreadPolicy.cpp BrowserService.cpp IPScanJob.cpp UpdateSourceFile.cpp GUILoaderDialog.cpp GUIDialogBoxeeGlobalSearch.cpp GUIWindowBoxeeLoginPrompt.cpp KeyboardManager.cpp GUIDialogBoxeeMessageScroll.cpp GUIDialogBoxeeNetworkNotification.cpp
SRCS += BoxeeHalStorageProvider.cpp BoxeeHalServices.cpp NullHalServices.cpp HalServices.cpp HalListenerImpl.cpp GUIDialogWirelessAuthentication.cpp
SRCS += InitializeBoxManager.cpp GUIDialogFirstTimeUseBase.cpp GUIDialogFirstTimeUseWithList.cpp GUIDialogFirstTimeUseLang.cpp GUIDialogFirstTimeUseWelcome.cpp GUIDialogFirstTimeUseWireless.cpp GUIDialogFirstTimeUseConfWirelessPassword.cpp GUIDialogFirstTimeUseConfWirelessSecurity.cpp GUIDialogFirstTimeUseConfWirelessSSID.cpp GUIDialogFirstTimeUseNetworkMessage.cpp GUIDialogFirstTimeUseUpdateMessage.cpp GUIDialogFirstTimeUseUpdateProgress.cpp GUIDialogFirstTimeUseSimpleMessage.cpp GUIDialogFirstTimeUseConfNetwork.cpp GUIWindowFirstTimeUseBackground.cpp GUIWindowFirstTimeUseCalibration.cpp
SRCS += GUIWindowTestBadPixelsManager.cpp GUIWindowTestBadPixels.cpp
SRCS += GUIDialogBoxeePaymentProducts.cpp GUIDialogBoxeePaymentTou.cpp GUIDialogBoxeePaymentUserData.cpp GUIDialogBoxeePaymentWaitForServerApproval.cpp GUIDialogBoxeePaymentOkPlay.cpp GUIDialogBoxeeChapters.cpp GUIDialogBoxeeExitVideo.cpp GUIDialogBoxeeBrowserCtx.cpp GUIDialogBoxeeVideoResume.cpp GUIDialogBoxeeQuickTip.cpp GUIDialogBoxeeSortDropdown.cpp GUIDialogFirstTimeUseMenuCust.cpp GUIDialogBoxeeTechInfo.cpp
SRCS += GUIDialogBoxeeSeekBar.cpp BoxeeDeviceManager.cpp GUIDialogBoxeePair.cpp GUIWindowBoxeeSettingsDevices.cpp GUIDialogBoxeeSeekableCtx.cpp
SRCS += LicenseConfig.cpp GUIDialogBoxeeChannelFilter.cpp BoxeeSimpleDialogWizardManager.cpp GUIDialogBoxeeWizardBase.cpp
SRCS += BoxeeBrowseMenuManager.cpp
SRCS += GUIDialogBoxeeLoginWizardChooseUserToAddType.cpp BoxeeLoginWizardManager.cpp GUIDialogBoxeeLoginWizardAddExistingUser.cpp GUIDialogBoxeeLoginWizardAddNewUser.cpp GUIDialogBoxeeLoginWizardNewUserDetails.cpp GUIDialogBoxeeLoginWizardTOU.cpp GUIDialogBoxeeLoginWizardConnectSocialServices.cpp GUIDialogBoxeeLoginEditExistingUser.cpp GUIDialogBoxeeLoginWizardMenuCust.cpp GUIDialogBoxeeLoginWizardQuickTip.cpp GUIDialogBoxeeLoginWizardConfirmation.cpp GUIDialogBoxeeLoginWizardQuickTipAirPlay.cpp GUIDialogBoxeeOTANoChannels.cpp
LIB=xbmc.a

DISTCLEAN_FILES=DllPaths_generated.h

include ../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))


