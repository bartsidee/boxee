#!/bin/bash
source ../common.sh
pushd faad2-2.7
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
