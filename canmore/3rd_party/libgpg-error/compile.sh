#!/bin/bash
source ../common.sh
pushd libgpg-error-1.7
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
