@ECHO OFF
CLS
COLOR 4A
TITLE Boxee for Windows Build Script
rem ----PURPOSE----
rem - Create a working XBMC build with a single click
rem -------------------------------------------------------------
rem Config
rem If you get an error that Visual studio was not found, SET your path for VSNET main executable.
rem ONLY needed if you have a very old bios, SET the path for xbepatch. Not needed otherwise.
rem If Winrar isn't installed under standard programs, SET the path for WinRAR's (freeware) rar.exe
rem and finally set the options for the final rar.
rem -------------------------------------------------------------
rem	CONFIG START
	IF "%VS100COMNTOOLS%"=="" (
	  set NET="%ProgramFiles%\Microsoft Visual Studio 10.0\Common7\IDE\VCExpress.exe"
	) ELSE (
	  set NET="%VS100COMNTOOLS%\..\IDE\VCExpress.exe"
	)
	IF NOT EXIST %NET% (
	  set DIETEXT=Visual Studio .NET 2010 Express was not found.
	  goto DIE
	) 
 
  set OPTS_EXE="..\VS2010Express\BOXEE for Windows.sln" /build "Debug (DirectX)"
	set CLEAN_EXE="..\VS2010Express\BOXEE for Windows.sln" /clean "Debug (DirectX)"
	set EXE= "..\VS2010Express\BOXEE\Debug (DirectX)\BOXEE.exe"
	
  rem	CONFIG END
  rem -------------------------------------------------------------
  
  
ECHO	 MMMMMMMMMMMMMMMMMMMMMMMNmddhhhhddmNMMMMMMMMMMMMMMMMMMMMMMMMM
ECHO	 MMMMMMMMMMMMMMMMMMMMmhyyyyyyyyyyyyyyhmMMMMMMMMMMMMMMMMMMMMMM
ECHO	 MMMMMMMMMMMMMMMMMMdyyyyyyyyyyyyyyyyyyyhmMMMMMMMMMMMMMMMMMMMM
ECHO	 MMMMMMMMMMMMMMMMNhyyyyyyyhddmmddhyyyyyyyhNMMMMMMMMMMMMMMMMMM
ECHO	 MMMMMMMMMMMMMMMNyyyyyyydNMMMMMMMMmdyyyyyyhNMMMMMMMMMMMMMMMMM
ECHO	 MMMMMMMMMMMMMMMhyyyyyhmMMMMMNNMMMMMmyyyyyydMMMMMMMMMMMMMMMMM
ECHO	 MMMMMMMMMMNmdhhyyyyyydMMMMmhyyhNMMMMhyyyyyyhddmNMMMMMMMMMMMM
ECHO	 MMMMMMMNmhyyyyyyyyyyydMMMMdyyyydMMMMdyyyyyyyyyyyhmNMMMMMMMMM
ECHO	 MMMMMMdyyyyyyyyyyyyyyhNMMMMmddmMMMMNhyyyyyyyyyyyyyhmMMMMMMMM
ECHO	 MMMMNhyyyyyyyyyyyyyyyyhNMMMMMMMMMMmhyyyyyyyyyyyyyyyyhNMMMMMM
ECHO	 MMMNhyyyyyyyyyyyyyyyyyyyhdNNMNNmdhyyyyyyyyyyyyyyyyyyyhNMMMMM
ECHO	 MMMdyyyyyyyyyyyyhhyyyyyyyyyyyyyyyyyyyhdNNNmdyyyyyyyyyydMMMMM
ECHO	 MMMhyyyyyyyyyyhNMMNdyyyyyyyyyyyyyyyhmMMMMMMMdyyyyyyyyyhMMMMM
ECHO	 MMNyyyyyyyyyyyhMMMMdyyyyyyyyyyyyyhmMMMMMMMMMmyyyyyyyyyhMMMMM
ECHO	 MMMhyyyyyyyyyyyhddhyyyyyyyyyyyyhmMMMMMMMMMMdyyyyyyyyyydMMMMM
ECHO	 MMMNyyyyyyyyyyyyyyyyyyyyyyyyyhmMMMMMMMMMMdhyyyyyyyyyyhNMMMMM
ECHO	 MMMMmyyyyyyyyyyyyyyyyyyyyyyhmMMMMMMMMMMdhyyyyyyyyyyyhmMMMMMM
ECHO	 MMMMMNhyyyyyyyyyyyyyyyyyyhmMMMMMMMMMMdhyyyyyyyyyyyydNMMMMMMM
ECHO	 MMMMMMMNdyyyyyyyyyyyyyyyyNMMMMMMMMMdhyyyyyyyyyyyyhmMMMMMMMMM
ECHO	 MMMMMMMMMMmdhhyyyyyyyyyyyNMMMMMMNdhyyyyyyyyyyyyhmMMMMMMMMMMM
ECHO	 MMMMMMMMMMMMMMMhyyyyyyyyyhdNMMNdhyyyyyyyyyyyyhmMMMMMMMMMMMMM
ECHO	 MMMMMMMMMMMMMMMNyyyyyyyyyyyyyyyyyyyyyyyyyyyhNMMMMMMMMMMMMMMM
ECHO	 MMMMMMMMMMMMMMMMmhyyyyyyyyyyyyyyyyyyyyyyyhNMMMMMMMMMMMMMMMMM
ECHO	 MMMMMMMMMMMMMMMMMNdyyyyyyyyyyyyyyyyyyyyhNMMMMMMMMMMMMMMMMMMM
ECHO	 MMMMMMMMMMMMMMMMMMMNdhyyyyyyyyyyyyyyhdNMMMMMMMMMMMMMMMMMMMMM
ECHO	 MMMMMMMMMMMMMMMMMMMMMMNddhyyyyyhhdmNMMMMMMMMMMMMMMMMMMMMMMMM
ECHO	 MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
ECHO	 MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
ECHO	 MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
ECHO	 M`    ..-+mMMNs:`  ./yMMd:`oNMMMy.:dM/        mM        -MMM
ECHO	 M`  yyy+  /Md` `+ys/  -Nm/  `oy.  -hM/  /oooooNM  .oooooyMMM
ECHO	 M`  ...` `dM- `NMMMMh  +MMm/    -hMMM/  .....:MM  `.....sMMM
ECHO	 M`  oooo` `m:  dMMMMo  sMMy.    `oNMM/  /oooosMM  .ooooohMMM
ECHO	 M`  +++/` `NN:  .//`  oMh.  -hm/  `sM/  -:::::NM  `:::::+MMM
ECHO	 M++++++++sNMMMdo/::+smMMNo:hMMMMm+oNMs++++++++NM++++++++oMMM	
	
      
  goto EXE_COMPILE
      
