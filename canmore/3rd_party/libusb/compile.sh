#!/bin/bash
source ../common.sh
pushd libusb-1.0.8
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
