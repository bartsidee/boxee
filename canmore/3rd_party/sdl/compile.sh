#!/bin/bash
source ../common.sh
pushd SDL-1.2.14
./configure --host=${HOST} --prefix=${PREFIX} --disable-audio --disable-joystick --disable-cdrom --disable-video-x11 --disable-video-photon --disable-video-cocoa --disable-video-fbcon --disable-video-directfb --disable-video-ps2gs --disable-video-svga --disable-video-vgl --disable-video-wscons --disable-video-xbios --disable-video-gem --disable-osmesa-shared
make -j6
#sudo make install
popd
