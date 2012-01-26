#!/bin/sh

function compile_python 
{
	pushd $1 2>&1 > /dev/null
	python2.4 -O > /dev/null << EOF
import compileall
compileall.compile_dir(".", force=1)
EOF
	popd 2>&1 > /dev/null
}

SRC=.
DEST=./Boxee.app

echo "Installing to" $DEST

if [ ! -f $SRC/VERSION ]; then
  echo You forgot to do make release
#  exit
fi

rm -rf $DEST
mkdir $DEST

# turn to absolute
pushd $SRC  > /dev/null 2>&1
SRC=`pwd`
popd > /dev/null 2>&1

pushd $DEST > /dev/null 2>&1
DEST=`pwd`
popd > /dev/null 2>&1

boxee_ver=`awk -F'"' ' /BOXEE_VERSION/ { print $2 } ' $SRC/xbmc/lib/libBoxee/bxversion.h`

####################################
# Create the Application structure #
####################################

mkdir $DEST/Contents
mkdir $DEST/Contents/Frameworks
mkdir $DEST/Contents/Resources
mkdir $DEST/Contents/MacOS

cp $SRC/osx-distro/Info.plist $DEST/Contents
sed -i"" -e 's/${VERSION}/'$boxee_ver'.'$ver'/g' ${DEST}/Contents/Info.plist
cp $SRC/osx-distro/PkgInfo $DEST/Contents
cp $SRC/osx-distro/version.plist $DEST/Contents
sed -i"" -e 's/${VERSION}/'$boxee_ver'.'$ver'/g' ${DEST}/Contents/version.plist
sed -i"" -e 's/${VER}/'$ver'/g' ${DEST}/Contents/version.plist

#############################
# Copy the Boxee executable #
#############################

cp $SRC/Boxee $DEST/Contents/MacOS
cp $SRC/bin-osx/Dcp* $DEST/Contents/MacOS

############################
# Copy the Boxee Resources #
############################

cp $SRC/osx-distro/BOXEE.icns $DEST/Contents/Resources/Boxee.icns
cp $SRC/osx-distro/BOXEE.rsrc $DEST/Contents/Resources/Boxee.rsrc

BOXEE_RES=$DEST/Contents/Resources/Boxee

mkdir $BOXEE_RES

