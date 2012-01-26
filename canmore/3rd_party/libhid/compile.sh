#!/bin/bash
source ../common.sh
pushd libhid-0.2.16
./configure --prefix=${PREFIX} --disable-swig
make -j6
#sudo make install
popd
