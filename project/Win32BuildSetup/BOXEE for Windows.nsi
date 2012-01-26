;Boxee for Windows install script
;Copyright (C) 2008 Team Boxee
;http://www.boxee.tv

;Script by chadoe

;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"
  !include "nsDialogs.nsh"
  !include "LogicLib.nsh"
;--------------------------------
;General

  ;Name and file
  Name "Boxee"
  OutFile "BoxeeSetup-v${xbmc_revision}.exe"

  XPStyle on
  
  ;Default installation folder
  InstallDir "$PROGRAMFILES\Boxee"

  ;Get installation folder from registry if available
  InstallDirRegKey HKLM "Software\BOXEE" ""

  ;Request application privileges for Windows Vista
  RequestExecutionLevel highest

;--------------------------------
;Variables

  Var StartMenuFolder
  Var DirectXSetupError
  var VSRedistSetupError
  var FlashSetupError
  
;--------------------------------
;Interface Settings

  !define MUI_HEADERIMAGE
  ;!define MUI_HEADERIMAGE_BITMAP "boxee-banner.bmp"
  ;!define MUI_HEADERIMAGE_RIGHT
  !define MUI_WELCOMEFINISHPAGE_BITMAP "boxee-left.bmp"
  ;!define MUI_COMPONENTSPAGE_SMALLDESC
  ;!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\webapps\ROOT\RELEASE-NOTES.txt"
  !define MUI_FINISHPAGE_LINK "Please visit http://www.boxee.tv for more information."
  !define MUI_FINISHPAGE_LINK_LOCATION "http://www.boxee.tv"
  !define MUI_FINISHPAGE_RUN "$INSTDIR\BOXEE.exe"
  !define MUI_FINISHPAGE_RUN_PARAMETERS "-p"
  !define MUI_FINISHPAGE_RUN_NOTCHECKED
  !define MUI_ABORTWARNING  
;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "eula.rtf"
  !insertmacro MUI_PAGE_DIRECTORY
  
  ;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM" 
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\BOXEE" 
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
  !insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder  

  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  UninstPage custom un.UnPageProfile un.UnPageProfileLeave
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"


;--------------------------------
;Uninstall Previous Version

Function .onInit
 
  ReadRegStr $R0 HKLM \
  "Software\BOXEE" \
  ""
  StrCmp $R0 "" done
 
  MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
  "A previous version of Boxee is already installed.$\n\
	Setup will uninstall the previous version before proceeding with this installation.$\n\
	Click 'OK' to uninstall the previous version or 'Cancel' to cancel this install. "\
  IDOK uninst
  Abort
 
;Run the uninstaller
uninst:
  ClearErrors
  ExecWait '$R0\Uninstall.exe /S _?=$INSTDIR' ;Do not copy the uninstaller to a temp file
 
  IfErrors no_remove_uninstaller done
    ;You can either use Delete /REBOOTOK in the uninstaller or add some code
    ;here to remove the uninstaller. Use a registry key to check
    ;whether the user has chosen to uninstall. If you are using an uninstaller
    ;components page, make sure all sections are uninstalled.
  no_remove_uninstaller:
 
done:
 
FunctionEnd


;--------------------------------
;Installer Sections

InstType "Full"
InstType "Minimal" 

Section "Kill flash player" SecKillFlash

  SectionIn 1 2
  
  ; Kill running flash player
  
  nsExec::Exec "TASKKILL /F /IM bxflplayer-win32.exe"
  nsExec::Exec "tskill" bxflplayer-win32.exe /a"

SectionEnd

