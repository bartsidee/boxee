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

set EXE= "..\VS2010Express\BOXEE\Release (DirectX)\BOXEE.exe"
  
  
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

  GOTO MAKE_BUILD_EXE

:MAKE_BUILD_EXE
  ECHO Copying files...

  ECHO Remove old BUILD_WIN32 directory...
  rmdir BUILD_WIN32 /S /Q

  ECHO Create new BUILD_WIN32\Boxee directory...
  md BUILD_WIN32\Boxee

  ECHO Create exclude file...
  Echo .svn>exclude.txt
  Echo .so>>exclude.txt
  Echo Thumbs.db>>exclude.txt
  Echo Desktop.ini>>exclude.txt
  Echo dsstdfx.bin>>exclude.txt
  Echo exclude.txt>>exclude.txt
  Echo mediasources.xml>>exclude.txt
  Echo advancedsettings.xml>>exclude.txt
  Echo guisettings.xml>>exclude.txt
  Echo profiles.xml>>exclude.txt
  Echo PictureIcon>>exclude.txt
  Echo sources.xml>>exclude.txt
  Echo userdata\cache\>>exclude.txt
  Echo userdata\database\>>exclude.txt
  Echo userdata\playlists\>>exclude.txt
  Echo userdata\script_data\>>exclude.txt
  Echo userdata\thumbnails\>>exclude.txt
  Echo userdata\profiles\>>exclude.txt
  Echo userdata\torrents\>>exclude.txt
  Echo userdata\downloads\>>exclude.txt
  Echo userdata\visualisations\>>exclude.txt
  Echo lib-osx>>exclude.txt
  Echo players\mplayer>>exclude.txt
  Echo FileZilla Server.xml>>exclude.txt
  Echo asound.conf>>exclude.txt
  Echo voicemasks.xml>>exclude.txt
  Echo Lircmap.xml>>exclude.txt
  Echo dxwebsetup.exe>>exclude.txt
  Echo libBoxee.lib>>exclude.txt
  Echo players\flashplayer\sl-osx>>exclude.txt
  Echo players\flashplayer\bxflplayer-osx>>exclude.txt
  Echo players\flashplayer\bxflplayer-linux>>exclude.txt
  Echo players\flashplayer\bxflplayer-i486-linux>>exclude.txt
  Echo players\flashplayer\bxflplayer-x86_64-linux>>exclude.txt
  Echo players\flashplayer\xulrunner\bin>>exclude.txt
  Echo players\flashplayer\xulrunner-i486-linux>>exclude.txt
  Echo players\flashplayer\xulrunner-i486-linux-jaunty>>exclude.txt
  Echo players\flashplayer\xulrunner-x86_64-linux>>exclude.txt
  Echo players\dvdplayer\avcodec-51-osx.a>>exclude.txt
  Echo players\dvdplayer\avformat-51-osx.a>>exclude.txt
  Echo players\dvdplayer\avutil-51-osx.a>>exclude.txt
  Echo players\dvdplayer\postproc-51-osx.a>>exclude.txt
  Echo players\dvdplayer\swscale-51-osx.a>>exclude.txt


  ECHO Copy Boxee.exe
  xcopy %EXE% BUILD_WIN32\Boxee > NUL

  ECHO Copy contents of userdata folder, including sources
  xcopy ..\..\userdata BUILD_WIN32\Boxee\userdata /E /Q /I /Y /EXCLUDE:exclude.txt > NUL
  copy ..\..\userdata\*.win BUILD_WIN32\Boxee\userdata > NUL

  ECHO Copy boxee license...
  xcopy ..\..\license BUILD_WIN32\Boxee\license /E /Q /I /Y /EXCLUDE:exclude.txt > NUL
  
  ECHO Copy dependencies
  xcopy dependencies\*.* BUILD_WIN32\Boxee /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  
  ECHO Copy dependencies\directx
  xcopy dependencies\directx\*.* BUILD_WIN32\Boxee\directx /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  
  ECHO Copy credits...
  xcopy ..\..\credits BUILD_WIN32\Boxee\credits /Q /I /Y /EXCLUDE:exclude.txt  > NUL

  ECHO Copy language...
  xcopy ..\..\language BUILD_WIN32\Boxee\language /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL

  ECHO Copy and rename visualizations...
  xcopy ..\..\visualisations\*_win32dx.vis BUILD_WIN32\Boxee\visualisations /Q /I /Y /EXCLUDE:exclude.txt > NUL
  move BUILD_WIN32\Boxee\visualisations\MilkDrop_win32dx.vis BUILD_WIN32\Boxee\visualisations\MilkDrop.vis
  move BUILD_WIN32\Boxee\visualisations\Waveform_win32dx.vis BUILD_WIN32\Boxee\visualisations\Waveform.vis
  xcopy ..\..\visualisations\Milkdrop BUILD_WIN32\Boxee\visualisations\Milkdrop /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL

  ECHO Copy system and players...
  xcopy ..\..\system BUILD_WIN32\Boxee\system /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL

  ECHO Remove leftover xulrunner directory
  rmdir BUILD_WIN32\Boxee\system\players\flashplayer\xulrunner /S /Q

  ECHO Copy media folder...  
  xcopy ..\..\media BUILD_WIN32\Boxee\media /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL

  ECHO Copy scripts...
  xcopy ..\..\scripts\OpenSubtitles BUILD_WIN32\Boxee\scripts\OpenSubtitles /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  copy ..\..\scripts\autoexec.py BUILD_WIN32\Boxee\scripts > NUL
  
  ECHO Copy sounds...
  xcopy ..\..\sounds BUILD_WIN32\Boxee\sounds /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL

  ECHO Copy web server...
  rem xcopy "..\..\web\Project Mayhem III" BUILD_WIN32\Boxee\web /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  
  IF EXIST config.ini FOR /F "tokens=* DELIMS=" %%a IN ('FINDSTR/R "=" config.ini') DO SET %%a
  
  IF EXIST error.log del error.log > NUL

  rem Reset the exclude file

  Echo .svn>exclude.txt
  Echo .so>>exclude.txt
  Echo Thumbs.db>>exclude.txt
  Echo Desktop.ini>>exclude.txt
  Echo dsstdfx.bin>>exclude.txt
  Echo exclude.txt>>exclude.txt

  ECHO Copy skin files...
  xcopy "..\..\skin\boxee" "BUILD_WIN32\Boxee\skin\boxee" /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL

  rem call buildboxeeskin.bat %skinpath%
  rem call buildboxeescripts.bat %scriptpath%
  rem call buildplugins.bat %pluginpath%

  rem reset variables
  SET skinpath=
  SET scriptpath=
  SET pluginpath=
  rem restore color and title, some scripts mess these up
  COLOR 1B
  TITLE Boxee for Windows Build Script

  IF EXIST exclude.txt del exclude.txt  > NUL
  ECHO ------------------------------------------------------------
  ECHO Build Succeeded!
 
  ECHO Press any key to exit...
  pause > NUL
