#!/bin/bash
source ../common.sh
pushd zlib-1.2.5
./configure --prefix=${PREFIX}
make -j6
#sudo make install
popd
