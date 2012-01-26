#!/bin/bash
source ../common.sh
pushd flac-1.2.1
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