:EXE_COMPILE
  rem ---------------------------------------------
  rem	check for existing xbe
  rem ---------------------------------------------
  IF EXIST %EXE% (
    goto EXE_EXIST
  )   
  goto COMPILE_NO_CLEAN_EXE
      
:EXE_EXIST
  ECHO ------------------------------------------------------------
  ECHO Found a previous Compiled WIN32 EXE!
  ECHO [1] a NEW EXE will be compiled for the BUILD_WIN32
  ECHO [2] existing EXE will be updated (quick mode compile) for the BUILD_WIN32
  ECHO ------------------------------------------------------------
  set /P XBMC_COMPILE_ANSWER=Compile a new EXE? [1/2]:
  if /I %XBMC_COMPILE_ANSWER% EQU 1 goto COMPILE_EXE
  if /I %XBMC_COMPILE_ANSWER% EQU 2 goto COMPILE_NO_CLEAN_EXE
      
:COMPILE_EXE
  ECHO Wait while preparing the build.
  ECHO ------------------------------------------------------------
  ECHO Cleaning Solution...
  %NET% %CLEAN_EXE%
  ECHO Compiling Solution...
  %NET% %OPTS_EXE%
  IF NOT EXIST %EXE% (
  	set DIETEXT="BOXEE.EXE failed to build!  See ..\vs2010express\BOXEE\Debug (DirectX)\BuildLog.htm for details."
  	goto DIE
  )   
  ECHO Done!
  ECHO ------------------------------------------------------------
  GOTO MAKE_BUILD_EXE
      
:COMPILE_NO_CLEAN_EXE
  ECHO Wait while preparing the build.
  ECHO ------------------------------------------------------------
  ECHO Compiling Solution...
  %NET% %OPTS_EXE%
  IF NOT EXIST %EXE% (
  	set DIETEXT="BOXEE.EXE failed to build!  See ..\vs2010express\BOXEE\Debug (DirectX)\BuildLog\BuildLog.htm for details."
  	goto DIE
  )
  ECHO Done!
  ECHO ------------------------------------------------------------
  GOTO MAKE_BUILD_EXE

:MAKE_BUILD_EXE
  ECHO Copying files...
  rmdir BUILD_WIN32 /S /Q
  md BUILD_WIN32\Boxee

  Echo .svn>exclude.txt
  Echo .so>>exclude.txt
  Echo Thumbs.db>>exclude.txt
  Echo Desktop.ini>>exclude.txt
  Echo dsstdfx.bin>>exclude.txt
  Echo exclude.txt>>exclude.txt
  rem and exclude potential leftovers
  Echo mediasources.xml>>exclude.txt
  Echo advancedsettings.xml>>exclude.txt
  Echo guisettings.xml>>exclude.txt
  Echo profiles.xml>>exclude.txt
  Echo sources.xml>>exclude.txt
  Echo userdata\cache\>>exclude.txt
  Echo userdata\database\>>exclude.txt
  Echo userdata\playlists\>>exclude.txt
  Echo userdata\script_data\>>exclude.txt
  Echo userdata\thumbnails\>>exclude.txt
  rem UserData\visualisations contains currently only xbox visualisationfiles
  Echo userdata\visualisations\>>exclude.txt
  rem other platform stuff
  Echo lib-osx>>exclude.txt
  Echo players\mplayer>>exclude.txt
  Echo FileZilla Server.xml>>exclude.txt
  Echo asound.conf>>exclude.txt
  Echo voicemasks.xml>>exclude.txt
  Echo Lircmap.xml>>exclude.txt

  xcopy %EXE% BUILD_WIN32\Boxee > NUL
  xcopy ..\..\userdata BUILD_WIN32\Boxee\userdata /E /Q /I /Y /EXCLUDE:exclude.txt > NUL
  copy ..\..\copying.txt BUILD_WIN32\Boxee > NUL
  copy ..\..\LICENSE.GPL BUILD_WIN32\Boxee > NUL
  xcopy dependencies\*.* BUILD_WIN32\Boxee /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  copy sources.xml.in.win BUILD_WIN32\Boxee\userdata > NUL

