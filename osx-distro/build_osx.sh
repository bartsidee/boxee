#!/bin/sh

cd ..

export PATH=/usr/local/bin:/opt/local/bin:$PATH
echo "PATH is:"
echo $PATH

git reset --hard
git clean -d -f -x -e build-number.txt

./bootstrap
if [ "$?" -ne "0" ]; then
  echo "************************************************************************************"
  echo "bootstrap failed"
  echo "************************************************************************************"
  exit 1
fi
./configure --enable-optimizations --disable-debug
if [ "$?" -ne "0" ]; then
  echo "************************************************************************************"
  echo "configure failed"
  echo "************************************************************************************"
  exit 2
fi


make clean
rm -Rf ./boxee.mpkg
rm -Rf ./Boxee.app
rm *.dmg

make -j4
if [ "$?" -ne "0" ]; then
  echo "************************************************************************************"
  echo "make failed"
  echo "************************************************************************************"
  exit 3
fi
make release
if [ "$?" -ne "0" ]; then
  echo "************************************************************************************"
  echo "make release failed"
  echo "************************************************************************************"
  exit 4
fi
make -C tools/TexturePacker -f Makefile.host_osx
if [ "$?" -ne "0" ]; then
  echo "************************************************************************************"
  echo "make TexturePacker failed"
  echo "************************************************************************************"
  exit 5
fi
make -C skin/boxee/media nocompress
if [ "$?" -ne "0" ]; then
  echo "************************************************************************************"
  echo "make textures failed"
  echo "************************************************************************************"
  exit 6
fi

osx-distro/package.sh
if [ "$?" -ne "0" ]; then
  echo "************************************************************************************"
  echo "Building CreateBoxeeInstaller failed"
  echo "************************************************************************************"
  exit 7
fi

/Developer/usr/bin/packagemaker --no-relocate -d osx-distro/boxee.pmdoc/
if [ "$?" -ne "0" ]; then
  echo "************************************************************************************"
  echo "packagemaker failed"
  echo "************************************************************************************"
  exit 8
fi

VER=`cat ./VERSION`
hdiutil create -srcfolder boxee.mpkg/ boxee-$VER.dmg
if [ "$?" -ne "0" ]; then
  echo "************************************************************************************"
  echo "DMG creation failed"
  echo "************************************************************************************"
  exit 9
fi
