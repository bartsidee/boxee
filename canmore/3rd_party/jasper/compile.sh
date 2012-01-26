#!/bin/bash
source ../common.sh
pushd jasper-1.900.1
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
