#!/bin/bash
source ../common.sh
pushd libksba-1.0.7
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
