#!/bin/bash
source ../common.sh
pushd libxml2-2.7.6
./configure --host=${HOST} --prefix=${PREFIX} --without-python
#make -j6
#sudo make install
popd
