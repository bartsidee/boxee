#!/bin/bash
source ../common.sh
pushd ntp-4.2.4p7
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
