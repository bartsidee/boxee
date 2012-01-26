@ECHO OFF
rem ----Usage----
rem BuildSetup [gl|dx] [clean|noclean]
rem gl for opengl build
rem dx for directx build (default)
rem clean to force a full rebuild
rem noclean to force a build without clean
rem exit to exit the console at the end of the compilation
CLS
COLOR 1B
TITLE BOXEE for Windows Build Script
rem ----PURPOSE----
rem - Create a working BOXEE build with a single click
rem -------------------------------------------------------------
rem Config
rem If you get an error that Visual studio was not found, SET your path for VSNET main executable.
rem -------------------------------------------------------------
rem	CONFIG START
SET target=dx
SET exitmode=noexit
rem SET buildmode=ask
SET buildmode=clean
FOR %%b in (%1, %2, %3, %4) DO (
	IF %%b==dx SET target=dx
	IF %%b==gl SET target=gl
	rem IF %%b==clean SET buildmode=clean
	IF %%b==exit SET exitmode=exit
	IF %%b==noclean SET buildmode=noclean
)
SET buildconfig=Release (DirectX)
IF %target%==gl SET buildconfig=Release (OpenGL)

	IF "%VS100COMNTOOLS%"=="" (
	  set NET="%ProgramFiles%\Microsoft Visual Studio 10.0\Common7\IDE\VCExpress.exe"
	) ELSE (
	  set NET="%VS100COMNTOOLS%..\IDE\VCExpress.exe"
	)
	IF NOT EXIST %NET% (
	  set DIETEXT=Visual Studio .NET 2010 Express was not found.
	  goto DIE
	) 
	
	IF %buildmode%==noclean (
	  set OPTS_EXE="..\VS2010Express\BOXEE for Windows.sln" /build "%buildconfig%"
	) ELSE (
	  set OPTS_EXE="..\VS2010Express\BOXEE for Windows.sln" /rebuild "%buildconfig%"
	)
	
	set CLEAN_EXE="..\VS2010Express\BOXEE for Windows.sln" /clean "%buildconfig%"
	set EXE= "..\VS2010Express\BOXEE\%buildconfig%\BOXEE.exe"
	
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
rem goto EXE_COMPILE

  ECHO Getting version...
  "%ProgramFiles%\Git\bin\sh.exe" --login -i < get_version.sh
  SET /p XBMC_REV=<VERSION
  
:EXE_COMPILE
  IF %buildmode%==clean goto COMPILE_EXE
  rem ---------------------------------------------
  rem	check for existing xbe
  rem ---------------------------------------------
  IF EXIST %EXE% (
    goto EXE_EXIST
  )
  goto COMPILE_EXE

:EXE_EXIST
  IF %buildmode%==noclean goto COMPILE_NO_CLEAN_EXE
  ECHO ------------------------------------------------------------
  ECHO Found a previous Compiled WIN32 EXE!
  ECHO [1] a NEW EXE will be compiled for the BUILD_WIN32
  ECHO [2] existing EXE will be updated (quick mode compile) for the BUILD_WIN32
  ECHO ------------------------------------------------------------
  set /P XBMC_COMPILE_ANSWER=Compile a new EXE? [1/2]:
  if /I %XBMC_COMPILE_ANSWER% EQU 1 goto COMPILE_EXE
  if /I %XBMC_COMPILE_ANSWER% EQU 2 goto COMPILE_NO_CLEAN_EXE
  
:COMPILE_EXE
  ECHO Please wait while building the client.
  ECHO ------------------------------------------------------------
  ECHO Cleaning Solution...
  %NET% %CLEAN_EXE%
  IF ERRORLEVEL 1 (
    type "..\vs2010express\BOXEE\%buildconfig%\BOXEE.log"
  	set DIETEXT="BOXEE.EXE failed to clean solution."
  	goto DIE
  )
  
  ECHO Compiling Solution...
  %NET% %OPTS_EXE%
  IF ERRORLEVEL 1 (
    type "..\vs2010express\BOXEE\%buildconfig%\BOXEE.log"
  	set DIETEXT="BOXEE.EXE failed to build!!!"
  	goto DIE
  )
  ECHO Done!
  ECHO ------------------------------------------------------------
  GOTO MAKE_BUILD_EXE
  
