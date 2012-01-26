#!/bin/bash
source ../common.sh
pushd libmms-0.4
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
