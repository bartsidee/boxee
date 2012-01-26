#!/bin/bash
source ../common.sh
pushd fribidi-0.19.2
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
