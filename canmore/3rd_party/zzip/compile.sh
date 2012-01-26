#!/bin/bash
source ../common.sh
pushd zziplib-0.13.59
./configure --host=${HOST} --prefix=${PREFIX}
#make -j6
#sudo make install
popd
