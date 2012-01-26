#!/bin/bash

if [[ `whoami` != 'root' ]]
then
   echo "Must run as root!"
   exit
fi

ubuntu=`lsb_release -c | awk '{ print $2; }'`
arch=`uname -a | awk '{ print $12; }'`
if [ ${arch} != "x86_64" ]; then
   arch="i486"
fi


ln -sf control.$arch control
curr_dir=`pwd`

SRC=../
TMP=./tmp
DEST=${TMP}/opt/boxee

/bin/rm -rf ${TMP}
mkdir -p ${DEST}/
mkdir -p ${TMP}/usr/share/applications
mkdir -p ${TMP}/usr/share/pixmaps

# language
mkdir -p ${DEST}/language/
cp -r ${SRC}/language/* ${DEST}/language/

# media
mkdir ${DEST}/media
cp ${SRC}/media/defaultrss.png  ${SRC}/media/downloadrss.png  ${SRC}/media/test_sound.mp3  ${SRC}/media/icon32x32-linux.png ${SRC}/media/icon.png ${DEST}/media
chmod 644 ${DEST}/media/*
mkdir ${DEST}/media/boxee_screen_saver
cp ${SRC}/media/boxee_screen_saver/* ${DEST}/media/boxee_screen_saver
mkdir ${DEST}/media/Fonts
cp ${SRC}/media/Fonts/boxee* ${DEST}/media/Fonts
chmod 644 ${DEST}/media/Fonts/*

# screensavers
mkdir ${DEST}/screensavers
cp ${SRC}/screensavers/Plasma.xbs  ${SRC}/screensavers/Solarwinds.xbs ${DEST}/screensavers

# scripts
mkdir ${DEST}/scripts
# Lyrics
cp -r ${SRC}/scripts/OpenSubtitles ${DEST}/scripts
cp ${SRC}/scripts/autoexec.py ${DEST}/scripts

# plugins
mkdir ${DEST}/plugins
mkdir ${DEST}/plugins/music
mkdir ${DEST}/plugins/pictures
mkdir ${DEST}/plugins/video

# skin
mkdir -p ${DEST}/skin/boxee
cp -r ${SRC}skin/boxee/* ${DEST}/skin/boxee
rm -f ${DEST}/skin/boxee/media/*.png
cp ${SRC}skin/boxee/media/Textures.xbt ${SRC}skin/boxee/media/textures.xml ${DEST}/skin/boxee/media

# system
mkdir ${DEST}/system
cp ${SRC}/system/*-${arch}-linux.so  ${SRC}/system/libboxeebrowser-${arch}-linux.so.0 ${SRC}/system/asound.conf ${SRC}/system/playercorefactory.xml ${SRC}/system/settingsmap.xml  ${DEST}/system
mkdir ${DEST}/system/players

# keymaps
mkdir ${DEST}/system/keymaps
cp ${SRC}/system/keymaps/* ${DEST}/system/keymaps

# shaders
mkdir ${DEST}/system/shaders
cp ${SRC}/system/shaders/* ${DEST}/system/shaders

# dvdplayer
mkdir ${DEST}/system/players/dvdplayer
cp ${SRC}/system/players/dvdplayer/*-${arch}-linux.so ${DEST}/system/players/dvdplayer
# seems like libmpeg2 could be causing us more problems than good. we use ffmpeg.
rm ${DEST}/system/players/dvdplayer/libmpeg2-${arch}-linux.so

# paplayer
mkdir ${DEST}/system/players/paplayer
cp ${SRC}/system/players/paplayer/*-${arch}-linux.so ${DEST}/system/players/paplayer

# flash player
mkdir ${DEST}/system/players/flashplayer
cp ${SRC}/system/players/flashplayer/{bxflplayer-${arch}-linux,libFlashLib-${arch}-linux.so} ${DEST}/system/players/flashplayer
chmod 755 ${DEST}/system/players/flashplayer/*
strip ${DEST}/system/players/flashplayer/*
mkdir ${DEST}/system/qt
cp -R ${SRC}/system/qt/linux ${DEST}/system/qt
strip ${DEST}/system/qt/linux/*.so
mkdir ${DEST}/system/python
cp ${SRC}/system/python/*-${arch}-linux.so ${DEST}/system/python
mkdir ${DEST}/system/python/lib
cp ${SRC}/system/python/Lib/*.so ${DEST}/system/python/lib
cd ${SRC}/xbmc/lib/libPython/Python/Lib
LD_LIBRARY_PATH=../ ../python -O >/dev/null << EOF
import compileall
compileall.compile_dir(".", force=1)
EOF
/bin/rm -rf test
tar cf - `find . -name \*.pyo` | (cd $curr_dir; cd ${DEST}/system/python/lib; tar xf -)
cd $curr_dir
mkdir ${DEST}/system/python/local
cd ${SRC}/system/python
tar cf - `find local -name \*.pyo` `find local -name \*.py` | (cd $curr_dir; cd ${DEST}/system/python; tar xf -)
cd $curr_dir

# scrapers
mkdir ${DEST}/system/scrapers
cp -r ${SRC}/system/scrapers/* ${DEST}/system/scrapers

# userdata
mkdir -p ${DEST}/UserData 
cp ${SRC}system/Lircmap.xml ${DEST}/system
cp ${SRC}system/charsets.xml ${DEST}/system
chmod 644 ${DEST}/system/*.xml
cp ${SRC}UserData/sources.xml.in.linux ${DEST}/UserData
cp ${SRC}UserData/sources.xml.in.diff.linux ${DEST}/UserData
cp ${SRC}UserData/shortcuts.xml.in.linux ${DEST}/UserData
chmod 644 ${DEST}/UserData/*
ln -s UserData ${DEST}/userdata

# visualizations
mkdir ${DEST}/visualisations
cp ${SRC}visualisations/opengl_spectrum.vis  ${SRC}visualisations/ProjectM.vis  ${SRC}visualisations/Waveform.vis ${DEST}/visualisations
cp -r ${SRC}visualisations/projectM  ${SRC}visualisations/projectM.presets ${DEST}/visualisations

# rtorrent
mkdir -p ${DEST}/bin
#cp ${SRC}/bin-linux/boxee-rtorrent ${DEST}/bin
#cp ${SRC}/UserData/rtorrent.rc.linux ${DEST}/UserData/rtorrent.rc

# binary
cp ${SRC}/Boxee ${DEST}/
strip ${DEST}/Boxee
cp ${SRC}/run-boxee-desktop.in ${DEST}/run-boxee-desktop

# desktop stuff
cp boxee.desktop ${TMP}/usr/share/applications
cp boxee.png ${TMP}/usr/share/pixmaps

# remove svn stuff
find tmp -type d -name .svn -exec rm -rf {} \; >/dev/null 2>&1

# update debian control file with version number
mkdir tmp/DEBIAN
VER=`cat ${SRC}/VERSION`
cat control | sed s/VER/${VER}/  > ${TMP}/DEBIAN/control

cp -f postinst ${TMP}/DEBIAN/
chmod 755 ${TMP}/DEBIAN/postinst

cp -f postrm ${TMP}/DEBIAN/
chmod 755 ${TMP}/DEBIAN/postrm

# chown to root it is packaged correctly
chown -R root.root ${TMP}

if [ -f ../VERSION ]
    then
        VER=`cat ../VERSION`
fi
dpkg-deb --build ./tmp boxee-${VER}.${arch}.deb
rm -rf tmp
