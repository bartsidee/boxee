#!/bin/bash
echo "Building @`date`"
echo `git status | head -1`
mv build-number.txt /tmp
git reset --hard
git clean -d -f -x
mv /tmp/build-number.txt .

export CANMORE_HOME=/opt/canmore
export BUILD_DEST=$CANMORE_HOME/sdk/build_i686/i686-linux-elf/
export DISPLAY=:0

./bootstrap && ./canmore/configure.sh && make -j8 && make release && make -C tools/TexturePacker -f Makefile.host && make -C skin/boxee/media nocompress

if [ "$?" -ne "0" ]; then
  echo "Build failed"
  exit 1
fi

sudo rm -rf /opt/canmore/targetfs/opt
sudo mkdir -p /opt/canmore/targetfs/opt/local
sudo tar xvJf /opt/targetfs-local-1.2.tar.xz -C /opt/canmore/targetfs/opt/local 
sudo ./canmore/install_full.sh

if [ "$?" -ne "0" ]; then
  echo "Build failed"
  exit 1
fi

sudo tar cvjf boxee-ce4100-`cat VERSION`.tar.bz2 -C /opt/canmore/targetfs opt
echo "Successfully created version `cat VERSION`"
