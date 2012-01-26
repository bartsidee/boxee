#!/bin/bash
source ../common.sh
pushd libbluray
./bootstrap
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