ECHO Copying files 1...
  
  xcopy ..\..\credits BUILD_WIN32\Boxee\credits /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  xcopy ..\..\language BUILD_WIN32\Boxee\language /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  rem screensavers currently are xbox only
  rem xcopy ..\..\screensavers BUILD_WIN32\Boxee\screensavers /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  xcopy ..\..\visualisations\*_win32.vis BUILD_WIN32\Boxee\visualisations /Q /I /Y /EXCLUDE:exclude.txt > NUL
  xcopy ..\..\visualisations\projectM BUILD_WIN32\Boxee\visualisations\projectM /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  xcopy ..\..\system BUILD_WIN32\Boxee\system /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  xcopy ..\..\plugins BUILD_WIN32\Boxee\plugins /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL

ECHO Copying files 2...
  
  rem In Boxee we need to copy all the media files independently
  xcopy ..\..\media BUILD_WIN32\Boxee\media /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  
  xcopy ..\..\sounds BUILD_WIN32\Boxee\sounds /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  xcopy "..\..\web\Project Mayhem III" BUILD_WIN32\Boxee\web /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  
  IF EXIST config.ini FOR /F "tokens=* DELIMS=" %%a IN ('FINDSTR/R "=" config.ini') DO SET %%a

ECHO Copying files 3...
  
  IF EXIST error.log del error.log > NUL
  call buildboxeeskin.bat %skinpath%
  call buildscripts.bat %scriptpath%
  call buildplugins.bat %pluginpath%
  rem reset variables
  SET skinpath=
  SET scriptpath=
  SET pluginpath=
  rem restore color and title, some scripts mess these up
  COLOR 4A
  TITLE Boxee for Windows Build Script

  IF EXIST exclude.txt del exclude.txt  > NUL
  ECHO ------------------------------------------------------------
  ECHO Build Succeeded!
  GOTO NSIS_EXE

:NSIS_EXE
  ECHO ------------------------------------------------------------
  ECHO Generating installer includes...
  call genNsisIncludesBoxee.bat
  ECHO ------------------------------------------------------------
  FOR /F "Tokens=2* Delims=]" %%R IN ('FIND /v /n "&_&_&_&" "..\..\.svn\entries" ^| FIND "[11]"') DO SET XBMC_REV=%%R
  SET XBMC_SETUPFILE=BoxeeSetup-Rev%XBMC_REV%.exe
  ECHO Creating installer %XBMC_SETUPFILE%...
  IF EXIST %XBMC_SETUPFILE% del %XBMC_SETUPFILE% > NUL
  rem get path to makensis.exe from registry, first try tab delim
  FOR /F "tokens=2* delims=	" %%A IN ('REG QUERY "HKLM\Software\NSIS" /ve') DO SET NSISExePath=%%B
  IF NOT EXIST "%NSISExePath%" (
    rem try with space delim instead of tab
	FOR /F "tokens=2* delims= " %%A IN ('REG QUERY "HKLM\Software\NSIS" /ve') DO SET NSISExePath=%%B
  )
  SET NSISExe=%NSISExePath%\makensis.exe
  "%NSISExe%" /V1 /X"SetCompressor /FINAL lzma" /Dxbmc_root="%CD%\BUILD_WIN32" /Dxbmc_revision="%XBMC_REV%" "BOXEE for Windows.nsi"
  IF NOT EXIST "%XBMC_SETUPFILE%" (
	  set DIETEXT=Failed to create %XBMC_SETUPFILE%.
	  goto DIE
  )
  del BUILD_WIN32\Boxee\userdata\sources.xml > NUL
  ECHO ------------------------------------------------------------
  ECHO Done!
  ECHO Setup is located at %CD%\%XBMC_SETUPFILE%
  ECHO ------------------------------------------------------------
  GOTO VIEWLOG_EXE
  
:DIE
  ECHO ------------------------------------------------------------
  ECHO !-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-
  ECHO    ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR
  ECHO !-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-
  set DIETEXT=ERROR: %DIETEXT%
  echo %DIETEXT%
  ECHO ------------------------------------------------------------

:VIEWLOG_EXE
  IF NOT EXIST "%CD%\..\vs2010express\BOXEE\Debug (DirectX)\BuildLog.htm" goto END
  set /P XBMC_BUILD_ANSWER=View the build log in your HTML browser? [y/n]
  if /I %XBMC_BUILD_ANSWER% NEQ y goto END
  start /D"%CD%\..\vs2010express\BOXEE\Debug (DirectX)\BuildLog.htm"
  goto END

:END
  ECHO Press any key to exit...
  pause > NUL
