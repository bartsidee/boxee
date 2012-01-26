#!/bin/bash
source ../common.sh
pushd pth-2.0.7
./configure --host=${HOST} --prefix=${PREFIX} --without-python
#make -j6
#sudo make install
popd
