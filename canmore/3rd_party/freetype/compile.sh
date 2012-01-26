#!/bin/bash
source ../common.sh
pushd freetype-2.4.4
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
