#!/bin/bash
source ../common.sh
pushd bluez-4.87
ALSA_CFLAGS=-I/opt/canmore/IntelCE/usr/include \
ALSA_LIBS="-L/opt/canmore/IntelCE/usr/lib" \
USB_CFLAGS=-I/opt/canmore/local/include \
USB_LIBS="-L/opt/canmore/local/lib -lusb" \
./configure --host=${HOST} --prefix=/opt/local
#make -j6
#sudo make install
popd
