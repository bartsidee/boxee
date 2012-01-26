#!/bin/bash
source ../common.sh
pushd libogg-1.2.2
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