:COMPILE_NO_CLEAN_EXE
  ECHO Please wait while preparing the build.
  ECHO ------------------------------------------------------------
  ECHO Compiling Solution...
  %NET% %OPTS_EXE%
  IF ERRORLEVEL 1 (
  	set DIETEXT="BOXEE.EXE failed to build!  See ..\vs2010express\BOXEE\%buildconfig%\BOXEE.log for details."
  	goto DIE
  )
  ECHO Done!
  ECHO ------------------------------------------------------------
  GOTO MAKE_BUILD_EXE

:MAKE_BUILD_EXE
  ECHO Copying files...
  rmdir BUILD_WIN32 /S /Q
  md BUILD_WIN32\Boxee

  rem Echo .svn>exclude.txt
  rem Echo .so>>exclude.txt
  rem Echo Thumbs.db>>exclude.txt
  rem Echo Desktop.ini>>exclude.txt
  rem Echo dsstdfx.bin>>exclude.txt
  rem Echo exclude.txt>>exclude.txt
  rem and exclude potential leftovers
  rem Echo mediasources.xml>>exclude.txt
  rem Echo advancedsettings.xml>>exclude.txt
  rem Echo guisettings.xml>>exclude.txt
  rem Echo profiles.xml>>exclude.txt
  rem Echo sources.xml>>exclude.txt
  rem Echo userdata\cache\>>exclude.txt
  rem Echo userdata\database\>>exclude.txt
  rem Echo userdata\playlists\>>exclude.txt
  rem Echo userdata\script_data\>>exclude.txt
  rem Echo userdata\thumbnails\>>exclude.txt
  rem UserData\visualisations contains currently only xbox visualisationfiles
  rem Echo userdata\visualisations\>>exclude.txt
  rem other platform stuff
  rem Echo lib-osx>>exclude.txt
  rem Echo players\mplayer>>exclude.txt
  rem Echo FileZilla Server.xml>>exclude.txt
  rem Echo asound.conf>>exclude.txt
  rem Echo voicemasks.xml>>exclude.txt
  rem Echo Lircmap.xml>>exclude.txt

  xcopy %EXE% BUILD_WIN32\Boxee > NUL
  xcopy ..\..\userdata BUILD_WIN32\Boxee\userdata /E /Q /I /Y > NUL
  copy ..\..\copying.txt BUILD_WIN32\Boxee > NUL
  copy ..\..\LICENSE.GPL BUILD_WIN32\Boxee > NUL
  copy ..\..\known_issues.txt BUILD_WIN32\Boxee > NUL
  xcopy dependencies\*.* BUILD_WIN32\Boxee /E /Q /I /Y > NUL
  copy sources.xml BUILD_WIN32\Boxee\userdata > NUL
  
  rem xcopy ..\..\credits BUILD_WIN32\Xbmc\credits /Q /I /Y /EXCLUDE:exclude.txt > NUL
  xcopy ..\..\language BUILD_WIN32\Boxee\language /E /Q /I /Y > NUL
  xcopy ..\..\license BUILD_WIN32\Boxee\license /E /Q /I /Y > NUL
  rem screensavers currently are xbox only
  rem xcopy ..\..\screensavers BUILD_WIN32\Xbmc\screensavers /E /Q /I /Y /EXCLUDE:exclude.txt > NUL
  if %target%==gl (
  xcopy ..\..\visualisations\*_win32.vis BUILD_WIN32\Boxee\visualisations /Q /I /Y > NUL
  xcopy ..\..\visualisations\projectM BUILD_WIN32\Boxee\visualisations\projectM /E /Q /I /Y > NUL
  ) else (
    xcopy ..\..\visualisations\*_win32dx.vis BUILD_WIN32\Boxee\visualisations /Q /I /Y > NUL
	xcopy ..\..\visualisations\Milkdrop BUILD_WIN32\Boxee\visualisations\Milkdrop /E /Q /I /Y > NUL
  )
  xcopy ..\..\system BUILD_WIN32\Boxee\system /E /Q /I /Y /EXCLUDE:excluded_files.txt > NUL
  xcopy ..\..\system\qt\win32_release\*.dll BUILD_WIN32\Boxee\system\players\flashplayer /Q /I /Y > NUL
  xcopy ..\..\system\qt\win32_release\plugins BUILD_WIN32\Boxee\system\qt\win32\plugins /E /Q /I /Y > NUL
  xcopy ..\..\media BUILD_WIN32\Boxee\media /E /Q /I /Y > NUL
  xcopy ..\..\skin BUILD_WIN32\Boxee\skin /E /Q /I /Y > NUL
  xcopy ..\..\scripts BUILD_WIN32\Boxee\scripts /E /Q /I /Y > NUL
  rem xcopy ..\..\sounds BUILD_WIN32\Xbmc\sounds /E /Q /I /Y /EXCLUDE:exclude.txt > NUL
  xcopy "..\..\web\Project_Mayhem_III" BUILD_WIN32\Boxee\web /E /Q /I /Y > NUL
  
  SET skinpath=%CD%\Add_skins
  SET scriptpath=%CD%\Add_scripts
  SET pluginpath=%CD%\Add_plugins
  rem override skin/script/pluginpaths from config.ini if there's a config.ini
  IF EXIST config.ini FOR /F "tokens=* DELIMS=" %%a IN ('FINDSTR/R "=" config.ini') DO SET %%a
  
  IF EXIST error.log del error.log > NUL
  call buildskins.bat "%skinpath%"
  call buildscripts.bat "%scriptpath%"
  call buildplugins.bat "%pluginpath%"
  rem reset variables
  SET skinpath=
  SET scriptpath=
  SET pluginpath=
  rem restore color and title, some scripts mess these up
  COLOR 1B
  TITLE BOXEE for Windows Build Script

  IF EXIST exclude.txt del exclude.txt > NUL
  ECHO ------------------------------------------------------------
  ECHO Build Succeeded!
  GOTO NSIS_EXE

