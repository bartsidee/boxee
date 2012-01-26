#!/bin/bash
source ../common.sh
pushd gdbm-1.8.3
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
