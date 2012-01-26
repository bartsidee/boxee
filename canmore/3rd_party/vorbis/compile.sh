#!/bin/bash
source ../common.sh
pushd libvorbis-1.3.2
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
