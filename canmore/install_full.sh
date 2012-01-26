#!/bin/bash
if [ ! -f VERSION ]; then
  echo You forgot to do make release
#  exit
fi

. cross_env

SRC=./
DEST=$CANMORE_HOME/targetfs/opt/boxee
STRIP=$CANMORE_HOME/toolchains/i686-cm-linux-strip

echo "Installing to" $DEST

${STRIP} system/libexif-i686-linux.so                                                  
${STRIP} system/python/python24-i686-linux.so                                          
${STRIP} system/libid3tag-i686-linux.so                                                
${STRIP} system/players/dvdplayer/libmpeg2-i686-linux.so                               
${STRIP} system/players/dvdplayer/libdvdnav-i686-linux.so                              
${STRIP} system/players/dvdplayer/libass-i686-linux.so                                 
${STRIP} system/players/dvdplayer/liba52-i686-linux.so                                 
${STRIP} system/players/dvdplayer/swscale-0.6.1-i686-linux.so                          
${STRIP} system/players/dvdplayer/libdts-i686-linux.so                                 
${STRIP} system/players/dvdplayer/avcodec-52-i686-linux.so                             
${STRIP} system/players/dvdplayer/avutil-50-i686-linux.so                              
${STRIP} system/players/dvdplayer/avformat-52-i686-linux.so     
${STRIP} system/players/dvdplayer/avcore-0.16.1-i686-linux.so               
${STRIP} system/players/dvdplayer/libfaad-i686-linux.so                                
${STRIP} system/players/dvdplayer/libao-i686-linux.so  
${STRIP} system/players/dvdplayer/libbluray.so                                
${STRIP} system/players/paplayer/stsoundlibrary-i686-linux.so                          
${STRIP} system/players/paplayer/vgmstream-i686-linux.so                               
${STRIP} system/players/paplayer/wavpack-i686-linux.so                                 
${STRIP} system/players/paplayer/adpcm-i686-linux.so                                   
${STRIP} system/players/paplayer/ac3codec-i686-linux.so                                
${STRIP} system/players/paplayer/MACDll-i686-linux.so                                  
${STRIP} system/players/paplayer/gensapu-i686-linux.so                                 
${STRIP} system/players/paplayer/dumb-i686-linux.so                                    
${STRIP} system/players/paplayer/libsidplay2-i686-linux.so                             
${STRIP} system/players/paplayer/timidity-i686-linux.so                                
${STRIP} system/players/paplayer/nosefart-i686-linux.so                                
${STRIP} system/players/paplayer/SNESAPU-i686-linux.so                                 
${STRIP} system/players/flashplayer/libFlashLib-i686-cm-linux.so                          
${STRIP} system/libboxeebrowser-i686-cm-linux.so.0
${STRIP} system/players/flashplayer/bxflplayer-i686-cm-linux
${STRIP} system/hdhomerun-i686-linux.so                                                
${STRIP} system/ImageLib-i686-linux.so 
${STRIP} Boxee
${STRIP} BoxeeLauncher/BoxeeLauncher

rm -rf ${DEST} 

mkdir -p ${DEST}/
mkdir -p ${DEST}/skin
mkdir -p ${DEST}/system

