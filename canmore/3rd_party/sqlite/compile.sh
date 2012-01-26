#!/bin/bash
source ../common.sh
pushd sqlite-autoconf-3070500
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
