#!/bin/bash
source ../common.sh
pushd libiconv-1.13.1
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