mkdir -p ${DEST}/language
cp ${SRC}/VERSION ${DEST}/
rsync -rl --exclude=.svn ${SRC}/language/{English,Italian,German,Spanish,French,Hebrew,Swedish,Danish,Dutch,Russian,Turkish,Arabic,Norwegian,Finnish,Czech,Polish,Portuguese\ \(Brazil\)} ${SRC}/language/availablelangs.xml ${DEST}/language
rsync -rl --exclude=.svn ${SRC}/media ${DEST}/
rm ${DEST}/media/Fonts/boxee.ttf
ln -s ../../skin/boxee/Fonts/arial.ttf ${DEST}/media/Fonts/boxee.ttf
rsync -rl --exclude=.svn ${SRC}/skin/boxee ${DEST}/skin
rsync -rl --exclude=.svn ${SRC}/scripts ${DEST}/
rm -rf  ${DEST}/skin/boxee/media/*
# disable startup sound
rm -f ${DEST}/skin/boxee/sounds/startup.wav
cp ${SRC}/skin/boxee/media/{Textures.xbt,textures.xml} ${DEST}/skin/boxee/media
#rm -rf ${DEST}/skin/boxee/media/{*.gif,*.png,Apple\ Movie\ Trailers,busy,flagging,Makefile}

# cleanup media
rm ${DEST}/media/{Boxee.ico,icon.png,test_sound.mp3,xbmc.icns,icon32x32-linux.png,icon32x32.png}

# cleanup scripts
rm -rf ${DEST}/scripts/RTorrent
rm -rf ${DEST}/scripts/Lyrics

mkdir ${DEST}/visualisations
cp ${SRC}/visualisations/opengl_spectrum.vis ${DEST}/visualisations/opengl_spectrum.vis
cp ${SRC}/visualisations/ProjectM.vis ${DEST}/visualisations/ProjectM.vis
mkdir ${DEST}/visualisations/projectM
cp ${SRC}/visualisations/projectM/Geiss\ -\ Tube.milk ${DEST}/visualisations/projectM/

# system
cp ${SRC}/system/*-i686-linux.so  ${DEST}/system
cp ${SRC}/system/libboxeebrowser-i686-cm-linux.so.0  ${DEST}/system
cp ${SRC}/system/*.xml  ${DEST}/system
cp ${SRC}/system/*.txt  ${DEST}/system
cp ${SRC}/system/*.pem  ${DEST}/system
rsync -rl --exclude=.svn ${SRC}/system/keymaps ${DEST}/system/
rsync -rl --exclude=.svn ${SRC}/system/shaders ${DEST}/system/

mkdir ${DEST}/system/python
cp ${SRC}/system/python/python24-i686-linux.so ${DEST}/system/python
rsync -rl --exclude=.svn ${SRC}/system/python/local ${DEST}/system/python/
pushd ${DEST}/system/python/local > /dev/null 2>&1
python2.4 -O -c 'import compileall; 
compileall.compile_dir("'.'", force=1, quiet=1)'
/bin/rm -rf simplejson/_speedups.so simplejson/tests
/bin/rm `find . -name "*.py"`
popd > /dev/null 2>&1

mkdir ${DEST}/system/players

# dvdplayer
mkdir ${DEST}/system/players/dvdplayer
cp ${SRC}/system/players/dvdplayer/*-i686-linux.so ${DEST}/system/players/dvdplayer
cp ${SRC}/system/players/dvdplayer/libbluray.so ${DEST}/system/players/dvdplayer

# paplayer
mkdir ${DEST}/system/players/paplayer
cp ${SRC}/system/players/paplayer/*-i686-linux.so ${DEST}/system/players/paplayer

# flashplayer
mkdir ${DEST}/system/players/flashplayer
cp ${SRC}/system/players/flashplayer/{libFlashLib-i686-cm-linux.so,libbxoverride-i686-cm-linux.so,bxflplayer-i686-cm-linux} ${DEST}/system/players/flashplayer
cp ${SRC}/system/players/flashplayer/{boxee.jpg,loading_animation.gif,preloader.html,loading.png} ${DEST}/system/players/flashplayer
pushd ${DEST}/system/players/flashplayer  > /dev/null 2>&1
popd  > /dev/null 2>&1
mkdir ${DEST}/system/players/flashplayer/plugins
cp ${SRC}/system/players/flashplayer/libflashplayer-i686-cm-linux.so  ${DEST}/system/players/flashplayer/plugins

# scrapers
rsync -rl --exclude=.svn ${SRC}/system/scrapers ${DEST}/system/
mkdir ${DEST}/UserData
cp ${SRC}/UserData/profiles.xml.in ${DEST}/UserData
cp ${SRC}/UserData/sources.xml.in.diff.embedded ${DEST}/UserData
cp ${SRC}/UserData/sources.xml.in.embedded ${DEST}/UserData
cp ${SRC}/UserData/shortcuts.xml.in.embedded ${DEST}/UserData

# hal
mkdir  ${DEST}/hal
cp ${SRC}/BoxeeHal/server/BoxeeHal ${DEST}/hal
cp ${SRC}/BoxeeHal/server/helper/BoxeeHelper ${DEST}/hal
rsync -rl --exclude=.svn ${SRC}/BoxeeHal/server/scripts ${DEST}/hal/

# launcher
cp ${SRC}/BoxeeLauncher/BoxeeLauncher ${DEST}/

# wrapper
cp ${SRC}/BoxeeWrapper/bxwrapper.so ${DEST}/system
cp ${SRC}/MemLeakDetector/leakdetector.so ${DEST}/system

# binary
cp ${SRC}/Boxee ${DEST}/
cp ${SRC}/canmore/run_boxee.sh ${DEST}/
cp ${SRC}/canmore/run_boxee_standalone.sh ${DEST}/
cp ${SRC}/canmore/run_boxee_local.sh ${DEST}/
cp ${SRC}/canmore/run_boxee_memleak.sh ${DEST}/
cp ${SRC}/canmore/get_boxee_resolution.sh ${DEST}/
cp ${SRC}/canmore/set_preferred_tvmode.sh ${DEST}/
cp ${SRC}/canmore/get_hdmi_native/get_hdmi_native ${DEST}/
cp ${SRC}/xbmc/lib/w_scan/w_scan ${DEST}/

# license
rsync -rl --exclude=.svn ${SRC}/license ${DEST}/

# run as root
touch ${DEST}/.run_as_root