Section "Boxee" SecXBMC
  SectionIn RO
  SectionIn 1 2 #section is in installtype Full and Minimal
  ;ADD YOUR OWN FILES HERE...
  
  SetOutPath "$INSTDIR"
  File "${xbmc_root}\Boxee\BOXEE.exe"
  File "${xbmc_root}\Boxee\*.dll"
  SetOutPath "$INSTDIR\license"
  File "${xbmc_root}\Boxee\license\*.*"
  SetOutPath "$INSTDIR\media"
  File /r /x *.so /x *.dylib "${xbmc_root}\Boxee\media\*.*"
  ;SetOutPath "$INSTDIR\sounds"
  ;File /r /x *.so /x *.dylib "${xbmc_root}\Boxee\sounds\*.*"
  ;Delete "$INSTDIR\system\players\flashplayer\js32.dll"
  ;Delete "$INSTDIR\system\players\flashplayer\nspr4.dll"
  ;Delete "$INSTDIR\system\players\flashplayer\flplayer.dll"
  SetOutPath "$INSTDIR\system"
  File /r /x *.so /x *.so.0 /x *.dylib /x xulrunner-armv6-linux /x osx /x linux /x win32_release /x win32_debug /x *.pdb "${xbmc_root}\Boxee\system\*.*"
  SetOutPath "$INSTDIR\userdata"
  File /r /x *.so /x *.dylib "${xbmc_root}\Boxee\userdata\*.*"
  SetOutPath "$INSTDIR\visualisations"
  File "${xbmc_root}\Boxee\visualisations\*.vis"
  SetOutPath "$INSTDIR\visualisations\Milkdrop"
  File /nonfatal /r "${xbmc_root}\Boxee\visualisations\Milkdrop\*.*"

  ;Store installation folder
  WriteRegStr HKLM "Software\BOXEE" "" $INSTDIR

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
    
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  ;Create shortcuts
  SetOutPath "$INSTDIR"
  CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\BOXEE.lnk" "$INSTDIR\BOXEE.exe" \
    "-p" "$INSTDIR\BOXEE.exe" 0 SW_SHOWNORMAL \
    "" "Start Boxee in fullscreen."

  ; Windowed mode not supported at install time
  ;CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Boxee (Windowed).lnk" "$INSTDIR\BOXEE.exe" \
  ;  "-p" "$INSTDIR\BOXEE.exe" 0 SW_SHOWNORMAL \
  ;  "" "Start Boxee in windowed mode."
  
  WriteINIStr "$SMPROGRAMS\$StartMenuFolder\Visit Boxee Online.url" "InternetShortcut" "URL" "http://www.boxee.tv"
  !insertmacro MUI_STARTMENU_WRITE_END  
	
  ;add entry to add/remove programs
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BOXEE" \
                 "DisplayName" "Boxee"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BOXEE" \
                 "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BOXEE" \
                 "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BOXEE" \
                 "NoRepair" 1
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BOXEE" \
                 "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BOXEE" \
                 "DisplayIcon" "$INSTDIR\BOXEE.exe,0"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BOXEE" \
                 "Publisher" "Boxee"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BOXEE" \
                 "HelpLink" "http://forum.boxee.tv"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BOXEE" \
                 "URLInfoAbout" "http://www.boxee.tv"
  Exec "$INSTDIR\BOXEE.exe -usf"

SectionEnd

SectionGroup "Language" SecLanguages
Section "English" SecLanguageEnglish
  SectionIn 1 2 #section is in installtype Full and Minimal
  SectionIn RO
  SetOutPath "$INSTDIR\language"
  File /r "${xbmc_root}\Boxee\language\*.*"
SectionEnd
;languages.nsi is generated by genNsisIncludes.bat
!include /nonfatal "languages.nsi"
SectionGroupEnd

SectionGroup "Skins" SecSkins
Section "Boxee Skin NG" SecSkinPMIII
  SectionIn 1 2 #section is in installtype Full and Minimal
  SectionIn RO
  ;SetOutPath "$INSTDIR\skin\boxee\language\English"
  ;File /r "${xbmc_root}\Boxee\language\English\*.*"
  SetOutPath "$INSTDIR\skin\boxee"
  File /r "${xbmc_root}\Boxee\skin\boxee\*.*"
SectionEnd
;skins.nsi is generated by genNsisIncludes.bat
!include /nonfatal "skins.nsi"
SectionGroupEnd

;SectionGroup "Scripts" SecScripts
;scripts.nsi is generated by genNsisIncludes.bat
!include /nonfatal "scripts.nsi"
;SectionGroupEnd

;SectionGroup "Plugins" SecPlugins
;plugins.nsi is generated by genNsisIncludes.bat
;!include /nonfatal "plugins.nsi"
;SectionGroupEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecXBMC ${LANG_ENGLISH} "Boxee"

  ;Assign language strings to sections
  ;!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    ;!insertmacro MUI_DESCRIPTION_TEXT ${SecXBMC} $(DESC_SecXBMC)
 ; !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Var UnPageProfileDialog
Var UnPageProfileCheckbox
Var UnPageProfileCheckbox_State
Var UnPageProfileEditBox

Function un.UnPageProfile
    !insertmacro MUI_HEADER_TEXT "Uninstall Boxee" "Remove Boxee's profile folder from your computer."
	nsDialogs::Create /NOUNLOAD 1018
	Pop $UnPageProfileDialog

	${If} $UnPageProfileDialog == error
		Abort
	${EndIf}

	${NSD_CreateLabel} 0 0 100% 12u "Do you want to delete the profile folder?"
	Pop $0

	${NSD_CreateText} 0 13u 100% 12u "$APPDATA\BOXEE\"
	Pop $UnPageProfileEditBox
    SendMessage $UnPageProfileEditBox ${EM_SETREADONLY} 1 0

	${NSD_CreateLabel} 0 46u 100% 24u "Leave unchecked to keep the profile folder for later use or check to delete the profile folder."
	Pop $0

	${NSD_CreateCheckbox} 0 71u 100% 8u "Yes, also delete the profile folder."
	Pop $UnPageProfileCheckbox
	

	nsDialogs::Show
