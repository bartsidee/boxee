#!/bin/bash
source ../common.sh
pushd fuse-2.8.5
./configure --host=${HOST} --prefix=${PREFIX}
#make -j6
#sudo make install
popd
