@ECHO OFF
CLS
COLOR 4A
TITLE Boxee for Windows Build Script
rem ----PURPOSE----
rem - Create a working Boxee build with a single click
rem -------------------------------------------------------------
rem Config
rem If you get an error that Visual studio was not found, SET your path for VSNET main executable.
rem ONLY needed if you have a very old bios, SET the path for xbepatch. Not needed otherwise.
rem If Winrar isn't installed under standard programs, SET the path for WinRAR's (freeware) rar.exe
rem and finally set the options for the final rar.
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


:NSIS_EXE
  ECHO ------------------------------------------------------------
  ECHO Generating installer includes...
  call genNsisIncludesBoxee.bat
  ECHO ------------------------------------------------------------
  FOR /F "Tokens=2* Delims=]" %%R IN ('FIND /v /n "&_&_&_&" "..\..\.svn\entries" ^| FIND "[11]"') DO SET XBMC_REV=%%R
  SET XBMC_SETUPFILE=boxee-0.9.21.%XBMC_REV%.exe
  ECHO Creating installer %XBMC_SETUPFILE%...
  IF EXIST %XBMC_SETUPFILE% del %XBMC_SETUPFILE% > NUL
  rem get path to makensis.exe from registry, first try tab delim
  FOR /F "tokens=2* delims=	" %%A IN ('REG QUERY "HKLM\Software\NSIS" /ve') DO SET NSISExePath=%%B
  echo curr path is %NSISExePath%
  IF NOT EXIST "%NSISExePath%" (
    rem try with space delim instead of tab
	FOR /F "tokens=2* delims= " %%A IN ('REG QUERY "HKLM\Software\NSIS" /ve') DO SET NSISExePath=%%B
  )
echo curr path is %NSISExePath%
  rem x64 - get path to makensis.exe from registry, first try tab delim
  FOR /F "tokens=2* delims=	" %%A IN ('REG QUERY "HKLM\SOFTWARE\Wow6432Node\NSIS" /ve') DO SET NSISExePath=%%B
  IF NOT EXIST "%NSISExePath%" (
    rem x64 - try with space delim instead of tab
	FOR /F "tokens=2* delims= " %%A IN ('REG QUERY "HKLM\SOFTWARE\Wow6432Node\NSIS" /ve') DO SET NSISExePath=%%B
  )
echo curr path is %NSISExePath%
  SET NSISExe=%NSISExePath%\makensis.exe
echo curr exe is %NSISExe%
  "%NSISExe%" /V1 /X"SetCompressor /FINAL lzma" /Dxbmc_root="%CD%\BUILD_WIN32" /Dxbmc_revision="%XBMC_REV%" "BOXEE for Windows.nsi"
  IF NOT EXIST "%XBMC_SETUPFILE%" (
	  set DIETEXT=Failed to create %XBMC_SETUPFILE%.
	  goto DIE
  )
  
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
  IF NOT EXIST "%CD%\..\vs2010express\BOXEE\Release (DirectX)\BuildLog.htm" goto END
  set /P XBMC_BUILD_ANSWER=View the build log in your HTML browser? [y/n]
  if /I %XBMC_BUILD_ANSWER% NEQ y goto END
  start /D"%CD%\..\vs2010express\BOXEE\Release (DirectX)\BuildLog.htm"
  goto END

:END
  ECHO Press any key to exit...
  pause > NUL