FunctionEnd

Function un.UnPageProfileLeave
${NSD_GetState} $UnPageProfileCheckbox $UnPageProfileCheckbox_State
FunctionEnd

Section "Uninstall"

  ;ADD YOUR OWN FILES HERE...
  Delete "$INSTDIR\BOXEE.exe"
  Delete "$INSTDIR\glew32.dll"
  Delete "$INSTDIR\jpeg.dll"
  Delete "$INSTDIR\libpng12-0.dll"
  Delete "$INSTDIR\libtiff-3.dll"
  Delete "$INSTDIR\SDL.dll"
  Delete "$INSTDIR\SDL_image.dll"
  Delete "$INSTDIR\zlib1.dll"
  Delete "$INSTDIR\libcurl.dll"
  Delete "$INSTDIR\MSVCP71.dll"
  Delete "$INSTDIR\msvcr71.dll"
  Delete "$INSTDIR\d3dx9_43.dll"
  ;Delete "$INSTDIR\boxee.log"
  ;Delete "$INSTDIR\boxee.old.log"
  Delete "$INSTDIR\gdbm3.dll"
  Delete "$INSTDIR\openldap.dll"
  Delete "$INSTDIR\libsasl.dll"
  Delete "$INSTDIR\boxee_catalog.db"
  RMDir /r "$INSTDIR\credits"
  RMDir /r "$INSTDIR\language"
  RMDir /r "$INSTDIR\license"
  RMDir /r "$INSTDIR\media"
  RMDir /r "$INSTDIR\plugins"
  RMDir /r "$INSTDIR\scripts"
  RMDir /r "$INSTDIR\skin"
  RMDir /r "$INSTDIR\sounds"
  RMDir /r "$INSTDIR\system"
  RMDir /r "$INSTDIR\userdata"
  RMDir /r "$INSTDIR\visualisations"

  Delete "$INSTDIR\Uninstall.exe"

  RMDir "$INSTDIR"
  
  ${If} $UnPageProfileCheckbox_State == ${BST_CHECKED}
    RMDir /r "$APPDATA\BOXEE\"
  ${EndIf}

  
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  Delete "$SMPROGRAMS\$StartMenuFolder\Boxee.lnk"
  ;Delete "$SMPROGRAMS\$StartMenuFolder\Boxee (Windowed).lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Visit Boxee Online.url"
  RMDir "$SMPROGRAMS\$StartMenuFolder"  
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BOXEE"

  DeleteRegKey /ifempty HKLM "Software\BOXEE"

SectionEnd

;--------------------------------
;DirectX webinstaller Section

;Section "DirectX Install" SEC_DIRECTX
 
;  SectionIn RO
 
;  SetOutPath "$TEMP"
;  File "${xbmc_root}\boxee\dxwebsetup.exe"
;  DetailPrint "Running DirectX Setup..."
;  ExecWait '"$TEMP\dxwebsetup.exe" /q /T:$TEMP' $DirectXSetupError
;  DetailPrint "Finished DirectX Setup"
 
;  Delete "$TEMP\dxwebsetup.exe"

;  SetOutPath "$INSTDIR"
;SectionEnd

;--------------------------------
;vs redist installer Section

Section "Microsoft Visual C++ 2008 Redistributable Package (x86)" SEC_VCREDIST

  SectionIn 1 2
  
  SetOutPath "$TEMP"
  File "${xbmc_root}\boxee\vcredist_x86.exe"
  DetailPrint "Running VS Redist Setup..."
  ExecWait '"$TEMP\vcredist_x86.exe" /q' $VSRedistSetupError
  DetailPrint "Finished VS Redist Setup"
 
  Delete "$TEMP\vcredist_x86.exe"
 
  SetOutPath "$INSTDIR"
SectionEnd

; -------------------------------
; flash plugin download and install section

;Section "Flash Plugin Install" SEC_FLASH

;   SectionIn 1 2

;   IfFileExists "$SYSDIR\Macromed\Flash\NPSWF32.dll" end

;   StrCpy $1 "install_flash_player.exe" 
;   StrCpy $2 "$TEMP\$1" 
;   DetailPrint "Downloading Flash Plugin..."
;   NSISdl::download_quiet "http://fpdownload.adobe.com/get/flashplayer/current/$1" $2 
;   DetailPrint "Finished Downloading Flash Plugin..."
;   Pop $R0 
;   StrCmp $R0 "success" install 
;   MessageBox MB_OK|MB_ICONSTOP "Error while downloading $1." 
;   Quit 
      
; install: 
;      DetailPrint "Running Flash Plugin Setup..."
;      ExecWait "$2 -install" 
;      DetailPrint "Finished Flash Plugin Setup..."
; end: 
;      DetailPrint "Flash Plugin already installed"

; SetErrorLevel 0

;SectionEnd
