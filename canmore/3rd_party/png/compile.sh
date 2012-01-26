#!/bin/bash
source ../common.sh
pushd libpng-1.4.5
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
