#!/bin/bash
source ../common.sh
pushd lzo-2.04
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
