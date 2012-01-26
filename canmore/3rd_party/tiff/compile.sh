#!/bin/bash
source ../common.sh
pushd tiff-3.9.4
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
