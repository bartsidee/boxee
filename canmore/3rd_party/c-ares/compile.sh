#!/bin/bash
source ../common.sh
pushd c-ares-1.7.4
./configure --host=${HOST} --prefix=${PREFIX}
#make -j6
#sudo make install
popd
