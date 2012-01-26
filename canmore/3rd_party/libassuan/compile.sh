#!/bin/bash
source ../common.sh
pushd libassuan-2.0.0
#pushd libassuan-1.0.4
./configure --host=${HOST} --prefix=${PREFIX} 
#--with-pth-prefix=${PREFIX}
#make -j6
#sudo make install
popd
