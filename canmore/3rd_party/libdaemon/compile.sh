#!/bin/bash
source ../common.sh
pushd libdaemon-0.14
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
