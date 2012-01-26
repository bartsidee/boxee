#!/bin/bash
source ../common.sh
pushd usbutils
autoreconf --install --symlink
./configure --host=${HOST} --prefix=${PREFIX} --datadir=/opt/local/share
make -j6 lsusb
sudo cp lsusb /opt/canmore/local/bin
sudo cp usb.ids /opt/canmore/local/share
popd
