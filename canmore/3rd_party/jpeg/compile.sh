#!/bin/bash
source ../common.sh
pushd jpeg-8b
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