:NSIS_EXE
  ECHO ------------------------------------------------------------
  ECHO Generating installer includes...
  call genNsisIncludes.bat
  ECHO ------------------------------------------------------------
  rem SET XBMC_REV=""
  rem FOR /F "Tokens=2* Delims=]" %%R IN ('FIND /v /n "&_&_&_&" "..\..\.svn\entries" ^| FIND "[11]"') DO SET XBMC_REV=%%R
  rem IF %XBMC_REV%=="" SET XBMC_REV=1.2
  
  SET XBMC_SETUPFILE=BoxeeSetup-v%XBMC_REV%.exe
  ECHO Creating installer %XBMC_SETUPFILE%...
  IF EXIST %XBMC_SETUPFILE% del %XBMC_SETUPFILE% > NUL
  rem get path to makensis.exe from registry, first try tab delim
  FOR /F "tokens=2* delims=	" %%A IN ('REG QUERY "HKLM\Software\NSIS" /ve') DO SET NSISExePath=%%B
  IF NOT EXIST "%NSISExePath%" (
    rem try with space delim instead of tab
	FOR /F "tokens=2* delims= " %%A IN ('REG QUERY "HKLM\Software\NSIS" /ve') DO SET NSISExePath=%%B
  )
  IF NOT EXIST "%NSISExePath%" (
    rem extra check for x64
    SET NSISExePath=%ProgramFiles(x86)%\NSIS
  )

  SET NSISExe=%NSISExePath%\makensis.exe
  "%NSISExe%" /V1 /X"SetCompressor /FINAL lzma" /Dxbmc_root="%CD%\BUILD_WIN32" /Dxbmc_revision="%XBMC_REV%" /Dxbmc_target="%target%" "BOXEE for Windows.nsi"
  IF ERRORLEVEL 1 (
	  set DIETEXT=Failed to create %XBMC_SETUPFILE%.
	  goto DIE
  )
  del BUILD_WIN32\Xbmc\userdata\sources.xml > NUL
  ECHO ------------------------------------------------------------
  ECHO Done!
  ECHO Setup is located at %CD%\%XBMC_SETUPFILE%
  ECHO ------------------------------------------------------------
  GOTO END
  
:DIE
  ECHO ------------------------------------------------------------
  ECHO !-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-
  ECHO    ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR
  ECHO !-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-
  set DIETEXT=ERROR: %DIETEXT%
  echo %DIETEXT%
  ECHO ------------------------------------------------------------
  if %exitmode%==exit exit 1

:VIEWLOG_EXE
  IF NOT EXIST "%CD%\..\vs2010express\BOXEE\%buildconfig%\" BOXEE.log" goto END
  rem set /P XBMC_BUILD_ANSWER=View the build log in your HTML browser? [y/n]
  rem if /I %XBMC_BUILD_ANSWER% NEQ y goto END
  type "%CD%\..\vs2010express\BOXEE\%buildconfig%\" BOXEE.log"
  goto END

:END
  ECHO Build finished.
  if %exitmode%==exit exit 0