# license
mkdir $BOXEE_RES/license
cp $SRC/license/*.txt $BOXEE_RES/license
cp $SRC/license/boxee.txt $BOXEE_RES/license/LICENSE.TXT

# bin
mkdir $BOXEE_RES/bin
cp $SRC/bin-osx/boxeeservice $BOXEE_RES/bin
cp $SRC/xbmc/osx/install_SCR $BOXEE_RES/bin

# config
mkdir $BOXEE_RES/config
cp $SRC/osx-distro/launch-agent.template $BOXEE_RES/config
cp $SRC/osx-distro/pre-install.sh $BOXEE_RES/config
cp $SRC/osx-distro/post-install.sh $BOXEE_RES/config
cp $SRC/osx-distro/launch-service.sh $BOXEE_RES/config
cp $SRC/boxee-manage-sources $BOXEE_RES/config

# language
mkdir $BOXEE_RES/language
rsync -rl ${SRC}/language/{English,Italian,German,Spanish,French,Hebrew,Swedish,Danish,Dutch,Russian,Turkish,Arabic,Norwegian,Finnish,Czech,Polish,Portuguese\ \(Brazil\)} ${SRC}/language/availablelangs.xml $BOXEE_RES/language

# media
rsync -rl ${SRC}/media $BOXEE_RES/

# skin
mkdir $BOXEE_RES/skin
rsync -rl ${SRC}/skin/boxee $BOXEE_RES/skin
rm -rf  $BOXEE_RES/skin/boxee/media/*
rm $BOXEE_RES/skin/boxee/sounds/startup.wav
cp ${SRC}/skin/boxee/media/{Textures.xbt,textures.xml} $BOXEE_RES/skin/boxee/media

# scripts
rsync -rl ${SRC}/scripts $BOXEE_RES/
rm -rf $BOXEE_RES/scripts/RTorrent
rm -rf $BOXEE_RES/scripts/Lyrics
compile_python $BOXEE_RES/scripts

# visualizations
mkdir $BOXEE_RES/visualisations
cp ${SRC}/visualisations/opengl_spectrum.vis $BOXEE_RES/visualisations/Spectrum.vis
cp ${SRC}/visualisations/ProjectM.vis $BOXEE_RES/visualisations/ProjectM.vis
cp ${SRC}/visualisations/Waveform.vis $BOXEE_RES/visualisations/Waveform.vis

mkdir $BOXEE_RES/visualisations/projectM
cp ${SRC}/visualisations/projectM/* $BOXEE_RES/visualisations/projectM/

# system
mkdir -p ${BOXEE_RES}/system
mkdir ${BOXEE_RES}/system/players

cp ${SRC}/system/*osx*.so*  ${BOXEE_RES}/system
cp ${SRC}/system/libboxeebrowser-x86-osx.0.dylib  ${BOXEE_RES}/system
cp ${SRC}/system/*.xml  ${BOXEE_RES}/system
cp ${SRC}/system/*.txt  ${BOXEE_RES}/system
cp ${SRC}/system/*.pem  ${BOXEE_RES}/system
rsync -rl ${SRC}/system/keymaps ${BOXEE_RES}/system/
rsync -rl ${SRC}/system/shaders ${BOXEE_RES}/system/

# qt
mkdir -p ${BOXEE_RES}/system/qt
rsync -rl ${SRC}/system/qt/osx ${BOXEE_RES}/system/qt

# dvdplayer
mkdir ${BOXEE_RES}/system/players/dvdplayer
cp ${SRC}/system/players/dvdplayer/*-osx.so ${BOXEE_RES}/system/players/dvdplayer
cp ${SRC}/system/players/dvdplayer/libbluray.so ${BOXEE_RES}/system/players/dvdplayer

# paplayer
mkdir ${BOXEE_RES}/system/players/paplayer
cp ${SRC}/system/players/paplayer/*-osx*.so* ${BOXEE_RES}/system/players/paplayer

# flashplayer
mkdir ${BOXEE_RES}/system/players/flashplayer
cp ${SRC}/system/players/flashplayer/{libFlashLib-x86_64-osx.dylib,bxflplayer-x86-osx} ${BOXEE_RES}/system/players/flashplayer

# scrapers
rsync -rl ${SRC}/system/scrapers ${BOXEE_RES}/system/

# Userdata
mkdir ${BOXEE_RES}/UserData
cp ${SRC}/UserData/profiles.xml.in ${BOXEE_RES}/UserData
cp ${SRC}/UserData/sources.xml.in.diff.osx ${BOXEE_RES}/UserData
cp ${SRC}/UserData/sources.xml.in.osx ${BOXEE_RES}/UserData
cp ${SRC}/UserData/shortcuts.xml.in.osx $BOXEE_RES/UserData

# scrapers
rsync -rl ${SRC}/system/scrapers ${BOXEE_RES}/system/

# python
mkdir ${BOXEE_RES}/system/python
rsync -rl ${SRC}/system/python/local $BOXEE_RES/system/python/
rsync -rl ${SRC}/system/python/spyce $BOXEE_RES/system/python/
rsync -rl ${SRC}/system/python/Lib $BOXEE_RES/system/python/
compile_python $BOXEE_RES/system/python
/bin/rm -rf ${BOXEE_RES}/system/python/local/simplejson/tests
/bin/rm ${BOXEE_RES}/system/python/local/_bxappsec.so
find $BOXEE_RES/system/python -name \*.py -exec rm {} \;

#
# copy all required frameworks from /opt/local
#
FILES=`find $DEST/Contents/MacOS/*`
FILES=$FILES" "`find $DEST/Contents/Resources/Boxee/bin/*`
FILES=$FILES" "`find $DEST -name "*.so"`
FILES=$FILES" "`find $DEST -name "*.so.*"`
FILES=$FILES" "`find $DEST -name "*.dylib" | grep -v Qt | grep -v plugins`
FILES=$FILES" "`find $DEST -name "*.dylib.*" | grep -v Qt | grep -v plugins`

for F in $FILES
do
#	echo "Processing $F..."

	LIBS=`otool -L $F | grep opt | awk '{print $1}'`
	
	for L in $LIBS
	do
		SL=`basename $L`
		
		if [ ! -e $DEST/Contents/Frameworks/$SL ]
		then
#			echo "Copying $SL..."
			cp $L $DEST/Contents/Frameworks/
			chmod 755 $DEST/Contents/Frameworks/$SL
		fi
	done
done

#
# Rename all the dependencies in the binaries
#
FILES=`find $DEST/Contents/MacOS/*`
FILES=$FILES" "`find $DEST/Contents/Resources/Boxee/bin/*`
FILES=$FILES" "`find $DEST -name "*.so"`
FILES=$FILES" "`find $DEST -name "*.so.*"`
FILES=$FILES" "`find $DEST -name "*.dylib" | grep -v Qt`
FILES=$FILES" "`find $DEST -name "*.dylib.*" | grep -v Qt`

for F in $FILES
do
	LIBS=`otool -L $F | grep opt | awk '{print $1}'`
	
	for L in $LIBS
	do
#		echo "Fixing $L in $F..."
		SL=`basename $L`
		install_name_tool -change "$L" @executable_path/../Frameworks/$SL "$F"
	done
done

#
# Prepare all the info files for the setup tool
#
sed -e 's/\$VERSION/'$boxee_ver'.'$ver'/g' $SRC/osx-distro/welcome.rtf > $SRC/osx-distro/welcome-gen.rtf
sed -e 's/\$VERSION/'$boxee_ver'.'$ver'/g' $SRC/osx-distro/info-orig.rtf > $SRC/osx-distro/info.rtf
sed -e 's/${VERSION}/'$boxee_ver'.'$ver'/g' $SRC/osx-distro/boxee.pmdoc/index.xml.in > $SRC/osx-distro/boxee.pmdoc/index.xml
sed -i"" -e 's/${VER}/'$ver'/g' $SRC/osx-distro/boxee.pmdoc/index.xml
sed -e 's/${VERSION}/'$boxee_ver'.'$ver'/g' $SRC/osx-distro/boxee.pmdoc/01boxee.xml.in > $SRC/osx-distro/boxee.pmdoc/01boxee.xml
